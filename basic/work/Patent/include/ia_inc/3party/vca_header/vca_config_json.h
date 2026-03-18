/***********************************************************************************************************************
*
* 版权信息：版权所有 (c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：vca_config_json.h
* 文件标识：VCA_CONFIG_JSON_H
* 摘    要：JSON配置文件协议定义声明文件
*
* 当前版本：1.0
* 作    者：童超
* 日    期：2019-05-28
* 备    注：初始版本
***********************************************************************************************************************/

#ifndef _VCA_CONFIG_JSON_H_
#define _VCA_CONFIG_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vca_base.h"
#include "vca_common.h"
#include "vca_types.h"
/***********************************************************************************************************************
*宏定义
***********************************************************************************************************************/
#define VCA_JSON_MODULE_MAX_NUM		4		// 模块数量
#define VCA_JSON_MODEL_MAX_NUM		16		// 单个模块模型最大数量
#define VCA_JSON_STAT_NUM			8       // 最多的触发状态数量
#define VCA_JSON_MAX_RULE_NUM		16      // 支持的最大的规则数量
#define VCA_JSON_MAX_REGION_NUM		8
#define VCA_JSON_MAX_PARAM_NUM		8
#define VCA_JSON_FILTER_NUM         16      //过滤类别数
#define VCA_JSON_SUB_TARGET_NUM     8       //子对象数量
#define VCA_JSON_MODEL_TUBE_MAX_LENGTH	16      // tube长度最大值
#define VCA_JSON_MODEL_TUBE_MIN_LENGTH	1       // tube长度最小值
#define VCA_JSON_MAX_UNITE_RULE_NUM	16      // 支持的最大组合规则数量
#define VCA_JSON_MAX_SINNGLE_RULE_NUM_FOR_UNITE		4		// 构成组合规则的最多单一规则数量
#define VCA_JSON_MIN_SINNGLE_RULE_NUM_FOR_UNITE		2		// 构成组合规则的最少单一规则数量
/***********************************************************************************************************************
* 结构体定义
***********************************************************************************************************************/
// 组件模块索引
typedef enum _VCA_JSON_MODULE_IDX_
{
	VCA_JSON_OBD_MODULE,					// 检测模块
	VCA_JSON_TRK_MODULE,					// 跟踪模块
	VCA_JSON_CLS_MODULE,					// 分类模块
	VCA_JSON_IED_MODULE						// 事件模块
}VCA_JSON_MODULE_IDX;

//后处理信息  暂时缺省
typedef struct _VCA_JSON_POST_PROC_INFO_
{
	unsigned int post_app_idx;			//后处理模块索引
}VCA_JSON_POST_PROC_INFO;

//前处理信息  暂时缺省
typedef struct _VCA_JSON_PRE_PROC_INFO_
{
	unsigned int pre_app_idx;			//前处理模块索引
}VCA_JSON_PRE_PROC_INFO;


//目标合并信息
typedef struct _VCA_JSON_COMBINE_INFO_
{
	int	src_target_type;				// 输入目标类型
	int	dst_target_type;				// 输出目标类型
}VCA_JSON_COMBINE_INFO;


//跟踪模块信息 （暂时缺省）
typedef struct _VCA_JSON_TRACK_INFO_
{
	int						merge_switch;							// 融合功能开关
	int						combine_num;
	VCA_JSON_COMBINE_INFO	combine_info[VCA_MAX_OBJ_NUM];			// 目标合并信息
}VCA_JSON_TRACK_INFO;

typedef struct _VCA_JSON_CROP_RESIZE_PARAM_
{
	char			reserved[64];
}VCA_JSON_CROP_RESIZE_PARAM;

//图像信息
typedef struct _VCA_JSON_IMAGE_INFO_
{
	VCA_BLOB_TYPE				target_source;						// 触发任务类型：检测、跟踪、触发
	unsigned int				img_w;								// 图像宽度
	unsigned int				img_h;								// 图像高度
	VCA_YUV_FORMAT				format;								// 输入图像格式
	void						*data;								// 输入图像数据
	int							image_num;							// 每个目标需要的缓存图像数量
	VCA_JSON_CROP_RESIZE_PARAM	param;
}VCA_JSON_IMAGE_INFO;


//模型参数
typedef struct _VCA_JSON_MODEL_INFO_
{
	VCA_BLOB_TYPE			 target_source;							// 触发任务类型：检测、跟踪、触发,可缺省
	int						 target_type;							// 待处理目标类型

	unsigned char			 model_id[VCA_MODEL_TYPE_LENGTH];		// 模型索引
	unsigned int			 model_version;

	VCA_JSON_POST_PROC_INFO	 post_process_info;						// 后处理信息 CLS特有
	VCA_JSON_PRE_PROC_INFO	 pre_process_info;						// 前处理信息 CLS特有
}VCA_JSON_MODEL_INFO;


