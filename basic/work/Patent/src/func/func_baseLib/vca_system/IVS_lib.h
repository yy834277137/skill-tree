/*******************************************************************************
* 
* 版权信息：版权所有 (c) 2009, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：IVS_lib.h
* 文件标识：HIK_IVS_LIB
* 摘    要：海康威视 IVS 库(DSP版本)的接口函数声明文件

* 更新版本：2.4.3.0
* 修    改：王道荣
* 日    期：2009年8月17日
* 备    注：修改Loiter事件函数，调整run事件阈值

* 更新版本：2.4.2.0
* 作    者：蔡巍伟
* 日    期：2009年8月4日
* 备    注：修正测试过程中发现的问题,改进目标进入检测算法

* 更新版本：2.4.1.0
* 作    者：蔡巍伟
* 日    期：2009年7月1日
* 备    注：修正测试过程中发现的问题

* 更新版本：2.3
* 作    者：蔡巍伟
* 日    期：2009年6月4日
* 备    注：优化亮度多高斯模型更新函数，速度有所提升

* 更新版本：2.1
* 作    者：蔡巍伟
* 日    期：2009年5月11日
* 备    注：修正IVS2.0版本中，规则设置后只有一条规则生效的BUG.
*           修正计数器重置过程中存在的BUG.
*           修改最大规则数目为8
*           加入LF双相机跟踪参数设置接口

* 更新版本：2.0
* 作    者：蔡巍伟
* 日    期：2009年4月17日
* 备    注：智能视频分析库2.0版
*
********************************************************************************/

#ifndef _HIK_IVS_LIB_H_
#define _HIK_IVS_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif
    
/**************************************************************************************************
* 宏定义
**************************************************************************************************/
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef int HRESULT;
#endif //!_HRESULT_DEFINED

#define MAX_RULE_NUM          8        //最多支持八条规则
#define MAX_TARGET_NUM        30       //最多同时监控30个移动目标
#define MAX_PATH_LEN          10       //最多10个轨迹点
#define MAX_POLYGON_POINT_NUM 10       //多边形最多10个顶点
#define MAX_CALIB_PT          6        //最大的标定点个数
#define MIN_CALIB_PT          4        //最小的标定点个数

//错误码定义
#define HIK_IVS_LIB_S_OK                          1           //成功	    
#define HIK_IVS_LIB_S_FAIL                        0           //失败	        
#define HIK_IVS_LIB_E_PARA_NULL	                  0x80000000  //参数指针为空	    
#define HIK_IVS_LIB_E_MEM_NULL	                  0x80000001  //传入的内存为空   
#define HIK_IVS_LIB_E_MEM_OVER	                  0x80000002  //内存溢出		    
#define HIK_IVS_LIB_RULE_ID_INVALID               0x80000003  //规则ID不符合要求 
#define HIK_IVS_LIB_RULE_ID_REUSED                0x80000004  //规则ID重复使用 
#define HIK_IVS_LIB_RULE_ID_NO_FOUND              0x80000005  //未找到规则ID 
#define HIK_IVS_LIB_RULE_USED_OUT                 0x80000006  //规则已经用满 
#define HIK_IVS_LIB_POLYGON_INVALID               0x80000007  //多边形不符合要求 
#define HIK_IVS_LIB_RULE_PARAM_INVALID            0x80000008  //规则参数不符合要求 
#define HIK_IVS_LIB_RULE_INVALID                  0x80000009  //规则不符合要求 
#define HIK_IVS_LIB_RECT_INVALID                  0x80000010  //矩形不符合要求
#define HIK_IVS_LIB_PARAMETER_SET_FAIL            0x80000011  //参数设置失败 
#define HIK_IVS_LIB_CAMERA_PARAM_NULL             0x80000012  //没有传入标定所得的摄像机参数   
#define HIK_IVS_LIB_CAMERA_PARAM_INVALID          0x80000013  //传入的摄像机参数不合理
#define HIK_IVS_LIB_RESOLUTION_UNSUPPORT          0x80000014  //分辨率不支持
#define HIK_IVS_LIB_DATA_UNALIGNED                0x80000015  //视频数据未对齐
#define HIK_IVS_LIB_EXPIRE                        0x80000016  //试用时间已过
#define HIK_IVS_LIB_PARAM_KEY_INVALID             0x80000017  //参数索引号越界
#define HIK_IVS_LIB_PARAM_VALUE_INVALID           0x80000018  //设置参数等级出错
#define HIK_IVS_LIB_E_IRAM_MEM_OVER	              0x80000019  //DSP IRAM 内存溢出

