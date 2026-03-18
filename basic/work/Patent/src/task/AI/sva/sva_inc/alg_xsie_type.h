/* @file       alg_xsie_type.h
 * @note       HangZhou Hikvision Digital Technology Co., Ltd.
 *             All right reserved
 * @brief       
 * @note       XSIE(安检危险品检测对外头文件)
 *           

 *
 * @version    v2.0.0
 * @author     张大伟
 * @date       2022.9.22
 * @note       XSIE 支持双芯片安检仪基线，支持图像和视频模式
 */
 
#ifndef _ALG_XSIE_TYPE_H_
#define _ALG_XSIE_TYPE_H_

#include "vca_base.h"
#include "vca_types.h"
#include "vca_lib_common.h"
/********************************************************************************************************************
* 宏定义
********************************************************************************************************************/
#define  XSIE_SECURITY_CLASS_NUM                (192)           // 支持的危险品最大类别数
#define  XSIE_SECURITY_MAX_ALARM_NUM            (192)           // 每帧最大的报警目标数
#define  XSIE_SECURITY_INPUT_IMAGE_DATA_NUM     (1)             // 输入图像(暂时只支持一种分辨率图像输入)
#define  XSIE_SECURITY_IMAGE_MAX_WIDTH          (1600)          // 缓存图像最大宽度
#define  XSIE_SECURITY_IMAGE_MAX_HEIGHT         (1280)          // 缓存图像最小高度
#define  XSIE_SECURITY_IMAGE_BASE_WIDTH         (1280)          // 缓存图像基础宽度
#define  XSIE_SECURITY_IMAGE_BASE_HEIGHT        (1024)          // 缓存图像基础高度
#define  XSIE_SECURITY_IMAGE_LARGE_WIDTH        (1600)          // 图像模式大机型图像宽度
#define  XSIE_SECURITY_IMAGE_LARGE_HEIGHT       (1280)          // 图像模式大机型图像高度
#define  XSIE_SECURITY_IMAGE_SMALL_WIDTH        (800)           // 图像模式小机型图像宽度
#define  XSIE_SECURITY_IMAGE_SMALL_HEIGHT       (640)           // 图像模式小机型图像高度
#define  XSIE_PACKAGE_NUM                       (16)            // 最大包裹个数
#define  XSIE_PACK_DET_NUM                      (32)            // 最大检测包裹数量
#define  XSIE_SECURITY_MAX_MODEL_NUM            (4)             // 最大模型数量


#define ICF_ALGP_DEBUG(pInitHandle, fmt, ...) LOG_DEBUG(pInitHandle, fmt, ##__VA_ARGS__)
#define ICF_ALGP_ERROR(pInitHandle, fmt, ...) LOG_ERROR(pInitHandle, fmt, ##__VA_ARGS__)

#define XSIE_ALGP_RETURN(bBool, sts) \
{\
    if (bBool)\
    {\
        return sts; \
    }\
}
#define  XSIE_ALGP_CHECK(pInitHdl, bBool, sts) \
{\
    if (bBool)\
    {\
        LOG_ERROR(pInitHdl, "[ICF_ALGP ERROR:] %s line %d sts 0x%x\n", __FUNCTION__, __LINE__, sts); \
    }\
}

#define XSIE_ALGP_CHECK_RETURN(pInitHdl, bBool, sts) \
{\
    if (bBool)\
    {\
        LOG_ERROR(pInitHdl, "[ICF_ALGP ERROR:] %s line %d sts 0x%x\n", __FUNCTION__, __LINE__, sts); \
        return sts; \
    }\
}
/********************************************************************************************************************
*安检仪基线应用引擎版本基础类型
********************************************************************************************************************/
//帧类型
typedef enum _XSIE_YUV_FORMAT_E
{ 
    XSIE_YUV420  = 0,      //U/V在水平、垂直方向1/2下采样；Y通道为平面格式，U/V打包存储
    XSIE_YUV422  = 1,      //U/V在水平方向1/2下采样；Y通道为平面格式，U/V打包存储
    XSIE_YV12    = 2,      //U/V在水平和垂直方向1/2下采样；平面格式，按Y/V/U顺序存储
    XSIE_UYVY    = 3,      //YUV422交叉，硬件下采样图像格式
    XSIE_YUV444  = 4,      //U/V无下采样；平面格式存储
    XSIE_YVU420  = 5,      //与XSIE_YUV420的区别是V在前U在后
    XSIE_BGR     = 6,      //BBBGGGRRR，BGR数据
    XSIE_BGRGPU  = 7,      //BBBGGGRRR，BGR数据存储在显存上
    XSIE_YUV_END = 0xFFFFFF
}XSIE_YUV_FORMAT_E;

