/** @file       alg_type_face.h
 *  @note       HangZhou Hikvision Digital Technology Co., Ltd.
 *              All right reserved
 *  @brief      SAE引擎人脸模块算子层接口头文件
 *  @note       
 *  
 *  @author     chenpeng43
 *  @version    v3.0.0
 *  @date       2021/09/07
 *  @note       重构引擎，算法升级至DFR v6.0
 * 
 *  @author     桂心哲
 *  @version    v2.0.0
 *  @date       2020/07/22
 *  @note       重构
 */
#ifndef __ALG_TYPE_FACE__
#define __ALG_TYPE_FACE__

#include <pthread.h>

#include "alg_type_vca.h"
#include "alg_type_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
// 人脸应用引擎版本号
#define SAE_FACE_MAX_FEAT_SIZE                   (2048)                  // 模型大小，最大支持4patch
#define SAE_FACE_MAX_LANDMARKS_NUM               (100)                   // 最大关键点个数 5+68

#define SAE_FACE_MAX_INPUT_NUM                   (4)                     // 最大支持输入数据个数
#define SAE_FACE_MAX_FACE_NUM                    (10)                    // 最大支持输出人脸个数
#define SAE_FACE_MAX_LANDMARK_FACE_NUM           (10)                    // 多人脸最大支持进行landmark的人脸个数
#define SAE_FACE_CALIB_MAX_NUM                   (10)                    // 标定支持最大人脸个数
#define SAE_FACE_CALIB_POINT_NUM                 (6)                     // 标定输出特征点个数
#define SAE_FACE_MAX_IMG_TYPE                    (2)                     // 支持两种图输出

#define SAE_FACE_MAX_REPO_NUM                    (1)                     // 比对模块支持的最大底库数量
#define SAE_FACE_MAX_COMPARE_TOP_N               (10)                    // 比对模块支持的TopN最大值

#define SAE_FACE_DFR_MAX_ATTRIBUTE_CLASS_NUM     (10)                    // 分类属性最大类别个
#define SAE_FACE_DFR_MAX_ATTRIBUTE_REGRESS_NUM   (100)                   // 回归属性最大类别个

#define SAE_FACE_SELECT_FRAME_QUE_MAX_NUM        (16)                    // 最大缓存队列为16个

#define SAE_FACE_MAX_STRING_NUM                  (256)                   // 字符串最大字符个数

#define SAE_FACE_ENUM_BASE                       (SAE_ENUM_BASE)
#define SAE_FACE_CFG_ENUM_BASE                   (SAE_ENUM_BASE + 0x100)

// 单光模式下，可见光 或 红外光 的图像序号
#define SAE_FACE_IMG_TYPE_SMALL                 (0)
#define SAE_FACE_IMG_TYPE_BIG                   (1)

// 双光模式下，可见光 与 红外光 的图像序号
#define SAE_FACE_IMG_TYPE_VL_SMALL               (0)
#define SAE_FACE_IMG_TYPE_VL_BIG                 (1)
#define SAE_FACE_IMG_TYPE_IR_SMALL               (2)
#define SAE_FACE_IMG_TYPE_IR_BIG                 (3)

/***********************************************************************************************************************
* graph id 和 node id 的宏定义
***********************************************************************************************************************/
/** @enum       SAE_FACE_GID_E
 *  @brief      人脸业务线 graph id 的宏定义
 */
typedef enum _SAE_FACE_GID_E
{
    SAE_FACE_GID_DET_TRACK_1                    = 1,
    SAE_FACE_GID_DET_TRACK_2                    = 11,
    SAE_FACE_GID_FEAT_LIVENESS_1                = 2,
    SAE_FACE_GID_FEAT_LIVENESS_2                = 12,
    SAE_FACE_GID_DET_FEATCMP_1                  = 3,
    SAE_FACE_GID_DET_FEATCMP_2                  = 13,
    SAE_FACE_GID_DET_FEATURE_1                  = 4,
    SAE_FACE_GID_DET_FEATURE_2                  = 5,  // 下发建模第二业务线，用于双路建模，特殊设置
    SAE_FACE_GID_HELMET                         = 6,
    SAE_FACE_GID_TRACK_SELECT                   = 7,  // 抓拍路0
    SAE_FACE_GID_TRACK_SELECT_1                 = 17, // todo pengbo ADD  抓拍路1
    SAE_FACE_GID_TRACK_SELECT_2                 = 18, // todo pengbo ADD  抓拍路2
    SAE_FACE_GID_TRACK_SELECT_3                 = 19, // todo pengbo ADD  抓拍路3
    SAE_FACE_GID_TRACK_SELECT_4                 = 20, // todo pengbo ADD  抓拍路3
    SAE_FACE_GID_TRACK_SELECT_5                 = 21, // todo pengbo ADD  抓拍路3


    SAE_FACE_GID_DET_FEATURE_LOG                = 44, // 建模业务线44
    SAE_FACE_GID_DET_FEATURE_CAP                = 54  // 建模业务线54
}SAE_FACE_GID_E;

/** @enum       SAE_FACE_NID_E
 *  @brief      人脸业务线 node id 的枚举类型
 */
typedef enum _SAE_FACE_NID_E
{
    SAE_FACE_NID_DET_TRACK_1_DFR_DETECT         = 5,
    SAE_FACE_NID_DET_TRACK_1_FD_TRACK           = 10,
    SAE_FACE_NID_DET_TRACK_1_FD_QUALITY         = 15,
    SAE_FACE_NID_DET_TRACK_1_DFR_LANDMARK       = 20,
    SAE_FACE_NID_DET_TRACK_1_3RDPARTY_BRIGHT    = 50,
    SAE_FACE_NID_DET_TRACK_1_3RDPARTY_CALIB     = 55,
    SAE_FACE_NID_DET_TRACK_1_DFR_QUALITY        = 25,
    SAE_FACE_NID_DET_TRACK_1_DFR_LIVENESS       = 30,
    SAE_FACE_NID_DET_TRACK_1_DFR_ATTRIBUTE      = 75,
    SAE_FACE_NID_DET_TRACK_1_POST               = 45,
    
    SAE_FACE_NID_DET_TRACK_2_DFR_DETECT         = 105,
    SAE_FACE_NID_DET_TRACK_2_FD_TRACK           = 110,
    SAE_FACE_NID_DET_TRACK_2_FD_QUALITY         = 115,
    SAE_FACE_NID_DET_TRACK_2_DFR_LANDMARK       = 120,
    SAE_FACE_NID_DET_TRACK_2_3RDPARTY_BRIGHT    = 150,
    SAE_FACE_NID_DET_TRACK_2_3RDPARTY_CALIB     = 155,
    SAE_FACE_NID_DET_TRACK_2_DFR_QUALITY        = 125,
    SAE_FACE_NID_DET_TRACK_2_DFR_LIVENESS       = 130,
    SAE_FACE_NID_DET_TRACK_2_DFR_ATTRIBUTE      = 175,
    SAE_FACE_NID_DET_TRACK_2_POST               = 145,
    
    SAE_FACE_NID_FEAT_LIVENESS_1_DFR_LIVENESS   = 31,
    SAE_FACE_NID_FEAT_LIVENESS_1_DFR_ATTRIBUTE  = 76,
    SAE_FACE_NID_FEAT_LIVENESS_1_DFR_FEATURE    = 35,
    SAE_FACE_NID_FEAT_LIVENESS_1_DFR_COMPARE    = 40,
    SAE_FACE_NID_FEAT_LIVENESS_1_POST           = 46,
    
    SAE_FACE_NID_FEAT_LIVENESS_2_DFR_LIVENESS   = 131,
    SAE_FACE_NID_FEAT_LIVENESS_2_DFR_ATTRIBUTE  = 176,
    SAE_FACE_NID_FEAT_LIVENESS_2_DFR_FEATURE    = 135,
    SAE_FACE_NID_FEAT_LIVENESS_2_DFR_COMPARE    = 140,
    SAE_FACE_NID_FEAT_LIVENESS_2_POST           = 146,
    
    SAE_FACE_NID_DET_FEATCMP_1_DFR_DETECT       = 6,
    SAE_FACE_NID_DET_FEATCMP_1_DFR_LANDMARK     = 21,
    SAE_FACE_NID_DET_FEATCMP_1_3RDPARTY_BRIGHT  = 51,
    SAE_FACE_NID_DET_FEATCMP_1_3RDPARTY_CALIB   = 56,
    SAE_FACE_NID_DET_FEATCMP_1_DFR_QUALITY      = 26,
    SAE_FACE_NID_DET_FEATCMP_1_DFR_LIVENESS     = 32,
    SAE_FACE_NID_DET_FEATCMP_1_DFR_ATTRIBUTE    = 77,
    SAE_FACE_NID_DET_FEATCMP_1_DFR_FEATURE      = 36,
    SAE_FACE_NID_DET_FEATCMP_1_DFR_COMPARE      = 41,
    SAE_FACE_NID_DET_FEATCMP_1_POST             = 47,
    
    SAE_FACE_NID_DET_FEATCMP_2_DFR_DETECT       = 106,
    SAE_FACE_NID_DET_FEATCMP_2_DFR_LANDMARK     = 121,
    SAE_FACE_NID_DET_FEATCMP_2_3RDPARTY_BRIGHT  = 151,
    SAE_FACE_NID_DET_FEATCMP_2_3RDPARTY_CALIB   = 156,
    SAE_FACE_NID_DET_FEATCMP_2_DFR_QUALITY      = 126,
    SAE_FACE_NID_DET_FEATCMP_2_DFR_LIVENESS     = 132,
    SAE_FACE_NID_DET_FEATCMP_2_DFR_ATTRIBUTE    = 177,
    SAE_FACE_NID_DET_FEATCMP_2_DFR_FEATURE      = 136,
    SAE_FACE_NID_DET_FEATCMP_2_DFR_COMPARE      = 141,
    SAE_FACE_NID_DET_FEATCMP_2_POST             = 147,
    
    SAE_FACE_NID_DET_FEATURE_1_DFR_DETECT       = 7,
    SAE_FACE_NID_DET_FEATURE_1_DFR_LANDMARK     = 22,
    SAE_FACE_NID_DET_FEATURE_1_DFR_QUALITY      = 27,
    SAE_FACE_NID_DET_FEATURE_1_DFR_FEATURE      = 37,
    SAE_FACE_NID_DET_FEATURE_1_POST             = 48,
    
    SAE_FACE_NID_DET_FEATURE_2_DFR_DETECT       = 81,
    SAE_FACE_NID_DET_FEATURE_2_DFR_LANDMARK     = 82,
    SAE_FACE_NID_DET_FEATURE_2_DFR_QUALITY      = 83,
    SAE_FACE_NID_DET_FEATURE_2_DFR_FEATURE      = 84,
    SAE_FACE_NID_DET_FEATURE_2_POST             = 85,
    
    SAE_FACE_NID_HELMET_CLS_HELMET              = 91,
    SAE_FACE_NID_HELMET_POST                    = 92,
    
    SAE_FACE_NID_TRACK_SELECT_DFR_DETECT        = 8,
    SAE_FACE_NID_TRACK_SELECT_FD_TRACK          = 11,
    SAE_FACE_NID_TRACK_SELECT_FD_QUALITY        = 16,
    SAE_FACE_NID_TRACK_SELECT_SELECT_FRAME      = 94,
    SAE_FACE_NID_TRACK_SELECT_POST              = 95,

    SAE_FACE_NID_TRACK_SELECT_1_DFR_DETECT      = 201,
    SAE_FACE_NID_TRACK_SELECT_1_FD_TRACK        = 211,
    SAE_FACE_NID_TRACK_SELECT_1_FD_QUALITY      = 221,
    SAE_FACE_NID_TRACK_SELECT_1_SELECT_FRAME    = 241,
    SAE_FACE_NID_TRACK_SELECT_1_POST            = 251,

    SAE_FACE_NID_TRACK_SELECT_2_DFR_DETECT      = 202,
    SAE_FACE_NID_TRACK_SELECT_2_FD_TRACK        = 212,
    SAE_FACE_NID_TRACK_SELECT_2_FD_QUALITY      = 222,
    SAE_FACE_NID_TRACK_SELECT_2_SELECT_FRAME    = 242,
    SAE_FACE_NID_TRACK_SELECT_2_POST            = 252,

    SAE_FACE_NID_TRACK_SELECT_3_DFR_DETECT      = 203,
    SAE_FACE_NID_TRACK_SELECT_3_FD_TRACK        = 213,
    SAE_FACE_NID_TRACK_SELECT_3_FD_QUALITY      = 223,
    SAE_FACE_NID_TRACK_SELECT_3_SELECT_FRAME    = 243,
    SAE_FACE_NID_TRACK_SELECT_3_POST            = 253,

    SAE_FACE_NID_TRACK_SELECT_4_DFR_DETECT      = 204,
    SAE_FACE_NID_TRACK_SELECT_4_FD_TRACK        = 214,
    SAE_FACE_NID_TRACK_SELECT_4_FD_QUALITY      = 224,
    SAE_FACE_NID_TRACK_SELECT_4_SELECT_FRAME    = 244,
    SAE_FACE_NID_TRACK_SELECT_4_POST            = 254,

    SAE_FACE_NID_TRACK_SELECT_5_DFR_DETECT      = 205,
    SAE_FACE_NID_TRACK_SELECT_5_FD_TRACK        = 215,
    SAE_FACE_NID_TRACK_SELECT_5_FD_QUALITY      = 225,
    SAE_FACE_NID_TRACK_SELECT_5_SELECT_FRAME    = 245,
    SAE_FACE_NID_TRACK_SELECT_5_POST            = 255,

    // 新增建模44
    SAE_FACE_NID_DET_FEATURE_LOG_DFR_DETECT       = 407,
    SAE_FACE_NID_DET_FEATURE_LOG_DFR_LANDMARK     = 422,
    SAE_FACE_NID_DET_FEATURE_LOG_DFR_QUALITY      = 427,
    SAE_FACE_NID_DET_FEATURE_LOG_DFR_FEATURE      = 437,
    SAE_FACE_NID_DET_FEATURE_LOG_POST             = 448,

    SAE_FACE_NID_DET_FEATURE_CAP_DFR_DETECT       = 507,
    SAE_FACE_NID_DET_FEATURE_CAP_DFR_LANDMARK     = 522,
    SAE_FACE_NID_DET_FEATURE_CAP_DFR_QUALITY      = 527,
    SAE_FACE_NID_DET_FEATURE_CAP_DFR_FEATURE      = 537,
    SAE_FACE_NID_DET_FEATURE_CAP_POST             = 548,

}SAE_FACE_NID_E;

