/***********************************************************************************************************************
* 版权信息：版权所有 (c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hkann.h
* 摘    要：cnn统一接口定义，包含GPU/x86, ma, 海思， arm， nt等平台
*
* 当前版本：1.0.0
* 作    者：李鹏飞yf2
* 日    期：2018-12-04
* 备    注：初始版本
***********************************************************************************************************************/

#ifndef _HKANN_ARM_LIB_H__
#define _HKANN_ARM_LIB_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "vca_base.h"


#if 0
#define HKANN_MEM_TAB_NUM           8           // 内存块数目
#define HKANN_HANDLE_NUM            8           // 外部传入最大句柄数
#define HKANN_BLOB_MAX_DIM          8           // blob的最大维度
#define HKANN_MAX_IN_BLOB           8           // process时输入最大blob数
#define HKANN_MAX_OUT_BLOB          32          // process时输出最大blob数
#define HKANN_ENUM_END             0xFFFFFFF    // 用于对其





// nchw 分别对应 shape 0， 1， 2， 3，
typedef enum _HKANN_LAYOUT_FORMAT_
{
    HKANN_FORMAT_NCHW = 0,              // nchw 格式
    HKANN_FORMAT_NHCW,                  // nhcw 格式
    HKANN_FORMAT_NHWC,                  // nhwc 格式
    HKANN_FORMAT_NCLHW,                 // nclhw 格式
    HKANN_FORMAT_NHCLW,                 // nhclw 格式
    HKANN_FORMAT_NUM = HKANN_ENUM_END
}HKANN_LAYOUT_FORMAT;

// 数据blob
typedef struct _HKANN_BLOB_
{
    int                 dim;                            // 维度
    int                 stride[HKANN_BLOB_MAX_DIM];     // 各个维度的stride
    int                 shape[HKANN_BLOB_MAX_DIM];      // 维度大小
    HKANN_LAYOUT_FORMAT format;                         // 数据格式
    VCA_DATA_TYPE       type;                           // 数据类型
    VCA_MEM_SPACE       space;                          // 数据内存类型
    void                *data;                          // 数据指针
    char                res[8];                         // 保留字段
} HKANN_BLOB;



//  输入blob 信息
typedef struct _HKANN_IN_BLOB_INFO_
{
    HKANN_BLOB              src_blob;           // 输入的blob   
    VCA_YUV_FORMAT          src_format;         // 输入数据格式（BGR\RGB\GRAY\NV12\YV12\I420）
    int                     valid_roi;          // 是否使用roi
    VCA_RECT_F              roi;                // 分析的roi区域
    char                    res[32];            // 保留字段，如果可以作为其他用途，例如cnn 用于转换数据
} HKANN_IN_BLOB_INFO;

//  输出blob 信息
typedef struct _HKANN_OUT_BLOB_INFO_
{
    int layer_idx;  // 输出的blob数据所在的层，可为负数，表示倒数，最后一层为-1
    int blob_idx;   // 输出的blob数据的序号（如果该层只有一个输出，则为0，有多个输出，可用序号选择）
} HKANN_OUT_BLOB_INFO;


// 句柄类型
typedef enum _HKANN_HANDLE_TYPE_
{
    HKANN_MODEL_HANDLE  = 0,            // 模型句柄
    HKANN_SCHD_HANDLE,                  // 调度句柄
    HKANN_LAYER_HANDLE,                 // 增加 自定义层 工厂句柄 类型
    HKANN_AUX_MEM_HANDLE,               // 辅存句柄
    HKANN_HANDLE_TYPE_NUM = HKANN_ENUM_END
}HKANN_HANDLE_TYPE;

/** @struct  _HKANN_HANDLE_
 * 	@brief	句柄
 *	@note
 */
typedef struct _HKANN_HANDLE_
{
    HKANN_HANDLE_TYPE   type;       // 句柄类型
    void               *handle;    // 句柄
}HKANN_HANDLE;


// 创建参数
typedef struct _HKANN_PARAM_
{
    int                         in_blob_num;                        // 输入blob数
    HKANN_IN_BLOB_INFO          in_blob_param[HKANN_MAX_IN_BLOB];   // blob信息
    int                         out_blob_num;                       // 输出blob数
    HKANN_OUT_BLOB_INFO         out_blob_info[HKANN_MAX_OUT_BLOB];  // 输出blobk信息

    int                         handle_num;                         // 句柄数
    HKANN_HANDLE                handle[HKANN_HANDLE_NUM];           // 句柄

    void                        *private_param;                     // 私有字段，cnn 是HKANN_CNN_PRIVATE_PARAM
    char                        res[16];                            // 预留字段
}HKANN_PARAM;


// 处理类型
typedef enum _HKANN_PROCTYPE_
{
    HKANN_PROC_TYPE_FORWARD,  /// 前向推理
    HKANN_PROC_TYPE_NUM = HKANN_ENUM_END
} HKANN_PROCTYPE;

