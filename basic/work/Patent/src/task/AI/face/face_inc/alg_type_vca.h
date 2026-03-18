/** @file       alg_type_vca.h
 *  @note       HangZhou Hikvision Digital Technology Co., Ltd.
 *              All right reserved
 *  @brief      算子层vca相关头文件
 *  @note       
 *  
 *  @author     guixinzhe
 *  @version    v2.0.0
 *  @date       2020/07/20
 *  @note       SAE所需的VCA相关内容整理
 */

#ifndef __ALG_TYPE_VCA__
#define __ALG_TYPE_VCA__

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
#define SAE_MAX_RULE_NUM                (8)             // 最多支持八条规则
#define SAE_VCA_MAX_VERTEX_NUM          (10)            // 多边形最大顶点数
#define SAE_VCA_ENUM_END                (0xFFFFFF)      // 定义枚举结束值，用于对齐
#define SAE_VCA_ENUM_END_V2             (0xFFFFFFFF)    // 定义枚举结束值，用于对齐
#define SAE_VCA_IMG_MAX_CHANEL          (5)             // 单帧图像支持的最大通道数

#define SAE_VCA_SIZE_ALIGN(size, align) (((size) + ((align)-1)) & ((unsigned int)(-(align))))
#define SAE_VCA_SIZE_ALIGN_8(size)      SAE_VCA_SIZE_ALIGN(size, 8)
#define SAE_VCA_SIZE_ALIGN_16(size)     SAE_VCA_SIZE_ALIGN(size, 16)
#define SAE_VCA_SIZE_ALIGN_32(size)     SAE_VCA_SIZE_ALIGN(size, 32)
#define SAE_VCA_SIZE_ALIGN_64(size)     SAE_VCA_SIZE_ALIGN(size, 64)
#define SAE_VCA_SIZE_ALIGN_128(size)    SAE_VCA_SIZE_ALIGN(size, 128)
/**************************************************************************************************
* 常用数学宏定义
**************************************************************************************************/
#define SAE_VCA_EPS              (0.0000001f)
#define SAE_VCA_PI               (3.1415926f)
#define SAE_VCA_ABS(a)           (((a) > 0) ? (a) : -(a))
#define SAE_VCA_ROUND(a)         (int)((a) + (((a) >= 0) ? 0.5 : -0.5))

//大值，小值
#define SAE_VCA_MAX(a, b)        (((a) > (b)) ? (a) : (b))
#define SAE_VCA_MIN(a, b)        (((a) < (b)) ? (a) : (b))
/***********************************************************************************************************************
* 基础数据结构体
***********************************************************************************************************************/
//矩形(浮点型)
typedef struct _SAE_VCA_RECT_F_
{
    float x;         //矩形左上角X轴坐标
    float y;         //矩形左上角Y轴坐标
    float width;     //矩形宽度
    float height;    //矩形高度
}SAE_VCA_RECT_F;

//点(浮点型) 
typedef struct _SAE_VCA_POINT_F_
{
    float x;
    float y;
}SAE_VCA_POINT_F;

//点(整型) 
typedef struct _SAE_VCA_POINT_I_
{
    int x;
    int y;
}SAE_VCA_POINT_I;

//多边形(浮点型)
typedef struct _SAE_VCA_POLYGON_F_
{
    unsigned int    vertex_num;                    //顶点数
    SAE_VCA_POINT_F point[SAE_VCA_MAX_VERTEX_NUM]; //顶点
} SAE_VCA_POLYGON_F;

//包围盒(整型)
typedef struct _SAE_VCA_BOX_I_
{
    int   left;      //左边界
    int   top;       //上边界
    int   right;     //右边界
    int   bottom;    //下边界
}SAE_VCA_BOX_I;

//矢量(浮点型)
typedef struct _SAE_VCA_VECTOR_F_
{
    SAE_VCA_POINT_F   start_point;   //起点
    SAE_VCA_POINT_F   end_point;     //终点
}SAE_VCA_VECTOR_F;

//尺寸类型
typedef enum _SAE_VCA_SIZE_MODE_
{
    SAE_IMAGE_PIX_MODE,      //根据像素大小设置，单位pixel
    SAE_REAL_WORLD_MODE,     //根据实际大小设置，单位m
    SAE_DEFAULT_MODE,        //缺省模式
    SAE_IMAGE_PIX_AREA_MODE, //像素面积模式
    SAE_REAL_WORLD_AREA_MODE //实际面积模式
} SAE_VCA_SIZE_MODE;