//LF模块错误码定义	
#define HIK_IVS_LIB_E_NEED_CALIBRATION            0x80000020  //需要标定
#define HIK_IVS_LIB_E_CALIBRATION_POINTS_NUM      0x80000021  //标定出错,因为标定点个数不对      
#define HIK_IVS_LIB_E_CALIBRATION_POINTS_LINEAR   0x80000022  //标定出错,因为标定点所有点共线   
#define HIK_IVS_LIB_E_NO_FOUND_TARGET_ID          0x80000023  //未找到要跟踪的目标ID 
#define HIK_IVS_LIB_E_NO_LF_TRACK_ABILITY         0x80000024  //设备不支持LF跟踪功能

/**************************************************************************************************
* 结构体定义
**************************************************************************************************/
//ivs内存管理结构体，传入内存
typedef struct _IVS_BUFFER_
{
	void           *ivs_buffer;          //缓存指针
	unsigned int    ivs_buf_size;        //缓存大小
	void           *ivs_isram_buf;       //DSP片内缓存指针, PC版不需要使用
	unsigned int    ivs_isram_buf_size;  //DSP片内缓存的大小，PC版不需要使用
	unsigned short  img_width;           //输入图像宽度
	unsigned short  img_height;          //输入图像高度
}IVS_BUFFER;


//视频帧结构体YUV_420
typedef struct _YUV_FRAME_
{
	unsigned char *y; //y数据指针
	unsigned char *u; //u数据指针
	unsigned char *v; //v数据指针
}YUV_FRAME;

//归一化点结构体
typedef struct _POINT_NORMALIZE_
{
    float x;
    float y;
}POINT_NORMALIZE;

//归一化多边形结构体
typedef struct _POLYGON_NORMALIZE_
{
    unsigned int     point_num;                      //多边形顶点个数
    POINT_NORMALIZE  point[MAX_POLYGON_POINT_NUM];   //保存多边形顶点信息
} POLYGON_NORMALIZE;

//归一化矩形结构体
typedef struct _RECT_NORMALIZE_
{
	float x;      //矩形左上角X轴坐标
	float y;      //矩形左上角Y轴坐标
	float width;  //矩形宽度
	float height; //矩形高度
}RECT_NORMALIZE;

//矢量
typedef struct _IVS_VECTOR_
{
    float   x; //矢量在X轴上投影[-1.0,1.0]
    float   y; //矢量在Y轴上投影[-1.0,1.0]
}IVS_VECTOR;

//报警类型
typedef enum _EVENT_TYPE_
{
    VCA_TRAVERSE_PLANE = 1,   //跨越警戒线
    VCA_ENTER_AREA     = 2,   //进入区域
    VCA_EXIT_AREA      = 4,   //离开区域
    VCA_INTRUSION      = 8,   //入侵
    VCA_LOITER         = 16,  //徘徊
    VCA_LEFT_TAKE      = 32,  //丢包捡包
    VCA_PARKING        = 64,  //停车
    VCA_RUN            = 128, //奔跑
    VCA_HIGH_DENSITY   = 256,  //人员密度
	VIOLENT_MOTION   = 1024,        //剧烈运动，dsp版本不支持
	TRAIL            = 2048,        //尾随，dsp版本不支持
	TARRY            = 4096,        //长时间逗留，只在ATM_SURROUND模式下支持
   	STICK_UP         = 0x40000000,  //贴纸条，只在ATM_PANEL模式下支持
    INSTALL_SCANNER  = 0x80000000   //安装读卡器，只在ATM_PANEL模式下支持	
}EVENT_TYPE;