//矩形(浮点型)
typedef struct _XSIE_RECT_F_T_
{
    float x;         //矩形左上角X轴坐标
    float y;         //矩形左上角Y轴坐标
    float width;     //矩形宽度
    float height;    //矩形高度
}XSIE_RECT_F_T;

//图像帧数据
typedef struct _XSIE_YUV_DATA_T_
{
    unsigned short      image_w;            // 图像宽度
    unsigned short      image_h;            // 图像高度
    unsigned int        pitch_y;            // 图像y处理跨度
    unsigned int        pitch_uv;           // 图像uv处理跨度
    XSIE_YUV_FORMAT_E   format;             // YUV格式
    unsigned char      *y;                  // y数据指针
    unsigned char      *u;                  // u数据指针
    unsigned char      *v;                  // v数据指针
    unsigned char       scale_rate;         // 下采样倍率，0或1表示原图，其他为降采样倍率如2，4，8
    unsigned char       reserved[15];       // 预留16个字节
}XSIE_YUV_DATA_T;    //44字节


// 液体类型定义
typedef enum _XSIE_LCD_LIQUID_TYPE_E_
{
    XSIE_LCD_DANGEROURS_LIQUID               = 0x0001,           // 危险液体(乳胶漆)
    XSIE_LCD_SAFE_LIQUID                     = 0x0002,           // 安全液体(可乐, 水，雪碧，芬达， 气泡水)
    XSIE_LCD_EMPTY_BOTTOLE                   = 0x0003,           // 金属空瓶    暂不支持
    XSIE_LCD_NOT_EMPTY_BOTTOLE               = 0x0004,           // 金属非空瓶   暂不支持
    XSIE_LCD_UNKNOWN_LIQUID                  = 0x0005,           // 未知液体
    XSIE_LCD_EASY_BURN_LIQUID                = 0x0006,           // 易燃液体 (75% 酒精, 95% 酒精, 煤油, 汽油)
    XSIE_LCD_EMPTY_PLASTIC                   = 0x0007,           // 塑料瓶空
    XSIE_LCD_OCCLUSION_LIQUID                = 0x0008,           // 液体遮挡
} XSIE_LCD_LIQUID_TYPE_E;

/********************************************************************************************************************
*安检仪基线应用引擎版本信息配置结构体
********************************************************************************************************************/
/* @struct   XSIE_SECURITY_VERSION_T
  * @brief    安检引擎版本信息
  */
typedef struct _XSIE_SECURITY_VERSION_T_
{
    unsigned int       nMajorVersion;    // 主版本号
    unsigned int       nSubVersion;      // 子版本号
    unsigned int       nRevisVersion;    // 修订版本号
    
    unsigned int       nVersionYear;     // 版本日期-年
    unsigned int       nVersionMonth;    // 版本日期-月
    unsigned int       nVersionDay;      // 版本日期-日
}XSIE_SECURITY_VERSION_T;


/* @struct   XSIE_GET_MODEL_VERSION_T
  * @brief    获取模型版本信息
  */
typedef struct  _XSIE_GET_MODEL_VERSION_T_
{
    unsigned int              ModelIndex;          // 模型索引 0 主视角模型，1 ai开放平台模型， 2 侧视角模型
    XSIE_SECURITY_VERSION_T   ModelVersion;        // 模型版本信息
}XSIE_GET_MODEL_VERSION_T;

/********************************************************************************************************************
*安检仪工作模式枚举
********************************************************************************************************************/
/* @struct   XSIE_SECURITY_ORIENTATION_TYPE_E
  * @brief    安检机运动方向类型
  */
typedef enum _XSIE_SECURITY_ORIENTATION_TYPE_E_
{    
    XSIE_SECURITY_ORIENTATION_TYPE_R2L = 0,         //  画面由右向左运动
    XSIE_SECURITY_ORIENTATION_TYPE_L2R = 1,         //  画面由左向右运动
    XSIE_SECURITY_ORIENTATION_TYPE_NUM
}XSIE_SECURITY_ORIENTATION_TYPE_E;
/********************************************************************************************************************
* set config配置选项
********************************************************************************************************************/
/* @struct   XSIE_OBD_CONFIG_INFO_T
  * @brief    OBD配置结构体定义
  */
