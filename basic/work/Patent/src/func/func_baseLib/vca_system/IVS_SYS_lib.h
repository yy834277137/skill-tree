/**
 * 
 * 版权信息: Copyright (c) 2013, 杭州海康威视数字技术股份有限公司
 * 
 * 文件名称: IVS_SYS_lib.h
 * 文件标识: HIKVISION_IVS_SYS_LIB
 * 摘    要: 海康威视智能信息库头文件
 *
 * 当前版本: 4.2.4
 * 作    者: 余翔
 * 日    期: 2020.8.24
 * 备    注:
 *          1.添加区域目标框结构体，和压缩解压缩接口
 *          2.添加违禁物颜色信息结构体
 *
 * 当前版本: 4.2.3
 * 作    者: 余翔
 * 日    期: 2020.2.19
 * 备    注:
 *          1.添加支持扩展颜色测温信息；
 *          2.添加缓存溢出标志，当内存读取溢出时，返回错误码
 *
 * 当前版本: 4.2.2
 * 作    者: 余翔
 * 日    期: 2020.1.10
 * 备    注:
 *            1.添加鹰眼跟踪红色四角框扩展信息类型
 *            2.添加色块填充信息
 *            3.支持规则com接口保留字节全部压缩
 *            4.添加版本号异常检测
 *            5.信息颜色车速信息
 *
 * 当前版本: 4.2.1
 * 作    者: 余翔
 * 日    期: 2019.8.8
 * 备    注:
 *            1.新增多算法库智能信息压缩解压缩接口，支持目标、规则、报警信息
 *
 * 当前版本: 4.1.7
 * 作    者: 余翔
 * 日    期: 2019.2.28
 * 备    注:
 *            1.对目标信息VCA_TARGET的reserved的6个字节进行保存压缩，用于外部扩展信息使用
 *
 * 当前版本: 4.1.6
 * 作    者: 余翔
 * 日    期: 2018.10.25
 * 备    注:
 *            1.支持目标多种颜色，使用VCA_TARGET中reserved的前4个字节
                reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB
              2.添加字节移位保护，防止崩溃。

 *
 * 当前版本: 4.1.5
 * 作    者: 辛安民
 * 日    期: 2017年12月20号
 * 备    注:
 *            1.支持规则多种颜色，使用VCA_RULE中reserved的前4个字节
               reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB
              2. IVS_RULE_DATA_to_system_com/IVS_RULE_DATA_sys_parse_com 打包及解析接口向前兼容
 *
 * 当前版本: 4.1.4
 * 作    者: 辛安民
 * 日    期: 2017年06月27号
 * 备    注:
 *            1.错误数据导致规则崩溃进行保护。
 *
 * 当前版本: 4.1.3
 * 作    者: 辛安民
 * 日    期: 2017年03月07号
 * 备    注:
 *            1.增加目标及规则框的打包解析新接口，不再依赖于算法库。
 *
 * 当前版本: 4.1.2
 * 作    者: 辛安民
 * 日    期: 2016年11月22号
 * 备    注:
 *            1.更新vca_common.h到1.2.9版本
 *            2.增加对 VCA_TARGET_LIST_V2的打包及解析支持
 *            3.注意IVS_META_DATA_to_system/IVS_META_DATA_sys_parse及IVS_META_DATA_to_system_v2/IVS_META_DATA_sys_parse_v2配对使用最优
 *			  考虑兼容性IVS_META_DATA_to_system_v2封装的数据可以使用IVS_META_DATA_sys_parse解析，返回HIK_IVS_SYS_LIB_E_DATA_OVERFLOW
 *			  此时可以进一步调用IVS_META_DATA_sys_parse_v2；IVS_META_DATA_to_system封装的数据不支持IVS_META_DATA_sys_parse_v2解析。
 *			 
 *
 * 当前版本: 4.1.1
 * 作    者: 许江浩
 * 日    期: 2014年10月17号
 * 备    注: 支持组合报警
 *
 * 当前版本: 4.1.0
 * 作    者: 许江浩
 * 日    期: 2014年08月05号
 * 备    注: 支持多算法库得到库类型
 *
 * 当前版本: 4.0.0
 * 作    者: 许江浩
 * 日    期: 2014年07月23号
 * 备    注: 增加打包新接口
 *
 * 当前版本: 3.1.1
 * 作    者: 辛安民
 * 日    期: 2010年11月17号
 * 备    注: 根据智能组更新后的vca_common.h将整个库重新修改,IVS_META_DATA_sys_parse接口的
 *           第一个参数进行修改之前的版本是VCA_TARGET_LIST，现在修正成VCA_SYS_TARGET_LIST，
 *           后者多了些参数，兼容之前老的打包库，通过解析METADATA中的标记来区分要不要解析
 *           轨迹和PATH
 *
 * 当前版本: 3.1.0
 * 作    者: 黄崇基
 * 日    期: 2010年10月23号
 * 备    注: 增加打包和解包事件列表和异常人脸规则数据的接口
 *
 * 更新版本: 3.0.0
 * 作    者: 黄崇基
 * 日    期: 2010年10月12号
 * 备    注: 根据智能组更新后的vca_common.h将整个库重新修改，IVS和IAS统一成一个库
 *                      
 *
 * 更新版本: 2.3.0
 * 作    者: 黄崇基
 * 日    期: 2010年7月6号
 * 备    注: 增加对IAS数据的封装和解封装支持，仍然调用与IVS一样的接口，但是需要将输入参数结构体指针
 *           转换成对应的结构体指针，例如打包VCA_SYS_TARGET_LIST时，需将TARGET_LIST *的指针强制转换成
 *           VCA_SYS_TARGET_LIST *的指针，HIK_ALERT和HIK_RULE_LIST一样，解包时需反过来操作
 *                    
 * 更新版本: 2.2.4
 * 作    者: 黄崇基
 * 日    期: 2009年10月26日
 * 备    注: 增加人脸识别加密数据的封装和解封装接口
 *
 * 更新版本: 2.2.3
 * 作    者: 陈军
 * 日    期: 2009年8月18日
 * 备    注: 修改IVS_SYS_Create第825行
 *
 * 更新版本: 2.2.2
 * 作    者: 陈军
 * 日    期: 2009年7月13日
 * 备    注: MetaData中ID由熵编码改为固定长度编码
 *
 * 更新版本: 2.2.1
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
 */

