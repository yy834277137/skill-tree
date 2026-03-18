/***********************************************************************************************************************
* 版权信息：版权所有(c) 2012-2013, 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: vca_lib_def.h
* 文件标识:
* 摘    要:统一框架算法库通用定义

* 当前版本: 1.1.0
* 作    者: 车军
* 日    期: 2018-12-8
* 备    注:1 更新Proc_info，删除部分无意义指针（指针必须声明num，否则以结构体参数形式定义）
           2 在Proc_info中增加metadata输出参数，并支持多平台数据；
		   3 新增报警输出规则结构体，为配置规则简化版定义
*
* 当前版本: 1.0.0
* 作    者: 车军
* 日    期: 2018-5-29
* 备    注:
***********************************************************************************************************************/
#ifndef _HIK_VCA_LIB_COMMOM_H_
#define _HIK_VCA_LIB_COMMOM_H_

#include "vca_base.h"
#include "vca_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VCA_RULE_MAX_REGION_NUM	    8             // 每个规则中支持的区域数量
#define VCA_MAX_APP_NUM            32             // 一个算法库支持最大的应用数量（回调）
#define VCA_EVENT_OBJ_STAT_NUM     16             // 事件触发条件数量
#define VCA_EVENT_SUB_TARGET_NUM     8            //子对象数量
#define VCA_MAX_SINNGLE_RULE_NUM_FOR_UNITE  4     // 构成组合规则的最多单一规则数量
#define VCA_MIN_SINNGLE_RULE_NUM_FOR_UNITE  2     // 构成组合规则的最少单一规则数量

/******************************************************************************
初始化信息定义
******************************************************************************/
// 应用类型
typedef enum _VCA_PRODUCT_TYPE_
{
	// 基线
	VCA_PRODUCT_OBD = 0x00000001,
	VCA_PRODUCT_BGS = 0x00000002,
	VCA_PRODUCT_LPR = 0x00000003,

	VCA_PRODUCT_CLS = 0x00000004,
	VCA_PRODUCT_GIR = 0x00000005,
	VCA_PRODUCT_PA = 0x00000006,
	VCA_PRODUCT_AVR = 0x00000007,
	VCA_PRODUCT_VFE = 0x00000008,
	VCA_PRODUCT_TSC = 0x00000009,
	VCA_PRODUCT_TRK = 0x0000000A,
	VCA_PRODUCT_MPR = 0x0000000B,
	VCA_PRODUCT_IED = 0x0000000C,
	VCA_PRODUCT_CTA = 0X0000000D,
	VCA_PRODUCT_RLA = 0X0000000E,

	// 行业
	VCA_PRODUCT_CSA = 0X01000000,
	VCA_PRODUCT_HBA = 0X01000001,			     // 
	VCA_PRODUCT_IPS = 0X01000002,				 // 	
	VCA_PRODUCT_IAS = 0X01000003,				 // 
	VCA_PRODUCT_IEC = 0X01000004,				 // 
	VCA_PRODUCT_IESTCH = 0X01000005,				 // 
	VCA_PRODUCT_ICS = 0X01000007,				 // 
	VCA_PRODUCT_MES = 0X01000008,				 // 
	VCA_PRODUCT_PDC = 0X01000009,				 // IED库POS
	VCA_PRODUCT_SHA = 0X0100000A,
	VCA_PRODUCT_WLG = 0X0100000B,
	VCA_PRODUCT_XSI = 0X0100000C,


	VCA_PRODUCT_END = VCA_ENUM_END_V2
}VCA_PRODUCT_TYPE;


typedef enum _VCA_IMG_PROC_MODE_
{
	VCA_IMG_PROC_GIA = 1,                                 //使用GPU图像处理算法
	VCA_IMG_PROC_CIA = 2,                                 //使用CPU图像处理算法
	VCA_IMG_PROC_HIA = 3,                                 //使用HISI芯片图像处理算法
	VCA_IMG_PROC_MIA = 4,                                 //使用MA芯片图像处理算法
	VCA_IMG_PROC_END = VCA_ENUM_END_V2
}VCA_IMG_PROC_MODE;

