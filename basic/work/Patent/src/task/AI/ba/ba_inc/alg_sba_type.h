/* @file       alg_sbae_type.h
 * @note       HangZhou Hikvision Digital Technology Co., Ltd.
 *             All right reserved
 * @brief      安检行为分析引擎对外接口
 * @note       安检行为分析引擎对外头文件，输入输出结构体、配置项
 *           
 * @version    v0.8.0
 * @author     chentao29
 * @date       2020.9.22
 * @note       初始版本
 * 
 * @version    v2.0.0
 * @author     zhenghaoran
 * @date       2021.2.4
 * @note       2.0.1 版本
 */
#ifndef _ALG_SBAE_TYPE_H_
#define _ALG_SBAE_TYPE_H_

#include "vca_base.h"

/********************************************************************************************************************
* 宏定义
********************************************************************************************************************/
#define  SBAE_MAJOR_VERSION               (2)           // 主版本
#define  SBAE_SUB_VERSION                 (0)           // 子版本
#define  SBAE_REVISION_VERSION            (0)           // 修订版本
#define  SBAE_VERSION_YEAR                (2021)        // 年
#define  SBAE_VERSION_MONTH               (2)           // 月
#define  SBAE_VERSION_DAY                 (4)           // 日

#define  SBAE_MAX_MODEL_PATH_NUM          (16)          // 最大模型路径数量
#define  SBAE_MAX_PATH_LEN                (256)         // 路径长度
#define  SBAE_MAX_FUN_NUM                 (8)           // 最大功能数量
#define  SBAE_MAX_RES_NUM                 (16)          // 最大报警数量

#define SBAE_MAX_TYPE_NUM                  (8)          // 最大分类的类别数
#define SBAE_DBA_MAX_LEVEL_NUM             (5)          // 最大报警等级数
/********************************************************************************************************************
* 枚举变量
********************************************************************************************************************/
/* @enum     SBAE_NODE_ID_E
  * @brief   Node ID定义,外部可根据此进行配置或取源
  */
typedef enum _SBAE_NODE_ID_E
{
    SBAE_NODE_BA                   = 2,           //行为分析节点
    SBAE_NODE_POST                 = 3            //结果发送节点
}SBAE_NODE_ID;

/* @enum     SBAE_DBA_PROC_MODE_E
  * @brief    DBA运行模式
  */
typedef enum _SBAE_DBA_PROC_MODE_E_
{
	SBAE_DBA_PROC_AUTO            = 0x00,	     // 根据车速判断自适应模式（非0且恒定速度为过检模式，其他实车模式）
	SBAE_DBA_PROC_DEMO            = 0x01,        // 过检模式：该模式下偏向于事件检出，一般过检、演示等情况采用此模式
	SBAE_DBA_PROC_REAL            = 0x02,        // 实车模式：该模式下偏向于事件检准，实车、产品发布时采用此模式
    SBAE_DBA_PROC_END             = VCA_ENUM_END
}SBAE_DBA_PROC_MODE_E;

/* @enum     SBAE_CONFIG_KEY_E
  * @brief     配置模式
  */
typedef enum _SBAE_CONFIG_KEY_E
{ 
    SBAE_ALG_CONFIG_SET_DBA_PARAMS = 0x10000,     // DBA（驾驶行为分析）参数配置
    SBAE_ALG_CONFIG_GET_DBA_PARAMS = 0x10001,     // DBA（驾驶行为分析）参数获取
    SBAE_ALG_CONFIG_GET_VERSION    = 0x10002,     // 获取版本信息, 复用ICF_VERSION
}SBAE_CONFIG_KEY_E;


/********************************************输出结果枚举变量***************************************************/
/* @enum     SBAE_BEHAVIOR_TYPE_E
  * @brief     行为模式
  */