/*内存对齐属性*/
typedef enum _SAE_VCA_MEM_ALIGNMENT_
{
    SAE_VCA_MEM_ALIGN_4BYTE   = 4,
    SAE_VCA_MEM_ALIGN_8BYTE   = 8,
    SAE_VCA_MEM_ALIGN_16BYTE  = 16,
    SAE_VCA_MEM_ALIGN_32BYTE  = 32,
    SAE_VCA_MEM_ALIGN_64BYTE  = 64,
    SAE_VCA_MEM_ALIGN_128BYTE = 128,
    SAE_VCA_MEM_ALIGN_256BYTE = 256,
    SAE_VCA_MEM_ALIGN_END     = SAE_VCA_ENUM_END
} SAE_VCA_MEM_ALIGNMENT;
/***********************************************************************************************************************
* 图像相关
***********************************************************************************************************************/
//数据类型
typedef enum _SAE_VCA_DATA_TYPE_
{
    SAE_VCA_DATA_U08 = 0x00000001,
    SAE_VCA_DATA_S08 = 0x00000002,

    SAE_VCA_DATA_U16 = 0x10000001,
    SAE_VCA_DATA_S16 = 0x10000002,
    SAE_VCA_DATA_F16 = 0x10000003,

    SAE_VCA_DATA_U32 = 0x20000001,
    SAE_VCA_DATA_S32 = 0x20000002,
    SAE_VCA_DATA_F32 = 0x20000003,

    SAE_VCA_DATA_U64 = 0x30000001,
    SAE_VCA_DATA_S64 = 0x30000002,
    SAE_VCA_DATA_F64 = 0x30000003,

    SAE_VCA_DATA_END = SAE_VCA_ENUM_END_V2
} SAE_VCA_DATA_TYPE;

//帧类型
typedef enum _SAE_VCA_YUV_FORMAT_
{
    SAE_VCA_YUV420  = 0,      //U/V在水平、垂直方向1/2下采样；Y通道为平面格式，U/V打包存储,NV12
    SAE_VCA_YUV422  = 1,      //U/V在水平方向1/2下采样；Y通道为平面格式，U/V打包存储
    SAE_VCA_YV12    = 2,      //U/V在水平和垂直方向1/2下采样；平面格式，按Y/V/U顺序存储
    SAE_VCA_UYVY    = 3,      //YUV422交叉，硬件下采样图像格式
    SAE_VCA_YUV444  = 4,      //U/V无下采样；平面格式存储
    SAE_VCA_YVU420  = 5,      //与SAE_VCA_YUV420的区别是V在前U在后,NV21
    SAE_VCA_BGR     = 6,      //BBBGGGRRR，BGR数据
    SAE_VCA_BGRGPU  = 7,      //BBBGGGRRR，BGR数据存储在显存上
    SAE_VCA_YUV_END = SAE_VCA_ENUM_END
}SAE_VCA_YUV_FORMAT;