//创建算法库时目标参数（控制内部内存分配）
typedef struct _VCA_MEM_OBJ_PARAM_
{
	unsigned short			max_object_num;				  // 最大支持目标数量
	unsigned short          max_component_num;            // 最大支持目标组件数量
	unsigned short          max_attr_num;                 // 最大支持目标属性数量
	unsigned short          max_keypt_num;                // 最大支持特征点数量
	unsigned short          max_traj_num;                 // 最大支持轨迹长度
	char					reserved[52];				  // 保留字节

	VCA_MAP                 feat_info;                    // 特征图信息

}VCA_MEM_OBJ_PARAM;

//创建算法库时规则参数（控制内部内存分配）
typedef struct _VCA_MEM_RULE_PARAM_
{
	unsigned short			max_rule_num;				  // 最大支持规则数量
	char					reserved[30];				  // 保留字节
}VCA_MEM_RULE_PARAM;


//创建算法库图像参数
typedef struct _VCA_MEM_IMG_PARAM_
{
	unsigned char       stich_mode;						   // 图像拼帧模式，参考FRCNN中拼帧模式
	unsigned char       in_frame_num;                      // 输入图像数量
	unsigned char       batch_size;                        // 单次处理batch数量
	unsigned char       img_proc_mode;					   // 图像处理模式
	char                reserved[12];
	VCA_MEDIA_FRAME     src_frame;  					   // 输入最大图像信息
}VCA_MEM_IMG_PARAM;

typedef struct _VCA_MEM_PLAT_PARAM_
{
	unsigned int    plat_type;
	void           *plat_param;
}VCA_MEM_PLAT_PARAM;

//回调函数注册
typedef struct _VCA_PRODUCT_APP_FUNC_
{
	unsigned int  product_type;
	unsigned int  reseved[7];
	unsigned int(*callback_getmem)(void         *mem_param,                          // GetmemSizse
								   unsigned int  mem_param_size,
								   void         *mem_tab);

	unsigned int(*callback_create)(void         *mem_param,                          // Create 
						           unsigned int  mem_param_size,
						           void         *mem_tab,
						           void          **handle);

	unsigned int(*callback_process)(void         *handle,
									void         *proc_in_buf,
									unsigned int  proc_in_buf_size,
									void         *proc_out_buf,
									unsigned int  proc_out_buf_size);

	unsigned int(*callback_set_config)(void		 *handle,
									  int		  config_type,
									  void		 *config_buf,
							          int		  buf_size);
	unsigned int(*callback_get_config)(void		 *handle,
									   int		  config_type,
									   void		 *config_buf,
									   int		  buf_size);
	unsigned int(*callback_release)(void		 *handle);
}VCA_PRODUCT_APP_FUNC;

//回调函数列表
typedef struct _VCA_PRODUCT_APP_LIST_
{
	unsigned int            product_num;
	VCA_PRODUCT_APP_FUNC    product_func[VCA_MAX_APP_NUM];

}VCA_PRODUCT_APP_LIST;

typedef struct _VCA_MEM_PARAM_V3_
{
	unsigned int            product_type;					// 产品类型
	unsigned int            process_type;                   // 功能类型
	void                   *plat_handle;				    // GPU平台为cuda句柄，HISI平台为schedule_handle
	void				   *model_handle;				    // 模型句柄
	VCA_MEM_OBJ_PARAM		obj_param;					    // 目标参数
	VCA_MEM_RULE_PARAM		rule_param;	                    // 规则参数
	VCA_MEM_IMG_PARAM       img_param;                      // 图像参数
	VCA_MEM_PLAT_PARAM      plat_param;                     // 平台相关参数
	VCA_PRODUCT_APP_LIST    procduct_app;                   // 应用app链表
	void				   *config_info;                    // 配置文件信息
	char					reserved[56];
} VCA_MEM_PARAM_V3;



/***********************************************************************************************************************
配置信息定义
***********************************************************************************************************************/
//区域用途
typedef enum _VCA_REGION_PURP_
{
	VCA_REGION_RULE          = 0x00000001,        // 规则区域
	VCA_REGION_LANE          = 0x00000002,        // 车道区域
	VCA_REGION_SHIELD        = 0x00000003,        // 屏蔽区域
	VCA_REGION_REFERENCE     = 0x00000004,        // 参考/辅助区域
	VCA_REGION_TRAFFIC_LIGHT = 0x00000005,        // 交通信号灯
	VCA_REGION_UNITE_RULE    = 0x00000006,        // 规则区域
	VCA_REGION_PURP_END      = VCA_ENUM_END
}VCA_REGION_PURP;