//能力类型
typedef enum _ABILITY_TYPE_
{
    VCA_TRAVERSE_PLANE_ABILITY = 1,    //跨越警戒线
    VCA_ENTER_AREA_ABILITY     = 2,    //进入区域
    VCA_EXIT_AREA_ABILITY      = 4,    //离开区域
    VCA_INTRUSION_ABILITY      = 8,    //入侵
    VCA_LOITER_ABILITY         = 16,   //徘徊
    VCA_LEFT_TAKE_ABILITY      = 32,   //丢包捡包
    VCA_PARKING_ABILITY        = 64,   //停车
    VCA_RUN_ABILITY            = 128,  //奔跑
    VCA_HIGH_DENSITY_ABILITY   = 256,  //人员密度
	VCA_LF_TRACK_ABILITY       = 512   //LF双摄像机跟踪能力
}ABILITY_TYPE;

//报警目标类型
typedef enum _TARGET_TYPE_
{
    TARGET_ANYTHING,  //人或车
    TARGET_HUMAN,     //人
    TARGET_VEHICLE    //车
}TARGET_TYPE;

//跨线方向类型
typedef enum _CROSS_DIRECTION_
{
    BOTH_DIRECTION, //双向
    LEFT_TO_RIGHT,  //由左至右
    RIGHT_TO_LEFT   //由右至左
}CROSS_DIRECTION;

//参数索引枚举
typedef enum _IVS_PARAM_KEY_
{
	OBJECT_DETECT_SENSITIVE  = 1,  //目标检测灵敏度
	BACKGROUND_UPDATE_RATE   = 2,  //背景更新速度
	SCENE_CHANGE_RATIO       = 3,  //场景变化检测最小值
	SUPPRESS_LAMP            = 4,  //是否抑制车头灯
	MIN_OBJECT_SIZE          = 5,  //能检测出的最小目标大小
	OBJECT_GENERATE_RATE     = 6,  //目标生成速度
	MISSING_OBJECT_HOLD      = 7,  //目标消失后继续跟踪时间
	MAX_MISSING_DISTANCE     = 8,  //目标消失后继续跟踪距离
    OBJECT_MERGE_SPEED       = 9,  //多个目标交错时，目标的融合速度
    REPEATED_MOTION_SUPPRESS = 10, //重复运动抑制
 	ILLUMINATION_CHANGE      = 11, //光影变化抑制开关
	TRACK_OUTPUT_MODE        = 12  //轨迹输出模式：0-输出目标的中心，1-输出目标的底部中心
}IVS_PARAM_KEY;

//视频状态
typedef enum _VIEW_STATE_
{
    GOOD_VIEW,   //正常 
    BAD_SIGNAL,  //不正常
    UNKNOWN_VIEW //摄像机被移动或遮挡
}VIEW_STATE;

//报警规则类型，
typedef enum  _RULE_TYPE_
{
    LINE_RULE,   //线规则 
    REGION_RULE  //区域规则
}RULE_TYPE;

//尺寸过滤器类型
typedef enum _SIZE_FILTER_TYPE_
{
	IMAGE_PIX_MODE, //根据像素大小设置
	REAL_WORLD_MODE //根据实际大小设置
}SIZE_FILTER_MODE;

//目标尺寸过滤器结构体
typedef struct _SIZE_FILTER_
{
	unsigned int       enable;   //是否激活尺寸过滤器
	SIZE_FILTER_MODE   mode;     //过滤器类型: 0-像素大小，1-实际大小
    RECT_NORMALIZE     min_rect; //最小目标框
    RECT_NORMALIZE     max_rect; //最大目标框
}SIZE_FILTER;

//计数器结构
typedef struct _COUNTER_
{
    unsigned int  counter1; //计数值1
    unsigned int  counter2; //计数值2
}COUNTER;

//跨线参数
typedef struct _PARAM_TRAVERSE_PLANE_
{
	CROSS_DIRECTION cross_direction; //跨越方向 
}PARAM_TRAVERSE_PLANE;

//入侵参数
typedef struct _PARAM_INTRUSION_
{
	unsigned int delay; //延迟参数: 25-3000(帧数)
}PARAM_INTRUSION;

//徘徊参数
typedef struct _PARAM_LOITER_
{
	unsigned int delay; //延迟参数: 范围:25-3000(帧数)
}PARAM_LOITER;

