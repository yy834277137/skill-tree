/***********************************************************************************************************************
* 
* 版权信息：版权所有 (c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：vca_common.h
* 文件标识：HIK_VCA_COMMOM_H_
* 摘    要：海康威视VCA公共数据结构体声明文件


* 当前版本：1.3.13
* 作    者：童俊艳
* 日    期：2022年4月25日
* 备    注：本版本相对V1.3.12的改动点如下：+
			1、VCA_EVENT_TYPE_EX增加司法行为及督导相关事件类型			 		
			   VCA_RULE_PARAM_SETS 规则参数结构体中新增攀高（PARAM_REACH_HEIGHT）、起身离床（PARAM_GET_UP_V2）、
			   玩手机（PARAM_PLAY_PHONE）、值岗状态参数（PARAM_DUTY_STATUS）等参数结构体
			   [提出者：童超\梁福禄]
			   
* 当前版本：1.3.12
* 作    者：童俊艳
* 日    期：2021年5月26日
* 备    注：本版本相对V1.3.11的改动点如下：+
			1、VCA_SET_CFG_TYPE枚举结构体中新增HVA专用设置配置项：VCA_SET_CFG_DAT_INFO、VCA_SET_CFG_GLOBAL_INFO、VCA_SET_CFG_NODE_INFO			 		
			   VCA_GET_CFG_TYPE 枚举结构体中新增HVA专用获取配置项：VCA_GET_CFG_DAT_INFO、VCA_GET_CFG_GLOBAL_INFO、VCA_GET_CFG_NODE_INFO
			   [提出者：童超\郜巍5]


* 当前版本：1.3.11
* 作    者：童俊艳
* 日    期：2020年11月16日
* 备    注：本版本相对V1.3.10的改动点如下：+
			1、VCA_AUX_AREA_TYPE 枚举结构体中新增区域配置类型 IPS_GET_UP_HEIGH_LOW_BED-0X6004、IPS_REACH_HIGHT_WINDOW-0X6005			 		
			   
			   
* 当前版本：1.3.10
* 作    者：童俊艳
* 日    期：2020年6月24日
* 备    注：本版本相对V1.3.9的改动点如下：+
			1、VCA_SET_CFG_TYPE枚举结构体中新增HVA专用字符串参数设置类型（VCA_SET_CFG_PARAM_INFO -0x9001）；
			   VCA_GET_CFG_TYPE枚举结构体中新增HVA专用字符串参数获取类型（VCA_GET_CFG_PARAM_INFO -0x9001）
			   [提出者：童超]
			
* 当前版本：1.3.9
* 作    者：童俊艳
* 日    期：2020年5月19日
* 备    注：本版本相对V1.3.8的改动点如下：+
			1、VCA_EVENT_TYPE_EX枚举结构体中新增一个CTA事件类型定义：OCR触发抓拍（VCA_CTA_OBJECT_OCR_TRIGGER -0x40004009）。[提出者：陈晓]

* 当前版本：1.3.8
* 作    者：童俊艳
* 日    期：2020年3月17日
* 备    注：本版本相对V1.3.7的改动点如下：+
			1、新增一个金融智能算法IAS库专用事件类型定义：白卡检测(VCA_IAS_WHITE_CARD-0x10080000)。[童昊浩]
			

* 当前版本：1.3.7
* 作    者：童俊艳
* 日    期：2020年2月17日
* 备    注：本版本相对V1.3.6的改动点如下：+
			1、新增两个事件类型定义：区域交叠比异常检测、区域状态变化检测（图像比对），应用在CTA中。[陆韶琦]


* 当前版本：1.3.6
* 作    者：童俊艳
* 日    期：2019年11月6日
* 备    注：本版本相对V1.3.5的改动点如下：+
			1、新增版本信息结构体VCA_LIB_VERSION_V2和获取V2版本信息配置量，V2版本信息利用修正版本号区分基线（0-99）|定制（100-199）|PK版本（200-299）。[张睿轩3]

* 当前版本：1.3.5
* 作    者：童俊艳
* 日    期：2019年10月30日
* 备    注：本版本相对V1.3.4的改动点如下：+
			1、异常状态检测参数PARAM_ABNORMAL_STATE中增加报警置信度阈值设置。此结构体目前仅在CTA中使用。[陈晓]

* 当前版本：1.3.4
* 作    者：童俊艳
* 日    期：2019年10月30日
* 备    注：本版本相对V1.3.3的改动点如下：+
		  1、扩展事件类型VCA_EVENT_TYPE_EX增加通用模板触发成员变量VCA_IED_OBJECT_TRIGGER_ANALYSIS。[童超]
		  2、异常状态检测参数结构体PARAM_ABNORMAL_STATE调整，此结构体目前仅在CTA中使用。[童超]

* 当前版本：1.3.3
* 作    者：童俊艳
* 日    期：2019年7月31日
* 备    注：本版本相对V1.3.2的改动点如下：+
		  1、增加奔跑参数结构体PARAM_RUN_V3，参数联合体VCA_RULE_PARAM_SETS增加PARAM_RUN_V3。[童超/曾钦清]


* 当前版本：1.3.2
* 作    者：童俊艳
* 日    期：2019年4月25日
* 备    注：本版本相对V1.3.1的改动点如下：+
		  1、VCA_EVENT_TYPE_EX中增加VCA_CTA_REGION_OVERLAP_RATIO事件类型，用于表征开放平台通用事件库CTA中的区域交叠比事件。[童超]


* 当前版本：1.3.1
* 作    者：童俊艳
* 日    期：2019年2月25日
* 备    注：本版本相对V1.3.0的改动点如下：+
		  1、增加开放平台异常状态分析、跨线检测、跨线目标统计以及区域目标统计等5种开放平台的功能以及相关参数，增加开放平台JSON配置索引；[童超]


* 当前版本：1.3.0
* 作    者：童俊艳
* 日    期：2018年5月30日
* 备    注：本版本相对V1.2.5的改动点如下：+
          1、增加区域人数、人群骤散、区域人群密度、排队 四个事件 及人群骤散、区域人群密度、排队三个事件的事件参数；[童超]
          2、增加快速移动、人员聚集、剧烈运动 三个已有功能的V2版参数；这三个功能后续在使用过程中必须说明支持的是哪版参数;[童超]
		  3、增加播放器显示控制复用vca_rule\vca_target 中的保留字节的说明；[陈益伟]
		  4、关于版本号的说明：基线版本号为避免混淆，版本号跳过1.2.6-1.2.9，升级为1.3.0。
		     中间有过的版本1.2.6-1.2.9，不作为正式版本。
		     这些中间版本变更的内容中能基线化的需求已吸收入版本1.2.5，不能基线化的如侦测规则PARAM_MD与事件类型VCA_IVS_MD已废弃。
			 
		  
* 当前版本：1.2.5
* 作    者：蔡巍伟
* 日    期：2014年8月7日
* 备    注：修正版本改动如下：+
*        1.增加智能信息算法库ID，区分智能信息的唯一性，具体涉及到的修改如下：
          对于target目标来说：
          ID编码复用目标结构体（VCA_TARGET)中的ID，其高8位用于算法库ID编码，低24位用于目标原有ID编号
          判断目标target的算法库类型，可通过其ID高8位（(target->ID) >> 24）与枚举VCA_LIBS_ID中定义的值确定
		  对于报警的alert信息来说：
		  采用target信息中的ID新来描述该alert是哪个算法库来的
          对于配置的rule_list来说：（需要应用层修改）
          采用rule_num来编码，其高8位用于算法库ID编码，低24位用于规则个数 
          可通过其高8位（(target->ID) >> 24）与枚举VCA_LIBS_ID中定义的值确定

* 当前版本：1.2.4
* 作    者：李林森
* 日    期：2014年7月22日
* 备    注：修正版本改动如下：+
*        1.添加vca_common.h对SMD全规则中64条规则的支持

* 当前版本：1.2.3
* 作    者：全晓臣、李林森
* 日    期：2014年6月1日
* 备    注：修正版本改动如下：+
*        1.增加目标数据来源的类型，给target中的type赋值，用于区分target的生成来源
*        2.增加META_HSR算法库的配置用宏，以及智能索引系统中所使用到的前段打包用GOP头结构

* 当前版本：1.2.2
* 作    者：蔡巍伟、戚红命
* 日    期：2012年11月28日
* 备    注：修正版本改动如下：+
*        1.加入音频帧，帧头结构体
*        2.加入攀高，如厕超时，放风场滞留事件参数结构体
*        3.修改离岗事件参数结构体
*        4.加入起床规则模式
*        5.规则参数联合体修改
*        6.事件类型枚举中添加：如厕超时，放风场滞留两类事件
*        7.获取/配置参数枚举类型中加入ITS专用配置信息
*        8.子处理函数枚举类型，人脸识别相关更新。
*        9.添加了Netra平台使用的内存类型
*       10.对于X86平台仍旧使用3个成员的VCA_MEM_TAB结构体

* 历史版本：1.2.1
* 作    者：戚红命
* 日    期：2012年03月15日
* 备    注：修正版本改动如下：+
*           1.加入用来表示DSP平台类型的枚举类型；
            2.将VCA_VIDEO_HEADER结构体的reserved修改为dsp_plat_type，用来指定DSP平台类型；

* 历史版本：1.2.0
* 作    者：蔡巍伟
* 日    期：2011年05月16日
* 备    注：修正版本改动如下：+
*           1.加入各智能算法库的配置信息枚举类型；

* 历史版本：1.1.0
* 作    者：蔡巍伟
* 日    期：2011年03月31日
* 备    注：修正版本改动如下：+
*           1.event_type中增加：起身检测，攀高检测，离岗检测三类事件类型；
*           2.修改快速移动参数结构体,结构体中新增delay变量, 修改变量run_distance名为speed。
*           3.规则参数联合体中新增客流量统计方向参数，离岗检测报警参数。
*           4.事件参数联合体中新增行为分析信息，并删除原有异常快速移动信息(abrun_info)。

* 历史版本：1.0.0
* 作    者：蔡巍伟
* 日    期：2010年12月23日
* 备    注：修正版本，对部分接口进行抽象及归类，提高算法库的扩展性

* 历史版本：0.0.9
* 作    者：蔡巍伟
* 日    期：2010年10月9日
* 备    注：初始版本
***********************************************************************************************************************/

#ifndef _HIK_VCA_COMMOM_H_
#define _HIK_VCA_COMMOM_H_

