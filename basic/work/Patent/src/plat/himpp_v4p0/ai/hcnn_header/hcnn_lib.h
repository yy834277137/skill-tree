/***********************************************************************************************************************
* 版权信息：版权所有 (c) 2010-2027, 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hcnn_lib.h
* 摘    要：海思3559平台HCNN接口定义
*
* 当前版本：2.11.1
* 作    者：张锑
* 日    期：2022/02/18
* 备    注：
* （1）NNIE：
* 1、支持AutoassignPostProc层，并支持跳过sigmoid操作。
* 2、Nms层支持max_per_img参数
*
* 历史版本：2.11.0
* 作    者：张锑
* 日    期：2022/01/07
* 备    注：
* （1）NNIE：
* 1、支持GFLProposal层
* 2、支持BatchNms层
* 3、支持FrcnProposaliBoxOnnx层
* 4、支持BboxesSelect层
* 5、支持input数据输入非第一个nnie段
* （2）HIA：
* 1、新增PC_LV_CORD_TRANS点云坐标转换算子源数据拷贝，且支持各帧拥有不同点数
*
* 历史版本：2.10.1
* 作    者：张锑
* 日    期：2021/12/03
* 备    注：
* （1）NNIE：
* 1、支持YoloLprPostProc层
* （2）HIA：
* 1、点云坐标转换的结果导出由只导出前三个浮点更新为全部导出
*
* 历史版本：2.10.0
* 作    者：张锑
* 日    期：2021/10/15
* 备    注：
* （1）版本号格式更新
* （2）NNIE：
* 1、ROIAlign层优化
* 2、CollectAndDistributeFpnRpnProposal层支持类别筛选
* 3、CsegPostproc层支持指定类别分割
* 4、增加debug功能，支持输入各段耗时与数据
* 5、FrcnRotateOutput层增加dw,dh限制
* 6、RpnProposalWithThresh层支持round_anchors，增加dw,dh限制，0输出保护。
*
* 历史版本：2.9.0
* 作    者：张锑
* 日    期：2021/7/17
* 备    注：
* （1）NNIE：
* 1、支持YoloOutputLand层
* 2、支持RpnProposalWithThresh层
* 3、支持RotateROIAlign层
* 4、支持FrcnRotateOutput层
* 5、Reshape层增加输出shape实时更新功能
* 6、修复RpnAttributeProposal层踩内存缺陷
* （2）DSP：
* 1、YoloOutputLand层优化
* 2、修复yoloproposal层解析参数和置信度比较缺陷
*
* 历史版本：2.8.0
* 作    者：张锑
* 日    期：2021/5/7
* 备    注：
* （1）NNIE：
* 1、支持RpnAttributeProposal层
* 2、支持NmsAttribute层
*
* 历史版本：2.7.4
* 作    者：张锑
* 日    期：2021/4/20
* 备    注：
* （1）NNIE：
* 1、修复FrcnOutput层数据溢出错误
* 2、CollectAndDistributeFpnRpnProposal层支持排序设置
* 3、CsegPostproc层支持300实例输入
*
* 历史版本：2.7.2
* 作    者：张锑
* 日    期：2021/3/20
* 备    注：
* （1）DSP：
* 1、支持ROIPoolingQuantize层
* （2）NNIE：
* 1、修复maskrcnn运行报错缺陷
*
* 历史版本：2.7.1
* 作    者：张锑
* 日    期：2021/3/5
* 备    注：
* （1）DSP：
* 1、支持yolo_quadra层三分支输入功能
* （2）NNIE：
* 1、支持CsegPostproc层
* （3）HIA：
* 1、优化HIA算子-点云坐标变换
*
* 历史版本：2.7.0
* 作    者：何映材
* 日    期：2021/1/8
* 备    注：
* （1）DSP：
* 1、input层扩展featuremap输入功能;
* 2、支持YoloOutputLand层;
* 3、支持Mask层;
* 4、支持NmsAttribute层;
* 5、支持RpnAttributeProposal层;
* 6、支持RpnProposal层;
* 7、支持HIA算子-点云坐标变换;
* 8、支持HIA算子-LV特征提取;
* （2）NNIE：
* 1、支持CollectAndDistributeFpnRpnProposal层;
* （3）HIA：
* 1、支持HIA算子-点云坐标变换;
* 2、支持HIA算子-LV特征提取;
*
* 历史版本：2.6.1
* 作    者：何映材
* 日    期：2020/11/18
* 备    注：
* （1）DSP：
* 1、input层支持featuremap输入；
*
* 历史版本：2.6.0
* 作    者：何映材
* 日    期：2020/09/30
* 备    注：
* （1）DSP：
* 1、支持permute层；
* 2、支持yoloqudra层
* 3、支持roi_align层;
* 4、完善unpooling层IDMA硬件限制
* 5、pool层增加cell_mod参数
* （2）NNIE：
* 1、新增proposal层的优化分支；
* 2、支持eastout多类别输出功能；
* 3、修复旋转矩阵层缺陷，确保结果与历史版本一致；
* 4、支持U08输入FM输入时，mean/scale操作；
* 5、NNIE删除NMS层多余过滤操作；
*
* 历史版本：2.5.3
* 作    者：何映材
* 日    期：2020/07/30
* 备    注：
*   1.该版本为HCNN正式版本，适用于新旧调度器；
*   2.DSP支持ROI_ALIGN层；
*
* 历史版本：2.5.2
* 作    者：黄斌10
* 日    期：2020/07/10
* 备    注：
*   1.该版本为HCNN正式版本，适用于新旧调度器；
*   2.NNIE支持eastout层多类别功能；
*   3.NNIE删除NMS层多余过滤操作；
*
* 历史版本：2.5.1
* 作    者：黄斌10
* 日    期：2020/06/19
* 备    注：
*   1.该版本为HCNN正式版本，适用于新旧调度器；
*   2.NNIE支持U08输入FM输入时，mean/scale操作；
*   3.NNIE修复旋转矩阵层缺陷，确保结果与历史版本一致；
*
* 历史版本：2.5.0
* 作    者：黄斌10
* 日    期：2020/06/03
* 备    注：
*   1.该版本为HCNN正式版本，适用于新旧调度器；
*   2.DSP端支持yoloproposal层3分支输入功能；
*   3.DSP端完善Setconfig配置功能；
*   4.DSP端完善Softmax层量化功能，修正0.5的误差；
*   5.NNIE端支持NmsAdas层；
*   6.NNIE端优化旋转矩阵层耗时；
*
* 历史版本：2.4.6
* 作    者：黄斌10
* 日    期：2020/04/12
* 备    注：
*   1.该版本适用于新调度器；
*   2.DSP为适配新调度器修改版本号起始地址；
*
* 历史版本：2.4.5
* 作    者：黄斌10
* 日    期：2020/03/21
* 备    注：
*   1.该版本DSP增加attention层所有类别输出功能
*   2.该版本DSP完善deconv层group功能、crop层以及softmax层功能
*
* 历史版本：2.4.4
* 作    者：zhengliupo
* 日    期：2019/12/27
* 备    注：
*   1.该版本DSP部分增加图像旋转算子功能
*   2.该版本NNIE增加NNIE子模型间数据搬运优化、MultiProposal层、CTWOutboxCornerShow层
*   3.NNIE扩展FrcnOutput层，添加对target_stds超参的解析功能
*
* 历史版本：2.4.3
* 作    者：黄斌10
* 日    期：2019/12/4
* 备    注：
*   1.该版本为HCNN调试版本
*   2.新增支持Hi3531DV200平台
*
* 历史版本：2.4.2
* 作    者：李欢22
* 日    期：2019/11/27
* 备    注：
*   1.该版本为HCNN正式发布的基线版本（2.4.x, x=0~99代表基线版本）
*   2.DSP端：更新LSTM层，新增支持YL_Park层；
*   3.NNIE端：新增支持kpnv8_proposal层，kpnv8out层，neighbor_eltwise层；
*
* 历史版本：2.4.1
* 作    者：李欢22
* 日    期：2019/11/8
* 备    注：
*   1.该版本为HCNN正式发布的基线版本（2.4.x, x=0~99代表基线版本）
*   2.DSP端：修复新bin和旧库的兼容性问题
*   3.NNIE端：支持旋转矩形层配置最大出框数目
*   4.修复和完善2.4.0发布后收集的问题和改进
*
* 历史版本：2.4.0
* 作    者：李欢22
* 日    期：2019/9/4
* 备    注：
*   1.该版本为HCNN正式发布版本
*   2.DSP端支持yolov3网络、LSTM网络、Normalize类网络
*   3.NNIE端支持LSTM网络
*
* 历史版本：2.3.9
* 作    者：黄斌10
* 日    期：2019/8/16
* 备    注：
*   1.该版本为HCNN正式发布版本
*
* 历史版本：2.3.7
* 作    者：黄斌10
* 日    期：2019/7/10
* 备    注：
*   1.该版本为HCNN正式发布版本
*   2.修复featreshape非前3.2G缺陷
*
* 历史版本：2.3.5
* 作    者：黄斌10
* 日    期：2019/5/24
* 备    注：
*   1.该版本为HCNN正式发布版本
*   2.支持人脸建模888
*   3.扩展了DSP端层功能(规格书)
*
* 历史版本：2.3.2
* 作    者：黄斌10
* 日    期：2019/3/7
* 备    注：
*   1.该版本为HCNN正式发布版本
*   2.该版本兼容2.3.1版本的所有功能及原有接口
*   3.该版本完善NNIE模型输出
*
* 历史版本：2.3.1
* 作    者：黄斌10
* 日    期：2019/2/18
* 备    注：
*   1.该版本为HCNN正式发布版本
*   2.该版本兼容2.3.0版本的所有功能及原有接口
*   3.该版本解决DSP端24bit溢出问题，需要配合使用最新转bin工具
*
* 历史版本：2.3.0
* 作    者：唐政
* 日    期：2019/1/5
* 备    注：
*   1.该版本为HCNN正式发布版本
*   2.该版本兼容2.1.0版本的所有功能及原有接口(增加DSP内存标识)
*   3.该版本支持yolov3（nnie）、多alpha量化网络（dsp）、dsp多batch优化等
*
* 历史版本：2.1.0
* 作    者：孟泽民
* 日    期：2018/08/10
* 备    注：
*   1.该版本为HCNN正式发布版本，基于海思SDK010版本!
*   2.该版本兼容1.1.0版本的所有功能及原有接口
*   3.该版本新增EX接口，用于3D与2D网络的新接口，用于后续升级
*
* 历史版本：2.0.5
* 作    者：唐政
* 日    期：2018/08/13
* 备    注：
*   1.修改算法库，适用于SDK010

* 历史版本：2.0.2
* 作    者：唐政
* 日    期：2018/07/25
* 备    注：
*   1.合入上海分支，主要改动是att优化
*
* 历史版本：2.0.1
* 作    者：唐政
* 日    期：2018/07/20
* 备    注：
*   1.NNIE支持私有reshape层
*   2.增加Compare算子分支（HIA）
*
* 历史版本：1.5.0
* 作    者：刘锦胜
* 日    期：2018/05/30
* 备    注：
*   1.优化yolo网络中featreshape层和yoloout层
*   2、新增mtcnn支持，效率调高55%(960x540分辨率)
*   3、最大输出blob数增大至64
*   4、支持feature 16bit 模型，网络平均耗时是原先的1.3倍
*   5、优化yolo网络，整体效率提升13.66%（1280x720分辨率）
*   6、支持所有层输出（除Relu、slice、featreshape，详见规格书）
*   7、新增attention层支持
*   8、加入每个线程利用率统计功能
*
* 历史版本：1.1.0
* 作    者：刘锦胜
* 日    期：2018/05/05
* 备    注：
*   1.该版本为HCNN正式发布版本，基于海思SDK006版本!
*   2.详细功能见release_note和HCNN规格书；
*   3.该版本与1.0.0版本功能完全相同,不同之处在于1.1.0基于海思SDK006,1.0.0基于海思SDK004
*
* 历史版本：1.0.0
* 作    者：刘锦胜
* 日    期：2018/04/28
* 备    注：
*   1.该版本为HCNN正式发布版本，基于海思SDK004版本!
*   2.详细功能见release_note和HCNN规格书；
*
* 历史版本：0.9.6
* 作    者：刘锦胜
* 日    期：2018/04/16
* 备    注：
*   1.该版本为算法调试版本，未经过批量稳定性测试，基于海思SDK004版本;
*     针对mtcnn优化DSP端input层和softmax层；
*
* 历史版本：0.9.5
* 作    者：刘锦胜
* 日    期：2018/04/08
* 备    注：
*   1.该版本为算法调试版本，未经过批量稳定性测试，基于海思SDK004版本;
*     NNIE支持yolo网络功能;
*     暂时不支持nv21输入，接口暂时保留，功能不保证，待1.0.0版本提供nv21输入功能；
*
* 历史版本：0.9.0
* 作    者：刘锦胜
* 日    期：2018/03/19
* 备    注：
*   1.该版本为人脸抓拍定制版本
*     支持NNIE人车检测网络、NNIE人脸检测网络和NNIE抓拍评分网络;
*     测试通过这三个网络配置在2颗NNIE上，运行48小时无异常;
*     该版本基于海思SDK 004内核版本高功耗模式，目前NNIE和DSP同时运行CNN网络，DSP会出现time out;
*
* 历史版本：0.8.5
* 作    者：刘锦胜
* 日    期：2018/03/07
* 备    注：
*   1.该版本只用于调试加密库，其它功能均不提供
* 
* 历史版本：0.8.0
* 作    者：刘锦胜
* 日    期：2017/11/27
* 备    注：
*   1.NNIE/DSP支持googlenet人车检测网络;
*   2.NNIE支持frcnn检测网络;
*
* 历史版本：0.5.0
* 作    者：山黎
* 日    期：2017/09/21
* 备    注：
***********************************************************************************************************************/
#ifndef _HCNN_LIB_H_
#define _HCNN_LIB_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
//当前版本号
#define HCNN_MAJOR_VERSION        2             // 主版本号
#define HCNN_SUB_VERSION          11            // 副版本号
#define HCNN_REVISION_VERSION     1             // 修订版本号