typedef struct _XSIE_OBD_CONFIG_INFO_T_
{
    unsigned int            ProductType;            // 产品类型
    unsigned int            ConfigType;             // 参考VCA_SET_CFG_TYPE和VCA_SET_GFG_TYPE
    unsigned int            InfoSize;               // 配置内存尺寸
    void                   *ConfigInfo;             // VCA_RULE_LIST_V4包含 rule_Info信息
}XSIE_OBD_CONFIG_INFO_T;

/* @struct   XSIE_CFG_OBJ_VAL_T
  * @brief    XSIE单项配置结构体
  */
typedef struct _XSIE_CFG_OBJ_VAL_T_
{
	int enable_flg;                                 // 是否有效
	int class_type;                                 // 类别
	int value;                                      // 数值
} XSIE_CFG_OBJ_VAL_T;

/* @struct   XSIE_SIZE_CONFIG_T
  * @brief    参数过滤模块结构体
  */
typedef struct _XSIE_SIZE_CONFIG_T_
{
    int                 sub_config_type;                            // 设置类别，目前仅0
    XSIE_CFG_OBJ_VAL_T  filter_size[XSIE_SECURITY_CLASS_NUM];       // 尺寸过滤列表
} XSIE_SIZE_CONFIG_T;

/* @struct   XSIE_SIZE_CONFIG_T
  * @brief    置信度设置模块结构体
  */
typedef struct _XSIE_CONF_CONFIG_T_
{
	int                 conf_config_type;                           // 0 主测视角一致。目前仅0
	XSIE_CFG_OBJ_VAL_T  conf_list[XSIE_SECURITY_CLASS_NUM];         // 置信度设置列表
} XSIE_CONF_CONFIG_T;

/* @struct   XSIE_STATUS_INFO_T
  * @brief    参数过滤模块结构体
  */
typedef struct _XSIE_STATUS_INFO_T_
{
    int status_return;                              // 返回状态码，正常应为0
    int node_id;                                    // 错误ID，正常应当为0，若非零，为第一次返回异常状态码的节点ID
    int line;                                       // 错误行数，正常应当为0，若非零，为第一次返回异常状态码的节点
} XSIE_STATUS_INFO_T;

/* @struct   XSIE_PASTE_CONFIG_INFO_T
  * @brief    包裹检测阈值设置结构体
  */
typedef struct _XSIE_PASTE_CONFIG_INFO_T_
{
    int                     filter_flag;            // 图像模式下，是否开启图包裹检测阈值过滤功能，若开启，割图宽、高低于阈值的输入图像不进行包裹检测
                                                    // 直接以输入割图宽高(SourYuvRect)作为包裹框，如不设置，默认开启
    VCA_RECT_F              paste_rect;             // 包裹检测阈值，如不设置，默认为（0，0，0.500，0.625），换算为分辨率即 640 x 640
}XSIE_PASTE_CONFIG_INFO_T;

/* @struct   XSIE_MAX_PACK_NUM_SET_INFO_T
  * @brief    图像模式下包裹进行违禁品检测个数阈值设置结构体
  */
typedef struct _XSIE_MAX_PACK_NUM_SET_INFO_T_
{
    int                     filter_flag;            // 图像模式下，是否开启图包裹进行违禁品检测个数阈值过滤功能，若开启，检测到的包裹大于设定阈值，则合并为一个包裹检测违禁品
    unsigned int            max_proc_pack_num;      // 图像模式下，是否开启图包裹进行违禁品检测个数阈值，必须大于0
}XSIE_MAX_PACK_NUM_SET_INFO_T;

/* @struct   XSIE_MAX_RESIZE_RATIO_CONFIG_INFO_T
  * @brief    图像模式下，包裹最大放大倍数阈值
  */
typedef struct _XSIE_MAX_RESIZE_RATIO_CONFIG_INFO_T_
{
    int                     resize_ratio_contrl_flag;   // 图像模式下，是否开启图包裹检测放大倍数控制，默认开启
    float                   max_resize_ratio;           // 图像模式下，包裹被放大的最大倍数，如不设置，默认为2.0
}XSIE_MAX_RESIZE_RATIO_CONFIG_INFO_T;

/* @struct   XSIE_SIZE_CONFIG_T
  * @brief    参数过滤模块配置项
  */
