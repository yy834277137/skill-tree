/**
 * 
 * 版权信息: Copyright (c) 2013, 杭州海康威视数字技术股份有限公司
 * 
 * 文件名称: IS_privt_define.h
 * 文件标识: HIKVISION_IVS_SYS_LIB
 * 摘    要: 海康威视智能信息扩展信息定义头文件
 *
 * 当前版本: 4.2.1
 * 作    者: 余翔
 * 日    期: 2019.8.13
 * 备    注:
 *          新建，IVS、ITS库通用扩展信息结构体
 **/

#ifndef _IS_PRIVT_DEFINE_H_
#define _IS_PRIVT_DEFINE_H_

#define IS_MAX_PRIVT_LEN    32              // 最大私有数据长度
#define IVS_TYPE_TARGE      1               // IVS目标类型
#define IVS_TYPE_RULE       3               // IVS规则类型

//IVS用户扩展信息类型
typedef enum _IS_PRIVT_INFO_
{
    IS_PRIVT_NONE        = 0,              // 无扩展信息
    IS_PRIVT_COLOR       = 1,              // 扩展颜色信息(IS_PRIVT_INFO_COLOR)
    IS_PRIVT_COLOR_LINE  = 2,              // 颜色轨迹信息(IS_PRIVT_INFO_COLORL)
    IS_PRIVY_SPEED_COORD = 3,              // 速度坐标信息(IS_PRIVT_INFO_SPECOORD)
    IS_PRIVT_COORD_S     = 4,              // 经纬度信息（IS_PRIVT_INFO_COORDS）
    IS_PRIVT_EAGLE_R     = 5,              // 鹰眼跟踪红色四角框(无信息)
    IS_PRIVT_COLOR_RANG  = 6,              // 色块填充(IS_PRIVT_INFO_COLOR)
    IS_PRIVT_HELMET      = 7,              // 安全帽信息(IS_PRIVT_INFO_HELMET)
    IS_PRIVT_COLOR_SPEED = 8,              // 颜色车速信息(IS_PRIVT_INFO_COLSPEED)
    IS_PRIVT_COLOR_TEMP  = 9,              // 颜色温度结构体（IS_PRIVT_INFO_TEMP）
    IS_PRIVT_COLOR_CONTRABAND = 10,             // 颜色违禁品结构体（IS_PRIVT_INFO_TEMP） 
} IS_PRIVT_INFO_TYPE;

// 颜色结构体
typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
}IS_PRIVT_INFO_COLOR;

// 颜色轨迹信息结构体
typedef struct
{
    IS_PRIVT_INFO_COLOR  color;
    unsigned char        line_time;      // 轨迹持续时间
    unsigned char        target_type;    // 车辆/人员类型
}IS_PRIVT_INFO_COLORL;

// 经纬度信息（双字节）
typedef struct
{
    unsigned short x;
    unsigned short y;
}IS_PRIVT_INFO_COORDS;


// 车速坐标信息结构体
typedef struct
{
    unsigned char x;
    unsigned char y;
    unsigned char speed;            // 速度
    unsigned char id;               // id
    unsigned char flag;             // 显示开关标志（0x04显示ID，0x02显示速度，0x01显示坐标）
    unsigned char reseverd[3];      // 保留位
}IS_PRIVT_INFO_SPECOORD;

//安全帽信息结构体
typedef struct
{
    IS_PRIVT_INFO_COLOR  color;
    unsigned char        helmet_state;      // 安全帽检测状态
    unsigned char        reseverd[3];       // 保留字段
}IS_PRIVT_INFO_HELMET;


// 颜色车速信息
typedef struct
{
    IS_PRIVT_INFO_COLOR color;              // 颜色
    unsigned char       speed;              // 速度
    unsigned char       reseverd[3];        // 保留字段
}IS_PRIVT_INFO_COLSPEED;

// 颜色温度结构体
typedef struct
{
    IS_PRIVT_INFO_COLOR color;              // 颜色
    unsigned char       temp_unit;          // 温度单位;0-摄氏度;1-华氏度;2开尔文
    unsigned char       font_size;          // 字号大小 8、12、16、20、24
    unsigned char       enable;             // 使能
    unsigned char       reseverd;           // 保留字段，字节对齐
    unsigned short      x;                  // X轴坐标,归一化到1000
    unsigned short      y;                  // Y周坐标,归一化到1000
    float               temp;               // 温度: [-40.0, 1000.0]
}IS_PRIVT_INFO_TEMP;


// 扩展信息结构体
typedef struct
{
    unsigned char        privt_type;                     // 用户扩展类型:详见IVS_PRIVT_INFO
    unsigned char        reseverd[6];                    // 目标类型扩展
    unsigned char        privt_len;                      // 用户扩展信息长度
    unsigned char        privt_data[IS_MAX_PRIVT_LEN];   // 用户扩展信息
}IS_PRIVT_INFO;


// 违禁物颜色结构体
typedef struct
{
    IS_PRIVT_INFO_COLOR color;                  // 颜色
    unsigned char       confidence;             // 违禁品置信度
    unsigned char       pos_len;                // 违禁品名称长度
    unsigned char       pos_data[22];           // 违禁品名称
    unsigned int        type;                   // 违禁品类型
}IS_PRIVT_INFO_CONTRABAND;


#endif