typedef enum _SBAE_BEHAVIOR_TYPE_E_
{
    SBAE_BEH_INVALID     = 0x0000,               // 无效
    SBAE_BEH_FCD         = 0x0010,               // 前车检测
    SBAE_BEH_FCL         = 0x0011,               // 前车起步
    SBAE_BEH_FCW         = 0x0020,               // 前车碰撞预警
    SBAE_BEH_UFCW        = 0x0021,               // 城市前车碰撞预警
    SBAE_BEH_PCD         = 0x0030,               // 行人检测
    SBAE_BEH_PCW         = 0x0040,               // 行人碰撞预警
    SBAE_BEH_PSCW        = 0x0041,               // 礼让行人报警
    SBAE_BEH_PSCW_N      = 0x0042,               // 未礼让行人报警
    SBAE_BEH_PSCW_N_PRE  = 0x0043,               // 未礼让行人报警-预抓拍
    SBAE_BEH_BSD_L       = 0x0050,               // 超车盲区预警(左侧)
    SBAE_BEH_BSD_R       = 0x0051,               // 超车盲区预警(右侧)
	SBAE_BEH_BSD_F       = 0x0052,               // 盲区预警(前方)
	SBAE_BEH_BSD_B       = 0x0053,               // 盲区预警(后方)
    SBAE_BEH_BSD         = 0x0054,               // 盲区预警(四周)
	SBAE_BEH_BSD_NEAR    = 0x0055,               // 盲区预警(四周近处)
	SBAE_BEH_BSD_FAR     = 0x0056,               // 盲区预警(四周远处)
	SBAE_BEH_BSD_L_1M = 0x0057,                  // 左视后置平视高度1米
	SBAE_BEH_BSD_L_2M = 0x0058,                  // 左视后置平视高度2米
	SBAE_BEH_BSD_L_3M = 0x0059,                  // 左视后置平视高度3米
	SBAE_BEH_BSD_R1M = 0x0060,                   // 右视后置平视高度1米
	SBAE_BEH_BSD_R2M = 0x0061,                   // 右视后置平视高度2米
	SBAE_BEH_BSD_R3M = 0x0062,                   // 右视后置平视高度3米
	SBAE_BEH_BSD_L_OVER_LOOK = 0x0063,           // 左视前置俯视
	SBAE_BEH_BSD_L_POST_OVER_LOOK = 0x0064,      // 左视后置俯视
	SBAE_BEH_BSD_R_OVER_LOOK = 0x0065,           // 右视前置俯视
	SBAE_BEH_BSD_R_POST_OVER_LOOK = 0x0066,      // 右视后置俯视
	SBAE_BEH_BSD_F_MID = 0x0067,                 // 前视居中安装
    SBAE_BEH_BSD_L_ALARM = 0x0068,               // 主动超车盲区预警(左侧)
    SBAE_BEH_BSD_R_ALARM = 0x0069,               // 主动超车盲区预警(右侧)

    SBAE_BEH_TCSS_UNKNOWN_BEDDING = 0x0080,       // 车厢密闭状态位置
    SBAE_BEH_TCSS_NO_BEDDING = 0x0081,           // 车厢未密闭
    SBAE_BEH_TCSS_HALF_BEDDING = 0x0082,         // 车厢半密闭
    SBAE_BEH_TCSS_FULL_BEDDING = 0x0083,         // 车厢全密闭
    SBAE_BEH_TCSS_UNKNOWN_LOAD = 0x0084,         // 车厢载货状态位置
    SBAE_BEH_TCSS_NO_LOAD = 0x0085,              // 车厢空载
    SBAE_BEH_TCSS_HALF_LOAD = 0x0086,            // 车厢半载
    SBAE_BEH_TCSS_FULL_LOAD = 0x0087,            // 车厢满载

    SBAE_BEH_LDD         = 0x0100,               // 车道检测
    SBAE_BEH_LDW_L       = 0x0110,               // 车道左偏离预警
    SBAE_BEH_LDW_R       = 0x0111,               // 车道右偏离预警
    SBAE_BEH_LDW_L_CROSS = 0x0112,               // 车道左越线报警
    SBAE_BEH_LDW_R_CROSS = 0x0113,               // 车道右越线报警

    SBAE_BEH_FDW_SUNGLASS= 0x0210,               // 红外阻断型墨镜预警
    SBAE_BEH_FDW_EYE     = 0x0211,               // 闭眼疲劳预警
    SBAE_BEH_FDW_YAWN    = 0x0212,               // 打哈欠疲劳预警
	SBAE_BEH_FDW_EYE_LIT = 0x0213,               // 轻度闭眼疲劳预警
	SBAE_BEH_FDW_NO_MASK = 0x0214,               // 未戴口罩报警
	SBAE_BEH_FDW_XU      = 0x0215,               // 嘘动作行为
	
    SBAE_BEH_NLF_L       = 0x0220,               // 左不目视前方预警
    SBAE_BEH_NLF_R       = 0x0221,               // 右不目视前方预警
    SBAE_BEH_NLF_U       = 0x0222,               // 上不目视前方预警
    SBAE_BEH_NLF_D       = 0x0223,               // 下不目视前方预警
    SBAE_BEH_NLF_LD      = 0x0224,               // 左下不目视前方预警
    SBAE_BEH_NLF_RD      = 0x0225,               // 右下不目视前方预警
    SBAE_BEH_UPW         = 0x0230,               // 打电话预警
    SBAE_BEH_SMW         = 0x0240,               // 抽烟预警
    SBAE_BEH_PUW         = 0x0250,               // 捡拾物品预警
    SBAE_BEH_BELT        = 0x0260,               // 安全带预警
    SBAE_BEH_CLOTH       = 0x0270,               // 工作服预警
    SBAE_BEH_TALK        = 0x0280,               // 谈话注意力分散预警
    SBAE_BEH_ABNORMAL    = 0x0290,               // 驾驶员异常预警
    SBAE_BEH_COVERCC     = 0x0291,               // 遮挡摄像头预警
	SBAE_BEH_ONE_HAND_ON = 0x02A0,               // 单手脱离方向盘
	SBAE_BEH_NO_HAND_ON  = 0x02A1,               // 双手脱离方向盘
	SBAE_BEH_WHDUPD      = 0x02A2,               // 头顶路玩手机
	SBAE_BEH_WHDUPD_DEMO = 0x02A3,               // 头顶路玩手机(过检灵敏度)
    SBAE_BEH_HGR_REJECT  = 0x02B0,               // 静态拒绝手势
    SBAE_BEH_HGR_OK      = 0x02B1,               // 静态OK手势
    SBAE_BEH_HGR_THUMB   = 0x02B2,               // 静态点赞手势
    SBAE_BEH_HGR_ROCK    = 0x02B3,               // 静态ROCK手势
	SBAE_BEH_HGR_HEART   = 0x02B4,               // 静态比心手势
	SBAE_BEH_HGR_YEAH    = 0x02B5,               // 静态比耶手势
	SBAE_BEH_HGR_FIST    = 0x02B6,               // 静态拳头手势
	SBAE_BEH_HGR_INDEX   = 0x02B7,               // 静态食指手势
	SBAE_BEH_HGR_BG      = 0x02B8,               // 静态背景手势

    SBAE_BEH_PALM_TO_LEFT    = 0x02C0,           // 手掌左滑
    SBAE_BEH_PALM_TO_RIGHT   = 0x02C1,           // 手掌右滑
    SBAE_BEH_PALM_TO_BYE     = 0x02C2,           // 手掌拜拜
    SBAE_BEH_INDEX_TO_CWISE  = 0x02C3,           // 食指顺时针画圈
    SBAE_BEH_INDEX_TO_ACWISE = 0x02C4,           // 食指逆时针画圈
    SBAE_BEH_PALM_TO_UP      = 0x02C5,           // 手掌上滑
    SBAE_BEH_PALM_TO_DOWN    = 0x02C6,           // 手掌下滑

    //SBAE_BEH_HPR_KPT         = 0x02D0,           // 手势关键点
	
	SBAE_BEH_WHDBELT     = 0x02A8,               // 头顶路未系安全带
	// 0x02E0-0x02Fx 为DBA向外输出的架设警告码
	SBAE_ABN_HEAD_NON_LEAVE        = 0x02E0,     // 无人脸输出---离岗
	SBAE_ABN_HEAD_NON_BIAS         = 0x02E1,     // 无人脸输出---镜头偏离
	SBAE_ABN_HEAD_TOO_BIG          = 0x02E2,     // 人脸过大，宽度大于450，
	SBAE_ABN_HEAD_TOO_SMALL        = 0x02E3,     // 人脸过小，宽度小于150
	SBAE_ABN_HEAD_TOO_BRIGHT       = 0x02E4,     // 人脸过暗，亮度大于220
	SBAE_ABN_HEAD_TOO_DARK         = 0x02E5,     // 人脸过亮，亮度小于60
	SBAE_ABN_HEAD_INVALID_POSITION = 0x02E6,     // 人脸位置太靠左、右、下
	SBAE_ABN_WHEEL_NON             = 0x02F0,     // 未检测到方向盘
	SBAE_ABN_WHEEL_HEAD_NON        = 0x02F1,     // 方向盘路未检测到头部
	SBAE_ABN_WHEEL_INVALID_POSITION = 0x02F2,    // 方向盘架设不规范


    SBAE_BEH_TSR         = 0x0300,               // 交通标志牌提醒
    SBAE_BEH_ROADMARK    = 0x0310,               // 路面标志提醒
    SBAE_BEH_RM_ZEB_SPD  = 0x0312,               // 路口（斑马线）超速
    SBAE_BEH_LIGHT       = 0x0320,               // 信号灯提醒
    SBAE_BEH_CROSS       = 0x0330,               // 路口提醒

    SBAE_BEH_ROAD_SEG      = 0x0400,             // 相机架设异常提醒(路面分割占比偏低)
    SBAE_BEH_VANISH_OFFSET = 0x0401,             // 相机架设异常提醒(消隐线异常)
    SBAE_BEH_VANISH_LOW    = 0x0402,             // 相机架设异常提醒(初始消隐线过低)
    SBAE_BEH_VANISH_HIGH   = 0x0403,             // 相机架设异常提醒(初始消隐线过高)
	SBAE_BEH_CARBODY_OFFSET = 0x0404,            // 相机架设异常提醒(车身占比异常)
    SBAE_BEH_ICW            = 0x0410,            // ADAS镜头遮挡预警
    SBAE_BEH_END                    = VCA_ENUM_END
} SBAE_BEHAVIOR_TYPE_E;