// 图像数据类型
typedef enum _SAE_VCA_IMG_FORMAT_
{
    //----------------------------------------------------------------------------------------------
    //注：宽度width原则上是指所见图像水平像素点个数，
    //所以YUV图像宽度都以Y分量宽度为准，
    //RGB、RGBD、RGBA都以一个分量的宽度为准；
    //而bayer格式都当作一个通道处理，宽度为一行数据各分量宽度和。
    //注：行间距是指数据平面两行开始间隔的数据个数，
    //比如RGB 3平面格式，每个平面宽度为width，则行间距step >= width;
    //比如RGB 3通道交织格式，每个平面宽度为width，则行间距step >= width*3;
    //----------------------------------//----------------------------------------------------------
    //mono (0~99)                       //| bit  | type |   store position    |        step        |
    //mono8                             //----------------------------------------------------------
    SAE_VCA_IMG_MONO_08 = 1,    //| 8b   | U08  |        D0           |         S0         |
    //mono12                            //----------------------------------------------------------
    SAE_VCA_IMG_MONO_12 = 2,    //| 12b  | S16  |        D0           |         S0         |
    //mono16                            //----------------------------------------------------------
    SAE_VCA_IMG_MONO_16 = 3,    //| 16b  | U16  |        D0           |         S0         |
    //----------------------------------//----------------------------------------------------------
    //YUV (100~199)                     //| bit  | type |   store position    |        step        |
    //YUV420 I420                       //----------------------------------------------------------
    SAE_VCA_IMG_YUV_I420 = 100,  //| 8b   | U08  |    Y:D0,U:D1,V:D2   |  Y:S0,U:S1,V:S2    |
    //YUV420 YV12                       //----------------------------------------------------------
    SAE_VCA_IMG_YUV_YV12 = 101,  //| 8b   | U08  |    Y:D0,V:D1,U:D2   |  Y:S0,V:S1,U:S2    |
    //YUV420 NV12                       //----------------------------------------------------------
    SAE_VCA_IMG_YUV_NV12 = 102,  //| 8b   | U08  |    Y:D0,UV:D1       |  Y:S0,UV:S1        |
    //YUV422 3 plane                    //----------------------------------------------------------
    SAE_VCA_IMG_YUV_422 = 103,  //| 8b   | U08  |    Y:D0,U:D1,V:D2   |  Y:S0,U:S1,V:S2    |
    //YUV422 UYVY                       //----------------------------------------------------------
    SAE_VCA_IMG_YUV_UYVY = 104,  //| 8b   | U08  |    YUV:D0           |  YUV:S0            |
    //YUV444 3 plane                    //----------------------------------------------------------
    SAE_VCA_IMG_YUV_444 = 105,  //| 8b   | U08  |    Y:D0,U:D1,V:D2   |  Y:S0,U:S1,V:S2    |
    //YUV420 NV21  
    SAE_VCA_IMG_YUV_NV21   = 106,  //| 8b   | U08  |   Y:D0,VU:D1       |  Y:S0,VU:S1   |
    //----------------------------------//----------------------------------------------------------
    //RGB (200~299)                     //| bit  | type |   store position    |        step        |
    //RBG 3 plane                       //----------------------------------------------------------
    SAE_VCA_IMG_RGB_RGB24_P3 = 200,  //| 8b   | U08  |    R:D0,G:D1,B:D2   |  R:S0,G:S1,B:S2    |
    //RGB 3 RGBRGB...                   //----------------------------------------------------------
    SAE_VCA_IMG_RGB_RGB24_C3 = 201,  //| 8b   | U08  |    RGB:D0           |  RGB:S0            |
    //RGB 4 plane(alpha)                //----------------------------------------------------------
    SAE_VCA_IMG_RGB_RGBA_P4 = 202,  //| 8b   | U08  | R:D0,G:D1,B:D2,A:D3 | R:S0,G:S1,B:S2,A:S3|
    //RGB 4 RGBARGBA...(alpha)          //----------------------------------------------------------
    SAE_VCA_IMG_RGB_RGBA_C4 = 203,  //| 8b   | U08  |    RGBA:D0          |  RGBA:S0           |
    //RGB 4 plane(depth)                //----------------------------------------------------------
    SAE_VCA_IMG_RGB_RGBD_P4 = 204,  //| 8b   | U08  | R:D0,G:D1,B:D2,A:D3 | R:S0,G:S1,B:S2,A:S3|
    //RGB 4 RGBDRGBD...(depth)          //----------------------------------------------------------
    SAE_VCA_IMG_RGB_RGBD_C4 = 205,  //| 8b   | U08  |    RGBA:D0          |  RGBA:S0           |
    //BGR 3 plane                       //----------------------------------------------------------
    SAE_VCA_IMG_RGB_BGR24_P3 = 206,  //| 8b   | U08  |    B:D0,G:D1,R:D2   |  B:S0,G:S1,R:S2    |
    //BGR 3 BGRBGR...                   //----------------------------------------------------------
    SAE_VCA_IMG_RGB_BGR24_C3 = 207,  //| 8b   | U08  |    BGR:D0           |  BGR:S0            |
    //----------------------------------//----------------------------------------------------------
    //bayer (300~399)                   //| bit  | type |   store position    |        step        |
    //bayer GRGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_GRBG_10 = 300,  //| 10b  | S16  |    GRBG:D0          |  GRBG:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_RGGB_10 = 301,  //| 10b  | S16  |    RGGB:D0          |  RGGB:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_BGGR_10 = 302,  //| 10b  | S16  |    BGGR:D0          |  BGGR:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_GBRG_10 = 303,  //| 10b  | S16  |    GBRG:D0          |  GBRG:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_GRBG_12 = 304,  //| 12b  | S16  |    GRBG:D0          |  GRBG:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_RGGB_12 = 305,  //| 12b  | S16  |    RGGB:D0          |  RGGB:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_BGGR_12 = 306,  //| 12b  | S16  |    BGGR:D0          |  BGGR:S0           |
    //bayer RGGB 10bit                  //----------------------------------------------------------
    SAE_VCA_IMG_BAYER_GBRG_12 = 307,  //| 12b  | S16  |    GBRG:D0          |  GBRG:S0           |
    //----------------------------------//----------------------------------------------------------

    // code img
    SAE_VCA_IMG_JPG = 400,

    // metadata 特征图等相关类型数据结构(500-599) 
    SAE_VCA_META_FLOW_FEAT = 500,            //    特征图数据
    SAE_VCA_IMG_END = SAE_VCA_ENUM_END_V2
}SAE_VCA_IMG_FORMAT;

/** @struct     SAE_VCA_YUV_DATA
 *  @brief      图像帧数据
 */