#include "vca_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
#define MAX_POLYGON_VERTEX       10       //多边形最大顶点数
#define MAX_RULE_NUM             8        //最多支持八条规则
#define MAX_RULE_NUM_EX          16       //扩展规则，最多支持十六条规则
#define MAX_RULE_NUM_V3          64       //扩展规则，最多支持64条规则(SMD全功能)
#define MAX_UNITE_RULE_NUM       16       //最多支持16条组合规则
#define MAX_RULE_NUM_IN_UNITE    4        //组合规则中最多组合4条单一规则
#define MAX_TARGET_NUM           30       //最多同时监控30个移动目标
#define MAX_SHIELD_NUM           4        //最多四个屏蔽区域
#define MAX_SHIELD_NUM_V2        8        //最多八个屏蔽区域
#define MAX_TARGET_NUM_V2        160      //最多同时监控160个移动目标
#define MAX_PARAM_NUM            200      //最多关键参数个数
#define MAX_AUX_AREA_NUM         16       //辅助区域最大数目
#define CAMERA_HEIGHT_AVAILABLE  1        //标定过程可以用已知的摄像机高度信息
#define CAMERA_ANGLE_AVAILABLE   2        //标定过程可以用已知的摄像机俯仰角度信息
#define CAMERA_HORIZON_AVAILABLE 4        //标定过程可以用已知的地平线信息

	//内存表宏定义
#ifndef _HIK_DSP_APP_
#define HIK_VCA_MTAB_NUM  1
#else
#define HIK_VCA_MTAB_NUM  3
typedef enum _VCA_DSP_PLATFORM_TYPE_
{
	VCA_PLAT_DM6467,   /* DM6467平台 */
	VCA_PLAT_DM6446,   /* DM6446平台 */
	VCA_PLAT_DM6437,   /* DM6437平台 */
	VCA_PLAT_DM648,    /* DM648 平台 */
	VCA_PLAT_DM8127,   /* DM8127平台 */
	VCA_PLAT_DM8148,   /* DM8148平台 */
	VCA_PLAT_DM8168    /* DM6168平台 */
} VCA_DSP_PLATFORM_TYPE;
#endif

/***********************************************************************************************************************
* 结构体定义
* 涉及到浮点型图像坐标，均为归一化图像坐标*
***********************************************************************************************************************/
//YUV420
typedef struct _VCA_YUV_FRAME_
{
	unsigned char *y;           //y数据指针
	unsigned char *u;           //u数据指针
	unsigned char *v;	        //v数据指针
}VCA_YUV_FRAME;

//帧头信息
typedef struct _VCA_VIDEO_HEADER_
{
	unsigned short image_width;        //图像宽度
	unsigned short image_height;       //图像高度
	unsigned short frame_rate;         //帧率(SMD:10-50)
	unsigned short dsp_plat_type;      //DSP 平台类型，X86下不用指定
    VCA_YUV_FORMAT format;             //YUV格式
	VCA_DATE_TIME  time_stamp;         //时标
}VCA_VIDEO_HEADER;

//视频输入帧
typedef struct _VCA_VIDEO_FRAME_
{
	VCA_VIDEO_HEADER   video_header;    //帧头信息
	VCA_YUV_FRAME      yuv_frame;       //YUV数据指针
}VCA_VIDEO_FRAME;

// 设备层输入配置信息
typedef struct _VCA_CONFIG_INFO_
{
	unsigned short view_state;  //视频状态
	unsigned char  reserved[8]; //保留字节
	VCA_DATE_TIME  date_time;
}VCA_CONFIG_INFO;
/***********************************************************************************************************************
*输入音频帧结构体
************************************************************************************************************************/
//音频帧头信息
typedef struct _VCA_AUDIO_HEADER_
{    
	unsigned int   channel_num;        //声道数 
	unsigned int   sample_rate;        //采样率 
	unsigned int   in_data_len;        //每帧输入数据样点数  
	unsigned int   bits_per_sample;    //每个采样点的bit数，公司现有的音频数据都是16bit
	unsigned int   reserved[4];        //保留字节
	VCA_DATE_TIME  time_stamp;         //时标
}VCA_AUDIO_HEADER;

typedef struct _VCA_AUDIO_FRAME_
{
	VCA_AUDIO_HEADER audio_header;     //音频头
	short            *audio_data;      //音频数据，输入数据指针 
}VCA_AUDIO_FRAME;

/***********************************************************************************************************************
*配置信息中与归一化点相关信息结构体定义
***********************************************************************************************************************/
//屏蔽区域
typedef struct _VCA_SHIELD_
{
	unsigned char     ID;            //规则ID，从1开始
	unsigned char     enable;        //是否激活
	unsigned char     reserved[2];   //保留字节
	VCA_POLYGON_F     polygon;       //屏蔽区对应的多边形区域
}VCA_SHIELD;

//屏蔽区链表
typedef struct _VCA_SHIELD_LIST_
{
	unsigned int  shield_num;
	VCA_SHIELD    shield[MAX_SHIELD_NUM];
}VCA_SHIELD_LIST;

typedef struct _VCA_SHIELD_LIST_V2_
{
	unsigned int  shield_num;
	VCA_SHIELD    shield[MAX_SHIELD_NUM_V2];
}VCA_SHIELD_LIST_V2;


/***********************************************************************************************************************
*配置信息中与归一化矩形相关信息结构体定义
***********************************************************************************************************************/
//尺寸类型
typedef enum _VCA_SIZE_MODE_
{
	IMAGE_PIX_MODE,           //根据像素大小设置，单位pixel
	REAL_WORLD_MODE,          //根据实际大小设置，单位m
	DEFAULT_MODE,             //缺省模式
	IMAGE_PIX_AREA_MODE,      //像素面积模式
	REAL_WORLD_AREA_MODE      //实际面积模式
}VCA_SIZE_MODE;

//目标尺寸过滤器结构体
typedef struct _VCA_SIZE_FILTER_
{
	unsigned int    enable;      //是否激活尺寸过滤器
	VCA_SIZE_MODE   mode;        //过滤器类型:       
    VCA_RECT_F      min_rect;    //最小目标框        
    VCA_RECT_F      max_rect;    //最大目标框        
}VCA_SIZE_FILTER;

/***********************************************************************************************************************
*配置信息中规则相关信息结构体定义
***********************************************************************************************************************/

//跨线方向类型
typedef enum _VCA_CROSS_DIRECTION_
{
    BOTH_DIRECTION,    //双向
    LEFT_TO_RIGHT,     //由左至右
    RIGHT_TO_LEFT      //由右至左
}VCA_CROSS_DIRECTION;

//跨线参数
typedef struct _PARAM_TRAVERSE_PLANE_
{
	VCA_CROSS_DIRECTION cross_direction; //跨越方向 
}PARAM_TRAVERSE_PLANE;

//跨线参数
typedef struct _PARAM_REACH_HEIGHT_EX_
{
	VCA_CROSS_DIRECTION cross_direction; //跨越方向 
}PARAM_REACH_HEIGHT_EX;

//攀高检测规则参数
typedef struct _PARAM_REACH_HEIGHT_
{
	VCA_CROSS_DIRECTION      cross_direction;  // 攀高线触碰方向 
	unsigned int			 delay;            // 攀高持续时间 [0, 600]秒
	unsigned int             alert_interval;   // 攀高两次报警时间间隔, 范围: [0, 600]秒
}PARAM_REACH_HEIGHT;

//区域入侵参数
typedef struct _PARAM_INTRUSION_
{
	unsigned int delay; 
}PARAM_INTRUSION;

//徘徊参数
typedef struct _PARAM_LOITER_
{
	unsigned int delay; 
}PARAM_LOITER;

//物品拿取放置参数
typedef struct _PARAM_LEFT_TAKE_
{
	unsigned int delay; 
}PARAM_LEFT_TAKE;

//异物粘贴参数
typedef struct _PARAM_STICK_UP_
{
	unsigned int delay; 
}PARAM_STICK_UP;

//读卡器参数
typedef struct _PARAM_INSTALL_SCANNER_
{
	unsigned int delay; 
}PARAM_INSTALL_SCANNER;

//停车参数
typedef struct _PARAM_PARKING_
{
	unsigned int delay; 
}PARAM_PARKING;

//异常人脸参数
typedef struct _PARAM_ABNORMAL_FACE_
{
	unsigned int delay;
	unsigned int mode;
}PARAM_ABNORMAL_FACE;

//如厕超时参数
typedef struct _PARAM_TOILET_TARRY_
{
	unsigned int delay; 
}PARAM_TOILET_TARRY;

//放风场滞留参数
typedef struct _PARAM_YARD_TARRY_
{
	unsigned int delay;	// 滞留时间阈值 
	unsigned int mode;	// 滞留模式（任意&单人，参考ips_lib.h)
}PARAM_YARD_TARRY;

//操作超时参数
typedef struct _PARAM_OVER_TIME_
{
	unsigned int delay;
}PARAM_OVER_TIME;

//快速移动检测模式
typedef enum _VCA_RUN_MODE_
{
	SINGLE_RUN_MODE,    				        // 单人奔跑模式
	CROWD_RUN_MODE,     						// 多人奔跑模式
}VCA_RUN_MODE;

//快速移动参数
typedef struct _PARAM_RUN_
{
	float         speed;     //速度(0.1~1.0)
	unsigned int  delay;     //报警延迟
	VCA_SIZE_MODE mode;      //参数模式,只支持像素模式和实际模式两种
}PARAM_RUN;

//快速移动参数(支持单人、多人奔跑）
typedef struct _PARAM_RUN_v2_
{
	unsigned int    speed;              // 速度灵敏度，范围:[0,100]
	unsigned int    delay;              // 时间，达到该时间后发出奔跑报警，范围: [0, 600]秒
	VCA_RUN_MODE    run_mode;			// 单人、多人奔跑模式
	VCA_SIZE_MODE   mode;               // 参数模式,只支持像素模式和实际模式两种
}PARAM_RUN_V2;

//快速移动参数(支持奔跑人数配置）
typedef struct _PARAM_RUN_v3_
{
    unsigned short   delay;            // 持续时间，奔跑持续过程达到该时间后发出奔跑报警，范围: [0, 600]秒
	unsigned short   run_num;          // 奔跑报警最少人数要求, 范围:[1, 50]
    unsigned int     reserved[4];      // 保留字节
}PARAM_RUN_V3; 
 
//人员聚集参数
typedef struct _PARAM_HIGH_DENSITY_
{
	float        alert_density;        // 人员聚集报警密度, 范围: [0.1, 1.0]
	unsigned int delay;                // 时间，达到该时间后发出拥挤报警
}PARAM_HIGH_DENSITY; 

