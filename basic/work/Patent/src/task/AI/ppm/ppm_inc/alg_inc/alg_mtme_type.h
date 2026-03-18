/* @file       alg_mtme_type.h
 * @note       HangZhou Hikvision Digital Technology Co., Ltd.
 *             All right reserved
 * @brief      MTME对外头文件
 * @note       
 *
 * @version    v2.0.0
 * @author     张俊力6
 * @date       2022.8.25
 * @note       RK3588版本
 * 
 * @version    v1.0.0
 * @author     chentao
 * @date       2020.01.20
 * @note       正式版本，优化缩放性能
 * 
 * @version    v0.8.0
 * @author     chentao
 * @date       2020.12.1
 * @note       初始版本
 */
#ifndef _ALG_MTME_TYPE_H_
#define _ALG_MTME_TYPE_H_


#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
* 宏定义
***************************************************************************************************/
/***************************************************************************************************
* 版本号
***************************************************************************************************/
#define MTME_MAJOR_VERSION                ( 2  )
#define MTME_SUB_VERSION                  ( 0  )
#define MTME_REVISION_VERSION             ( 0  )
#define MTME_VERSION_YEAR                 ( 2023 )
#define MTME_VERSION_MONTH                ( 1  )
#define MTME_VERSION_DAY                  ( 15 )

#define MTME_MAX_OBJ_NUM                  ( 16 )     // 当帧最多目标数目
#define MTME_MAX_PACK_NUM                 ( 8 )      // 每个目标最多包裹数目
#define MTME_MAX_PATH_LENGTH              ( 256 )    // 最大路径长度
#define MTME_FACE_PIC_WIDTH               ( 1920 )   // 人脸图像宽度
#define MTME_FACE_PIC_HEIGHT              ( 1080 )   // 人脸图像高度
#define MTME_MATRIX_DIM                   ( 3 )      // 相机参数矩阵维度
#define MTME_SHIELD_REGION_MAX_NUM        ( 4 )      // 屏蔽区域个数
#define MTME_MAX_RECORED_OBJ_NUM          ( 32)      // 最大处理目标32个
#define MTME_DOST_CAM_PARAM               ( 8 )      // 相机的畸变系数
#define MTME_MAX_CALIB_IMAGE_NUM          ( 32 )     // 最大标定图像数量
#define MTME_MAX_SVCALIB_IMAGE_NUM        ( 16 )     // 最大单目相机标定图像数量
#define MTME_MAX_VERTEX_NUM               ( 10 )     // 屏蔽区域最大区域点数
#define MTME_MAX_HSG_NUM                  ( 7 )      // 目标关节点数目
#define MTME_MAX_HTR_HSG_NUM              ( 10 )     // HTR目标关节点数目，与原先对齐

/***************************************************************************************************
* 枚举
***************************************************************************************************/
/* @enum     MTME_DATA_TYPE_E
 * @brief    数据包算法处理中间结果，数据类型alg_type
 */
typedef enum _MTME_DATA_TYPE_E
{
    MTME_HKP_DATA            = 2900,         // 关节点检测（预留）
    MTME_HTR_DATA            = 2901,         // 头肩关节点关联算子（预留）
    MTME_FD_TRK_DATA         = 2902,         // 人脸检测跟踪算子
    MTME_DFR_DATA            = 2903,         // 人脸dfr评分算子
    MTME_MATCH_PACK_DATA     = 2904,         // 目标管理、可见光包裹抓拍、目标跨线管理算子
    MTME_MATCH_FACE_DATA     = 2905,         // 跨相机关联人脸算子
    MTME_CABLIC_DATA         = 3000,         // 跨相机标定算子
    MTME_SVCABLIC_DATA       = 4000          // 单目相机标定算子
}MTME_DATA_TYPE_E;

/* @enum     MTME_NODE_ID_E
 * @brief    节点枚举
 */