typedef enum _XSIE_CONFIG_GPARAM_ENUM_T_
{
    XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_WIDTH  = 0xC0002002, // 全局目标, 合法尺寸宽下界
    XSIE_CONFIG_GPARAM_FLTSIZE_LOWER_HEIGHT = 0xC0002003, // 全局目标, 合法尺寸高下界
    XSIE_CONFIG_GPARAM_FLTSIZE_UPPER        = 0xC0002004, // 全局目标, 合法尺寸对角线上界
    XSIE_CONFIG_STOPVALID_FRM               = 0x000c,      // 强制分割帧数
    XSIE_CONFIG_TEMPALTE_GAP                = 0x000d,      // FFT模板帧更新间隔
    XSIE_CONFIG_ALLFORWARD_FLAG             = 0x000e,      // 画面是否始终前进
    XSIE_CONFIG_PACKAGE_STRICTNESS          = 0x0010,      // 算法库包裹检出严格度
} XSIE_CONFIG_GPARAM_ENUM_T;

/* @struct   XSIE_OBD_SET_MODE_E
  * @brief    OBD检测模块配置类型
  */
typedef enum _XSIE_OBD_SET_MODE_E_
{
    XSIE_MAIN_OBD_CONFIG                     = 10001,         // OBD主视角配置
    XSIE_SIDE_OBD_CONFIG                     = 10002,         // OBD侧视角配置
    XSIE_MAIN_AI_OBD_CONFIG                  = 10003,         // AI OBD主视角配置
    XSIE_XSI_CONFIG_TYPE_GPARAM              = 10004,         // XSIE过滤参数配置
    XSIE_PASTE_THRS_CONFIG                   = 10005,         // 图像模式贴图阈值设置，小于该阈值的直接将贴图作为一个包裹送去检测
    XSIE_MAX_RESIZE_RATIO_CONFIG             = 10006,         // 图像模式下，包裹放大倍数阈值
    XSIE_SECURITE_SET_STOPVALID_FRM          = 10007,         // 强制分割帧数
                                                              // 包裹强制分割帧数配置（若不设置默认为36000）             
                                                              // 包裹强制分割帧数，即当一个包裹连续 xsi_stop_valid_frm.value 帧都没有给出valid信号时，包裹强制分割
                                                              // 即StopValid置1；
    XSIE_SECURITY_SET_TEMPALTE_GAP           = 10008,         // 获取FFT模板帧更新帧数
                                                              // FFT模板帧更新间隔配置（若不设置默认为5，高速可略微减少，低速可略微提高，但必须大于0）
                                                              // FFT模板帧更新间隔，视频模式下图像位移估计FFT模板帧每 xsi_fft_template_gap.value 更新一次
                                                              // 该设置仅在视频模式下生效，在图像模式无效，图像模式下请勿设置
    XSIE_SET_MAX_PACK_PROC_NUM               = 10009,         // 设置图像模式下，最大的，送去进行违禁品检测的包裹个数
    XSIE_SECURITE_SET_ALLFORWARD_FLAG        = 10010,         // 是否配置画面始终前进标志，仅在分包模式下生效
    XSIE_SET_CONFIG_TYPE_SENSITY             = 10011,         // 设置违禁品置信度
    XSIE_SET_PACKAGE_STRICTNESS              = 10012,         // 设置包裹前沿检出灵敏度，范围0-9，默认为5
}XSIE_OBD_SET_MODE_E;



/* @struct    XSIE_OBD_SOURCE_E
  * @brief    数据源类型
  */
typedef enum _XSIE_OBD_SOURCE_E_
{
    XSIE_MAIN_OBD             = 0,                     // 主视角OBD
    XSIE_AI_OBD               = 1,                     // AI开放平台
    XSIE_SIDE_OBD             = 2,                     // 侧视角OBD
    XSIE_MAIN_PD              = 3,                     // 主视角PD
    XSIE_SIDE_PD              = 4,                     // 侧视角PD
} XSIE_OBD_SOURCE_E;

/* @struct   XSIE_OBD_LOSS_TYPE_E
  * @brief    包裹没有进行违禁品检测原因列表
  */
typedef enum _XSIE_OBD_LOSS_TYPE_E_
{
    XSIE_LOSS_SOURCE_FRM      = 1,                     // 包裹没有对应原始帧数据（仅侧路出现）
    XSIE_ARRAY_COVERED        = 2,                     // OBD处理能力不足，被覆盖
    XSIE_OBD_PROC_TOO_LONG    = 3                      // OBD结果返回时间过长，已经无效

} XSIE_OBD_LOSS_TYPE_E;

/* @struct   XSIE_SECURITY_PROCESS_MODE_E
  * @brief   安检仪处理模式
  */ 