//子区域用途
typedef enum _VCA_SUB_REGION_PURP_
{
	//交通专用0x01～0x7F
	VCA_LANE_TOP_LINE       = 0x01,                 // 车道上边界
	VCA_LANE_BOTTOM_LINE    = 0x02,                 // 车道下边界
	VCA_LANE_LEFT_LINE      = 0x03,                 // 车道左边界
	VCA_LANE_RIGHT_LINE     = 0x04,                 // 车道右边界

	VCA_PR_REIGION          = 0x10,                 // 车牌识别区域

	//行业专用0x80～0x9F
	VCA_COLOR_REGION        = 0x80,                  // 取色区域
	VCA_SUB_REGION_PURP_END = VCA_ENUM_END
}VCA_SUB_REGION_PURP;

//配置信息
typedef struct _VCA_CONFIG_INFO_V2_
{
	unsigned int			product_type;				 // 产品类型
	unsigned int			config_type;				 // 参考VCA_SET_CFG_TYPE和VCA_SET_GFG_TYPE
	unsigned int			info_size;					 // 配置内存尺寸
	unsigned char			reserved[16];   
	void					*config_info;
}VCA_CONFIG_INFO_V2;


//规则区域
typedef struct _VCA_RULE_REGION_
{
	unsigned char          id;								   // 是否有效
	unsigned char          valid;							   // 是否有效
	unsigned char          move_dir;                           // 参考VCA_CROSS_DIRECTION
	unsigned char          region_purp;                        // 子区域用途，参考VCA_SUB_REGION_PURP
	unsigned char          reserved[12];					   // 保留字段
	VCA_REGION             region;							   // 顶点 
}VCA_RULE_REGION;

typedef struct _VCA_RULE_REGION_LIST_
{
	unsigned char			region_num;
	unsigned char			reserved[7];
	VCA_MAP					region_mask;						// 规则区域掩码
	VCA_RULE_REGION			region[VCA_RULE_MAX_REGION_NUM];
}VCA_RULE_REGION_LIST;

typedef struct _VCA_DET_REGION_
{
	VCA_MAP					region_mask;						// 规则区域掩码
	VCA_RULE_REGION			region;
}VCA_DET_REGION;

// 触发判断条件
typedef enum _VCA_TRIGGER_JUDGE_CONDITION_
{
	VCA_TRG_LESS_MIN           = 0x01,						    //  小于等于最小值
	VCA_TRG_GREATER_MAX        = 0x02,                          //  大于等于最大值
	VCA_TRG_BETWEEN_MIN_MAX    = 0x03,                          //  大于最小值且小于最大值	
	VCA_TRG_NONBETWEEN_MIN_MAX = 0x04,                          //  小于等于最小值或者大于等于最大值

}VCA_TRG_JUDGE_CONDITION;

// 触发判断条件
typedef enum _VCA_RELATION_TRIGGER_TYPE_
{
	VCA_RELATION_TRIGGER_MAIN_OBJ		  = 0x01,				 //  主对象单独报警
	VCA_RELATION_TRIGGER_MAIN_AND_SUB_OBJ = 0x02,                //  主对象和子对象联合报警
	VCA_RELATION_COUNT                    = 0x03,                //  关联统计
}VCA_RELATION_TRIGGER_TYPE;

//规则触发条件值
typedef struct _VCA_STATE_CONDITION_PARAM_
{
	unsigned char				valid;						        // 是否启用标志位
	unsigned int				status_type;                        // 状态类型
	unsigned int				status_val;                         // 状态值
	unsigned int				model_tube_length;                  // 分类模型tube长度[1,24]
	unsigned char               rerserved[20];                   
}VCA_STATE_CONDITION_PARAM;

//规则触发条件值
typedef struct _VCA_NUMBER_CONDITION_PARAM_
{
	unsigned char				valid;						       // 是否启用标志位
	unsigned int				min_trigger_num;                   // 触发最小目标数量
	unsigned int				max_trigger_num;                   // 触发最大目标数量
	unsigned char               rerserved[24];
	VCA_TRG_JUDGE_CONDITION     judge_condition;                   // 判断条件

}VCA_NUM_CONDITION_PARAM;

