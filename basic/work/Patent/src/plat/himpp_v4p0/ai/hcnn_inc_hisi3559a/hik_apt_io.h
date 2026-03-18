/***************************************************************************************************
* 版权信息：版权所有(c) , 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: hik_apt_io.h
* 文件标识: __HIK_APT_IO_H__
* 摘    要: 通用文件IO
*
* 当前版本: 0.1.0
* 作    者: 崔莫磊
* 日    期: 2019-06-18
* 备    注: 新建
* 
***************************************************************************************************/
#ifndef __HIK_APT_IO_H__
#define __HIK_APT_IO_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include "hik_apt_dbg.h"
#include "hik_apt_err.h"

#ifndef HIK_APT_CHECK_BREAK
#define HIK_APT_CHECK_BREAK(condi, err_id, ret_val) \
if (condi) \
{                                    \
    HIK_APT_ERR("ERROR(%x) %s %d:"#condi"\r\n", err_id, __FUNCTION__, __LINE__); \
	ret_val = err_id; \
	break; \
} 
#endif	

#ifndef HIK_APT_CHECK_GOTO
#define HIK_APT_CHECK_GOTO(condi, err_id, ret_val, go_label) \
if (condi) \
{                                    \
	HIK_APT_IND("ERROR(%x) %s %d:"#condi"\r\n", err_id, __FUNCTION__, __LINE__); \
	ret_val = err_id; \
	goto label; \
}
#endif	

#ifdef _WIN32
#ifndef va_start
#define va_start _crt_va_start
#endif
#ifndef va_arg
#define va_arg _crt_va_arg
#endif
#ifndef va_end
#define va_end _crt_va_end
#endif
#endif

static int32_t hik_apt_load_data(const char * path, uint8_t *p_data, size_t buf_len, size_t *file_size)
{
	FILE* fp = fopen(path, "rb");
	size_t f_size;
	int32_t ret = HIK_APT_S_OK;
	HIK_APT_CHECK_ERR(fp == NULL, HIK_APT_ERR_NULL_PTR);
	do //equal with try catch
	{
		fseek(fp, 0, SEEK_END);
		f_size = ftell(fp);
        if(f_size > buf_len || f_size <= 0)
        {
            break;
        }
		fseek(fp, 0, SEEK_SET);
		ret = fread(p_data, 1, f_size, fp);
        if(ret <= 0)
        {
            break;
        }

		if (file_size)
		{
			*file_size = f_size;
		}

	} while (0);

	fclose(fp);
	return HIK_APT_S_OK;
}


static int32_t hik_apt_save_data(const char * path, uint8_t *p_data, size_t data_size)
{
	FILE* fp = fopen(path, "wb");
	int32_t ret = HIK_APT_S_OK;

	HIK_APT_CHECK_ERR(fp == NULL, HIK_APT_ERR_NULL_PTR);
	do //equal with try catch
	{
		ret = fwrite(p_data, data_size, 1, fp);
        if(ret <= 0)
        {
            ret = HIK_APT_S_FAIL;
            break;
        }

	} while (0);

	fclose(fp);
	return ret;
}


static char * hik_apt_get_title(const char *fmt, ...)
{
	static char p_str_buf[512];
	va_list args;
	va_start(args, fmt);
#if (defined _WIN32)
	vsprintf_s(p_str_buf, sizeof(p_str_buf), fmt, args);
#else
	vsprintf(p_str_buf, fmt, args);
#endif
	va_end(args);
	return p_str_buf;

}


static char * hik_apt_replace_surfix(const char *name, const char *sur_fix)
{
	static char p_str_buf[512];
	char *p_indx;
	strcpy(p_str_buf, name);
	p_indx = strrchr(p_str_buf, '.');
	if (p_indx)
	{
		*p_indx = 0;
	}
	return strcat(p_str_buf, sur_fix);
}

#define HIK_APT_SAVE_DATA2TXT(path, p_data, row, clom, stride, fmt) \
do{ FILE* ___fp = fopen(path, "wb"); \
if(___fp){ \
    int ___i, ___j; \
    for(___i = 0; ___i < (row); ___i++){ \
        for(___j = 0; ___j < (clom); ___j++){ \
            fprintf(___fp, fmt" ", (p_data)[___i *(stride) + ___j]); \
        } \
        fprintf(___fp, "\r\n"); \
    } \
    fclose(___fp); \
} \
}while(0)


#ifndef HIK_APT_LOG_OUT
static void hik_apt_dump_out(char *format, ...)
{
    FILE *fp;
    va_list args;
    fp = fopen("log.txt", "a");
    if (fp == NULL)
    {
        return;
    }
    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);
    fclose(fp);
}

static uint32_t hik_apt_check_sum(uint8_t *p_data, size_t data_len)
{
    size_t i;
    uint32_t sum = 0;
    for(i = 0; i < data_len; i++)
    {
        sum += p_data[i];
    }
    return sum;
}
#define HIK_APT_LOG_OUT hik_apt_dump_out
#define HIK_APT_LOG_VAL(val, fmt) HIK_APT_LOG_OUT("%s %d:"#val"="fmt"\r\n", __FUNCTION__, __LINE__, val)
#define HIK_APT_LOG_SUM(ptr, len) HIK_APT_LOG_OUT("%s %d:sum("#ptr", %d)=%d\r\n", __FUNCTION__, __LINE__, len, hik_apt_check_sum(ptr, len))
#endif

#endif