#ifndef _HIK_IVS_SYSTEM_LIB_H_
#define _HIK_IVS_SYSTEM_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vca_common.h"
#include "IAS_ABNFACE_lib.h"
#include "IS_privt_define.h"

/* 返回值类型宏定义*/
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
	typedef int HRESULT;
#endif // !_HRESULT_DEFINED


//错误码定义
#define HIK_IVS_SYS_LIB_S_OK                    1     // 成功	    
#define HIK_IVS_SYS_LIB_S_FAIL                  0     // 失败	        
#define HIK_IVS_SYS_LIB_E_PARA_NULL	      0x80000000  // 参数指针为空	    
#define HIK_IVS_SYS_LIB_E_MEM_OVER	      0x80000001  // 内存溢出	
#define HIK_IVS_SYS_LIB_E_SCALE_WH        0x80000002  // ScaleWidth 或ScaleHeight数值超过范围
#define HIK_IVS_SYS_LIB_E_DATA_OVERFLOW   0x80000003  // 数值超过范围

/**	@struct     IVS_SYS_PROC_PARAM
 *	@brief	    库与外部进行数据交换的参数结构体
 */
typedef struct _HKI_IVS_SYS_PROC_PARAM_
{   
	unsigned short scale_width;     //只在封装时用到，宽度方向上缩放比例 (不能大于0x1FFF)
	unsigned short scale_height;    //只在封装时用到，高度方向上缩放比例 (不能大于0x1FFF)
		
	void          *buf;      //封装时，输出数据的缓存；    解封装时，输入数据的缓存
	unsigned int   buf_size; //封装时，输出数据的缓存大小；解封装时，存放输入数据的缓存大小
	unsigned int   len;      //封装时，输出数据的长度；    解封装时，输入数据的实际长度
		
}IVS_SYS_PROC_PARAM;