/***********************************************************************************************************************
* 枚举
***********************************************************************************************************************/

/** @enum       SAE_FUNC_MODULE_E
 *  @brief      引擎功能模块索引
 */
typedef enum _SAE_FUNC_MODULE_E
{
    SAE_FUNC_MODULE_DET                       = 0,  // 检测模块
    SAE_FUNC_MODULE_TRK                       = 1,  // 跟踪模块
    SAE_FUNC_MODULE_FDQLTY                    = 2,  // FD评分模块
    SAE_FUNC_MODULE_LDMK                      = 3,  // 关键点模块
    SAE_FUNC_MODULE_FRQLTY                    = 4,  // FR评分模块
    SAE_FUNC_MODULE_LIVE                      = 5,  // 活体模块
    SAE_FUNC_MODULE_ATTR                      = 6,  // 属性模块
    SAE_FUNC_MODULE_FEAT                      = 7,  // 建模模块
    SAE_FUNC_MODULE_CMP                       = 8,  // 比对模块
    SAE_FUNC_MODULE_SELECT                    = 9,  // 选帧模块
    SAE_FUNC_MODULE_MAX_IDX                         // 最大索引值

} SAE_FUNC_MODULE_E;

/** @enum       SAE_FACE_PROC_TYPE_E
 *  @brief      人脸引擎业处理类型
 */
typedef enum _SAE_FACE_PROC_TYPE_E
{
    // GID = SAE_FACE_GID_DET_TRACK_1 or SAE_FACE_GID_DET_TRACK_2
    SAE_FACE_PROC_TYPE_DET_TRACK           = SAE_ENUM_BASE + 0, // 人脸引擎检测、跟踪、关键点、匹配、评分、活体，跟踪可配置、活体可配置
    SAE_FACE_PROC_TYPE_DET_TRACK_V2        = SAE_ENUM_BASE + 1, // 人脸引擎检测、跟踪、关键点、匹配、评分、活体，跟踪可配置、活体可配置，支持双光输入

    // GID = SAE_FACE_GID_FEAT_LIVENESS_1 or SAE_FACE_GID_FEAT_LIVENESS_2
    SAE_FACE_PROC_TYPE_FEAT_LIVENESS       = SAE_ENUM_BASE + 2, // 人脸引擎活体、建模、比对，输出特征、活体置信度和比对top1信息，活体可配置，比对可配置

    // GID = SAE_FACE_GID_DET_FEATCMP_1 or SAE_FACE_GID_DET_FEATCMP_2
    SAE_FACE_PROC_TYPE_DET_FEATCMP         = SAE_ENUM_BASE + 3, // 人脸引擎检测、关键点、匹配、评分、活体、建模、比对特征库输出，活体配置
    SAE_FACE_PROC_TYPE_DET_FEATCMP_V2      = SAE_ENUM_BASE + 4, // 人脸引擎检测、关键点、匹配、评分、活体、建模、比对特征库输出，活体配置，支持双光输入

    // GID = SAE_FACE_GID_DET_FEATURE_1
    SAE_FACE_PROC_TYPE_DET_FEATURE         = SAE_ENUM_BASE + 5, // 人脸引擎检测、建模输出特征（应用于图片下发）
    SAE_FACE_PROC_TYPE_LANDMARK_FEATURE    = SAE_ENUM_BASE + 6, // 人脸引擎双路建模关键点建模（应用于图片下发）

    // GID = SAE_FACE_GID_DET_FEATURE_1 or SAE_FACE_GID_DET_FEATURE_2
    SAE_FACE_PROC_TYPE_DM_PREPROCESS       = SAE_ENUM_BASE + 7, // 人脸引擎双路建模预处理(仅做检测)
    SAE_FACE_PROC_TYPE_DM_LANDMARK_FEATURE = SAE_ENUM_BASE + 8, // 人脸引擎双路建模关键点建模
    SAE_FACE_PROC_TYPE_DM_FEATURE          = SAE_ENUM_BASE + 9, // 人脸引擎双路建模

    // GID = SAE_FACE_GID_HELMET
    SAE_FACE_PROC_TYPE_HELMET              = SAE_ENUM_BASE + 10, // 安全帽检测功能

    // GID = SAE_FACE_GID_TRACK_SELECT
    SAE_FACE_PROC_TYPE_TRACK_SELECT        = SAE_ENUM_BASE + 11, // 人脸引擎检测、跟踪、选帧模式


    SAE_FACE_PROC_TYPE_INVALID = SAE_ENUM_END
} SAE_FACE_PROC_TYPE_E;




/** @enum       SAE_FACE_PROC_PRIO_TYPE_E
 *  @brief      人脸引擎处理优先级类型
 */
typedef enum _SAE_FACE_PROC_PRIO_TYPE_E
{
    SAE_FACE_PROC_PRIO_TYPE_LOW         = SAE_ENUM_BASE + 0,            // 人脸引擎处理优先级低
    SAE_FACE_PROC_PRIO_TYPE_MID         = SAE_ENUM_BASE + 1,            // 人脸引擎处理优先级中
    SAE_FACE_PROC_PRIO_TYPE_HIGH        = SAE_ENUM_BASE + 2,            // 人脸引擎处理优先级高

    SAE_FACE_PROC_PRIO_TYPE_INVALID     = SAE_ENUM_END
} SAE_FACE_PROC_PRIO_TYPE_E;

/** @enum       SAE_FACE_FACE_PROC_TYPE_E
 *  @brief      人脸引擎人脸处理类型
 */
typedef enum _SAE_FACE_FACE_PROC_TYPE_E
{
    SAE_FACE_FACE_PROC_TYPE_SINGLE_FACE_MOST_CENTER  = SAE_ENUM_BASE + 0, // 选择最中间的人脸（单人脸）
    SAE_FACE_FACE_PROC_TYPE_SINGLE_FACE_BIGGEST      = SAE_ENUM_BASE + 1, // 选择最大的人脸（单人脸）
    SAE_FACE_FACE_PROC_TYPE_SINGLE_FACE_HIGHEST_CONF = SAE_ENUM_BASE + 2, // 选择置信度最高的人脸（单人脸）
    SAE_FACE_FACE_PROC_TYPE_MULTI_FACE               = SAE_ENUM_BASE + 3, // 选择所有检测人脸（多人脸）

    SAE_FACE_FACE_PROC_TYPE_INVALID = SAE_ENUM_END
} SAE_FACE_FACE_PROC_TYPE_E;

/** @enum       SAE_FACE_LIVENESS_TYPE_E
 *  @brief      人脸引擎活体处理类型
 */
typedef enum _SAE_FACE_LIVENESS_TYPE_E
{
    SAE_FACE_LIVENESS_TYPE_DISABLE      = SAE_ENUM_BASE + 0,            // 人脸引擎 禁止 活体类型
    SAE_FACE_LIVENESS_TYPE_VL           = SAE_ENUM_BASE + 1,            // 人脸引擎 可见光 活体类型
    SAE_FACE_LIVENESS_TYPE_IR           = SAE_ENUM_BASE + 2,            // 人脸引擎 红外光 活体类型
    SAE_FACE_LIVENESS_TYPE_VL_IR        = SAE_ENUM_BASE + 3,            // 人脸引擎 可见光 红外光 活体类型

    SAE_FACE_LIVENESS_TYPE_INVALID      = SAE_ENUM_END
} SAE_FACE_LIVENESS_TYPE_E;