//模型列表参数
typedef struct _VCA_JSON_MODEL_LIST_INFO_
{
	int							model_num;							// 模型数量
	VCA_JSON_MODEL_INFO			model_info[VCA_JSON_MODEL_MAX_NUM];	// 模型参数
}VCA_JSON_MODEL_LIST_INFO;

//单个模块的应用配置信息
typedef struct _VCA_JSON_MODULE_CONFIG_INFO_
{
	int							module_idx;							// 任务索引
	int							module_switch;						// 模块内功能开关
	VCA_JSON_MODEL_LIST_INFO	model_list;							// OBD CLS模块特有
	VCA_JSON_IMAGE_INFO			image_info;
	VCA_JSON_TRACK_INFO			track_info;
	VCA_JSON_POST_PROC_INFO		post_proc_info;
}VCA_JSON_MODULE_CONFIG_INFO;

//整个应用配置信息
typedef struct _VCA_JSON_APP_CONFIG_INFO_
{
	VCA_JSON_MODULE_CONFIG_INFO	config[VCA_JSON_MODULE_MAX_NUM];
}VCA_JSON_APP_CONFIG_INFO;

//跨线方向类型
typedef enum _VCA_JSON_CROSS_DIRECTION_
{
	VCA_JSON_NON_DIRRECTION,								// 没有指定方向
	VCA_JSON_BOTH_DIRECTION,								// 双向
	VCA_JSON_LEFT_TO_RIGHT,									// 由左至右
	VCA_JSON_RIGHT_TO_LEFT								    // 由右至左
}VCA_JSON_CROSS_DIRECTION; 


//规则参数
typedef struct _VCA_JSON_RULE_PARAM_
{
	unsigned int			sensitivity;				  // 规则灵敏度参数，范围[0,100]
	unsigned int		    duration;                     // 持续时间，范围: [0, 600]秒	
	unsigned int            trigger_interval;			  // 报警间隔，范围: [0, 600]秒，大于1时有效
	unsigned int            max_trigger_num;		      // 最大触发次数，范围: [1, 100]
	unsigned char           max_trigger_valid;            // 最大触发次数是否有效 [0,1] 0-配置最大触发次数不起作用,1-起作用
	unsigned int            count_interval;               // 数量统计间隔，[1, 600]秒
	unsigned int		    alert_conf_threshold;         // 报警阈值[0-1000）,大于此阈值的报警、统计结果才会输出	
	VCA_SIZE_FILTER         size_filter;                  //尺寸过滤器
}VCA_JSON_RULE_PARAM;


//规则区域
typedef struct _VCA_JSON_RULE_REGION_
{
	unsigned char          id;								   // 是否有效
	unsigned char          valid;							   // 是否有效
	unsigned char          move_dir;                           // 参考VCA_JSON_CROSS_DIRECTION
	VCA_REGION             region;							   // 顶点 
}VCA_JSON_REGION;

typedef struct _VCA_JSON_REGION_LIST_
{
	unsigned char			region_num;
	VCA_JSON_REGION			region[VCA_JSON_MAX_REGION_NUM];
}VCA_JSON_REGION_LIST;

// 触发判断条件
typedef enum _VCA_JSON_JUDGE_CONDITION_
{
	VCA_JSON_TRG_LESS_MIN           = 0x01,						     //  小于等于最小值
	VCA_JSON_TRG_GREATER_MAX        = 0x02,                          //  大于等于最大值
	VCA_JSON_TRG_BETWEEN_MIN_MAX    = 0x03,                          //  大于最小值且小于最大值	
	VCA_JSON_TRG_NONBETWEEN_MIN_MAX = 0x04,                          //  不在最小值和最大值范围内

}VCA_JSON_JUDGE_CONDITION;

//规则触发条件值
typedef struct _VCA_JSON_STATE_PARAM_
{
	unsigned char				valid;						        // 是否启用标志位
	unsigned int				status_type;                        // 状态类型
	unsigned int				status_val;                         // 状态值
	unsigned char			    model_id[VCA_MODEL_TYPE_LENGTH];	// 模型索引
	unsigned int				model_tube_length;					// 分类模型tube长度[1,24]
}VCA_JSON_STATE_PARAM;

//规则触发条件值
typedef struct _VCA_JSON_NUMBER_PARAM_
{
	unsigned char				valid;						       // 是否启用标志位
	unsigned int				min_trigger_num;                   // 触发最小目标数量
	unsigned int				max_trigger_num;                   // 触发最大目标数量
	VCA_JSON_JUDGE_CONDITION    judge_condition;                   // 判断条件

}VCA_JSON_NUM_PARAM;