/* @enum     SBAE_OBJ_TYPE_E
  * @brief    目标类型
  */
typedef enum _SBAE_OBJ_TYPE_E_
{
 SBAE_OBJ_UNKNOWN            = 0,           //未知  0
    SBAE_OBJ_NEGATIVE           = 0x01,        //无效目标  1
    SBAE_OBJ_GENERAL            = 0x02,        //通用障碍物  2
    SBAE_OBJ_SONAR              = 0x10,        //超声波探测目标  16
    SBAE_OBJ_RADAR              = 0x20,        //毫米波探测目标  32
    SBAE_OBJ_LIDAR              = 0x30,        //激光探测目标  48

    SBAE_OBJ_VEHICLE            = 0x100,       //机动车 0x1XX  256
    SBAE_OBJ_LARGE_VEHICLE      = 0x101,       //大车 车辆种类从101~11F  257
    SBAE_OBJ_SMALL_VEHICLE      = 0x102,       //小车  258
    SBAE_OBJ_BUS                = 0x103,       //公交  259
    SBAE_OBJ_TRUCK              = 0x104,       //货车  260

    SBAE_OBJ_VEHICLE_HEAD       = 0x120,       //车头 车辆部件从120~12F  288
    SBAE_OBJ_VEHICLE_TAIL       = 0x121,       //车尾  289
    SBAE_OBJ_VEHICLE_FRONT_FACE = 0x122,       //车前脸  290
    SBAE_OBJ_VEHICLE_BACK_FACE  = 0x123,       //车后脸  291
    SBAE_OBJ_WHEEL              = 0x124,       //车轮  292
    SBAE_OBJ_PLATE              = 0x125,       //车牌  293
    SBAE_OBJ_CAR_WINDOW         = 0x126,       //车窗  294

    SBAE_OBJ_NON_VEHICLE        = 0x200,       //非四轮车0x2XX  512
    SBAE_OBJ_TWO_WHEEL          = 0x201,       //二轮车  513
    SBAE_OBJ_MOTOR              = 0x202,       //摩托车  514
    SBAE_OBJ_CYCLIST            = 0x203,       //自行车  515
    SBAE_OBJ_THREE_WHEEL        = 0x204,       //三轮车  516
    SBAE_OBJ_TWO_WHEEL_UNMANNED = 0x205,       //无人二轮车  517

    SBAE_OBJ_HUMAN              = 0x300,       //行人0x3XX  768
    SBAE_OBJ_FACE               = 0x310,       //人脸  784
      
    SBAE_OBJ_ROAD               = 0x400,       //路面0x4XX  1024
    SBAE_OBJ_LANE               = 0x401,       //车道线  1025
    SBAE_OBJ_ROADMARKING        = 0x402,       //路面标志  1026
    SBAE_OBJ_ZEBRA              = 0x403,       //斑马线  1027
    SBAE_OBJ_ROADEDGE           = 0x404,       //路沿  1028
    SBAE_OBJ_PARK               = 0x410,       //车位线  1040
    SBAE_OBJ_PARK_POINT         = 0x411,       //车位角点 1041

    SBAE_OBJ_TRAFFICSIGN        = 0x420,       //交通标志，包括限速标志、限重标志等  1056
    SBAE_OBJ_TRAFFICLIGHT       = 0x421,       //交通灯  1057

    SBAE_OBJ_CONE               = 0x450,       //三角堆  1104

    SBAE_OBJ_SMOG               = 0x510,       //烟雾  1296
    SBAE_OBJ_FLAME              = 0x520,       //火灾  1312
    SBAE_OBJ_BANNER             = 0x530,       //条幅  1328
    SBAE_OBJ_BOOTH              = 0x540,       //摊位  1344

	SBAE_OBJ_DIRECTIONSIGN      = 0x550,       //导流牌  1360
	SBAE_OBJ_ANTICOLLISION      = 0x560,       //防撞条  1376
	SBAE_OBJ_FIRE_HYDRANT       = 0x570,       //消防栓  1392
	SBAE_OBJ_GATE               = 0x580,       //闸机杆  1408

	SBAE_OBJ_DRIVER             = 0xA01,       // 司机行为
	SBAE_OBJ_FRONT_PASSENGER    = 0xA02,       // 前排乘客行为
	SBAE_OBJ_BACK_PASSENGER     = 0xA04,       // 后排乘客行为
    SBAE_OBJ_END                     = VCA_ENUM_END
} SBAE_OBJ_TYPE_E;

