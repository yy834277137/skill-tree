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
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_fop.h"
#include "dspttk_util.h"


/**
 * On success, select() return the number of file descriptors contained in the three returned descriptor sets
 * (that is, the total number of bits that are set in readfds, writefds, exceptfds)
 * which may be zero if the timeout expires before anything interesting happens.
 * On error, -1 is returned, and errno is set appropriately;
 * the sets and timeout become undefined, so do not rely on their contents after an error.
 */


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
UINT32 dspttk_writen(int fd, const void *buf, size_t count)
{
    UINT32 nleft = count, nwritten = 0;
    UINT8 *ptr = (UINT8 *)buf;

    if ((fd < 0) || (NULL == buf) || (0 == count))
    {
        PRT_INFO("fd[%d] OR buf[%p] OR count[%lu] is invalid\n", fd, buf, count);
        return -1;
    }

    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);
        if (-1 == nwritten)
        {
            if (errno == EINTR)
            {
                PRT_INFO("EINTR accurred, try to write again\n");
                continue;
            }
            else
            {
                PRT_INFO("write failed, errno: %d, %s\n", errno, strerror(errno));
                break;
            }
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    return (count - nleft);
}

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
int dspttk_readn(int fd, void *buf, const size_t buf_size)
{
    int nread = 0, try_times = 3;
#if 0 // def DEBUG
    UINT32 i = 0;
#endif

    if ((fd < 0) || (NULL == buf) || (0 == buf_size))
    {
        PRT_INFO("fd[%d] OR buf[%p] OR buf_size[%lu] is invalid\n", fd, buf, buf_size);
        return -1;
    }

    while (try_times > 0)
    {
        nread = read(fd, buf, buf_size);
        if (0 < nread && nread <= buf_size)
        {
#if 0 // def DEBUG
            PRT_DBG("read over, nread: %d...\n", nread);
            for (i = 0; i < nread; i++)
            {
                PRT_LLY("%02X ", *((CHAR *)buf + i));
            }
            PRT_LLY("\n");
#endif
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                PRT_INFO("EINTR accurred, try[%d] again\n", try_times);
                try_times--;
            }
            else
            {
                PRT_INFO("read failed, ret_val: %d, buf_size: %lu, errno: %d, %s\n", nread, buf_size, errno, strerror(errno));
                nread = 0;
                break;
            }
        }
    }

    return nread;
}

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
SAL_STATUS dspttk_read_file(CHAR *p_file_name, long offset, void *p_data, UINT32 *size)
{
    SAL_STATUS ret_val = SAL_SOK;
    FILE *fp = NULL;
    long file_len = 0;
    UINT32 read_size = 0;

    if (NULL == p_file_name || NULL == p_data || NULL == size)
    {
        PRT_INFO("p_file_name[%p] OR p_data[%p] OR size[%p] is NULL\n", p_file_name, p_data, size);
        return SAL_FAIL;
    }

    if (access(p_file_name, 0) != 0) // 不存在fopen会出现出错误
    {
        PRT_INFO("file %s is non-existent!\n", p_file_name);
        return SAL_FAIL;
    }
    fp = fopen(p_file_name, "r");
    if (NULL != fp)
    {
        fseek(fp, 0, SEEK_END);
        file_len = ftell(fp);
        if (offset >= 0)
        {
            fseek(fp, offset, SEEK_SET);
        }
        else
        {
            fseek(fp, offset, SEEK_END);
        }
        if (file_len <= 0) // 获取到的文件当前读写位置为0或出错，直接输出0并返回
        {
            *size = 0;
            fclose(fp);
            return SAL_SOK;
        }
    }
    else
    {
        PRT_INFO("open %s failed, errno:%d, %s\n", p_file_name, errno, strerror(errno));
        return SAL_FAIL;
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

    ret_val = fread(p_data, sizeof(UINT8), read_size, fp);
    if (read_size == ret_val)
    {
        *size = read_size;
        ret_val = SAL_SOK;
    }
    else
    {
        PRT_INFO("fread '%s' failed, size need to read: %u, ret_val: %u, errno: %d, %s\n",
                 p_file_name, read_size, ret_val, ferror(fp), strerror(ferror(fp)));
        clearerr(fp);
        ret_val = SAL_FAIL;
    }

    fclose(fp);

    return ret_val;
}


/**
 * @fn      dspttk_error_code
 * @brief   获取错误码
 *
 * @param   const char *msg[IN] 需要打印输出的字符串
 *
 * @return  SAL_SOK-读取文件成功，SAL_FAIL-读取失败
 */
void dspttk_error_code(const char *msg)
{
    perror(msg);
}


/**
 * @fn      dspttk_remove_dir_or_file
 * @brief   删除当前文件夹或指定文件
 *
 * @param   p_name[IN] 文件夹名货文件名
 *
 * @return  SAL_SOK-写入文件成功，SAL_FAIL-写入失败
 */
SAL_STATUS dspttk_remove_dir_or_file(CHAR *p_name, RM_ATTR rm_attr)
{
    DIR *dir;
    SAL_STATUS ret_val = SAL_FAIL;
    CHAR sfolder_path[512] = {0};
    CHAR p_name_tmp[64] = {0};/* 执行mv命令后的临时文件夹名 */
    UINT64 time = 0;

    if (NULL == p_name)
    {
        PRT_INFO("p_name[%p] is invalid\n", p_name);
        return SAL_FAIL;
    }

    if((dir = opendir(p_name))== NULL)
    {
        PRT_INFO("error opendir p_name %s\n", p_name);
        return SAL_EINTR;
    }
    else
    {
        if (rm_attr == RM_FILE_ONLY)
        {
            time = dspttk_get_clock_ms();
            snprintf(p_name_tmp, sizeof(p_name_tmp), "%s_bak_%lld", p_name, time);
            /*先重命名后建立新的空文件夹再删除之前的文件夹 */
            snprintf(sfolder_path, sizeof(sfolder_path), "mv %s %s" , p_name, p_name_tmp);
            ret_val = system(sfolder_path);
            if (ret_val == SAL_FAIL)
            {
                dspttk_error_code("mv error");
                return SAL_FAIL;
            }
            if (system("chmod 777 *") == SAL_FAIL)
            {
                dspttk_error_code("chmod 777 *");
                return SAL_FAIL;
            }
            snprintf(sfolder_path, sizeof(sfolder_path), "hik_rm %s -rf &" , p_name_tmp);
            ret_val = system(sfolder_path);
            if (ret_val == SAL_FAIL)
            {
                dspttk_error_code("hik_rm tmp error");
                return SAL_FAIL;
            }
            ret_val = system("hik_rm ./*_bak_* -rf &");
            if (ret_val == SAL_FAIL)
            {
                dspttk_error_code("hik_rm -rf error");
                return SAL_FAIL;
            }
            
        }
    }

    return ret_val;
}

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
SAL_STATUS dspttk_write_file(CHAR *p_file_name, long offset, int whence, void *p_data, UINT32 size)
{
    SAL_STATUS ret_val = SAL_SOK;
    FILE *fp = NULL;
    CHAR fopen_mode[4] = "w";

    if (NULL == p_file_name || NULL == p_data)
    {
        PRT_INFO("p_file_name[%p] OR p_data[%p] is invalid\n", p_file_name, p_data);
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
        if (access(p_file_name, 0) != 0) // 不存在fopen会出现出错误
        {
            PRT_INFO("file %s is non-existent!\n", p_file_name);
            return SAL_FAIL;
        }
    }

    fp = fopen(p_file_name, fopen_mode);
    if (NULL == fp)
    {
        PRT_INFO("open %s failed, mode: %s, errno:%d, %s\n", p_file_name, fopen_mode, errno, strerror(errno));
        return SAL_FAIL;
    }

    if (0 == strcmp(fopen_mode, "r+"))
    {
        ret_val = fseek(fp, offset, whence);
        if (ret_val != 0)
        {
            PRT_INFO("fseek %s failed, offset: %ld, whence: %d, errno: %d, %s\n",
                     p_file_name, offset, whence, errno, strerror(errno));
            fclose(fp);
            return SAL_FAIL;
        }
    }

    ret_val = fwrite(p_data, sizeof(UINT8), size, fp);
    if (size == ret_val) /* 验证输入文件大小是否正确 */
    {
        ret_val = SAL_SOK;
    }
    else
    {
        PRT_INFO("fwrite '%s' failed, size need to write: %u, ret_val: %u, errno: %d, %s\n",
                 p_file_name, size, ret_val, errno, strerror(errno));
        ret_val = SAL_FAIL;
    }

    fflush(fp);
    fclose(fp);
    sync();

    return ret_val;
}

/**
 * @fn		dspttk_get_file_suffixes
 * @brief	获取文件的后缀名，后缀名是以.xxx结尾的
 *
 * @param	filename[IN] 文件名，可以是完整路径
 * @param	suffixes[OUT] 后缀名，比如9131.xml的后缀名为xml
 *
 * @return	>0：后缀名在文件名的起始位置，比如9131.xml返回值5，其他：异常
 */
int dspttk_get_file_suffixes(const CHAR *filename, CHAR *suffixes)
{
    CHAR *p_pos = NULL;
    SAL_STATUS ret_val = SAL_SOK;

    if (NULL == filename)
    {
        PRT_INFO("the filename is NULL\n");
        return -1;
    }

    p_pos = (CHAR *)strrchr(filename, '.');
    if (NULL != p_pos)
    {
        if (NULL != suffixes)
        {
            strncpy(suffixes, p_pos + 1, strlen(filename) + filename - p_pos);
        }
        ret_val = p_pos - filename + 1;
    }
    else
    {
        ret_val = -1;
    }

    return ret_val;
}

/**
 * @fn      dspttk_get_file_size
 * @brief   获取文件大小
 *
 * @param   file_name[IN] 文件名，不在当前路径下的需使用绝对路径
 *
 * @return  On success, the file's size is returned.
 *          On error, -1 is returned.
 */
long dspttk_get_file_size(CHAR *file_name)
{
    long file_len = -1;
    struct stat file_stat;
    FILE *fp = NULL;

    if (NULL == file_name)
    {
        PRT_INFO("file_name is NULL\n");
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
            fclose(fp);
        }
        else
        {
            PRT_INFO("open %s failed, errno:%d, %s\n", file_name, errno, strerror(errno));
        }
    }

    return file_len;
}


