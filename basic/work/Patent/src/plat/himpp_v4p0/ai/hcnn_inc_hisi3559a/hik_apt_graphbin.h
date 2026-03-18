/******************************************************************************
* 
* 版权信息：版权所有 (c) 201X, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：hik_apt_graphbin.h
* 文件标示：__HIK_APT_GRAPHBIN_H__
* 摘    要：从graph bin中解析信息的函数
* 
* 当前版本：1.0
* 作    者：秦川6
* 日    期：2021-01-19
* 备    注：
******************************************************************************/
#ifndef __HIK_APT_GRAPHBIN_H__
#define __HIK_APT_GRAPHBIN_H__
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ads_lib.h"
#include "hik_apt_dbg.h"
#include "hik_apt_err.h"
typedef struct _HIK_APT_GRAPHBIN_HEAD_
{
    char            head_str[4];     //MA固定为"HMAM"字符格式；HIS固定为"HHIM"字符格式
    unsigned int    model_type[2];   //模型功能类型
    unsigned int    version;         //模型版本号
    unsigned int    reserved[28];    //预留并保证16字节对齐
} HIK_APT_GRAPHBIN_HEAD;


/***********************************************************************************************************************
* 功  能: 从文件头中获取版本信息
* 参  数:
*         p_graph_bin_head        -I   graph bin head的指针
*         graph_bin_size          -I   graph bin head的大小
*         p_ver                   -O   版本信息
* 返回值: 状态码
* 备  注: 无
***********************************************************************************************************************/
static int32_t hik_apt_graphbin_get_version(const void *p_graph_bin_head, size_t graph_bin_size, ADS_VERSION_INFO *p_ver)
{
    const HIK_APT_GRAPHBIN_HEAD *bin_head = (HIK_APT_GRAPHBIN_HEAD *)p_graph_bin_head;
    HIK_APT_CHECK_ERR(graph_bin_size != sizeof(HIK_APT_GRAPHBIN_HEAD), HIK_APT_ERR_INVALID_PARAM);
    HIK_APT_CHECK_ERR(p_graph_bin_head == NULL, HIK_APT_ERR_NULL_PTR);
    HIK_APT_CHECK_ERR(p_ver == NULL, HIK_APT_ERR_NULL_PTR);

    memset(p_ver, 0 , sizeof(ADS_VERSION_INFO));
    strncpy(p_ver->name , "GRAPH_BIN", 32);
    
    p_ver->ver_value[0] = (bin_head->version >> 26) & 0x003f;
    p_ver->ver_value[1] = (bin_head->version >> 21) & 0x001f;
    p_ver->ver_value[2] = (bin_head->version >> 16) & 0x001f;
    p_ver->date_value[0] = (bin_head->version >> 9) & 0x007f;
    p_ver->date_value[1] = (bin_head->version >> 5) & 0x000f;
    p_ver->date_value[2] = (bin_head->version >> 0) & 0x001f;

    return HIK_APT_S_OK;
}



#endif //__HIK_APT_GRAPHBIN_H__