typedef enum MTME_NODE_ID_E_
{
    // GRAPH 1
    MTME_DECODE_NODE_ID              = 1, 
    MTME_HKP_NODE_ID                 = 2,    // 节点预留
    MTME_HTR_NODE_ID                 = 3,    // 节点预留
    MTME_FD_TRK_NODE_ID              = 4, 
    MTME_DFR_NODE_ID                 = 5,
    MTME_MATCH_PACK_NODE_ID          = 6,
    MTME_MATCH_FACE_NODE_ID          = 7,
    MTME_POST_NODE_ID                = 8,

    // GRAPH 2
    MTME_DECODE_CABLIC_NODE_ID       = 9, 
    MTME_CABLIC_NODE_ID              = 10,
    MTME_CABLIC_POST_NODE_ID         = 11,

    // GRAPH 3
    MTME_DECODE_SVCABLIC_NODE_ID     = 12,
    MTME_SVCABLIC_NODE_ID            = 13,
    MTME_SVCABLIC_POST_NODE_ID       = 14,

    MTME_NODE_ID_AMOUNT              = 0x7ffffff
} MTME_NODE_ID_E;

/* @enum     MTME_SET_CFG_E
 * @brief    引擎配置项枚举
 */
typedef enum MTME_SET_CFG_E_
{
    MTME_SET_PACK_MATCH_SENSITIVITY    = 0x1000,         // 配置可见光包裹关联灵敏度 枚举类型 MTME_PACK_MATCH_SENSITIVITY_E
    MTME_SET_PACK_MATCH_ROI            = 0x1001,         // 配置可见光包裹关联ROI区域   MTME_RECT_T 浮点归一化框
    MTME_SET_FACE_MATCH_THR            = 0x1002,         // 配置人脸关联参数IOU阈值  default 15% ,取值（0， 1]浮点类型
    MTME_SET_FACE_MATCH_ROI            = 0x1003,         // 配置人脸抓拍ROI区域  MTME_RECT_T 浮点归一化框
    MTME_SET_FACE_MATCH_MTRIX_PARAM    = 0x1004,         // 配置人脸关联R/T矩阵/人脸相机内参/三目内参 MTME_FACE_MATCH_MTRIX_T
    MTME_SET_DFR_SCORE_FILTER          = 0x1005,         // 配置人脸评分阈值 dfault 0.15,取值（0，1], 浮点类型
    MTME_SET_OBJ_SHIELD_REGION         = 0x1006,         // 配置垂直检测屏蔽区域（功能保留 暂时不支持）
    MTME_SET_CAPTURE_REGION            = 0x1007,         // 配置人脸抓拍跨线A-B区域 MTME_CAPTURE_REGION_T 
    MTME_SET_OBJ_HEIGHT_THRESH         = 0x1008,         // 配置目标高度过滤阈值(暂时不支持) int类型           
    MTME_SET_OBJ_CONFIDENCE_THRESH     = 0x1009          // 配置目标置信度过滤阈值 MTME_OBJ_CONFIDENCE_PARAM_T
}MTME_SET_CFG_E;

/* @enum     MTME_GET_CFG_E
 * @brief    引擎获取项枚举
 */
typedef enum MTME_GET_CFG_E_
{
    MTME_GET_PACK_MATCH_SENSITIVITY    = 0x1000,         // 配置可见光包裹关联灵敏度 枚举类型 MTME_PACK_MATCH_SENSITIVITY_E
    MTME_GET_PACK_MATCH_ROI            = 0x1001,         // 配置可见光包裹关联ROI区域   MTME_RECT_T 浮点归一化框
    MTME_GET_FACE_MATCH_THR            = 0x1002,         // 配置人脸关联参数IOU阈值  default 15% ,取值（0， 1]浮点类型
    MTME_GET_FACE_MATCH_ROI            = 0x1003,         // 配置人脸抓拍ROI区域  MTME_RECT_T 浮点归一化框
    MTME_GET_FACE_MATCH_MTRIX_PARAM    = 0x1004,         // 配置人脸关联R/T矩阵/人脸相机内参/三目内参 MTME_FACE_MATCH_MTRIX_T
    MTME_GET_DFR_SCORE_FILTER          = 0x1005,         // 配置人脸评分阈值 dfault 0.15,取值（0，1], 浮点类型
    MTME_GET_OBJ_SHIELD_REGION         = 0x1006,         // 配置垂直检测屏蔽区域（功能保留 暂时不支持）
    MTME_GET_CAPTURE_REGION            = 0x1007,         // 配置人脸抓拍跨线A-B区域 MTME_CAPTURE_REGION_T 
    MTME_GET_OBJ_HEIGHT_THRESH         = 0x1008,         // 配置目标高度过滤阈值            
    MTME_GET_OBJ_CONFIDENCE_THRESH     = 0x1009,         // 配置目标置信度过滤阈值 MTME_OBJ_CONFIDENCE_PARAM_T  
    MTME_GET_APP_VERSION               = 0x2000          // 获取引擎版本信息        
}MTME_GET_CFG_E;