/**
 * @fn      dspttk_get_file_num
 * @brief   获取目录下的文件数量
 * 
 * @param   [IN] pPath 目录地址，不在当前路径下的需使用绝对路径
 * 
 * @return  INT32 >=0：文件名数量；<0：获取失败
 */
INT32 dspttk_get_file_num(CHAR *pPath)
{
    INT32 s32Count = 0;
    DIR *pDir = NULL;
    struct dirent *pDirName = NULL;

    if (NULL == pPath)
    {
        PRT_INFO("pPath is NULL\n");
        return -1;
    }

    /* 打开目录 */
    pDir = opendir(pPath);
    if (pDir == NULL)
    {
        PRT_INFO("opendir '%s' failed, errno: %d, %s\n", pPath, errno, strerror(errno));
        return -1;
    }

    /* readdir()必须循环调用，读完目录下所有文件/文件夹，readdir()才会返回NULL */
    while ((pDirName = readdir(pDir)) != NULL)
    {
        if (pDirName->d_type == DT_REG) // 普通文件
        {
            s32Count++;
        }
    }

    closedir(pDir);

    return s32Count;
}


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
INT32 dspttk_get_file_name_list(CHAR *pPath, CHAR **ppRegFile[])
{
    INT32 i = 0, s32Idx = 0, s32Count = 0, s32ScanNum = 0;
    CHAR **ppFileName = NULL;
    struct dirent **ppDirent = NULL;

    if (NULL == pPath)
    {
        PRT_INFO("pPath is NULL\n");
        return -1;
    }

    s32Count = dspttk_get_file_num(pPath);
    if (s32Count <= 0)
    {
        PRT_INFO("%s maybe empty\n", pPath);
        return s32Count;
    }

    ppFileName = (CHAR **)dspttk_array_pointers_alloc(s32Count, 260); // 路径的最大长度为260字节
    if (NULL == ppFileName)
    {
        PRT_INFO("dspttk_array_pointers_alloc failed, DirCnt: %d\n", s32Count);
        return -1;
    }

    s32ScanNum = scandir(pPath, &ppDirent, NULL, alphasort);
    if (s32ScanNum < 0)
    {
        PRT_INFO("scandir %s failed, errno: %d, %s\n", pPath, errno, strerror(errno));
        free(ppFileName);
        return -1;
    }

    if (s32ScanNum > 2) // 跳过.和..这两个目录
    {
        for (i = 0; i < s32ScanNum; i++)
        {
            /* 表明这是一个文件夹 */
            if (DT_REG == ppDirent[i]->d_type && s32Idx < s32Count)
            {
                snprintf(ppFileName[s32Idx++], 260, "%s/%s", pPath, ppDirent[i]->d_name);
            }
        }
    }

    for (i = 0; i < s32ScanNum; i++)
    {
        free(ppDirent[i]);
    }
    free(ppDirent);

    if (s32Idx > 0)
    {
        *ppRegFile = ppFileName;
    }
    else
    {
        free(ppFileName);
    }

    return s32Idx;
}