//丢包/捡包参数
typedef struct _PARAM_LEFT_TAKE_
{
	unsigned int delay; //延迟参数: 范围:25-3000(帧数)
}PARAM_LEFT_TAKE;

//停车参数
typedef struct _PARAM_PARKING_
{
	unsigned int delay; //延迟参数: 范围:25-3000(帧数)
}PARAM_PARKING;

//奔跑参数
typedef struct _PARAM_RUN_
{
	float  run_distance;  //人奔跑最大距离, 范围: [0.1, 1.0]
}PARAM_RUN;

//人员聚集参数
typedef struct _PARAM_HIGH_DENSITY_
{
	float density; //密度比率, 范围: [0.1, 1.0]
}PARAM_HIGH_DENSITY;

//长时间滞留参数
typedef struct _PARAM_TARRY_
{
	unsigned int delay; //延迟参数: 范围:100-3000(帧数)
}PARAM_TARRY;

//贴纸条参数
typedef struct _PARAM_STICK_UP_
{
	unsigned int delay; //延迟参数: 范围:250-1500(帧数)
}PARAM_STICK_UP;

//读卡器参数
typedef struct _PARAM_INSTALL_SCANNER_
{
	unsigned int delay; //延迟参数: 范围:250-1500(帧数)
}PARAM_INSTALL_SCANNER;

//规则参数联合体
typedef union _RULE_PARAM_SETS_
{
	int                  param[6];       //参数
	PARAM_TRAVERSE_PLANE traverse_plane; //跨线参数  
	PARAM_INTRUSION      intrusion;      //入侵参数
	PARAM_LOITER         loiter;         //徘徊参数
	PARAM_LEFT_TAKE      left_take;      //丢包/捡包参数
	PARAM_PARKING        parking;        //停车参数
	PARAM_RUN            run;            //奔跑参数
	PARAM_HIGH_DENSITY   high_density;   //人员聚集参数
}RULE_PARAM_SETS;

//规则参数
typedef struct _RULE_PARAM_
{
	unsigned int          sensitivity; //规则灵敏度参数，范围[1,5]
	unsigned int          reserved[4]; //预留字节
	RULE_PARAM_SETS       param;       //规则参数
	SIZE_FILTER           size_filter; //尺寸过滤器	
}RULE_PARAM;

//警戒规则
typedef struct _HIK_RULE_
{
	unsigned char      ID;             //规则ID
	unsigned char      enable;         //是否激活;
	unsigned char      rule_type;      //规则类型，0: line; 1: region; 2: global
	unsigned char      target_type;    //警戒目标类型，2: Human; 1: Vehicle; 0: anything
    unsigned int       event_type;     //警戒事件类型
	
	unsigned char      reset_counter;  //流量统计重置标记
	unsigned char      update_rule;    //规则更新标记
	unsigned char      enable_counter; //是否开启事件计数器
    unsigned char      reserved[5];    //保留字节
	
    RULE_PARAM         rule_param;     //规则参数
    POLYGON_NORMALIZE  polygon;        //规则对应的多边形区域
}HIK_RULE;

//警戒规则链表
typedef struct _HIK_RULE_LIST_
{
    unsigned int  rule_num;           //链表中规则数量
    HIK_RULE      rule[MAX_RULE_NUM]; //规则数组
}HIK_RULE_LIST;

//目标轨迹
typedef struct _PATH_
{
	unsigned int     point_num; //轨迹中轨迹点的数量 
	POINT_NORMALIZE  point[MAX_PATH_LEN];
}PATH;

//目标速度信息
typedef struct  _VECTOR_INFO_
{
	float vx; //X轴速度
	float vy; //Y轴速度
	float ax; //X轴加速度
	float ay; //Y轴加速度
} VECTOR_INFO;

//目标信息结构体
typedef struct _TARGET_STRU_
{
    unsigned int    ID;          //ID
    unsigned char   type;        //类型
    unsigned char   reserved[7]; //保留字节
    RECT_NORMALIZE  rect;        //目标框
	PATH            path;        //轨迹
	VECTOR_INFO     vector_info; //速度
}TARGET;

//目标链表
typedef struct _TARGET_LIST_
{
    unsigned char target_num;             //链表中目标数量
    unsigned char reserved[7];            //保留字节
	TARGET        target[MAX_TARGET_NUM]; //目标数组
}TARGET_LIST;