//人员聚集参数(支持最少聚集人数）
typedef struct _PARAM_HIGH_DENSITY_V2_
{
	unsigned int   alert_density;       // 人员聚集报警密度, 范围: [0, 100]
	unsigned int   alert_number;        // 人员聚集报警最少人数, 范围: [2, 50]
	unsigned int   alert_interval;      // 人员聚集前后两次报警时间间隔, 范围: [0, 600]秒
	unsigned int   delay;               // 时间，达到该时间后发出拥挤报警,范围: [0, 600]秒
}PARAM_HIGH_DENSITY_V2;   

//剧烈运动模式
typedef enum _VCA_VIOLENT_MODE_
{
    VCA_VIOLENT_VIDEO_MODE,             //纯视频模式
    VCA_VIOLENT_VIDEO_AUDIO_MODE,       //音视频联合模式
    VCA_VIOLENT_AUDIO_MODE              //纯音频模式
}VCA_VIOLENT_MODE;

//剧烈运动参数
typedef struct _PARAM_VIOLENT_
{
	unsigned int        delay;
    VCA_VIOLENT_MODE    mode;
}PARAM_VIOLENT;

//剧烈运动参数
typedef struct _PARAM_VIOLENT_V2_
{
	unsigned int        delay;			// 达到该时间后发出剧烈运动报警，范围: [0, 600]秒
	unsigned int        inteval;		// 剧烈运动前后两次报警时间间隔, 范围: [0, 600]秒
    VCA_VIOLENT_MODE    mode;			// 剧烈运动模式
}PARAM_VIOLENT_V2;

//客流量统计进入方向参数
typedef struct _PARAM_FLOW_COUNTER_
{
	VCA_VECTOR_F direction;
}PARAM_FLOW_COUNTER;

//值岗状态模式
typedef enum _VCA_LEAVE_POS_MODE_
{
	VCA_LEAVE_POSITION_LEAVE_MODE        = 0x01,  //离岗模式
	VCA_LEAVE_POSITION_SLEEP_MODE        = 0x02,  //睡岗模式
	VCA_LEAVE_POSITION_LEAVE_SLEEP_MODE  = 0x04   //离岗睡岗模式
}VCA_LEAVE_POS_MODE;

// 值岗状态报警模式
typedef enum _VCA_LEAVE_ALERT_MODE_
{
	VCA_LEAVE_POSITION_SINGLE_ALERT        = 0x01,  // 单次报警
	VCA_LEAVE_POSITION_INTERVAL_ALERT      = 0x02,  // 间隔报警，隔了一段时间后重复报警
	VCA_LEAVE_POSITION_DUAL_ALERT          = 0x04   // 在事件发生时和结束后分别报警
}VCA_LEAVE_ALERT_MODE;

//值岗人数模式
typedef enum _VCA_LEAVE_POS_PERSON_MODE_
{
    VCA_LEAVE_POSITION_SINGLE_MODE,             //单人值岗模式
    VCA_LEAVE_POSITION_DOUBLE_MODE              //双人值岗模式
}VCA_LEAVE_POS_PERSON_MODE;

//主从通道设置
typedef enum _VCA_LEAVE_POS_CHANNEL_TYPE_
{
    VCA_LEAVE_POSITION_INDEPENDENT,                  //独立通道
    VCA_LEAVE_POSITION_MASTER,                       //主通道
    VCA_LEAVE_POSITION_SLAVE                         //辅通道
}VCA_LEAVE_POS_CHANNEL_TYPE;

//离岗检测报警参数
typedef struct _PARAM_LEAVE_POST_
{
	VCA_LEAVE_POS_MODE         mode;           //离岗状态模式
	unsigned int               leave_delay;    //无人报警时间
	unsigned int               static_delay;   //睡觉报警时间
	VCA_LEAVE_POS_PERSON_MODE  peo_mode;       //离岗人数模式
    VCA_LEAVE_POS_CHANNEL_TYPE chan_type;      //通道属性，独立通道，主通道，辅通道
}PARAM_LEAVE_POST;

// 值岗状态报警参数
typedef struct _PARAM_DUTY_STATUS_
{
	VCA_LEAVE_POS_MODE         pos_mode;       //值岗状态模式
	unsigned int               leave_delay;    //无人报警时间
	unsigned int               static_delay;   //睡觉报警时间
	VCA_LEAVE_POS_PERSON_MODE  peo_mode;       //离岗人数模式
    VCA_LEAVE_ALERT_MODE       alert_mode;     //报警模式,单次报警、间隔报警、前后报警
}PARAM_DUTY_STATUS;

//倒地检测参数
typedef struct _PARAM_FALL_DOWN_
{
	unsigned int delay;
}PARAM_FALL_DOWN;

//声强突变参数
typedef struct _PARAM_AUDIO_ABNORMAL_
{
	unsigned int  decibel;       //声音强度
	unsigned char audio_mode;    //声音检测模式，0：启用灵敏度检测；1：启用分贝阈值检测
}PARAM_AUDIO_ABNORMAL;

//起床规则模式
typedef enum _VCA_GET_UP_MODE_
{
	VCA_GET_UP_OVER_BED_MODE  = 0x1,       //大床通铺模式
	VCA_GET_UP_AREA_MOVE_MODE = 0x2,       //高低铺模式
	VCA_GET_UP_SITTING_MODE   = 0x3        //坐立起身模式
}VCA_GET_UP_MODE;

//起身检测
typedef struct _PARAM_GET_UP_
{
	VCA_GET_UP_MODE mode;                  //起身检测模式
}PARAM_GET_UP;

//起身检测V2版本参数（包含重点犯人起身检测）
typedef struct _PARAM_GET_UP_V2_
{
	VCA_GET_UP_MODE		mode;				// 起身检测模式
	unsigned int		delay;				// 起身离床持续时间，范围: [0, 600]秒	
	unsigned int        alert_interval;		// 起身离床报警间隔，范围: [0, 600]秒，大于1时有效
}PARAM_GET_UP_V2;


// 玩手机报警参数
typedef struct _PARAM_PLAY_PHONE_
{
	unsigned int		delay;				// 玩手机持续时间，范围: [0, 600]秒	
	unsigned int        alert_interval;		// 玩手机报警间隔，范围: [0, 600]秒，大于1时有效
}PARAM_PLAY_PHONE;


//静坐
typedef struct _PARAM_STATIC_
{
    int delay;
}PARAM_STATIC;

// 双目倒地规则
typedef struct _PARAM_BV_FALL_DOWN_
{
    float        height;                        // 倒地高度阈值
    unsigned int delay;                         // 倒地时间
}PARAM_BV_FALL_DOWN; 

// 双目站立检测规则
typedef struct _PARAM_BV_STAND_UP_
{
    float        height;                        // 站立高度阈值
    unsigned int delay;                         // 站立时间
}PARAM_BV_STAND_UP; 

// 双目人数检测及间距大于、小于、等于、不等于判断模式
typedef enum _VCA_COMPARE_MODE_
{
    VCA_MORE_MODE,                              // 大于模式
    VCA_LESS_MODE,                              // 小于模式
    VCA_EQUAL_MODE,                             // 等于模式
    VCA_UNEQUAL_MODE                            // 不等于模式
}VCA_COMPARE_MODE;

// 双目人数检测规则
typedef struct _PARAM_BV_PEOPLE_NUM_
{
    unsigned int     people_num;                // 人数
    unsigned int     people_state;              // 有无人状态,1:有人 0:无人
    VCA_COMPARE_MODE mode;                      // 大于小于模式
    unsigned int     delay;                     // 延时
}PARAM_BV_PEOPLE_NUM;

// 双目间距检测规则
typedef struct _PARAM_BV_PEOPLE_DISTANCE_
{
    float            distance;                  // 两目标间距
    VCA_COMPARE_MODE mode;                      // 大于小于模式
    unsigned int     delay;                     // 延时
}PARAM_BV_PEOPLE_DISTANCE;

//庭审着装类型
typedef enum _VCA_COURT_CLOTH_TYPE_
{
	VCA_COURT_CLOTH_ROBE		= 0x1,				//法袍
	VCA_COURT_CLOTH_UNIF_WIN	= 0x2,				//冬装制服
	VCA_COURT_CLOTH_UNIF_SUM	= 0x3				//夏装制服
}VCA_COURT_CLOTH_TYPE;

//庭审时间规则
typedef struct _PARAM_COURT_
{
	unsigned int				late_time;			// 迟到时间界限 30～1800秒（30分）
	unsigned int				leave_time;			// 早退时间界限 30～1800秒（30分）
	VCA_COURT_CLOTH_TYPE		ref_cloth_type;		// 参考着装类型
}PARAM_COURT;

// 双目物品遗留拿取规则
typedef struct _PARAM_BV_LEFT_TAKE_
{
    float        low_height;                    // 高度下限,单位米
	float        high_height;                   // 高度上限,单位米
    unsigned int delay;                         // 延时
}PARAM_BV_LEFT_TAKE; 

//双目奔跑
typedef struct _PARAM_BV_PEOPLE_RUN_
{
    float        speed_th;                      // 人员最大运动速度阈值，单位米/秒
	unsigned int delay;                         // 延时
}PARAM_BV_PEOPLE_RUN;

//双目滞留
typedef struct _PARAM_BV_PEOPLE_STOP_
{
	int         stop_th;                       // 人员最大停留时间阈值，单位秒
	unsigned int delay;                         // 延时
}PARAM_BV_PEOPLE_STOP;

//双目行程过长
typedef struct _PARAM_BV_PEOPLE_TRIP_
{
	int          trip_th;                       // 人员最大行程阈值，单位米
	unsigned int delay;                         // 延时
}PARAM_BV_PEOPLE_TRIP;

//双目跨线
typedef struct _PARAM_BV_PEOPLE_CROSS_
{
	VCA_CROSS_DIRECTION cross_direction;         //跨越方向
    unsigned int delay;                         // 延时
}PARAM_BV_PEOPLE_CROSS;

//双目剧烈运动
typedef struct _PARAM_BV_VIOLENT_MOTION_
{	       
	unsigned int delay;                         // 持续时间
	unsigned int sensitive;                     // 灵敏度
}PARAM_BV_VIOLENT_MOTION;

//双目离岗
typedef struct _PARAM_BV_LEAVE_POSITION_
{	
	unsigned int delay;                         // 持续时间
    unsigned int sensitive;                     // 人体检测灵敏度
	unsigned int need_people_num;               // 需在岗人数
}PARAM_BV_LEAVE_POSITION;

//双目区域人数统计
typedef struct _PARAM_BV_REGION_PEO_NUM_
{	
	unsigned int sensitive;                     // 人体检测灵敏度
}PARAM_BV_REGION_PEO_NUM;