/**
 * @fn      dspttk_get_dir_num
 * @brief   获取目录下文件夹个数，支持逐级目录遍历
 *
 * @param   [IN] pPath 目录地址，不在当前路径下的需使用绝对路径
 * @param   [IN] s32Hierarchy 最多遍历的层次数，0表示仅当前级目录
 * 
 * @return  INT32 >=0：文件夹数量；<0：获取失败
 */
INT32 dspttk_get_dir_num(CHAR *pPath, INT32 s32Hierarchy)
{
    INT32 i = 0, s32ScanNum = 0, s32Count = 0, s32Ret = 0;
    struct dirent **pDirName = NULL;
    CHAR sDirPath[260] = {0};

    if (pPath == NULL)
    {
        PRT_INFO("pPath is NULL\n");
        return -1;
    }

    /* 扫描文件夹 */
    s32ScanNum = scandir(pPath, &pDirName, NULL, alphasort);
    if (s32ScanNum < 0)
    {
        PRT_INFO("scandir %s failed, errno: %d, %s\n", pPath, errno, strerror(errno));
        return -1;
    }

    if (s32ScanNum > 2) // 跳过.和..这两个目录
    {
        for (i = 0; i < s32ScanNum; i++)
        {
            if (strcmp(pDirName[i]->d_name, ".") == 0 || strcmp(pDirName[i]->d_name, "..") == 0)
            {
                continue; /* 跳过当前目录以及父目录 */
            }

            if (DT_DIR == pDirName[i]->d_type) /* 这是一个文件夹 */
            {
                s32Count++;

                if (s32Hierarchy > 0) // 继续向下遍历
                {
                    snprintf(sDirPath, sizeof(sDirPath), "%s/%s", pPath, pDirName[i]->d_name);
                    s32Ret = dspttk_get_dir_num(sDirPath, s32Hierarchy - 1);
                    if (s32Ret > 0)
                    {
                        s32Count += s32Ret;
                    }
                }
            }
        }
    }

    for (i = 0; i < s32ScanNum; i++) /* pDirName是个链表，按序释放资源 */
    {
        free(pDirName[i]);
    }
    free(pDirName);

    return s32Count;
}