/** @enum       SAE_FACE_ERROR_PROC_TYPE_E
 *  @brief      人脸错误返回方式
 */
typedef enum _SAE_FACE_ERROR_PROC_TYPE_E
{
    SAE_FACE_ERROR_PROC_FACE            = SAE_ENUM_BASE + 0,             // 人脸级错误返回，只要有人脸错误就停止处理并返回错误码
    SAE_FACE_ERROR_PROC_IMG             = SAE_ENUM_BASE + 1,             // 图像级错误返回，单人脸（一个人脸错误就停止处理并返回错误码）
                                                                         // 多人脸（可见光 or 红外光 所有人脸处理错误才停止处理并返回错误码）
    SAE_FACE_ERROR_PROC_MODULE          = SAE_ENUM_BASE + 2,             // 模块级错误返回，如果双图模式，需要 可见光 + 红外光 图像全部错误，才会停止处理并返回错误码

    SAE_FACE_ERROR_PROC_TYPE_INVALID    = SAE_ENUM_END
} SAE_FACE_ERROR_PROC_TYPE_E;

/** @enum       SAE_FACE_FR_QLTY_CLASS_E
 *  @brief      人脸质量评分档位
 */
typedef enum _SAE_FACE_FR_QLTY_CLASS_E
{
    SAE_FACE_FR_QLTY_CLASS_HIGH          = SAE_ENUM_BASE + 0,            // 苛刻条件
    SAE_FACE_FR_QLTY_CLASS_LOW           = SAE_ENUM_BASE + 1,            // 宽松条件
    
    SAE_FACE_FR_QLTY_CLASS_TYPE_INVALID  = SAE_ENUM_END
} SAE_FACE_FR_QLTY_CLASS_E;

/** @enum       SAE_FACE_FD_SHD_TYPE_E
 *  @brief      遮挡配置类型
 */
typedef enum _SAE_FACE_FD_SHD_TYPE_E
{
    SAE_FACE_FD_SHD_NONE                = SAE_ENUM_BASE + 0,             // 无遮挡
    SAE_FACE_FD_SHD_SS_QZ               = SAE_ENUM_BASE + 1,             // 瞬时轻中
    SAE_FACE_FD_SHD_GD_QZ               = SAE_ENUM_BASE + 2,             // 固定轻中
    SAE_FACE_FD_SHD_YZ                  = SAE_ENUM_BASE + 3,             // 严重
    SAE_FACE_FD_SHD_OSD                 = SAE_ENUM_BASE + 4,             // OSD

    SAE_FACE_FD_SHD_INVALID             = SAE_ENUM_END
} SAE_FACE_FD_SHD_TYPE_E;

/** @enum       SAE_FACE_CFG_TYPE_E
 *  @brief      引擎配置类型
 */
typedef enum _SAE_FACE_CFG_TYPE_E
{
    // 注册函数配置
    SAE_FACE_CFG_CB_YUV2RGB_FUNC        = SAE_FACE_CFG_ENUM_BASE + 0x01,            // 设置YUV转RGB注册函数
    SAE_FACE_CFG_CB_CALC_BRIGHT_FUNC    = SAE_FACE_CFG_ENUM_BASE + 0x02,            // 设置门禁亮度计算注册函数
    SAE_FACE_CFG_CB_CALIBRATION_FUNC    = SAE_FACE_CFG_ENUM_BASE + 0x03,            // 设置标定算法注册函数
    SAE_FACE_CFG_CB_CROP_FUNC           = SAE_FACE_CFG_ENUM_BASE + 0x04,            // 设置、获取CROP注册函数

    // 人脸参数配置
    SAE_FACE_CFG_DET_CONF_THRESH        = SAE_FACE_CFG_ENUM_BASE + 0x20,            // 设置、获取检测置信度阈值(可见光)
    SAE_FACE_CFG_DET_CONF_THRESH_IR     = SAE_FACE_CFG_ENUM_BASE + 0x21,            // 设置、获取检测置信度阈值(红外光)
    SAE_FACE_CFG_FD_TRK_GENERATE_RATE   = SAE_FACE_CFG_ENUM_BASE + 0x22,            // 设置、获取人脸跟踪目标生成速度
    SAE_FACE_CFG_FD_TRK_SENSITIVITY     = SAE_FACE_CFG_ENUM_BASE + 0x23,            // 设置、获取人脸跟踪灵敏度
    SAE_FACE_CFG_FD_TRK_RULE_LIST       = SAE_FACE_CFG_ENUM_BASE + 0x24,            // 设置、获取跟踪人脸大小和区域规则
    SAE_FACE_CFG_FD_TRK_RESET           = SAE_FACE_CFG_ENUM_BASE + 0x25,            // 重置跟踪模块
    SAE_FACE_CFG_FACE_QUALITY_CLASS     = SAE_FACE_CFG_ENUM_BASE + 0x26,            // 获取人脸质量档位对应评分
    SAE_FACE_CFG_FACE_QUALITY_THRSH     = SAE_FACE_CFG_ENUM_BASE + 0x27,            // 设置、获取人脸质量校验阈值(可见光)
    SAE_FACE_CFG_FACE_QUALITY_THRSH_IR  = SAE_FACE_CFG_ENUM_BASE + 0x28,            // 设置、获取人脸质量校验阈值(红外光)
    SAE_FACE_CFG_CALC_BRIGHT_THRSH      = SAE_FACE_CFG_ENUM_BASE + 0x29,            // 设置、获取亮度过滤阈值参数(可见光)
    SAE_FACE_CFG_CALC_BRIGHT_THRSH_IR   = SAE_FACE_CFG_ENUM_BASE + 0x2A,            // 设置、获取亮度过滤阈值参数(红外光)

    SAE_FACE_CFG_FR_ATTR_GLASS_THRESHOLD       =    SAE_FACE_CFG_ENUM_BASE + 0x30,   // 配置眼镜置信度阈值，默认阈值：6.0版本0.8855    // todo pengbo ADD
    SAE_FACE_CFG_FR_ATTR_MASK_THRESHOLD        =    SAE_FACE_CFG_ENUM_BASE + 0x31,   // 配置口罩置信度阈值，默认阈值：6.0版本0.6859    // todo pengbo ADD
    SAE_FACE_CFG_FR_ATTR_HAT_THRESHOLD         =    SAE_FACE_CFG_ENUM_BASE + 0x32,   // 配置帽子置信度阈值，默认阈值：6.0版本0.6746    // todo pengbo ADD
    SAE_FACE_CFG_FR_ATTR_MANNER_THRESHOLD      =    SAE_FACE_CFG_ENUM_BASE + 0x33,   // 配置口罩规范置信度阈值，默认阈值：7.0版本，0.0 // todo pengbo ADD
    SAE_FACE_CFG_FR_ATTR_MASK_TYPE_THRESHOLD   =    SAE_FACE_CFG_ENUM_BASE + 0x34,   // 配置口罩类型置信度阈值，默认阈值：7.0版本，0.0 // todo pengbo ADD
    SAE_FACE_CFG_FR_ATTR_GENDER_THRESHOLD      =    SAE_FACE_CFG_ENUM_BASE + 0x35,   // 配置性别置信度阈值，默认阈值：6.0版本0.7466    // todo pengbo ADD
    SAE_FACE_CFG_FR_ATTR_EXPRESS_THRESHOLD     =    SAE_FACE_CFG_ENUM_BASE + 0x36,   // 配置表情置信度阈值，默认阈值：6.0版本0.0       // todo pengbo ADD

    SAE_FACE_CFG_LIVENESS_THRSH         = SAE_FACE_CFG_ENUM_BASE + 0x40,            // 活体阈值-可见光
    SAE_FACE_CFG_LIVENESS_THRSH_IR      = SAE_FACE_CFG_ENUM_BASE + 0x41,            // 活体阈值-红外光

    SAE_FACE_CFG_CMP_IMPORT_FEATREPO   = SAE_FACE_CFG_ENUM_BASE + 0x50,            // 导入人脸特征仓库  // todo pengbo ADD

    SAE_FACE_CFG_FD_SELECT_FRAME_SETTING= SAE_FACE_CFG_ENUM_BASE + 0x60,            // 设置、获取抓拍选帧策略
    SAE_FACE_CFG_FD_SELECT_FRAME_CLEAN  = SAE_FACE_CFG_ENUM_BASE + 0x61,            // 设置清零选帧缓存
    
    SAE_FACE_CFG_TYPE_INVALID           = SAE_ENUM_END
} SAE_FACE_CFG_TYPE_E;

/** @enum       SAE_FACE_QLTY_FILTER_TYPE_E
 *  @brief      人脸质量过滤类型
 */
typedef enum _SAE_FACE_QLTY_FILTER_TYPE_E
{
    SAE_FACE_QLTY_FILTER_TYPE_DETECT_CONF   = 0x00000001, // 检测框置信度过滤
    SAE_FACE_QLTY_FILTER_TYPE_LANDMARK_CONF = 0x00000002, // 特征点置信度过滤
    SAE_FACE_QLTY_FILTER_TYPE_EYE_DISTANCE  = 0x00000004, // 眼距过滤
    SAE_FACE_QLTY_FILTER_TYPE_COLOR_CONF    = 0x00000008, // 彩色置信度过滤
    SAE_FACE_QLTY_FILTER_TYPE_GRAY_SCALE    = 0x00000010, // 灰阶过滤
    SAE_FACE_QLTY_FILTER_TYPE_GRAY_MEAN     = 0x00000020, // 灰度均值过滤
    SAE_FACE_QLTY_FILTER_TYPE_GRAY_VARIANCE = 0x00000040, // 灰度方差过滤
    SAE_FACE_QLTY_FILTER_TYPE_POSE_PITCH    = 0x00000080, // 俯仰角姿态过滤
    SAE_FACE_QLTY_FILTER_TYPE_POSE_YAW      = 0x00000100, // 左右旋转姿态过滤
    SAE_FACE_QLTY_FILTER_TYPE_POSE_ROLL     = 0x00000200, // 左右平面内旋转姿态过滤
    SAE_FACE_QLTY_FILTER_TYPE_CLEARITY      = 0x00000400, // 清晰度过滤
    SAE_FACE_QLTY_FILTER_TYPE_VISIBLE       = 0x00000800, // 遮挡过滤
    SAE_FACE_QLTY_FILTER_TYPE_FRONT_SCORE   = 0x00001000, // 正面成都过滤
    SAE_FACE_QLTY_FILTER_TYPE_FACE_SCORE    = 0x00002000, // 综合评分过滤

    SAE_FACE_QLTY_FILTER_TYPE_INVALID       = SAE_ENUM_END
} SAE_FACE_QLTY_FILTER_TYPE_E;