#define HIK_CARD_SERIAL_NUM_LEN    12    //人脸识别数据的序列号的字节数

/**	@struct     HIK_FACE_IDENTIFICATION
 *	@brief	    人脸识别数据
 */
typedef struct _HIK_FACE_IDENTIFICATION_
{   
	unsigned char  *encrypt_data_buf; //加密数据
	unsigned char   card_serial_num[HIK_CARD_SERIAL_NUM_LEN];   //序列号
	unsigned int    data_len; //加密数据的长度  
	
}HIK_FACE_IDENTIFICATION;

/**	@enum       HIK_IVS_DATATYPE
 *	@brief	    打包解析数据类型
 */
typedef enum _HIK_IVS_DATATYPE_
{
	IVS_TYPE_META_DATA              = 0x0001,   
	IVS_TYPE_EVENT_DATA             = 0x0002,
	IVS_TYPE_RULE_DATA              = 0x0003,   
	IVS_TYPE_EVENT_LIST             = 0x0004,
	IVS_TYPE_FACE_IDENTIFICATION    = 0x0005,   
	IVS_FACE_DETECT_RULE            = 0x0006,
}HIK_IVS_DATATYPE;

//#define INOUT_PACKET_VERSION 0x0100 // 在201408之前的版本都送出0x0100
//#define INOUT_PACKET_VERSION 0x0101 // 64条规则
#define INOUT_PACKET_VERSION 0x0102   // 算法库版本
/**	@struct     HIK_DATA_INOUT_PACKET
 *	@brief	    META DATA 打包解析新接口结构
 */
typedef struct _HIK_IVS_DATA_INOUT_PACKET_
{   
	unsigned short  version;    // 结构版本 当前版本0x0102
	unsigned short  type;       // 打包解析类型 HIK_IVS_DATATYPE;
	unsigned short  num;        // num 结构大小宏定义值
	unsigned short  datalen;    // 数据大小
	unsigned char*  pdata;      // 数据地址
	unsigned char   algo_type;  // 算法库类型
	unsigned char   reserved[3];// 保留

}HIK_IVS_DATA_INOUT_PACKET;


/*循环显示类型范围 ( 0, 0xffff)*/
typedef enum _CIRCLE_SHOW_TYPE_
{
	SHOW_TYPE_NORMAL = 0,   //例如普通智能，行为分析规则框
	SHOW_TYPE_LINEBG_ONE_HALF,      //例如人脸规则框，普通绿色(在用) 1/2
	SHOW_TYPE_LINEBR_ONE_HALF,      //例如人脸目标框，加粗红色(在用) 1/2 
	SHOW_TYPE_LINEBG_TWO_THIRD ,  //例如人脸规则框 ，普通绿色 2/3
	SHOW_TYPE_LINEBR_TWO_THIRD,   //例如人脸目标框，加粗红色 2/3
} CIRCLE_SHOW_TYPE;

/*匹配类型,目前包含2种，精确和普通匹配, 范围 (0x10000, 0xffff0000)*/
typedef enum _MATCH_TYPE_
{
	MATCH_TYPE_NORMAL   =  (1 << 16),       
	MATCH_TYPE_ACCURACY =  (1 << 17),
} MATCH_TYPE;


//目标链表
typedef struct _HIK_VCA_TARGET_LIST_COM_
{
	unsigned int         type; //见CIRCLE_SHOW_TYPE,MATCH_TYPE
	unsigned int         target_num; 
	VCA_TARGET*          p_target;
}HIK_VCA_TARGET_LIST_COM;

//规则链表
typedef struct _HIK_VCA_RULE_LIST_COM_
{
	unsigned int        type; //见CIRCLE_SHOW_TYPE,MATCH_TYPE
	unsigned int        rule_num; 
	VCA_RULE*           p_rule;
}HIK_VCA_RULE_LIST_COM;


