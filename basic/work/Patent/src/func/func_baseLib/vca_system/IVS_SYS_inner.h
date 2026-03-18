/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: IVS_SYS_inner.h
* 文件标识: HIKVISION_IVS_SYS_INNER_H
* 摘    要: 海康威视智能信息语法实现库内部头文件
*
* 当前版本: 2.2.2
* 作    者: 陈军
* 日    期: 2009年7月13日
* 备    注: MetaData中ID由熵编码改为固定长度编码
*
* 当前版本: 2.2.1
* 作    者: 陈军
* 日    期: 2009年5月13日
* 备    注: (1)在IVS_SYS_PARSE.c中，对涉及到个数的参数(如target_num等)进行
*              大小判断，对超过最大值的数，赋值为0或1.
*           (2)在IVS_SYS_Create.c中，IVS_EVENT_DATA_to_system()函数对
*              是否有报警信息进行判断：	if (p_event_data->alert)
*
* 更新版本: 2.2
* 作    者: 陈军
* 日    期: 2009年5月4日
* 备    注: (1)IVS_lib.h，HIK_RULE_INFO结构体中增加两个填充字节用于对齐
*           (2)IVS_SYS_Create.c，IVS_EVENT_DATA_to_system()函数的
*              重新设置PaddingFlag修改为：
*               bm->buf_start[0] |= padding_flag << 4 ;
*
* 更新版本: 2.1
* 作    者: 陈军
* 日    期: 2009年4月17日
* 备    注: IVS2.0接口更新：
*           (1) 去除目前IVS库不支持功能(人数统计，逆行)相关的参数。
*           (2) 去除目前没有使用的预留参数。
*           (3) 将HIK_RULE,和HIK_RULE_INFO两个结构体中的参数联合体，
*               用typedef进行类型定义，减少重复代码。
*           (4) 开放IVS2.0库中新加入的几个参数。
*
* 更新版本: 2.0
* 作    者: 陈军
* 日    期: 2009年4月15日
* 备    注: 根据新接口声明文件(IVS_lib.h 版本2.0)做了修改
*
* 更新版本: 1.1
* 作    者: 陈军
* 日    期: 2009年3月23日
* 备    注: 将RULE_DATA中的保留字节放入循环内部
*
* 更新版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月12日
* 备    注:
*           
*******************************************************************************
*/

#ifndef _IVS_SYS_INNER_H_
#define _IVS_SYS_INNER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "IVS_SYS_lib.h"
#include "type_def.h"

//#define IVS_PACK_VERSION		0x0203
//#define IVS_PACK_VERSION		0x0300 // 此版本开始支持64条规则
//#define IVS_PACK_VERSION		0x0400 // 此版本开始支持多算法库
#define IVS_PACK_VERSION		0x0401 // 此版本开始支持组合报警
#define IVS_EXTEND_TARGET_NUM_MARK      0x2424 //'$$' //扩展码
#define IVS_EXTEND_TYPE_MARK            0x2323 //'##' //扩展码
#define IVS_EXTEN_TYPE_COLOR            0x2324 //'#$' 颜色扩展
#define IVS_EXTEN_TYPE_RESVERD_ALL      0x2325 // 保留所有reseverd字段信息

// 多算法库版本宏定义
#define MAX_ALERT_NUM            160       //最多同时支持160个报警事件
#define IVS_MULTI_VERSION               0x3000 // 多算法库版本

// 区域接口版本宏定义
#define IVS_MULTI_BLOB                  0x3100 // 区域版本宏定义
/**********************************************************************************
* 宏定义
**********************************************************************************/

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef TRUE
#define  TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MARKER_BIT               1

#define CONST_15_BITS_OPERATOR 0x7FFF

#define CONST_14_BITS_OPERATOR 0x3FFF

#define CONST_13_BITS_OPERATOR 0x1FFF

#define CONST_15_BITS_0X4000   0x4000

#define CONST_15_BITS_MASK     0x7FFF
#define CONST_30_BITS_MASK     0x3FFFFFFF

#define  CHECK_ERROR(state, error_code) \
         if(state)                      \
         {                              \
	         return error_code;         \
         }  

#define IVS_SYS_MAX(a, b)      ((a) > (b) ? (a) : (b))  //!< Macro returning max value
#define IVS_SYS_MIN(a, b)      ((a) < (b) ? (a) : (b))  //!< Macro returning min value	



/*************************************************************************************
* 结构体定义
**************************************************************************************/
//内存管理结构体
typedef struct 
{
    WORD     scale_width;
	WORD     scale_height;

	DWORD	 cnt; 		    // current bitcounter;
	DWORD    byte_buf;	    // current buffer for last written BYTE;
	
	PBYTE    cur_pos;		// position where next byte to write in buffer
	PBYTE    buf_start;		// start of buffer
	DWORD    buf_size;      // buffer total size 
    BYTE     over_flag;     // 溢出标志
}IVS_SYS_BUF_MANAGER;


#ifdef __cplusplus
}
#endif 

#endif