/***********************************************************************************************************************
* 参数配置结构体
***********************************************************************************************************************/
/** @struct     SAE_FACE_RANGE_T
 *  @brief      浮点取值范围
 */
typedef struct _SAE_FACE_RANGE_T
{
    float                       low;                      // 灰度参数浮点取值范围0~255
    float                       high;                     // 灰度参数浮点取值范围0~255
} SAE_FACE_RANGE_T;

/** @struct     SAE_FACE_CFG_YUV2RGB_FUNC_T
 *  @brief      YUV转RGB注册函数配置结构体
 */
typedef struct _SAE_FACE_CFG_YUV2RGB_FUNC_T
{
    void                        *func_yuv2rgb;            // yuv2rgb注册函数
} SAE_FACE_CFG_YUV2RGB_FUNC_T;

/** @struct     SAE_FACE_CFG_CALC_BRIGHT_FUNC_T
 *  @brief      亮度计算注册函数配置结构体
 */
typedef struct _SAE_FACE_CFG_CALC_BRIGHT_FUNC_T
{
    void                        *func_calc_bright;        // 亮度计算注册函数
} SAE_FACE_CFG_CALC_BRIGHT_FUNC_T;

/** @struct     SAE_FACE_CFG_CALIB_FUNC_T
 *  @brief      标定算法注册函数配置结构体
 */
typedef struct _SAE_FACE_CFG_CALIB_FUNC_T
{
    void                        *func_calib;              // 标定算法注册函数
} SAE_FACE_CFG_CALIB_FUNC_T;

/** @struct     SAE_FACE_CFG_CROP_FUNC_T
 *  @brief      YUV转RGB注册函数配置结构体
 */
typedef struct _SAE_FACE_CFG_CROP_FUNC_T
{
    void                        *func_crop;               // crop注册函数
} SAE_FACE_CFG_CROP_FUNC_T;

/** @struct     SAE_FACE_CMP_CMD_E
 *  @brief      RN平台设置接口CMD枚举范围 [0X6000，0X6FFF],共4096个
 */ 
typedef enum _SAE_FACE_CMP_CMD_E
{
    // COMPARE 模块(增删改只在Setconfig接口生效)
    SAE_FACE_COMPARE_IMPORT             = 0X6001,      // 修改特征命令  // todo pengbo 
} SAE_FACE_CMP_CMD_E;

/** @struct     SAE_FACE_CFG_FEATREPO_T
 *  @brief      人脸底库参数配置
 */
typedef struct _SAE_FACE_CFG_FEATREPO_INFO_T
{
    unsigned int                index;                    // 底库导入/导出位置
    unsigned int                feat_data_stride;         // 特征数据跳转量
    unsigned int                feat_repo_num;            // 特征库特征数目
    void                       *feat_repo_addr;           // 特征库特征首地址
    pthread_mutex_t            *feat_repo_mutex_ptr;      // 底库锁
    float                       sim_thresh;               // 特征相似度阈值
    SAE_FACE_CMP_CMD_E          cmd;                      // 底库加载/查看命令
} SAE_FACE_CFG_FEATREPO_INFO_T;

/** @struct     SAE_FACE_CFG_FEATREPO_T
 *  @brief      人脸底库参数配置
 */
typedef struct _SAE_FACE_CFG_FEATREPO_T
{
    unsigned int                 mask_id; // 口罩人脸底库id(0 < mask_id < SAE_FACE_MAX_REPO_NUM), repo_info[mask_id] 表示口罩人脸底库
    unsigned int                 repo_num;                          // 人脸底库数量
    SAE_FACE_CFG_FEATREPO_INFO_T repo_info[SAE_FACE_MAX_REPO_NUM];  // 人脸底库
} SAE_FACE_CFG_FEATREPO_T;

/** @struct     SAE_FACE_CFG_LIVE_THRESH_T
 *  @brief      活体相似度阈值参数配置
 */
typedef struct _SAE_FACE_CFG_LIVE_THRESH_T
{
    float                       live_thresh;              // 活体相似度阈值
} SAE_FACE_CFG_LIVE_THRESH_T;

/** @struct     SAE_FACE_CFG_DET_CONF_THRESH_T
 *  @brief      检测置信度阈值参数配置
 */
typedef struct _SAE_FACE_CFG_DET_CONF_THRESH_T
{
    float                       det_conf_thresh;          // 可见光/红外光 检测置信度阈值
} SAE_FACE_CFG_DET_CONF_THRESH_T;

/** @struct     SAE_FACE_CFG_BRIGHT_THRESH_T
 *  @brief      亮度过滤阈值参数配置
 */
typedef struct _SAE_FACE_CFG_BRIGHT_THRESH_T
{
    SAE_FACE_RANGE_T            bright_thresh;            // 可见光/红外光 亮度过滤阈值参数
} SAE_FACE_CFG_BRIGHT_THRESH_T;

/** @struct     SAE_FACE_CFG_TRACK_GENERATE_RATE_T
 *  @brief      跟踪目标生成速率
 */
typedef struct _SAE_FACE_CFG_TRACK_GENERATE_RATE_T
{
    int                         generate_rate;            // 目标生成速度，范围[1,5]，默认值3
} SAE_FACE_CFG_TRACK_GENERATE_RATE_T;

/** @struct     SAE_FACE_CFG_TRACK_SENSITIVITY_T
 *  @brief      跟踪检测灵敏度
 */
typedef struct _SAE_FACE_CFG_TRACK_SENSITIVITY_T
{        
    int                         sensitivity_value;        // 目标检测灵敏度，范围[1,5]，默认值是5
} SAE_FACE_CFG_TRACK_SENSITIVITY_T;

/** @struct     SAE_FACE_CFG_TRACK_RULE_T
 *  @brief      跟踪人脸大小和区域参数配置结构体
 */
typedef struct _SAE_FACE_CFG_TRACK_RULE_T
{
    SAE_VCA_RULE_LIST           rule_list;                // 人脸跟踪规则链表，默认配置全图跟踪
} SAE_FACE_CFG_TRACK_RULE_T;

/** @struct     SAE_FACE_CFG_TRACK_RESET_T
 *  @brief      人脸跟踪重置状态参数配置结构体
 */
typedef struct _SAE_FACE_CFG_TRACK_RESET_T
{
    int           reset;                                 // 人脸跟踪重置状态位，reset=1表示重置
} SAE_FACE_CFG_TRACK_RESET_T;

/** @struct     SAE_FACE_QLTY_THRSH_T
 *  @brief      人脸评分校验阈值结构体
 */
typedef struct _SAE_FACE_QLTY_THRSH_T
{
    float                       landmark_confidence;      // 特征点置信度0~1.0
    unsigned int                detect_orientation;       // 暂时用不到
    float                       eye_distance;             // 两眼间距,真实像素值>0
    float                       color_confidence;         // 彩色置信度（实际不会使用）0~1.0
    unsigned int                gray_scale;               // 灰阶，评价曝光 0~255
    SAE_FACE_RANGE_T            gray_mean_range;          // 灰度均值范围 0~255
    SAE_FACE_RANGE_T            gray_variance_range;      // 灰度方差范围；0~128
    float                       clearity_score;           // 清晰度评分，范围0~1
    float                       pose_pitch;               // 平面外上下俯仰角，人脸朝上为正，范围-90~90
    float                       pose_yaw;                 // 平面外左右偏转角，人脸朝左为正，范围-90~90
    float                       pose_roll;                // 平面内旋转角，人脸顺时针旋转为正，范围-90~90
    float                       pose_confidence;          // 姿态(pose_pitch、pose_yaw、pose_roll) 置信度，范围0~1
    float                       frontal_score;            // 正面程度评分，范围0~1
    float                       visible_score;            // 可见性评分（即不遮挡），范围0~1，0表示完全遮挡，1表示完全不遮挡
    unsigned int                reseved[32];              // 保留字节，后续扩展评分项
    float                       face_score;               // 人脸评分，范围0~1
} SAE_FACE_QLTY_THRSH_T;

/** @struct     SAE_FACE_CFG_QLTY_THRSH_T
 *  @brief      人脸评分校验阈值配置参数结构体
 */
typedef struct _SAE_FACE_CFG_QLTY_THRSH_T
{
    SAE_FACE_QLTY_THRSH_T       qty_thresh;               // 可见光/红外光 评分过滤阈值参数
} SAE_FACE_CFG_QLTY_THRSH_T;

/** @struct     SAE_FACE_CFG_QLTY_CLASS_T
 *  @brief      人脸图像质量档位参数配置
 */
typedef struct _SAE_FACE_CFG_QLTY_CLASS_T
{
    SAE_FACE_FR_QLTY_CLASS_E     qty_class;              // 人脸质量档位参数
    SAE_FACE_QLTY_THRSH_T        qty_thresh;             // 默认评分过滤阈值参数
} SAE_FACE_CFG_QLTY_CLASS_T;

/** @struct     SAE_FACE_CFG_SELECT_FRAME_PARAM_T
 *  @brief      抓拍选帧模块配置参数结构体
 */
typedef struct _SAE_FACE_CFG_SELECT_FRAME_PARAM_T
{
    int                         reset_flag;              // 复位标记, 清空历史缓存
    int                         select_gap;              // 抓拍一帧的时间(帧)间隔
    int                         unusual_select_gap;      // 抓拍一帧异常帧的时间(帧)间隔, 预留
    int                         capture_num_max;         // 最多抓拍张数
    float                       quality_comp_thresh;     // 抓拍效果更好的频分阀值, 0-1
    float                       quality_thresh;          // 抓拍评分最低的阀值, 0-1
    float                       crop_rect_x;             // 抠图外扩比例
    float                       crop_rect_y_up;          // 抠图外扩比例
    float                       crop_rect_y_down;        // 抠图外扩比例
} SAE_FACE_CFG_SELECT_FRAME_PARAM_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_GLASS_THRESHOLD_T
 *  @brief      配置眼镜置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_GLASS_THRESHOLD_T
{
    float                       glass_thresh; 
} SAE_FACE_CFG_FR_ATTR_GLASS_THRESHOLD_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_MASK_THRESHOLD_T
 *  @brief      配置口罩置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_MASK_THRESHOLD_T
{
    float                       mask_thresh;  
} SAE_FACE_CFG_FR_ATTR_MASK_THRESHOLD_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_HAT_THRESHOLD_T
 *  @brief      配置帽子置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_HAT_THRESHOLD_T
{
    float                       hat_thresh;  
} SAE_FACE_CFG_FR_ATTR_HAT_THRESHOLD_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_MANNER_THRESHOLD_T
 *  @brief      配置口罩规范置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_MANNER_THRESHOLD_T
{
    float                       manner_thresh;  
} SAE_FACE_CFG_FR_ATTR_MANNER_THRESHOLD_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_MASK_TYPE_THRESHOLD_T
 *  @brief      配置口罩类型置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_MASK_TYPE_THRESHOLD_T
{
    float                       maskType_thresh;           
} SAE_FACE_CFG_FR_ATTR_MASK_TYPE_THRESHOLD_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_GENDER_THRESHOLD_T
 *  @brief      配置性别置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_GENDER_THRESHOLD_T
{
    float                       gender_thresh;           
} SAE_FACE_CFG_FR_ATTR_GENDER_THRESHOLD_T;

