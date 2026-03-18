/** @file       alg_type_base.h
 *  @note       HangZhou Hikvision Digital Technology Co., Ltd.
 *              All right reserved
 *  @brief      算子层接口头文件
 *  @note   
 
 *  @author     chenpeng43
 *  @version    v3.0.1
 *  @date       2021/09/29
 *  @note       增加算子类型定义，从ICF_type.h迁移过来

 *  @author     chenpeng43
 *  @version    v3.0.0
 *  @date       2021/09/07
 *  @note       重构引擎，算法升级至DFR v6.0
 *
 *  @author     guixinzhe
 *  @version    v2.1.0
 *  @date       2020/12/03
 *  @note       支持 NT52X 平台
 * 
 *  @author     guixinzhe
 *  @version    v2.0.0
 *  @date       2020/07/22
 *  @note       重构
 * 
 *  @author     wangxiao
 *  @version    v1.3.4
 *  @date       2020/05/27
 *  @note       新建
 */

#ifndef __ALG_TYPE_BASE__
#define __ALG_TYPE_BASE__

#include "ICF_MemPool.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
// 应用版本号
#define SAE_MAJOR_VERSION           (3)                     // 主版本号
#define SAE_SUB_VERSION             (1)                     // 副版本号
#define SAE_REVISION_VERSION        (0)                     // 修订版本号

#define SAE_DATE_YEAR               (23)                    // 版本日期，年
#define SAE_DATE_MONTH              (5)                     // 版本日期，月
#define SAE_DATE_DAY                (15)                     // 版本日期，日

#define SAE_ENUM_BASE               (0x00001000)            // 定义枚举基地址
#define SAE_ENUM_END                (0xFFFFFFFF)            // 定义枚举结束值,用于对齐
/***********************************************************************************************************************
* 枚举
***********************************************************************************************************************/
/** @enum       SAE_IRIS_GTYPE_E
 *  @brief      业务功能 graph type 的枚举类型
 */
typedef enum _SAE_IRIS_GTYPE_E
{
    SAE_GTYPE_FACE = 1,
    SAE_GTYPE_IDR  = 2
} SAE_GTYPE_E;

/** @enum       SAE_PLATFORM_TYPE_E
 *  @brief      核心id
 */
typedef enum _SAE_PLATFORM_TYPE_E
{
    SAE_PLATFORM_TYPE_NT_535     = 0, // NT_535
    SAE_PLATFORM_TYPE_NT_687     = 1, // NT_687
    SAE_PLATFORM_TYPE_NT_525_528 = 2, // NT_525_528
    SAE_PLATFORM_TYPE_NT_520     = 3, // NT_520
    SAE_PLATFORM_TYPE_RK_3588    = 4, // RK_3588  
    SAE_PLATFORM_TYPE_NT_560     = 5, // NT_560
} SAE_PLATFORM_TYPE_E;


/** @enum       SAE_FACE_IMG_TYPE_E
 *  @brief      人脸图像类型或光源类型
 */
typedef enum _SAE_IMG_TYPE_E
{
    SAE_FACE_IMG_TYPE_RGB         = SAE_ENUM_BASE + 0,            // 人脸图像可见光，简称白光
    SAE_FACE_IMG_TYPE_IR          = SAE_ENUM_BASE + 1,            // 人脸图像红外光，简称红光

    SAE_TYPE_INVALID     = SAE_ENUM_END
} SAE_IMG_TYPE_E;


/** @enum       SAE_ANA_DATA_TYPE_E
 *  @brief      SAE 算子类型
 */
typedef enum _SAE_ANA_DATA_TYPE_E
{
    // SAE 算子类别
    ICF_ANA_DFR_DETECT_DATA         = 0x10000001,  // 检测
    ICF_ANA_FD_TRACK_DATA           = 0x10000002,  // FD追踪
    ICF_ANA_FD_QUALITY_DATA         = 0x10000003,  // FD评分
    ICF_ANA_DFR_LANDMARK_DATA       = 0x10000004,  // DFR关键点
    ICF_ANA_DFR_QUALITY_DATA        = 0x10000005,  // DFR评分 评分校验
    ICF_ANA_DFR_LIVENESS_DATA       = 0x10000006,  // DFR活体
    ICF_ANA_DFR_FEATURE_DATA        = 0x10000007,  // DFR特征
    ICF_ANA_DFR_COMPARE_DATA        = 0x10000008,  // DFR比对
    ICF_ANA_NIA_YUV2RGB_DATA        = 0x10000009,  // NIA yuv2rgb
    ICF_ANA_3RDPARTY_CALIB_DATA     = 0x1000000a,  // 第三方标定算子
    ICF_ANA_3RDPARTY_BRIGHT_DATA    = 0x1000000b,  // 第三方亮度算子
    ICF_ANA_DETECT_FLOW_CTRL        = 0x1000000c,  // 检测流控算子
    ICF_ANA_FEATURE_FLOW_CTRL       = 0x1000000d,  // 建模流控算子
    ICF_ANA_DFR_DETECT2_DATA        = 0x1000000e,  // FD检测2
    ICF_ANA_DFR_LANDMARK2_DATA      = 0x1000000f,  // FR关键点2
    ICF_ANA_DFR_QUALITY2_DATA       = 0x10000010,  // FR评分2
    ICF_ANA_DFR_FEATURE2_DATA       = 0x10000011,  // FR特征2
    ICF_ANA_CLS_HELMET_DATA         = 0x10000012,  // CLA 安全帽
    ICF_ANA_SELECT_FRAME_DATA       = 0x10000013,  // 选帧模块
    ICF_ANA_DFR_ATTRIBUTE_DATA      = 0x10000014,  // 属性模块
    ICF_ANA_DFR_COMPARE_1V1_DATA    = 0X10000015,  // DFR 1v1比对模块

    ICF_ANA_IDR_DETLOC_DATA         = 0x10000020,  // 二维码检测模块
    ICF_ANA_IDR_REC_DATA            = 0x10000021,  // 二维码识别模块

} SAE_ANA_DATA_TYPE_E;