typedef enum _XSIE_SECURITY_PROCESS_MODE_E
{
    XSIE_SECURITY_PROC_MODE_IMAGE_SINGLE       = 0,                                  // 图像单路模式
    XSIE_SECURITY_PROC_MODE_VIDEO_SINGLE       = 1,                                  // 视频单路模式
    XSIE_SECURITY_PROC_MODE_VIDEO_DUAL         = 2,                                  // 视频双通道关联模式
    XSIE_SECURITY_PROC_MODE_PACK_DIV           = 3,                                  // 包裹分割模式，自RK_V2.3.0版本/NT_V2.2.0版本添加
} XSIE_SECURITY_PROCESS_MODE_E;

/* @struct   _XSIE_SECURITY_APP_MODE_E
  * @brief   安检仪应用场景
  */ 
typedef enum _XSIE_SECURITY_APP_MODE_E_
{
    XSIE_SECURITY_APP_NORMAL                   = 0,                                  // 日常安检模式
    XSIE_SECURITY_APP_MF                       = 1,                                  // 物流模式
    XSIE_SECURITY_APP_RT                       = 2,                                  // 物流模式

} XSIE_SECURITY_APP_MODE_E;

/* @struct   XSIE_SECURITY_AI_OBD_MODE_E
  * @brief   启动开放平台模式
  */
typedef enum _XSIE_SECURITY_AI_OBD_MODE_E
{ 
    XSIE_SECURITY_WITHOUT_AI_OBD               = 0,                                  // 不加入AI开放平台
    XSIE_SECURITY_WITH_MAIN_AI_OBD             = 1,                                  // 主视角加入AI开放平台
} XSIE_SECURITY_AI_OBD_MODE_E;

// GET参数配置
typedef enum _XSIE_SECURITY_GET_MODE_E
{ 
    XSIE_SECURITY_GET_OBD_VERSION            = 10001,                               // 获取OBD算法库版本
    XSIE_SECURITY_GET_OBD_MODEL_VERSION      = 10002,                               // 获取OBD模型版本
    XSIE_SECURITY_GET_XSI_VERSION            = 10003,                               // 获取XSI算法库版本
    XSIE_SECURITY_GET_PD_VERSION             = 10004,                               // 获取PD算法库版本
    XSIE_SECURITY_GET_PD_MODEL_VERSION       = 10005,                               // 获取PD模型版本
    XSIE_SECURITY_GET_LCD_VERSION            = 10006,                               // 获取LCD算法库版本
    XSIE_SECURITE_GET_STOPVALID_FRM          = 10007,                               // 强制分割帧数
                                                                                    // 包裹强制分割帧数配置（若不设置默认为36000）             
                                                                                    // 包裹强制分割帧数，即当一个包裹连续 xsi_stop_valid_frm.value 帧都没有给出valid信号时，包裹强制分割
                                                                                    // 即StopValid置1；
                                                                                    // 该设置仅在视频模式下生效，在图像模式无效，图像模式下请勿设置

    XSIE_SECURITY_GET_TEMPALTE_GAP           = 10008,                               // 获取FFT模板帧更新帧数
                                                                                    // FFT模板帧更新间隔配置（若不设置默认为5，高速可略微减少，低速可略微提高，但必须大于0）
                                                                                    // FFT模板帧更新间隔，视频模式下图像位移估计FFT模板帧每 xsi_fft_template_gap.value 更新一次
                                                                                    // 该设置仅在视频模式下生效，在图像模式无效，图像模式下请勿设置

    XSIE_SECURITY_GET_MAX_PACK_PROC_NUM      = 10009,                               // 获取图像模式下，最大的送去进行违禁品检测的包裹个数
    XSIE_SECURITE_GET_ALLFORWARD_FLAG        = 10010,                               // 是否配置画面始终前进标志，仅分包模式下配置生效
    XSIE_SECURITE_GET_TYPE_SESITY            = 10011,                               // 获取动态的违禁品置信度（即当前的违禁品置信度）
    XSIE_SECURITE_GET_PK_SESITY              = 10012,                               // 获取默认的违禁品PK置信度（静态的PK置信度）
    XSIE_SECURITY_GET_SIZE_FILTER_LH         = 10013,                               // 获取尺寸过滤高下界 
    XSIE_SECURITY_GET_SIZE_FILTER_LW         = 10014,                               // 获取尺寸过滤宽下界 
    XSIE_SECURITY_GET_SIZE_FILTER_UPPER      = 10015,                               // 获取异常大尺寸过滤上界
    XSIE_GET_PACKAGE_STRICTNESS              = 10016,                               // 获取包裹灵敏度
} XSIE_SECURITY_GET_MODE_E;

/* @struct   XSIE_NODE_ID_E
  * @brief    OBD检测模块配置类型
  */