/**
 * @fn      dspttk_get_dir_name_list
 * @brief   获取目录下的文件夹名，支持逐级目录遍历
 * @warning 函数内部有malloc申请内存，使用完成后，ppDir需调用free()释放
 *
 * @param   [IN] pPath 目录地址，不在当前路径下的需使用绝对路径
 * @param   [OUT] ppDir 存储文件夹字符串的指针，输出的文件夹名带pPath前缀，第0个元素为pPath本身
 * @param   [IN] s32Hierarchy 最多遍历的层次数，0表示仅当前目录
 * 
 * @return  INT32 >0：文件夹数量，pPath为目录时也记录在内；<0：获取失败，ppDir非法，无需free()
 */
INT32 dspttk_get_dir_name_list(CHAR *pPath, CHAR **ppDir[], INT32 s32Hierarchy)
{
    INT32 s32ScanNum = 0, s32DirCnt = 0, s32DirIdx = 1, s32HieRes = 0;
    INT32 i = 0, j = 0, k = 0, m = 0, n = 0;
    struct dirent **ppDirent = NULL;
    struct stat stInfo = {0};
    CHAR **ppDirName = NULL, *pDirName = NULL;

    if (pPath == NULL)
    {
        PRT_INFO("pPath is NULL\n");
        return -1;
    }

    s32HieRes = SAL_MAX(s32Hierarchy, 0); // 最小为0，不允许为负数
    s32DirCnt = dspttk_get_dir_num(pPath, s32HieRes);
    if (s32DirCnt > 0)
    {
        s32DirCnt++; // 加上pPath目录本身
    }
    else
    {
        if (0 == s32DirCnt && 0 == stat(pPath, &stInfo) && S_ISDIR(stInfo.st_mode))
        {
            s32DirCnt = 1; // 第0个目录为pPath本身
        }
        else
        {
            return -1;
        }
    }

    ppDirName = (CHAR **)dspttk_array_pointers_alloc(s32DirCnt, 260); // 路径的最大长度为260字节
    if (NULL != ppDirName)
    {
        strcpy(ppDirName[0], pPath); // 第0个目录为pPath本身
        if (1 == s32DirCnt) // 仅有pPath本身，直接返回
        {
            goto EXIT;
        }
    }
    else
    {
        PRT_INFO("dspttk_array_pointers_alloc failed, DirCnt: %d\n", s32DirCnt);
        return -1;
    }

    do {
        m = j; // 上级目录起始
        n = SAL_MIN(j + s32DirIdx, s32DirCnt); // 上级目录结束
        j = s32DirIdx; // 下一次从该次结束处开始
        for (k = m; k < n; k++)
        {
            pDirName = (k < 0) ? pPath : ppDirName[k];
            s32ScanNum = scandir(pDirName, &ppDirent, NULL, alphasort);
            if (s32ScanNum < 0)
            {
                PRT_INFO("scandir %s failed, errno: %d, %s\n", pDirName, errno, strerror(errno));
                continue;
            }

            if (s32ScanNum > 2) // 跳过.和..这两个目录
            {
                for (i = 0; i < s32ScanNum; i++)
                {
                    if (strcmp(ppDirent[i]->d_name, ".") == 0 || strcmp(ppDirent[i]->d_name, "..") == 0)
                    {
                        continue; /* 跳过当前目录以及父目录 */
                    }

                    /* 表明这是一个文件夹 */
                    if (DT_DIR == ppDirent[i]->d_type)
                    {
                        snprintf(ppDirName[s32DirIdx++], 260, "%s/%s", pDirName, ppDirent[i]->d_name);
                        if (s32DirIdx >= s32DirCnt)
                        {
                            break;
                        }
                    }
                }
            }

            for (i = 0; i < s32ScanNum; i++)
            {
                free(ppDirent[i]);
            }
            free(ppDirent);
            ppDirent = NULL;
        }
    } while (s32HieRes--);

EXIT:
    *ppDir = ppDirName;
    return s32DirIdx;
}