//版本日期
#define HCNN_VERSION_YEAR         2022          // 年份
#define HCNN_VERSION_MONTH        2             // 月份
#define HCNN_VERSION_DAY          18            // 日期

//内存资源数
#define HCNN_MEM_TAB_NUM          3             // 内存MEM_TAB个数

//输入输出
#define HCNN_BLOB_MAX_DIM         4             // HCNN_BLOB最大维度,nchw
#define HCNN_BLOB_MAX_DIM_EX      5             // HCNN_BLOB最大维度,(扩展支持3d网络)

//最大输入BLOB数量
#define HCNN_BLOB_IN_MAX_NUM      8             // 输入支持最多BLOB数量

//最大输出BLOB数量
#define HCNN_BLOB_OUT_MAX_NUM     64            // 输出支持最多BLOB数量(NNIE模型最大输出只有支持16个，DSP最多64个)

// 定义枚举结束值
#define HCNN_ENUM_END             0xFFFFFF      // 用于对齐

/***********************************************************************************************************************
* 枚举
***********************************************************************************************************************/
//内存对齐属性
typedef enum _HCNN_MEM_ALIGNMENT_E
{
    HCNN_MEM_ALIGN_4BYTE    = 4,
    HCNN_MEM_ALIGN_8BYTE    = 8,
    HCNN_MEM_ALIGN_16BYTE   = 16,
    HCNN_MEM_ALIGN_32BYTE   = 32,
    HCNN_MEM_ALIGN_64BYTE   = 64,
    HCNN_MEM_ALIGN_128BYTE  = 128,
    HCNN_MEM_ALIGN_256BYTE  = 256,
    HCNN_MEM_ALIGN_END      = HCNN_ENUM_END
} HCNN_MEM_ALIGNMENT_E;