typedef enum _XSIE_NODE_ID_E_
{
    XSIE_DEC_NODE_1                = 1,             // 主通道拷贝节点
    XSIE_DEC_NODE_2                = 2,             // 侧通道拷贝节点
    XSIE_ROUTE_NODE_1              = 4,             // 路由节点1
    XSIE_ROUTE_NODE_2              = 5,             // 路由节点2
    XSIE_ROUTE_NODE_3              = 6,             // 路由节点3
    XSIE_ROUTE_NODE_1_SIDE         = 7,             // 路由节点1侧通道
    XSIE_ROUTE_NODE_2_SIDE         = 8,             // 路由节点2侧通道
    XSIE_ROUTE_NODE_3_SIDE         = 9,             // 路由节点3侧通道
    XSIE_PD_NODE_1                 = 10,            // 主通道包裹检测节点
    XSIE_PD_NODE_2                 = 11,            // 侧通道包裹检测节点
    XSIE_OBD_NODE_1                = 14,            // 主通道违禁品检测节点
    XSIE_OBD_NODE_2                = 15,            // 侧通道违禁品检测节点
    XSIE_AIOBD_NODE_1              = 18,            // 主通道AI检测节点
    XSIE_AIOBD_NODE_2              = 19,            // 侧通道AI检测节点（2.0.0版本暂不用）
    XSIE_CLS_NODE_1                = 22,            // 主通道分类检测节点
    XSIE_CLS_NODE_2                = 23,            // 侧通道AI检测节点
    XSIE_SYN_NODE_1                = 26,            // 同步节点1
    XSIE_SYN_NODE_2                = 27,            // 同步节点2
    XSIE_SYN_NODE_3                = 28,            // 同步节点3
    XSIE_SYN_NODE_1_SIDE           = 29,            // 同步节点1侧通道
    XSIE_SYN_NODE_2_SIDE           = 30,            // 同步节点2侧通道
    XSIE_SYN_NODE_3_SIDE           = 31,            // 同步节点3侧通道
    XSIE_XSI_NODE                  = 32,            // XSI
    XSIE_XSI_NODE_1                = 33,            // 主通道XSI
    XSIE_XSI_NODE_2                = 34,            // 侧通道XSI
    XSIE_XSI_NODE_3                = 35,            // 包裹分割模式XSI
    XSIE_LCD_NODE_1                = 40,            // 液体识别节点1
    XSIE_LCD_NODE_2                = 41,            // 液体识别节点2
    XSIE_POST_NODE_1               = 98,            // POST节点1
    XSIE_POST_NODE_2               = 99,            // POST节点2
}XSIE_NODE_ID_E;

/********************************************************************************************************************
* 安检仪输入信息
********************************************************************************************************************/
/* @struct   XSIE_SECURITY_INPUT_PARAM_T
  * @brief    安检仪输入配置信息
  */
typedef struct _XSIE_SECURITY_INPUT_PARAM_T
{
    XSIE_RECT_F_T                       MainRoiRect;                                        // 检测区域,要求与FRCNN输入一致  //仅视频模式有效，图像模式无效
    XSIE_RECT_F_T                       SideRoiRect;                                        // 检测区域,要求与FRCNN输入一致  //仅视频模式有效，图像模式无效
    float                               PackOverX2;                                         // 判断最新包裹右边界距离图片右边界的距离的阈值T，大于这个阈值说明包裹已经完全出来了。
    XSIE_SECURITY_ORIENTATION_TYPE_E    Orientation;                                        // 安检机运动方向
    int                                 PDProcessGap;                                       // 包裹检测处理间隔
    int                                 PkSwitch;                                           // PK模式是否开启，1：是，0否
    int                                 SizeFilterSwitch;                                   // 尺寸过滤是否开启，1：是，0否
    int                                 ColorFilterSwitch;                                  // 颜色过滤是否开启，1：是，0否
    int                                 MainViewDelayTimeSeconds;                           // 主视角输出危险品保留时间
    int                                 SideViewDelayTimeSeconds;                           // 侧视角输出危险品保留时间
    float                               ObdDownsampleScale;                                 // OBD检测下采样比例
    float                               ZoomInOutThres;                                     // 放大缩小阈值
    int                                 PackageSensity;                                     // 包裹前沿灵敏度(1-10) 数值越大 越严格
}XSIE_SECURITY_INPUT_PARAM_T;

/* @struct   XSIE_SECURITY_INPUT_INFO_T
  * @brief    液体识别输入数据信息结构体
  */