typedef struct _VCA_JSON_RATIO_PARAM_
{
	unsigned char				valid;						       // 是否启用标志位
	float				        min_trigger_num;                   // 触发最小目标数量
	float				        max_trigger_num;                   // 触发最大目标数量
	VCA_JSON_JUDGE_CONDITION    judge_condition;                   // 判断条件
}VCA_JSON_RATIO_PARAM;

//分类模型与触发类型的映射参数
typedef struct _VCA_JSON_MODEL_RULE_INFO_
{

	int						 node_id;								 // 当前任务模型序号
	int						 father_node_id;						 // 父节点模型序号  

	int                      father_node_type;                       // 当串联等多个分类任务时，父节点的属性标签为当前的分类条件
	int						 father_node_result;					 // 当串联等多个分类任务时，父节点的结果为当前的分类条件
	int						 target_type;							 // 待处理目标类型
	unsigned char			 model_id[VCA_MODEL_TYPE_LENGTH];	     // 模型索引
}VCA_JSON_MODEL_RULE_INFO;

//屏蔽类别参数
typedef struct  _VCA_JSON_FILTER_PARAM_
{
	unsigned int             	obj_type;							 // 目标类型
	unsigned int                obj_status_num;                      // 触发条件数量
	VCA_JSON_STATE_PARAM	    obj_status[VCA_JSON_STAT_NUM];	     // 目标状态
	VCA_JSON_MODEL_RULE_INFO    model_rule_relation[VCA_JSON_STAT_NUM]; // 分类模型与触发规则的映射关系
}VCA_JSON_FILTER_PARAM;

//静止过滤参数 ADD FROM HMY
typedef struct  _VCA_JSON_STILL_FILTER_PARAM_
{
	//unsigned int             	obj_type;							 // 目标类型
	unsigned char				valid;						       // 是否启用标志位
	unsigned int				sensitive;                     // 灵敏度
}VCA_JSON_STILL_FILTER_PARAM;

//屏蔽条件类别列表
typedef struct  _VCA_JSON_FILTER_LIST_
{
	unsigned int filter_num;
	VCA_JSON_FILTER_PARAM filter_param[VCA_JSON_FILTER_NUM];
}VCA_JSON_FILTER_LIST;


typedef struct  _VCA_JSON_SUB_TARGET_INFO_
{
	unsigned int             	obj_type;							 // 目标类型
	unsigned int                obj_status_num;                      // 触发条件数量
	VCA_JSON_STATE_PARAM	    obj_status[VCA_JSON_STAT_NUM];	     // 目标状态
}VCA_JSON_SUB_TARGET_INFO;

//关联分析结构体
typedef struct  _VCA_JSON_RELATION_ANALYSIS_PARAM
{
	unsigned char				valid;										// 是否启用标志位
	unsigned char				analysis_type;								// 分析类型
	unsigned int				sub_target_num;								// 子对象数量
	VCA_JSON_SUB_TARGET_INFO    sub_target_info[VCA_JSON_SUB_TARGET_NUM];   // 子对象信息
}VCA_JSON_RELATION_ANALYSIS_PARAM;

//规则触发条件
typedef struct _VCA_JSON_TRG_CONDITION_
{
	unsigned char				valid;						         // 是否启用标志位
	unsigned int             	obj_type;							 // 目标类型
	unsigned int                obj_status_num;                      // 触发条件数量
	VCA_JSON_STATE_PARAM	    obj_status[VCA_JSON_STAT_NUM];	     // 目标状态
	VCA_JSON_NUM_PARAM          num_condition;                       // 触发目标判断调条件
	VCA_JSON_RATIO_PARAM        ratio_condition;                     // 触发目标判断调条件
	VCA_JSON_FILTER_LIST        filter_list;                         //过滤目标参数
	VCA_JSON_RELATION_ANALYSIS_PARAM  relation_analysis_condition;   //关联分析条件
	VCA_JSON_STILL_FILTER_PARAM still_filter;						//触发目标静止过滤参数 add from hmy
}VCA_JSON_TRG_CONDITION;



// 区域规则结构体
typedef struct _VCA_JSON_RULE_INFO_
{
	unsigned char			 rule_id;							     // 规则ID,ID从1开始编号
	unsigned char            rule_order;                             // 规则触发顺序，前置规则ID，为0时表示不启用
	unsigned char			 valid;						             // 是否有效
	unsigned char            b_update;                               // 是否更新（0-不更新， 1-更新）
	unsigned int             event_type;                             // 事件类型

	VCA_JSON_TRG_CONDITION   condition;						         // 触发条件	
	VCA_JSON_RULE_PARAM      rule_param;				             // 参数指针 参考vca_common.h
	VCA_JSON_REGION_LIST     region_list;				             // 多边形or折线

	VCA_JSON_MODEL_RULE_INFO model_rule_relation[VCA_JSON_STAT_NUM]; // 分类模型与触发规则的映射关系

}VCA_JSON_RULE_INFO;