/***************************************************************************************************
*  配置参数结构体
***************************************************************************************************/
/** @struct    MTME_OBJ_CONFIDENCE_PARAM_T
 *  @brief     设置检测目标置信度，内部默认0.35
 */ 
typedef struct MTME_OBJ_CONFIDENCE_PARAM_T_
{
    int                enable;                            // 使能
    float              filter_threshold;                  // 阈值
} MTME_OBJ_CONFIDENCE_PARAM_T;


/** @enum      MTME_PACK_MATCH_SENSITIVITY_E
 *  @brief     可见光包裹关联灵敏度枚举
 */ 
typedef enum MTME_PACK_MATCH_SENSITIVITY_E_
{
    MTME_PACK_MATCH_SENSITIVITY_STEP1      = 0x01,        // 灵敏度 1 
    MTME_PACK_MATCH_SENSITIVITY_STEP2      = 0x02,        // 灵敏度 2
    MTME_PACK_MATCH_SENSITIVITY_STEP3      = 0x03,        // 灵敏度 3
    MTME_PACK_MATCH_SENSITIVITY_STEP4      = 0x04,        // 灵敏度 4
    MTME_PACK_MATCH_SENSITIVITY_STEP5      = 0x05         // 灵敏度 5
} MTME_PACK_MATCH_SENSITIVITY_E;


/** @enum      MTME_FACE_MATCH_MTRIX_T
 *  @brief     人脸关联R/T矩阵/人脸相机内参/三目内参逆矩阵，当前通过现场标定给出
 */ 
typedef struct MTME_FACE_MATCH_MATRIX_T_
{
   float    f32Cam_height;                                         // 相机安装高度m
   double   r_p2p[MTME_MATRIX_DIM][MTME_MATRIX_DIM];               // 跨相机关联R矩阵
   double   t_p2p[MTME_MATRIX_DIM];                                // 跨相机关联T矩阵
   double   face_matrix[MTME_MATRIX_DIM][MTME_MATRIX_DIM];         // 人脸相机内参
   double   dist_param[MTME_DOST_CAM_PARAM];                       // 人脸相机畸变参数
   double   tri_matrix_inv[MTME_MATRIX_DIM][MTME_MATRIX_DIM];      // 三目中路相机内参逆矩阵
}MTME_FACE_MATCH_MTRIX_T;

/** @enum      MTME_POINT_T
 *  @brief     浮点坐标
 */ 
typedef struct _MTME_POINT_T_
{
    float   x;
    float   y;
}MTME_POINT_T;


/** @enum      MTME_HTR_POINT3D_T
 *  @brief     三维坐标点(浮点)
 */ 
typedef struct _MTME_HTR_POINT3D_T_
{
    float   x;
    float   y;
    float   z;
} MTME_HTR_POINT3D_T;

/** @struct    MTME_RECT_T
 *  @brief     坐标框信息
 */ 
typedef struct _MTME_RECT_T_
{
    float   x;
    float   y;
    float   width;
    float   height;
}MTME_RECT_T;

/** @struct    MTME_YUV_FORMAT_T
 *  @brief     输入的数据格式
 */ 