//内存分配空间
typedef enum _HCNN_MEM_SPACE_E
{
    HCNN_MEM_EXTERNAL_PROG,                     // 外部程序存储区
    HCNN_MEM_INTERNAL_PROG,                     // 内部程序存储区
    HCNN_MEM_EXTERNAL_TILERED_DATA,             // 外部Tilered数据存储区
    HCNN_MEM_EXTERNAL_CACHED_DATA,              // 外部可Cache存储区
    HCNN_MEM_EXTERNAL_UNCACHED_DATA,            // 外部不可Cache存储区
    HCNN_MEM_INTERNAL_DATA,                     // 内部存储区
    HCNN_MEM_EXTERNAL_TILERED8 ,                // 外部Tilered数据存储区8bit，Netra/Centaurus特有
    HCNN_MEM_EXTERNAL_TILERED16,                // 外部Tilered数据存储区16bit，Netra/Centaurus特有
    HCNN_MEM_EXTERNAL_TILERED32 ,               // 外部Tilered数据存储区32bit，Netra/Centaurus特有
    HCNN_MEM_EXTERNAL_TILEREDPAGE,              // 外部Tilered数据存储区page形式，Netra/Centaurus特有
    HCNN_MEM_EXTERNAL_DATA_IVE,                 // add by wxc @ 20160422
    HCNN_MA_MEM_IN_DDR_CACHED,                  // MA平台DDR CACHED存储区
    HCNN_MA_MEM_IN_DDR_UNCACHED,                // MA平台DDR UNCACHED存储区
    HCNN_MA_MEM_IN_CMX_CACHED,                  // MA平台CMX CACHED存储区
    HCNN_MA_MEM_IN_CMX_UNCACHED,                // MA平台CMX UNCACHED存储区
    HCNN_MEM_MALLOC,                            // malloc的内存
    HCNN_MEM_MMZ_WITH_CACHE,                    // MMZ内存带cache
    HCNN_MEM_MMZ_NO_CACHE,                      // MMZ内存不带cache
    HCNN_MEM_MMZ_WITH_CACHE_PRIORITY,           // MMZ内存带cache(dsp优先申请内存段)
    HCNN_MEM_MMZ_NO_CACHE_PRIORITY,             // MMZ内存不带cache(dsp优先申请内存段)
    HCNN_MEM_EXTERNAL_END = HCNN_ENUM_END
} HCNN_MEM_SPACE_E;