//规则触发条件值
typedef struct _VCA_RATIO_CONDITION_PARAM_
{
	unsigned char				valid;						                    
    float                       min_trigger_num;                   // 触发最小目标数量
	float 				        max_trigger_num;                   // 触发最大目标数量
	unsigned char               rerserved[24];
	VCA_TRG_JUDGE_CONDITION     judge_condition;  // 判断条件
}VCA_RATIO_CONDITION_PARAM;


//屏蔽类别参数
typedef struct  _VCA_FILTER_PARAM_
{
	unsigned int             	obj_type;							 // 目标类型
	unsigned int                obj_status_num;                      // 触发条件数量
	VCA_STATE_CONDITION_PARAM	obj_status[VCA_EVENT_OBJ_STAT_NUM];	     // 目标状态
}VCA_FILTER_PARAM;

//静止过滤参数 ADD FROM HMY
typedef struct  _VCA_STILL_FILTER_PARAM_
{
	//unsigned int             	obj_type;							 // 目标类型
	unsigned char				valid;						       // 是否启用标志位
	unsigned int				sensitive;                     // 灵敏度
}VCA_STILL_FILTER_PARAM;

//屏蔽条件类别列表
typedef struct  _VCA_FILTER_CONDITION_LIST_
{
	unsigned int     filter_num;
	VCA_FILTER_PARAM filter_param[VCA_EVENT_OBJ_STAT_NUM];
}VCA_FILTER_CONDITION_LIST;

typedef struct  _VCA_SUB_TARGET_INFO_
{
	unsigned int             	obj_type;							 // 目标类型
	unsigned int                obj_status_num;                      // 触发条件数量
	VCA_STATE_CONDITION_PARAM	obj_status[VCA_EVENT_OBJ_STAT_NUM];  // 目标状态
}VCA_SUB_TARGET_INFO;

//关联分析结构体
typedef struct  _VCA_RELATION_ANALYSIS_PARAM_
{
	unsigned char				valid;										// 是否启用标志位
	unsigned char				analysis_type;								// 分析类型
	unsigned int				sub_target_num;								// 子对象数量
	VCA_SUB_TARGET_INFO         sub_target_info[VCA_EVENT_SUB_TARGET_NUM];  // 子对象信息
}VCA_RELATION_ANALYSIS_PARAM;

//规则触发条件
typedef struct _VCA_RULE_CONDITION_
{	 
	unsigned char				valid;						         // 是否启用标志位
	unsigned int             	obj_type;							 // 目标类型
	unsigned int                obj_status_num;                      // 触发条件数量
	unsigned short              cls_clip_num;			             // 状态分类每次需要的图片数或者视频clip数，范围: [0, 100]    
	unsigned short              trigger_interval;		             // 状态触发分类间隔时间
	unsigned char               rerserved[19];
	VCA_STATE_CONDITION_PARAM	obj_status[VCA_EVENT_OBJ_STAT_NUM];	 // 目标状态
	VCA_NUM_CONDITION_PARAM     num_condition;                       // 触发目标判断条件
	VCA_RATIO_CONDITION_PARAM   ratio_condition;                      // 触发目标判断条件
	VCA_FILTER_CONDITION_LIST   filter_list;                        //过滤目标参数
	VCA_RELATION_ANALYSIS_PARAM relation_analysis_condition;        //关联分析条件
	VCA_STILL_FILTER_PARAM		still_filter;						//触发目标静止过滤参数 add from hmy

}VCA_RULE_CONDITION;


// 区域规则结构体
typedef struct _VCA_RULE_INFO_V4_
{
	unsigned char			 id;							    // 规则ID
	unsigned char            rule_order;                        // 规则触发顺序
	unsigned char			 valid;						        // 是否有效
	unsigned char            b_update;                          // 是否更新（0-不更新， 1-更新）
	unsigned char            reserved[60];
	unsigned int			 product_type;			            // 确定启用那个算法
	unsigned int             event_type;                        // 事件类型

	VCA_RULE_CONDITION       condition;						    // 触发条件	
	VCA_RULE_PARAM           rule_param;				        // 参数指针 参考vca_common.h
	VCA_RULE_REGION_LIST     region_list;				        // 多边形or折线
	VCA_INFO                 add_info;                          // 规则附加信息(保留扩展能力)
	
}VCA_RULE_INFO_V4;