/* @enum     SBAE_MDM_TYPE_E
  * @brief    测距类型枚举
  */
typedef enum _SBAE_MDM_TYPE_E_
{
    SBAE_MDM_INVALID                 = 0,                     // 无效
    SBAE_MDM_VIDEO                   = 1,                     // 相机测距
    SBAE_MDM_RADAR                   = 2,                     // 雷达测距
    SBAE_MDM_SONAR                   = 3,                     // 超声波测距
    SBAE_MDM_RVF                     = 4,                     // 融合测距
    SBAE_MDM_END                     = VCA_ENUM_END
}SBAE_MDM_TYPE_E;

/* @enum     SBAE_MOTION_TYPE_E
  * @brief    运动状态
  */
typedef enum _SBAE_MOTION_TYPE_E_
{
    SBAE_MOTION_INVALID               = 0,                    // 不明
    SBAE_MOTION_STATIC                = 1,                    // 目标(绝对)静止
    SBAE_MOTION_MOVE                  = 2,                    // 运动
    SBAE_MOTION_MOVE_ACCE             = 3,                    // 加速运动
    SBAE_MOTION_MOVE_DECE             = 4,                    // 减速运动
    SBAE_MOTION_MOVE_CONST            = 5,                    // 匀速运动
    SBAE_MOTION_MOVE_BACK             = 6,                    // 倒车
    SBAE_MOTION_END                   = VCA_ENUM_END
} SBAE_MOTION_TYPE_E;

/* @enum     _SBAE_DBA_OBJ_ATT_E
  * @brief    MOD属性信息解析结构体
  */
typedef enum _SBAE_DBA_OBJ_ATT_E
{
	SBAE_DBA_OBJ_EYE_CLOSE       = 0x10,          // 脸部属性-闭眼
	SBAE_DBA_OBJ_EYE_OPEN        = 0x11,          // 脸部属性-睁眼

	SBAE_DBA_OBJ_MOUTH_CLOSE     = 0x20,          // 脸部属性-嘴巴闭合
	SBAE_DBA_OBJ_MOUTH_OPEN      = 0x21,          // 脸部属性-嘴巴张开

	SBAE_DBA_OBJ_UNWEAR_SUNGLASS = 0x30,          // 脸部属性-未戴墨镜
	SBAE_DBA_OBJ_WEAR_SUNGLASS   = 0x31,          // 脸部属性-戴墨镜
	SBAE_DBA_OBJ_GLASS_LIGHT     = 0x32,          // 脸部属性-镜片反光

	SBAE_DBA_OBJ_UNWEAR_MASK     = 0x40,          // 脸部属性-未戴口罩
	SBAE_DBA_OBJ_WEAR_MASK       = 0x41,          // 脸部属性-戴口罩

	SBAE_DBA_OBJ_LEFT_EAR_CALL   = 0x50,          // 附耳打电话,左边
	SBAE_DBA_OBJ_RIGHT_EAR_CALL  = 0x51,          // 附耳打电话,右边
	SBAE_DBA_OBJ_WATCH_PHONE     = 0x52,          // 看手机
	SBAE_DBA_OBJ_HAND_LEFT       = 0x53,			 // 不打电话，左边有手
	SBAE_DBA_OBJ_HAND_RIGHT      = 0x54,          // 不打电话，右边有手
	SBAE_DBA_OBJ_HEAD_PHONE      = 0x55,          // 头夹手机
	SBAE_DBA_OBJ_BACKGROUND      = 0x56,          // 不打电话，背景
	SBAE_DBA_OBJ_NOT_OBVIOUS     = 0x57,          // 头夹手机，不明显


	SBAE_DBA_OBJ_DNOT_FIND_SMOKE = 0x60,          // 手部属性-未找到香烟
	SBAE_DBA_OBJ_FIND_SMOKE      = 0x61,          // 手部属性-找到香烟
	SBAE_DBA_OBJ_FIND_LIGHT_SMOKE= 0x62,          // 手部属性-找到(点燃的)香烟

	SBAE_DBA_OBJ_ABNORMAL_LEAVE = 0x70,           // 驾驶员异常-离岗
	SBAE_DBA_OBJ_ABNORMAL_COVER = 0x71,           // 驾驶员异常-遮挡
	SBAE_DBA_OBJ_ABNORMAL_OTHER = 0x72,           // 驾驶员异常-其它异常

	SBAE_DBA_OBJ_EYE_CLOSE_VIDEO = 0x80,          // 脸部属性-闭眼-视频模型
	SBAE_DBA_OBJ_EYE_OPEN_VIDEO  = 0x81,          // 脸部属性-睁眼-视频模型

	SBAE_DBA_OBJ_TSN_NON         = 0x90,          // 视频行为-正常
	SBAE_DBA_OBJ_TSN_SMOKE       = 0x91,          // 视频行为-抽烟
	SBAE_DBA_OBJ_TSN_CALL        = 0x92,          // 视频行为-打电话

	SBAE_DBA_OBJ_EYE_IQA_CLOSE   = 0xA0,            // 脸部属性-闭眼-IQA模型
	SBAE_DBA_OBJ_EYE_IQA_OPEN    = 0xA1,             // 脸部属性-睁眼-IQA模型

	SBAE_DBA_OBJ_HPC_UNKNOWN    = 0xB0,           // 手势背景、未知类
	SBAE_DBA_OBJ_HPC_REJECT     = 0xB1,           // 拒绝手势
	SBAE_DBA_OBJ_HPC_OK         = 0xB2,           // OK手势
	SBAE_DBA_OBJ_HPC_THUMB      = 0xB3,			 // 大拇指手势
	SBAE_DBA_OBJ_HPC_ROCK       = 0xB4,           // ROCK手势


	SBAE_DBA_OBJ_ID_END = VCA_ENUM_END
}SBAE_DBA_OBJ_ATT_E;