//内存属性
typedef enum _HCNN_MEM_ATTRS_E
{
    HCNN_MEM_SCRATCH,                           // 可复用内存，能在多路切换时有条件复用 
    HCNN_MEM_PERSIST,                           // 不可复用内存
    HCNN_MEM_ATTRS_END = HCNN_ENUM_END
} HCNN_MEM_ATTRS_E;

//平台类型
typedef enum _HCNN_MEM_PLAT_E
{
    HCNN_MEM_PLAT_CPU,                          // CPU内存
    HCNN_MEM_PLAT_GPU,                          // GPU内存
    HCNN_MEM_PLAT_MA,                           // MA平台
    HCNN_MEM_PLAT_HI3559,                       // 海思平台
    HCNN_MEM_PLAT_END = HCNN_ENUM_END
} HCNN_MEM_PLAT_E;

//HCNN 模型类型
typedef enum _HCNN_MODEL_TYPE_E
{
    HCNN_MODEL_TYPE_DSP,                        // DSP模型
    HCNN_MODEL_TYPE_NNIE,                       // NNIE模型
    HCNN_MODEL_TYPE_DSP_3D,                     // DSP 3D网络模型
    HCNN_MODEL_TYPE_NNIE_3D                     // NNIE 3D网络模型
} HCNN_MODEL_TYPE_E;