// todo pengbo ADD
/** @struct     SAE_FACE_CFG_FR_ATTR_EXPRESS_THRESHOLD_T
 *  @brief      配置表情置信度阈值
 */
typedef struct _SAE_FACE_CFG_FR_ATTR_EXPRESS_THRESHOLD_T
{
    float                       express_thresh;           
} SAE_FACE_CFG_FR_ATTR_EXPRESS_THRESHOLD_T;

/***********************************************************************************************************************
* 能力集配置结构体
***********************************************************************************************************************/
/** @struct     SAE_FACE_ABILITY_DFR_DETECT_PARAM_T
 *  @brief      DFR_DETECT 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_DETECT_PARAM_T
{
    int  enable;                                // 人脸检测模块使能 [0: 不使能 1:使能]
    int  max_width;                             // 人脸检测接受的最大分辨率，宽
    int  max_height;                            // 人脸检测接受的最大分辨率，高
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
    int  max_face_num;                          // 最大大可检测的人脸数（0~10）预留
} SAE_FACE_ABILITY_DFR_DETECT_PARAM_T;

/** @struct     SAE_FACE_ABILITY_FD_TRACK_PARAM_T
 *  @brief      FD_TRACK 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_FD_TRACK_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    int  max_width;                             // 人脸跟踪接受的最大分辨率，宽
    int  max_height;                            // 人脸跟踪接受的最大分辨率，高
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_FD_TRACK_PARAM_T;

/** @struct     SAE_FACE_ABILITY_FD_QUALITY_PARAM_T
 *  @brief      FD_QUALITY 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_FD_QUALITY_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    int  max_width;                             // 人脸FD评分接受的最大分辨率，宽
    int  max_height;                            // 人脸FD评分接受的最大分辨率，高
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_FD_QUALITY_PARAM_T;

/** @struct     SAE_FACE_ABILITY_DFR_LANDMARK_PARAM_T
 *  @brief      DFR_LANDMARK 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_LANDMARK_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_DFR_LANDMARK_PARAM_T;

/** @struct     SAE_FACE_ABILITY_DFR_QUALITY_PARAM_T
 *  @brief      DFR_QUALITY 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_QUALITY_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_DFR_QUALITY_PARAM_T;

/** @struct     SAE_FACE_ABILITY_DFR_LIVENESS_PARAM_T
 *  @brief      DFR_LIVENESS 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_LIVENESS_PARAM_T
{
    SAE_FACE_LIVENESS_TYPE_E    enable_type;                        // 活体使能类型 参考 SAE_FACE_LIVENESS_TYPE_E 枚举
    int                         max_width;                          // 人脸活体接受的最大分辨率，宽
    int                         max_height;                         // 人脸活体接受的最大分辨率，高
    char                        model_bin[SAE_FACE_MAX_STRING_NUM]; // 模型文件路径
} SAE_FACE_ABILITY_DFR_LIVENESS_PARAM_T;

/** @struct     SAE_FACE_ABILITY_DFR_ATTRIBUTE_PARAM_T
 *  @brief      DFR_ATTRIBUTE 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_ATTRIBUTE_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_DFR_ATTRIBUTE_PARAM_T;

/** @struct     SAE_FACE_ABILITY_DFR_FEATURE_PARAM_T
 *  @brief      DFR_FEATURE 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_FEATURE_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_DFR_FEATURE_PARAM_T;

/** @struct     SAE_FACE_ABILITY_DFR_COMPARE_PARAM_T
 *  @brief      DFR_COMPARE 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_DFR_COMPARE_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
    int  patch_num;
    int  feat_dim;
    int  head_length;
    int  max_feat_num;
    int  top_n;
} SAE_FACE_ABILITY_DFR_COMPARE_PARAM_T;

/** @struct     SAE_FACE_ABILITY_SELECT_FRAME_PARAM_T
 *  @brief      SELECT_FRAME 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_SELECT_FRAME_PARAM_T
{
    int enable;                                 // 模块使能 [0: 不使能 1:使能]
    int max_width;                              // 人脸选帧接受的最大分辨率，宽
    int max_height;                             // 人脸选帧接受的最大分辨率，高
    int sel_type;
    int track_num_max;
    int crop_flag;
    int crop_queue_len;
    int crop_image_w_max;
    int crop_image_h_max;
} SAE_FACE_ABILITY_SELECT_FRAME_PARAM_T;

/** @struct     SAE_FACE_ABILITY_SELECT_FRAME_PARAM_T
 *  @brief      SELECT_FRAME 节点能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_CLS_HELMET_PARAM_T
{
    int  enable;                                // 模块使能 [0: 不使能 1:使能]
    char model_bin[SAE_FACE_MAX_STRING_NUM];    // 模型文件路径
} SAE_FACE_ABILITY_CLS_HELMET_PARAM_T;


/** @struct     SAE_FACE_ABILITY_PARAM_T
 *  @brief      人脸模块能力集结构体
 */
typedef struct _SAE_FACE_ABILITY_PARAM_T
{
    SAE_ABILITY_MODEL_T                    model_buff;
    SAE_BASE_ABILITY_PLATFORM_PARAM_T      platform;
    SAE_FACE_ABILITY_DFR_DETECT_PARAM_T    dfr_detect;
    SAE_FACE_ABILITY_FD_TRACK_PARAM_T      fd_track;
    SAE_FACE_ABILITY_FD_QUALITY_PARAM_T    fd_quality;
    SAE_FACE_ABILITY_DFR_LANDMARK_PARAM_T  dfr_landmark;
    SAE_FACE_ABILITY_DFR_QUALITY_PARAM_T   dfr_quality;
    SAE_FACE_ABILITY_DFR_LIVENESS_PARAM_T  dfr_liveness;
    SAE_FACE_ABILITY_DFR_ATTRIBUTE_PARAM_T dfr_attribute;
    SAE_FACE_ABILITY_DFR_FEATURE_PARAM_T   dfr_feature;
    SAE_FACE_ABILITY_DFR_COMPARE_PARAM_T   dfr_compare;
    SAE_FACE_ABILITY_SELECT_FRAME_PARAM_T  select_frame;
    SAE_FACE_ABILITY_CLS_HELMET_PARAM_T    cls_helmet;
} SAE_FACE_ABILITY_PARAM_T;


/***********************************************************************************************************************
* 输入输出结构体
***********************************************************************************************************************/
/** @struct     SAE_FACE_DFR_DET_T
 *  @brief      FR检测人脸信息
 */
typedef struct _SAE_FACE_FR_DET_T
{
    unsigned int                id;                      // 目标框ID，多人脸时按照左上->右下顺序排列
    SAE_VCA_RECT_F              rect;                    // 目标框，左上角做标的xy+框宽高，范围0~1
    float                       confidence;              // 目标框置信度，范围0~1
    unsigned int                orientation;             // 检测框朝向，范围是0,90,180,270
} SAE_FACE_DFR_DET_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_DET_T
 *  @brief      FD检测模块输出结构体
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_DET_T
{
    SAE_FACE_DFR_DET_T          detect_out[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];   // FD检测输出人脸信息
    int                         detect_face_num[SAE_FACE_MAX_IMG_TYPE];                     // FD检测输出人脸数量
    int                         img_sts[SAE_FACE_MAX_IMG_TYPE];                             // 检测模块分别针对两种图输出的状态码
    int                         module_sts;                                                 // 检测模块总状态码
} SAE_FACE_OUT_DATA_DFR_DET_T;

/** @struct     SAE_FACE_FD_TRK_T
 *  @brief      FD跟踪人脸信息结构体
 */
typedef struct _SAE_FACE_FD_TRK_T
{
    SAE_FACE_DFR_DET_T          face_info;               // 跟踪结果的人脸信息
    unsigned int                valid;                   // 跟踪结果是否有效
} SAE_FACE_FD_TRK_T;

/** @struct     SAE_FACE_SELECT_INFO_T
 *  @brief      FD跟踪人脸信息人脸ID筛选输入信息结构体
 */
typedef struct _SAE_FACE_SELECT_INFO_T
{
    unsigned int                track_face_num[SAE_FACE_MAX_IMG_TYPE];                              // 选择筛选如landmark的人脸数目，每类型图送入数目小于等于3，不使用筛选功能时配置为0
    SAE_FACE_FD_TRK_T           track_info[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_LANDMARK_FACE_NUM];  // 选择的人脸信息结构体数组
} SAE_FACE_SELECT_INFO_T;

/** @struct     SAE_FACE_OUT_DATA_FD_TRK_T
 *  @brief      FD跟踪模块输出结构体
 */
typedef struct _SAE_FACE_OUT_DATA_FD_TRK_T
{
    SAE_FACE_FD_TRK_T           track_out[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];    // FD跟踪输出人脸信息
    int                         track_face_num[SAE_FACE_MAX_IMG_TYPE];
    SAE_FACE_SELECT_INFO_T      select_info;                                                // 筛选后人脸信息，用户可在输出回调中配置筛选结果
    int                         module_sts;                                                 // 跟踪模块总状态码
} SAE_FACE_OUT_DATA_FD_TRK_T;

/** @struct     SAE_FACE_QLTY_T
 *  @brief      人脸评分输出结构体
 */