typedef struct _SAE_VCA_YUV_DATA_
{
    unsigned short      image_w;            //图像宽度
    unsigned short      image_h;            //图像高度
    unsigned int        pitch_y;            //图像y处理跨度
    unsigned int        pitch_uv;           //图像uv处理跨度
    SAE_VCA_YUV_FORMAT  format;             //YUV格式
    unsigned char       *y;                 //y数据指针
    unsigned char       *u;                 //u数据指针
    unsigned char       *v;                 //v数据指针
    unsigned char       scale_rate;         //下采样倍率，0或1表示原图，其他为降采样倍率如2，4，8
    unsigned char       reserved[15];       //预留16个字节
}SAE_VCA_YUV_DATA;    //44字节

/** @struct     SAE_VCA_IMG
 *  @brief      图像
 */
typedef struct _SAE_VCA_IMG_
{
    unsigned int          data_format;                    // 图像格式, 按照数据类型 SAE_VCA_IMG_FORMAT 赋值
    unsigned int          data_mem_type;                  // 图像所在的内存类型
    unsigned int          data_type;                      // 图像数据类型, SAE_VCA_DATA_TYPE
    unsigned int          data_lenth;                     // 图像数据长度，一般JPG数据会使用

    unsigned int          img_w;                          // 图像宽度
    unsigned int          img_h;                          // 图像高度
    unsigned int          stride_w[SAE_VCA_IMG_MAX_CHANEL];   // 每个通道行间距
    unsigned int          stride_h[SAE_VCA_IMG_MAX_CHANEL];   // 每个通道列间距
    void                 *data[SAE_VCA_IMG_MAX_CHANEL];       // 数据存储地址
}SAE_VCA_IMG;//72~88BYTE
/***********************************************************************************************************************
* 配置信息中规则相关
***********************************************************************************************************************/
//跨线方向类型
typedef enum _SAE_VCA_CROSS_DIRECTION_
{
    SAE_BOTH_DIRECTION, //双向
    SAE_LEFT_TO_RIGHT,  //由左至右
    SAE_RIGHT_TO_LEFT   //由右至左
} SAE_VCA_CROSS_DIRECTION;

//跨线参数
typedef struct _SAE_PARAM_TRAVERSE_PLANE_
{
    SAE_VCA_CROSS_DIRECTION cross_direction; //跨越方向 
}SAE_PARAM_TRAVERSE_PLANE;

//跨线参数
typedef struct _SAE_PARAM_REACH_HEIGHT_EX_
{
    SAE_VCA_CROSS_DIRECTION cross_direction; //跨越方向 
}SAE_PARAM_REACH_HEIGHT_EX;

//区域入侵参数
typedef struct _SAE_PARAM_INTRUSION_
{
    unsigned int delay; 
}SAE_PARAM_INTRUSION;

//徘徊参数
typedef struct _SAE_PARAM_LOITER_
{
    unsigned int delay; 
}SAE_PARAM_LOITER;

//物品拿取放置参数
typedef struct _SAE_PARAM_LEFT_TAKE_
{
    unsigned int delay; 
}SAE_PARAM_LEFT_TAKE;

//异物粘贴参数
typedef struct _SAE_PARAM_STICK_UP_
{
    unsigned int delay; 
}SAE_PARAM_STICK_UP;

//读卡器参数
typedef struct _SAE_PARAM_INSTALL_SCANNER_
{
    unsigned int delay; 
}SAE_PARAM_INSTALL_SCANNER;

//停车参数
typedef struct _SAE_PARAM_PARKING_
{
    unsigned int delay; 
}SAE_PARAM_PARKING;

//异常人脸参数
typedef struct _SAE_PARAM_ABNORMAL_FACE_
{
    unsigned int delay;
    unsigned int mode;
}SAE_PARAM_ABNORMAL_FACE;

//如厕超时参数
typedef struct _SAE_PARAM_TOILET_TARRY_
{
    unsigned int delay; 
}SAE_PARAM_TOILET_TARRY;

//放风场滞留参数
typedef struct _SAE_PARAM_YARD_TARRY_
{
    unsigned int delay;    // 滞留时间阈值 
    unsigned int mode;     // 滞留模式（任意&单人，参考ips_lib.h)
}SAE_PARAM_YARD_TARRY;

//操作超时参数
typedef struct _SAE_PARAM_OVER_TIME_
{
    unsigned int delay;
}SAE_PARAM_OVER_TIME;

//快速移动检测模式
typedef enum _SAE_VCA_RUN_MODE_
{
    SAE_SINGLE_RUN_MODE, // 单人奔跑模式
    SAE_CROWD_RUN_MODE,  // 多人奔跑模式
} SAE_VCA_RUN_MODE;

//快速移动参数
typedef struct _SAE_PARAM_RUN_
{
    float         speed;     //速度(0.1~1.0)
    unsigned int  delay;     //报警延迟
    SAE_VCA_SIZE_MODE mode;      //参数模式,只支持像素模式和实际模式两种
}SAE_PARAM_RUN;