/* 支持多算法结构体 */

//扩展目标框结构体
typedef struct 
{
    VCA_TARGET         target;             // 目标框参数
    IS_PRIVT_INFO      privt_info;         // 扩展信息
}HIK_MULT_VCA_TARGET;

//扩展规则框结构体
typedef struct 
{
    VCA_RULE           rule;               // 规则参数
    IS_PRIVT_INFO      privt_info;         // 扩展信息
}HIK_MULT_VCA_RULE;


//扩展报警框结构体
typedef struct 
{
    VCA_ALERT          alert;              // 报警参数
    IS_PRIVT_INFO      privt_info;         // 扩展信息
}HIK_MULT_VCA_ALERT;


//扩展目标框结构体链表
typedef struct
{
    unsigned int            type;
    unsigned int            target_num;
    unsigned int            arith_type;
    HIK_MULT_VCA_TARGET*    p_target;
}HIK_MULT_VCA_TARGET_LIST;


//扩展规则框结构体链表
typedef struct
{
    unsigned int          type;
    unsigned int          rule_num;
    unsigned int          arith_type;
    HIK_MULT_VCA_RULE*    p_rule;
}HIK_MULT_VCA_RULE_LIST;


//扩展报警框结构体链表
typedef struct
{
    unsigned int           type;
    unsigned int           alert_num;//最多同时支持160个报警事件
    unsigned int           arith_type;
    HIK_MULT_VCA_ALERT*    p_alert;
}HIK_MULT_VCA_ALERT_LIST;


// 区域目标框
typedef struct 
{
    VCA_BLOB_BASIC_INFO     target;               // 区域目标参数
    IS_PRIVT_INFO           privt_info;           // 扩展信息
}HIK_TARGET_BLOB;


// 区域目标框链表
typedef struct
{
    unsigned int       type;
    unsigned int       target_num;
    unsigned int       arith_type;
    HIK_TARGET_BLOB*   p_target;
}HIK_TARGET_BLOB_LIST;


/***************************************************
库接口函数声明
****************************************************/



//输出帧内目标信息数组(Meta Data)数据
HRESULT IVS_META_DATA_to_system(VCA_TARGET_LIST     *p_meta_data,
							    IVS_SYS_PROC_PARAM  *param);

//输出报警事件结构体数据
HRESULT IVS_EVENT_DATA_to_system(VCA_ALERT          *p_event_data,
						         IVS_SYS_PROC_PARAM *param);

//输出警戒规则数组结构体数据
HRESULT IVS_RULE_DATA_to_system(VCA_RULE_LIST      *p_rule_data,
							    IVS_SYS_PROC_PARAM *param);

//封装事件信息列表
HRESULT IVS_EVENT_LIST_to_system(VCA_EVT_INFO_LIST *p_evt_list,
							    IVS_SYS_PROC_PARAM *param);

//封装人脸识别数据
HRESULT IVS_FACE_IDENTIFICATION_to_system(HIK_FACE_IDENTIFICATION *face_idt, 
									      IVS_SYS_PROC_PARAM      *param);

//封装异常人脸检测规则参数
HRESULT IVS_FACE_DETECT_RULE_to_system(FACE_DETECT_RULE   *p_face_rule,
							           IVS_SYS_PROC_PARAM *param);

//解析码流，从中获取帧内目标信息数组数据
HRESULT IVS_META_DATA_sys_parse(VCA_TARGET_LIST     *p_meta_data,
							    IVS_SYS_PROC_PARAM  *param);

//解析码流，从中获取报警事件结构体数据
HRESULT IVS_EVENT_DATA_sys_parse(VCA_ALERT           *p_event_data,
						         IVS_SYS_PROC_PARAM  *param);

//输出警戒规则数组结构体数据
HRESULT IVS_RULE_DATA_sys_parse(VCA_RULE_LIST       *p_rule_data,
							    IVS_SYS_PROC_PARAM  *param);