//网络优先级枚举
typedef enum _HCNN_NET_PRIORITY_E
{
    HCNN_NET_HIGH,                               // 网络优先级-高
    HCNN_NET_MID,                                // 网络优先级-中
    HCNN_NET_LOW,                                // 网络优先级-低
    HCNN_NET_AMOUNT                              // 网络优先级总数
} HCNN_NET_PRIORITY_E;

//支持的输入数据格式
typedef enum _HCNN_INPUT_FORMAT_E
{
    HCNN_INPUT_FORMAT_BLOB       = 0,            // 通用数据格式BLOB
    HCNN_INPUT_FORMAT_YUV_NV21   = 1,            // YUV_NV12格式，YYYY VU VU
    HCNN_INPUT_FORMAT_AMOUNT
} HCNN_INPUT_FORMAT_E;

//HCNN支持数据类型
typedef enum _HCNN_BLOB_DATA_TYPE_E
{
    HCNN_BLOB_DATA_TYPE_UINT8   = 0,             // 基本数据类型unsigned char
    HCNN_BLOB_DATA_TYPE_INT32   = 1,             // 基本数据类型unsigned int
    HCNN_BLOB_DATA_TYPE_FLOAT32 = 2,             // 基本数据类型float32
    HCNN_BLOB_DATA_TYPE_INT8    = 3,             // 基本数据类型signed char,attention网络专用
    HCNN_BLOB_DATA_TYPE_AMOUNT
} HCNN_BLOB_DATA_TYPE_E;

//HCNN BLOB数据的存储格式
typedef enum _HCNN_BLOB_STORE_FORMAT_E
{
    HCNN_BLOB_STORE_FORMAT_WHCN    = 0,          // 数据存储格式WHCN，内存中排布:先排w再H再C再N
    HCNN_BLOB_STORE_FORMAT_WHLCN   = 1,          // 数据存储格式WHLCN，内存中排布:先排w再H再L再C再N
    HCNN_BLOB_STORE_FORMAT_AMOUNT
} HCNN_BLOB_STORE_FORMAT_E;

/***********************************************************************************************************************
* 结构体
***********************************************************************************************************************/
//HCNN MEM_TAB
typedef struct _HCNN_MEM_TAB_T
{
    size_t               size;                   // 以BYTE为单位的内存大小
    HCNN_MEM_ALIGNMENT_E alignment;              // 内存对齐属性, 建议为128
    HCNN_MEM_SPACE_E     space;                  // 内存分配空间
    HCNN_MEM_ATTRS_E     attrs;                  // 内存属性
    void                *base;                   // 分配出的内存指针
    void                *phy_base;               // 分配出的物理内存指针
    HCNN_MEM_PLAT_E      plat;                   // 平台
} HCNN_MEM_TAB_T;

