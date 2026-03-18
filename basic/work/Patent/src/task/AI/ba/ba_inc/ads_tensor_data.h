/***************************************************************************************************
* 版权信息：版权所有(c) , 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称: ads_tensor_data.h
* 文件标识: _ADS_TENSOR_DATA_H_
* 摘    要: 定义各张量输入输出数据基本单元
* 数据分为参数信息、传感器信息、目标感知信息
* 为了加强规范，置信度、灵敏度、重叠度等度量都需要转换到1~100的定点数
* 历史版本: 1.0.0
* 作    者: 邝宏武
* 日    期: 2019-3-26
* 备    注: 新建张量元素定义
***************************************************************************************************/
#ifndef _ADS_TENSOR_DATA_H_
#define _ADS_TENSOR_DATA_H_
#include "hik_apt_srl_syntax.h"
#include "vca_base.h"

#define ADS_MAX_TYPE_NUM            8       // 最大分类的类别数
#define ADS_MAX_KEYPT_NUM           8       // 关键点最大个数
#define ADS_MAX_FEAT_DIM            128     // 目标最大提取特征维度
#define ADS_MAX_3D_POS_FEAT_DIM     20      // 最大人脸3D姿态维度
#define ADS_RSA_MAX_SCANLINE        640     // 最大扫描线长度
#define ADS_RSA_CURVE_PARAM_NUM     4       // 曲线参数个数
#define ADS_RSA_MAX_LINE_NUM       16       // 总车道线数
#define ADS_RSA_MAX_LANE_NUM        5       // 总车道个数

// 车身CAN信息标志位
#define ADS_SENSOR_VEHINFO_MASK_SPEED        (0x1U<<0)
#define ADS_SENSOR_VEHINFO_MASK_BRAKE        (0x1U<<1)
#define ADS_SENSOR_VEHINFO_MASK_TURN         (0x1U<<2)
#define ADS_SENSOR_VEHINFO_MASK_STEERING     (0x1U<<3)
#define ADS_SENSOR_VEHINFO_MASK_GEAR         (0x1U<<4)
#define ADS_SENSOR_VEHINFO_MASK_WIPER        (0x1U<<5)
#define ADS_SENSOR_VEHINFO_MASK_LOW_BEAM     (0x1U<<6)
#define ADS_SENSOR_VEHINFO_MASK_HIGH_BEAM    (0x1U<<7)
#define ADS_SENSOR_VEHINFO_MASK_STOP_LIGHT   (0x1U<<8)
#define ADS_SENSOR_VEHINFO_MASK_BACKUP_LIGHT (0x1U<<9)
#define ADS_SENSOR_VEHINFO_MASK_BATTERY_VOL  (0x1U<<10)
#define ADS_SENSOR_VEHINFO_MASK_MILEAGE      (0x1U<<11)
#define ADS_SENSOR_VEHINFO_MASK_FOG_LIGHT    (0x1U<<12)
#define ADS_SENSOR_VEHINFO_MASK_HAZARD_LIGHT  (0x1U<<13)
#define ADS_SENSOR_VEHINFO_MASK_OPEN_THE_DOOR (0x1U<<14)

//目标类型
typedef enum SRLM_DECORATOR _ADS_OBJ_TYPE_
{
    ADS_OBJ_UNKNOWN            = 0,           //未知  0
    ADS_OBJ_NEGATIVE           = 0x01,        //无效目标  1
    ADS_OBJ_GENERAL            = 0x02,        //通用障碍物  2
    ADS_OBJ_SONAR              = 0x10,        //超声波探测目标  16
    ADS_OBJ_RADAR              = 0x20,        //毫米波探测目标  32
    ADS_OBJ_LIDAR              = 0x30,        //激光探测目标  48

    ADS_OBJ_VEHICLE            = 0x100,       //机动车 0x1XX  256
    ADS_OBJ_LARGE_VEHICLE      = 0x101,       //大车 车辆种类从101~11F  257
    ADS_OBJ_SMALL_VEHICLE      = 0x102,       //小车  258
    ADS_OBJ_BUS                = 0x103,       //公交  259
    ADS_OBJ_TRUCK              = 0x104,       //货车  260

    ADS_OBJ_VEHICLE_HEAD       = 0x120,       //车头 车辆部件从120~12F  288
    ADS_OBJ_VEHICLE_TAIL       = 0x121,       //车尾  289
    ADS_OBJ_VEHICLE_FRONT_FACE = 0x122,       //车前脸  290
    ADS_OBJ_VEHICLE_BACK_FACE  = 0x123,       //车后脸  291
    ADS_OBJ_WHEEL              = 0x124,       //车轮  292
    ADS_OBJ_PLATE              = 0x125,       //车牌  293
    ADS_OBJ_CAR_WINDOW         = 0x126,       //车窗  294

    ADS_OBJ_NON_VEHICLE        = 0x200,       //非四轮车0x2XX  512
    ADS_OBJ_TWO_WHEEL          = 0x201,       //二轮车  513
    ADS_OBJ_MOTOR              = 0x202,       //摩托车  514
    ADS_OBJ_CYCLIST            = 0x203,       //自行车  515
    ADS_OBJ_THREE_WHEEL        = 0x204,       //三轮车  516
    ADS_OBJ_TWO_WHEEL_UNMANNED = 0x205,       //无人二轮车  517

    ADS_OBJ_HUMAN              = 0x300,       //行人0x3XX  768
    ADS_OBJ_FACE               = 0x310,       //人脸  784
      
    ADS_OBJ_ROAD               = 0x400,       //路面0x4XX  1024
    ADS_OBJ_LANE               = 0x401,       //车道线  1025
    ADS_OBJ_ROADMARKING        = 0x402,       //路面标志  1026
    ADS_OBJ_ZEBRA              = 0x403,       //斑马线  1027
    ADS_OBJ_ROADEDGE           = 0x404,       //路沿  1028
    ADS_OBJ_PARK               = 0x410,       //车位线  1040
    ADS_OBJ_PARK_POINT         = 0x411,       //车位角点 1041

    ADS_OBJ_TRAFFICSIGN        = 0x420,       //交通标志，包括限速标志、限重标志等  1056
    ADS_OBJ_TRAFFICLIGHT       = 0x421,       //交通灯  1057

    ADS_OBJ_CONE               = 0x450,       //三角堆  1104

    ADS_OBJ_SMOG               = 0x510,       //烟雾  1296
    ADS_OBJ_FLAME              = 0x520,       //火灾  1312
    ADS_OBJ_BANNER             = 0x530,       //条幅  1328
    ADS_OBJ_BOOTH              = 0x540,       //摊位  1344

	ADS_OBJ_DIRECTIONSIGN      = 0x550,       //导流牌  1360
	ADS_OBJ_ANTICOLLISION      = 0x560,       //防撞条  1376
	ADS_OBJ_FIRE_HYDRANT       = 0x570,       //消防栓  1392
	ADS_OBJ_GATE               = 0x580,       //闸机杆  1408

	ADS_OBJ_DRIVER             = 0xA01,       // 司机行为
	ADS_OBJ_FRONT_PASSENGER    = 0xA02,       // 前排乘客行为
	ADS_OBJ_BACK_PASSENGER     = 0xA04,       // 后排乘客行为

    ADS_OBJ_END                = VCA_ENUM_END
} ADS_OBJ_TYPE;


/******************************************* 参数信息 *****************************************/
// IMU信息
typedef struct SRLM_DECORATOR _IMU_DATA_
{
    int     Z;                                        // z方向加速度 单位 10e-7 m/s2
    int     Y;                                        // y方向加速度 单位 10e-7 m/s2
    int     X;                                        // x方向加速度 单位 10e-7 m/s2
    int     ZG;                                       // z方向陀螺仪转动数据 单位 10e-6 °/s
    int     YG;                                       // y方向陀螺仪转动数据 单位 10e-6 °/s
    int     XG;                                       // x方向陀螺仪转动数据 单位 10e-6 °/s
}IMU_DATA; // 24字节