//人员骤散
typedef struct _PARAM_CROWD_DISAPPEAR_
{
	unsigned int        sensitive;				    // 人员骤散的灵敏度, 范围: [0, 100]
	unsigned int        number;						// 人员骤散报警最少人数, 范围: [2, 50]
	unsigned int		delay;					    // 骤散时间,范围: [0, 60]秒
}PARAM_CROWD_DISAPPEAR;

//排队
typedef struct _PARAM_QUEUE_UP_
{
	unsigned int		queue_num;				    // 排队人数, 范围: [1, 1000]
	unsigned int		queue_time;                 // 排队时间，达到该时间后发出拥挤报警
}PARAM_QUEUE_UP;

//区域人群密度
typedef struct _PARAM_CROWD_DENSITY_MODE_
{
	unsigned int		crowd_mode_type;			// 人群密度模式
}PARAM_CROWD_DENSITY_MODE;

// 态势人群密度模式
typedef enum _PARAM_CROWD_MODE_TYPE_
{
	VCA_MIX_MODE = 0,                               // 人员密度自适应融合模式
	VCA_DET_MODE = 1,						        // 人员密度纯检测模式
	VCA_REG_MODE = 2,						        // 人员密度纯密度模式

}PARAM_CROWD_MODE_TYPE;

//异常状态检测参数
typedef struct _PARAM_ABNORMAL_STATE_
{
	unsigned int		     delay;                    // 异常状态持续时间，范围: [0, 1800]秒	
	unsigned int             alert_interval;		   // 报警间隔，范围: [0, 1800]秒，大于1时有效
	unsigned int             count_interval;		   // 统计间隔，范围: [1, 1800]秒
	unsigned int             max_trigger_num;		   // 最大触发次数，范围: [1, 100]
	unsigned char            max_trigger_valid;        // 最大触发次数是否有效 [0,1] 0-配置最大触发次数不起作用,1-起作用
	unsigned int		     alert_conf_threshold;     // 报警阈值[0-1000）,大于此阈值的报警、统计结果才会输出
}PARAM_ABNORMAL_STATE;



//规则参数联合体
typedef union _VCA_RULE_PARAM_SETS_
{
	int                         param[6];           //参数
	PARAM_TRAVERSE_PLANE        traverse_plane;     //跨线参数  
    PARAM_REACH_HEIGHT_EX       reach_height_ex;    //攀高参数
	PARAM_INTRUSION             intrusion;          //区域入侵参数
	PARAM_LOITER                loiter;             //徘徊参数
	PARAM_LEFT_TAKE             left_take;          //物品拿取放置参数
	PARAM_PARKING               parking;            //停车参数
	PARAM_RUN                   run;                //快速移动参数
	PARAM_HIGH_DENSITY          high_density;       //人员聚集参数
	PARAM_VIOLENT               violent;            //剧烈运动参数
	PARAM_ABNORMAL_FACE         abnormal_face;      //异常人脸参数
	PARAM_OVER_TIME             over_time;          //操作超时
	PARAM_STICK_UP              stick_up;           //异物粘贴参数
	PARAM_INSTALL_SCANNER       insert_scanner;     //安装读卡器
	PARAM_FLOW_COUNTER          flow_counter;       //客流量统计进入方向
	PARAM_LEAVE_POST            leave_post;         //离岗检测报警时间
	PARAM_FALL_DOWN             fall_down;          //倒地检测参数
	PARAM_AUDIO_ABNORMAL        audio_abnormal;     //声音异常
	PARAM_GET_UP                get_up;             //起身检测参数
	PARAM_YARD_TARRY            yard_tarry;         //放风场滞留时间
    PARAM_TOILET_TARRY          toilet_tarry;       //如厕超时时间
    PARAM_STATIC                stic;               //静坐
	PARAM_BV_FALL_DOWN          bv_fall_down;       // 双目倒地
    PARAM_BV_STAND_UP           bv_stand_up;        // 双目站立
    PARAM_BV_PEOPLE_NUM         bv_people_num;      // 双目人数 
    PARAM_BV_PEOPLE_DISTANCE    bv_people_distance; // 双目间距
	PARAM_COURT					court;				//科技法庭参数
    PARAM_BV_LEFT_TAKE          bv_left_take;       // 双目物品遗留拿取
    PARAM_BV_PEOPLE_RUN         bv_people_run;      // 双目人员奔跑
	PARAM_BV_PEOPLE_STOP        bv_people_stop;     // 双目人员逗留
    PARAM_BV_PEOPLE_TRIP        bv_people_trip;     // 双目人员行程
    PARAM_BV_PEOPLE_CROSS       bv_people_cross;    // 双目人员跨线
	PARAM_BV_VIOLENT_MOTION     bv_people_violent_motion;     //剧烈运动
	PARAM_BV_LEAVE_POSITION     bv_people_leave_pos;          //离岗检测
    PARAM_BV_REGION_PEO_NUM     bv_region_peo_num;            //双目区域人数统计 
	PARAM_CROWD_DISAPPEAR       crowd_disappear;    // 人群骤散
	PARAM_QUEUE_UP				queue_up;           // 排队等候
	PARAM_CROWD_DENSITY_MODE    crowd_mode;			// 密度估计
	PARAM_RUN_V2                run_param;          // 快速移动参数V2版本
	PARAM_RUN_V3                run_param_ex;       // 快速移动参数V3版本
	PARAM_HIGH_DENSITY_V2       gather_param;       // 人员聚集参数V2版本
	PARAM_VIOLENT_V2			violent_param;		// 剧烈运动参数V2版本
	PARAM_ABNORMAL_STATE		abnormal_state;		// 异常状态检测
	PARAM_REACH_HEIGHT		    climb_state;        // 攀高检测参数
	PARAM_GET_UP_V2             get_up_v2_state;    // 起身离床检测参数V2版本
	PARAM_PLAY_PHONE            play_phone;         // 玩手机检测参数
	PARAM_DUTY_STATUS           duty_state;         // 值岗状态参数

}VCA_RULE_PARAM_SETS;

//报警规则类型，
typedef enum  _VCA_RULE_TYPE_
{
    LINE_RULE,    //线规则 
	REGION_RULE,  //区域规则
	LINE_EX_RULE, //折线规则
	UNITE_RULE    //组合规则
}VCA_RULE_TYPE;

// 事件类型
typedef enum _VCA_EVENT_TYPE_
{ 
    VCA_TRAVERSE_PLANE  = 0x01,         //穿越警戒面
    VCA_ENTER_AREA      = 0x02,         //进入区域
    VCA_EXIT_AREA       = 0x04,         //离开区域
    VCA_INTRUSION       = 0x08,         //区域入侵
    VCA_LOITER          = 0x10,         //徘徊
    VCA_LEFT_TAKE       = 0x20,         //物品拿取放置
    VCA_PARKING         = 0x40,         //停车
    VCA_RUN             = 0x80,         //快速移动
    VCA_HIGH_DENSITY    = 0x100,        //人员聚集
    VCA_FLOW_COUNTER    = 0x200,        //客流量
    VCA_VIOLENT_MOTION  = 0x400,        //剧烈运动
    VCA_TRAIL           = 0x800,        //尾随
    VCA_LEFT            = 0x1000,       //物品遗留
    VCA_TAKE            = 0x2000,       //物品拿取
    VCA_ABNORMAL_FACE   = 0x4000,       //异常人脸
    VCA_GET_UP          = 0x8000,       //起身检测
    VCA_REACH_HEIGHT    = 0x10000,      //攀高检测
    VCA_LEAVE_POSITION  = 0x20000,      //离岗检测
    VCA_FACE_CAPTURE	= 0x40000,      //正常人脸
    VCA_FALL_DOWN       = 0x80000,      //倒地检测
    VCA_AUDIO_ABNORMAL  = 0x100000,     //声音异常检测
    VCA_MULTI_FACES     = 0x200000,     //多张人脸
    VCA_TOILET_TARRY    = 0x400000,     //如厕超时
    VCA_YARD_TARRY      = 0x800000,     //放风场滞留
    VCA_REACH_HEIGHT_EX = 0x1000000,    //攀高检测(折线)
	VCA_TRAVERSE_PLANE_EX = 0x2000000,  //穿越警戒面(折线)
	VCA_CALL            = 0x4000000,    //打电话检测
    VCA_HUMAN_ENTER     = 0x10000000,   //人员进入
    VCA_OVER_TIME       = 0x20000000,   //操作超时
    VCA_STICK_UP        = 0x40000000,   //异物粘贴              
    VCA_INSTALL_SCANNER = 0x80000000    //安装读卡器          	
}VCA_EVENT_TYPE;