typedef struct _SAE_FACE_QLTY_T
{
    float               landmark_confidence;            // 特征点置信度0~1.0
    unsigned int        detect_orientation;             // 小姿态为2，大姿态为1，DFR4.0以后版本已弃用
    float               eye_distance;                   // 两眼间距,真实像素值
    float               color_confidence;               // 彩色置信度，范围0~1
    unsigned int        gray_scale;                     // 灰阶数，范围0~255
    float               gray_mean;                      // 灰阶均值，范围0~255
    float               gray_variance;                  // 灰阶均方差，范围0~128
    float               clearity_score;                 // 清晰度评分，范围0~1
    float               pose_pitch;                     // 三维空间欧拉角上下俯仰角pitch，人脸朝上（相机仰视视角）为正，范围-80~80
    float               pose_yaw;                       // 三维空间欧拉角左右偏转角yaw，图片视角人脸朝左为正，范围-120~120
    float               pose_roll;                      // 三维空间欧拉角翻滚角roll，图片视角人脸逆时针旋转为正，范围-60~60
    float               pose_confidence;                // 姿态(pose_pitch、pose_yaw、pose_roll)置信度，范围0~1
    float               frontal_score;                  // 正面程度评分，范围0~1
    float               visible_score;                  // 可见性评分（即不遮挡），范围0~1，0表示完全遮挡，1表示完全不遮挡
    unsigned int        reserved[32];                   // 保留字节，后续扩展评分项
    float               face_score;                     // 人脸评分，范围0~1

    // add
    float               detect_confidence;              // 检测框置信度0~1.0
    unsigned int        invisible_cls;                  // 人脸受遮挡类型，多标签，二进制形式表示, 0b00000000无遮挡（全0）, 0b00000001口罩遮挡, 0b00000010墨镜遮挡, 
                                                        // 0b00000100帽子遮挡, 0b00001000 OSD字符遮挡, 0b00010000头发遮挡, 0b00100000人手遮挡，
                                                        // 0b01000000人头遮挡，0b10000000其它遮挡, ，0b100000000手机遮挡, 0b00000011既有口罩也有墨镜遮挡
    unsigned int        face_cls;                       // 检测到的人脸类型，0表示正常人脸，1表示非人脸
    unsigned int        illumination_cls;               // 光照种类, 0正常光照，1过暗，2过曝
    float               illumination_score;             // 光照评分, 预留
    float               left_eye_open;                  // 左眼睁闭状态，范围0~1，表示张闭程度
    float               right_eye_open;                 // 右眼睁闭状态，范围0~1，表示张闭程度
    float               mouth_open;                     // 嘴巴张闭状态，范围0~1，表示张闭程度
    float               blurring_cls;                   // 模糊类型,预留
    float               face_complete_score;            // 人脸完整程度,特指人脸部位不全情况,独立于有遮挡物的遮挡评分,预留
    float               face_imcomplete_cls;            // 人脸不完整类型，预留;
    float               farlive_confidence;             //?远距离活体，活体置信度, 范围0~1         

    //add 20210812 wjl
    float               skin_visibility;                //皮肤可见度,皮肤未被遮挡的区域占面部皮肤区域面积的比例，等于1-皮肤遮挡率
    float               eyebrow_visibility;             //眉毛可见度, 预留
    float               eye_visibility;                 //眼睛可见度
    float               nose_visibility;                //鼻子可见度
    float               mouth_visibility;               //嘴巴可见度
    float               forehead_visibility;            //额头可见度, 预留

    //add 20211012 liqiang
    float               illumination_intensity;         //光照强度，0最暗，1最亮
    float               illumination_uniformity;        //光照均匀性，0最不均匀，1最均匀

} SAE_FACE_QLTY_T;

/** @struct     SAE_FACE_QLTY_FILTER_T
 *  @brief      人脸质量过滤状态结构体
 */
typedef struct _SAE_FACE_QLTY_FILTER_T
{
    int                         filter;                 // 是否过滤状态
    SAE_FACE_QLTY_FILTER_TYPE_E filter_type;            // 过滤类型， SAE_FACE_QLTY_FILTER_TYPE_E 类型逻辑组合
} SAE_FACE_QLTY_FILTER_T;

/** @struct     SAE_FACE_OUT_DATA_FD_QLTY_T
 *  @brief      FD评分模块输出结构体
 */
typedef struct _SAE_FACE_OUT_DATA_FD_QLTY_T
{
    SAE_FACE_QLTY_T             quality_out[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];      // FD评分输出人脸信息
    SAE_FACE_QLTY_FILTER_T      quality_filter[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];   // FD评分人脸过滤信息,暂未使用
    int                         quality_face_num[SAE_FACE_MAX_IMG_TYPE];
    int                         face_sts[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];         // FD评分模块分别针对两种图输出的状态码
    int                         img_sts[SAE_FACE_MAX_IMG_TYPE];                                 // FD评分模块分别针对两种图输出的状态码
    int                         module_sts;                                                     // FD评分模块状态码
} SAE_FACE_OUT_DATA_FD_QLTY_T;

/** @struct     SAE_FACE_LANDMARK_INFO_T
 *  @brief      人脸关键点信息结构体
 */
typedef struct _SAE_FACE_LANDMARK_INFO_T
{
    unsigned int                number;                                   // 关键点个数
    SAE_VCA_POINT_F             landmarks[SAE_FACE_MAX_LANDMARKS_NUM];    // 人脸关键点坐标信息，xy坐标，范围0~1,前5个关键点顺序 左眼中心，右眼中心，鼻尖，左嘴角，右嘴角
    float                       confidence;                               // 关键点置信度，范围0~1
} SAE_FACE_LANDMARK_INFO_T;

/** @struct     SAE_FACE_FR_FACE_T
 *  @brief      FR人脸信息输出结构体（含关键点）
 */
typedef struct _SAE_FACE_FR_FACE_T
{
    unsigned int                 id;                      // 目标框ID，多人脸时按照坐上->右下顺序排列，返回0表示无人脸
    SAE_VCA_RECT_F               rect;                    // 目标框，左上角做标的xy+框宽高，范围0~1
    float                        confidence;              // 目标框置信度，范围0~1
    unsigned int                 orientation;             // 检测框朝向，范围是0,90,180,270
    SAE_FACE_LANDMARK_INFO_T     landmark_info;           // 人脸关键点位置
} SAE_FACE_FR_FACE_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_LANDMARK_T
 *  @brief      关键点输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_LANDMARK_T
{
    SAE_FACE_FR_FACE_T          face_info[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];        // 支持输出可见光和红外光人脸信息
    int                         landmark_face_num[SAE_FACE_MAX_IMG_TYPE];                       // 关键点输出人脸个数

    int                         face_sts[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];         // 关键点模块分别针对各个人脸输出的状态码
    int                         img_sts[SAE_FACE_MAX_IMG_TYPE];                                 // 关键点模块分别针对两种图输出的状态码
    int                         module_sts;                                                     // 关键点模块总状态码
} SAE_FACE_OUT_DATA_DFR_LANDMARK_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_QLTY_T
 *  @brief      评分输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_QLTY_T
{
    SAE_FACE_QLTY_T             quality_info[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];     // DFR人脸评分信息
    SAE_FACE_QLTY_FILTER_T      quality_filter[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];   // DFR人脸评分过滤信息
    int                         quality_face_num[SAE_FACE_MAX_IMG_TYPE];                        // DFR人脸评分输出人脸个数
    int                         face_sts[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];         // DFR人脸评分模块分别针对各个人脸输出的状态码
    int                         img_sts[SAE_FACE_MAX_IMG_TYPE];                                 // DFR人脸评分模块分别针对两种图输出的状态码
    int                         module_sts;                                                     // DFR人脸评分模块总状态码
} SAE_FACE_OUT_DATA_DFR_QLTY_T;

/** @struct     SAE_FACE_DFR_ATTRIBUTE_CLASS_T
 *  @brief      人脸属性类别结构体
 */
typedef struct _SAE_FACE_DFR_ATTRIBUTE_CLASS_T

{
    float                       confidence[SAE_FACE_DFR_MAX_ATTRIBUTE_CLASS_NUM];     // 所有类别置信度，范围0~1
    int                         predict_label;                                        // 算法预测类别，范围视类别数而定，{-1,0,1,2,...,N-1}表示N
    unsigned int                class_num;                                            // 有效类别个数
} SAE_FACE_DFR_ATTRIBUTE_CLASS_T;

/** @struct     SAE_FACE_DFR_ATTRIBUTE_REGRESS_T
 *  @brief      年龄属性结构体
 */
typedef struct _SAE_FACE_DFR_ATTRIBUTE_REGRESS_T
{
    float                       confidence[SAE_FACE_DFR_MAX_ATTRIBUTE_REGRESS_NUM];      // 年龄置信度，范围0~1
    float                       predict_label;                                          // 算法预测类别，范围视类别数而定，{0,1,2,...,N-1}表示N
    float                       interval;                                               // 浮动区间，以较大可信度的预测区间
} SAE_FACE_DFR_ATTRIBUTE_REGRESS_T;

/** @struct     SAE_FACE_DFR_ATTRIBUTE_INFO_T
 *  @brief      人脸属性(语义)信息结构体   
 */
typedef struct SAE_FACE_DFR_ATTRIBUTE_INFO_T
{
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      glass;                                    // 眼镜，0表示无眼镜，1表示戴眼镜，2表示带墨镜，-1表示未知
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      mask;                                     // 口罩，0表示无口罩，1表示戴口罩，-1表示未知
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      hat;                                      // 帽子，0表示无帽，,1表示戴帽子，-1表示未知
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      reserved1;                                // 保留字节
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      mask_manner;                              // 口罩佩戴规范，0不戴口罩，1口罩遮住鼻子及以下，2口罩遮住嘴巴及以下，3口罩遮住下巴及以下，4口罩悬挂于脖子，-1其它未知  // add by yzl 202206
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      mask_type;                                // 口罩类型， 0不戴口罩，1普通非医用口罩，2普通医用口罩，3不带呼吸阀N95口罩，4带呼吸阀N95口罩，-1其它未知 // add by yzl 202206
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      gender;                                   // 性别，0表示女，1表示男，-1表示未知
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      reserved2;                                // 保留字节
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      reserved3;                                // 保留字节
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      express;                                  // 表情，0表示中性，1高兴，2惊讶, 3表示害，, 4表示厌恶, 5表示难过, 6表示愤怒，-1表示未知
    SAE_FACE_DFR_ATTRIBUTE_REGRESS_T    age;                                      // 年龄，范，0~99，小，1周岁的都认为0岁，大于等于99岁都认为99，
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      age_stage;                                // 年龄段，0 表示儿童 0-6岁， 1表示少年 7-17岁，2表示青年 18-40岁，3表示中年 41-65岁，4表示老年 >=66
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      hair_style;                               // 发型，预留
    SAE_FACE_DFR_ATTRIBUTE_CLASS_T      face_shape;                               // 脸型，预留
    unsigned char                       reserved[64];                             // 保留字节
} SAE_FACE_DFR_ATTRIBUTE_INFO_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_ATTR_T
 *  @brief      属性输出结果结构体, 只针对可见光
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_ATTR_T
{
    SAE_FACE_DFR_ATTRIBUTE_INFO_T   attribute_info[SAE_FACE_MAX_FACE_NUM];  // 属性信息
    int                             attribute_face_num;                     // 属性输出人脸个数
    int                             face_sts[SAE_FACE_MAX_FACE_NUM];        // 属性模块分别针对各个人脸输出的状态码
    int                             module_sts;                                                    // 属性模块总状态码
} SAE_FACE_OUT_DATA_DFR_ATTR_T;