//HCNN model结构体
typedef struct _HCNN_MODEL_PARAM_T
{
    unsigned char       *model_buffer;           // 模型buffer
    int                  model_size;             // 模型buffer大小
    HCNN_MODEL_TYPE_E    model_type;             // 模型类型
    int                  reserved[12];           // 保留字段
} HCNN_MODEL_PARAM_T; //64字节

//HCNN输出结构体
typedef struct _HCNN_OUT_BLOB_INFO_T
{
    int                  layer_idx;              // 输出的blob数据所在的层，可为负数，表示倒数，最后一层为-1
    int                  blob_idx;               // 输出的blob数据的序号（如果该层只有一个输出，则为0，有多个输出，可用序号选择）
} HCNN_OUT_BLOB_INFO_T;

//HCNN模块输入参数
typedef struct _HCNN_PARAM_T
{
    int                     in_blob_num;                                              // 输入BLOB数量
    int                     out_blob_num;                                             // 输出BLOB数量
    
    int                     in_blob_shape[HCNN_BLOB_IN_MAX_NUM][HCNN_BLOB_MAX_DIM];   // 输入BLOB维度
    int                     in_blob_stride_w[HCNN_BLOB_IN_MAX_NUM];                   // 输入BLOB宽度跨距
    int                     in_blob_stride_h[HCNN_BLOB_IN_MAX_NUM];                   // 输入BLOB高度跨距
    HCNN_MEM_SPACE_E        in_blob_mem_type[HCNN_BLOB_IN_MAX_NUM];                   // 输入BLOB内存类型
    HCNN_INPUT_FORMAT_E     input_format[HCNN_BLOB_IN_MAX_NUM];                       // 输入BLOB格式
    HCNN_BLOB_DATA_TYPE_E   in_blob_data_type[HCNN_BLOB_IN_MAX_NUM];                  // 输入BLOB数据类型

    HCNN_OUT_BLOB_INFO_T    out_blob_info[HCNN_BLOB_OUT_MAX_NUM];                     // 输出BLOB信息

    void                   *scheduler_handle;                                         // 调度器handle
    void                   *model_handle;                                             // 模型handle

    HCNN_NET_PRIORITY_E     net_priority;                                             // 优先级

    int                     reserved[9];                                              // 保留字段
} HCNN_PARAM_T; //384字节

//HCNN库数据表示形式
typedef struct _HCNN_BLOB_T
{
    int                      shape[HCNN_BLOB_MAX_DIM];                                // BLOB数据维度（n，c，h，w）
    int                      stride_w;                                                // BLOB宽度stride
    int                      stride_h;                                                // BLOB高度stride    
    HCNN_BLOB_STORE_FORMAT_E store_format;                                            // BLOB存储格式
    HCNN_BLOB_DATA_TYPE_E    data_type;                                               // BLOB数据类型
    void                    *data;                                                    // BLOB数据首地址
    int                      reserved[10];                                            // 保留字段
} HCNN_BLOB_T; 

//HCNN输入
typedef struct _HCNN_INPUT_T
{ 
    int                     in_blob_num;
    HCNN_BLOB_T             in_blob[HCNN_BLOB_IN_MAX_NUM];                            // 输入BLOB
    HCNN_INPUT_FORMAT_E     in_format[HCNN_BLOB_IN_MAX_NUM];                          // 输入格式
    HCNN_MEM_SPACE_E        in_blob_mem_type[HCNN_BLOB_IN_MAX_NUM];                   // 输入BLOB内存类型
    int                     reserved[14];                                             // 保留字段
} HCNN_INPUT_T; //768字节

//HCNN输出
typedef struct _HCNN_OUTPUT_T
{
    int                     out_blob_num;
    HCNN_BLOB_T             out_blob[HCNN_BLOB_OUT_MAX_NUM];                           // 输出BLOB
    int                     reserved[14];                                              // 保留字段
} HCNN_OUTPUT_T; //704字节