// 组合规则内各单一规则的触发间隔
typedef struct _VCA_UNITE_BASE_ID_
{
	unsigned char		     single_id;								// 单一规则ID
}VCA_UNITE_BASE_ID;

// 组合规则内各单一规则的触发间隔
typedef struct _VCA_UNITE_TIME_INTERVAL_
{
	unsigned short		     min_interval;						      // 最小触发间隔
	unsigned short		     max_interval;						      // 最大触发间隔

}VCA_UNITE_TIME_INTERVAL;

//组合规则触发条件
typedef struct _VCA_UNITE_TRG_CONDITION_
{
	unsigned char				   max_trigger_valid;													// 最大触发次数是否有效 [0,1] 0-配置最大触发次数不起作用,1-起作用
	unsigned int				   max_trigger_num;														// 最大触发次数，范围: [1, 100]
	unsigned int				   alert_interval;														 // 报警间隔，范围: [0, 1800]秒，大于1时有效
	unsigned int             	   unite_type;															// 触发类型	
	unsigned int				   base_id_num;															// 单一规则数量数量
	VCA_UNITE_BASE_ID			   base_id[VCA_MAX_SINNGLE_RULE_NUM_FOR_UNITE];							// 单一规则ID
	VCA_UNITE_TIME_INTERVAL        single_rule_time_interval[VCA_MAX_SINNGLE_RULE_NUM_FOR_UNITE - 1];	// 组合规则内各单一规则的触发间隔
}VCA_UNITE_TRG_CONDITION;

//组合规则报警输出
typedef struct _VCA_UNITE_ALERT_INFO_
{
	unsigned int				   base_id_num;															// 单一规则数量数量
	VCA_UNITE_BASE_ID			   base_id[VCA_MAX_SINNGLE_RULE_NUM_FOR_UNITE];							// 单一规则ID
}VCA_UNITE_ALERT_INFO;

// 关联目标信息
typedef struct _VCA_ALERT_RELATION_INFO_
{
	unsigned int			blob_relation_num;						// 关联目标数量
	unsigned int		    blob_relation_id[VCA_MAX_OBJ_NUM];      // 每个关联目标的id
}VCA_ALERT_RELATION_INFO;


// 数量报警条件中各目标ID信息
typedef struct _VCA_ALERT_ID_FOR_NUMBERCONDITION_
{
	unsigned int			blob_for_numbercondition_num;					// 触发数量报警条件的目标数量
	unsigned int		    blob_for_numbercondition_id[VCA_MAX_OBJ_NUM];   // 每个目标的id
}VCA_ALERT_ID_FOR_NUMBERCONDITION;

// 组合区域规则结构体
typedef struct _VCA_UNITE_RULE_INFO_V4_
{
	unsigned char			 id;							    // 规则ID
	unsigned char			 valid;						        // 是否有效
	unsigned char            b_update;                          // 是否更新（0-不更新， 1-更新）
	unsigned char            reserved[60];
	unsigned int			 product_type;			            // 确定启用那个算法
	unsigned int             event_type;                        // 事件类型

	VCA_UNITE_TRG_CONDITION  unite_condition;			        // 触发条件	
	VCA_RULE_PARAM		     unite_rule_param;				     // 参数指针 参考vca_common.h
}VCA_UNITE_RULE_INFO_V4;
//对外输出的规则信息
typedef struct _VCA_OUTPUT_RULE_INFO_
{
	unsigned char			 id;                                // 规则ID
	unsigned char			 reserved[55];
	unsigned int			 product_type;			            // 确定启用那个算法
	unsigned int             event_type;                        // 事件类型
	VCA_RULE_REGION_LIST     region_list;                       // 规则区域链表
}VCA_OUTPUT_RULE_INFO;