// process 输出参数(forward)
typedef struct _HKANN_FORWARD_IN_INFO_
{
    // 注意：如果输入多个blob，而且要减去均值，则减均值的blob只能放在第0个，
    int                       in_blob_num;                 // blob 数
    HKANN_IN_BLOB_INFO        in_blob[HKANN_MAX_IN_BLOB];  // blob 信息

    // forward执行范围: [0, end_layer_idx]，注意其中包含end_layer_idx
    // 值可以为负数，表示倒数，最后一层为-1
    // 要执行整个网络，可将end_layer_idx = -1
    int      end_layer_idx;
    char     res[64];
} HKANN_FORWARD_IN_INFO;



// process 输出参数(forward)
typedef struct _HKANN_FORWARD_OUT_INFO_
{
    int         blob_num;                       // 输出个数
    HKANN_BLOB output_blob[HKANN_MAX_OUT_BLOB]; // 输出blbo， 需要填充输出所需要数据类型 
    char     res[64];
} HKANN_FORWARD_OUT_INFO;


#endif

/***********************************************************************************************************************
* 接口函数
***********************************************************************************************************************/
/***********************************************************************************************************************
* 功  能：获取模型所需的内存
* 参  数： params_info -I     模型相关参数信息
*         mem_tab     -O      内存申请表
* 返回值：状态码
***********************************************************************************************************************/
int HKANN_ARM_GetModelMemSize(VCA_MODEL_INFO * params_info, VCA_MEM_TAB_V3 mem_tab[HKANN_MEM_TAB_NUM]);




/***************************************************************************************************
* 功  能: 创建模型
* 参  数: params_info            - I 模型相关参数信息
*         mem_tab                - I 内存申请表
*         handle                 - O 智能库句柄
* 返回值: 状态码
***************************************************************************************************/
int HKANN_ARM_CreateModel(VCA_MODEL_INFO     *params_info,
                          VCA_MEM_TAB_V3 mem_tab[HKANN_MEM_TAB_NUM],
                          void           **handle);


/***************************************************************************************************
* 功  能: 获取智能库库所需内存大小
* 参  数: params_info            - I 参数信息
*         mem_tab                - O 内存申请表
* 返回值: 返回状态码
***************************************************************************************************/
int HKANN_ARM_GetMemSize(HKANN_PARAM *params_info,
                        VCA_MEM_TAB_V3 mem_tab[HKANN_MEM_TAB_NUM]);


/***************************************************************************************************
* 功  能: 创建智能库内存
* 参  数: params_info            - I 参数信息
*         mem_tab                - I 内存申请表
*         handle                 - O 智能库句柄
* 返回值: 状态码
***************************************************************************************************/
int HKANN_ARM_Create(HKANN_PARAM *params_info,
                   VCA_MEM_TAB_V3 mem_tab[HKANN_MEM_TAB_NUM],
                   void **handle);

/*************************************************************************************************
* 功  能：设置智能库配置信息
* 参  数：handle                  - I      智能库句柄
*         cmd                     - I      设置类型
*         cfg                     - I      指向外部输入配置信息的指针
*         cfg_size                - I      配置信息占用内存大小
* 返回值：返回错误码
***************************************************************************************************/
int HKANN_ARM_SetConfig(void *handle,
                    int  cmd,
                    void *cfg,
                    int   cfg_size);

/*************************************************************************************************
* 功  能：获取智能库配置信息
* 参  数：handle                  - I      智能库句柄
*         cmd                     - I      设置类型
*         cfg                     - O      指向外部输入配置信息的指针
*         cfg_size                - I      配置信息占用内存大小
* 返回值：返回错误码
***************************************************************************************************/
int HKANN_ARM_GetConfig(void *handle,
                        int  cmd,
                        void *cfg,
                        int   cfg_size);

/***************************************************************************************************
* 功  能: 智能库主调用
* 参  数: handle                 - I 算法库句柄
*         proc_type              - I 处理类型
*         in_buf                 - I 算法库输入缓存
*         in_buf_size            - I 输入缓存大小
*         out_buf                - O 算法库输出缓存
*         out_buf_size           - I 输出缓存大小
* 返回值: 返回错误码
***************************************************************************************************/
int HKANN_ARM_Process(void *handle,
                    int   proc_type,
                    void *in_buf,
                    int   in_buf_size,
                    void *out_buf,
                    int   out_buf_size);



/***************************************************************************************************
* 功  能: 释放算法库
* 参  数: handle                 - I 算法库句柄
* 返回值: 返回错误码
***************************************************************************************************/
int HKANN_ARM_Release(void *handle);


/***************************************************************************************************
* 功  能: 释放模型
* 参  数: handle                 - I 算法库句柄
* 返回值: 返回错误码
***************************************************************************************************/
int HKANN_ARM_ReleaseModel(void *handle);


/***************************************************************************************************
* 功  能: 获取版本号
* 参  数: 无
* 返回值: 版本号
***************************************************************************************************/
int HKANN_ARM_GetVersion();



#ifdef __cplusplus
}
#endif

#endif