//快速移动参数(支持单人、多人奔跑）
typedef struct _SAE_PARAM_RUN_v2_
{
    unsigned int        speed;              // 速度灵敏度，范围:[0,100]
    unsigned int        delay;              // 时间，达到该时间后发出奔跑报警，范围: [0, 600]秒
    SAE_VCA_RUN_MODE    run_mode;            // 单人、多人奔跑模式
    SAE_VCA_SIZE_MODE   mode;               // 参数模式,只支持像素模式和实际模式两种
}SAE_PARAM_RUN_V2;

//快速移动参数(支持奔跑人数配置）
typedef struct _SAE_PARAM_RUN_v3_
{
    unsigned short   delay;            // 持续时间，奔跑持续过程达到该时间后发出奔跑报警，范围: [0, 600]秒
    unsigned short   run_num;          // 奔跑报警最少人数要求, 范围:[1, 50]
    unsigned int     reserved[4];      // 保留字节
}SAE_PARAM_RUN_V3; 
 
//人员聚集参数
typedef struct _SAE_PARAM_HIGH_DENSITY_
{
    float        alert_density;        // 人员聚集报警密度, 范围: [0.1, 1.0]
    unsigned int delay;                // 时间，达到该时间后发出拥挤报警
}SAE_PARAM_HIGH_DENSITY; 

//人员聚集参数(支持最少聚集人数）
typedef struct _SAE_PARAM_HIGH_DENSITY_V2_
{
    unsigned int   alert_density;       // 人员聚集报警密度, 范围: [0, 100]
    unsigned int   alert_number;        // 人员聚集报警最少人数, 范围: [2, 50]
    unsigned int   alert_interval;      // 人员聚集前后两次报警时间间隔, 范围: [0, 600]秒
    unsigned int   delay;               // 时间，达到该时间后发出拥挤报警,范围: [0, 600]秒
}SAE_PARAM_HIGH_DENSITY_V2;   

//剧烈运动模式
typedef enum _SAE_VCA_VIOLENT_MODE_
{
    SAE_VCA_VIOLENT_VIDEO_MODE,             //纯视频模式
    SAE_VCA_VIOLENT_VIDEO_AUDIO_MODE,       //音视频联合模式
    SAE_VCA_VIOLENT_AUDIO_MODE              //纯音频模式
}SAE_VCA_VIOLENT_MODE;

//剧烈运动参数
typedef struct _SAE_PARAM_VIOLENT_
{
    unsigned int        delay;
    SAE_VCA_VIOLENT_MODE    mode;
}SAE_PARAM_VIOLENT;

//剧烈运动参数
typedef struct _SAE_PARAM_VIOLENT_V2_
{
    unsigned int            delay;            // 达到该时间后发出剧烈运动报警，范围: [0, 600]秒
    unsigned int            inteval;          // 剧烈运动前后两次报警时间间隔, 范围: [0, 600]秒
    SAE_VCA_VIOLENT_MODE    mode;             // 剧烈运动模式
}SAE_PARAM_VIOLENT_V2;

//客流量统计进入方向参数
typedef struct _SAE_PARAM_FLOW_COUNTER_
{
    SAE_VCA_VECTOR_F direction;
}SAE_PARAM_FLOW_COUNTER;

//值岗状态模式
typedef enum _SAE_VCA_LEAVE_POS_MODE_
{
    SAE_VCA_LEAVE_POSITION_LEAVE_MODE        = 0x01,  //离岗模式
    SAE_VCA_LEAVE_POSITION_SLEEP_MODE        = 0x02,  //睡岗模式
    SAE_VCA_LEAVE_POSITION_LEAVE_SLEEP_MODE  = 0x04   //离岗睡岗模式
}SAE_VCA_LEAVE_POS_MODE;

//值岗人数模式
typedef enum _SAE_VCA_LEAVE_POS_PERSON_MODE_
{
    SAE_VCA_LEAVE_POSITION_SINGLE_MODE,             //单人值岗模式
    SAE_VCA_LEAVE_POSITION_DOUBLE_MODE              //双人值岗模式
}SAE_VCA_LEAVE_POS_PERSON_MODE;

//主从通道设置
typedef enum _SAE_VCA_LEAVE_POS_CHANNEL_TYPE_
{
    SAE_VCA_LEAVE_POSITION_INDEPENDENT,                  //独立通道
    SAE_VCA_LEAVE_POSITION_MASTER,                       //主通道
    SAE_VCA_LEAVE_POSITION_SLAVE                         //辅通道
}SAE_VCA_LEAVE_POS_CHANNEL_TYPE;