// 影响驾驶室报警等级的因素
typedef struct _SBAE_BEH_LEVEL_FACTOR_
{
	short speed;                                      // 报警速度阈值    范围:[0,255],  默认:0
	short duration;                                   // 持续闭眼时间(单位s)， 范围:(0,10], 默认:60s
	short ratio;                                      // 持续比例,       范围:(0,100], 默认:50
	short perclos_range;                              // 灵敏度区间      范围:[0, 20]   默认:10
}SBAE_BEH_LEVEL_FACTOR;

// 危险驾驶行为类型，需与传入参数的顺序对应
typedef enum _SBAE_FUNC_TYPE_
{
	SBAE_DBA_FUNC_HCD      = 0x00,                               // 遮挡摄像头检测
	SBAE_DBA_FUNC_ABNORMAL = 0x01,                               // 驾驶员异常检测
	SBAE_DBA_FUNC_UPD      = 0x02,	                            // 打电话检测
	SBAE_DBA_FUNC_SMD      = 0x03,	                            // 抽烟检测
	SBAE_DBA_FUNC_BELT     = 0x04,                               // 安全带检测
	SBAE_DBA_FUNC_SUNGLASS = 0x05,                               // 红外阻断型墨镜检测
	SBAE_DBA_FUNC_NO_MASK  = 0x06,                               // 未佩戴口罩检测
	SBAE_DBA_FUNC_XUING    = 0x07,                               // 嘘手势识别
	SBAE_DBA_FUNC_HGR      = 0x08,                               // 静态手势识别  

	SBAE_DBA_FUNC_FDD      = 0x09,	                            // 疲劳驾驶检测
	SBAE_DBA_FUNC_NLF      = 0x0A,	                            // 不目视前方检测

	SBAE_DBA_FUNC_CLOTH    = 0x0B,                               // 工作服检测
	SBAE_DBA_FUNC_TALK     = 0x0C,                               // 注意力分散检测
	SBAE_DBA_FUNC_PUD      = 0x0D,                               // 捡拾物品检测
	SBAE_DBA_FUNC_END    = VCA_ENUM_END
}SBAE_FUNC_TYPE_E;


/********************************************************************************************************************
* 配置结构体
********************************************************************************************************************/
// 驾驶室报警等级
typedef struct _SBAE_DBA_BEH_LEVEL_INFO_
{
	int level_num;                                       // 需要报警的等级数 范围:[0,4],  默认:0
	SBAE_BEH_LEVEL_FACTOR lev_factor[SBAE_DBA_MAX_LEVEL_NUM];  // 各个等级的报警阈值
}SBAE_DBA_BEH_LEVEL_INFO;
/* @struct     SBAE_DBA_FDD_PARAMS_T
  * @brief     疲劳检测参数
  */
typedef struct _SBAE_DBA_FDD_PARAMS_T_
{
	int           fdd_switch;                // 疲劳功能开关，          范围:[0,1],   默认:1
	float         fdd_duration_slight;       // 轻度疲劳连续闭眼持续时间(单位s)， 范围:(0,10],  默认:1.0s
	float         fdd_duration_severe;       // 重度疲劳连续闭眼持续时间(单位s)， 范围:(0,30],  默认:1.5s
	float         fdd_duration_yawn;		 // 打哈欠疲劳驾驶持续时间(单位s)，   范围:(0,10],  默认:3
	short         blink_fdd_num_th;          // 1分钟内眨眼n次，每次半秒以上报为疲劳，默认10。[0,60]
	short         is_blink_clear;            // 若为1，则n次后清空，否则持续
	int           blink_range;               // 眨眼统计区间，范围(0,600]，默认60

	float         perclos_ratio;             // perclos的闭眼比例,                范围:(0,100], 默认:50
	float         perclos_range;             // perclos的闭眼范围,                范围:(0,10],  默认:3s
	int           fdd_sensitive;             // 疲劳驾驶报警灵敏度,               范围:(0,100], 默认:50
	int           speed_thresh;              // 报警速度阈值                      范围:[0,255], 默认:0
	SBAE_DBA_BEH_LEVEL_INFO bev_lev_info;         // 报警行为等级信息
	unsigned char reserved[16];		         // 预留
}SBAE_DBA_FDD_PARAMS_T;