/** @struct     SAE_FACE_OUT_DATA_CALC_BRIGHT_T
 *  @brief      亮度计算输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_CALC_BRIGHT_T
{
    int                         bright_value[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];     // 亮度值
    int                         bright_face_num[SAE_FACE_MAX_IMG_TYPE];                         // 亮度计算人脸个数
    int                         face_sts[SAE_FACE_MAX_IMG_TYPE][SAE_FACE_MAX_FACE_NUM];         // 亮度计算模块分别针对各个人脸输出的状态码
    int                         img_sts[SAE_FACE_MAX_IMG_TYPE];                                 // 亮度计算模块分别针对两种图输出的状态码
    int                         module_sts;                                                     // 亮度计算模块总状态码
} SAE_FACE_OUT_DATA_CALC_BRIGHT_T;

/** @struct     SAE_FACE_MATCH_T
 *  @brief      按照人脸方位匹配后的信息
 */
typedef struct _SAE_FACE_MATCH_T
{
    int                         match_cnt;                             // 匹配成功的人脸数量
    int                         left_idx[SAE_FACE_CALIB_MAX_NUM];      // 左图像匹配成功的下标
    int                         right_idx[SAE_FACE_CALIB_MAX_NUM];     // 右图像匹配成功的下标
    float                       match_conf[SAE_FACE_CALIB_MAX_NUM];    // 匹配的置信度
} SAE_FACE_MATCH_T;

/** @struct     SAE_FACE_OUT_DATA_CALIB_T
 *  @brief      标定输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_CALIB_T
{
    SAE_FACE_MATCH_T            match_info;                                                     // 输出匹配结构体
    SAE_VCA_POINT_F             face_feature[SAE_FACE_CALIB_MAX_NUM][SAE_FACE_CALIB_POINT_NUM]; // 输出投影后的人脸特征
                                                                                                // 输出人脸关键点顺序为（左眼、右眼、鼻子、左嘴、右嘴、人脸框左上角顶点）
    int                         module_sts;                                                     // 校正计算模块状态码
} SAE_FACE_OUT_DATA_CALIB_T;

/** @struct     SAE_FACE_LIVENESS_INFO_T
 *  @brief      活体信息结构体
 */
typedef struct _SAE_FACE_LIVENESS_INFO_T
{
    int                         live_label;              // -1表示不确定，0 表示非活体，1表示活体 
    float                       live_confidence;         // 活体置信度
} SAE_FACE_LIVENESS_INFO_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_LIVENESS_T
 *  @brief      活体输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_LIVENESS_T
{
    SAE_FACE_LIVENESS_INFO_T     liveness_info[SAE_FACE_MAX_FACE_NUM];      // 活体信息
    int                          liveness_face_num;                         // 活体计算人脸个数
    int                          face_sts[SAE_FACE_MAX_FACE_NUM];           // 活体模块分别针对每个人脸的状态码
    int                          module_sts;                                // 算法模块返回码
} SAE_FACE_OUT_DATA_DFR_LIVENESS_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_FEATURE_T
 *  @brief      建模模块输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_FEATURE_T
{
    unsigned char                feat_data[SAE_FACE_MAX_FACE_NUM][SAE_FACE_MAX_FEAT_SIZE];  // 模型数据
    int                          feat_face_num;                                             // 建模计算人脸个数
    unsigned int                 feat_len;                                                  // 模型大小
    int                          face_sts[SAE_FACE_MAX_FACE_NUM];                           // 建模模块分别针对每个人脸的状态码
    int                          module_sts;                                                // 建模模块状态码
} SAE_FACE_OUT_DATA_DFR_FEATURE_T;

/** @struct     SAE_FACE_COMPARE_REPO_INFO_T
 *  @brief      1vN比对底库比对结果信息结构体
 */
typedef struct _SAE_FACE_COMPARE_REPO_INFO_T
{
    int                          enable;                                            // 底库是否使能
    float                        sim_max[SAE_FACE_MAX_COMPARE_TOP_N];               // topK目标相似度
    int                          idx[SAE_FACE_MAX_COMPARE_TOP_N];                   // topK目标位置
    int                          repo_sts;                                          // 1vN比对底库模块返回码
} SAE_FACE_COMPARE_REPO_INFO_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_COMPARE_T
 *  @brief      MvN比对模块输出结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_COMPARE_T
{
    int                          compare_face_num;                                  // 比对数量
    SAE_FACE_COMPARE_REPO_INFO_T compare_repo_info[SAE_FACE_MAX_FACE_NUM];          // 各个特征比对结果
    int                          face_sts[SAE_FACE_MAX_FACE_NUM];                   // 各个特征比对错误码
    int                          module_sts;                                        // 比对模块返回码
} SAE_FACE_OUT_DATA_DFR_COMPARE_T;

/** @struct     SAE_FACE_OUT_DATA_CLS_HELMET_T
 *  @brief      安全帽模块计算输出结果结构体
 */
typedef struct _SAE_FACE_CLS_OUT_DATA_CLS_HELMET_T
{
    float                        wearing_conf;            // 佩戴安全帽的置信度
    float                        not_wearing_conf;        // 未佩戴安全帽的置信度
    int                          module_sts;              // 安全帽模块状态码
} SAE_FACE_OUT_DATA_CLS_HELMET_T;

/** @struct     SAE_FACE_DATA_INFO_T
 *  @brief      输入数据信息结构体
 */
typedef struct _SAE_FACE_DATA_INFO_T
{
    SAE_VCA_YUV_DATA             yuv_data;                // yuv数据，目前人脸检测只支持SAE_VCA_YVU420，即NV12，二维码检测只支持MONO8
    SAE_VCA_RECT_F               roi_rect;                // 输入图像ROI支持（归一化参数）,范围不大于全图，最小长宽不小于32像素
    unsigned int                 frame_num;               // 帧号，从0开始，逐帧加1
    unsigned int                 time_stamp;              // 时戳，采集时戳或解码时戳
    int                         *yuv_use_flag;            // YUV图像释放标志位，在抓拍选帧节点使用，其他节点不使用
    int                          reserved[4];             // 预留字段
} SAE_FACE_DATA_INFO_T;


/** @struct     SAE_FACE_IMG_DESCRIBE_T
 *  @brief      图像描述
 */
typedef struct _SAE_FACE_IMG_DESCRIBE_T
{
    int                         valid;            // 图像是否有效 [0:无效 1:有效]
    SAE_IMG_TYPE_E              img_type;         // 该位置的光源是白光还是红光
    int                         reserved[2];      // 预留字段
} SAE_FACE_IMG_DESCRIBE_T;


/** @struct     SAE_FACE_IMG_TYPE_INFO_T
 *  @brief      图像类型信息描述
 */
typedef struct _SAE_FACE_IMG_TYPE_INFO_T
{
    SAE_IMG_TYPE_E              proc_main_type;                            // 处理的主类型为白光或红光，即指定类型后，跟踪、属性、建模模块使用该图像
    SAE_FACE_IMG_DESCRIBE_T     img_type_describe[SAE_FACE_MAX_IMG_TYPE];  // 图像类型及有效性描述，索引0对应输入的图像索引0和1，索引1对应输入图像的索引2和3
                                                                            // 如只在输入图像的2和3上放红光的大小图，则 img_type_describe[0].valid = 0; img_type_describe[1].valid = 1; 
                                                                            // img_type_describe[1].img_type = SAE_FACE_IMG_TYPE_IR;

    int                         reserved[4];                               // 预留字段
} SAE_FACE_IMG_TYPE_INFO_T;

/** @struct     SAE_FACE_SELECT_FRAME_INFO_T
 *  @brief      抓拍图像中目标信息(目前抓拍模块只支持1图1人脸)
 */
typedef struct _SAE_FACE_SELECT_FRAME_INFO_T
{
    unsigned int                id;                      // 目标框ID，多人脸时按照左上->右下顺序排列
    SAE_VCA_RECT_F              rect_src;                // 原始图目标框，左上角做标的xy+框宽高，范围0~1
    SAE_VCA_RECT_F              rect_crop;               // 抓拍图目标框，左上角做标的xy+框宽高，范围0~1
    float                       confidence;              // 目标框置信度，范围0~1
    SAE_FACE_DATA_INFO_T        image_info_src;          // 原始图信息
    SAE_FACE_DATA_INFO_T        image_info_crop;         // 抓拍图信息
} SAE_FACE_SELECT_FRAME_INFO_T;

/** @struct     SAE_FACE_OUT_DATA_SELECT_FRAME_T
 *  @brief      抓拍选帧模块数据结果结构体
 */
typedef struct _SAE_FACE_OUT_DATA_SELECT_FRAME_T
{
    int                          select_face_num;
    SAE_FACE_SELECT_FRAME_INFO_T select_face_info[SAE_FACE_SELECT_FRAME_QUE_MAX_NUM]; // 源图中人脸的坐标信息
    int                          select_frame_sts;                                    // 抓拍选帧模块状态码
} SAE_FACE_OUT_DATA_SELECT_FRAME_T;

/** @struct     SAE_FACE_DATA_PRIO_T
 *  @brief      输入数据优先级结构体
 */
typedef struct _SAE_FACE_DATA_PRIO_T
{
    SAE_FACE_PROC_PRIO_TYPE_E       det_priority;            // 检测任务优先级
    SAE_FACE_PROC_PRIO_TYPE_E       feat_priority;           // 建模任务优先级
    int                             reserved[2];             // 预留字段
} SAE_FACE_DATA_PRIO_T;

/** @struct     SAE_FACE_LOG_TIME_T
 *  @brief      LOG模块时间数据信息
 */
typedef struct _SAE_FACE_LOG_TIME_T
{
    long long                       dfr_detect;
    long long                       fd_track;
    long long                       fd_quality;
    long long                       dfr_landmark;
    long long                       dfr_quality;
    long long                       dfr_attribute;
    long long                       dfr_liveness;
    long long                       dfr_feature;
    long long                       dfr_compare;
    long long                       cls_helmet;
    long long                       select_frame;
    long long                       third_calc_bright;
    long long                       third_calib;
} SAE_FACE_LOG_TIME_T;

/** @struct     SAE_FACE_LOG_INFO_T
 *  @brief      LOG模块时间数据信息
 */
typedef struct _SAE_FACE_LOG_INFO_T
{
    unsigned int                frame_num;              // 帧号，从0开始，逐帧加1
    unsigned int                top_idx;                // 比对相似度最高底库证件号
    float                       top_sim;                // 比对相似度（top1）
    float                       sim_thresh;             // 相似度阈值
    float                       live_conf;              // 活体置信度
    float                       live_thresh;            // 活体置信度阈值
    unsigned int                pass_flag;              // 比对结果（通过/未通过）
    float                       pose_pitch;             // 人脸上下姿态
    float                       pose_yaw;               // 人脸左右姿态
    float                       clearity_score;         // 人脸清晰度评分
    float                       visible_score;          // 人脸遮挡评分    
    float                       det_conf;               // 人脸检测置信度
    float                       det_thresh;             // 人脸检测置信度阈值
    int                         face_mask_result;       // 口罩检测结果
    float                       face_mask_conf;         // 戴口罩置信度
    int                         reserved[1];            // 预留到64字节
} SAE_FACE_LOG_INFO_T;