typedef struct _XSIE_IN_LCD_DATA_T_
{
    char                               *InFeatRaw;                                          // 输入特征图,按顺序依次为Le He Zmap(均为16bit 2字节)
    int                                 FeatW;                                              // 特征宽
    int                                 FeatH;                                              // 特征高
}XSIE_IN_LCD_DATA_T;

/* @struct   XSIE_SECURITY_INPUT_INFO_T
  * @brief    安检仪输入数据信息结构体
  */
typedef struct _XSIE_SECURITY_INPUT_INFO_T
{
    int                                 MainNum;                                            // 主视角输入图像类别数量
    XSIE_YUV_DATA_T                     MainYuvData[XSIE_SECURITY_INPUT_IMAGE_DATA_NUM];    // 主视角yuv数据，目前只支持XSIE_YVU420，即NV21
    int                                 SourYuvRectValid;                                   // 坐标有效信号
    XSIE_RECT_F_T                       SourYuvRect[XSIE_SECURITY_INPUT_IMAGE_DATA_NUM];    // 在图像模式+ AI模式下,需要输入原图的尺寸
    XSIE_IN_LCD_DATA_T                  InLcdData;                                          // LCD输入特征
    int                                 SideNum;                                            // 主视角输入图像类别数量
    XSIE_YUV_DATA_T                     SideYuvData[XSIE_SECURITY_INPUT_IMAGE_DATA_NUM];    // 主视角yuv数据，目前只支持XSIE_YVU420，即NV21
    XSIE_SECURITY_INPUT_PARAM_T         ParamIn;                                            // 运行的配置参数信息
    int                                 Reserved[4];                                        // 预留字段
}XSIE_SECURITY_INPUT_INFO_T;

/* @struct   XSIE_SECURITY_INPUT_T
  * @brief    安检仪输入数据结构体
  */
typedef struct _XSIE_SECURITY_INPUT_T_
{
    XSIE_SECURITY_INPUT_INFO_T          InDataInfo;                                         // 输入帧信息
}XSIE_SECURITY_INPUT_T;

/********************************************************************************************************************
* 安检仪各模块输出
********************************************************************************************************************/
/* @struct   XSIE_PACKAGE_T
  * @brief   包裹信息
  */
typedef struct _XSIE_PACKAGE_T_
{    
    unsigned int                        PakageID;                                   // 包裹ID
    unsigned int                        PackLifeFrame;                              // 包裹从开始到当前帧所经历的帧数
    XSIE_RECT_F_T                       PackageRect;                                // 包裹目标框
    float                               PackageForwardLocat;                        // 包裹前沿位置输出
    unsigned int                        PackageValid;                               // 包裹是否有效，1：包裹已经分割完成，有效----0：包裹未分割完成，无效
    unsigned int                        IsHistoryPackage;                           // 该位置包裹是否成为了历史包裹，1：是，该包裹已成为历史包裹，该位置可被复用---
                                                                                    // -0：否，仍是新包裹，需要继续维护
    BOOL                                StopValid;                                  // 为长时间静止强制valid新增字段
    BOOL                                ChangeForward;                              // 是否改变了包裹前沿（当置1时，该帧及下一帧可能改变包裹前沿）
}XSIE_PACKAGE_T;

/* @struct   XSIE_PACKAGE_DET_T
  * @brief   包裹检测信息
  */
typedef struct _XSIE_PACKAGE_DET_T_
{
    unsigned int                        DataValid;                                  // 当前数据是否有效
    unsigned int                        PakageID;                                   // 包裹ID
    unsigned int                        PackFrameNum;                               // 包裹被判定做OBD的帧数
    XSIE_RECT_F_T                       PackageRect;                                // 包裹目标框
    unsigned int                        PackDetProc;                                // 该包裹是否进行违禁品检测，1：是 0：否
    unsigned int                        PackValid;                                  // 该包裹是否Valid，1：是 0：否
    XSIE_OBD_LOSS_TYPE_E                ObdLossType;                                // 该包裹未做检测的原因
}XSIE_PACKAGE_DET_T;

/* @struct   ICF_SECURITY_ALERT_TARGET_T
  * @brief   报警目标信息
  */
typedef struct _XSIE_SECURITY_ALERT_TARGET_T_
{    
    unsigned int                        ID;                                          // ID    
    XSIE_RECT_F_T                       Rect;                                        // 目标框    
    unsigned int                        Type;                                        // 目标类型    
    unsigned char                       AlarmFlg;                                    // 目标的报警标志位，TRUE为报警，FALSE为不报警    
    float                               Confidence;                                  // 目标置信度    
    float                               VisualConfidence;                            // 视觉置信度，用于客户端显示置信度。目前初定 visual_confidence = confidence/2.0+0.50.
    unsigned int                        LiquidProcFlag;                              // 是否经过液体识别处理，0为未处理，1为处理
    int                                 LiquidType;                                  // 液体识别结果，当LiquidProcFlag为1时有效，否则无效，类别参考XSIE_LCD_LIQUID_TYPE_E
    float                               ConfidenceLcd;                               // 液体识别结果置信度
    unsigned int                        Reserved[3];                                 // reserved
}XSIE_SECURITY_ALERT_TARGET_T;