/* @struct     SBAE_DBA_NLF_PARAMS_T
  * @brief     不目视前方功能参数
  */
typedef struct _SBAE_DBA_NLF_PARAMS_T_
{ 
	int           nlf_switch;                // 不目视前方功能开关，    范围:[0,1],    默认:1
	float         nlf_duration;              // 不目视前方持续时间，    范围:(0,30],   默认:5
//*--------------center--camera-----
//                 |    /
//                 |   /
//                 |  /
//                 | /
//               driver
	int           dist_driver_to_center;     // 司机到仪表盘的距离，单位cm,默认80cm
	int           dist_center_to_camera;     // 相机到中心的距离，  单位cm,默认10cm
	int           turn_sensitive[4];         // 转向(左、右、上、下)灵敏度，范围:(0,100],  默认:[25，25，40，10]
	int           speed_thresh;              // 报警速度阈值,       范围:[0,255], 默认:0
	SBAE_DBA_BEH_LEVEL_INFO bev_lev_info;         // 报警行为等级信息
	unsigned char reserved[28];		         // 预留
}SBAE_DBA_NLF_PARAMS_T;

/* @struct     SBAE_DBA_BEHAVIOR_PARAMS_T
  * @brief     危险行为(打电话、抽烟、安全带、捡拾物品等)功能参数
  */
typedef struct _SBAE_DBA_BEHAVIOR_PARAMS_T_
{ 
	SBAE_FUNC_TYPE_E type;                      // 行为类型
	unsigned char bev_duration;              // 危险行为持续时间,    范围:(0,10], 默认:3,  参数列表最大128byte,采用unsigned char为了节约空间
	unsigned char bev_sensitive;             // 危险行为报警灵敏度,  范围:(0,100], 默认:50 参数列表最大128byte,采用unsigned char为了节约空间
	unsigned char reserved[2];		         // 预留
	int           speed_thresh;              // 报警速度阈值,       范围:[0,255], 默认:0
}SBAE_DBA_BEHAVIOR_PARAMS_T;

// 架设位置选项(第0位表示架设的位置，第1位表示相机横竖)
typedef enum _SBAE_DBA_INSTALL_LOCATION_
{
	SBAE_DBA_INSTALL_MIDDLE   = 0x00,	             // 正装
	SBAE_DBA_INSTALL_LEFT     = 0x01,	             // 左装（A柱中）
	SBAE_DBA_INSTALL_RIGHT    = 0x02,	             // 右装（中控台）
	SBAE_DBA_INSTALL_MIRROR   = 0x03,	             // 后视镜

	SBAE_DBA_INSTALL_VERTICAL = 0x10,	             // 竖装

	SBAE_DBA_INSTALL_END    = VCA_ENUM_END
}SBAE_DBA_INSTALL_LOCATION;

// DBA - 项目ID，用于进行各项目定制
typedef enum _SBAE_DBA_PROJ_ID_
{
	SBAE_DBA_PROJ_COMMON  = 0x00,             // 默认，通用项目
	SBAE_DBA_PROJ_ES11    = 0x01,             // 长城ES11项目
	SBAE_DBA_PROJ_CX11    = 0x02,             // 吉利CX11项目
	SBAE_DBA_PROJ_D22     = 0x03,             // 小鹏D22项目
	SBAE_DBA_PROJ_B561    = 0x04,             // 长安B561项目
	SBAE_DBA_PROJ_ID_END  = VCA_ENUM_END
}SBAE_DBA_PROJ_ID;

//目标分类属性
typedef struct _SBAE_OBJ_ATTR_
{
    int             type_num;                       // 属性个数
    unsigned int    attr_type[SBAE_MAX_TYPE_NUM];    // 属性类型
    unsigned short  attr_conf[SBAE_MAX_TYPE_NUM];    // 属性置信度(归一化1到100）
}SBAE_OBJ_ATTR; // 52 BYTES

/* @struct     SBAE_DBA_COMMON_PARAMS_T
  * @brief     通用功能参数
  */
typedef struct _SBAE_DBA_COMMON_PARAMS_T_
{ 
	int                       *function_status;             // 状态字（打桩码）
	void                      *inner_debug_info;            // 外部传入的用于算法内部调试的指针(产品调用时无须关注)
	unsigned int               frame_idx;                   // 图像帧序号
	int                        frame_width;                 // 算法处理的图像宽
	int                        frame_height;                // 算法处理的图像高
	float  					   fps;							// 帧率
	int                        cur_speed;                   // 当前车速（默认为0） 
	unsigned int               time_stamp;                  // 外部传入的时间戳
	SBAE_DBA_PROC_MODE_E              dba_proc_mode;               // DBA处理模式，0x00-自适应, 0x01 - 过检， 0x02 - 实车
	SBAE_DBA_INSTALL_LOCATION       dba_install_loc;             // DBA架设位置
	VCA_RECT_I                 proc_area;                   // 算法的处理区域（如主、副、后排）
	SBAE_DBA_PROJ_ID                proj_ID;                     // 项目ID，用于定制
	unsigned char              reserved[12];		        // 预留
}SBAE_DBA_COMMON_PARAMS_T;

/* @struct   SBAE_DBA_PROC_PARAMS_T
  * @brief   DBA参数配置结构体
  */