// 区域规则链表结构体
typedef struct _VCA_RULE_LIST_V4_
{
	unsigned char		     rule_num;						 // 规则区域数量
	unsigned char		     unite_rule_num;				 // 组合规则区域数量
	unsigned char            reserved[10];
	VCA_RULE_INFO_V4		 *rule_Info;						 // 规则区域链表
	VCA_UNITE_RULE_INFO_V4	 *unite_rule_Info;				 // 组合规则区域链表
	VCA_DET_REGION           det_region;                     //检测区域
}VCA_RULE_LIST_V4;



/***********************************************************************************************************************
处理输出信息定义
***********************************************************************************************************************/

//输出区域状态类型（需要和规则功能配置类型相同，还需讨论）
typedef enum _VCA_OUT_REGION_STAT_
{
	VCA_OUT_TRAFFIC_DATA		= 0x00000001,                      // 交通数据
	VCA_OUT_REGION_COUNT		= 0x00000002,                      // 区域目标数量统计
	VCA_OUT_CROSS_COUNT			= 0x00000003,                      // 跨线目标数量统计
	VCA_OUT_QUEUE_STAT			= 0x00000004,                      // 区域队列状态
	VCA_OUT_AUDIO_DB			= 0x00000005,                      // 音频分贝
	VCA_OUT_CROWD_DENSITY       = 0x00000006,                      // 人员密度
	VCA_OUT_OVERLAP_RATIO       = 0x00000007,                      // 目标占规则区域交叠比
	VCA_OUT_REGION_END          = VCA_ENUM_END
}VCA_REGION_OUT_STAT;

//交通数据
typedef struct _VCA_TRAFFIC_DATA_
{
	unsigned char			loop_trig;					           // 当前车道线圈0状态（0无效，1到达，2离开）
	unsigned char			loop_vel;					           // 当前车道速度，0无效，其他为有效速度
	unsigned char			congestion_deg;				           // 拥堵度，0无效，1～100为有效拥堵度
	unsigned char			vehicle_type;				           // 车辆类型，0-小型车，1-中型车，2-大型车
	unsigned char			queue_length;				           // 队列长度，0无效，1～255为有效排队长度值，单位m
	unsigned char			time_occupy;				           // 时间占有率，0,100
	unsigned char			space_occupy;				           // 空间占有率，0,100
	unsigned char			loop_trig1;					           // 当前车道线圈1状态（0无效，1到达，2离开）
	unsigned char			obj_dir;					           // 车辆到达时正背向，0 未知，1 正向， 2背向
	unsigned char			obj_dir1;
	unsigned char			reserved[54];				           // 保留字节 64 - 10 = 54
}VCA_TRAFFIC_DATA;

typedef enum _VCA_REGION_DENSITY_LEVEL_
{
	VCA_REGION_DENSITY_SPARSE		= 0x1,				          // 稀疏 
	VCA_REGION_DENSITY_LIGHT		= 0x2,				          // 轻度
	VCA_REGION_DENSITY_MID			= 0x3,				          // 中度
	VCA_REGION_DENSITY_HEAVY		= 0x4,				          // 重度
	VCA_REGION_DENSITY_END = VCA_ENUM_END
}VCA_REGION_DENSITY_LEVEL;


//区域计数
typedef struct _VCA_REGION_COUNT_
{
	unsigned int				target_num;                      // 区域目标数
	unsigned char				density_level;                   // 区域密度等级,参考VCA_REGION_CROWND_LEVEL
	unsigned char				density_ratio;                   // 区域密度比例(0~100)
	unsigned char				reserved[58];
}VCA_REGION_COUNT;

//跨线计数
typedef struct _VCA_CROSS_COUNT_
{
	unsigned int				enter_num;
	unsigned int				leave_num;
	unsigned char				reserved[56];
}VCA_CROSS_COUNT;

//队列状态
typedef struct _VCA_QUEUE_STAT_
{
	unsigned short				queue_lenth;   				    // 排队人数
	unsigned short				wait_time;     				    // 排队时间（单位：秒）
	unsigned char				reserved[60];
}VCA_QUEUE_STAT;

//人群密度估计输出热度图结构体
typedef struct _VCA_CDE_PARAM_
{
	unsigned short		        heat_map_w;					   // 输出热度图宽度
	unsigned short		        heat_map_h;					   // 输出热度图高度
	unsigned short  	        crowd_num;					   // 输出人数
	unsigned char	           *heat_map;					   // 输出热度图
	unsigned char		        reserved[54];
}VCA_CDE_PARAM;