// CAN参数
typedef struct SRLM_DECORATOR _CAN_DATA_TYPE_
{
    unsigned int   vechileFlg;                    // 标识速度等信息的有效性
    unsigned int   front_left_speed[4];           // 前左轮速度信号, 单位0.01Km/h，接收4次速度信息，速度始终为正数
    unsigned int   front_right_speed[4];          // 前右轮速度信号, 单位0.01Km/h，接收4次速度信息，速度始终为正数
    unsigned int   back_left_speed[4];            // 后左轮速度信号, 单位0.01Km/h，接收4次速度信息，速度始终为正数
    unsigned int   back_right_speed[4];           // 后右轮速度信号, 单位0.01Km/h，接收4次速度信息，速度始终为正数
    unsigned char  brake;                         // 刹车信号，0：表示没踏刹车;1：表示踏刹车
    unsigned char  turn;                          // 转向灯信号，0：表示没打转向灯;1：表示打左转向灯;2：表示打右转向灯;3：表示打双闪
    short          steering_angle;                // 方向盘转角信号，正数：表示方向盘向右转，负数：表示方向盘向左转;其绝对值数字越大，表明所打的幅度就越大;
    unsigned char  gear;                          // 档位信号，0:表示空档;1：表示前进档;2：表示后退档;3：表示P档
    unsigned char  wiper;                         // 雨刮信号，0：表示没启动;1：表示启动一级;2：表示启动2级;3：表示启动3级
    unsigned char  low_beam;                      // 近光灯信号，0：表示没启动;1：表示启动
    unsigned char  high_beam;                     // 远光灯信号，0：表示没启动;1：表示启动
    unsigned char  stop_lamp;                     // 刹车灯信号，0：表示没启动;1：表示启动
    unsigned char  back_up_light;                 // 倒车灯信号，0：表示没启动;1：表示启动
    unsigned short bat_vol;                       // 电池电压信号，单位：0.1V 
    unsigned int   trip_distance;                 // 车辆行驶里程，单位：km
    unsigned int   engine_speed;                  // 发动机转速，单位：round/min
    unsigned char  fog_lamp;                      // 雾灯信号，0：表示没启动;1：表示启动
    unsigned char  hazard_light;                  // 双闪灯信号，0：表示没启动;1：表示启动
    short          front_wheel_angle;             // 前轮偏转角，左负右正，范围-1000到1000，精度0.1度
    short          angleSpeed;                    // 车身横摆角速度,范围:-81.91~81.91°/s,精度:0.01°/s 
    short          rateSpeed;                     // 车身横向加速度,范围:-12.45~12.45m/s2,精度0.01m/s2 
    unsigned int   refresh_flag;                  // 数据刷新标志，0：表示未刷新；1：表示已刷新
	unsigned int   open_the_door;                  // 开关门信号，0：表示未开门；1：表示开门
    unsigned char  reserved[8];                  // 预留字节 
} CAN_DATA_TYPE;                                  // 112 Bytes

typedef struct SRLM_DECORATOR _CAN_LOC_DATA_TYPE_
{
    int                valid;
    unsigned int       time_stamp;           // 时间戳
    short              VehFLPulse;           // 左前车轮脉冲
    short              VehFRPulse;           // 右前车轮脉冲
    short              VehRLPulse;           // 左后车轮脉冲
    short              VehRRPulse;           // 右后车轮脉冲
    short              VehGearPos;           // 车身当前档位 1 - P，2 - N，3 - R，4 - D
    short              VehWAngle;            // 车身方向盘转角
    short              vehicle_speed;        // 车速  0.01 Km/h
    unsigned char      reserved[90];         // 预留字节
} CAN_LOC_DATA_TYPE;                         // 112 Bytes

// GPS参数
typedef struct SRLM_DECORATOR _GPS_POS_DATA_
{
    unsigned int  SOL_Status;                     //解算方式，解算状态 0:解算完成
    unsigned int  POS_Type;                       //位置类型，得到的位置状态 50：NARROW_INT
    double        Lat;                            //纬度，北纬为正，单位为度
    double        Lon;                            //经度，东经为正，单位为度
    double        Hgt;                            //高度，天向为正，单位为米
    float         Undulation;                     //坐标系常差，海拔高度和WGS-84坐标系高度常差
    unsigned int  DatumID;                        //坐标系ID 默认WGS-84坐标系
    float         Latdev;                         //纬度标准差，单位米
    float         Londev;                         //经度标准差，单位米
    float         Hgtdev;                         //高度标准差，单位米
    char          StnID[4];                       //基站ID,4组(多基站下使用)
    float         Diff_age;                       //差分时间，计算差分信号的时间
    float         Sol_age;                        //解算时间，解算耗时
    unsigned char SV;                             //跟踪卫星星数，跟踪到的卫星星数
    unsigned char solnSV;                         //解算星数，参与解算的卫星星数
    unsigned char solnL1SV;                       //单频卫星信号个数
    unsigned char solnMultiSVs;                   //双频卫星信号个数
    unsigned char Reserved;                       //预留位
    unsigned char extsolstat;                     //扩展解算方式，如RTK认证
    unsigned char location_mask1;                 //Galile_BD掩码，Galile和北斗使用标志，
    //bit0=1表示GalileoE1使用，bit1-3留用
    //bit4 北斗B1使用，bit5北斗B2使用，bit6-7 留用
    unsigned char location_mask2;                 //GPS_GLONASS掩码，GPS系统和GNSS系统使用标志，
    //bit0=1 GPS L1使用，bit1=1 GPS L2使用，bit2=1 GPS L5使用，bit3留用
    //bit4 GNSS L1使用，bit5 GNSS L2使用，bit6-7留用
    unsigned int crc;                             //校验码
    unsigned char reserved[4];                    //保留字节
} GPS_POS_DATA; /*80Bytes*/

// 速度数据
typedef struct SRLM_DECORATOR _GPS_VEL_DATA_
{
    unsigned int SOL_Status;                      //解算方式，解算状态 0：解算正常
    unsigned int VEL_Type;                        //速度类型，得到速度的状态 50：NARROW_INT
    float        Latency;                         //速度延时，得到速度的延时，单位S
    float        Age;                             //差分时间，计算差分信号的时间
    double       HorSpd;                          //水平速度，为非负数单位 m/s
    double       TrkGnd;                          //航向角，正北方向为0，单位度
    double       VerSpd;                          //垂直速度，上正下负，单位m/s
    float        Reserved;                        //预留位
    unsigned int crc;                             //校验码  
} GPS_VEL_DATA; /*48Bytes*/


typedef struct SRLM_DECORATOR _GPS_INSPVAS_DATA_
{
    unsigned int Ins_flag;                        //联合输出标志位
    unsigned int GNSSWeek;                        //GNSS周数，自1980-1-6日起的周数
    double       Secs;                            //周内秒，周内运行的秒数
    double       Latitude;                        //纬度，北纬为正，单位度
    double       Longitude;                       //经度，东经为正，单位度
    double       Height;                          //高度，天向为正，单位为米
    double       NorthV;                          //北向速度，北正南负，单位米/秒
    double       EastV;                           //东向速度，东正西负，单位米/秒
    double       UpV;                             //天向速度，上正下负，单位米/秒
    double       Roll;                            //侧倾角，右正左负，单位度
    double       Pitch;                           //俯仰角，上正下负，单位度
    double       Azimuth;                         //横摆角，右正左负，单位度
    unsigned int status;                          //组合导航状态，解算状态良好 
    unsigned int crc;                             //校验码
} GPS_INSPVAS_DATA; /*96Bytes*/


typedef struct SRLM_DECORATOR _GPS_DATA_TYPE_
{
    GPS_POS_DATA     locationPos;                // GPS消息数据
    GPS_VEL_DATA     locationVel;                // GPS速度信息
    GPS_INSPVAS_DATA insData;                    // 组合导航消息数据
} GPS_DATA_TYPE;     /* 224 Bytes*/

// 畸变参数结构体
typedef struct SRLM_DECORATOR _ADS_CAM_DISTORTION_
{
    float k_1;  // 径向畸变参数1
    float k_2;  // 径向畸变参数2
    float p_1;  // 切向畸变参数1
    float p_2;  // 切向畸变参数2
    float k_3;  // 径向畸变参数3
    float k_4;  // 径向畸变参数4
}ADS_CAM_DISTORTION;