typedef struct _SBAE_DBA_PROC_PARAMS_T
{
	SBAE_DBA_FDD_PARAMS_T              stFddParams;                           // 疲劳检测参数输入
	SBAE_DBA_NLF_PARAMS_T              stNlfParams;                           // 不目视前方检测参数输入	
    SBAE_DBA_COMMON_PARAMS_T           stCommonParams;                        // DBA通用参数
	SBAE_DBA_BEHAVIOR_PARAMS_T         stBehaviorInput[SBAE_MAX_FUN_NUM];     // 危险行为(打电话、抽烟、安全带、捡拾物品等)功能参数	
    int                                stBehaviorNum;                         // 危险行为的个数
	SBAE_OBJ_ATTR               share_params;                // 共享参数（算法内部占用,外部不需要赋值）
	unsigned char			   reserved[96];	            // 预留 
}SBAE_DBA_PROC_PARAMS_T;

/********************************************************************************************************************
* 输入结构体
********************************************************************************************************************/
/* @struct   SBAE_INPUT_INFO_T
  * @brief   输入信息
  */
typedef struct _SBAE_INPUT_INFO_T_
{  
    void                *pData;                         // 图像数据
    int                  nSpeed;                        // 车速
    int                  byReserved[16];                // 预留 
}SBAE_INPUT_INFO_T;

/*******************************************************************************************************************
* 输出结构体定义
********************************************************************************************************************/
/* @struct   SBAE_OBJ_MDM_T
  * @brief   // 测速测距信息（相机坐标系）
  */
typedef struct _SBAE_OBJ_MDM_T_
{
    SBAE_MDM_TYPE_E     eMdmType;                    // 测距类型
    VCA_SIZE_F          stObjSize;                   // 目标实际尺寸（宽度和高度 单位m）
    VCA_3D_POINT_F      stRvfPos;                    // 综合后的相对位置[m]
    VCA_POINT_F         stRvfVel;                    // 综合后的相对速度[m/s]（靠近>0 远离<0）
    VCA_3D_POINT_F      stCamPos;                    // 相机相对位置[m]
    VCA_POINT_F         stCamVel;                    // 相机相对速度[m/s]（靠近>0 远离<0）

    VCA_POINT_F         stRelAcc;                    // 相对加速度[m/s^2]（速度梯度信息）
    SBAE_MOTION_TYPE_E  eMotionType;                 // 目标运动类型
    int                 nTtc;                        // 碰撞时间[ms] 0~255,000

    VCA_POINT_F         stCoordinate[4];             // 目标实际坐标[m]：纵向横向（左前-右前-右后-左后）
    short               sCoordConf;                  // 当前目标视觉测距的置信度
	short               sAngle;                      // 目标车头朝向 -180~180度
	//unsigned char       importance;                // 重要性排序队列1~n，(CIPV车辆+小方位角目标排序）,CIPV模块维护,MOT策略使用
	unsigned char       byCutInOut;                  // ==0未知 ==1在本车道  ==2不在本车道 ==3 CutIn ==4 Cutout
  unsigned char       obj_is_show;                // 是否输出目标（0-不显示  1-显示）
	unsigned char       byReserved[2];               // 保留字节
} SBAE_OBJ_MDM_T;  

/* @struct   SBAE_BEHAVIOR_INFO
  * @brief   报警信息输出
  */
typedef struct _SBAE_BEHAVIOR_INFO_T_
{
    SBAE_BEHAVIOR_TYPE_E eType;                       // 行为类型
    unsigned short       usLevel;                     // 行为等级，值域[1,100]
    unsigned short       usConf;                      // 报警置信度，值域[1,100]
	int                  nObjValid;                   // 报警时是否存在有效目标，1为有效，0为无效
	VCA_RECT_F           stRect;                      // 目标图像位置（非图像目标该值无效）
    int                  nObjId;                      // 目标Id，与目标输出队列匹配
    SBAE_OBJ_MDM_T      *pstMdmInf;                   // 位置速度信息
    int                  nAttrType;                   // 报警目标的属性（不生效）
	int                  nSensorId;                   // 鱼眼图的id
	SBAE_OBJ_TYPE_E      eObjType;                    // 鱼眼图目标类型
    unsigned char        byReserved[28];              // 预留
} SBAE_BEHAVIOR_INFO_T;    

// 车载异常行为对外输出的警告信息（无人脸、人脸过大、过小等）	
typedef struct _SBAE_ABNORMAL_WARNING_
{
	SBAE_BEHAVIOR_TYPE_E   type;                              // 警告类型
	unsigned short         level;                             // 警告等级，值域[1,100]	
	unsigned char		       reserved[32];		                  // 预留
} SBAE_ABNORMAL_WARNING;

/* @struct   SBAE_OUTPUT_INFO
  * @brief   结果输出
  */
typedef struct _SBAE_OUTPUT_INFO_T_
{
	int                    nAlarmNum;                   // DBA报警个数
	SBAE_BEHAVIOR_INFO_T   stResInfo[SBAE_MAX_RES_NUM]; // DBA输出结果队列
  SBAE_ABNORMAL_WARNING  dba_warning;                      // DBA异常警告码（无人脸、人脸过大、过小等）	
	unsigned char		       byReserved[32];		        // 预留
} SBAE_OUTPUT_INFO_T;

/************************************************POS信息输出********************************************************************/
/* @struct   SBAE_POS_MOD_OBJ_T
  * @brief   精简版的检测信息
  */
typedef struct _SBAE_POS_MOD_OBJ_T_
{
    VCA_RECT_S             stObjPos;                     // 目标位置
    int                    nObjProb;                     // 目标置信度（检测），输出范围，[0,100]
} SBAE_POS_MOD_OBJ_T;