//简化的规则信息, 包含规则的基本信息
typedef struct _HIK_RULE_INFO_
{
	unsigned char         ID;           //警戒规则计数器 ID
	unsigned char         rule_type;    //规则类型，0: line; 1: region; 2: global
	unsigned char         pad[2];       //对齐字节
	unsigned int          event_type;   //警戒事件类型
	unsigned int          reserved[4];  //保留字节
	COUNTER               counter;      //事件计数器
    RULE_PARAM_SETS       param;        //规则参数
	POLYGON_NORMALIZE     polygon;      //规则对应的多边形区域
}HIK_RULE_INFO;

//简化目标结构体，不包含轨迹信息
typedef struct _ALERT_TARGET_
{
	unsigned int    ID;          //ID
	unsigned char   type;        //类型
	unsigned char   reserved[3]; //保留字节
	RECT_NORMALIZE  rect;        //目标框
}ALERT_TARGET;

//报警结构体
typedef struct  _HIK_ALERT_
{
    unsigned char    alert;       //有无报警信息: 0-没有，1-有
    unsigned char    view_state;  //场景状态:0-正常, 1-场景变化
  	unsigned char    reserved[6]; //保留字节
	HIK_RULE_INFO    rule_info;   //报警事件对应的简化规则信息
    ALERT_TARGET     target;      //报警目标信息         
}HIK_ALERT;

//PTZ坐标参数结构体
typedef struct _PTZ_F_
{
	float pan;  //pan值
	float tilt; //tilt值
	float zoom; //zoom值
}PTZ_F;

//输入标定点结构体
typedef struct _CB_POINT_
{
	POINT_NORMALIZE image_point; //输入的图像平面坐标，归一化到0-1
	PTZ_F           ptz_value;   //输入的PTZ坐标
}CB_POINT;

//定义目标跟踪时相关信息结构体
typedef struct _LF_TRACK_INFO_
{
	PTZ_F			ptz_val;  //输出的PTZ坐标
	unsigned int    track_id; //需跟踪的目标ID
}LF_TRACK_INFO;

//输入标定点结构体
typedef struct _CB_POINT_LIST_
{
	CB_POINT        cb_point[MAX_CALIB_PT]; //标定点组
	unsigned int    point_num;              //标定点个数
}CB_POINT_LIST;

//LF跟踪参数
typedef struct _PARAM_LF_TRACK_
{
    unsigned int lf_track_mode;       //双摄像机跟踪模式 0-不跟踪，1-报警触发跟踪， 2-目标ID触发跟踪
	unsigned int camera_track_hold;   //球机跟踪最短时间 5-20秒
	unsigned int reserved[4];         //保留字节
}PARAM_LF_TRACK;

/**************************************************************************************************
* 接口函数
**************************************************************************************************/
/******************************************************************************
* 功  能：获取版本号
* 参  数：OUT：
*         version_num - 获得当前版本号
*         date        - 获得版本日期
* 返回值：返回错误码
* 备  注：返回的整数除以10即是当前版本号
******************************************************************************/
HRESULT HIKIVS_GetVersion(int *version_num, int *date);		

/******************************************************************************
* 功  能：获取能力集
* 参  数：OUT:
*         ability - 能力集
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_GetAbility(unsigned int *ability);
	
/******************************************************************************
* 功  能：获取IVS库所需内存大小
* 参  数：IN:
*         param     - 输入参数，确定输入图像的宽高
*		  OUT:
*		  param     - param中保存内存大小信息
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_GetMemSize(IVS_BUFFER *param);  


/******************************************************************************
* 功  能：创建IVS视频分析
* 参  数：IN:
*         buf     - 缓存管理器，指定IVS的创建地址
*		  OUT:
*		  handle  - IVS句柄
* 返回值：错误码
******************************************************************************/
HRESULT HIKIVS_Create(IVS_BUFFER *buf, void **handle); 


