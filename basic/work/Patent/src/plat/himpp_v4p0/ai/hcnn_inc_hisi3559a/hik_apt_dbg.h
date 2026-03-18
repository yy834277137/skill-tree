/***************************************************************************************************
* 版权信息：版权所有(c) , 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: hik_apt_dbg.h
* 文件标识: __HIK_APT_DEBUG_H__
* 摘    要: 通用调试信息接口
*
* 当前版本: 0.1.0
* 作    者: 崔莫磊
* 日    期: 2019-06-18
* 备    注: 新建
* 
***************************************************************************************************/
#ifndef __HIK_APT_DEBUG_H__
#define __HIK_APT_DEBUG_H__
#include <stdio.h>
#include <stdint.h>

//-1:None, 0:Basic(CheckError, DUMP, DBG) 1:Error  2:Warning 3:Indicate 4:All(+MSG)
#ifndef HIK_APT_DBG_LEVEL
#define HIK_APT_DBG_LEVEL 1
#endif

#ifndef HIK_APT_DUMP
#if (defined __UITRON)
#include <Debug.h>
#define HIK_APT_DUMP debug_msg
#elif (defined XM4)
extern void debug_msg(const char *fmtstr, ...);
#define HIK_APT_DUMP debug_msg
#else
#define HIK_APT_DUMP printf
#endif
#endif

#if !(defined _WIN32) && !(defined __linux)
//#define printf HIK_APT_DUMP
#endif

#if (HIK_APT_DBG_LEVEL < 0)
#undef HIK_APT_DUMP
#define HIK_APT_DUMP
#endif

#if (HIK_APT_DBG_LEVEL >= 0)
#ifdef __linux
#define HIK_APT_ERR(fmt, args ...) fprintf(stdout, fmt, ##args)
#else
#define HIK_APT_ERR HIK_APT_DUMP
#endif
#endif

#if (HIK_APT_DBG_LEVEL > 1)
#define HIK_APT_WRN HIK_APT_DUMP
#else
#define HIK_APT_WRN
#endif

#if (HIK_APT_DBG_LEVEL > 2)
#define HIK_APT_IND HIK_APT_DUMP
#else
#define HIK_APT_IND
#endif

#if (HIK_APT_DBG_LEVEL > 3)
#define HIK_APT_MSG HIK_APT_DUMP
#else
#define HIK_APT_MSG
#endif

#ifndef HIK_APT_DBG
#ifdef _WIN32
#define HIK_APT_DBG(fmt, ...) HIK_APT_DUMP("%s %d:"fmt, __FILE__, __LINE__, __VA_ARGS__)
#else
#define HIK_APT_DBG(fmt, args ...) HIK_APT_DUMP("%s %d:"fmt, __FILE__, __LINE__, ##args)
#endif
#endif

#if (HIK_APT_DBG_LEVEL < 0)
#define HIK_APT_CHECK_ERR
#else
#ifndef HIK_APT_CHECK_ERR
#if 1
#define HIK_APT_CHECK_ERR(condi, err_id) \
do{ \
    if (condi) \
    { \
        HIK_APT_ERR("ERROR(%x) %s %d:"#condi"\r\n", err_id, __FUNCTION__, __LINE__); \
        return err_id; \
    } \
} while (0)
#else
#define HIK_APT_CHECK_ERR(condi, err_id) \
do{ \
    HIK_APT_DUMP("CHECK_ERR:%s %d\r\n", __FUNCTION__, __LINE__);\
    if (condi) \
    { \
        HIK_APT_ERR("ERROR(%x) %s %d:"#condi"\r\n", err_id, __FUNCTION__, __LINE__); \
        return err_id; \
    } \
} while (0)
#endif
#endif
#endif

#ifndef HIK_APT_CHECK_RETURN
#define HIK_APT_CHECK_RETURN(condi, err_id, ret_val) \
do \
{ \
    if (condi) \
    {                                    \
        HIK_APT_IND("ERROR(%x) %s %d:"#condi"\r\n", err_id, __FUNCTION__, __LINE__); \
        return ret_val; \
    } \
} while (0)
#endif


#ifndef HIK_APT_CHECK_GOTO
#define HIK_APT_CHECK_GOTO(sts, err_code, to) \
    do { \
    if(sts) \
{ \
    ret_val = err_code;\
    HIK_APT_IND("\r\nERROR(%x) %s %d: %s\n", err_code,  __FUNCTION__, __LINE__, #sts); \
    goto to; \
    } \
    } while(0)
#endif

#ifndef HIK_APT_CHECK_ERR_GOTO
#define HIK_APT_CHECK_ERR_GOTO(sts, err_code, to) \
    do { \
    if(sts) \
{ \
    HIK_APT_ERR("\r\nERROR(%x) %s %d: "#sts"\n", err_code,  __FUNCTION__, __LINE__); \
    goto to; \
    } \
    } while(0)
#endif

#ifndef HIK_APT_CHECK_ERR_FMT
#ifdef _WIN32
#define HIK_APT_CHECK_ERR_FMT(sts, err_code, fmt, ...) \
do {\
if (sts) \
{ \
	HIK_APT_ERR("\r\nERROR(%x) %s %d: "fmt"\n", err_code, __FUNCTION__, __LINE__, __VA_ARGS__); \
	return err_code; \
} \
} while (0)
#else
#define HIK_APT_CHECK_ERR_FMT(sts, err_code, fmt, args...) \
do {\
	\
if (sts) \
{ \
	HIK_APT_ERR("\r\nERROR(%x) %s %d: "fmt"\n", err_code, __FUNCTION__, __LINE__, ##args); \
	return err_code; \
} \
} while (0)
#endif
#endif


#ifndef HIK_APT_DUMP_VAL
#define HIK_APT_DUMP_VAL(val, fmt) HIK_APT_ERR("%s %d: "#val" = "fmt"\r\n", __FUNCTION__, __LINE__, val);
#endif

static int32_t HIK_APT_GET_1D_SUM(uint8_t *p_img, int32_t w, int32_t h)
{
    int32_t i = 0;
    int32_t sum = 0;
    for (i = 0; i < w * h; i++)
    {
        sum += p_img[i];
    }
    return sum;
}

static int32_t HIK_APT_GET_2D_SUM(uint8_t *p_img, int32_t w, int32_t h, int32_t stride)
{
    int32_t i = 0;
    int32_t j = 0;
    int32_t sum = 0;
    uint8_t *p_tmp = NULL;
    for (i = 0; i < h; i++)
    {
        p_tmp = p_img + i * stride;
        for (j = 0; j < w; j++)
        {
            sum += p_tmp[j];
        }        
    }
    return sum;
}

#endif