// 相机内参
typedef struct SRLM_DECORATOR _ADS_CAM_INNER_
{
    VCA_POINT_F               focal_length;     // 相机焦点
    VCA_POINT_F               optical_center;   // 光心
    VCA_POINT_F               ccd_size;         // ccd实际尺寸(单位：mm)
    float                     focal_dist;       // 焦距(单位：mm)
    ADS_CAM_DISTORTION        distortion;       // 畸变参数
    unsigned char             distortion_type;  // 畸变类型 1为普通相机标定文件，2为鱼眼相机标定文件
    unsigned char             reserved[3];      // 保留字节
}ADS_CAM_INNER; // 56字节

// 相机外参
typedef struct SRLM_DECORATOR _ADS_CAM_OUTER_
{
    float           veh_w;                      // 车辆宽度x
    float           veh_l;                      // 车辆长度y
    float           veh_h;                      // 车辆高度z
    VCA_3D_POINT_F  cam_pos;                    // 相机相对车辆坐标(0~1.0)，从左到右，从前到后，从上到下

    float           cam_pitch;                  // 相机安装-俯仰角
    float           cam_yaw;                    // 相机安装-偏转角
    float           cam_roll;                   // 相机安装-旋转角

    float           vanish_y;                   // 相机消隐线参数（归一化坐标0~1，从上往下计，常规为0.5）
    unsigned char   reserved[4];                // 保留字节
}ADS_CAM_OUTER; // 44字节

// 通用传感器外参
typedef struct SRLM_DECORATOR _ADS_SENSOR_OUTER_
{
    float       yaw;                            // 偏转角（rad）
    float       pitch;                          // 俯仰角（rad）
    float       roll;                           // 旋转角（rad）
    float       horizontal;                     // 水平偏移（m）
    float       vertical;                       // 垂直偏移（m）
    float       height;                         // 高度偏移（m）
}ADS_SENSOR_OUTER; // 24字节

// 时间模式
typedef enum SRLM_DECORATOR _ADS_TIME_MODE_TYPE_
{
    ADS_TIME_INVALID = 0,       // 无效值
    ADS_TIME_DAY = 1,       // 白天
    ADS_TIME_NIGHT = 2,       // 夜晚
    ADS_TIME_DAWN_DUSK = 3,       // 黎明或黄昏
    ADS_TIME_END = VCA_ENUM_END
} ADS_TIME_MODE_TYPE;
/******************************************* 传感器信息 *****************************************/

// 激光雷达传感器数据存储协议
// shape[0]: lidar_num              传感器个数，默认为1
// shape[1]: orient_num             总的方位角数 = 包数*每个包采样的方位角
// shape[2]: laser_lines_num        扫描线数
// shape[3]: 32                     底层的3个INT32数据+保留字
typedef struct SRLM_DECORATOR _LIDAR_DATA_
{
    int     lidar_distance;                     // 距离
    int     lidar_reflectivity;                 // 反射强度
    int     lidar_yaw;                          // 方位角
    int     reserved;                           // 保留字
}LIDAR_DATA;


// 毫米波CFAR数据
// shape[0]: cfar_num       
// shape[1] : 1            
// shape[2] : 1            
// shape[3] : 16          
typedef struct SRLM_DECORATOR _MM_CFAR_DATA_
{
    short          mm_vertical_distance;      // 雷达检测目标纵向距离 0.01米
    short          mm_horizontal_distance;    // 雷达返回目标横向距离 左负右正0.01米 
    short          mm_velocity;               // 雷达CFAR相对速度 前正后负 0.01米/秒
    short          mm_height;                 // 雷达CFAR高度 0.01米
    short          mm_ampitiude;              // 雷达CFAR幅度值（dB）
    short          mm_SNR;                    // 雷达CFAR信噪比（dB）
    short          mm_cfar_type;              // 雷达CFAR数据类型（0:无效, 1:前向窄波束CFAR, 2:前向宽波束CFAR, 3:前向聚类, 4:角雷达左前, 5:角雷达右前, 6：角雷达左后, 7：角雷达右后）
    char           mm_cfar_status;            // 雷达CFAR点云状态 (0：有效，1：无效，2：静止，3：运动)
    char           mm_cluster_num;            // 雷达CFAR点云聚类标签
} MM_CFAR_DATA;  //16字节

// 航迹数据
// shape[0]: track_num            
// shape[1]: 1                  
// shape[2]: 1                  
// shape[3]: 24                   
typedef struct SRLM_DECORATOR _MM_TRACK_DATA_
{
    short   mm_tracknum;                            // 雷达航迹编号 
    short   mm_vertical_distance;                   // 雷达检测目标纵向距离 0.01米
    short   mm_horizontal_distance;                 // 雷达返回目标横向距离 左负右正0.01米 
    short   mm_velocity;                            // 雷达返回目标速度  前正后负 0.01米/秒
    short   mm_height;                              // 雷达返回目标高度 0.01米
    short   mm_amplitude;                           // 雷达返回目标幅度（dB）
    short   mm_SNR;                                 // 雷达返回目标信噪比（dB）
    short   mm_state;                               // 雷达返回目标航迹状态
    short   mm_horizontal_velocity;                 // 雷达返回目标速度  左负右正 0.01米/秒
    short   mm_cluster_num;                         // 雷达航迹聚类标签
    short   reserved[2];                            // 预留字节
} MM_TRACK_DATA;// 24字节

// ADC数据存储协议：point_per_chirp * 2 * chirps_per_frame * rx_antenna_num
// shape[0]: tx_antenna_num             发射天线数
// shape[1]: rx_antenna_num             接收天线数
// shape[2]: chirps_per_frame           每帧中chirps数目
// shape[3]: point_per_chirp            chirp采样点数


// 单个超声波数据信号格式
// shape[0]: frame_num           多个时间点接收的超声波信号队列
// shape[1]: sonar_num           超声波信号队列
// shape[2]: group_num           单个超声波某一时刻获取的信号组数
// shape[3]: sizeof(SONAR_DATA)  单个超声波单组信号
typedef struct SRLM_DECORATOR _SONAR_DATA_
{
    unsigned short  distance;                       // 障碍物距离，范围：0-500cm，单位：cm
    unsigned short  echowidth;                      // 雷达回波宽度，范围：0-128us，单位：8us
    unsigned short  strength;                       // 雷达回波强度，为新一代超声波信号预留
    unsigned short  reserved;                       // 预留字节
} SONAR_DATA;

/******************************************* 检测目标信息 *****************************************/
// 目标检测信息
typedef struct SRLM_DECORATOR _ADS_OBJ_DET_
{
    VCA_RECT_F      rect;                           //目标检测位置
    int             id;                             //检测目标Id
    ADS_OBJ_TYPE    obj_type;                       //目标类型
    int             type_id;                        //目标类型序号（1,2,3,...用于多目标关联）

    unsigned char   confidence;                     //检测置信度[1,100]
    unsigned char   conf_flg;                       //置信度高低标志（0为低，1为高）
    unsigned char   type_ignore;                    //目标关联时类型是否忽略(1：关联时不用类型)
    unsigned char   sensor_id;                      //传感器编号1~

    short           pt_num;                         //关键点个数
    short           key_conf;                       //关键点置信度
    HIK_APT_SRLM_FIELD_VEC(0, pt_num, ADS_MAX_KEYPT_NUM)
    VCA_POINT_F     kpoint[ADS_MAX_KEYPT_NUM];      //关键点
    float           angle;                          //局部航向角[0,2*PI]
    int             angle_valid;                    //航向角有效标志位
}ADS_OBJ_DET; // 172 BYTES

//目标分类属性
typedef struct SRLM_DECORATOR _ADS_OBJ_ATTR_
{
    int             type_num;                       // 属性个数
    HIK_APT_SRLM_FIELD_VEC(0, type_num, ADS_MAX_TYPE_NUM)
    unsigned int    attr_type[ADS_MAX_TYPE_NUM];    // 属性类型
    HIK_APT_SRLM_FIELD_VEC(0, type_num, ADS_MAX_TYPE_NUM)
    unsigned short  attr_conf[ADS_MAX_TYPE_NUM];    // 属性置信度(归一化1到100）
}ADS_OBJ_ATTR; // 52 BYTES

// 目标特征提取方法
typedef enum SRLM_DECORATOR _ADS_FEAT_TYPE_
{
    ADS_FEAT_UNKNOW     = 0,                        // 未知
    ADS_FEAT_HIST       = 1,                        // 颜色直方图
    ADS_FEAT_CNN_COS    = 2,                        // CNN特征（余弦距离度量）
    ADS_FEAT_KEYPT      = 3,                        // 关键点特征
    ADS_FEAT_END        = VCA_ENUM_END
} ADS_FEAT_TYPE;