typedef enum _MTME_YUV_FORMAT_T_
{ 
    MTME_YUV_YUV420  = 0                     // 输入为YUV420格式数据，默认为NV21
}MTME_YUV_FORMAT_T; 

/** @struct    MTME_YUV_DATA_T
 *  @brief     输入的YUV数据
 */
typedef struct _MTME_YUV_DATA_T_
{
    unsigned short       image_w;            // 图像宽
    unsigned short       image_h;            // 图像高
    MTME_YUV_FORMAT_T    format;             // YUV数据格式
    unsigned char        *y;                 // y数据地址
    unsigned char        *u;                 // u数据地址
    unsigned char        *v;                 // v数据地址
    unsigned char        reserved[15];       // 保留字节
}MTME_YUV_DATA_T;  

// 目标区域
typedef struct  _MTME_OBJ_POLYGON_T_
{
    unsigned int           vertex_num;                                // 多边型点数
    MTME_POINT_T           point[MTME_MAX_VERTEX_NUM];                // 多边形实际点
} MTME_OBJ_POLYGON_T;

// 屏蔽区域链表
typedef struct MTME_BPC_SHIELD_LIST_
{
    unsigned int           shield_num;                                // 屏蔽区域个数
    unsigned int           shield_enable;                             // 屏蔽区域是否生效
    MTME_OBJ_POLYGON_T     shield[MTME_SHIELD_REGION_MAX_NUM];
} MTME_BPC_SHIELD_LIST;

//跨线抓拍区域设置
typedef struct MTME_CAPTURE_REGION_T_
{
    MTME_OBJ_POLYGON_T     region_a;                                  // 目标跨线抓拍A区域
    MTME_OBJ_POLYGON_T     region_b;                                  // 目标跨线抓拍B区域
}MTME_CAPTURE_REGION_T;

// 关键点状态
typedef enum _MTME_HTR_JOINTS_STATE_E_
{
    MTME_HTR_JOINTS_INVISIBLE  = 0,                                   // 关键点不可见
    MTME_HTR_JOINTS_VISIBLE    = 1,                                   // 关键点可见
}MTME_HTR_JOINTS_STATE_E;

// 人体关键点索引
typedef enum _MTME_HTR_JOINTS_MAIN_TYPE_
{
    MTME_HTR_JOINTS_LWRIST    = 0,                                    // 左手腕
    MTME_HTR_JOINTS_LELBOW    = 1,                                    // 左肘
    MTME_HTR_JOINTS_LSHOUDER  = 2,                                    // 左肩
    MTME_HTR_JOINTS_HEAD      = 3,                                    // 头
    MTME_HTR_JOINTS_RSHOULDER = 4,                                    // 右肩
    MTME_HTR_JOINTS_RELBOW    = 5,                                    // 右肘
    MTME_HTR_JOINTS_RWRIST    = 6,                                    // 右手腕
}MTME_HTR_JOINTS_MAIN_TYPE;

/** @struct    MTME_HTR_JOINTS_T
 *  @brief     单人关键点检测结果
 */
typedef struct _MTME_HTR_JOINTS_T_
{
    MTME_HTR_JOINTS_MAIN_TYPE    type;                               // 关节点类型
    MTME_HTR_JOINTS_STATE_E      state;                              // 关节点状态
    MTME_POINT_T                 point_2d;                           // 关节点2D坐标
    MTME_HTR_POINT3D_T           point_3d;                           // 关节点3D坐标
    float                        score;                              // 关节点评分
}MTME_HTR_JOINTS_T;

/** @struct    MTME_HTR_HKP_MSG_T
 *  @brief     单人关键点检测结果
 */
typedef struct _MTME_HTR_HKP_MSG_T_
{
    int                          id;                                 // 关节点ID
    MTME_HTR_JOINTS_T            joints[MTME_MAX_HTR_HSG_NUM];       // 关键点坐标
}MTME_HTR_HKP_MSG_T;