//离岗检测报警参数
typedef struct _SAE_PARAM_LEAVE_POST_
{
    SAE_VCA_LEAVE_POS_MODE          mode;           //离岗状态模式
    unsigned int                    leave_delay;    //无人报警时间
    unsigned int                    static_delay;   //睡觉报警时间
    SAE_VCA_LEAVE_POS_PERSON_MODE   peo_mode;       //离岗人数模式
    SAE_VCA_LEAVE_POS_CHANNEL_TYPE  chan_type;      //通道属性，独立通道，主通道，辅通道
}SAE_PARAM_LEAVE_POST;

//倒地检测参数
typedef struct _SAE_PARAM_FALL_DOWN_
{
    unsigned int delay;
}SAE_PARAM_FALL_DOWN;

//声强突变参数
typedef struct _SAE_PARAM_AUDIO_ABNORMAL_
{
    unsigned int  decibel;       //声音强度
    unsigned char audio_mode;    //声音检测模式，0：启用灵敏度检测；1：启用分贝阈值检测
}SAE_PARAM_AUDIO_ABNORMAL;

//起床规则模式
typedef enum _SAE_VCA_GET_UP_MODE_
{
    SAE_VCA_GET_UP_OVER_BED_MODE  = 0x1,       //大床通铺模式
    SAE_VCA_GET_UP_AREA_MOVE_MODE = 0x2,       //高低铺模式
    SAE_VCA_GET_UP_SITTING_MODE   = 0x3        //坐立起身模式
}SAE_VCA_GET_UP_MODE;

//起身检测
typedef struct _SAE_PARAM_GET_UP_
{
    SAE_VCA_GET_UP_MODE mode;                  //起身检测模式
}SAE_PARAM_GET_UP;

//静坐
typedef struct _SAE_PARAM_STATIC_
{
    int delay;
}SAE_PARAM_STATIC;

// 双目倒地规则
typedef struct _SAE_PARAM_BV_FALL_DOWN_
{
    float        height;                        // 倒地高度阈值
    unsigned int delay;                         // 倒地时间
}SAE_PARAM_BV_FALL_DOWN; 

// 双目站立检测规则
typedef struct _SAE_PARAM_BV_STAND_UP_
{
    float        height;                        // 站立高度阈值
    unsigned int delay;                         // 站立时间
}SAE_PARAM_BV_STAND_UP; 

// 双目人数检测及间距大于、小于、等于、不等于判断模式
typedef enum _SAE_VCA_COMPARE_MODE_
{
    SAE_VCA_MORE_MODE,                              // 大于模式
    SAE_VCA_LESS_MODE,                              // 小于模式
    SAE_VCA_EQUAL_MODE,                             // 等于模式
    SAE_VCA_UNEQUAL_MODE                            // 不等于模式
}SAE_VCA_COMPARE_MODE;

// 双目人数检测规则
typedef struct _SAE_PARAM_BV_PEOPLE_NUM_
{
    unsigned int            people_num;                // 人数
    unsigned int            people_state;              // 有无人状态,1:有人 0:无人
    SAE_VCA_COMPARE_MODE    mode;                      // 大于小于模式
    unsigned int            delay;                     // 延时
}SAE_PARAM_BV_PEOPLE_NUM;

// 双目间距检测规则
typedef struct _SAE_PARAM_BV_PEOPLE_DISTANCE_
{
    float                   distance;                  // 两目标间距
    SAE_VCA_COMPARE_MODE    mode;                      // 大于小于模式
    unsigned int            delay;                     // 延时
}SAE_PARAM_BV_PEOPLE_DISTANCE;

//庭审着装类型
typedef enum _SAE_VCA_COURT_CLOTH_TYPE_
{
    SAE_VCA_COURT_CLOTH_ROBE        = 0x1,                //法袍
    SAE_VCA_COURT_CLOTH_UNIF_WIN    = 0x2,                //冬装制服
    SAE_VCA_COURT_CLOTH_UNIF_SUM    = 0x3                //夏装制服
}SAE_VCA_COURT_CLOTH_TYPE;

//庭审时间规则
typedef struct _SAE_PARAM_COURT_
{
    unsigned int                late_time;             // 迟到时间界限 30～1800秒（30分）
    unsigned int                leave_time;            // 早退时间界限 30～1800秒（30分）
    SAE_VCA_COURT_CLOTH_TYPE    ref_cloth_type;        // 参考着装类型
}SAE_PARAM_COURT;

// 双目物品遗留拿取规则
typedef struct _SAE_PARAM_BV_LEFT_TAKE_
{
    float        low_height;                    // 高度下限,单位米
    float        high_height;                   // 高度上限,单位米
    unsigned int delay;                         // 延时
}SAE_PARAM_BV_LEFT_TAKE; 

//双目奔跑
typedef struct _SAE_PARAM_BV_PEOPLE_RUN_
{
    float        speed_th;                      // 人员最大运动速度阈值，单位米/秒
    unsigned int delay;                         // 延时
}SAE_PARAM_BV_PEOPLE_RUN;