// 提取的目标特征
typedef struct SRLM_DECORATOR _ADS_OBJ_FEAT_
{
    ADS_FEAT_TYPE   type;                           // 特征类型
    float           data[ADS_MAX_FEAT_DIM];         // 特征数据
    int             dim;                            // 特征维度
}ADS_OBJ_FEAT;

//目标跟踪属性
typedef struct SRLM_DECORATOR _ADS_OBJ_TRACK_
{
    // MOT结果
    VCA_RECT_F      rect;                           //目标跟踪位置
    int             id;                             //跟踪目标id
    VCA_TR_STATE    state;                          //跟踪状态
    ADS_OBJ_TYPE    obj_type;                       //跟踪后的目标类型
    int             det_update;                        //检测信息是否更新
    short           pt_num;                         //关键点个数
    short           key_conf;                       //关键点置信度

    HIK_APT_SRLM_FIELD_VEC(0, pt_num, ADS_MAX_KEYPT_NUM)
    VCA_POINT_F     kpoint[ADS_MAX_KEYPT_NUM];      //关键点

    unsigned short  det_frm_cnt;                    //关联检测帧数累计
    unsigned short  lost_frm_cnt;                   //丢失帧数累计
    unsigned short  trk_frm_cnt;                    //跟踪帧数累计（从目标新建开始）
    unsigned char   static_flag;                    //目标静止标志[0静止，1运动]
    unsigned char   cut_deg;                        //截断程度[0~100]
    VCA_POINT_F     pred_vel;                       //预测速度(非归一化坐标)

    // VOT结果
    unsigned short  move_dis_flag;                  //单帧移动距离有效标志
    unsigned short  move_dis_conf;                  //单帧移动距离置信度
    VCA_POINT_F     move_dis;                       //单帧移动距离（亚像素级）
    float           size_scale;                     //单帧目标尺度缩放比率
    // 透传
    unsigned char   sensor_id;                      //传感器编号1~
    unsigned char   reserrved[15];                  //预留字节
}ADS_OBJ_TRACK; // 96字节 + 68字节


// 测距类型枚举
typedef enum SRLM_DECORATOR _ADS_MDM_TYPE_
{
    ADS_MDM_INVALID = 0,                            // 无效
    ADS_MDM_VIDEO   = 1,                            // 相机测距
    ADS_MDM_RADAR   = 2,                            // 雷达测距
    ADS_MDM_SONAR   = 3,                            // 超声波测距
    ADS_MDM_RVF     = 4,                            // 融合测距
    ADS_MDM_END     = VCA_ENUM_END
}ADS_MDM_TYPE;

// 运动状态
typedef enum SRLM_DECORATOR _ADS_MOTION_TYPE_
{
    ADS_MOTION_INVALID      = 0,                    // 不明
    ADS_MOTION_STATIC       = 1,                    // 目标(绝对)静止
    ADS_MOTION_MOVE         = 2,                    // 运动
    ADS_MOTION_MOVE_ACCE    = 3,                    // 加速运动
    ADS_MOTION_MOVE_DECE    = 4,                    // 减速运动
    ADS_MOTION_MOVE_CONST   = 5,                    // 匀速运动
    ADS_MOTION_MOVE_BACK    = 6,                    // 倒车
    ADS_MOTION_END = VCA_ENUM_END
} ADS_MOTION_TYPE;

// 目标三维尺寸
typedef struct SRLM_DECORATOR _ADS_OBJ_SIZE_
{
    float  obj_width;
    float  obj_length;
    float  obj_height;
} ADS_OBJ_SIZE;

// 测速测距信息（相机坐标系）
typedef struct SRLM_DECORATOR _ADS_OBJ_MDM_
{
    ADS_MDM_TYPE        mdm_type;                   // 测距类型
    ADS_OBJ_SIZE        obj_size;                   // 目标实际尺寸（宽度、长度、高度 单位m）
    VCA_3D_POINT_F      rvf_pos;                    // 综合后的相对位置[m]
    VCA_POINT_F         rvf_vel;                    // 综合后的相对速度[m/s]（靠近>0 远离<0）
    VCA_3D_POINT_F      cam_pos;                    // 相机相对位置[m]
    VCA_POINT_F         cam_vel;                    // 相机相对速度[m/s]（靠近>0 远离<0）

    VCA_POINT_F         rel_acc;                    // 相对加速度[m/s^2]（速度梯度信息）
    ADS_MOTION_TYPE     motion_type;                // 目标运动类型
    int                 ttc;                        // 碰撞时间[ms] 0~255,000

    VCA_POINT_F         coordinate[4];              // 目标实际坐标[m]：纵向横向（左前-右前-右后-左后）
    short               coord_conf;                 // 当前目标视觉测距的置信度
	short               angle;                      // 目标车头朝向 -180~180度
	unsigned char       cut_in_out;                 // ==0未知 ==1在本车道  ==2不在本车道 ==3 CutIn ==4 Cutout（AVP模式复用为坐标系模式，RRV需要调用）
    unsigned char       obj_is_show;                // 是否输出目标（0-不显示  1-显示）
	unsigned char       reserved[2];                // 保留字节
} ADS_OBJ_MDM;    //56字节->60

// 目标3D姿态(pitch,yaw,roll)
typedef struct SRLM_DECORATOR _ADS_3D_POSE_
{
    int       num;
    HIK_APT_SRLM_FIELD_VEC(0, num, ADS_MAX_3D_POS_FEAT_DIM)
	float data[ADS_MAX_3D_POS_FEAT_DIM];
}ADS_3D_POSE;   // 84 BYTES

// 目标3D框信息
typedef struct SRLM_DECORATOR _ADS_3DBOX_INF_
{
    unsigned short      side_num;           // 有效面个数

    HIK_APT_SRLM_FIELD_VEC(0, side_num, 6)
    unsigned char       side_valid[6];      // 有效面标记，上,下,左,右,前,后

    HIK_APT_SRLM_FIELD_VEC(0, side_num, 6)
    VCA_POINT_S         side[6][4];         // 总共6个面，一个面有4个顶点
} ADS_3DBOX_INF;    // 56 BYTES






//目标基本信息
typedef struct SRLM_DECORATOR _ADS_OBJ_INFO_
{
    int                 valid;              // 1有效，0无效

    ADS_OBJ_DET         *det_inf;           // 检测信息
    ADS_OBJ_TRACK       *track_inf;         // 目标跟踪信息
    ADS_OBJ_ATTR        *attr_inf;          // 目标分类属性
    ADS_OBJ_FEAT        *feat_inf;          // 关键点/特征信息
    ADS_3D_POSE         *pose_inf;          // 目标3D姿态信息
    ADS_OBJ_MDM         *mdm_inf;           // 测距测速信息
    unsigned char        det_valid;         // 检测有效标志位
    unsigned char        track_valid;       // 跟踪有效标志位
    unsigned char        attr_valid;        // 分类属性有效标志位
    unsigned char        feat_valid;        // 关键点/特征有效标志位
    unsigned char        pose_valid;        // 目标3D姿态信息有效标志位
    unsigned char        mdm_valid;         // 测距测速信息有效标志位
    unsigned char        importance;        // 重要性排序队列(针对跟踪目标)1~n，MOT策略使用。前视通过CIPV车辆+小方位角目标排序,CIPV模块维护。
    unsigned char       reserved[9];       //预留16个字节
} ADS_OBJ_INFO;    //72字节



// 关联组合后的目标信息
typedef struct SRLM_DECORATOR _ADS_ASSO_OBJ_
{
    ADS_OBJ_INFO         *obj_inf;          // 目标主体信息
    int                  sub_num;           // 部件个数

    HIK_APT_SRLM_FIELD_VEC(0, sub_num, 4)
    ADS_OBJ_INFO         *sub_inf[4];       // 部件信息（同源或异源部件）
    int                  conf;              // 组件置信度[1~100]
}ADS_ASSO_OBJ;

/******************************************* 道路识别信息 *****************************************/
// 目标颜色
typedef enum SRLM_DECORATOR _ADS_RSA_COLOR_
{
    ADS_COLOR_INVALID = 0,               // 无效
    ADS_COLOR_WHITE   = 1,               // 白色
    ADS_COLOR_YELLOW  = 2,               // 黄色
    ADS_COLOR_END     = VCA_ENUM_END
} ADS_RSA_COLOR;