/***********************************************************************************************************************
*事件类型枚举
*高6位为模块编号（最多64个模块），低26位为事件类型排列（一位一个事件,单个模块最多26个事件）
*各个模块的分段以及范围参考下表：
*      模块名       模块编码(高6位)               区间范围
*       ivs          0000 01                0x04000001-0x06000000
*       pdc          0000 10                0x08000001-0x0A000000
*       ips          0000 11                0x0C000001-0x0E000000
*       ias          0001 00                0x10000001-0x12000000
*       scd          0001 01                0x14000001-0x16000000
*       ies          0001 10                0x18000001-0x1A000000
*       ibv          0001 11                0x1C000001-0x20000000
***********************************************************************************************************************/
typedef enum _VCA_EVENT_TYPE_EX_
{ 
    //ivs专用,区间0x04000001-0x06000000
    VCA_IVS_LOITER            = 0x04000001,   //徘徊
    VCA_IVS_LEFT_TAKE         = 0x04000002,   //物品拿取放置
    VCA_IVS_INTRUSION         = 0x04000004,   //区域入侵
    VCA_IVS_EXIT_AREA         = 0x04000008,   //离开区域
    VCA_IVS_ENTER_AREA        = 0x04000010,   //进入区域
    VCA_IVS_TRAVERSE_PLANE    = 0x04000020,   //穿越警戒面
    VCA_IVS_TRAVERSE_PLANE_EX = 0x04000040,   //穿越警戒面(折线)
    VCA_IVS_VIOLENT_MOTION    = 0x04000080,   //剧烈运动
    VCA_IVS_RUN               = 0x04000100,   //快速移动
    VCA_IVS_LEFT              = 0x04000200,   //物品遗留
    VCA_IVS_TAKE              = 0x04000400,   //物品拿取
    VCA_IVS_PARKING           = 0x04000800,   //停车
    VCA_IVS_HIGH_DENSITY      = 0x04001000,   //人员聚集
	VCA_IVS_MD                = 0x04001200,   //移动侦测
	VCA_IVS_UNITE_ALARM       = 0x04002000,   //组合报警
	VCA_IVS_CROWD_NUM         = 0x04004000,   //区域人数
	VCA_IVS_CROWD_DISAPPER    = 0x04008000,   //人群骤散
	VCA_IVS_CROWD_DENSITY	  = 0x04010000,	  //区域人群密度
	VCA_IVS_QUEUE			  = 0x04020000,   //排队检测

    //pdc专用,区间0x08000001-0x0A000000
    VCA_PDC_FLOW_COUNTER      = 0x08000001,   //客流量

    //ips专用,区间0x0C000001-0x0E000000
    VCA_IPS_VIOLENT_MOTION    = 0x0C000001,   //剧烈运动
    VCA_IPS_GET_UP            = 0x0C000002,   //起身检测
    VCA_IPS_FELON_GET_UP      = 0x0C000004,   //重点犯人起身
    VCA_IPS_REACH_HEIGHT      = 0x0C000008,   //攀高检测
    VCA_IPS_REACH_HEIGHT_EX   = 0x0C000010,   //攀高检测(折线)
    VCA_IPS_LEAVE_POSITION    = 0x0C000020,   //离岗检测
    VCA_IPS_YARD_TARRY        = 0x0C000040,   //放风场滞留
    VCA_IPS_TOILET_TARRY      = 0x0C000080,   //如厕超时
    VCA_IPS_AUDIO_ABNORMAL    = 0x0C000100,   //声强突变
    VCA_IPS_STAND_UP          = 0x0C000200,   //起立（审讯室使用）
    VCA_IPS_STATIC            = 0x0C000400,   //静坐
    VCA_IPS_HIGH_DENSITY      = 0x0C000800,   //人员聚集	
    VCA_IPS_THROW             = 0x0C001000,   //抛物
	VCA_IPS_DUTY_STATUS       = 0x0C002000,   //值岗状态
	VCA_IPS_PLAY_PHONE        = 0x0C004000,   //玩手机
	VCA_IPS_CLOTH_CLS         = 0x0C008000,   //制服分类
	VCA_IPS_PEOPLE_NUM        = 0x0C010000,   //区域人数（放风场滞留、如厕超时、人数异常）	
	//ias专用,区间0x10000001-0x12000000 
	VCA_IAS_TRAVERSE_PLANE    = 0x10000001,   //穿越警戒面 
	VCA_IAS_EXIT_AREA         = 0x10000002,   //离开区域 
	VCA_IAS_ENTER_AREA        = 0x10000004,   //进入区域 
	VCA_IAS_LEFT_TAKE         = 0x10000008,   //物品拿取放置 
	VCA_IAS_INTRUSION         = 0x10000010,   //区域入侵 
	VCA_IAS_LOITER            = 0x10000020,   //徘徊 
	VCA_IAS_VIOLENT_MOTION    = 0x10000040,   //剧烈运动 
	VCA_IAS_TRAIL             = 0x10000080,   //尾随 
	VCA_IAS_AUDIO_ABNORMAL    = 0x10000100,   //声音异常检测 
	VCA_IAS_FALL_DOWN         = 0x10000400,   //倒地检测 
	VCA_IAS_STICK_UP          = 0x10000200,   //异物粘贴 
	VCA_IAS_INSTALL_SCANNER   = 0x10000800,   //安装读卡器 
	VCA_IAS_OVER_TIME         = 0x10001000,   //操作超时 
	VCA_IAS_HUMAN_ENTER       = 0x10002000,   //人员进入 
	VCA_IAS_ABNORMAL_FACE     = 0x10004000,   //异常人脸 
	VCA_IAS_FACE_CAPTURE      = 0x10008000,   //正常人脸 
	VCA_IAS_MULTI_FACES       = 0x10010000,   //多张人脸 
	VCA_IAS_SGL_FACES         = 0x10020000,   //墨镜人脸
	VCA_IAS_CALL              = 0x10040000,   //打电话检测
	VCA_IAS_WHITE_CARD		  = 0x10080000,   //白卡检测

	//scd专用，区间0x14000001-0x16000000 
	VCA_SCD_CAMERA_TAMPER     = 0x14000001,   //相机遮挡
	VCA_SCD_CAMERA_MOVE       = 0x14000002,   //相机移动
	
	// ies(文教)专用,区间0x1800001-0x1A000000
	VCA_IES_TEACHING         = 0x18000001,   // 授课
	VCA_IES_ANSWER           = 0x18000002,   // 回答问题（通过alert结构中的reserved[0]的值1为开始、0为结束）
	VCA_IES_SIT_DOWN         = 0x18000003,   //坐下（内部使用）
    VCA_IES_WRITING          = 0x18000004,   // 板书

    // ibv专用,区间0x1C000001-0x1F000000
    VCA_IBV_FALL_DOWN         = 0x1C000001,   // 倒地
    VCA_IBV_STAND_UP          = 0x1C000002,   // 站立
    VCA_IBV_PEOPLE_NUM        = 0x1C000004,   // 人数
    VCA_IBV_PEOPLE_DISTANCE   = 0x1C000008,   // 间距
    VCA_IBV_LEFT_TAKE         = 0x1C000010,   // 物品遗留
	VCA_IBV_STU_EVENT         = 0x1C000011,   // 学生事件
	VCA_IBV_RUN               = 0x1C000012,    // 快速移动
	VCA_IBV_STOP              = 0x1C000013,    // 逗留
	VCA_IBV_TRIP              = 0x1C000014,    // 行程
	VCA_IBV_CROSS             = 0x1C000015,    // 跨线
	VCA_IBV_VIOLENT_MOTION    = 0x1C000016,   //剧烈运动
	VCA_IBV_LEAVE_POSITION    = 0x1C000017,    //离岗检测
	VCA_IBV_REGION_PEO_NUM    = 0x1C000018,   // 区域人数统计
		// ics（庭审）专用,区间0x20000001-0x
	VCA_ICS_COURT_CLOTH			= 0x20000001,	//庭审着装分析
	VCA_ICS_COURT_ABSENT		= 0x20000002,	//缺席
	VCA_ICS_COURT_LATE			= 0x20000004,	//迟到
	VCA_ICS_COURT_LEAVE			= 0x20000008,	//早退
	VCA_ICS_COURT_AWAY			= 0x20000010,	//中途离席
	VCA_ICS_COURT_FIGHT			= 0x20000020,	//庭审剧烈运动
	VCA_ICS_COURT_INTRUSION		= 0x20000040,	//庭审区域入侵

	// 通用触发库（CTA）使用，区间0x40004001-0x
	VCA_CTA_OBJECT_ABNORMAL_STATE = 0x40004001,   // 区域目标异常状态检测
	VCA_CTA_REGION_ABNORMAL_STATE = 0x40004002,   // 区域异常状态检测（区域属性）
	VCA_CTA_CROSSLINE			  = 0x40004003,   // 跨线目标检测
	VCA_CTA_CROSSLINE_NUM         = 0x40004004,   // 跨线目标统计
	VCA_CTA_OBJECT_COUNT          = 0x40004005,   // 区域目标数统计
	VCA_CTA_REGION_OVERLAP_RATIO  = 0x40004006,   // 区域交叠比统计
    VCA_CTA_REGION_OVERLAP_RATIO_ABNORMAL_STATE = 0x40004007,// 区域交叠比异常检测
	VCA_CTA_OBJECT_DIFF_STATE     = 0x40004008,   // 区域状态变化检测（图像比对，相对背景）
	VCA_CTA_OBJECT_OCR_TRIGGER    = 0x40004009,   // OCR触发抓拍

	// 通用目标触发模块（IED内部回调使用），区间0x4008001-0x
	VCA_IED_OBJECT_TRIGGER_ANALYSIS = 0x40008001, // 目标触发分析（开放平台全分析/结构化抓拍）

}VCA_EVENT_TYPE_EX;

//简化的规则信息, 包含规则的基本信息
typedef struct _VCA_RULE_INFO_
{
	unsigned char         ID;           //警戒规则ID，如果是组合规则，对应的ID链表见reserved
	unsigned char         rule_type;    //规则类型，0: 线规则; 1: 区域规则；2：折线规则；3：组合规则;
	unsigned char         reserved[6];  //对齐字节，如果是组合报警，第1位表示被组合的单一规则的条数，2-5位分别为单一规则对应的ID
	unsigned int          event_type;   //警戒事件类型
	VCA_RULE_PARAM_SETS   param;        //规则参数
	VCA_POLYGON_F         polygon;      //规则对应的多边形区域
	//COUNTER               *counter;
}VCA_RULE_INFO;

//规则参数
typedef struct _VCA_RULE_PARAM_
{
	unsigned int          sensitivity; //规则灵敏度参数
	unsigned int          reserved[4]; //预留字节
	VCA_RULE_PARAM_SETS   param;       //规则参数
	VCA_SIZE_FILTER       size_filter; //尺寸过滤器	
}VCA_RULE_PARAM;

//警戒规则
typedef struct _VCA_RULE_
{
	unsigned char      ID;             //规则ID
	unsigned char      enable;         //是否激活;
	unsigned char      rule_type;      //规则类型，0: line; 1: region;
	unsigned char      target_type;    //警戒目标类型，2: Human; 1: Vehicle; 0: anything
    unsigned int       event_type;     //警戒事件类型
	
	unsigned char      reset_counter;  //流量统计重置标记
	unsigned char      update_rule;    //规则更新标记
	unsigned char      enable_counter; //是否开启事件计数器
    unsigned char      reserved[5];    //保留字节,第一个字节用来表示使用新的事件类型还是老的；0：老的；1：新的;
									   // 支持规则多种颜色，使用reserved的前4个字节,播放器使用
									   // reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB

    VCA_RULE_PARAM     rule_param;     //规则参数
    VCA_POLYGON_F      polygon;        //规则对应的多边形区域
}VCA_RULE;

//警戒规则链表
typedef struct _VCA_RULE_LIST_
{
    unsigned int  rule_num;           //链表中规则数量
    VCA_RULE      rule[MAX_RULE_NUM]; //规则数组
}VCA_RULE_LIST;

//扩展警戒规则链表
typedef struct _VCA_RULE_LIST_EX_
{
	unsigned int  rule_num;              //链表中规则数量
	VCA_RULE      rule[MAX_RULE_NUM_EX]; //规则数组
}VCA_RULE_LIST_EX;

