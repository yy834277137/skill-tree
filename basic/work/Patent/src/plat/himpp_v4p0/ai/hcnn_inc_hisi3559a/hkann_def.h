/******************************************************************************
* 
* 版权信息：版权所有 (c) 201X, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：hkann_def.h
* 文件标示：__HKANN_DEF_H__
* 摘    要： HKANN的结构体定义
* 
* 当前版本：1.0
* 作    者：秦川6
* 日    期：2020-08-05
* 备    注：
******************************************************************************/

#ifndef __HKANN_DEF_H__
#define __HKANN_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "vca_base.h"

/***********************************************************************************************************************
* HKANN ARM 私有结构体定义
***********************************************************************************************************************/
#include "hkann_arm.h"
#include "hcnn_define.h"


/***********************************************************************************************************************
* HKANN 结构体定义 (从hkann_lib.h复制, 合并了arm版本的修改)
***********************************************************************************************************************/

#define HKANN_MEM_TAB_NUM           8           // 内存块数目
#define HKANN_HANDLE_NUM            8           // 外部传入最大句柄数
#define HKANN_BLOB_MAX_DIM          8           // blob的最大维度
#define HKANN_MAX_IN_BLOB           8           // process时输入最大blob数
#define HKANN_MAX_OUT_BLOB          32          // process时输出最大blob数
#define HKANN_ENUM_END              0xFFFFFFF   // 用于对齐



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
    float               fl_out_scale;                   //定点转浮点系数
    char                res[4];                         // 保留字段
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
    HKANN_APT_CNN_MODEL_HANDLE,           // HIK_APT_CNN封装的模型句柄
    HKANN_APT_CNN_SCHED_HANDLE,           // HIK_APT_CNN_SCHED封装的模型句柄
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
    char                        is_fix_out;                         // 输出是否指定为定点输出
    char                        res[15];                            // 预留字段
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









#ifdef __cplusplus
}
#endif
#endif //__HKANN_DEF_H__