// 道路标线类型
typedef enum SRLM_DECORATOR _ADS_RSA_TYPE_
{
    ADS_RSA_TYPE_INVALID        = 0,    // 无效
    ADS_RSA_TYPE_LANE           = 1,    // 通用车道线
    ADS_RSA_TYPE_SOLID          = 2,    // 单实线
    ADS_RSA_TYPE_DASHEND        = 3,    // 单虚线
    ADS_RSA_TYPE_SOLID_DOU      = 4,    // 双实线
    ADS_RSA_TYPE_DASHEND_DOU    = 5,    // 双虚线
    ADS_RSA_TYPE_SOL_DAS        = 6,    // 实虚线
    ADS_RSA_TYPE_DAS_SOL        = 7,    // 虚实线
    ADS_RSA_TYPE_ROAD           = 8,    // 可行驶路面
    ADS_RSA_TYPE_ROADEDGE       = 9,    // 路沿
    ADS_RSA_TYPE_STOPLINE       = 10,   // 停止线
    ADS_RSA_TYPE_ZEBRA          = 11,   // 斑马线
    ADS_RSA_TYPE_GUIDELINE      = 12,   // 导流线
    ADS_RSA_TYPE_ROADMARK       = 13,   // 路面标志
    ADS_RSA_TYPE_VP_LEFT        = 14,   // 消隐线左半部分
    ADS_RSA_TYPE_VP_RIGHT       = 15,   // 消隐线右半部分

	ADS_RSA_TYPE_SIDEWALK       = 16,   //人行道
	ADS_RSA_TYPE_TRAFFICISLANDS = 17,   //安全岛
	ADS_RSA_TYPE_NONMOTORIZEDVEHICLELANE = 18,   //非机动车道
	ADS_RSA_TYPE_GREENBELT = 19,   //绿化带
	ADS_RSA_TYPE_FENCE     = 20,   //栏杆
	ADS_RSA_TYPE_ISOLATEWALL  = 21,      //隔离墙
	ADS_RSA_TYPE_BACKGROUND = 22,      //背景
	ADS_RSA_TYPE_CARBODY = 23,         //车身



    // 以下车位相关
    ADS_RSA_TYPE_PARK           = 100,	// 车位线
    ADS_RSA_TYPE_PARK_ROD       = 101,	// 档轮杆
    ADS_PSA_LINE_SLOT 		    = 102,	// 车位线
    ADS_PSA_ROAD_SIDE 			= 103,	// 路沿
    ADS_PSA_WHEEL_BLOCK 		= 104,	// 挡轮器
	ADS_PSA_OTHER               = 105,	// 其他类型
    ADS_RSA_TYPE_END            = VCA_ENUM_END
} ADS_RSA_TYPE;

// 扫描线
typedef struct SRLM_DECORATOR _ADS_RSA_SCANLINE_
{
    unsigned short start;
    unsigned short end;
    unsigned short pos;
    unsigned short id;
} ADS_RSA_SCANLINE;

typedef struct SRLM_DECORATOR _ADS_RSA_INST_
{
    ADS_RSA_TYPE     sem_label;           // 实例掩膜中的标签
    int              ins_label;           // 实例掩膜中的编号

    int              area;                // 实例的面积
    float            mean_prob;           // 实例的语义概率平均值
    float            mean_dist;           // 各个点到聚类中心的平均距离，衡量实例的聚类特性
    float            radius;              // 各个点到聚类中心的最大距离
    VCA_BOX_I        bbox;                // 实例的包围框

    short            run_length_num_x;                         // 游程编码个数
    short            run_length_num_y;                         // 游程编码个数

    HIK_APT_SRLM_FIELD_VEC(0, run_length_num_x, ADS_RSA_MAX_SCANLINE)
    ADS_RSA_SCANLINE aRunLength_x[ADS_RSA_MAX_SCANLINE];       // 垂直扫描线

    HIK_APT_SRLM_FIELD_VEC(0, run_length_num_y, ADS_RSA_MAX_SCANLINE)
    ADS_RSA_SCANLINE aRunLength_y[ADS_RSA_MAX_SCANLINE];       // 水平扫描线
}ADS_RSA_INST;

// 车道线信息
typedef struct SRLM_DECORATOR _ADS_RSA_LINE_FIT_
{
    int         npts;                                       // 车道线点数
    HIK_APT_SRLM_FIELD_VEC(0, npts, ADS_RSA_MAX_SCANLINE)
    VCA_POINT_F pts[ADS_RSA_MAX_SCANLINE];                  // 车道线点信息

    float       ynear;                                      // 近点
    float       yfar;                                       // 远点
    float       curve_param[ADS_RSA_CURVE_PARAM_NUM];       // 直接拟合的曲线方程
    float       regular_param[ADS_RSA_CURVE_PARAM_NUM];     // 正则后方程
    float       lmbda;                                      // 正则系数
    float       straight_param[3];                          // 直线参数 ax + by + c = 0
} ADS_RSA_LINE_FIT;

// 车道线参数信息
typedef struct SRLM_DECORATOR _ADS_RSA_LINE_PARAM_
{
    float       ynear;                                      // 近点
    float       yfar;                                       // 远点
    float       regular_param[ADS_RSA_CURVE_PARAM_NUM];     // 三次曲线方程
    float       straight_param[3];                          // 直线参数
} ADS_RSA_LINE_PARAM;

typedef struct SRLM_DECORATOR _ADS_RSA_LINE_SHOW_
{
    int         id;                                         // 车道线id
    
    int         npts_uv;                                       // 图像车道线点数
    HIK_APT_SRLM_FIELD_VEC(0, npts_uv, ADS_RSA_MAX_SCANLINE)
    VCA_POINT_F pts_uv[ADS_RSA_MAX_SCANLINE];                  // 图像车道线点信息

    int         npts_xy;                                       // 路面车道线点数
    HIK_APT_SRLM_FIELD_VEC(0, npts_xy, ADS_RSA_MAX_SCANLINE)
    VCA_POINT_F pts_xy[ADS_RSA_MAX_SCANLINE];                  // 路面车道线点信息
}ADS_RSA_LINE_SHOW;

// 车道线来源
typedef enum SRLM_DECORATOR _ADS_RSA_LINE_SRC_
{
    ADS_RSA_LINE_SRC_DETECT  = 1,               // 实际检测
    ADS_RSA_LINE_SRC_MODEL   = 2,               // 模型推算
    ADS_RSA_LINE_SRC_HISTORY = 3,               // 历史预测
    ADS_RSA_LINE_SRC_EGO     = 4,               // 自运动估计
    ADS_RSA_LINE_SRC_END     = VCA_ENUM_END
} ADS_RSA_LINE_SRC;

// 车道结构信息
typedef enum SRLM_DECORATOR _ADS_RSA_LINE_POS_
{
    ADS_RSA_LINE_POS_OTHER_LEFT  = 0x00,         // 其他左侧车道线
    ADS_RSA_LINE_POS_LEFT_LEFT   = 0x01,         // 左侧车道的左侧车道线
    ADS_RSA_LINE_POS_LEFT        = 0x02,         // 当前车道的左侧车道线

    ADS_RSA_LINE_POS_RIGHT       = 0x10,         // 当前车道的右侧车道线
    ADS_RSA_LINE_POS_RIGHT_RIGHT = 0x11,        // 右侧车道的右侧车道线
    ADS_RSA_LINE_POS_OTHER_RIGHT = 0x12,        // 其他右侧车道线

    ADS_RSA_LINE_POS_TOP         = 0x13,        // 车位顶部
    ADS_RSA_LINE_POS_BOTTOM      = 0x14,        // 车位底部
    ADS_RSA_LINE_POS_UNKNOWN     = 0x20,


    ADS_PSA_TOP = 0x30,              //  顶边
    ADS_PSA_LEFT = 0x31,             // 左侧边
    ADS_PSA_BOTTOM = 0x32,           // 底边
    ADS_PSA_RIGHT = 0x33,            // 右侧边
    ADS_PSA_OTHERS = 0x34,           // 其它

    ADS_RSA_LINE_POS_END         = VCA_ENUM_END
}ADS_RSA_LINE_POS;