//扩展警戒规则链表
typedef struct _VCA_RULE_LIST_V3_
{
	unsigned int  rule_num;              //链表中规则数量
	VCA_RULE      rule[MAX_RULE_NUM_V3]; //规则数组
}VCA_RULE_LIST_V3;
#define   MAX_COLOR_AREA_NUM  3
//颜色区域
typedef struct _VCA_COLOR_AREA_
{
	unsigned char     ID;            //颜色区域ID
	unsigned char     valid;         //是否有效;
	unsigned char     update_flg;    //是否更新标记
	VCA_RECT_F        rect;          //颜色区域
	VCA_VIDEO_FRAME   video_frame;   //视频输入帧
}VCA_COLOR_AREA;

//颜色区域链表
typedef struct _VCA_COLOR_AREA_LIST_
{
	unsigned int    area_num;                        //有效的颜色区域个数
	VCA_COLOR_AREA  color_area[MAX_COLOR_AREA_NUM];  //颜色区域信息
}VCA_COLOR_AREA_LIST;

//规则关联颜色信息结构体
typedef struct _VCA_RULE_COLOR_
{
	unsigned char        rule_id;      //规则ID	
	unsigned char        enable;       //是否激活;
	VCA_COLOR_AREA_LIST  area_list;    //颜色区域链表
}VCA_RULE_COLOR;

/***********************************************************************************************************************
* 组合报警相关结构体定义
***********************************************************************************************************************/
// 组合报警类型
typedef enum _VCA_UNITE_TYPE_
{
	UNITE_AND,      //不分先后触发
	UNITE_THEN,     //按先后顺序依次触发
}VCA_UNITE_TYPE;

// 间隔时间
typedef struct _VCA_TIME_INTERVAL_
{
	int	  min_interval;		// 最小时间间隔，单位s
	int	  max_interval;	    // 最大时间间隔，单位s
}VCA_TIME_INTERVAL;

// 组合规则
typedef struct _VCA_UNITE_RULE_
{
	unsigned char		ID;							                // 组合报警规则ID
	unsigned char       enable;                                     // 是否激活
	unsigned char 		unite_type;						            // 组合报警类型，AND、THEN
	unsigned char		rule_num; 						            // 实际使用的组合报警规则条数  
	unsigned char		base_ID[MAX_RULE_NUM_IN_UNITE];			    // 原始规则ID数组，里面按顺序存放单一规则ID
	unsigned int        event_type;                                 // 事件类型
	VCA_TIME_INTERVAL	time_interval[MAX_RULE_NUM_IN_UNITE - 1];   // 多个规则的间隔时间
}VCA_UNITE_RULE;

// 组合规则链表
typedef struct _VCA_UNITE_RULE_LIST_
{
	unsigned int    rule_num;                       //链表中组合规则数量
	VCA_UNITE_RULE  unite_rule[MAX_UNITE_RULE_NUM]; //组合规则数组
}VCA_UNITE_RULE_LIST;

/***********************************************************************************************************************
*配置信息中绘制辅助区域相关信息结构体定义
***********************************************************************************************************************/
//辅助区域(auxiliary area)类型
/* 辅助区域配置类型字段分配：
0x0001  - 0x1000  -------------------------- 通用
0x1001  - 0x2000  -------------------------- IVS专用
0x2001  - 0x3000  -------------------------- IAS专用
0X3001  - 0X4000  -------------------------- PDC专用
0X4001  - 0X5000  -------------------------- 人脸检测专用
0X5001  - 0X6000  -------------------------- IPS专用
0X6001  - 0X7000  -------------------------- 人脸识别专用
*/
typedef enum _VCA_AUX_AREA_TYPE_
{
    IPS_LEAVE_POSITION_OVERLAP_REG_TYPE = 0X6001,    //离岗共同区域
    IPS_GET_UP_OVER_BED_LOCATION        = 0X6002,    //通铺大床位置
	ICS_JUDGMENT_SEAT                   = 0X6003,    //审判席座位
    IPS_GET_UP_HEIGH_LOW_BED            = 0X6004,    //高低铺位置
	IPS_REACH_HIGHT_WINDOW              = 0X6005,    //攀高窗口
}VCA_AUX_AREA_TYPE;

//辅助区域结构体
typedef struct _VCA_AUX_AREA_
{
    unsigned int      enable;               //是否启用
    unsigned int      rule_id;              //规则索引
    VCA_AUX_AREA_TYPE type;                 //辅助区域类型
    unsigned char     reserved[4];
    VCA_POLYGON_F     polygon;              //多边形
}VCA_AUX_AREA;

//辅助区域列表
typedef struct _VCA_AUX_AREA_LIST_
{
    unsigned int      aux_num;                                //辅助规则数量
    unsigned char     reserved[4];
    VCA_AUX_AREA      aux_area[MAX_AUX_AREA_NUM];             //辅助规则信息
}VCA_AUX_AREA_LIST;

/***********************************************************************************************************************
*配置信息中取色相关信息结构体定义
***********************************************************************************************************************/
//取色方式
typedef enum _VCA_COLOR_METHODS_
{
    COLOR_PALETTE,                     //调色板取色
    COLOR_PIC                          //图片取色
}VCA_COLOR_METHODS;

//颜色取块信息,ips用于取马甲颜色信息
typedef struct _VCA_COLOR_INFO_
{
    unsigned int        enable;                 //有效位
    VCA_COLOR_METHODS   type;                   //取色方式，调色板还是图片取色
    unsigned char       reserved[4];
    VCA_RECT_F          rect;                   //颜色块，归一化之后
    VCA_VIDEO_FRAME     frame;                  //图像信息
}VCA_COLOR_INFO;

/***********************************************************************************************************************
*配置信息中高级参数相关信息结构体定义
***********************************************************************************************************************/

// 算法库参数结构体
typedef struct _VCA_KEY_PARAM_
{
	int  index;
	int  value; 
}VCA_KEY_PARAM;

// 参数链表
typedef struct _VCA_KEY_PARAM_LIST_
{
	unsigned int   param_num;
	VCA_KEY_PARAM  param[MAX_PARAM_NUM];
}VCA_KEY_PARAM_LIST;

// 算法库版本信息V2
typedef struct _VCA_LIB_VERSION_V2_
{
	unsigned char  major_version;      // 主版本号
	unsigned char  sub_version;        // 子版本号
	unsigned short revision_version;   // 修订版本号，0-99基线版本、100-199PK版本、200-299定制版本 
	unsigned short ver_year;           // 年
	unsigned char  ver_month;          // 月
	unsigned char  ver_day;            // 日
}VCA_LIB_VERSION_V2;

/***********************************************************************************************************************
*配置信息中标定操作相关信息结构体定义
***********************************************************************************************************************/
//标定线类型
typedef enum _VCA_LINE_MODE_
{
	HEIGHT_LINE,
	LENGTH_LINE
}VCA_LINE_MODE;

//标定线数据
typedef struct _VCA_LINE_SEGMENT_
{
	VCA_POINT_F    start_pnt;
	VCA_POINT_F    end_pnt;
	float          value;
	VCA_LINE_MODE  mode;
}VCA_LINE_SEGMENT;

// 摄像机参数结构体
typedef struct _VCA_CAMERA_PARAM_
{
	float	     camera_height;       // 摄像机离地面高度
	float	     camera_angle;        // 摄像机镜头与水平线的夹角，用弧度标示
	float        horizon;             // 场景中的地平线
    unsigned int mode;                // 标定参数模式
}VCA_CAMERA_PARAM;

/***********************************************************************************************************************
*配置信息中双目标定操作相关信息结构体定义
***********************************************************************************************************************/
// 双目图像矫正参数结构体
typedef struct _VCA_BINOC_RECTIFY_PARAM_
{
    float cam_internal_mat[3][3];                   // 相机内参矩阵
    float dist_coeffs[8];                           // 畸变矫正系数
    float rotate_mat[3][3];                         // 旋转矩阵
    float project_mat[3][4];                        // 投影矩阵
}VCA_BINOC_RECTIFY_PARAM;

// 左右相机矫正参数
typedef struct _VCA_BV_CALIB_PARAM_
{
    float                   reproject_mat[4][4];    // 重投影矩阵
    VCA_BINOC_RECTIFY_PARAM l_cam_param;            // 左相机标定参数
    VCA_BINOC_RECTIFY_PARAM r_cam_param;            // 右相机标定参数
}VCA_BV_CALIB_PARAM;

// 双目相机安装角度参数
typedef struct _VCA_BV_CAMERA_FIX_PARAM_
{
    float   camera_height;                          // 双目相机架设高度
    float   pitch_angle;                            // 俯仰角度
    float   incline_angle;                          // 倾斜角度
}VCA_BV_CAMERA_FIX_PARAM; 

// 双目联动球机标定参数
typedef struct _VCA_BV_PTZ_CALIB_PARAM_
{
	float             ptz_height;				    // 球机在地面坐标系绝对高度
	float             base_pan;					    // 以角度为单位
	float             base_tilt;                    // 以角度为单位
	float             zoom_factor;				    // 倍率放大系数
	int               image_w;                      // 图像宽度
	int               image_h;                      // 图像高度
	int               shift_x;                      // 球机位置在地面坐标系相对于双目x方向平移
	int               shift_y;					    // 球机位置在地面坐标系相对于双目y方向平移
	int               is_up;                        // 是否正立
	unsigned char     reserved[4];                  // 保留字节   
}VCA_BV_PTZ_CALIB_PARAM; 
/***********************************************************************************************************************
*目标相关信息结构体定义
***********************************************************************************************************************/

//报警目标类型
typedef enum _VCA_TARGET_TYPE_
{
    TARGET_ANYTHING,  //人或车
	TARGET_HUMAN,     //人
	TARGET_VEHICLE    //车
}VCA_TARGET_TYPE;

//target来源类型
typedef enum _VCA_TARGET_FROM_TYPE_
{
	TARGET_FROM_BGS = 3,   //此目标框来自背景建模类算库，如SMD,IVS和ATK
	TARGET_FROM_FD  = 4    //次目标框来自人脸检测算法库
}VCA_TARGET_FROM_TYPE;

/***********************************************************************************************************************
*目标归属算法库ID编码
*ID编码复用目标结构体（VCA_TARGET)中的ID，其高8位用于算法库ID编码，低24位用于目标原有ID编号
*判断目标target的算法库类型，可通过其ID高8位（(target->ID) >> 24）与枚举VCA_LIBS_ID中定义的值确定
*其中各算法库的编码方式如下表：
*______________________________________
*      模块名    |   模块编码(高8位)  |
*________________|____________________|
*       SMD      |   10000000         |
*________________|____________________|
*       FD       |   01000000         |
*________________|____________________|
*       PDC      |   11000000         |
*________________|____________________|
***********************************************************************************************************************/
typedef enum _VCA_LIBS_ID_
{
	VCA_LIB_SMD    = 0x10000000,  //SMD编号
	VCA_LIB_FD     = 0x01000000,  //FD编号
	VCA_LIB_PDC    = 0x11000000   //PDC编号
}VCA_LIBS_ID;