/***************************************************************************************************
* 标定模块输入与输出结构体
***************************************************************************************************/
/*  @struct MTME_MCC_CAM_PARAM_T
*   @brief  相机的参数矩阵
*/
typedef struct _MTME_MCC_CAM_PARAM_T
{
    double inter_matrix[MTME_MATRIX_DIM][MTME_MATRIX_DIM];             // 相机基础矩阵
    double dist_param[MTME_DOST_CAM_PARAM];                            // 相机畸变参数矩阵
}MTME_MCC_CAM_PARAM_T;

/*  @struct MTME_MCC_CAM_SIZE_T
*   @brief  标定棋盘格的宽和高（黑白相间的点数）
*/
typedef struct _MTME_MCC_CAM_SIZE_T
{
    int    width;                                                      // 宽
    int    height;                                                     // 高
}MTME_MCC_CAM_SIZE_T;

/*  @struct MTME_MCC_CABLI_IN_T
*   @brief  跨相机标定输入结构体
*/
typedef struct _MTME_MCC_CABLI_IN_T
{
    int                   width;                                       // 图像宽
    int                   height;                                      // 图像高
    int                   nimages;                                     // 图像个数
    double                square_size;                                 // 图像块大小,单位m
    MTME_MCC_CAM_SIZE_T   chessboard_size;                             // 棋盘格角点尺寸，默认（13，10）
    unsigned char        *titlcam_img_y[MTME_MAX_CALIB_IMAGE_NUM];     // 倾斜相机图像
    unsigned char        *vertical_img_y[MTME_MAX_CALIB_IMAGE_NUM];    // 垂直相机图像
    MTME_MCC_CAM_PARAM_T  vertical_cam;                                // 倾斜相机参数
    MTME_MCC_CAM_PARAM_T  tiltcam;                                     // 垂直相机参数
}MTME_MCC_CABLI_IN_T;

/*  @struct MTME_MCC_CABLI_OUT_T
*   @brief  跨相机标定输出结构体
*/
typedef struct _MTME_MCC_CABLI_OUT_T
{  
    int    cablic_status;                                              // 标定状态，反馈为0时表示有效，为其他值需要重新标定
    double R_matrix[MTME_MATRIX_DIM][MTME_MATRIX_DIM];                 // 旋转矩阵
    double T_matrix[MTME_MATRIX_DIM];                                  // 偏移矩阵
    double face_inv_matrix[MTME_MATRIX_DIM][MTME_MATRIX_DIM];          // 倾斜相机逆矩阵
    double thr_inv_matrix[MTME_MATRIX_DIM][MTME_MATRIX_DIM];           // 垂直相机内参逆矩阵
    double rms;                                                        // 像素点偏差
}MTME_MCC_CABLI_OUT_T;


/***************************************************************************************************
* 单目相机标定模块输入与输出结构体
***************************************************************************************************/
/*  @struct MTME_MCC_STC_CAM_FL
*   @brief  单目相机焦距
*/
typedef enum _MTME_MCC_STC_CAM_FL_E_
{
    MTME_MCC_STC_FL_2  = 0x1001,                 // 2mm焦距
    MTME_MCC_STC_FL_3  = 0x2001,                 // 2.8mm焦距
    MTME_MCC_STC_FL_4  = 0x3001,                 // 4mm焦距
    MTME_MCC_STC_FL_OV = 0x4001,                 
    MTME_MCC_STC_FL_6  = 0x5001,                 // 6mm焦距
    MTME_MCC_STC_FL_8  = 0x6001,                 // 8mm焦距
    MTME_MCC_STC_FL_43 = 0x7001,               
    MTME_MCC_STC_FL_12 = 0x8001                  // 12mm焦距
}MTME_MCC_STC_CAM_FL_E;

