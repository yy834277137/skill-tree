/********************************************************************************
* 
* 版权信息：版权所有 (c) 2008, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：descriptor_parse.c
* 文件标识：HIKDSC
* 摘    要：海康威视私有描述子解析代码
*
* 当前版本：0.3
* 作    者：俞海
* 日    期：2009年2月9日
* 备	注：
* 2008.07.22	初始版本
* 2009.02.09	将原来stream_descriptor修改为basic_descriptor, 
*				另行增加stream_descriptor，主要为rtp应用传输原始流类型和帧号
********************************************************************************/
#ifndef _HIK_DESCRIPTOR_PARSE_H_
#define _HIK_DESCRIPTOR_PARSE_H_

#include "type_dscrpt_common.h"
#define HIKDSC_E_STM_ERR       (-1)

/******************************************************************************
* 功  能：  解析描述子区域（该区域可能包含多个描述子）
* 参  数：  buffer      - 解析缓冲区
*           len         - 解析缓冲区的长度
*   	    stream_info - 流信息结构体
* 返回值：  解析正常时返回descriptor area长度，否则返回错误码
******************************************************************************/
int HIKDSC_parse_descriptor_area(unsigned char *buffer, 
								 unsigned int len, 
								 unsigned int parse_stream_desc, 
								 HIK_STREAM_INFO *stream_info);

#endif