//目标信息结构体
typedef struct _VCA_TARGET_
{
    unsigned int      ID;          //ID
    unsigned char     type;        //类型(VCA_TARGET_FROM_TYPE)
	unsigned char     alarm_flg;   //目标的报警标志位，TRUE为报警状态，FALSE不为报警状态
    unsigned char     reserved[6]; //保留字节
	                               // 以下是播放库显示复用信息
	                              // 1）支持目标多种颜色，使用reserved的前4个字节 
                                  //   reserved[0] = 0xdb (R+G+B十六进制和)，表示是颜色版本，reserved[1]为R，reserved[2]为G，reserved[3]为B，表示RGB
                                  // 2）采用两字节标识是否是马赛克，reserved[0] = 0xMS （mosaic 缩写)，表示是马赛克版本，
                                  //   reserved[1]为马赛克标识，1表示为马赛克，0表示非马赛克；

    VCA_RECT_F        rect;        //目标框
}VCA_TARGET;

//目标链表
typedef struct _VCA_TARGET_LIST_
{
    unsigned char target_num;             //链表中目标数量
    unsigned char reserved[7];            //保留字节
	VCA_TARGET    target[MAX_TARGET_NUM]; //目标数组
}VCA_TARGET_LIST;

//目标链表
typedef struct _VCA_TARGET_LIST_V2_
{
    unsigned char target_num;             //链表中目标数量
    unsigned char reserved[7];            //保留字节
    VCA_TARGET    target[MAX_TARGET_NUM_V2]; //目标数组
}VCA_TARGET_LIST_V2;
/***********************************************************************************************************************
*报警相关信息结构体定义
***********************************************************************************************************************/

//视频状态
typedef enum _VCA_VIEW_STATE_
{
    GOOD_VIEW,        //视频状态正常 
	BAD_SIGNAL,       //视频信号不正常
	UNKNOWN_VIEW,      //摄像机被移动或遮挡
    MOVE_SIGNAL
}VCA_VIEW_STATE;

//报警结构体
typedef struct  _VCA_ALERT_
{
    unsigned char     alert;       //有无报警信息: 0-没有，1-有
    unsigned char     view_state;  //场景状态:0-正常, 1-场景变化，2－摄像机遮挡
  	unsigned char     reserved[6]; //保留字节
	VCA_RULE_INFO     rule_info;   //报警事件对应的简化规则信息
    VCA_TARGET        target;      //报警目标信息         
}VCA_ALERT;

/***********************************************************************************************************************
*事件相关信息结构体定义
***********************************************************************************************************************/

//流量统计结构体(返回流量信息)
typedef struct _PDC_COUNTER_INFO_
{
    unsigned int leave_num;
    unsigned int enter_num;
}PDC_COUNTER_INFO;

//事件参数值联合体
typedef union  _VCA_EVT_VALUE_
{
	int					param[8];          //参数
	int					human_in;          //有无人接近
	float				crowd_density;     //人群拥挤密度值
	float               motion_entropy;    //剧烈运动熵
	float               audio_entropy;     //音频强度
	PDC_COUNTER_INFO    pdc_counter;       //客流量信息
	float               abrun_info[4];     //abrun_info[0]速度最大值,abrun_info[1]速度阈值
	                                       //abrun_info[2]累计时间,abrun_info[3]时间阈值
}VCA_EVT_VALUE;

//事件信息结构体
typedef struct _VCA_EVT_INFO_
{
	unsigned char   enable;           //使能标记
	unsigned char   rule_id;          //规则id
	unsigned char   reserved[6];      //保留字节
	unsigned int    event_type;       //事件类型
	VCA_EVT_VALUE   event_value;      //事件参数值	
}VCA_EVT_INFO;

//事件信息列表结构体
typedef struct _VCA_EVT_INFO_LIST_
{
	unsigned char   event_num;                   //激活的事件数量
	unsigned char   reserved[7];                 //保留字节
	VCA_EVT_INFO    event_info[MAX_RULE_NUM];    //事件信息，按照规则存放	
}VCA_EVT_INFO_LIST;

//扩展的事件信息列表结构体
typedef struct _VCA_EVT_INFO_LIST_EX_
{
	unsigned char   event_num;                   //激活的事件数量
	unsigned char   reserved[7];                 //保留字节
	VCA_EVT_INFO    event_info[MAX_RULE_NUM_EX]; //事件信息，按照规则存放	
}VCA_EVT_INFO_LIST_EX;

typedef struct _VCA_EVT_INFO_LIST_V3_
{
	unsigned char   event_num;                   //激活的事件数量
	unsigned char   reserved[7];                 //保留字节
	VCA_EVT_INFO    event_info[MAX_RULE_NUM_V3]; //事件信息，按照规则存放	
}VCA_EVT_INFO_LIST_V3;

/***********************************************************************************************************************
*主函数通信结构体定义
***********************************************************************************************************************/

// 算法库之间通信信息结构体
typedef struct _VCA_COM_INFO_
{
	unsigned int size;
	void*        com_data;
}VCA_COM_INFO;

/***********************************************************************************************************************
*配置命令信息
***********************************************************************************************************************/

/* 配置类型字段分配：
0x0001  - 0x1000  -------------------------- 通用配置信息
0x1001  - 0x2000  -------------------------- IVS专用配置信息
0x2001  - 0x3000  -------------------------- IAS专用配置信息
0X3001  - 0X4000  -------------------------- PDC专用配置信息
0X4001  - 0X5000  -------------------------- 人脸检测专用配置信息
0X5001  - 0X6000  -------------------------- IPS专用配置信息
0X6001  - 0X7000  --------------------------人脸识别专用配置信息
*/

//算法库配置数据类型枚举
typedef enum _VCA_SET_CFG_TYPE_
{
	//通用配置信息
	VCA_SET_CFG_RULE_LIST         = 0x0001,    //规则链表
	VCA_SET_CFG_SINGLE_PARAM      = 0x0002,    //单个参数
	VCA_SET_CFG_PARAM_LIST        = 0x0003,    //参数表
	VCA_SET_CFG_DEFAULT_PARAM     = 0x0004,    //默认参数
	VCA_SET_CFG_RESTART_LIB       = 0x0005,    //重启算法库
	VCA_SET_CFG_SHIELD_REGION     = 0x0006,    //屏蔽区域
	VCA_SET_CFG_SIZE_FILTER       = 0x0007,    //尺寸过滤器
	VCA_SET_CFG_CALIB_CAM         = 0x0008,    //相机标定信息
	VCA_SET_CFG_DISABLE_CALIB     = 0x0009,    //取消相机标定
    VCA_SET_CFG_SET_CALLBACK      = 0x000A,    //回调函数
    VCA_SET_CFG_RULE_LIST_EX      = 0x000B,    //扩展规则链表
	VCA_SET_CFG_RULE_LIST_V3      = 0x000C,    //扩展规则链表(SMD全功能)
	VCA_SET_CFG_UNITE_RULE_LIST   = 0x000D,    //组合规则链表
	VCA_SET_CFG_ROI_REGION		  = 0x000E,    //ROI区域
	
	//IVS专用配置信息
	VCA_SET_CFG_CAL_LF_SYS               = 0x1001,    //LF双摄像机标定信息
	VCA_SET_CFG_LF_MANUAL_ID             = 0x1002,    //LF手动跟踪目标ID
    VCA_SET_CFG_META_HSR_PTZ             = 0x1003,    //设置PTZ信息，用于筛选指定场景的回放
	VCA_SET_CFG_CAL_LF_SYS_PTZ_ID        = 0x1004,	   //LF双摄像机标定（输入球机ID）
    VCA_SET_CFG_RULE_COLOR               = 0x1005,    //规则中的颜色区域
    VCA_SET_CFG_EAGLE_EYE_CAL_SYS_PTZ_ID = 0x1006,	   //LF双摄像机标定（输入球机ID）
	
	//IAS专用配置信息
	VCA_SET_CFG_CHANNEL_MODE      = 0x2001,    //通道类型
	VCA_SET_CFG_ENTER_REGION      = 0x2002,    //进入区域
	VCA_SET_CFG_MICRO_DETECTOR    = 0x2003,    //微波传感器
	
	//PDC 专用配置信息
	VCA_SET_CFG_COUNTER_RESET     = 0x3001,    //计数器重置

	//人脸检测 TI DM385、8127等平台FD使用参数
    VCA_SET_CFG_SET_SEM_LOCK      = 0x4001,   // 信号量锁函数
    VCA_SET_CFG_SET_SEM_UNLOCK    = 0x4002,   // 信号量解锁函数
    VCA_SET_CFG_SET_IRQ_FUNC      = 0x4003,   // 中断函数

    //IPS 专用配置信息
    VCA_SET_CFG_COLOR_INFO        = 0x5001,    //取色
	VCA_SET_CFG_AUX_AREA_LIST     = 0x5002,    //辅助区域设置
	
    // IBV 专用配置信息
    VCA_SET_CFG_BV_CALIB_PARAM    = 0x6001,    //双目相机标定参数
    VCA_SET_CFG_BV_FIX_PARAM      = 0x6002,    //双目相机安装参数
	VCA_SET_CFG_BV_PTZ_CALIB_PARAM = 0X6003,    //双目联动球机安装参数
	
	//ITS 专用配置信息
	VCA_SET_CFG_REFER_REGION      = 0x7001,    //参考区域
	VCA_SET_CFG_LANE              = 0x7002,    //配置车道
	VCA_SET_CFG_SCENE_PARAM       = 0x7003,    //配置场景参数
	VCA_SET_CFG_TPS_RULE          = 0x7004,    //配置参数规则
	VCA_SET_CFG_AID_RULE          = 0x7005,    //配置事件规则
	VCA_SET_CFG_DEL_ALL_CFG       = 0x7006,    //清空配置
	VCA_SET_CFG_AID_RULE_EX       = 0x7007,    //配置带方向规则
	VCA_SET_CFG_LANE_EX           = 0x7008,    //配置车道，配合VCA_SET_CFG_AID_RULE_EX
   
    //开放平台 专用配置信息
	VCA_SET_CFG_JSON_FILE		  = 0x8001,    //输入JSON文件配置

	//HVA 专用配置
	VCA_SET_CFG_PARAM_INFO        = 0x9001,	   //配置字符串参数信息
	VCA_SET_CFG_DAT_INFO		  = 0x9002,    //配置序列化参数信息
	VCA_SET_CFG_GLOBAL_INFO		  = 0x9003,    //配置全局参数信息
	VCA_SET_CFG_NODE_INFO		  = 0x9004     //配置节点参数信息

}VCA_SET_CFG_TYPE;