/** @struct     SAE_FACE_LOG_DATA_T
 *  @brief      LOG数据信息
 */
typedef struct _SAE_FACE_LOG_DATA_T
{
    SAE_FACE_LOG_TIME_T             module_time;
    SAE_FACE_LOG_INFO_T             module_info;
} SAE_FACE_LOG_DATA_T;

/** @struct     SAE_FACE_IN_DATA_INPUT_T
 *  @brief      SAE引擎人脸输入结构体
 */
typedef struct _SAE_FACE_IN_DATA_INPUT_T
{
    SAE_FACE_PROC_TYPE_E            proc_type;                          // 人脸引擎处理类型
    SAE_FACE_FACE_PROC_TYPE_E       face_proc_type;                     // 人脸处理类型
    SAE_FACE_ERROR_PROC_TYPE_E      error_proc_type;                    // 错误处理类型
    SAE_FACE_LIVENESS_TYPE_E        liveness_type;                      // 活体算法类型标志，决定活体算法种类（不使能，可见光，红外光）
    unsigned int                    attribute_enable;                   // 人脸属性使能开关（0关闭，1打开）
    unsigned int                    ls_img_enable;                      // 大小图使能开关（0关闭，1打开）
    unsigned int                    track_enable;                       // 跟踪使能开关（0关闭，1打开）
    unsigned int                    compare_enable;                     // 比对使能开关（0关闭，1打开）----识别流程中如果需要比对，则打开，否则关闭
    SAE_FACE_DATA_PRIO_T            data_priority;                      // 输入数据处理优先级，包含检测和建模任务两个优先级设置
    SAE_FACE_DATA_INFO_T            data_info[SAE_FACE_MAX_INPUT_NUM];  // 输入（可见光、红外光）大小图输入时，第一帧是小图，第二帧是大图，
                                                                        // 双光输入时，0,1 或 2,3 上为同一种光源，光源类型在img_type_info中指定定
    SAE_FACE_IMG_TYPE_INFO_T        img_type_info;                      // 1. 指定data_info中索引0,1或2,3上的光源类型以及有效性
                                                                        // 2. 指定主处理光源，以确定双光输入时，跟踪、属性和建模使用主处理光源
    SAE_FACE_FR_FACE_T              face_param[SAE_FACE_MAX_FACE_NUM];  // 人脸信息     用于 SAE_FACE_PROC_TYPE_FEAT_LIVENESS 流程 、
    int                             face_num;                           // 人脸数量     SAE_FACE_PROC_TYPE_LANDMARK_FEATURE、 SAE_FACE_PROC_TYPE_DM_LANDMARK_FEATURE 流程
    SAE_FACE_LOG_DATA_T             log_data;                           // LOG数据信息
    void                           *priv_data;                          // 用户预留
    int                             priv_data_size;                     // priv_data指向的对象大小
    int                             reserved[16];                       // 预留字段
} SAE_FACE_IN_DATA_INPUT_T;

/** @struct     SAE_FACE_OUT_DATA_DFR_COMPARE_1V1_T
 *  @brief      1V1比对数据结果
 */
typedef struct _SAE_FACE_OUT_DATA_DFR_COMPARE_1V1_T
{
    float                        sim_max;                 // top1目标相似度
    int                          compare_sts;             // 1V1比对模块状态码
} SAE_FACE_OUT_DATA_DFR_COMPARE_1V1_T;

/** @struct     SAE_FACE_IN_DATA_FEATCMP_1V1_T
 *  @brief      SAE引擎1V1比对输入结构体
 */
typedef struct _SAE_FACE_IN_DATA_FEATCMP_1V1_T
{
    void                        *feat1;                   // 特征1
    void                        *feat2;                   // 特征2
    int                          feat_len;                // 特征长度

    void                        *priv_data;               // 用户预留
    int                          priv_data_size;          // priv_data指向的对象大小
    int                          reserved[16];            // 预留字段
} SAE_FACE_IN_DATA_FEATCMP_1V1_T;

/** @struct     SAE_FACE_IN_DATA_FEATCMP_1VN_T
 *  @brief      SAE引擎1VN比对输入结构体
 */
typedef struct _SAE_FACE_IN_DATA_FEATCMP_1VN_T
{
    void                        *feat_addr;        // 待比较特征数据地址
    int                          feat_repo_num;    // 比对库最大数量
    void                        *feat_repo_addr;   // 比对库数据地址
    int                          feat_repo_stride; // 比对库stride
} SAE_FACE_IN_DATA_FEATCMP_1VN_T;

/** @struct     SAE_FACE_FEATURE_VERSION_INRO_T
 *  @brief      SAE引擎特征信息头（16字节）
 */
typedef struct _SAE_FACE_FEATURE_VERSION_INRO_T
{
    char feat_version[16];
} SAE_FACE_FEATURE_VERSION_INRO_T;

/** @fn       SAE_FACE_FeatMutex
 *  @brief    人脸特征库互斥锁注册函数
 *  @return   int
 */
typedef int (*SAE_FACE_FeatMutex)(void);

/** @fn       SAE_FACE_CheckFeature
 *  @brief    特征校验接口(后续版本将废弃，建议迁移新接口)
 *  @param    feat_data             [in]   - 输入图像灰度数据
 *  @param    feat_size             [in]   - 输入人脸信息
 *  @return   int
 */
int SAE_FACE_CheckFeature(unsigned char *feat_data, unsigned int feat_size);

/** @fn       SAE_FACE_FeatCmp1v1
 *  @brief    1v1特征比对接口(后续版本将废弃，建议迁移新接口)
 *  @param    in_data               [in]   - 输入图像灰度数据
 *  @param    in_size               [in]   - 输入人脸信息
 *  @param    sim                   [out]  - 输出亮度计算结果
 *  @return   int
 */
int SAE_FACE_FeatCmp1v1(SAE_FACE_IN_DATA_FEATCMP_1V1_T *in_data, int in_size, float *sim);

/** @fn       SAE_FACE_Feature_GetVersion
 *  @brief    获取算法建模特征头信息(后续版本将废弃，建议迁移新接口)
 *  @param    pInitHandle               [in]          - ICF初始化句柄
 *  @param    feature_version_info      [out]         - 输出的特征信息头结构体
 *  @return   状态码
 *  @note
 */
int SAE_FACE_Feature_GetVersion(SAE_FACE_FEATURE_VERSION_INRO_T *feature_version_info);

/** @fn       SAE_FACE_DFR_Compare_Extern_Create
 *  @brief    创建外部compare句柄
 *  @param    pInitHandle               [in]            - ICF句柄
 *  @param    pMemPool                  [in]            - 内存池句柄
 *  @param    pAbility                  [in]            - COMPARE能力集
 *  @param    pCreateHandle             [out]           - 待创建句柄
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_Create( void                       *pInitHandle,
                                        void                       *pMemPool,
                                        SAE_FACE_ABILITY_PARAM_T   *pAbility,
                                        void                      **pCreateHandle);

/** @fn       SAE_FACE_DFR_Compare_Extern_Destroy
 *  @brief    销毁外部compare句柄
 *  @param    pCreateHandle             [in]            - 外部compare句柄
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_Destroy(void *pCreateHandle);

/** @fn       SAE_FACE_DFR_Compare_Extern_Update_Repo
 *  @brief    导入底库
 *  @param    pCreateHandle             [in]            - 外部compare句柄
 *  @param    pFeatRepo                 [in]            - 底库信息结构体
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_Update_Repo(void *pCreateHandle, SAE_FACE_CFG_FEATREPO_INFO_T *pFeatRepo);

/** @fn       SAE_FACE_DFR_Compare_Extern_1vN
 *  @brief    销毁外部compare句柄
 *  @param    pCreateHandle             [in]            - 外部compare句柄
 *  @param    pInData1vN                [in]            - 1vN输入参数
 *  @param    pOutData1vN               [out]           - 1vN输出参数
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_1vN(void                           *pCreateHandle,
                                    SAE_FACE_IN_DATA_FEATCMP_1VN_T *pInData1vN,
                                    SAE_FACE_COMPARE_REPO_INFO_T   *pOutData1vN);

/** @fn       SAE_FACE_DFR_Compare_Extern_1v1
 *  @brief    获取参数
 *  @param    pCreateHandle      [in]         - 外部compare句柄
 *  @param    in_data            [in]         - 输入结构体指针
 *  @param    in_size            [in]         - 输入结构体大小
 *  @param    sim                [out]        - 相似度
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_1v1(void                              *pCreateHandle,
                                    SAE_FACE_IN_DATA_FEATCMP_1V1_T    *in_data, 
                                    int                                in_size, 
                                    float                             *sim);

/** @fn       SAE_DFR_Compare_Extern_CheckFeature
 *  @brief    获取参数
 *  @param    pCreateHandle      [in]         - 外部compare句柄
 *  @param    feat_data          [in]         - 特征数据
 *  @param    feat_size          [in]         - 特征数据大小
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_CheckFeature(void            *pCreateHandle,
                                             unsigned char   *feat_data,
                                             unsigned int     feat_size);

/** @fn       SAE_FACE_DFR_Compare_Extern_GetVersion
 *  @brief    获取算法建模特征头信息
 *  @param    pCreateHandle             [in]          - 外部compare句柄
 *  @param    feature_version_info      [out]         - 输出的特征信息头结构体
 *  @return   状态码
 *  @note
 */
int SAE_FACE_DFR_Compare_Extern_GetVersion(void                            *pCreateHandle,
                                           SAE_FACE_FEATURE_VERSION_INRO_T *feature_version_info);

/** @fn       SAE_FACE_CalibProcess
 *  @brief    门禁使用标定算法注册函数
 *  @param    in              [in]   - 输入数据指针
 *  @param    out             [out]  - 输出数据指针
 *  @return   int
 */
typedef int (*SAE_FACE_CalibProcess)(void *in, void *out);

/** @fn       SAE_FACE_CalcBrightness
 *  @brief    门禁使用亮度算法注册函数
 *  @param    y_bright              [in]   - 输入图像灰度数据
 *  @param    faceInfo              [in]   - 输入人脸信息
 *  @param    brightness            [out]  - 输出亮度计算结果
 *  @return   int
 */
typedef int (*SAE_FACE_CalcBrightness)(void *y_bright, SAE_FACE_FR_FACE_T *faceInfo, int *brightness);

/** @fn       SAE_FACE_Release_Cur_Sel_Frame
 *  @brief    外部接口释放当前帧抠图缓存
 *  @param    user_out              [in]   - 图像中选帧信息
 *  @return   int
 */
int SAE_FACE_Release_Cur_Sel_Frame(SAE_FACE_OUT_DATA_SELECT_FRAME_T *sel_out);



#ifdef __cplusplus
}
#endif

#endif