//HCNN模块输入参数(扩展支持3D网络)
typedef struct _HCNN_PARAM_EX_T
{
    int                     in_blob_num;                                               // 输入BLOB数量
    int                     out_blob_num;                                              // 输出BLOB数量
    
    int                     in_blob_shape[HCNN_BLOB_IN_MAX_NUM][HCNN_BLOB_MAX_DIM_EX]; // 输入BLOB维度
    int                     in_blob_stride_w[HCNN_BLOB_IN_MAX_NUM];                    // 输入BLOB宽度跨距
    int                     in_blob_stride_h[HCNN_BLOB_IN_MAX_NUM];                    // 输入BLOB高度跨距
    HCNN_MEM_SPACE_E        in_blob_mem_type[HCNN_BLOB_IN_MAX_NUM];                    // 输入BLOB内存类型
    HCNN_INPUT_FORMAT_E     input_format[HCNN_BLOB_IN_MAX_NUM];                        // 输入BLOB格式
    HCNN_BLOB_DATA_TYPE_E   in_blob_data_type[HCNN_BLOB_IN_MAX_NUM];                   // 输入BLOB数据类型

    HCNN_OUT_BLOB_INFO_T    out_blob_info[HCNN_BLOB_OUT_MAX_NUM];                      // 输出BLOB信息

    void                   *scheduler_handle;                                          // 调度器handle
    void                   *model_handle;                                              // 模型handle

    HCNN_NET_PRIORITY_E     net_priority;                                              // 优先级

    int                     reserved[9];                                               // 保留字段
} HCNN_PARAM_EX_T;

//HCNN库数据表示形式(扩展支持3D网络)
typedef struct _HCNN_BLOB_EX_T
{
    int                      shape[HCNN_BLOB_MAX_DIM_EX];                              // BLOB数据维度（n，c, l, h，w）
    int                      stride_w;                                                 // BLOB宽度stride
    int                      stride_h;                                                 // BLOB高度stride    
    HCNN_BLOB_STORE_FORMAT_E store_format;                                             // BLOB存储格式
    HCNN_BLOB_DATA_TYPE_E    data_type;                                                // BLOB数据类型
    void                    *data;                                                     // BLOB数据首地址
    int                      reserved[10];                                             // 保留字段
} HCNN_BLOB_EX_T; 

//HCNN输入(扩展支持3D网络)
typedef struct _HCNN_INPUT_EX_T
{ 
    int                     in_blob_num;
    HCNN_BLOB_EX_T          in_blob[HCNN_BLOB_IN_MAX_NUM];                             // 输入BLOB
    HCNN_INPUT_FORMAT_E     in_format[HCNN_BLOB_IN_MAX_NUM];                           // 输入格式
    HCNN_MEM_SPACE_E        in_blob_mem_type[HCNN_BLOB_IN_MAX_NUM];                    // 输入BLOB内存类型
    int                     reserved[14];                                              // 保留字段
} HCNN_INPUT_EX_T; 

//HCNN输出(扩展支持3D网络)
typedef struct _HCNN_OUTPUT_EX_T
{
    int                     out_blob_num;
    HCNN_BLOB_EX_T          out_blob[HCNN_BLOB_OUT_MAX_NUM];                           // 输出BLOB
    int                     reserved[14];                                              // 保留字段
} HCNN_OUTPUT_EX_T; 