/* @struct   XSIE_SECURITY_ALERT_T
  * @brief    智能模块报警信息
  */
typedef struct  _XSIE_SECURITY_ALERT_T_
{    
    unsigned int                        MainViewTargetNum;                              // 主视角报警数    
    XSIE_SECURITY_ALERT_TARGET_T        MainViewTarget[XSIE_SECURITY_MAX_ALARM_NUM];    // 主视角报警目标    
    unsigned int                        SideViewTargetNum;                              // 侧视角报警数    
    XSIE_SECURITY_ALERT_TARGET_T        SideViewTarget[XSIE_SECURITY_MAX_ALARM_NUM];    // 侧视角报警目标   
    unsigned int                        Package_force;                                  // 包裹强制分割的触发标识，1：强制分割，0：未强制分割  
    unsigned int                        Zoom_valid;                                     // 放大缩小触发标识，1：触发，0：未触发
    unsigned int                        MainViewPackageNum;                             // 主视角新出现的包裹数
    XSIE_PACKAGE_T                      MainViewPackageLoc[XSIE_PACKAGE_NUM];           // 主路包裹信息
    unsigned int                        MainViewPackDetNum;                             // 主视角进行OBD检测的包裹结果个数
    XSIE_PACKAGE_DET_T                  MainViewPackDet[XSIE_PACK_DET_NUM];             // 主视角进行OBD检测的包裹结果，根据DataValid判定该数据是否有效
    unsigned int                        SideViewPackageNum;                             // 侧视角新出现的包裹数
    XSIE_PACKAGE_T                      SideViewPackageLoc[XSIE_PACKAGE_NUM];           // 侧路包裹信息
    unsigned int                        SideViewPackDetNum;                             // 侧视角进行OBD检测的包裹结果个数
    XSIE_PACKAGE_DET_T                  SideViewPackDet[XSIE_PACK_DET_NUM];             // 侧视角进行OBD检测的包裹结果，根据DataValid判定该数据是否有效
    unsigned int                        CandidateFlag;                                  // 记录当前帧是否可能是候选正样本（一般可能存在刀枪等罕见样本）,用于样本回收?
    float                               FrameOffset;                                    // 输出帧差
    float                               PackageForwardLocat;                            // 包裹前沿位置输出
}XSIE_SECURITY_ALERT_T;

/* @struct   XSIE_SECURITY_XSI_INFO_T
  * @brief    智能模块输出信息
  */
typedef struct _XSIE_SECURITY_XSI_INFO_T_
{    
    unsigned int                        FrameNum;                                    // 帧号
    unsigned int                        TimeStamp;                                   // 时戳
    unsigned int                        ObdFrameNum;                                 // 最近一次做OBD的帧号
    XSIE_SECURITY_ALERT_T               Alert;                                       // 报警信息
    XSIE_YUV_DATA_T                     SideImg;                                     // 透传侧通道图像
}XSIE_SECURITY_XSI_INFO_T;

/* @struct   XSIE_SECURITY_XSI_POS_INFO
  * @brief    智能模块POS信息
  */
typedef struct _XSIE_SECURITY_XSI_POS_INFO_T_
{
    void        *XsiePosBuf;                                                            // 引擎POS指针
    unsigned int XsiePosSize;                                                           // 引擎pos信息结构体大小
} XSIE_SECURITY_XSI_POS_INFO_T;

/* @struct   ICF_SECURITY_XSI_TARGET_T
  * @brief    XSI结果输出
  */
typedef struct _XSIE_SECURITY_XSI_OUT_T_
{
    XSIE_STATUS_INFO_T                  XsiStatusInfo;                                      // 状态码信息
    XSIE_SECURITY_XSI_INFO_T            XsiOutInfo;                                         // 输出结果
    XSIE_SECURITY_INPUT_INFO_T          XsiAddInfo;                                         // XSI透传输入参数信息
    XSIE_SECURITY_XSI_POS_INFO_T        XsiPosInfo;                                         // XSI透传POS信息
}XSIE_SECURITY_XSI_OUT_T;

#endif