//输出事件列表结构体数据
HRESULT IVS_EVENT_LIST_sys_parse(VCA_EVT_INFO_LIST  *p_evt_list,
							    IVS_SYS_PROC_PARAM  *param);

//解析输出人脸识别数据
HRESULT IVS_FACE_IDENTIFICATION_sys_parse(HIK_FACE_IDENTIFICATION *face_idt,
								          IVS_SYS_PROC_PARAM      *param);
//输出异常人脸检测规则参数
HRESULT IVS_FACE_DETECT_RULE_sys_parse(FACE_DETECT_RULE   *p_face_rule,
							           IVS_SYS_PROC_PARAM *param);

//打包新接口
HRESULT IVS_DATA_to_system(unsigned char *pstdata, IVS_SYS_PROC_PARAM *param);

//解析新接口
HRESULT IVS_DATA_sys_parse(unsigned char *pstdata,IVS_SYS_PROC_PARAM *param, unsigned int nType);


//输出帧内目标信息数组(Meta Data)数据
HRESULT IVS_META_DATA_to_system_v2(VCA_TARGET_LIST_V2     *p_meta_data,
								IVS_SYS_PROC_PARAM  *param);

//解析码流，从中获取帧内目标信息数组数据
HRESULT IVS_META_DATA_sys_parse_v2(VCA_TARGET_LIST_V2     *p_meta_data,
								IVS_SYS_PROC_PARAM  *param);


//封装目标数据
HRESULT IVS_META_DATA_to_system_com(HIK_VCA_TARGET_LIST_COM* p_meta_data, IVS_SYS_PROC_PARAM* param);

//封装规则数据
HRESULT IVS_RULE_DATA_to_system_com(HIK_VCA_RULE_LIST_COM* p_rule_data, IVS_SYS_PROC_PARAM* param);

//解析目标数据
HRESULT IVS_META_DATA_sys_parse_com(HIK_VCA_TARGET_LIST_COM* p_meta_data, IVS_SYS_PROC_PARAM* param);

//解析规则数据
HRESULT IVS_RULE_DATA_sys_parse_com(HIK_VCA_RULE_LIST_COM* p_rule_data, IVS_SYS_PROC_PARAM* param);

/* 支持多算法函数接口 */

//封装目标数据
HRESULT IVS_MULT_META_DATA_to_system(HIK_MULT_VCA_TARGET_LIST* p_meta_data, IVS_SYS_PROC_PARAM* param);

//解析目标数据
HRESULT IVS_MULT_META_DATA_sys_parse(HIK_MULT_VCA_TARGET_LIST* p_meta_data, IVS_SYS_PROC_PARAM* param);


//封装规则数据
HRESULT IVS_MULT_RULE_DATA_to_system(HIK_MULT_VCA_RULE_LIST* p_rule_data, IVS_SYS_PROC_PARAM* param);

//解析规则数据
HRESULT IVS_MULT_RULE_DATA_sys_parse(HIK_MULT_VCA_RULE_LIST* p_rule_data, IVS_SYS_PROC_PARAM* param);

//封装报警数据
HRESULT IVS_MULT_ALERT_DATA_to_system(HIK_MULT_VCA_ALERT_LIST* p_alert_data, IVS_SYS_PROC_PARAM* param);

//解析报警数据
HRESULT IVS_MULT_ALERT_DATA_sys_parse(HIK_MULT_VCA_ALERT_LIST* p_alert_data, IVS_SYS_PROC_PARAM* param);


/* 扩展区域接口 */

//封装目标数据
HRESULT IVS_META_BLOB_DATA_to_system(HIK_TARGET_BLOB_LIST* p_target_data, IVS_SYS_PROC_PARAM* param);

//解析目标数据
HRESULT IVS_META_BLOB_DATA_sys_parse(HIK_TARGET_BLOB_LIST* p_target_data, IVS_SYS_PROC_PARAM* param);


#ifdef __cplusplus
}
#endif 

#endif