/***********************************************************************************************************************
* 接口函数
***********************************************************************************************************************/
/***********************************************************************************************************************
* 功  能：获取网络模型所需内存大小
* 参  数：*
*         model_param           -I      模型参数
*         mem_tab               -O      所需内存的描述
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_GetModelMemSize(HCNN_MODEL_PARAM_T    *model_param,
                         HCNN_MEM_TAB_T         mem_tab[HCNN_MEM_TAB_NUM]);

/***********************************************************************************************************************
* 功  能：创建网络模型
* 参  数：*
*         model_param           -I      模型参数
*         mem_tab               -I      所需内存的描述
*         model_handle          -O      模型handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_CreateModel(HCNN_MODEL_PARAM_T    *model_param,
                     HCNN_MEM_TAB_T         mem_tab[HCNN_MEM_TAB_NUM],
                     void                 **model_handle);
                        
/***********************************************************************************************************************
* 功  能：销毁网络模型（由于海思还未正式发布NNIE接口，该接口暂时预留，正式发布版本可能会去掉该接口）
* 参  数：*
*         model_handle          -I      模型handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_ReleaseModel(void  *model_handle);

/***********************************************************************************************************************
* 功  能：获取网络运行内存大小
* 参  数：*
*         cnn_param             -I      CNN参数
*         mem_tab               -O      所需内存的描述
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_GetMemSize(HCNN_PARAM_T          *cnn_param,
                    HCNN_MEM_TAB_T         mem_tab[HCNN_MEM_TAB_NUM]);

/***********************************************************************************************************************
* 功  能：创建算法库句柄
* 参  数：*
*         cnn_param             -I      CNN参数
*         mem_tab               -O      所需内存的描述
*         net_handle            -O      算法库句柄
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_Create(HCNN_PARAM_T              *cnn_param,
                HCNN_MEM_TAB_T             mem_tab[HCNN_MEM_TAB_NUM],
                void                     **net_handle);

/***********************************************************************************************************************
* 功  能：执行CNN网络
* 参  数：*
*         net_handle        -I      网络handle句柄
*         in_buf            -I      输入缓存
*         in_size           -I      输入缓存大小
*         out_buf           -O      输出缓存
*         out_size          -I      输出缓存大小
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_Process(void                *net_handle,
                 void                *in_buf,
                 int                  in_size,
                 void                *out_buf,
                 int                  out_size);

/***********************************************************************************************************************
* 功  能：销毁算法库（由于海思还未正式发布NNIE接口，该接口暂时预留，正式发布版本可能会去掉该接口）
* 参  数：*
*          net_handle        -I      网络handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_Release(void   *net_handle);

/***********************************************************************************************************************
* 功  能：获取网络模型所需内存大小(扩展到3D网络)
* 参  数：*
*         model_param           -I      模型参数
*         mem_tab               -O      所需内存的描述
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_GetModelMemSize_EX(HCNN_MODEL_PARAM_T    *model_param,
                            HCNN_MEM_TAB_T         mem_tab[HCNN_MEM_TAB_NUM]);

/***********************************************************************************************************************
* 功  能：创建网络模型(扩展到3D网络)
* 参  数：*
*         model_param           -I      模型参数
*         mem_tab               -I      所需内存的描述
*         model_handle          -O      模型handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_CreateModel_EX(HCNN_MODEL_PARAM_T    *model_param,
                        HCNN_MEM_TAB_T         mem_tab[HCNN_MEM_TAB_NUM],
                        void                 **model_handle);
                        
/***********************************************************************************************************************
* 功  能：销毁网络模型（由于海思还未正式发布NNIE接口，该接口暂时预留，正式发布版本可能会去掉该接口）
* 参  数：*
*         model_handle          -I      模型handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_ReleaseModel_EX(void  *model_handle);

/***********************************************************************************************************************
* 功  能：获取网络运行内存大小(扩展到3D网络)
* 参  数：*
*         cnn_param             -I      CNN参数
*         mem_tab               -O      所需内存的描述
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_GetMemSize_EX(HCNN_PARAM_EX_T       *cnn_param,
                       HCNN_MEM_TAB_T         mem_tab[HCNN_MEM_TAB_NUM]);

/***********************************************************************************************************************
* 功  能：创建算法库句柄(扩展到3D网络)
* 参  数：*
*         cnn_param             -I      CNN参数
*         mem_tab               -O      所需内存的描述
*         net_handle            -O      算法库句柄
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_Create_EX(HCNN_PARAM_EX_T           *cnn_param,
                   HCNN_MEM_TAB_T             mem_tab[HCNN_MEM_TAB_NUM],
                   void                     **net_handle);

/***********************************************************************************************************************
* 功  能：执行CNN网络(扩展到3D网络)
* 参  数：*
*         net_handle        -I      网络handle句柄
*         in_buf            -I      输入缓存
*         in_size           -I      输入缓存大小
*         out_buf           -O      输出缓存
*         out_size          -I      输出缓存大小
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_Process_EX(void                *net_handle,
                    void                *in_buf,
                    int                  in_size,
                    void                *out_buf,
                    int                  out_size);

/***********************************************************************************************************************
* 功  能：销毁算法库（由于海思还未正式发布NNIE接口，该接口暂时预留，正式发布版本可能会去掉该接口）
* 参  数：*
*          net_handle        -I      网络handle
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_Release_EX(void   *net_handle);

/***********************************************************************************************************************
* 功  能：设置配置参数
* 参  数：*
*         handle            -I      CNN句柄
*         config_type       -I      配置类型
*         in_buf            -I      指向输入配置信息缓存的指针
*         buf_size          -I      配置信息缓存大小
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_SetConfig(void              *handle,
                   int                config_type,
                   void              *in_buf,
                   int                buf_size);

/***********************************************************************************************************************
* 功  能：获取配置参数
* 参  数：*
*         handle            -I      CNN句柄
*         config_type       -I      配置类型
*         out_buf           -O      指向输出配置信息缓存的指针
*         buf_size          -I      配置信息缓存大小
* 返回值：状态码
***********************************************************************************************************************/
int HCNN_GetConfig(void            *handle,
                   int              config_type,
                   void            *out_buf,
                   int              buf_size);

/***********************************************************************************************************************
* 功  能: 获取HCNN库版本号
* 参  数: 无
* 返回值: HCNN库版本号
***********************************************************************************************************************/
int HCNN_GetVersion();

#ifdef __cplusplus 
}
#endif

#endif /* _HCNN_LIB_H_ */