//感兴趣目标占规则区域交叠比结构体
typedef struct _VCA_OVERLAP_PARAM_
{
	float                   overlap_ratio;                    // 输出感兴趣目标占规则区域交叠比
	unsigned char		    reserved[60];
}VCA_OVERLAP_PARAM;

//区域状态参数
typedef union _VCA_RULE_STAT_PARAM_
{
	char						reserved[64];
	VCA_TRAFFIC_DATA			traffic_data;
	VCA_REGION_COUNT			region_count;
	VCA_QUEUE_STAT				queue_stat;
	PDC_COUNTER_INFO            pdc_counter;
	VCA_CDE_PARAM               cde_param;
	VCA_OVERLAP_PARAM           overlap_param;
}VCA_RULE_STAT_PARAM;

// 处理区域状态信息
typedef struct _VCA_RULE_STAT_INFO_
{
	unsigned char				region_id;                        // 对应区域ID
	unsigned char				stat_type;                        // VCA_REGION_OUT_STAT
	unsigned int                product_type;					  // 产品类型
	unsigned char				reserved[14];
	VCA_RULE_STAT_PARAM			stat_param;					      //详细状态key有个行业自行定义
}VCA_RULE_STAT_INFO;

// 处理区域状态链表
typedef struct _VCA_RULE_STAT_LIST_
{
	unsigned int				region_num;				          // 区域状态数
	VCA_RULE_STAT_INFO	       *region_state;                     // 区域状态数组
}VCA_RULE_STAT_LIST;


// 报警信息结构体
typedef struct _VCA_RULE_ALERT_INFO_
{

	unsigned int					  alert_level;				          // 报警等级，报警置信度
	VCA_BLOB_BASIC_INFO				  alert_blob;                         // 报警触发目标
	VCA_OUTPUT_RULE_INFO			  alert_region;                       // 报警规则区域链表
	VCA_ALERT_ID_FOR_NUMBERCONDITION  *alert_id_for_numbercondition;      //数量报警条件中各目标ID信息(只读，最多64个目标)
	VCA_MEDIA_LIST					  media_list;                          // 报警图片
	VCA_UNITE_ALERT_INFO			  unite_alert_info;                    // 组合报警信息（组合报警中没有alert_blob、alert_region信息，单一报警中无unite_condition信息）
	VCA_ALERT_RELATION_INFO			  *alert_relation_info;                // 关联目标信息（只读，最多64个目标）
	unsigned int                      relation_id;                        // 编排关联目标ID
	unsigned int					  reserved[15];
}VCA_RULE_ALERT_INFO;

typedef struct _VCA_RULE_ALERT_LIST_
{
	unsigned int			 alert_num;				             // 区域状态数
	VCA_RULE_ALERT_INFO	    *alert;                              // 区域状态数组
}VCA_RULE_ALERT_LIST;


// 算法库调试POS信息
typedef struct _VCA_POS_INFO_
{	
	unsigned int		    valid;					              // 是否启用	
	unsigned int		    pos_size;				              // POS大小
	unsigned int	        product_type;				          // POS类型，VCA_PRODUCT_MODULE
	void				   *pos_buf;					          // POS内存块指针
}VCA_POS_INFO;


#define  VCA_MAX_METADATA_NUM        16                            // metadata数量

//metadata类型
typedef enum _VCA_METADATA_TYPE_
{
	VCA_METADATA_NORM     = 1,                                     // 通用metadata                                  
	VCA_METADATA_DEBUG    = 2,                                     // 调试信息                                     
}VCA_METADATA_TYPE;

typedef struct _VCA_METADATA_INFO_
{
	unsigned int		    valid;					               // 是否启用	
	unsigned int		    metadata_size;				           // metadata
	VCA_MEM_PLAT	        plat_type;				               // VCA_MEM_PLAT
	VCA_METADATA_TYPE       metadata_type;                         // VCA_METADATA_TYPE
	unsigned int            reserved[8];                   
	void				   *data;					               // metadata数据
}VCA_METADATA_INFO;

//调试信息列表
typedef struct _VCA_POS_LIST_
{
	unsigned int           pos_num;
	VCA_POS_INFO          *pos_info[VCA_MAX_APP_NUM];
}VCA_POS_LIST;