// 区域规则链表结构体
typedef struct _VCA_JSON_RULE_LIST_
{
	unsigned char		     rule_num;							    // 规则区域数量
	VCA_JSON_RULE_INFO		 rule_info[VCA_JSON_MAX_RULE_NUM];	    // 规则区域链表

}VCA_JSON_RULE_LIST;

// 区域规则参数
typedef struct _VCA_JSON_PARAM_INFO_
{
	unsigned char		     module_idx;						  // 模块索引
	unsigned int             key;                                 // 关键值idx
	unsigned int             value;                               // 关键值对应的值

}VCA_JSON_PARAM_INFO;

// 区域规则参数链表
typedef struct _VCA_JSON_PARAM_LIST_
{
	unsigned char		     param_num;						      // 规则参数数量
	VCA_JSON_PARAM_INFO      param[VCA_JSON_MAX_PARAM_NUM];       // 规则参数

}VCA_JSON_PARAM_LIST;

// 组合规则内各单一规则的触发间隔
typedef struct _VCA_JSON_UNITE_BASE_ID_
{
	unsigned int		     single_id;						      // 最小触发间隔
}VCA_JSON_UNITE_BASE_ID;

// 组合规则内各单一规则的触发间隔
typedef struct _VCA_JSON_UNITE_TIME_INTERVAL_
{
	unsigned int		     min_interval;						      // 最小触发间隔
	unsigned int		     max_interval;						      // 最大触发间隔

}VCA_JSON_UNITE_TIME_INTERVAL;

//组合规则触发条件
typedef struct _VCA_JSON_UNITE_TRG_CONDITION_
{
	unsigned char				   max_trigger_valid;													// 最大触发次数是否有效 [0,1] 0-配置最大触发次数不起作用,1-起作用
	unsigned int				   max_trigger_num;														// 最大触发次数，范围: [1, 100]
	unsigned int				   alert_interval;														 // 报警间隔，范围: [0, 1800]秒，大于1时有效
	unsigned int             	   unite_type;															// 触发类型
	unsigned int                   base_id_num;															// 单一规则数量数量
	VCA_JSON_UNITE_BASE_ID		   base_id[VCA_JSON_MAX_SINNGLE_RULE_NUM_FOR_UNITE];					// 单一规则ID
	VCA_JSON_UNITE_TIME_INTERVAL   single_rule_time_interval[VCA_JSON_MAX_SINNGLE_RULE_NUM_FOR_UNITE - 1];	// 组合规则内各单一规则的触发间隔												// 触发目标判断调条件
}VCA_JSON_UNITE_TRG_CONDITION;

// 组合规则结构体
typedef struct _VCA_JSON_UNITE_RULE_INFO_
{
	unsigned char			       unite_rule_id;						  // 组合规则ID,ID从1开始编号
	unsigned char			       valid;						         // 是否有效
	unsigned char                  b_update;                              // 是否更新（0-不更新， 1-更新）
	unsigned int				   event_type;                           // 事件类型
	VCA_JSON_UNITE_TRG_CONDITION   unite_condition;						 // 触发条件
}VCA_JSON_UNITE_RULE_INFO;

// 组合规则链表结构体
typedef struct _VCA_JSON_UNITE_RULE_LIST_
{
	unsigned char				unite_rule_num;									// 组合规则区域数量
	VCA_JSON_UNITE_RULE_INFO	unite_rule_info[VCA_JSON_MAX_RULE_NUM];	// 组合规则区域链表

}VCA_JSON_UNITE_RULE_LIST;
//配置信息
typedef struct _VCA_JSON_CONFIG_INFO_
{
	VCA_JSON_RULE_LIST	    event_rule_list;				      // 规则链表信息，参考VCA_JSON_RULE_LIST
	VCA_JSON_REGION_LIST    det_region_list;                      // 检测区域链表
	VCA_JSON_PARAM_LIST     param_list;                           // 参数链表信息，参考VCA_JSON_PARAM_LIST
	VCA_JSON_UNITE_RULE_LIST event_unite_rule_list;				  // 组合规则链表信息，参考VCA_JSON_UNITE_RULE_LIST
}VCA_JSON_CONFIG_INFO;






#ifdef __cplusplus
}
#endif

#endif