/***********************************************************************************************************************
* 结构体
***********************************************************************************************************************/
/** @struct     SAE_BASE_ABILITY_PLATFORM_PARAM_T
 *  @brief      引擎平台参数结构体
 */
typedef struct _SAE_BASE_ABILITY_PLATFORM_PARAM_T
{
    // 平台类型，"0---535，1---687，2---525，3---520，4---RK3568
    // 目前只有525/520平台需要该选项，因为需要在引擎层面进行区分核心绑定操作，其他平台配置无效"
    SAE_PLATFORM_TYPE_E          type;
} SAE_BASE_ABILITY_PLATFORM_PARAM_T;

/** @struct     SAE_PKG_VERSION_T
 *  @brief      算法库版本信息结构体
 */
typedef struct _SAE_PKG_VERSION_T_
{  
    char                         algo_name[64];           // 算法库名称
    unsigned int                 major_version;           // 算法库主版本号
    unsigned int                 minor_version;           // 算法库子版本号
    unsigned int                 revis_version;           // 算法库修正版本号
    char                         plat_name[64];           // 算法库运行平台
    char                         sys_info[64];            // 算法库适配系统
    char                         accuracy[32];            // 算法库计算精度
    char                         encryption[32];          // 算法库加密方式
    char                         build_time[32];          // 算法库编译日期
    char                         version_properties[32];  // 算法库版本属性
} SAE_PKG_VERSION_T;

/** @struct     SAE_APP_VERSION_T
 *  @brief      引擎应用版本信息
 */
typedef struct _SAE_APP_VERSION_T
{
    unsigned int                 major_version;           // 主版本号
    unsigned int                 minor_version;           // 子版本号
    unsigned int                 revis_version;           // 修订版本号

    unsigned int                 version_year;            // 版本日期-年
    unsigned int                 version_month;           // 版本日期-月
    unsigned int                 version_day;             // 版本日期-日
} SAE_APP_VERSION_T;

/** @struct     SAE_VERSION_T
 *  @brief      引擎版本信息
 */
typedef struct _SAE_VERSION_T
{
    SAE_APP_VERSION_T        sae_app_version;         // 引擎应用版本信息
    SAE_PKG_VERSION_T        dfr_pkg_version;         // DFR算法包名信息
    SAE_PKG_VERSION_T        dfir_pkg_version;        // DFIR算法包名信息，暂未填充
    SAE_PKG_VERSION_T        cls_pkg_version;         // CLS相关算法包名信息，暂未填充
    SAE_PKG_VERSION_T        idr_pkg_version;         // IDR算法包名信息，暂未填充
} SAE_VERSION_T;

/** @struct     SAE_ABILITY_PARAM_T
 *  @brief      模型缓存结构体
 */
typedef struct _SAE_ABILITY_MODEL_T
{
    unsigned int        nSize;         // 外部模型缓存大小
    char               *pData;         // 外部模型数据缓存地址
} SAE_ABILITY_MODEL_T;

/** @fn       SAE_GetVersion
 *  @brief    引擎版本信息获取
 *  @param    version_info        [out]       - 版本信息指针
 *  @param    version_size        [in]        - 版本信息大小
 *  @return   状态码
 *  @note
 */
int SAE_GetVersion(void *pVersionParam,
                   int   nSize);

/** @fn     int MemPoolObjInit(void *pInitHandle, ICF_MEM_CONFIG *pMemPoolConfig, void **pMemPool)
*   @brief  内存池初始化对外接口
*   @param  pMemPoolConfig      [in]      - 内存池初始化参数
*   @return int
*/
int MemPoolObjInit_V2(void *pInitHandle, ICF_MEM_CONFIG *pMemPoolConfig, void **pMemPool);

/** @fn     int MemPoolObjFInit()
*   @brief  内存池销毁对外接口
*   @param  pInitHandle        [in]     - 引擎句柄
*   @return int
*/
int MemPoolObjFinit_V2(void *pMemPool);

#ifdef __cplusplus 
}
#endif

#endif