// 导流线属性结构体
typedef struct SRLM_DECORATOR _ADS_RSA_GUD_INFO_
{
    int                 valid;
    int                 line_id;
    int                 match_id;
    int                 cross_valid;        // 状态为1时，交叉点和车辆汇入状态有效
    int                 is_merge;           // 状态为1时，导流线为车辆汇入场景，为0时，为车辆流出场景
    int                 area;               // 导流线像素总量
    VCA_POINT_F         cross_xy;           // 交叉点在车体坐标系下坐标
    VCA_POINT_I         cross_uv;           // 交叉点在图像坐标系下坐标
} ADS_RSA_GUD_INFO;

typedef struct SRLM_DECORATOR _ADS_RSA_LINE_ATTR_
{
    int                 conf;                    // 置信度[1~100]
    ADS_RSA_COLOR       color;                   // 颜色
    ADS_RSA_TYPE        type;                    // 线类型
    ADS_RSA_LINE_POS    current;                 // 线的位置信息,当前车道，左线，右线
    float               curvature;               // 曲率
    float               width;                   // 路面坐标系下车道线的宽度
    int                 sheltered;               // 车道线被车辆等目标遮挡
    int                 lane_index;              // 单帧车道线索引（只对当前帧有效）
    unsigned char       fit_conf;                // 近端拟合贴合程度[1~100]
    unsigned char       reserved[7];             // 保留8字节
}ADS_RSA_LINE_ATTR;

// 车道线数据
typedef struct SRLM_DECORATOR _ADS_RSA_LINE_
{
    ADS_RSA_LINE_ATTR   attr;                   // 车道线属性
    ADS_RSA_LINE_SRC    state;                  // 跟踪状态
    ADS_RSA_LINE_PARAM  line_uv;                // 车道线（图像坐标系）
    ADS_RSA_LINE_PARAM  line_xy;                // 车道线（路面坐标系）
    int                 id;                     // id号
}ADS_RSA_LINE;

//车道位置信息
typedef enum SRLM_DECORATOR _ADS_RSA_LANE_POS_
{
	ADS_RSA_LANE_POS_LEFT_LEFT = 0,    // 左左车道
	ADS_RSA_LANE_POS_LEFT = 1,         // 左车道
	ADS_RSA_LANE_POS_CUR = 2,          // 主车道
	ADS_RSA_LANE_POS_RIGHT = 3,        // 右车道
	ADS_RSA_LANE_POS_RIGHT_RIGHT = 4,  // 右右车道
	ADS_RSA_LANE_POS_END = VCA_ENUM_END
} ADS_RSA_LANE_POS;

// 车道信息
typedef struct SRLM_DECORATOR _ADS_RSA_LANE_
{
	ADS_RSA_LANE_POS  lane_type;    // 车道类型，0:左左，1:左，2:主，3:右，4:右右
	ADS_RSA_LINE     *left_line;   // 车道左车道线
	ADS_RSA_LINE     *right_line;  // 车道右车道线
	float             lane_width;    // 车道宽度
} ADS_RSA_LANE;

typedef struct SRLM_DECORATOR _ADS_RSA_LANE_PROPERPTY_
{
    float               current_width;          // 当前车道宽度
    float               rotate;                 // 车道偏转角度
    int                 rainy;                  // 晴雨状态
}ADS_RSA_LANE_PROPERPTY;

// 可行驶区域信息
typedef struct SRLM_DECORATOR _ADS_FREESPACE_POS_
{
	float             x;                  // 归一化的x坐标（原图像坐标系下）
	float             y_start;            // 归一化的y起始坐标（原图像坐标系下）
	float             y_end;              // 归一化的y终止坐标（原图像坐标系下）
}ADS_FREESPACE_POS;

//斑马线结果
typedef struct SRLM_DECORATOR _ADS_RSA_ZEBRA_
{
    int           id;                   //斑马线id
    VCA_POLYGON_F zebra_on_image;       //斑马线多边形框（图像坐标系）
    VCA_POLYGON_F zebra_on_road;        //斑马线多边形框（路面坐标系）
}ADS_RSA_ZEBRA;


// 自车运动信息来源
typedef enum SRLM_DECORATOR _ADS_MOTION_SOURCE_
{
    ADS_MOTION_CAN        = 0,                  // CAN车速
    ADS_MOTION_GPS        = 1,                  // GPS车速
    ADS_MOTION_EME        = 2,                  // 自运动估计车速
    ADS_MOTION_SOURCE_END = VCA_ENUM_END,
} ADS_MOTION_SOURCE;

// 车辆运动信息
typedef struct SRLM_DECORATOR _ADS_MOTION_INF_
{
    ADS_MOTION_TYPE     motion_state;           // 运动状态
    int                 state_conf;             // 运动状态置信度[1-100]
    int                 bumpy_conf;             // 颠簸置信度

    float               trans;                  // 平移速度(km/h)
    float               theta;                  // 旋转速度(弧度/s,左正右负)
    ADS_MOTION_SOURCE   trans_source;           // 运动速度来源（CAN或EME）
    ADS_MOTION_SOURCE   theta_source;           // 运动角度来源（CAN或EME）
    int                 eme_conf;               // 自运动估计速度置信度
	VCA_POINT_F         trans_integral;         // 本车平移量(单位m: 本帧位置较上一帧位置变化/车头方向移动为正)(x:横向左正;y:纵向前为正)  
	float               theta_integral;         // 本车旋转角度(弧度,左正右负)
	unsigned char       valid_intergral;        // 本车移动有效标志位
	unsigned char       reserved[3];            // 保留字节

    unsigned int        time_stamp; 
}ADS_MOTION_INF; // 32 BYTES ->44 BYTES ->48

// 消隐点信息
typedef struct SRLM_DECORATOR _ADS_VANISH_INF_
{
    VCA_POINT_F         pt;                     // 消隐点坐标
    int                 pt_conf;                // 消隐点坐标计算置信度[1-100]
    float               move_dis;               // 消隐线纵向移动量
    int                 move_dis_conf;          // 消隐线移动量置信度[1-100]
}ADS_VANISH_INF;

// 自标定类型
typedef enum SRLM_DECORATOR _ADS_AUTO_CAL_TYPE_
{
	ADS_AUTO_CAL_TYPE_INVALID = 0,       // 无标定
	ADS_AUTO_CAL_TYPE_DYNAMICS = 1,      // 动态标定
	ADS_AUTO_CAL_TYPE_STATIC = 2,        // 静态标定
	ADS_AUTO_CAL_TYPE_END = VCA_ENUM_END
} ADS_AUTO_CAL_TYPE;

// 相机参数自标定返回枚举值
typedef enum SRLM_DECORATOR _ADS_AUTO_CALIB_OUTPUT_
{
    ADS_AUTO_CALIB_OFF     = 0,                   // 自标定未开启
    ADS_AUTO_CALIB_PROCESS = 1,                   // 自标定进行中，进度可以参考calib_procedure

    ADS_AUTO_CALIB_OK = 2,                    // 自标定成功
	ADS_AUTO_CALIB_LAN_NONE = 3,              // 当前车道未检测到车道线
	ADS_AUTO_CALIB_LAN_SHORT = 4,             // 车道线过短、相交
	ADS_AUTO_CALIB_LAN_NOT_STRAIGHT = 5,      // 车道线不够直
	ADS_AUTO_CALIB_LOW_CONF = 6,              // 置信度较低
	ADS_AUTO_CALIB_INSTALL_ERR_HIGH = 7,      // 消隐点位置偏高（相机俯仰角偏低）
	ADS_AUTO_CALIB_INSTALL_ERR_LOW = 8,       // 消隐点位置偏低（相机俯仰角偏高）
	ADS_AUTO_CALIB_INSTALL_ERR_LEFT = 9,      // 消隐点位置偏左（相机偏转角偏右）
	ADS_AUTO_CALIB_INSTALL_ERR_RIGHT = 10,    // 消隐点位置偏右（相机偏转角偏左）
	ADS_AUTO_CALIB_PARAM_INVALID = 11,        // 相机参数异常

	// 以下为动态标定相关
	ADS_AUTO_CALIB_STEERING = 12,             // 未直线行驶
	ADS_AUTO_CALIB_SPEED_LOW = 13,            // 车速过低
	ADS_AUTO_CALIB_END = VCA_ENUM_END,
}ADS_AUTO_CALIB_OUTPUT;