/* @struct   SBAE_POS_MOD_INFO_T
  * @brief   需要显示在Pos的感知框的信息
  */
typedef struct _SBAE_POS_MOD_INFO_T_
{
    int                    nObjNum;                       // 目标数量
    SBAE_POS_MOD_OBJ_T     stObjArray;                    // 目标列表
}SBAE_POS_MOD_INFO_T;

/* @struct   SBAE_DBA_NLF_INFO_T
  * @brief   不目视前方结构体
  */
typedef struct _SBAE_DBA_NLF_INFO_T_
{
    int                     nNlfLastSeconds;              // 不目视前方持续时间，    范围:(0,10],   默认:5
    short                   sTurnSensitive[4];            // 转向(左、右、上、下)灵敏度，范围:(0,100],  默认:[50，50，50，50]

    float                   fAngle[4];                    // mop输入的角度值
    short                   sNlfCount[4];                 // 累积帧数

    float                   fNlfPoseBench[3];             // 实车架设基准角度
    float                   fNlfRelativeAngle[3];         // 实车相对角度
    unsigned char           byNlfHeadTurn[4];             // 转头动作状态（左、右、上、下）

    unsigned char           byReserved[32];
}SBAE_DBA_NLF_INFO_T;

/* @struct   SBAE_DBA_FDD_INFO_T
  * @brief   疲劳驾驶结构体
  */
typedef struct _SBAE_DBA_FDD_INFO_T_
{
    float                  fFddDurationSevere;             // 重度疲劳驾驶持续时间，  范围:(0,10],  默认:3
    float                  fFddDurationSlight;             // 轻度疲劳驾驶持续时间，  范围:(0,10],  默认:3
    float                  fFddDurationYawn;		       // 打哈欠疲劳驾驶持续时间，范围:(0,10],  默认:3
    short                  sFddSensitive;                  // 疲劳驾驶报警灵敏度,     范围:(0,100], 默认:50

    short                  sEyeClosedCount;                // 3s内闭眼帧数统计
    short                  sContinuousEyeClosedCnt;        // 2s内连续闭眼帧数统计
    short                  sMouthOpenCount;                // 3s内打哈欠帧数统计
    float                  fContinuousEyeClosedRate;       // 2s内连续闭眼比率

    float                  fMouthDistAve;                  // 嘴部平均距离
    float                  fMouthDistRate;                 // 嘴部高宽比
    float                  fEyelidDist;
    int                    nContinuousMouthOpenCnt;
    int                    nFilterFlag;                    // 被过滤标志位，包含戴墨镜、打电话、抽烟、喝酒、揉眼睛
    float                  fFddLevel;                      // 疲劳帧积累程度
    float                  fFddLevelAccu;                  // 单帧疲劳帧积累程度

    // 网络分类置信度
    unsigned char          byFddPro[4];                    // 单帧疲劳模型的分类结果
    unsigned char          byFddVideoPro[4];               // 视频疲劳模型的分类结果
    unsigned char          byYawnPro[4];                   // 打哈欠模型的分类结果
    float                  fGlassPro[3];

    unsigned char          byReserved[32];
}SBAE_DBA_FDD_INFO_T;

/* @struct   SBAE_DBA_SBD_INFO_T
  * @brief   打电话中间结果结构体
  */
typedef struct _SBAE_DBA_SBD_INFO_T_
{
    SBAE_FUNC_TYPE_E       eType;                          // 行为类型
    short                  sBevDuration;                   // 危险行为持续时间,    范围:(0,10], 默认:3
    short                  sBevSensitive;                  // 危险行为报警灵敏度,  范围:(0,100], 默认:50
    short                  sBevAlertTh;                    // 报警阈值
    short                  sBevAlertCnt;                   // 多帧累积状态
}SBAE_DBA_SBD_INFO_T;


/* @struct   SBAE_DBA_POS_T
  * @brief   POS结果输出
  */
typedef struct _SBAE_DBA_POS_HANDLE_T_
{
    // 1、外部输入
    unsigned int           unVersion;                      // 图像帧序号
    unsigned int           unFrameIdx;                     // 图像帧序号
    short                  sFrameWidth;                    // 图像宽
    short                  sFrameHeight;                   // 图像高
    float			   	         fFps;			               // 帧率
    int                    nCurSpeed;                      // 当前车速（默认为0） 
    unsigned long long     unTimeStamp;                    // 外部传入的时间戳

    // 2、运行中关键变量
    SBAE_POS_MOD_INFO_T    stPosMod[6];                    // 感知的类别,0-人脸 1-打电话 2-抽烟 3-安全带

    int head_persist_num;
    
    // 3、各模块中间结果信息（含各模块的输入参数）
    SBAE_DBA_NLF_INFO_T    stDbaNlfInfo;                   // 不目视前方信息
    SBAE_DBA_FDD_INFO_T    stDbaFddInfo;                   // 疲劳驾驶结构体
    SBAE_DBA_SBD_INFO_T    stDbaSbdInfo[SBAE_MAX_FUN_NUM]; // 行为分析         

    // 3、DBA输出
    SBAE_BEHAVIOR_TYPE_E   eDbaResult[SBAE_MAX_FUN_NUM];    // DBA输出结果队列

    unsigned char          byReserved2[32];
} SBAE_DBA_POS_T;

/* @struct   SBAE_OUTPUT_T
  * @brief   引擎结果输出
  */
typedef struct _SBAE_OUTPUT_T_
{
    SBAE_OUTPUT_INFO_T      stOutInfo;                     // 引擎结果信息 
    SBAE_DBA_POS_T          stPosInfo;                     // POS信息
} SBAE_OUTPUT_T;


#endif