// 公共输入信息结构体

typedef struct _VCA_PROC_INFO_
{
	unsigned char             reserved[20];
	unsigned int              product_type;                         // 产品类型
	unsigned int              process_type;                         // 功能类型	
	VCA_MEDIA_LIST            media_list;                           // 输入图像信息

	unsigned int              blob_list_num;                        // 目标链表数量
	VCA_BLOB_INFO_LIST       *blob_list;                            // 目标链表

	unsigned int              metadata_num;                         // metadata数量
	VCA_METADATA_INFO         metadata[VCA_MAX_METADATA_NUM];       // metadata信息
	

	VCA_MAP_LIST              map_list;		                        // 处理所需的图(前景、帧差、光流等)
	VCA_RULE_ALERT_LIST       alert_list;                           // 报警信息链表
	VCA_RULE_STAT_LIST        region_state;			                // 区域状态链表

	VCA_POS_LIST              pos_list;

}VCA_PROC_INFO;





/***********************************************************************************************************************
* 功  能：获取HMS库所需内存大小
* 参  数：IN:
*         mem_param					     - I   内存参数结构(图像的分辨率/帧率等信息)
*		  mem_param_size 				 - I   参数大小
*		  mem_tab 						 - I   内存分配表
* 返回值：返回错误码
***********************************************************************************************************************/
int VCA_GetMemSize(VCA_MEM_PARAM_V3    *mem_param,
	               unsigned int         mem_param_size,
				   VCA_MEM_TAB_V3       mem_tab[VCA_MAX_MEM_TAB_NUM]);

/***********************************************************************************************************************
* 功  能：创建HMS库
* 参  数：IN:
*		  mem_param						 - I   内存参数结构体
*		  mem_param_size 				 - I   参数大小
*		  mem_tab 						 - I   内存分配表
*		  handle						 - I/O 智能库句柄
* 返回值：错误码
***********************************************************************************************************************/
int VCA_Create(VCA_MEM_PARAM_V3    *mem_param,
	           unsigned int         mem_param_size, 
	           VCA_MEM_TAB_V3       mem_tab[VCA_MAX_MEM_TAB_NUM],
	           void                **handle);

/***********************************************************************************************************************
* 功  能：设置HMS库配置信息
* 参  数：IN：
*         handle						 - I/O 智能库句柄
*         video_data				     - I   视频信息
*         in_buf						 - I   算法库输入缓存
*         in_buf_size					 - I   输入缓存大小
*         out_buf						 - O   算法库输出缓存
*         out_buf_size					 - I   输出缓存大小
* 返回值：返回错误码
***********************************************************************************************************************/
int VCA_Process(void               *handle,
	            void               *proc_in_buf,
	            unsigned int        proc_in_buf_size,
				void               *proc_out_buf,
				unsigned int        proc_out_buf_size);

/***********************************************************************************************************************
* 功  能：设置通用索引库配置信息
* 参  数：IN：
*         handle						 - I/O 智能库句柄
*         config_type				     - I   配置信息ID标号，用于区分配置信息
*         config_buf				     - I   指向外部输入配置信息的指针
*         buf_size				         - I   配置信息占用内存大小
* 返回值：返回错误码
***********************************************************************************************************************/
int VCA_SetConfig(void					*handle,
	              int					 config_type,
	              void					*config_buf,
	              int					 buf_size);

/***********************************************************************************************************************
* 功  能：设置通用索引库配置信息
* 参  数：IN：
*         handle						 - I/O 智能库句柄
*         config_type				     - I   配置信息ID标号，用于区分配置信息
*         config_buf				     - I   指向外部输入配置信息的指针
*         buf_size				         - I   配置信息占用内存大小
* 返回值：返回错误码
***********************************************************************************************************************/
int VCA_GetConfig(void					*handle,
				  int					 config_type,
				  void					*config_buf,
				  int					 buf_size);


/***********************************************************************************************************************
* 功  能：释放内部申请缓存
* 参  数：IN：
*         handle						 - I/O 智能库句柄
* 返回值：返回错误码
***********************************************************************************************************************/
int VCA_Release(void					*handle);

#ifdef __cplusplus
}
#endif

#endif /* _VCA_LIB_DEF_H_ */