// 自标定信息
typedef struct SRLM_DECORATOR _ADS_CALIB_INFO_
{
    ADS_AUTO_CALIB_OUTPUT  calib_out;               // 自标定结果输出
    int                    calib_procedure;         // 标定进度（0-100）

    VCA_POINT_F            pt_near;                 // 自标定消隐点位置
	float                  calib_pitch_angle;       // 自标定俯仰角
	float                  calib_yaw_angle;         // 自标定偏转角
}ADS_CALIB_INFO;
/******************************************* 行为识别信息 *****************************************/
// 重要目标选择信息
typedef struct SRLM_DECORATOR _ADS_CIPX_INF_
{
    int                 id;                     // 重要目标ID信息1~
    unsigned char       conf;                   // 目标重要性判断置信度[1,100]
    unsigned char       cut_in_out;             // 目标切入/切出状态： ==0 本车道行驶 ==1 切入本车道 ==2 切出本车道
    unsigned char       locate2lane;            // 目标相对本车道关系[-2(左2以外) -1(左1) 0（当前车道内） 1(右1) 2(右2以外)]
    unsigned char       reserved[5];            // 保留字节
} ADS_CIPX_INF;

// 行为模式
typedef enum SRLM_DECORATOR _ADS_BEHAVIOR_TYPE_
{
    ADS_BEH_INVALID     = 0x0000,               // 无效
    ADS_BEH_FCD         = 0x0010,               // 前车检测
    ADS_BEH_FCL         = 0x0011,               // 前车起步
    ADS_BEH_FCW         = 0x0020,               // 前车碰撞预警
    ADS_BEH_UFCW        = 0x0021,               // 城市前车碰撞预警
    ADS_BEH_PCD         = 0x0030,               // 行人检测
    ADS_BEH_PCW         = 0x0040,               // 行人碰撞预警
    ADS_BEH_PSCW        = 0x0041,               // 礼让行人报警
    ADS_BEH_PSCW_N      = 0x0042,               // 未礼让行人报警
    ADS_BEH_PSCW_N_PRE  = 0x0043,               // 未礼让行人报警-预抓拍
    ADS_BEH_BSD_L       = 0x0050,               // 超车盲区预警(左侧)
    ADS_BEH_BSD_R       = 0x0051,               // 超车盲区预警(右侧)
	ADS_BEH_BSD_F       = 0x0052,               // 盲区预警(前方)
	ADS_BEH_BSD_B       = 0x0053,               // 盲区预警(后方)
    ADS_BEH_BSD         = 0x0054,               // 盲区预警(四周)
	ADS_BEH_BSD_NEAR    = 0x0055,               // 盲区预警(四周近处)
	ADS_BEH_BSD_FAR     = 0x0056,               // 盲区预警(四周远处)
	ADS_BEH_BSD_L_1M = 0x0057,                  // 左视后置平视高度1米
	ADS_BEH_BSD_L_2M = 0x0058,                  // 左视后置平视高度2米
	ADS_BEH_BSD_L_3M = 0x0059,                  // 左视后置平视高度3米
	ADS_BEH_BSD_R1M = 0x0060,                   // 右视后置平视高度1米
	ADS_BEH_BSD_R2M = 0x0061,                   // 右视后置平视高度2米
	ADS_BEH_BSD_R3M = 0x0062,                   // 右视后置平视高度3米
	ADS_BEH_BSD_L_OVER_LOOK = 0x0063,           // 左视前置俯视
	ADS_BEH_BSD_L_POST_OVER_LOOK = 0x0064,      // 左视后置俯视
	ADS_BEH_BSD_R_OVER_LOOK = 0x0065,           // 右视前置俯视
	ADS_BEH_BSD_R_POST_OVER_LOOK = 0x0066,      // 右视后置俯视
	ADS_BEH_BSD_F_MID = 0x0067,                 // 前视居中安装
    ADS_BEH_BSD_L_ALARM = 0x0068,               // 主动超车盲区预警(左侧)
    ADS_BEH_BSD_R_ALARM = 0x0069,               // 主动超车盲区预警(右侧)

    ADS_BEH_TCSS_UNKNOWN_BEDDING = 0x0080,       // 车厢密闭状态位置
    ADS_BEH_TCSS_NO_BEDDING = 0x0081,           // 车厢未密闭
    ADS_BEH_TCSS_HALF_BEDDING = 0x0082,         // 车厢半密闭
    ADS_BEH_TCSS_FULL_BEDDING = 0x0083,         // 车厢全密闭
    ADS_BEH_TCSS_UNKNOWN_LOAD = 0x0084,         // 车厢载货状态位置
    ADS_BEH_TCSS_NO_LOAD = 0x0085,              // 车厢空载
    ADS_BEH_TCSS_HALF_LOAD = 0x0086,            // 车厢半载
    ADS_BEH_TCSS_FULL_LOAD = 0x0087,            // 车厢满载

    ADS_BEH_LDD         = 0x0100,               // 车道检测
    ADS_BEH_LDW_L       = 0x0110,               // 车道左偏离预警
    ADS_BEH_LDW_R       = 0x0111,               // 车道右偏离预警
    ADS_BEH_LDW_L_CROSS = 0x0112,               // 车道左越线报警
    ADS_BEH_LDW_R_CROSS = 0x0113,               // 车道右越线报警

    ADS_BEH_FDW_SUNGLASS= 0x0210,               // 红外阻断型墨镜预警
    ADS_BEH_FDW_EYE     = 0x0211,               // 闭眼疲劳预警
    ADS_BEH_FDW_YAWN    = 0x0212,               // 打哈欠疲劳预警
	ADS_BEH_FDW_EYE_LIT = 0x0213,               // 轻度闭眼疲劳预警
	ADS_BEH_FDW_NO_MASK = 0x0214,               // 未戴口罩报警
	ADS_BEH_FDW_XU      = 0x0215,               // 嘘动作行为
	
    ADS_BEH_NLF_L       = 0x0220,               // 左不目视前方预警
    ADS_BEH_NLF_R       = 0x0221,               // 右不目视前方预警
    ADS_BEH_NLF_U       = 0x0222,               // 上不目视前方预警
    ADS_BEH_NLF_D       = 0x0223,               // 下不目视前方预警
    ADS_BEH_NLF_LD      = 0x0224,               // 左下不目视前方预警
    ADS_BEH_NLF_RD      = 0x0225,               // 右下不目视前方预警
    ADS_BEH_UPW         = 0x0230,               // 打电话预警
    ADS_BEH_SMW         = 0x0240,               // 抽烟预警
    ADS_BEH_PUW         = 0x0250,               // 捡拾物品预警
    ADS_BEH_BELT        = 0x0260,               // 安全带预警
    ADS_BEH_CLOTH       = 0x0270,               // 工作服预警
    ADS_BEH_TALK        = 0x0280,               // 谈话注意力分散预警
    ADS_BEH_ABNORMAL    = 0x0290,               // 驾驶员异常预警
    ADS_BEH_COVERCC     = 0x0291,               // 遮挡摄像头预警
	ADS_BEH_ONE_HAND_ON = 0x02A0,               // 单手脱离方向盘
	ADS_BEH_NO_HAND_ON  = 0x02A1,               // 双手脱离方向盘
	ADS_BEH_WHDUPD      = 0x02A2,               // 头顶路玩手机
	ADS_BEH_WHDUPD_DEMO = 0x02A3,               // 头顶路玩手机(过检灵敏度)
    ADS_BEH_HGR_REJECT  = 0x02B0,               // 静态拒绝手势
    ADS_BEH_HGR_OK      = 0x02B1,               // 静态OK手势
    ADS_BEH_HGR_THUMB   = 0x02B2,               // 静态点赞手势
    ADS_BEH_HGR_ROCK    = 0x02B3,               // 静态ROCK手势
	ADS_BEH_HGR_HEART   = 0x02B4,               // 静态比心手势
	ADS_BEH_HGR_YEAH    = 0x02B5,               // 静态比耶手势
	ADS_BEH_HGR_FIST    = 0x02B6,               // 静态拳头手势
	ADS_BEH_HGR_INDEX   = 0x02B7,               // 静态食指手势
	ADS_BEH_HGR_BG      = 0x02B8,               // 静态背景手势

    ADS_BEH_PALM_TO_LEFT    = 0x02C0,           // 手掌左滑
    ADS_BEH_PALM_TO_RIGHT   = 0x02C1,           // 手掌右滑
    ADS_BEH_PALM_TO_BYE     = 0x02C2,           // 手掌拜拜
    ADS_BEH_INDEX_TO_CWISE  = 0x02C3,           // 食指顺时针画圈
    ADS_BEH_INDEX_TO_ACWISE = 0x02C4,           // 食指逆时针画圈
    ADS_BEH_PALM_TO_UP      = 0x02C5,           // 手掌上滑
    ADS_BEH_PALM_TO_DOWN    = 0x02C6,           // 手掌下滑

    //ADS_BEH_HPR_KPT         = 0x02D0,           // 手势关键点
	
	ADS_BEH_WHDBELT     = 0x02A8,               // 头顶路未系安全带
	// 0x02E0-0x02Fx 为DBA向外输出的架设警告码
	ADS_ABN_HEAD_NON_LEAVE        = 0x02E0,     // 无人脸输出---离岗
	ADS_ABN_HEAD_NON_BIAS         = 0x02E1,     // 无人脸输出---镜头偏离
	ADS_ABN_HEAD_TOO_BIG          = 0x02E2,     // 人脸过大，宽度大于450，
	ADS_ABN_HEAD_TOO_SMALL        = 0x02E3,     // 人脸过小，宽度小于150
	ADS_ABN_HEAD_TOO_BRIGHT       = 0x02E4,     // 人脸过暗，亮度大于220
	ADS_ABN_HEAD_TOO_DARK         = 0x02E5,     // 人脸过亮，亮度小于60
	ADS_ABN_HEAD_INVALID_POSITION = 0x02E6,     // 人脸位置太靠左、右、下
	ADS_ABN_WHEEL_NON             = 0x02F0,     // 未检测到方向盘
	ADS_ABN_WHEEL_HEAD_NON        = 0x02F1,     // 方向盘路未检测到头部
	ADS_ABN_WHEEL_INVALID_POSITION = 0x02F2,    // 方向盘架设不规范


    ADS_BEH_TSR         = 0x0300,               // 交通标志牌提醒
    ADS_BEH_ROADMARK    = 0x0310,               // 路面标志提醒
    ADS_BEH_RM_ZEB_SPD  = 0x0312,               // 路口（斑马线）超速
    ADS_BEH_LIGHT       = 0x0320,               // 信号灯提醒
    ADS_BEH_CROSS       = 0x0330,               // 路口提醒

    ADS_BEH_ROAD_SEG      = 0x0400,             // 相机架设异常提醒(路面分割占比偏低)
    ADS_BEH_VANISH_OFFSET = 0x0401,             // 相机架设异常提醒(消隐线异常)
    ADS_BEH_VANISH_LOW    = 0x0402,             // 相机架设异常提醒(初始消隐线过低)
    ADS_BEH_VANISH_HIGH   = 0x0403,             // 相机架设异常提醒(初始消隐线过高)
	ADS_BEH_CARBODY_OFFSET = 0x0404,            // 相机架设异常提醒(车身占比异常)
    ADS_BEH_ICW            = 0x0410,            // ADAS镜头遮挡预警

    ADS_BEH_END         = VCA_ENUM_END
} ADS_BEHAVIOR_TYPE;