/*  @struct MTME_MCC_SV_CABLI_IN_T
*   @brief  单目相机标定输入
*/
typedef struct _MTME_MCC_SV_CABLI_IN_T
{
    int              image_num;                                   // 标定图像的数量,一般取9宫格数据
    unsigned char   *image_data[MTME_MAX_SVCALIB_IMAGE_NUM];      // 标定图像数据    
    int              img_w;                                       // 图像宽
    int              img_h;                                       // 图像高

    int              use_product_line_mode;                       // true为产线模式（即一张图包含9张标定板）
    int              fisheye_model;                               // 是否使用鱼眼模型，默认设置为0
    int              fast_search;                                 // 快速搜索棋盘格，默认设置为0
    
    int              focal_length;                                // 相机焦距， MTME_MCC_STC_CAM_FL_E
    float            pixel_size;                                  // 像元尺寸, 单位:um， 一般为2.9保持不变

    int              chessboard_width;                            // 棋盘格宽度方向角点数量
    int              chessboard_height;                           // 棋盘格高度方向角点数量
    int              chessboard_num;                              // 单张图像中的棋盘格数目
    float            squaresize;                                  // 棋盘格小方格尺寸
    int              bshow_corners;                               // 是否显示检测到的角点,默认设置为0
}MTME_MCC_SV_CABLI_IN_T;

/*  @struct MTME_MCC_SV_CABLI_OUT_T
*   @brief  单目标定输出
*/
typedef struct _MTME_MCC_SV_CABLI_OUT_T
{
    float cam_internal_mat[MTME_MATRIX_DIM][MTME_MATRIX_DIM];      // 内参矩阵
    float dist_coeffs[MTME_DOST_CAM_PARAM];                        // 畸变参数
    float rms;                                                     // 像素点偏差
    int   cablic_status;                                           // 反馈状态码，为0时表示状态正常
}MTME_MCC_SV_CABLI_OUT_T;


/***************************************************************************************************
* 引擎的输入和输出结构体（异步给出对应目标的结果，包括三个部分可见光包裹关联结果、人脸关联结果。通过目标ID关联）
***************************************************************************************************/
/***************************************************************************************************
* 引擎输入结构体
***************************************************************************************************/
/** @struct    MTME_OBJ_INPUT_INFO_T
 *  @brief     目标输入信息
 */
typedef struct MTME_OBJ_INPUT_INFO_T_
{
    int                       obj_num;                                               // 目标数量
    int                       obj_id[MTME_MAX_RECORED_OBJ_NUM];                      // 头肩目标ID号
    int                       obj_disappear[MTME_MAX_RECORED_OBJ_NUM];               // 目标消失信号    为1时表示目标消失
    int                       obj_valid[MTME_MAX_RECORED_OBJ_NUM];                   // 头肩目标有效信号
    MTME_RECT_T               obj_headrect[MTME_MAX_RECORED_OBJ_NUM];                // 头肩框坐标
    float                     obj_height[MTME_MAX_RECORED_OBJ_NUM];                  // 目标高度信息
    MTME_HTR_HKP_MSG_T        obj_joinmsg[MTME_MAX_RECORED_OBJ_NUM];                 // 目标关节点信息
}MTME_OBJ_INPUT_INFO_T;

/** @struct    MTME_INPUT_T
 *  @brief     人包关联输入信息
 */
typedef struct MTME_INPUT_T_
{
    int                       work_mode;                                             // 工作模式，暂时设置为0，后续拓展
    MTME_YUV_DATA_T           face_image;                                            // 输入人脸相机YUV数据 1080P
    MTME_YUV_DATA_T           vertical_image;                                        // 输入垂直相机YUV数据 1080P
    MTME_YUV_DATA_T           vertical_dis_image;                                    // 输入垂直相机深度图图像
    MTME_OBJ_INPUT_INFO_T     obj_info;                                              // 头肩目标信息
}MTME_INPUT_T;


/***************************************************************************************************
* 引擎输出结构体
***************************************************************************************************/
/** @struct    MTME_PACK_MATCH_INFO_T
 *  @brief     可见光包裹关联信息结构体，这里默认一人同一时刻放置的所有包裹归为同一个包裹，根据时序进行包裹区分
 */