/******************************************************************************
* 功  能：进行智能分析
* 参  数：IN:
*         handle          - IVS模块句柄
*		  yuv_frame       - 当前图像
*		  OUT:
*		  alert           - 输出报警信息
*         target_list     - 输出目标信息链表
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_Process(void        *handle,
					   YUV_FRAME   *yuv_frame, 
					   HIK_ALERT   *alert, 
					   TARGET_LIST *target_list);  


/******************************************************************************
* 功  能：更新所有的报警规则
* 参  数：IN：
*         handle      - ALERT模块句柄
*         rule_list   - 报警规则链表
* 返回值：返回错误码 
******************************************************************************/
HRESULT HIKIVS_UpdateRule(void *handle, HIK_RULE_LIST *rule_list); 

/******************************************************************************
* 功  能：获取所有的报警规则
* 参  数：IN：
*         handle      - IVS模块句柄
*		  OUT：
*         rule_list   - 获取的规则链表
* 返回值：返回错误码 
******************************************************************************/
HRESULT HIKIVS_GetRule(void *handle, HIK_RULE_LIST *rule_list);

/******************************************************************************
* 功  能：设置IVS的处理参数
* 参  数：IN：
*         handle	  - IVS模块句柄
*         param_key   - 参数索引
*         param_val   - 参数等级
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_SetParam(void *handle, IVS_PARAM_KEY param_key, int param_val);

/******************************************************************************
* 功  能：获得IVS的处理参数
* 参  数：IN：
*         handle	  - IVS模块句柄
*         param_key   - 参数索引
*         param_val   - 参数等级
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_GetParam(void *handle, IVS_PARAM_KEY param_key, int *param_val);

/******************************************************************************
* 功  能：设置IVS中的所有参数为默认值
* 参  数：IN：
*         handle      - IVS模块句柄
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_SetDefaultParam(void *handle);

/******************************************************************************
* 功  能：重启IVS模块
* 参  数：IN：
*         handle      - IVS模块句柄
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_RestartLib(void *handle);

/******************************************************************************
* 功  能：标定LF双摄像机系统
* 参  数：IN：
*         handle        - IVS模块句柄
*         cb_point_list - 用于标定的图像坐标和PTZ坐标集合
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_CalibrateLFSystem(void *handle, CB_POINT_LIST *cb_point_list);

/******************************************************************************
* 功  能：将图像坐标转换成PTZ坐标
* 参  数：IN:
*         handle        - IVS模块句柄
*         img_pt        - 图像坐标点
*         OUT:
*         ptz_val       - 转换得到的PTZ值
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_ImagePos2PTZ(void            *handle, 
						    POINT_NORMALIZE *img_pt, 
						    PTZ_F           *ptz_val);

/******************************************************************************
* 功  能：获取球机跟踪位置
* 参  数：IN:
*         handle         - IVS模块句柄
*         manual_id      - 在手工锁定目标模式下，需输入的跟踪目标ID
*         OUT:
*         lf_track_info  - 输出跟踪相关信息, 包含PTZ和ID信息
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_GetPTZValue(void  *handle, LF_TRACK_INFO *lf_track_info, unsigned int manual_id);

/******************************************************************************
* 功  能：设置LF跟踪参数
* 参  数：IN:
*         handle         - IVS模块句柄
*         lf_param       - LF跟踪参数结构体
* 返回值：返回错误码
******************************************************************************/
HRESULT HIKIVS_SetLFTrackParam(void *handle, PARAM_LF_TRACK *lf_param);

/**************************************************************************************************
* 调试接口
**************************************************************************************************/
/******************************************************************************
* 功  能：获取当前前景图像
* 参  数：IN：
*         handle     -  ivs句柄
*		  OUT：
*         fg_img     -  前景图像
* 返回值：错误码
******************************************************************************/
HRESULT IVS_GetFgImage(void *handle, unsigned char *fg_img);


/******************************************************************************
* 功  能：获得目标的速度矢量
* 参  数：IN：
*         handle	    - IVS模块句柄
*		  target_ID     - 目标ID
*		  OUT：
*         vector_info	- 目标对应的速度信息
* 返回值：返回错误码
******************************************************************************/
HRESULT IVS_GetTargetVector(void *handle, unsigned int target_ID, VECTOR_INFO *vector_info);

#ifdef __cplusplus
}
#endif 

#endif /* _HIK_IVS_LIB_H_ */