//配置参数类型
typedef enum _VCA_GET_CFG_TYPE_
{
	//通用配置信息
	VCA_GET_CFG_RULE_LIST         = 0x0001,    //规则链表
	VCA_GET_CFG_SINGLE_PARAM      = 0x0002,    //单个参数
	VCA_GET_CFG_PARAM_LIST        = 0x0003,    //参数表
	VCA_GET_CFG_SHIELD_REGION     = 0x0004,    //屏蔽区域
	VCA_GET_CFG_SIZE_FILTER       = 0x0005,    //尺寸过滤器
	VCA_GET_CFG_CALIB_CAM         = 0x0006,    //相机标定信息
	VCA_GET_CFG_VERSION           = 0x0007,    //版本信息
	VCA_GET_CFG_ABILITY           = 0x0008,    //能力集信息
	VCA_GET_CFG_RULE_LIST_EX      = 0x0009,    //扩展规则列表
	VCA_GET_CFG_RULE_LIST_V3      = 0x000A,    //扩展规则列表(SMD全功能)
	VCA_GET_CFG_UNITE_RULE_LIST   = 0x000B,    //组合规则链表
	VCA_GET_CFG_ROI_REGION		  = 0x000C,    //ROI区域
	VCA_GET_CFG_VERSION_V2        = 0x000D,    //版本信息V2,对应VCA_LIB_VERSION_V2，修订版本号：0-99基线版本、100-199PK版本、200-299定制版本 

	//IAS专用配置信息
	VCA_GET_CFG_CHANNEL_MODE      = 0x2001,    //通道类型
	VCA_GET_CFG_ENTER_REGION      = 0x2002,    //进入区域
	VCA_GET_CFG_MICRO_DETECTOR    = 0x2003,    //微波传感器

    //IPS 专用配置信息
    VCA_GET_CFG_COLOR_INFO        = 0x5001,    //取色
	VCA_GET_CFG_AUX_AREA_LIST     = 0x5002,    //辅助区域设置
	
	// IBV 专用配置信息
    VCA_GET_CFG_BV_CALIB_PARAM    = 0x6001,    //双目相机标定参数
    VCA_GET_CFG_BV_FIX_PARAM      = 0x6002,    //双目相机安装参数
    VCA_GET_CFG_BV_PTZ_CALIB_PARAM = 0X6003,   //双目联动球机安装参数

	//ITS 专用配置信息
	VCA_GET_CFG_REFER_REGION      = 0x7001,
	VCA_GET_CFG_LANE              = 0x7002,
	VCA_GET_CFG_SCENE_PARAM       = 0x7003,
	VCA_GET_CFG_TPS_RULE          = 0x7004,    //获取参数规则
	VCA_GET_CFG_AID_RULE          = 0x7005,   //获取事件规则
	VCA_GET_CFG_AID_RULE_EX       = 0x7006,    //获取带方向规则

    //开放平台 专用配置信息
	VCA_GET_CFG_JSON_FILE         = 0x8001,    //获取JSON文件配置信息

	//HVA 专用配置
	VCA_GET_CFG_PARAM_INFO		  = 0x9001,	   //获取配置字符串参数信息
	VCA_GET_CFG_DAT_INFO		  = 0x9002,    //获取序列化参数信息
	VCA_GET_CFG_GLOBAL_INFO		  = 0x9003,    //获取全局参数信息
	VCA_GET_CFG_NODE_INFO		  = 0x9004     //获取节点参数信息

}VCA_GET_CFG_TYPE;

//子函数操作类型
typedef enum _VCA_SUBFUNC_TYPE_
{
	//通用子处理类型
	VCA_SUBFUNC_GETREALVALUE                = 0x0001,   //标定校验
    VCA_SUBFUNC_HSR_FRAME_EVT               = 0x0002,   //输入target_list和rule，得到报警事件(以帧为单位处理)
    VCA_SUBFUNC_HSR_OBJECT_EVT              = 0x0003,   //输入一个目标的所有轨迹点和rule，得到报警链表(以目标为单位处理)
    VCA_SUBFUNC_META_GETINFO                = 0x0004,   //获得metadata中的某个数据 
		
	//IVS
	VCA_SUBFUNC_IMAGEPOS2PTZ                = 0x1001,   // 将图像坐标转换成PTZ坐标
	VCA_SUBFUNC_GETPTZVALUE                 = 0x1002,   // 获取球机跟踪位置
	VCA_SUBFUNC_IMAGERECT2PTZ           = 0x1003,   //将图像坐标转换成PTZ坐标
	VCA_SUBFUNC_FEATURE_PT_MATCH        = 0x1004,   //枪机球机图像匹配，输入图像地址，图像宽高，输出两幅图像特征匹配归一化位置
	VCA_SUBFUNC_IMAGEPOS2PTZ_ID         = 0x1005,	//根据球机ID将图像坐标转换成PTZ坐标
	VCA_SUBFUNC_IMAGERECT2PTZ_ID        = 0x1006,	//根据球机ID将图像坐标转换成PTZ坐标
	VCA_SUBFUNC_CNN_CHECK_FAKE          = 0x1007,	//CNN去误报
	VCA_SUBFUNC_KILL_TRAGET_TRACK	    = 0x1008,	//强制停止某个目标的跟踪
	VCA_SUBFUNC_SET_CANCEL_DOME		    = 0x1009,	//注销球机
    
    //鹰眼处理
    VCA_SUBFUNC_EAGLE_EYE_EVENT_PROCESS     = 0x100A,   //鹰眼事件判断
    VCA_SUBFUNC_EAGLE_EYE_GETPTZVALUE       = 0x100B,   //鹰眼获取跟踪目标的球机PTZ坐标
    VCA_SUBFUNC_EAGLE_EYE_SET_CANCEL_DOME   = 0x100C,   //鹰眼注销某个枪机
    VCA_SUBFUNC_EAGLE_EYE_KILL_TRAGET_TRACK = 0x100D,	//鹰眼强制停止某个目标的跟踪
    VCA_SUBFUNC_EAGLE_EYE_IMAGEPOS2PTZ      = 0x100E,   //鹰眼将图像坐标转换成PTZ坐标
    VCA_SUBFUNC_EAGLE_EYE_IMAGERECT2PTZ     = 0x100F,   //鹰眼将图像坐标转换成PTZ坐标
    VCA_SUBFUNC_EAGLE_EYE_FEATURE_PT_MATCH  = 0x1020,   //枪机球机图像匹配，输入图像地址，图像宽高，输出两幅图像特征匹配归一化位置
	
	//人脸检测
	VCA_SUBFUNC_PROC_IMG                    = 0x4001,   // 单幅灰度图像人脸检测
	VCA_SUBFUNC_GET_EXPO_INFO               = 0x4002,   // 根据人脸区域信息计算曝光信息
	VCA_SUBFUNC_FACIAL_FEATURE_LOCATE       = 0x4003,   // 人脸特征点定位（检测库）
    VCA_SUBFUNC_DETECT_FACE_FROM_ROI        = 0x4004,   // 在图像roi区域检测人脸
    VCA_SUBFUNC_DETECT_FACE_FROM_HMS_ROI    = 0x4005,   // 从头检测检测结果中检测人脸
	VCA_SUBFUNC_RELEASE_HASI_HANDLE         = 0x4006,   // 释放海思函数占用资源

    // 异常人脸相关处理
    VCA_SUBFUNC_DETECT_COMPONENT            = 0x4006,   // 人脸部件检测
    VCA_SUBFUNC_DETECT_HEAD		            = 0x4007,   // 人头检测

    // 人脸识别
    VCA_SUBFUNC_BUILD_MODEL                 = 0x4101,   // 人脸建模
    VCA_SUBFUNC_FEATURE_POINT_LOCATE        = 0x4102,   // 人脸特征点定位（识别库）
    VCA_SUBFUNC_IMAGE_QUALITY_ASESSMENT     = 0x4103,   // 人脸质量评分判断
	VCA_SUBFUNC_FRONTAL_FACE_DETECT         = 0x4104,   // 准正面人脸检测（识别库）
    VCA_SUBFUNC_SIMILARITY_CUT_OFF          = 0x4105,   // 相似度截断
    VCA_SUBFUNC_SEMANTIC_SIMILARITY         = 0x4106,   // 语义相似度校正

    // 属性识别
    VCA_SUBFUNC_ESTIMATE_STATURE            = 0x4201,   // 身高估计
    VCA_SUBFUNC_BINOCULAR_CALIB             = 0x4202,   // 双目姿态角度标定
	VCA_SUBFUNC_BINOCULAR_CALIB_CHECK       = 0x4203,   // 双目标定检验
	VCA_SUBFUNC_BINOCULAR_PTZ_CALIB         = 0x4204,   // 双目联动球机联动标定
	VCA_SUBFUNC_BINOCULAR_PTZ_CHECK         = 0x4205,   // 双目联动球机联动标定校验
	VCA_SUBFUNC_BINOCULAR_PTZ_TUNING        = 0x4206,   // 双目联动球机联动标定微调

	//人脸检测 TI DM385、8127等平台FD使用参数
    VCA_SUBFUNC_FD385_STABILIZE             = 0x4301,   // FD模块检测结果稳定化操作,目前版本要求输入、输出为同一个Buf;
    VCA_SUBFUNC_FD385_FILTER                = 0x4302,   // FD模块检测结果过滤操作;

    VCA_SUBFUNC_AUDIO_DEAL                  = 0x5001,   // 声音子处理函数
	VCA_SUNFUNC_CHECK_VEHICLE               = 0x6001    // 校验报警
}VCA_SUBFUNC_TYPE;


/***********************************************************************************************************************
*索引数据打包结构体定义
***********************************************************************************************************************/
typedef struct _VCA_INDEX_GOP_HEADER_
{
    unsigned int    tag;              //标示tag位，为“VIGH”
    unsigned int  	version;          //版本号
    unsigned int    length;           //整个GOP的数据长度，包括多个索引数据和此GOP头
    unsigned int    time_s;           //前端绝对时间，单位秒
    unsigned short  time_ms;          //前端绝对时间，单位毫秒
	unsigned char   reserved[2];	  //保留字节，用于四字节对齐
    unsigned int    time_interval_ms; //GOP持续时间，单位毫秒
}VCA_INDEX_GOP_HEADER;
	
#ifdef __cplusplus
}
#endif 

#endif /* _HIK_VCA_COMMOM_H_ */