typedef struct MTME_PACK_MATCH_INFO_T_
{
    int                       id;                                      // 目标ID
    int                       pack_id;                                 // 包裹ID,最多单人8个包裹
    int                       frame_num;                               // 帧号
    MTME_YUV_DATA_T           pack_img;                                // 包裹小图
    MTME_YUV_DATA_T           pack_bg_img;                             // 包裹背景图
}MTME_PACK_MATCH_INFO_T;

/** @struct    MTME_PACK_MATCH_OUT_T
 *  @brief     可见光包裹关联信息结构体
 */
typedef struct MTME_PACK_MATCH_OUT_T_
{
    int                       obj_num;                                 //关联目标数目
    MTME_PACK_MATCH_INFO_T    pack_match_info[MTME_MAX_OBJ_NUM];       //关联目标信息
}MTME_PACK_MATCH_OUT_T;

/** @struct    MTME_FACE_MATCH_INFO_T
 *  @brief     人脸关联信息结构体
 */
typedef struct MTME_FACE_MATCH_INFO_T_
{
    int                       id;                                      // 目标ID
    int                       frame_num;                               // 帧号
    float                     face_score;                              // 人脸评分
    MTME_RECT_T               face_rect;                               // 人脸框
    MTME_YUV_DATA_T           face_img;                                // 人脸抠图片
    MTME_YUV_DATA_T           face_bg_img;                             // 人脸背景图
}MTME_FACE_MATCH_INFO_T;

/** @struct    MTME_XSI_MATCH_OUT_T
 *  @brief     人脸关联信息
 */
typedef struct MTME_FACE_MATCH_OUT_T_
{
    int                       obj_num;                                 // 关联目标数目
    MTME_FACE_MATCH_INFO_T    face_match_info[MTME_MAX_OBJ_NUM];       // 关联目标信息
}MTME_FACE_MATCH_OUT_T;

/////////////////POS信息输出模块//////////////////////////
/** @struct    MTME_FACE_INFO_T
 *  @brief     跟踪人脸信息
 */
typedef struct _MTME_FACE_INFO_CPY_T_
{
    int            face_num;
    int            face_id[MTME_MAX_OBJ_NUM];
    MTME_RECT_T    det_out[MTME_MAX_OBJ_NUM];              // 目标框的坐标
    float          face_score[MTME_MAX_OBJ_NUM];           // 人脸评分
}MTME_FACE_INFO_CPY_T;


/* @struct   MTME_HANDLE_TAGET_INFO_T
 * @brief    引擎句柄中目标外围信息
 */
typedef struct _MTME_HANDLE_TAGET_INFO_T_
{
    int                 target_valid;                                               // 目标有效
    int                 target_id;                                                  // 关联目标ID
    int                 target_visible;                                             // 目标可见
} MTME_HANDLE_TAGET_INFO_T;


/* @struct   MTME_HANDLE_MATCH_PACK_INFO_T
 * @brief    引擎句柄中包裹匹配结构体
 */
typedef struct _MTME_HANDLE_CAPTURE_INFO_T_
{
    //穿越状态 1未进入区域A 2进入区域A 3离开区域A 4进入区域B 5离开区域B, 有效跨线目标需要满足状态变化顺序 1-2-3-4-5
    unsigned int                cross_status;
    //触发抓拍状态为A-B区域里为触发抓拍状态，离开为非触发抓拍状态
    unsigned int                capture_status;
    //目标跨线结果 1为有效跨线目标，多种状态可细分如逆向、徘徊等
    unsigned int                cross_result;

    int                 joint_num;                                                  // 关节点数目
    MTME_RECT_T         obj_ret;                                                    // 头肩框信息
    MTME_HTR_HKP_MSG_T  joint_point;                                                // 关键点

} MTME_HANDLE_CAPTURE_INFO_T;

/* @struct   MTME_HANDLE_MATCH_PACK_INFO_T
 * @brief    引擎句柄中包裹匹配结构体
 */