//双目滞留
typedef struct _SAE_PARAM_BV_PEOPLE_STOP_
{
    int          stop_th;                       // 人员最大停留时间阈值，单位秒
    unsigned int delay;                         // 延时
}SAE_PARAM_BV_PEOPLE_STOP;

//双目行程过长
typedef struct _SAE_PARAM_BV_PEOPLE_TRIP_
{
    int          trip_th;                       // 人员最大行程阈值，单位米
    unsigned int delay;                         // 延时
}SAE_PARAM_BV_PEOPLE_TRIP;

//双目跨线
typedef struct _SAE_PARAM_BV_PEOPLE_CROSS_
{
    SAE_VCA_CROSS_DIRECTION cross_direction;         //跨越方向
    unsigned int delay;                         // 延时
}SAE_PARAM_BV_PEOPLE_CROSS;

//双目剧烈运动
typedef struct _SAE_PARAM_BV_VIOLENT_MOTION_
{           
    unsigned int delay;                         // 持续时间
    unsigned int sensitive;                     // 灵敏度
}SAE_PARAM_BV_VIOLENT_MOTION;

//双目离岗
typedef struct _SAE_PARAM_BV_LEAVE_POSITION_
{    
    unsigned int delay;                         // 持续时间
    unsigned int sensitive;                     // 人体检测灵敏度
    unsigned int need_people_num;               // 需在岗人数
}SAE_PARAM_BV_LEAVE_POSITION;

//双目区域人数统计
typedef struct _SAE_PARAM_BV_REGION_PEO_NUM_
{    
    unsigned int sensitive;                     // 人体检测灵敏度
}SAE_PARAM_BV_REGION_PEO_NUM;

//人员骤散
typedef struct _SAE_PARAM_CROWD_DISAPPEAR_
{
    unsigned int        sensitive;                    // 人员骤散的灵敏度, 范围: [0, 100]
    unsigned int        number;                       // 人员骤散报警最少人数, 范围: [2, 50]
    unsigned int        delay;                        // 骤散时间,范围: [0, 60]秒
}SAE_PARAM_CROWD_DISAPPEAR;

//排队
typedef struct _SAE_PARAM_QUEUE_UP_
{
    unsigned int        queue_num;                  // 排队人数, 范围: [1, 1000]
    unsigned int        queue_time;                 // 排队时间，达到该时间后发出拥挤报警
}SAE_PARAM_QUEUE_UP;

//区域人群密度
typedef struct _SAE_PARAM_CROWD_DENSITY_MODE_
{
    unsigned int        crowd_mode_type;            // 人群密度模式
}SAE_PARAM_CROWD_DENSITY_MODE;

// 态势人群密度模式
typedef enum _SAE_PARAM_CROWD_MODE_TYPE_
{
    SAE_VCA_MIX_MODE = 0,                                // 人员密度自适应融合模式
    SAE_VCA_DET_MODE = 1,                                // 人员密度纯检测模式
    SAE_VCA_REG_MODE = 2,                                // 人员密度纯密度模式

}SAE_PARAM_CROWD_MODE_TYPE;

//异常状态检测参数
typedef struct _SAE_PARAM_ABNORMAL_STATE_
{
    unsigned int             delay;                    // 异常状态持续时间，范围: [0, 1800]秒    
    unsigned int             alert_interval;           // 报警间隔，范围: [0, 1800]秒，大于1时有效
    unsigned int             count_interval;           // 统计间隔，范围: [1, 1800]秒
    unsigned int             max_trigger_num;          // 最大触发次数，范围: [1, 100]
    unsigned char            max_trigger_valid;        // 最大触发次数是否有效 [0,1] 0-配置最大触发次数不起作用,1-起作用
    unsigned int             alert_conf_threshold;     // 报警阈值[0-1000）,大于此阈值的报警、统计结果才会输出
}SAE_PARAM_ABNORMAL_STATE;