// 报警信息输出
typedef struct SRLM_DECORATOR _ADS_BEHAVIOR_INFO_
{
    ADS_BEHAVIOR_TYPE   type;                   // 行为类型
    unsigned short      level;                  // 行为等级，值域[1,100]
    unsigned short      conf;                   // 报警置信度，值域[1,100]
	int                 obj_valid;              // 报警时是否存在有效目标，1为有效，0为无效
	VCA_RECT_F          rect;                   // 目标图像位置（非图像目标该值无效）
    int                 obj_id;                 // 目标Id，与目标输出队列匹配
    ADS_OBJ_MDM         *mdm_inf;               // 位置速度信息
    int                 attr_type;              // 报警目标的属性
	int                 sensor_id;              // 鱼眼图的id
	ADS_OBJ_TYPE        obj_type;               // 鱼眼图目标类型
    unsigned char       reserved[28];           // 预留
}ADS_BEHAVIOR_INFO;     //72字节

// 信号灯光源类型
typedef enum SRLM_DECORATOR _VCA_ADAS_LIGHT_TYPE_
{
    VCA_LIGHT_INVALID = 0x00,

    VCA_LIGHT_RED_LEFT = 0x01,  // 红圆
    VCA_LIGHT_GREEN_LEFT = 0x02,  // 红左
    VCA_LIGHT_RED_STRIDE = 0x03,  // 红直
    VCA_LIGHT_GREEN_STRIDE = 0x04,  // 红右
    VCA_LIGHT_RED_RIGHT = 0x05,  // 红左&调头

    VCA_LIGHT_GREEN_RIGHT = 0x06,  // 绿圆
    VCA_LIGHT_RED_ROUND = 0x07,  // 绿左
    VCA_LIGHT_GREEN_ROUND = 0x08,  // 绿直
    VCA_LIGHT_RED_LEFT_TURN = 0x0B,  // 绿右
    VCA_LIGHT_GREEN_LEFT_TURN = 0x0C,  // 绿左&调头

    VCA_LIGHT_YELLOW_LEFT = 0x0D,  // 黄左
    VCA_LIGHT_YELLOW_STRIDE = 0x0E,  // 黄直
    VCA_LIGHT_YELLOW_RIGHT = 0x0F,  // 黄右
    VCA_LIGHT_YELLOW_ROUND = 0x10,  // 黄圆
    VCA_LIGHT_YELLOW_LEFT_TURN = 0x11,  // 黄左&调头

    VCA_ADAS_LIGHT_END = VCA_ENUM_END
} VCA_ADAS_LIGHT_TYPE;

// FCW报警参数
typedef struct SRLM_DECORATOR _ADS_FCW_PARAM_
{
    int     fcw_flag;    // FCW报警标志位
    int     ufcw_flag;   // UFCW报警标志位
    int     fcw_sens;    // FCW灵敏度
    int     ufcw_sens;   // UFCW灵敏度
    int     fcw_th1;     // 前车静止  
    int     fcw_th2;     // 前车速度固定
    int     fcw_th3;     // 前车速度降低
    int     ufcw_th;
} ADS_FCW_PARAM;

// PCW报警参数
typedef struct SRLM_DECORATOR _ADS_PCW_PARAM_
{
    int     pcw_sens;  // PCW置信度
    int     pcw_th1;   // 行人静止
    int     pcw_th2;   // 机动车与行人同向运动
    int     pcw_th3;   // 行人横向运动
    int     pcw_th4;   // 非机动车
    int     pcw_th5;   // 备用灵敏度
} ADS_PCW_PARAM;

// PSCW报警参数
typedef struct _ADS_PSCW_PARAM_
{
    int           pscw_n_flag;              //未礼让行人预警标志位                 0关闭 1开启 
    int           inter_speed_flag;         //路口超速报警标志位                   0关闭 1开启 
    int           inter_warn_flag;          //路口斑马线提醒标志位                 0关闭 1开启 
    unsigned int  pscw_n_speed_thr;         //未礼让行人速度阈值                   单位0.01km/h
    unsigned int  inter_speed_thr;          //路口超速速度阈值                     单位0.01km/h
    int           inter_warn_dis_thr;       //路口斑马线提醒距离阈值               单位 米
    int           no_zebra_cnt_thr;         //bsdf连续启动无斑马线复位计数阈值     (范围1~20）
    int           pscw_n_sens;              //未礼让行人灵敏度                     [0-100,默认配置50]
    int           inter_speed_sens;         //路口超速灵敏度                       [0-100,默认配置50]
    int           inter_warn_sens;          //路口斑马线提醒灵敏度                 [0-100,默认配置50]
}ADS_PSCW_PARAM;

// DBA报警事件属性
typedef enum _ADS_DBA_ATTR_TYPE_
{
	ADS_DBA_ATTR_UNKNOWN       = 0,           //未知  0
	ADS_DBA_ATTR_SMK_UNLIGHT   = 0x64,        //抽烟未点燃
	ADS_DBA_ATTR_SMK_LIGHT     = 0x65,        //抽烟点燃
	ADS_DBA_ATTR_END           = VCA_ENUM_END
}ADS_DBA_ATTR_TYPE;

#endif