typedef struct _MTME_HANDLE_MATCH_PACK_INFO_T_
{
    unsigned int                trigger_cnt;                                                // 包裹关联触发计数，帧间记录，ID对应
    unsigned int                remove_cnt;                                                 // 包裹放置离开标志位
    unsigned int                flag_package_num;                                           // 每个人关联的包裹数目记录第几个包裹
    unsigned int                flag_package;                                               // 关联上包裹全局记录
    int                         package_frame[MTME_MAX_PACK_NUM];                           // 包裹对应的帧号
    int                         roi_con_count;                                              // 关节点重合数量
    unsigned int                flag_occupy;                                                // 占用标识，记录帧号，单再次触发时候帧大于128帧就删除
} MTME_HANDLE_MATCH_PACK_INFO_T;


/* @struct   MTME_HANDLE_MATCH_FACE_INFO_T
 * @brief    引擎句柄中包裹匹配结构体
 */
typedef struct _MTME_HANDLE_MATCH_FACE_INFO_T_
{
    int                     target_id;                    // 人脸匹配
    int                     face_frame;
    unsigned int            flag_face;                    // 关联上人脸全局记录
    float                   face_score;                   // 人脸评分记录
    MTME_RECT_T             face_rect;                    // 当前关联的人脸框
    char                   *face_img;                     // 人脸图像数据记录（可考虑帧缓存多张数据后续筛选
    char                   *face_bg_img;                  // 人脸背景图数据记录帧号
    int                     face_img_w;
    int                     face_img_h;
    int                     crop_result;                  // 该目标已经被抓拍

    MTME_RECT_T             face_rect_fore;               // 预测人脸框
    MTME_OBJ_POLYGON_T      head_rect_map;                // 头肩坐标映射点
    float                   rect_iou;                     // 关联的IOU值
    int                     head_face_con;                // 头肩人脸关联成功标示(关联IOU区域输出为2)
    MTME_RECT_T             head_face_con_rect;           // 关联成功对应的人脸框坐标

    unsigned int           flag_occupy;                  // 占用标识，记录帧号，单再次触发时候帧大于128帧就删除
} MTME_HANDLE_MATCH_FACE_INFO_T;

/* @struct   MTME_HANDLE_TAGET_LIST_T
 * @brief    引擎句柄中目标外围信息
 */
typedef struct _MTME_HANDLE_TAGET_LIST_T_
{
    MTME_HANDLE_TAGET_INFO_T        target_info[MTME_MAX_RECORED_OBJ_NUM];          // 目标外围信息
    MTME_HANDLE_MATCH_PACK_INFO_T   match_pack_info[MTME_MAX_RECORED_OBJ_NUM];      // 包裹关联目标结构体
    MTME_HANDLE_CAPTURE_INFO_T      capture_info[MTME_MAX_RECORED_OBJ_NUM];         // 跨线抓拍信息
    MTME_HANDLE_MATCH_FACE_INFO_T   match_face_info[MTME_MAX_RECORED_OBJ_NUM];      // 跨线抓拍信息
} MTME_HANDLE_TAGET_LIST_T;

/** @struct    MTME_RELATION_INFO_T_
 *  @brief     pos信息解析结构体
 */
typedef struct MTME_RELATION_INFO_CPY_T_
{

    MTME_HANDLE_TAGET_LIST_T     target_list;                                        // 目标链表
 
    // 人脸相机真实跟踪目标信息
    MTME_FACE_INFO_CPY_T         face_trk;                                           // 人脸检测目标信息    
 
}MTME_RELATION_INFO_CPY_T;


/** @struct    MTME_MATCH_INFO_OUT_T
 *  @brief     引擎结果输出
 */
typedef struct MTME_MATCH_INFO_OUT_T_
{
    MTME_RELATION_INFO_CPY_T     pos_data;                                 // POS信息结构体
    MTME_FACE_MATCH_OUT_T        face_match_data;                          // 人脸目标匹配信息
}MTME_MATCH_INFO_OUT_T;


#ifdef __cplusplus
}
#endif

#endif  // _ALG_MTME_TYPE_H_