//规则参数联合体
typedef union _SAE_VCA_RULE_SAE_PARAM_SETS_
{
    int                      param[6];                 //参数
    SAE_PARAM_TRAVERSE_PLANE     traverse_plane;           //跨线参数
    SAE_PARAM_REACH_HEIGHT_EX    reach_height_ex;          //攀高参数
    SAE_PARAM_INTRUSION          intrusion;                //区域入侵参数
    SAE_PARAM_LOITER             loiter;                   //徘徊参数
    SAE_PARAM_LEFT_TAKE          left_take;                //物品拿取放置参数
    SAE_PARAM_PARKING            parking;                  //停车参数
    SAE_PARAM_RUN                run;                      //快速移动参数
    SAE_PARAM_HIGH_DENSITY       high_density;             //人员聚集参数
    SAE_PARAM_VIOLENT            violent;                  //剧烈运动参数
    SAE_PARAM_ABNORMAL_FACE      abnormal_face;            //异常人脸参数
    SAE_PARAM_OVER_TIME          over_time;                //操作超时
    SAE_PARAM_STICK_UP           stick_up;                 //异物粘贴参数
    SAE_PARAM_INSTALL_SCANNER    insert_scanner;           //安装读卡器
    SAE_PARAM_FLOW_COUNTER       flow_counter;             //客流量统计进入方向
    SAE_PARAM_LEAVE_POST         leave_post;               //离岗检测报警时间
    SAE_PARAM_FALL_DOWN          fall_down;                //倒地检测参数
    SAE_PARAM_AUDIO_ABNORMAL     audio_abnormal;           //声音异常
    SAE_PARAM_GET_UP             get_up;                   //起身检测参数
    SAE_PARAM_YARD_TARRY         yard_tarry;               //放风场滞留时间
    SAE_PARAM_TOILET_TARRY       toilet_tarry;             //如厕超时时间
    SAE_PARAM_STATIC             stic;                     //静坐
    SAE_PARAM_BV_FALL_DOWN       bv_fall_down;             // 双目倒地
    SAE_PARAM_BV_STAND_UP        bv_stand_up;              // 双目站立
    SAE_PARAM_BV_PEOPLE_NUM      bv_people_num;            // 双目人数
    SAE_PARAM_BV_PEOPLE_DISTANCE bv_people_distance;       // 双目间距
    SAE_PARAM_COURT              court;                    //科技法庭参数
    SAE_PARAM_BV_LEFT_TAKE       bv_left_take;             // 双目物品遗留拿取
    SAE_PARAM_BV_PEOPLE_RUN      bv_people_run;            // 双目人员奔跑
    SAE_PARAM_BV_PEOPLE_STOP     bv_people_stop;           // 双目人员逗留
    SAE_PARAM_BV_PEOPLE_TRIP     bv_people_trip;           // 双目人员行程
    SAE_PARAM_BV_PEOPLE_CROSS    bv_people_cross;          // 双目人员跨线
    SAE_PARAM_BV_VIOLENT_MOTION  bv_people_violent_motion; //剧烈运动
    SAE_PARAM_BV_LEAVE_POSITION  bv_people_leave_pos;      //离岗检测
    SAE_PARAM_BV_REGION_PEO_NUM  bv_region_peo_num;        //双目区域人数统计
    SAE_PARAM_CROWD_DISAPPEAR    crowd_disappear;          // 人群骤散
    SAE_PARAM_QUEUE_UP           queue_up;                 // 排队等候
    SAE_PARAM_CROWD_DENSITY_MODE crowd_mode;               // 密度估计
    SAE_PARAM_RUN_V2             run_param;                // 快速移动参数V2版本
    SAE_PARAM_RUN_V3             run_param_ex;             // 快速移动参数V3版本
    SAE_PARAM_HIGH_DENSITY_V2    gather_param;             // 人员聚集参数V2版本
    SAE_PARAM_VIOLENT_V2         violent_param;            // 剧烈运动参数V2版本
    SAE_PARAM_ABNORMAL_STATE     abnormal_state;           // 异常状态检测

} SAE_VCA_RULE_PARAM_SETS;

//目标尺寸过滤器结构体
typedef struct _SAE_VCA_SIZE_FILTER_
{
    unsigned int        enable;      //是否激活尺寸过滤器
    SAE_VCA_SIZE_MODE   mode;        //过滤器类型
    SAE_VCA_RECT_F      min_rect;    //最小目标框
    SAE_VCA_RECT_F      max_rect;    //最大目标框
}SAE_VCA_SIZE_FILTER;

//规则参数
typedef struct _SAE_VCA_RULE_PARAM_
{
    unsigned int                sensitivity; //规则灵敏度参数
    unsigned int                reserved[4]; //预留字节
    SAE_VCA_RULE_PARAM_SETS     param;       //规则参数
    SAE_VCA_SIZE_FILTER         size_filter; //尺寸过滤器
}SAE_VCA_RULE_PARAM;

//警戒规则
typedef struct _SAE_VCA_RULE_
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

    SAE_VCA_RULE_PARAM     rule_param;     //规则参数
    SAE_VCA_POLYGON_F      polygon;        //规则对应的多边形区域
}SAE_VCA_RULE;

//警戒规则链表
typedef struct _SAE_VCA_RULE_LIST_
{
    unsigned int        rule_num;               //链表中规则数量
    SAE_VCA_RULE        rule[SAE_MAX_RULE_NUM]; //规则数组
}SAE_VCA_RULE_LIST;

#ifdef __cplusplus 
}
#endif

#endif

