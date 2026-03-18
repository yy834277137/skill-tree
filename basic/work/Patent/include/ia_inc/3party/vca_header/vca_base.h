/**************************************************************************************************
* 
* 版权信息：版权所有 (c) 2010-2018, 杭州海康威视软件有限公司, 保留所有权利
*
* 文件名称：vca_base.h
* 文件标识：_HIK_VCA_BASE_H_
* 摘    要：海康威视VCA公共基础数据结构体声明文件，用于后续杭州和上海组件化开发

* 当前版本：2.1.0
* 作    者：车军、童超、张泽瀚
* 日    期：2022年3月8日
* 备    注：增加多模态传感器结构体，点云输入结构体 VCA_3D_SENSOR_POINT_F 
            新增3D检测框输出结构体 VCA_CUBE_F、VCA_POLYHEDRON_F、VCA_3D_OBJ_INFO、VCA_3D_OBJ_LIST

* 当前版本：2.0.20
* 作    者：车军、童超
* 日    期：2022年1月25日
* 备    注：增加RN平台相关平台类型、内存类型、DL处理类型，修改NT平台类型、内存类型和DL处理类型的注释说明
            增加车辆属性模型类型

* 历史版本：2.0.19
* 作    者：车军
* 日    期：2021年8月4日
* 备    注：修改VCA_MEM_ATTRS，增加内存属性，提升HVA中内存可复用性

* 当前版本：2.0.18
* 作    者：车军
* 日    期：2021年6月28日
* 备    注：增加RK平台相关平台类型、内存类型、DL处理类型

* 当前版本：2.0.17
* 作    者：车军
* 日    期：2021年6月7日
* 备    注：增加目标类别

* 当前版本：2.0.16
* 作    者：车军
* 日    期：2021年5月28日
* 备    注：增加ARM 深度学习处理类型
            新增特征比对模型类型


* 当前版本：2.0.15
* 作    者：车军
* 日    期：2021年4月12日
* 备    注：新增HD平台定义
            新增部分模型类型

* 当前版本：2.0.14
* 作    者：车军
* 日    期：2021年1月18日
* 备    注：新增MARS平台定义,VCA_MEM_SPACE复用VCA_HCNN_MEM_XXX

* 当前版本：2.0.13
* 作    者：车军
* 日    期：2021年1月7日
* 备    注：新增G5、A7A8平台DL处理模式
          			
* 当前版本：2.0.12
* 作    者：车军
* 日    期：2020年11月24日
* 备    注：新增K91平台DL处理模式
            修改部分注释

* 当前版本：2.0.11
* 作    者：车军
* 日    期：2020年8月4日
* 备    注：新增车牌PQE模型和非机动车车牌模型

* 当前版本：2.0.10
* 作    者：车军
* 日    期：2020年7月23日
* 备    注：1、增补二轮车相关分类模型

* 当前版本：2.0.10
* 作    者：车军
* 日    期：2020年7月21日
* 备    注：1、增加人体识别相关模型类型
            2、增加非机动车检索相关模型类型；
			3、增加车载应用相关模型类型；
			4、增加非机动车检测、HMS目标检测模型类型

* 当前版本：2.0.9
* 作    者：车军
* 日    期：2020年5月25日
* 备    注：1、增加昇腾相关平台和内存信息

* 当前版本：2.0.8
* 作    者：车军
* 日    期：2020年1月13日
* 备    注：1、增加人体、非机动车评分模型
            2、增加寒武纪内存标志
			3、增加副驾驶特征模型
			4、增加人脸人体关联后处理模型

* 当前版本：2.0.7
* 作    者：车军
* 日    期：2019年11月1日
* 备    注：1、修复与开发平台头文件冲突问题

* 当前版本：2.0.6
* 作    者：车军
* 日    期：2019年10月28日
* 备    注：1、增加工业OCR模型
            2、支持x86各指令集的模型处理模式；
			3、支持寒武纪和比特大陆平台模式

* 当前版本：2.0.5
* 作    者：车军
* 日    期：2019年7月25日
* 备    注：1、增加模型类型

* 当前版本：2.0.4
* 作    者：车军
* 日    期：2019年3月13日
* 备    注：1、增加出入口场景车辆外观识别模型
            2、增加VCA_TR_INFO_V3
            3、增补飞机、无人机、人头、头肩、轮船等基本目标类型

* 当前版本：2.0.3
* 作    者：车军
* 日    期：2019年2月18日
* 备    注：1、增加GPU平台深度学习处理模式VCA_DL_PROC_NV_TENSORCORE，支持T4，xavier平台VCA_DL_PROC_NV_TENSORCORE
			
* 当前版本：2.0.2
* 作    者：车军
* 日    期：2019年1月8日
* 备    注：1、增加VCA_MEM_BUF_V2，用于明确物理缓存起始终止位置
           2、增加驾驶室特征及牌识识别模型；
           3、增加多部件检测目标类别
           4、增加目标结构体VCA_OBJ_LIST_V3。
		   5、修改HI的DL_PRO_TYPE，增加3D处理模式
* 当前版本：2.0.1
* 作    者：车军
* 日    期：2018年12月19日
* 备    注：1 增加3D坐标结构体VCA_3D_POINT_F;
            2 增加车载相关模型
			3 增加交通相关目标类型
			4 新增跟踪结构体VCA_TR_INFO_V2

* 当前版本：2.0.0
* 作    者：车军
* 日    期：2018年12月12日
* 备    注：1 在模型类型中增加行人部件模型、人体分割模型、行李箱模型
            2 增加图像格式、数据格式定义
			3 增加开放平台目标信息定义VCA_BLOB_INFO_LIST
			4 增加图像格式定义VCA_MAP，多媒体帧定义VCA_MEDIA_FRAME，图（矩阵）定义VCA_MAP
			
* 当前版本：1.1.5
* 作    者：车军
* 日    期：2018年7月10日
* 备    注：1 在模型类型中增加VTS使用的检测分割类型
            2 车辆特征模型中增加头盔检测白天晚上类型
                      
* 当前版本：1.1.4
* 作    者：车军
* 日    期：2018年6月8日
* 备    注：1 在帧类型中增加2个BGR相关枚举类型
            2 车辆特征模型中增加驾驶室成像质量分级模型
            3 车牌模型中增加4个牌识相关模型
            4 增加4个OCR相关模型：集装箱号模型、拖车号模型、身份证模型、关检测检测模型
* 当前版本：1.1.2
* 作    者：车军
* 日    期：2018年5月4日
* 备    注：1 增加目标类型如下：公交、货车、烟雾、火焰、条幅、摊位

* 当前版本：1.1.1
* 作    者：车军
* 日    期：2018年3月16日
* 备    注：1 增加人体、车辆升级模型类型
            2 增加商品建模、菜品建模模型类型
            3 修复格式缺陷

* 当前版本：1.1.0
* 作    者：车军
* 日    期：2018年2月5日
* 备    注：1 增加VCA_MEM_TAB_V3，支持物理内存（非HISI3559可以不用），64位系统内存尺寸支持4GB以上
            2 VCA_MEM_PLAT支持VCA_MEM_PLAT_HI3559
			3 内存类型支持Hi3559内存类型
			4 模型分析类型支持GPU：VCA_DL_PROC_RT_INT8
			5 模型分析类型支持Hi3559：VCA_DL_PROC_INT8_HI_NNIE和VCA_DL_PROC_INT8_HI_DSP 
			
* 当前版本：1.0.4                   
* 作    者：车军
* 日    期：2017年12月18日
* 备    注：1 增加车辆属性正背向模型
            2 增加三轮车分类模型
			3 二三轮车违法载人
			4 附属物识别车脸模型
			5 附属物识别车背模型
			6 附属物识别车顶模型

* 当前版本：1.0.3
* 作    者：车军
* 日    期：2017年9月21日
* 备    注：1 增加出租车识别模型索引
            2 增加人脸评分模型索引

* 当前版本：1.0.2
* 作    者：车军
* 日    期：2017年6月1日
* 备    注：1 增加MA平台内存类型
            2 内存类型增加MA平台内存类型

* 当前版本：1.0.1
* 作    者：车军
* 日    期：2017年5月18日
* 备    注：1 增加人体建模、车辆建模索引模型
            2 模型信息中增加分析类型（FP32，FP16，INT8）
			3 增加深度学习车牌识别相关模型类型

* 当前版本：1.0.0
* 作    者：车军
* 日    期：2017年3月17日
* 备    注：1 增加临牌分类模型类型
            2 增加目标链表V2，不限制目标个数

* 当前版本：0.10.0
* 作    者：车军
* 日    期：2017年1月23日
* 备    注：1 增加机动车车型识别模型
           2 增加非机动车车型识别模型

* 当前版本：0.9.0
* 作    者：车军
* 日    期：2016年11月29日
* 备    注：1 增加天窗站人模型
            2 增加二轮车、三轮车等检测类型
            3 VCA_MODEL_INFO中model_type修改为int类型，兼容模型类型自定义扩展

* 当前版本：0.8.0
* 作    者：车军
* 日    期：2016年9月19日
* 备    注：1 增加治安场景车辆属性模型定义
            2 增加场景分类模型定义
			3 增加车辆正背向分类模型定义

* 当前版本：0.7.0
* 作    者：车军
* 日    期：2016年8月24日
* 备    注：1 增加驾驶室特征模型定义
           2 增加车辆组件枚举类型
           3 完善boosting检测类型；
          4 增加FRCNN目标检测类型
          5 增加二轮车、三轮车目标类型定义

* 当前版本：0.6.0
* 作    者：车军
* 日    期：2016年7月17日
* 备    注：1 修正导入模型类型
*           2 支持深度学习算法库VCA_MODEL_PARAM以及VCA_MEM_TAB_V2

* 当前版本：0.5.0
* 作    者：车军
* 日    期：2015年12月3日
* 备    注：1 增加支持GPU的内存分配空间VCA_MEM_TAB_V2
           2 增加支持深度学习算法库的初始化参数结构体VCA_MEM_PARAM_V2 

* 当前版本：0.4.0
* 作    者：全晓臣 邝宏武
* 日    期：2015年12月3日
* 备    注：1、修改目标检测方法的枚举类型，
               高4位为检测方法编号   （最多16个模块），参考VCA_DET_TECH
			   高5-10位为目标类型编号（最多64种类型），参考VCA_DET_TYPE
			   高11-14位为方向编号   （最多16种方向），参考VCA_DET_DIRECTION

* 当前版本：0.3.0
* 作    者：车军 邝宏武
* 日    期：2015年11月10日
* 备    注：1、VCA_YUV_DATA结构体中增加下采样倍率scale_rate，0或1表示原图，其他为降采样倍率如2，4，8
*           2、增加颜色枚举VCA_COLOR_TYPE
*           3、增加跟踪目标链表VCA_OBJ_TR_LIST

* 当前版本：0.2.0
* 作    者：全晓臣 邝宏武
* 日    期：2014年12月10日
* 备    注：VCA_MEM_TAB结构体，只要定义了_HIK_COMPLEX_MEM_或DSP或_HIK_DSP_APP_，
*           就采用5参数内存方式，否则采用3参数内存方式

* 当前版本：0.1.0
* 作    者：全晓臣 邝宏武
* 日    期：2014年10月10日
* 备    注：初始版本
**************************************************************************************************/
#ifndef _HIK_VCA_BASE_H_
#define _HIK_VCA_BASE_H_
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef unsigned int HRESULT;
#endif /* _HRESULT_DEFINED */

#define VCA_MEM_TAB_NUM         3        // 内存表的个数
#define VCA_MAX_VERTEX_NUM      10       // 多边形最大顶点数
#define VCA_MAX_OBJ_NUM         64       // 支持目标最大个数
#define VCA_ENUM_END 	  	    0xFFFFFF // 定义枚举结束值，用于对齐
#define VCA_ENUM_END_V2	  	    0xFFFFFFFF // 定义枚举结束值，用于对齐
#define VCA_MAX_PATH_LEN        1024     // 模型路径长度
#define VCA_MAX_MODEL_NUM       16       // 一次最多导入的模型数量
#define VCA_MODEL_MEM_TAB_NUM   4        // 模型内存tab个数
#define VCA_CUDA_MEM_TAB_NUM    1        // CUDA内存个数
#define VCA_PLHD_MAX_VERTEX_NUM 32       // 多边形与多边体最大顶点数 

//场景类型
typedef enum _VCA_SCENE_MODE_
{
	VCA_UNKOWN_SCENCE   = 0,     //未知场景
	VCA_FD_CP_SCENE     = 1,     //人员卡口场景
	VCA_HVT_CP_SCENE    = 2,     //车辆卡口场景
	VCA_MIX_CP_SCENE    = 3,     //混行卡口场景
	VCA_PS_CP_SCENE     = 4,     //治安卡口场景Public Security
	VCA_SCENE_MODE_END  = VCA_ENUM_END
}VCA_SCENE_MODE;

/**************************************************************************************************
* 点
**************************************************************************************************/
//点(短整型）
typedef struct _VCA_POINT_S_
{
    short x;
    short y;
}VCA_POINT_S;

//点(整型) 
typedef struct _VCA_POINT_I_
{
    int x;
    int y;
}VCA_POINT_I;

//点(浮点型) 
typedef struct _VCA_POINT_F_
{
    float x;
    float y;
}VCA_POINT_F;

// 三维坐标点(浮点型)
typedef struct _VCA_3D_POINT_F_
{
	float x;
	float y;
	float z;
} VCA_3D_POINT_F;
/**************************************************************************************************
* 矩形
**************************************************************************************************/
//矩形(短整型)
typedef struct _VCA_RECT_S_
{
	short x;         //矩形左上角X轴坐标
	short y;         //矩形左上角Y轴坐标
	short width;     //矩形宽度
	short height;    //矩形高度
}VCA_RECT_S;

//矩形(整型)
typedef struct _VCA_RECT_I_
{
    int   x;         //矩形左上角X轴坐标
    int   y;         //矩形左上角Y轴坐标
    int   width;     //矩形宽度
    int   height;    //矩形高度
}VCA_RECT_I;

//矩形(浮点型)
typedef struct _VCA_RECT_F_
{
    float x;         //矩形左上角X轴坐标
    float y;         //矩形左上角Y轴坐标
    float width;     //矩形宽度
    float height;    //矩形高度
}VCA_RECT_F;

/**************************************************************************************************
* 包围盒
**************************************************************************************************/
//包围盒(短整型)
typedef struct _VCA_BOX_S_
{
	short left;      //左边界
	short top;       //上边界
	short right;     //右边界
	short bottom;    //下边界
}VCA_BOX_S;

//包围盒(整型)
typedef struct _VCA_BOX_I_
{
    int   left;      //左边界
    int   top;       //上边界
    int   right;     //右边界
    int   bottom;    //下边界
}VCA_BOX_I;

//包围盒(浮点型)
typedef struct _VCA_BOX_F_
{
    float left;      //左边界
    float top;       //上边界
    float right;     //右边界
    float bottom;    //下边界
}VCA_BOX_F;

/**************************************************************************************************
* 多边形
**************************************************************************************************/
//多边形(短整型)
typedef struct _VCA_POLYGON_S_
{
	unsigned int  vertex_num;                  //顶点数
	VCA_POINT_S   point[VCA_MAX_VERTEX_NUM];   //顶点
}VCA_POLYGON_S;

//多边形(整型)
typedef struct _VCA_POLYGON_I_
{
	unsigned int  vertex_num;                  //顶点数
	VCA_POINT_I   point[VCA_MAX_VERTEX_NUM];   //顶点
}VCA_POLYGON_I;

//多边形(浮点型)
typedef struct _VCA_POLYGON_F_
{
	unsigned int  vertex_num;                  //顶点数
	VCA_POINT_F   point[VCA_MAX_VERTEX_NUM];   //顶点
}VCA_POLYGON_F;

/**************************************************************************************************
* 矢量
**************************************************************************************************/
//矢量（短整型）
typedef struct _VCA_VECTOR_S_
{
    VCA_POINT_S   start_point;   //起点
    VCA_POINT_S   end_point;     //终点
}VCA_VECTOR_S;

//矢量（整型）
typedef struct _VCA_VECTOR_I_
{
	VCA_POINT_I   start_point;   //起点
	VCA_POINT_I   end_point;     //终点
}VCA_VECTOR_I;

//矢量(浮点型)
typedef struct _VCA_VECTOR_F_
{
    VCA_POINT_F   start_point;   //起点
    VCA_POINT_F   end_point;     //终点
}VCA_VECTOR_F;

/**************************************************************************************************
* 尺寸
**************************************************************************************************/
//尺寸(短整型)
typedef struct _VCA_SIZE_S_
{
    short width;
    short height;
}VCA_SIZE_S;

//尺寸(整型)
typedef struct _VCA_SIZE_I_
{
    int  width;
    int  height;
}VCA_SIZE_I;

//尺寸(浮点型)
typedef struct _VCA_SIZE_F_
{
    float  width;
    float  height;
}VCA_SIZE_F;

/**************************************************************************************************
* 内存
**************************************************************************************************/
/***********************************************************************************************************************
*内存管理器VCA_MEM_TAB结构体定义
***********************************************************************************************************************/

/*内存对齐属性*/
typedef enum _VCA_MEM_ALIGNMENT_
{
    VCA_MEM_ALIGN_4BYTE    = 4,
    VCA_MEM_ALIGN_8BYTE    = 8,
    VCA_MEM_ALIGN_16BYTE   = 16,
    VCA_MEM_ALIGN_32BYTE   = 32,
    VCA_MEM_ALIGN_64BYTE   = 64,
    VCA_MEM_ALIGN_128BYTE  = 128,
    VCA_MEM_ALIGN_256BYTE  = 256,
    VCA_MEM_ALIGN_END      = VCA_ENUM_END
}VCA_MEM_ALIGNMENT;

/* 内存属性 */
typedef enum _VCA_MEM_ATTRS_
{
    VCA_MEM_SCRATCH,                 /* 可复用内存，能在多路切换时有条件复用 */ 
    VCA_MEM_PERSIST,                 /* 不可复用内存 */ 
    VCA_MEM_DYNAMIC,                 /* 动态分配内存，允许外部动态调整 */ 
	VCA_MEM_OUTBUFFER,               /* 输出内存，根据运行关联有条件复用 */
    VCA_MEM_ATTRS_END = VCA_ENUM_END
}VCA_MEM_ATTRS;

typedef enum _VCA_MEM_PLAT_
{
	VCA_MEM_PLAT_CPU,                 /* CPU内存 */ 
	VCA_MEM_PLAT_GPU,                 /* GPU内存 */ 
	VCA_MEM_PLAT_MA,                  /* MA平台 */
	VCA_MEM_PLAT_HI3559,              /* 海思平台 */
	VCA_MEM_PLAT_NT,				  /* NT平台 */
	VCA_MEM_PLAT_MLU,                 /* 寒武纪平台*/
	VCA_MEM_PLAT_ASCEND,              /* 昇腾平台*/
    VCA_MEM_PLAT_MARS,                /* MARS平台*/
	VCA_MEM_PLAT_HD,                  /* HD平台*/
	VCA_MEM_PLAT_RN,                  /* RN平台 */
	VCA_MEM_PLAT_SN,                  /* SN平台 */
	VCA_MEM_PLAT_END = VCA_ENUM_END
}VCA_MEM_PLAT;

/* 内存分配空间 */
typedef enum _VCA_MEM_SPACE_ 
{
    VCA_MEM_EXTERNAL_PROG,              /* 外部程序存储区          */  
    VCA_MEM_INTERNAL_PROG,              /* 内部程序存储区          */
    VCA_MEM_EXTERNAL_TILERED_DATA,      /* 外部Tilered数据存储区   */
    VCA_MEM_EXTERNAL_CACHED_DATA,       /* 外部可Cache存储区       */
    VCA_MEM_EXTERNAL_UNCACHED_DATA,     /* 外部不可Cache存储区     */
    VCA_MEM_INTERNAL_DATA,              /* 内部存储区              */
    VCA_MEM_EXTERNAL_TILERED8 ,         /* 外部Tilered数据存储区8bit，Netra/Centaurus特有 */
    VCA_MEM_EXTERNAL_TILERED16,         /* 外部Tilered数据存储区16bit，Netra/Centaurus特有 */
    VCA_MEM_EXTERNAL_TILERED32 ,        /* 外部Tilered数据存储区32bit，Netra/Centaurus特有 */
    VCA_MEM_EXTERNAL_TILEREDPAGE,       /* 外部Tilered数据存储区page形式，Netra/Centaurus特有 */
	VCA_MEM_EXTERNAL_DATA_IVE,          // add by wxc @ 20160422
	VCA_MA_MEM_IN_DDR_CACHED,           /* MA平台DDR CACHED存储区   */
	VCA_MA_MEM_IN_DDR_UNCACHED,         /* MA平台DDR UNCACHED存储区 */
	VCA_MA_MEM_IN_CMX_CACHED,           /* MA平台CMX CACHED存储区   */
	VCA_MA_MEM_IN_CMX_UNCACHED,         /* MA平台CMX UNCACHED存储区 */
	VCA_HCNN_MEM_MALLOC,                /* malloc的内存*/
	VCA_HCNN_MEM_MMZ_WITH_CACHE,        /* MMZ内存带cache*/
    VCA_HCNN_MEM_MMZ_NO_CACHE,          /* MMZ内存不带cache*/
    VCA_CNN_MEM_CPU,                    /* CNN库CPU内存 */
    VCA_CNN_MEM_GPU,                    /* CNN库GPU内存 */
	VCA_CNN_MEM_NT,						/* NT平台MMZ内存 */
	VCA_HCNN_MEM_MMZ_WITH_CACHE_PRIORITY, /* MMZ内存带cache, 优先申请3.2G内存以内*/
    VCA_HCNN_MEM_MMZ_NO_CACHE_PRIORITY,   /* MMZ内存不带cache, 优先申请3.2G内存以内*/
	VCA_CNN_MEM_MLU,                     /* MLU内存*/
	VCA_CNN_MEM_AICORE,                  /* 昇腾AI Core内存*/
	VCA_HD_MEM_MMZ_WITH_CACHE,          /*HD平台MMZ内存带cache*/
	VCA_HD_MEM_MMZ_NO_CACHE,            /*HD平台MMZ内存不带cache*/
	VCA_RN_MEM_MMZ_CMA_WITH_CACHE,      /* RN平台CMA内存带cache，其中CMA内存为物理连续内存 */
	VCA_RN_MEM_MMZ_CMA_NO_CACHE,        /* RN平台CMA内存不带cache */
	VCA_RN_MEM_MMZ_IOMMU_WITH_CACHE,    /* RN平台IOMMU内存带cache，其中IOMMU内存为物理非连续内存 */
	VCA_RN_MEM_MMZ_IOMMU_NO_CACHE,      /* RN平台IOMMU内存不带cache */
	VCA_RN_MEM_MALLOC,                  /* RN平台malloc内存 */
	VCA_SN_MEM_MMZ_WITH_CACHE,      	/* SN平台MMZ内存带cache，MMZ内存为物理连续内存 */
	VCA_SN_MEM_MMZ_NO_CACHE,        	/* SN平台MMZ内存不带cache */
	VCA_SN_MEM_MALLOC,                  /* SN平台malloc内存 */
    VCA_MEM_EXTERNAL_END = VCA_ENUM_END
}VCA_MEM_SPACE;

#if ((!defined _HIK_COMPLEX_MEM_) && (!defined DSP) && (!defined _HIK_DSP_APP_))
typedef struct _VCA_MEM_TAB_
{
    unsigned int        size;       /* 以BYTE为单位的内存大小*/
    VCA_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
    void*               base;       /* 分配出的内存指针 */
} VCA_MEM_TAB;
#else
/* 内存分配结构体 */
typedef struct _VCA_MEM_TAB_
{
    unsigned int        size;       /* 以BYTE为单位的内存大小*/
    VCA_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
    VCA_MEM_SPACE       space;      /* 内存分配空间 */
    VCA_MEM_ATTRS       attrs;      /* 内存属性 */
    void                *base;       /* 分配出的内存指针 */
} VCA_MEM_TAB;
#endif  /* ((!defined _HIK_COMPLEX_MEM_) && (!defined DSP) && (!defined _HIK_DSP_APP_)) */

typedef struct _VCA_MEM_TAB_V2_
{
	unsigned int        size;       /* 以BYTE为单位的内存大小*/
	VCA_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
	VCA_MEM_SPACE       space;      /* 内存分配空间 */
	VCA_MEM_ATTRS       attrs;      /* 内存属性 */
	void                *base;      /* 分配出的内存指针 */
	VCA_MEM_PLAT        plat;       /* 平台 */
} VCA_MEM_TAB_V2;

typedef struct _VCA_MEM_TAB_V3_
{
	size_t              size;       /* 以BYTE为单位的内存大小*/
	VCA_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
	VCA_MEM_SPACE       space;      /* 内存分配空间 */
	VCA_MEM_ATTRS       attrs;      /* 内存属性 */
	void                *base;      /* 分配出的内存指针 */
	void                *phy_base;  /* 分配出的物理内存指针,HISI平台可用 */
	VCA_MEM_PLAT        plat;       /* 平台 */
} VCA_MEM_TAB_V3;

//存储空间管理结构体
typedef struct _VCA_MEM_BUF_
{
    void     *start;             //缓存起始位置
    void     *end;               //缓存结束位置
    void     *cur_pos;           //缓存空余起始位置
}VCA_MEM_BUF;

//存储空间管理结构体
typedef struct _VCA_MEM_BUF_V2_
{
    void     *start;             //缓存起始位置
    void     *end;               //缓存结束位置
    void     *cur_pos;           //缓存空余起始位置
    size_t     start_phy;             //物理缓存起始位置
    size_t     end_phy;               //物理缓存结束位置
    size_t     cur_pos_phy;           //物理缓存空余起始位置
}VCA_MEM_BUF_V2;
/***********************************************************************************************************************
*图像结构体定义（新的结构体）
***********************************************************************************************************************/

//帧类型
typedef enum _VCA_YUV_FORMAT_
{ 
    VCA_YUV420  = 0,      //U/V在水平、垂直方向1/2下采样；Y通道为平面格式，U/V打包存储
    VCA_YUV422  = 1,      //U/V在水平方向1/2下采样；Y通道为平面格式，U/V打包存储
    VCA_YV12    = 2,      //U/V在水平和垂直方向1/2下采样；平面格式，按Y/V/U顺序存储
    VCA_UYVY    = 3,      //YUV422交叉，硬件下采样图像格式
    VCA_YUV444  = 4,      //U/V无下采样；平面格式存储
    VCA_YVU420  = 5,      //与VCA_YUV420的区别是V在前U在后
	VCA_BGR     = 6,      //BBBGGGRRR，BGR数据
	VCA_BGRGPU  = 7,      //BBBGGGRRR，BGR数据存储在显存上
    VCA_YUV_END = VCA_ENUM_END
}VCA_YUV_FORMAT;

//内存参数，涉及内存分配，参数修改需重新启动算法库
typedef struct _VCA_MEM_PARAM_
{
	int            product_type;    //产品类型
	int            image_w;         //图像宽
	int            image_h;         //图像高
	VCA_YUV_FORMAT format;          //图像格式
	unsigned int   frame_rate;      //帧率
	unsigned char  reserved[32];    //预留参数
}VCA_MEM_PARAM;    //52字节



typedef enum _VCA_MODEL_TYPE_
{
	VCA_MODEL_TYPE_INVALID		= 0,         // 无效

	// 车辆属性（VFR）
	VCA_MODEL_TYPE_VBR_FRONT   = 0x00000001,    // 品牌子品牌（正向）
	VCA_MODEL_TYPE_VCR_FRONT   = 0x00000002,    // 车身颜色（正向）
	VCA_MODEL_TYPE_VSD         = 0x00000003,    // 驾驶室特征
	VCA_MODEL_TYPE_SVD         = 0x00000004,    // 驾驶室特征
	VCA_MODEL_TYPE_UPD         = 0x00000005,    // 驾驶室特征
	VCA_MODEL_TYPE_VKD_0       = 0x00000006,    // 特征点模型
	VCA_MODEL_TYPE_VKD_1       = 0x00000007,    // 特征点模型
	VCA_MODEL_TYPE_VBR_BACK    = 0x00000008,    // 品牌子品牌（背向）
	VCA_MODEL_TYPE_VBR_NONVEH  = 0x00000009,    // 品牌子品牌（非机动车）
	VCA_MODEL_TYPE_VCR_BACK    = 0x0000000A,    // 车身颜色（背向）
	VCA_MODEL_TYPE_MOD         = 0x0000000B,    // 驾驶室特征(多目标)
	VCA_MODEL_TYPE_SBD         = 0x0000000C,    // 驾驶室特征(安全带)
	VCA_MODEL_TYPE_YLD         = 0x0000000D,    // 驾驶室特征(多目标)
	VCA_MODEL_TYPE_VBD         = 0x0000000E,    // 驾驶室特征(多目标)
	VCA_MODEL_TYPE_AVR         = 0x0000000F,    // 治安场景车辆属性
	VCA_MODEL_TYPE_VFB         = 0x00000010,    // 车辆正背向
	VCA_MODEL_TYPE_TSC         = 0x00000011,    // 治安场景分类识别
	VCA_MODEL_TYPE_VTR_VEH     = 0x00000012,    // 机动车车型识别
	VCA_MODEL_TYPE_VTR_NOV     = 0x00000013,    // 摩托车、电动车车型识别
	VCA_MODEL_TYPE_TPD         = 0x00000014,    // 驾驶室特征（临牌车车脸分类）
	VCA_MODEL_TYPE_BTIR_FRONT  = 0x00000015,    // 二三轮车违法载人
	VCA_MODEL_TYPE_VATR_FRONT  = 0x00000016,	// 附属物识别车脸模型
	VCA_MODEL_TYPE_VATR_BACK   = 0x00000017,	// 附属物识别车背模型
	VCA_MODEL_TYPE_VATR_TOP    = 0x00000018,	// 附属物识别车顶模型
	VCA_MODEL_TYPE_PDVS        = 0x00000020,    // 天窗站人
	VCA_MODEL_TYPE_TAXI        = 0x00000021,    // 出租车检测
	VCA_MODEL_TYPE_VAR_FRONT   = 0x00000022,    // 车辆属性识别（正向VAR）
    VCA_MODEL_TYPE_VAR_BACK    = 0x00000023,    // 车辆属性识别（背向VAR）
	VCA_MODEL_TYPE_CIR         = 0x00000024,    // 驾驶室特征（驾驶室成像质量分级模型）
	VCA_MODEL_TYPE_HMD_DAY     = 0x00000025,    // 头盔检测（白天模型）
	VCA_MODEL_TYPE_HMD_NT	   = 0x00000026,    // 头盔检测（夜晚模型） 
	VCA_MODEL_TYPE_SMD         = 0x00000027,    // 驾驶室特征（抽烟）
	VCA_MODEL_TYPE_VKD_CRK     = 0x00000028,    // 特征点模型（出入口）
	VCA_MODEL_TYPE_CMOBO       = 0x00000029,    // 基础分类模型
	VCA_MODEL_TYPE_MOD_OBJ     = 0x00000030,    // 驾驶室特征(物品检测)
	VCA_MODEL_TYPE_DBD         = 0x00000031,    // 驾驶室特征(危险品分类)
	VCA_MODEL_TYPE_DLD         = 0x00000031,    // 驾驶室特征(危险品分类),与DBD使用相同 
	VCA_MODEL_TYPE_VAR_C_FRONT = 0x00000032,    // 出入口场景 车辆外观识别（正向）
    VCA_MODEL_TYPE_VAR_C_BACK  = 0x00000033,    // 出入口场景 车辆外观识别（背向预留）
    VCA_MODEL_TYPE_MCD         = 0x00000034,    // 泥头车盖板状态判断模型
	VCA_MODEL_TYPE_VAR_LOGO    = 0x00000035,    // VAR车标检测模型
	VCA_MODEL_TYPE_BUR         = 0x00000036,    // 客车用途识别模型
	VCA_MODEL_TYPE_EVTR        = 0x00000037,    // 欧洲车型识别
	VCA_MODEL_TYPE_SLS         = 0x00000038,    // 限速标志识别	
	VCA_MODEL_TYPE_CHF         = 0x00000039,    // 驾驶室特征(副驾驶乘客) 
	VCA_MODEL_TYPE_BWMT        = 0x00000040,    // 二轮车多任务分类
    VCA_MODEL_TYPE_HMD_PFT     = 0x00000041,    // 头盔检测后分类模型（正向）
    VCA_MODEL_TYPE_HMD_PBC     = 0x00000042,    // 头盔检测后分类模型（背向）
	VCA_MODEL_TYPE_VTR_FRONT   = 0x00000043,    // 车型识别（正向）
    VCA_MODEL_TYPE_VTR_BACK    = 0x00000044,    // 车型识别 (背向)
    VCA_MODEL_TYPE_VCR		   = 0x00000045,    // 车身颜色混合

	//人员属性（PA）
	VCA_MODEL_TYPE_PA          = 0x00000101,     //人体属性

	//建模
	VCA_MODEL_TYPE_IRPR		   = 0x00000201,     // 人体建模
	VCA_MODEL_TYPE_IRVR		   = 0x00000202,     // 车辆建模
	VCA_MODEL_TYPE_IDX_IRPR	   = 0x00000203,     // 人体以图搜图索引模型
	VCA_MODEL_TYPE_IDX_IRVR	   = 0x00000204,     // 车辆以图搜图索引模型
	VCA_MODEL_TYPE_UP_IRPR     = 0x00000205,     // 人体以图搜图升级模型
    VCA_MODEL_TYPE_UP_IRVR     = 0x00000206,     // 车辆以图搜图升级模型
    VCA_MODEL_TYPE_IRCR        = 0x00000207,     // 商品以图搜图建模模型
    VCA_MODEL_TYPE_IRDR        = 0x00000208,     // 菜品以图搜图建模模型 
	VCA_MODEL_TYPE_IRPPR       = 0x00000209,     // 人体部件建模
	VCA_MODEL_TYPE_SEG         = 0x0000020A,     // 分割模型 
    VCA_MODEL_TYPE_IRLR        = 0x0000020B,     // 行李箱以图搜图建模模型
	VCA_MODEL_TYPE_IRNVR       = 0x0000020C,     // 非机动车以图搜图建模模型
    VCA_MODEL_TYPE_PREID       = 0x0000020D,     // 行人识别建模模型
    VCA_MODEL_TYPE_IDX_PREID   = 0x0000020E,     // 行人识别索引模型


	//目标分类模型
	VCA_MODEL_TYPE_CLS         = 0x00000301,     // 目标分类模型
	VCA_MODEL_TYPE_DIR_SHD     = 0x00000302,     // 目标遮挡朝向分类模型
	VCA_MODEL_TYPE_TRICYCLE_CLS = 0x00000303,    // 三轮车分类模型

	//行为分析模型	
	VCA_MODEL_TYPE_CDE			= 0x00000401,     // 人群密度估计模型
	VCA_MODEL_TYPE_VCB			= 0x00000402,	 // 剧烈运动视频分类模型
	VCA_MODEL_TYPE_FALL			= 0x00000403,	 // 人员倒地视频分类模型
	VCA_MODEL_TYPE_RUN			= 0x00000404,	 // 快速移动视频分类模型 
	VCA_MODEL_TYPE_CLOTH        = 0x00000405,	 // 制服分类模型  
	
   // 车牌模型
    VCA_MODEL_TYPE_LPR_FRCNN    = 0x00000501,    // 车牌FRCNN检测
    VCA_MODEL_TYPE_LPR_LSTM     = 0x00000502,    // 车牌LSTM字符识别
    VCA_MODEL_TYPE_LPR_ATT      = 0x00000503,    // 车牌Attention字符识别
    VCA_MODEL_TYPE_LPR_CNN     = 0x00000504,     // 车牌类型分类网络
	VCA_MODEL_TYPE_LPR_QUALITY = 0x00000505,     // 车牌质量评判网络
	VCA_MODEL_TYPE_LPR_YOLO    = 0x00000506,     // 车牌YOLO模型
	VCA_MODEL_TYPE_LPR_DETECT  = 0x00000507,     // 车牌检测模型
	VCA_MODEL_TYPE_LPR_RECOG   = 0x00000508,     // 车牌识别模型
	VCA_MODEL_TYPE_LPR_CLASS   = 0x00000509,     // 车牌分类模型
	VCA_MODEL_TYPE_LPR_ATT_FEA = 0x0000050A,     // 车牌识别模型-提取att-fea-map模型 59使用
	VCA_MODEL_TYPE_LPR_ATT_RNN = 0x0000050B,     // 车牌识别模型-RNN模型             59-dsp-使用
    VCA_MODEL_TYPE_LPR_STN_FEA = 0x0000050C,     // 车牌识别模型-提取stn-fea-map模型 59使用	 
	VCA_MODEL_TYPE_LPR_DETECT_2ND = 0x0000050D,  // 车牌第二个检测模型
	VCA_MODEL_TYPE_LPR_ATT_2ND = 0x0000050E,     // 车牌第2个识别模型
	VCA_MODEL_TYPE_LPR_PQE     = 0x0000050F,     //车牌PQE模型
	
	// 非机动车车牌模型
	VCA_MODEL_TYPE_NMLPR_DETECT     = 0x00000551,       //非机动车车牌检测模型
	VCA_MODEL_TYPE_NMLPR_DETECT_2ND = 0x00000552,       //非机动车车牌第二个检测模型
	VCA_MODEL_TYPE_NMLPR_CLASS      = 0x00000553,       //非机动车车牌分类模型
	VCA_MODEL_TYPE_NMLPR_RECOG      = 0x00000554,       //非机动车车牌识别模型1
	VCA_MODEL_TYPE_NMLPR_RECOG_2ND  = 0x00000555,       //非机动车车牌识别模型2
	VCA_MODEL_TYPE_NMLPR_QUALITY    = 0x00000556,       //非机动车车牌质量评判网络
	VCA_MODEL_TYPE_NMLPR_ATT_FEA    = 0x00000557,       //非机动车车牌识别模型-提取att-fea-map模型 59使用
	VCA_MODEL_TYPE_NMLPR_ATT_RNN    = 0x00000558,       //非机动车车牌识别模型-RNN模型             59-dsp-使用
	VCA_MODEL_TYPE_NMLPR_STN_FEA    = 0x00000559,       //非机动车车牌识别模型-提取stn-fea-map模型 59使用
	VCA_MODEL_TYPE_NMLPR_PQE        = 0x0000055A,       //非机动车车牌PQE模型
	
	//评分模型 
	VCA_MODEL_TYPE_FR_MM        = 0x000000601,    //人脸评分模型
	VCA_MODEL_TYPE_IQA_HUMAN = 0x000000602,     //人体评分模型
	VCA_MODEL_TYPE_IQA_TWO_WHEEL = 0x000000603, //二轮车评分模型
	VCA_MODEL_TYPE_IQA_THREE_WHEEL = 0x000000604, //三轮车评分模型
	
	//目标检测模型
	VCA_MODEL_TYPE_VTS_MOD      = 0x000000701,    // VTS视频触发检测模型
	VCA_MODEL_TYPE_HMS_OBD      = 0x000000702,    // HMS通用检测模型（全类别）
	VCA_MODEL_TYPE_PED_OBD      = 0x000000703,    // 人体通用检测模型（三类：人体、人脸、头肩）
	VCA_MODEL_TYPE_NOV_OBD      = 0x000000704,    // 非机动车通用检测模型
	 
	//目标分割模型
	VCA_MODEL_TYPE_VTS_MOS      = 0x000000801,    // VTS视频触发分割模型
	
	//相似性比对模型
	VCA_MODEL_TYPE_ASSO_POST = 0x000000901,       // 人脸人体关联后处理模型
	
	//特征聚类模型
	VCA_MODEL_TYPE_PR_CLSTR = 0x00000A01,         // 人员特征聚类模型
	
	
	// 集装箱箱号模型
 	VCA_MODEL_TYPE_CNR_MOD      = 0x0000F001,     // 箱号检测模型
	VCA_MODEL_TYPE_CNR_TR       = 0x0000F002,     // 箱号识别模型
    VCA_MODEL_TYPE_CNR_CLS      = 0x0000F003,     // 箱门识别模型
    VCA_MODEL_TYPE_CNR_ATT_FEA  = 0x0000F004,     // 箱号识别模型-提取att-fea-map模型 59使用
    VCA_MODEL_TYPE_CNR_ATT_RNN  = 0x0000F005,     // 箱号识别模型-RNN模型             59-dsp-使用
	
	// 拖车号模型
	VCA_MODEL_TYPE_CTR_MOD      = 0x0000F011,     // 拖车号检测模型
	VCA_MODEL_TYPE_CTR_TR       = 0x0000F012,     // 拖车号检测模型
			  
	// 身份证模型
 	VCA_MODEL_TYPE_IDCI_LMD       = 0x0000F021,     // 身份证定位模型
	VCA_MODEL_TYPE_IDCI_TR_ID     = 0x0000F022,     // 身份证号码识别模型
	VCA_MODEL_TYPE_IDCI_TR_BIRTH  = 0x0000F023,     // 身份证出生识别模型
	VCA_MODEL_TYPE_IDCI_TR_NAME   = 0x0000F024,     // 身份证姓名识别模型
	VCA_MODEL_TYPE_IDCI_TR_ADD    = 0x0000F025,     // 身份证地址识别模型
	VCA_MODEL_TYPE_IDCI_TR_SEX    = 0x0000F026,     // 身份证性别识别模型
	VCA_MODEL_TYPE_IDCI_TR_NATION = 0x0000F027,     // 身份证字符识别模型：MZ
	VCA_MODEL_TYPE_IDCI_LOI       = 0x0000F028,     // 身份证版面分析模型
	VCA_MODEL_TYPE_IDCI_DEC       = 0x0000F029,     // 身份证检测模型
	
	// 关键词检测模型
 	VCA_MODEL_TYPE_KWS_MOD        = 0x0000F031,     // 关键词检测模型
	VCA_MODEL_TYPE_KWS_TR         = 0x0000F032,     // 关键词检测模型
	VCA_MODEL_TYPE_DBA_MOD          = 0x000A0001,   // DBA模型（0x000A0xxx）
	VCA_MODEL_TYPE_DBA_LD           = 0x000A0002,
	VCA_MODEL_TYPE_DBA_SMD          = 0x000A0003,
	VCA_MODEL_TYPE_DBA_CALL         = 0x000A0004,
	VCA_MODEL_TYPE_DBA_TALK         = 0x000A0005,
	VCA_MODEL_TYPE_DBA_FDD          = 0x000A0006,
	VCA_MODEL_TYPE_DBA_SMD_LIGHT    = 0x000A0007,
	VCA_MODEL_TYPE_DBA_FACE_ATT     = 0x000A0008,
	VCA_MODEL_TYPE_DBA_FACE_YAWN    = 0x000A0009,
	VCA_MODEL_TYPE_DBA_BELT         = 0x000A000A,
	VCA_MODEL_TYPE_DBA_WHD          = 0x000A000B,
	VCA_MODEL_TYPE_DBA_HCD          = 0x000A000C,
	VCA_MODEL_TYPE_DBA_CLOTH        = 0x000A000D,
	VCA_MODEL_TYPE_DBA_HAND			= 0x000A000E,
	VCA_MODEL_TYPE_DBA_YAW          = 0x000A000F,
	VCA_MODEL_TYPE_DBA_PITCH        = 0x000A0010,
	VCA_MODEL_TYPE_DBA_SMF          = 0x000A0011,
    VCA_MODEL_TYPE_DBA_BMOD         = 0x000A0012,   // DBA模型（人脸路大模型检测）
    VCA_MODEL_TYPE_DBA_BELT_SEG     = 0x000A0013,   // DBA模型（人脸路安全带分割）
    VCA_MODEL_TYPE_DBA_FDDV_P1      = 0x000A0014,   // DBA模型（人脸路疲劳视频模型，part1）
    VCA_MODEL_TYPE_DBA_FDDV_P2      = 0x000A0015,   // DBA模型（人脸路疲劳视频模型，part2）

    VCA_MODEL_TYPE_DBA_BELTSEG_F    = 0x000A0016,   // DBA模型（头顶路正装安全带分割）
    VCA_MODEL_TYPE_DBA_BELTSEG_S    = 0x000A0017,   // DBA模型（头顶路侧装安全带分割）
    VCA_MODEL_TYPE_DBA_WHDV_P1      = 0x000A0018,   // DBA模型（头顶路方向盘脱把视频模型，part1）
    VCA_MODEL_TYPE_DBA_WHDV_P2      = 0x000A0019,   // DBA模型（头顶路方向盘脱把视频模型，part1）
	VCA_MODEL_TYPE_DBA_FDD_IQA		= 0x000A001A,   // DBA模型（人脸路疲劳IQA模型）
	
    //通用文字模型
    VCA_MODEL_TYPE_GOCR_TR        = 0x0000F041,     // 通用文字行识别模型
    VCA_MODEL_TYPE_GOCR_LOI       = 0x0000F042,     // 通用文字行定位模型
    VCA_MODEL_TYPE_GOCR_CRCG      = 0x0000F043,     // 通用文字单字识别模型
    VCA_MODEL_TYPE_GOCR_CLOI      = 0x0000F044,     // 通用文字单字检测模型
    VCA_MODEL_TYPE_GOCR_CTD       = 0x0000F045,     // 通用文字弯曲行检测模型
    
	// 汽车VIN码模型
	VCA_MODEL_TYPE_VNR_MOD      = 0x0000F051,     // 汽车VIN码检测模型
	VCA_MODEL_TYPE_VNR_TR       = 0x0000F052,     // 汽车VIN码识别模型
	
		// 工业OCR模型
	VCA_MODEL_TYPE_MV_MOD      = 0x000F0601,     // 工业OCR检测模型
	VCA_MODEL_TYPE_MV_TR       = 0x000F0602,     // 工业OCR识别模型
	
	VCA_MODEL_TYPE_TYPE_END	= VCA_ENUM_END,
}VCA_MODEL_TYPE;

//深度学习算法库应用数据类型索引
typedef enum _VCA_DL_PROCESS_TYPE_
{
	VCA_DL_PROC_UNKNOW          = 0,
    VCA_DL_PROC_FP32            = 1,              // FP32处理方式
	VCA_DL_PROC_FP16            = 2,              // FP16处理方式
	VCA_DL_PROC_INT8            = 3,              // FP32处理方式
	VCA_DL_PROC_INT8_HI_NNIE    = 4,              // HI NN 处理方式,不再使用，请使用VCA_DL_PROC_HI_NNIE
	VCA_DL_PROC_INT8_HI_DSP     = 5,              // HI DSP 处理方式，不再使用，请使用VCA_DL_PROC_HI_DSP
	VCA_DL_PROC_RT_INT8         = 6,              // TensorRT INT8处理方式
	VCA_DL_PROC_HI_NNIE_3D      = 7,              // HI NN 3D处理方式
	VCA_DL_PROC_HI_DSP_3D       = 8,              // HI DSP 3D处理方式
	VCA_DL_PROC_HI_NNIE         = 9,              // HI NN 处理方式
	VCA_DL_PROC_HI_DSP          = 10,             // HI DSP 处理方式	
	VCA_DL_PROC_NV_TENSORCORE   = 11,             // NVIDIA GPU tensorcore，支持xavier和T4，目前只支持int8 
	VCA_DL_PROC_NV_DLA          = 12,             // NVIDIA GPU DLA 处理方式，支持xavier，目前不支持，先占用字段
	VCA_DL_PROC_X86_FP32_C      = 13,
    VCA_DL_PROC_X86_FP32_SSE    = 14,
    VCA_DL_PROC_X86_FP32_AVX    = 15,
    VCA_DL_PROC_X86_FP32_AVX2   = 16,
    VCA_DL_PROC_X86_FP32_AVX512 = 17,
    VCA_DL_PROC_X86_FP32_ADJ    = 30,              //<  自适应， 中间可能还会加入其他指令集,预留
    VCA_DL_PROC_MLU_OFFLINE     = 31,              ///< MLU平台
    VCA_DL_PROC_MLU_ONLINE      = 32,              ///<MLU在线模式
    VCA_DL_PROC_BITMAIN         = 33,              ///< 比特大陆处理方式
	VCA_DL_PROC_K91             = 34,              // 支持K91平台DL处理模式
	VCA_DL_PROC_G5              = 35,              // 支持前端G5系列，后端A7A8A9等NT平台DL处理模式
	VCA_DL_PROC_HD              = 36,              // HD平台处理方式
	VCA_DL_PROC_ARM_GPU         = 37,              // ARM平台 使用移动端GPU推理
    VCA_DL_PROC_ARM_ADJ         = 38,              // 自适应模式，中间可能还会加入其它的处理类型，预留
	VCA_DL_PROC_RN              = 39,              // RN平台处理方式
	VCA_DL_PROC_SN              = 40,              // SN平台处理方式
	VCA_DL_PROC_END	= VCA_ENUM_END_V2,
}VCA_DL_PROCESS_TYPE;

//模型信息
typedef struct _VCA_MODEL_INFO_
{
	int            model_type;                      // 模型类型,参考VCA_MODE_TYPE和HIKFASTER_RCNN_MODEL_TYPE
	unsigned int   model_size;                      // 模型尺寸
	void          *model_buf;                       // 模型内存
	int            dl_proc_type;                    // 详见VCA_DL_PROCESS_TYPE
	char           reserved[60];
} VCA_MODEL_INFO;

//模型列表
typedef struct _VCA_MODEL_LIST_
{
	int            model_num;                       // 模型数量
	VCA_MODEL_INFO model_info[VCA_MAX_MODEL_NUM];   // 模型信息
} VCA_MODEL_LIST; 

// 模型参数
typedef struct _VCA_MODEL_PARAM_
{
	VCA_MODEL_LIST model_list;
} VCA_MODEL_PARAM; 


//图像帧数据
typedef struct _VCA_YUV_DATA_
{
    unsigned short image_w;            //图像宽度
    unsigned short image_h;            //图像高度
    unsigned int   pitch_y;            //图像y处理跨度
    unsigned int   pitch_uv;           //图像uv处理跨度
    VCA_YUV_FORMAT format;             //YUV格式
    unsigned char  *y;                 //y数据指针
    unsigned char  *u;                 //u数据指针
    unsigned char  *v;                 //v数据指针
	unsigned char  scale_rate;         //下采样倍率，0或1表示原图，其他为降采样倍率如2，4，8
    unsigned char  reserved[15];       //预留16个字节
}VCA_YUV_DATA;    //44字节

//时间信息
typedef struct _VCA_DATE_TIME_
{
    short year;
    short month;
    short day_of_week;
    short day;
    short hour;
    short minute;
    short second;
    short milliseconds;
} VCA_DATE_TIME;    //16字节

//帧头信息
typedef struct _VCA_FRAME_HEADER_
{
    unsigned int   frame_rate;         //帧率
    unsigned int   frame_num;          //帧号
    unsigned int   time_stamp;         //时间戳（单位毫秒）
    VCA_DATE_TIME  sys_time;           //系统时间
    unsigned char  reserved[32];       //预留32个字节
}VCA_FRAME_HEADER;    //60字节

//视频输入帧
typedef struct _VCA_VIDEO_DATA_
{
    VCA_FRAME_HEADER   frame_header;    //帧头信息
    VCA_YUV_DATA       yuv_frame;       //YUV数据指针
}VCA_VIDEO_DATA;    //104字节

typedef struct _VCA_MEM_PARAM_V2_
{
	int            product_type;     // 产品类型
	VCA_VIDEO_DATA frame;            // 图像信息
	void           *cuda_handle;     // GPU平台为cuda句柄，HISI平台为schedule_handle
	void           *modle_handle;    // 模型句柄
	char            reserved[64];
} VCA_MEM_PARAM_V2; 


/***********************************************************************************************************************
*目标结构体
***********************************************************************************************************************/
// 目标检测所使用的技术
typedef enum _VCA_DET_TECH_
{
	VCA_DET_TECH_UNKNOWN   = 0,   // 未知检测方法
	VCA_DET_TECH_BGS       = 1,   // 背景建模方法
	VCA_DET_TECH_PLATE     = 2,   // 牌识检测方法
	VCA_DET_TECH_BOOST     = 3,   // BOOST检测方法
	VCA_DET_TECH_DPM       = 4,   // DPM检测方法
	VCA_DET_TECH_CNN       = 5,   // CNN检测方法
	VCA_DET_TECH_END       = 16
} VCA_DET_TECH;

// 目标检测出来的类型
typedef enum _VCA_DET_TYPE_
{
	VCA_DET_TYPE_UNKNOWN     = 0,   // 未知的类型
	VCA_DET_TYPE_FACE        = 1,   // 人脸
	VCA_DET_TYPE_HEAD        = 2,   // 人头
	VCA_DET_TYPE_HESH        = 3,   // 头肩
	VCA_DET_TYPE_PED         = 4,   // 人体
	VCA_DET_TYPE_PLATE       = 5,   // 车牌
	VCA_DET_TYPE_VEHICLE     = 6,   // 车辆
	VCA_DET_TYPE_CONE        = 7,   // 三角堆
	VCA_DET_TYPE_TWO_WHEEL   = 8,   // 二轮车
	VCA_DET_TYPE_THREE_WHEEL = 9,   // 三轮车
	VCA_DET_TYPE_END         = 64
} VCA_DET_TYPE;

// 目标检测出来的方向
typedef enum _VCA_DET_DIRECTION_
{
	VCA_DET_DIRECTION_UNKNOWN    = 0,   // 未知的方向
	VCA_DET_DIRECTION_FRONT      = 1,   // 正向
	VCA_DET_DIRECTION_TAIL       = 2,   // 背向
	VCA_DET_DIRECTION_LEFT       = 3,   // 向左
	VCA_DET_DIRECTION_RIGHT      = 4,   // 向右
	VCA_DET_DIRECTION_END        = 16
} VCA_DET_DIRECTION;

// 车辆组件
typedef enum _VCA_VEH_PARTS_
{
	VCA_VEH_UNKNOWN              = 0,   // 未知的方向
	VCA_VEH_LIGHT                = 1,   // 车灯
	VCA_VEH_WINDOW               = 2,   // 车窗
	VCA_VEH_END                  = 16
} VCA_VEH_PARTS;

/***********************************************************************************************************************
*目标检测方法
*高4位为检测方法编号（最多16个模块），高5-12位为目标类型编号（最多64种类型），高13-16位为方向编号（最多16种类型）
*检测方法参考VCA_DET_TECH结构体
*目标类型参考VCA_DET_TYPE结构体
*方向参考VCA_DET_DIRECTION结构体
***********************************************************************************************************************/
typedef enum _VCA_DET_METHOD_
{
	VCA_DET_INVALID = -1,  //检测方法无效

	// 背景建模检测方法
	VCA_DET_BGS = ((VCA_DET_TECH_BGS << 28) | (VCA_DET_TYPE_UNKNOWN << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
    VCA_DET_BGS_VEH_FRONT_LIGHT   = ((VCA_DET_TECH_BGS    << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_FRONT << 18) | (VCA_VEH_LIGHT << 14)),
    VCA_DET_BGS_VEH_TAIL_LIGHT    = ((VCA_DET_TECH_BGS    << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_TAIL  << 18) | (VCA_VEH_LIGHT << 14)),

	// 牌识检测方法
	VCA_DET_PLATE = ((VCA_DET_TECH_PLATE << 28) | (VCA_DET_TYPE_PLATE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),

	// BOOST检测方法
	VCA_DET_BOOST_FACE = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_FACE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_BOOST_PED = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_BOOST_PED_HESH = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_TYPE_HESH << 18)),
	VCA_DET_BOOST_VEH = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_BOOST_VEH_FRONT = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_FRONT << 18)),
	VCA_DET_BOOST_VEH_TAIL = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_TAIL << 18)),
	VCA_DET_BOOST_VEH_FRONT_LIGHT = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_FRONT << 18) | (VCA_VEH_LIGHT << 14)),
	VCA_DET_BOOST_VEH_TAIL_LIGHT = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_TAIL << 18) | (VCA_VEH_LIGHT << 14)),
	VCA_DET_BOOST_VEH_FRONT_WIN = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_FRONT << 18) | (VCA_VEH_WINDOW << 14)),
	VCA_DET_BOOST_VEH_TAIL_WIN = ((VCA_DET_TECH_BOOST << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_TAIL << 18) | (VCA_VEH_WINDOW << 14)),

	// DPM检测方法
	// 人头
	VCA_DET_DPM_HEAD = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HEAD << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_DPM_HEAD_FRONT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HEAD << 22) | (VCA_DET_DIRECTION_FRONT << 18)),
	VCA_DET_DPM_HEAD_TAIL = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HEAD << 22) | (VCA_DET_DIRECTION_TAIL << 18)),
	VCA_DET_DPM_HEAD_LEFT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HEAD << 22) | (VCA_DET_DIRECTION_LEFT << 18)),
	VCA_DET_DPM_HEAD_RIGHT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HEAD << 22) | (VCA_DET_DIRECTION_RIGHT << 18)),

	// 头肩
	VCA_DET_DPM_HESH = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HESH << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_DPM_HESH_FRONT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HESH << 22) | (VCA_DET_DIRECTION_FRONT << 18)),
	VCA_DET_DPM_HESH_TAIL = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HESH << 22) | (VCA_DET_DIRECTION_TAIL << 18)),
	VCA_DET_DPM_HESH_LEFT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HESH << 22) | (VCA_DET_DIRECTION_LEFT << 18)),
	VCA_DET_DPM_HESH_RIGHT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_HESH << 22) | (VCA_DET_DIRECTION_RIGHT << 18)),

	// 人体
	VCA_DET_DPM_PED = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_DPM_PED_FRONT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_FRONT << 18)),
	VCA_DET_DPM_PED_TAIL = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_TAIL << 18)),
	VCA_DET_DPM_PED_LEFT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_LEFT << 18)),
	VCA_DET_DPM_PED_RIGHT = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_RIGHT << 18)),

	// 车辆，车辆目前不能输出方向，只能输出小车和大车，用VCA_OBJ_TYPE描述
	VCA_DET_DPM_VEH = ((VCA_DET_TECH_DPM << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),

	//FRCNN目标检测方法
	VCA_DET_CNN_PED         = ((VCA_DET_TECH_CNN << 28) | (VCA_DET_TYPE_PED << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_CNN_VEHICLE     = ((VCA_DET_TECH_CNN << 28) | (VCA_DET_TYPE_VEHICLE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_CNN_FACE        = ((VCA_DET_TECH_CNN << 28) | (VCA_DET_TYPE_FACE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_CNN_CONE        = ((VCA_DET_TECH_CNN << 28) | (VCA_DET_TYPE_CONE << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_CNN_TWO_WHEEL   = ((VCA_DET_TECH_CNN << 28) | (VCA_DET_TYPE_TWO_WHEEL << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),
	VCA_DET_CNN_THREE_WHEEL = ((VCA_DET_TECH_CNN << 28) | (VCA_DET_TYPE_THREE_WHEEL << 22) | (VCA_DET_DIRECTION_UNKNOWN << 18)),

    VCA_DET_END              = VCA_ENUM_END
} VCA_DET_METHOD;

//目标类型
typedef enum _VCA_OBJ_TYPE_
{
    VCA_OBJ_UNKNOWN        = 0,         //未知
    VCA_OBJ_NEGATIVE       = 0x01,      //无效目标
    VCA_OBJ_LARGE_VEHICLE  = 0x10,      //大车
    VCA_OBJ_SMALL_VEHICLE  = 0x20,      //小车
    VCA_OBJ_MOTOR          = 0x30,      //摩托车
    VCA_OBJ_HUMAN          = 0x40,      //行人    
    VCA_OBJ_NON_VEHICLE    = 0x50,      //非四轮车
    VCA_OBJ_TWO_WHEEL      = 0x60,      //二轮车
    VCA_OBJ_THREE_WHEEL    = 0x70,      //三轮车  
	VCA_OBJ_FACE           = 0x80,      //人脸
	VCA_OBJ_CONE           = 0x90,      //三角堆
	VCA_OBJ_BUS            = 0x100,     //公交
    VCA_OBJ_TRUCK          = 0x110,     //货车
    VCA_OBJ_SMOG           = 0x120,     //烟雾
    VCA_OBJ_FLAME          = 0x130,     //火灾
    VCA_OBJ_BANNER         = 0x140,     //条幅
    VCA_OBJ_BOOTH          = 0x150,     //摊位
	VCA_OBJ_HESH           = 0x160,     //头肩
	VCA_OBJ_HEAD           = 0x170,     //人头

	VCA_OBJ_TRAFFICSIGN    = 0x200,     //交通标志，包括限速标志、限重标志等
	VCA_OBJ_TRAFFICLIGHT   = 0x201,     //交通灯
	VCA_OBJ_LANE           = 0x202,     //车道线
	VCA_OBJ_ROADEDGE       = 0x203,     //路沿
	VCA_OBJ_ROADMARKING    = 0x204,     //路面标志
	VCA_OBJ_WHEEL          = 0x205,     //车轮
	VCA_OBJ_VEHICLE_HEAD   		= 0x206, //车头
    VCA_OBJ_VEHICLE_TAIL   		= 0x207, //车尾
    VCA_OBJ_VEHICLE_FRONT_FACE  = 0x208, //车前脸
    VCA_OBJ_VEHICLE_BACK_FACE   = 0x209, //车后脸
    VCA_OBJ_PLATE				= 0x20A, //车牌
    VCA_OBJ_CAR_WINDOW	        = 0x20B, //车窗
    
    VCA_OBJ_ROAD                = 0x20C, // 路面
	VCA_OBJ_ZEBRA               = 0x20D, // 斑马线
    VCA_OBJ_GENERAL             = 0x20E, // 通用障碍物
	
	VCA_OBJ_AIRCRAFT            = 0x301, // 飞机
	VCA_OBJ_UVA                 = 0x302, // 无人机
    VCA_OBJ_SHIP                = 0x303, // 船舶

	//行业定制目标类别
	VCA_OBJ_POLICE              = 0x401,//警察
	VCA_OBJ_POLICE_VEH          = 0x402,//警车
	VCA_OBJ_ROAD_CONSTRUCTOR    = 0x403,//施工人员
	VCA_OBJ_CONSTRUCTION_VEH    = 0x404,//施工车
    VCA_OBJ_END            = VCA_ENUM_END
} VCA_OBJ_TYPE;

//目标基本信息
typedef  struct _VCA_OBJ_INFO_
{
    int                 valid;          //是否有效
    int                 id;             //（参考）ID号，不能确定时为0
    VCA_DET_METHOD      det_method;     //检测方法
    VCA_OBJ_TYPE        type;           //目标类型
    VCA_RECT_F          rect;           //目标位置
    int                 confidence;     //检测置信度
    unsigned char       reserved[16];   //预留16个字节
} VCA_OBJ_INFO;    //52字节

// 目标链表
typedef struct _VCA_OBJ_LIST_
{
    int             obj_num;              //目标个数
	unsigned char   reserved[16];         //预留16个字节
    VCA_OBJ_INFO    obj[VCA_MAX_OBJ_NUM]; //每个目标信息
}VCA_OBJ_LIST;

typedef struct _VCA_OBJ_LIST_V2_
{
	int             obj_num;              //目标个数
	unsigned char   reserved[16];         //预留16个字节
	VCA_OBJ_INFO    *obj; //每个目标信息
}VCA_OBJ_LIST_V2;
typedef struct _VCA_OBJ_LIST_V3_
{
    int             obj_num;              //目标个数
	unsigned char   reserved[16];         //预留16个字节
    VCA_OBJ_INFO    obj[1024];            //每个目标信息
}VCA_OBJ_LIST_V3;   //交通算法专用，5140字节

// 颜色类别
typedef enum _VCA_COLOR_TYPE_
{
    VCA_CLR_UNSUPPORT  = 0,               //不支持
    VCA_CLR_RED        = 1,               //红
    VCA_CLR_YELLOW     = 2,               //黄 
    VCA_CLR_GREEN      = 3,               //绿
    VCA_CLR_GREENB     = 4,               //青
    VCA_CLR_BLUE       = 5,               //蓝
    VCA_CLR_PURPLE     = 6,               //紫
    VCA_CLR_PINK       = 7,               //粉
    VCA_CLR_BROWN      = 8,               //棕
    VCA_CLR_WHITE      = 9,               //白
    VCA_CLR_GRAY       = 10,              //灰
    VCA_CLR_BLACK      = 11,              //黑
	VCA_CLR_ORANGE     = 12,              //橙
	VCA_CLR_MIXTURE    = 13,              //混色
    VCA_CLR_END        = VCA_ENUM_END
}VCA_COLOR_TYPE;

//跟踪状态枚举
typedef enum _VCA_TR_STATE_
{
    VCA_TR_FREE         = 0,            //状态位空闲
    VCA_TR_ON           = 1,            //跟踪进行
    VCA_TR_LOST         = 2,            //跟踪丢失
    VCA_TR_CANCLE       = 3,            //跟踪取消
    VCA_TR_ABNORMAL     = 4,            //跟踪异常
    VCA_TR_END          = VCA_ENUM_END
}VCA_TR_STATE;

//目标跟踪结果
typedef  struct _VCA_TR_INFO_
{
    VCA_TR_STATE        tr_state;       //目标的输出跟踪状态
    int                 id;             //（参考）ID号，不能确定时为0
    VCA_RECT_F          rect;           //目标位置
    int                 confidence;     //跟踪置信度
    VCA_POINT_F         move_dis;       //当前目标参考移动量
    VCA_DET_METHOD      det_method;     //检测方法
    unsigned char       reserved[12];   //预留12个字节
}VCA_TR_INFO;    //52字节

typedef  struct _VCA_TR_INFO_V2_
{
	VCA_TR_STATE             state;                        //跟踪状态
	int                      id;                           //目标编号
	VCA_RECT_I               rect;                         //矩形框位置
	VCA_OBJ_TYPE             type;                         //目标类型
	int                      confidence;                   //跟踪置信度

	VCA_DET_METHOD           det_method;                   //检测方法
	int                      asso_det_id;                  //被关联的检测目标编号
	int                      det_frm_cnt;                  //关联检测帧数累计
	int                      lost_frm_cnt;                 //丢失帧数累计
	int                      trk_frm_cnt;                  //跟踪帧数累计（从目标新建开始）

	void                    *attr;                         //目标属性（透传）

	VCA_POINT_F              move_dis;                     //单帧移动距离（亚像素级）
	float                    size_scale;                   //单帧目标尺度缩放比率
	float                    move_dis_conf;                //单帧移动距离置信度
	int                      move_dis_flag;                //单帧移动距离有效标志
	int                      static_flag;                  //目标静止标志

	unsigned char            reserved[16];                 //预留字节
}VCA_TR_INFO_V2;    //96字节

typedef  struct _VCA_TR_INFO_V3_
{
	VCA_TR_STATE             state;                        //跟踪状态
	int                      id;                           //目标编号
	VCA_RECT_F               rect;                         //矩形框位置
	VCA_OBJ_TYPE             type;                         //目标类型
	int                      confidence;                   //跟踪置信度

	VCA_DET_METHOD           det_method;                   //检测方法
	int                      asso_det_id;                  //被关联的检测目标编号
	int                      det_frm_cnt;                  //关联检测帧数累计
	int                      lost_frm_cnt;                 //丢失帧数累计
	int                      trk_frm_cnt;                  //跟踪帧数累计（从目标新建开始）

	void                    *attr;                         //目标属性（透传）

	VCA_POINT_F              move_dis;                     //单帧移动距离（亚像素级）
	float                    size_scale;                   //单帧目标尺度缩放比率
	float                    move_dis_conf;                //单帧移动距离置信度
	int                      move_dis_flag;                //单帧移动距离有效标志
	int                      static_flag;                  //目标静止标志

	unsigned char            reserved[16];                 //预留字节
}VCA_TR_INFO_V3;    //96字节

// 目标链表（跟踪）
typedef struct _VCA_OBJ_TR_LIST_
{
    int             obj_num;              //目标个数
    unsigned char   reserved[16];         //预留16个字节
    VCA_TR_INFO     obj[VCA_MAX_OBJ_NUM]; //每个目标信息
}VCA_OBJ_TR_LIST;

/***********************************************************************************************************************
* V2.0新增数据
***********************************************************************************************************************/

#define VCA_MAX_MEM_TAB_NUM         32              // 最大table数量
#define VCA_MAP_MAX_DIM             8               // map支持最大维度
#define VCA_IMG_MAX_CHANEL          5               // 单帧图像支持的最大通道数
#define VCA_MAX_IMG_RESOLUTION_NUM  3               //一帧图像支持的分辨率数量
#define VCA_MODEL_TYPE_LENGTH       36              // 模型打包model_type信息长度
/***********************************************************************************************************************
* 结构体
***********************************************************************************************************************/
//数据类型
typedef enum _VCA_DATA_TYPE_
{
	VCA_DATA_U08 = 0x00000001,
	VCA_DATA_S08 = 0x00000002,

	VCA_DATA_U16 = 0x10000001,
	VCA_DATA_S16 = 0x10000002,
	VCA_DATA_F16 = 0x10000003,

	VCA_DATA_U32 = 0x20000001,
	VCA_DATA_S32 = 0x20000002,
	VCA_DATA_F32 = 0x20000003,

	VCA_DATA_U64 = 0x30000001,
	VCA_DATA_S64 = 0x30000002,
	VCA_DATA_F64 = 0x30000003,

	VCA_DATA_END = VCA_ENUM_END_V2
}VCA_DATA_TYPE;


// 图像数据类型
typedef enum _VCA_IMG_FORMAT_
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
	VCA_IMG_MONO_08 = 1,    //| 8b   | U08  |        D0           |         S0         |
	//mono12                            //----------------------------------------------------------
	VCA_IMG_MONO_12 = 2,    //| 12b  | S16  |        D0           |         S0         |
	//mono16                            //----------------------------------------------------------
	VCA_IMG_MONO_16 = 3,    //| 16b  | U16  |        D0           |         S0         |
	//----------------------------------//----------------------------------------------------------
	//YUV (100~199)                     //| bit  | type |   store position    |        step        |
	//YUV420 I420                       //----------------------------------------------------------
	VCA_IMG_YUV_I420 = 100,  //| 8b   | U08  |    Y:D0,U:D1,V:D2   |  Y:S0,U:S1,V:S2    |
	//YUV420 YV12                       //----------------------------------------------------------
	VCA_IMG_YUV_YV12 = 101,  //| 8b   | U08  |    Y:D0,V:D1,U:D2   |  Y:S0,V:S1,U:S2    |
	//YUV420 NV12                       //----------------------------------------------------------
	VCA_IMG_YUV_NV12 = 102,  //| 8b   | U08  |    Y:D0,UV:D1       |  Y:S0,UV:S1        |
	//YUV422 3 plane                    //----------------------------------------------------------
	VCA_IMG_YUV_422 = 103,  //| 8b   | U08  |    Y:D0,U:D1,V:D2   |  Y:S0,U:S1,V:S2    |
	//YUV422 UYVY                       //----------------------------------------------------------
	VCA_IMG_YUV_UYVY = 104,  //| 8b   | U08  |    YUV:D0           |  YUV:S0            |
	//YUV444 3 plane                    //----------------------------------------------------------
	VCA_IMG_YUV_444 = 105,  //| 8b   | U08  |    Y:D0,U:D1,V:D2   |  Y:S0,U:S1,V:S2    |
	//YUV420 NV21  
	VCA_IMG_YUV_NV21   = 106,  //| 8b   | U08  |   Y:D0,VU:D1       |  Y:S0,VU:S1   |
	//----------------------------------//----------------------------------------------------------
	//RGB (200~299)                     //| bit  | type |   store position    |        step        |
	//RBG 3 plane                       //----------------------------------------------------------
	VCA_IMG_RGB_RGB24_P3 = 200,  //| 8b   | U08  |    R:D0,G:D1,B:D2   |  R:S0,G:S1,B:S2    |
	//RGB 3 RGBRGB...                   //----------------------------------------------------------
	VCA_IMG_RGB_RGB24_C3 = 201,  //| 8b   | U08  |    RGB:D0           |  RGB:S0            |
	//RGB 4 plane(alpha)                //----------------------------------------------------------
	VCA_IMG_RGB_RGBA_P4 = 202,  //| 8b   | U08  | R:D0,G:D1,B:D2,A:D3 | R:S0,G:S1,B:S2,A:S3|
	//RGB 4 RGBARGBA...(alpha)          //----------------------------------------------------------
	VCA_IMG_RGB_RGBA_C4 = 203,  //| 8b   | U08  |    RGBA:D0          |  RGBA:S0           |
	//RGB 4 plane(depth)                //----------------------------------------------------------
	VCA_IMG_RGB_RGBD_P4 = 204,  //| 8b   | U08  | R:D0,G:D1,B:D2,A:D3 | R:S0,G:S1,B:S2,A:S3|
	//RGB 4 RGBDRGBD...(depth)          //----------------------------------------------------------
	VCA_IMG_RGB_RGBD_C4 = 205,  //| 8b   | U08  |    RGBA:D0          |  RGBA:S0           |
	//BGR 3 plane                       //----------------------------------------------------------
	VCA_IMG_RGB_BGR24_P3 = 206,  //| 8b   | U08  |    B:D0,G:D1,R:D2   |  B:S0,G:S1,R:S2    |
	//BGR 3 BGRBGR...                   //----------------------------------------------------------
	VCA_IMG_RGB_BGR24_C3 = 207,  //| 8b   | U08  |    BGR:D0           |  BGR:S0            |
	//----------------------------------//----------------------------------------------------------
	//bayer (300~399)                   //| bit  | type |   store position    |        step        |
	//bayer GRGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_GRBG_10 = 300,  //| 10b  | S16  |    GRBG:D0          |  GRBG:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_RGGB_10 = 301,  //| 10b  | S16  |    RGGB:D0          |  RGGB:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_BGGR_10 = 302,  //| 10b  | S16  |    BGGR:D0          |  BGGR:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_GBRG_10 = 303,  //| 10b  | S16  |    GBRG:D0          |  GBRG:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_GRBG_12 = 304,  //| 12b  | S16  |    GRBG:D0          |  GRBG:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_RGGB_12 = 305,  //| 12b  | S16  |    RGGB:D0          |  RGGB:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_BGGR_12 = 306,  //| 12b  | S16  |    BGGR:D0          |  BGGR:S0           |
	//bayer RGGB 10bit                  //----------------------------------------------------------
	VCA_IMG_BAYER_GBRG_12 = 307,  //| 12b  | S16  |    GBRG:D0          |  GBRG:S0           |
	//----------------------------------//----------------------------------------------------------

	// code img
	VCA_IMG_JPG = 400,

	// metadata 特征图等相关类型数据结构(500-599) 
	VCA_META_FLOW_FEAT = 500,			//	特征图数据
	VCA_IMG_END = VCA_ENUM_END_V2
}VCA_IMG_FORMAT;


/***********************************************************************************************************************
基本参数定义
***********************************************************************************************************************/
/***********************************************************************************************************************
*输入音频帧结构体
************************************************************************************************************************/
//音频帧头信息
typedef struct _VCA_AUDIO_FRAME_V2_
{
	unsigned int     channel_num;                           // 声道数 
	unsigned int     sample_rate;                           // 采样率 
	unsigned int     in_data_len;                           // 每帧输入数据样点数  
	unsigned int     bits_per_sample;                       // 每个采样点的bit数，公司现有的音频数据都是16bit
	unsigned int     reserved[4];                           // 保留字节
	VCA_DATE_TIME    time_stamp;                            // 时标
	short            *audio_data;                         // 音频数据，输入数据指针 
}VCA_AUDIO_FRAME_V2;



typedef enum _VCA_MAP_NAME_
{
	VCA_MAP_MASK   = 0x00000001,                          // 掩码
	VCA_MAP_DIFF   = 0x00000003,                          // 帧差
	VCA_MAP_DEPTH  = 0x00000004,                          // 深度图
	VCA_MAP_MOTION = 0x00000005,                          // 光流图
	VCA_MAP_FEAT   = 0x00000006,                          // 特征图
	VCA_MAP_END = VCA_ENUM_END_V2
}VCA_MAP_NAME;

//图结构体：前景图、特征图等
typedef struct _VCA_MAP_
{
	unsigned int         shape_num;						  // 实际使用的图维度个数
	unsigned int         shape[VCA_MAP_MAX_DIM];	      // 图维度（n、c、l，h，w）
	unsigned int         data_type;                       // 数据类型VCA_DATA_TYPE
	unsigned int         data_mem_type;                   // 图像数据所在平台VCA_MEM_PLAT
	unsigned int         map_name;                        // 预留自定义数据名称VCA_MAP_NAME
	void                *map_data;                        // 数据
}VCA_MAP;

// 图结构体链表
typedef struct _VCA_MAP_LIST_
{
	int                   map_num;						 // 图数量
	VCA_MAP              *map;						     // 图信息
	unsigned char         reserved[16];
} VCA_MAP_LIST;


//图像
typedef struct _VCA_IMG_
{
	unsigned int          data_format;                    // 图像格式，按照数据类型VCA_IMG_FORMAT赋值
	unsigned int          data_mem_type;                  // 图像所在的内存类型
	unsigned int          data_type;                      // 图像数据类型
	unsigned int          data_lenth;                     // 图像数据长度，一般JPG数据会使用

	unsigned int          img_w;                          // 图像宽度
	unsigned int          img_h;                          // 图像高度
	unsigned int          stride_w[VCA_IMG_MAX_CHANEL];   // 每个通道行间距
	unsigned int          stride_h[VCA_IMG_MAX_CHANEL];   // 每个通道列间距
	void                 *data[VCA_IMG_MAX_CHANEL];       // 数据存储地址
}VCA_IMG;//72~88BYTE

typedef struct _VCA_VIDEO_FRAME_V2_
{
	VCA_FRAME_HEADER		frame_header;
	VCA_IMG					img_data[VCA_MAX_IMG_RESOLUTION_NUM];
}VCA_VIDEO_FRAME_V2;


typedef struct _VCA_MEDIA_FRAME_
{
	VCA_VIDEO_FRAME_V2 video;
	VCA_AUDIO_FRAME_V2 audio;
}VCA_MEDIA_FRAME;

typedef struct _VCA_MEDIA_LIST_
{
	unsigned int					media_num;
	VCA_MEDIA_FRAME				   *media;
}VCA_MEDIA_LIST;

//区域类型
typedef enum _VCA_REGION_TYPE_
{
	VCA_REGION_POLYLINE    = 1,                            // 折线
	VCA_REGION_POLYGON     = 2,					           // 多边形
	VCA_REGION_RECT        = 3,                            // 矩形
	VCA_REGION_ROTATE_RECT = 4,							   //旋转矩形
	VCA_REGION_END = VCA_ENUM_END_V2
}VCA_REGION_TYPE;

//旋转矩形
typedef struct _VCA_ROTATE_RECT_F_
{
	float				  cx;								// 矩形中心点X轴坐标
	float				  cy;								// 矩形中心点Y轴坐标
	float				  width;							// 矩形宽度
	float				  height;							// 矩形高度
	float				  theta;							// 旋转矩形角度
}VCA_ROTATE_RECT_F;
//区域联合体
typedef struct _VCA_REGION_
{
	unsigned int region_type;                               // 参考VCA_REGION_TYPE
	char         reserved[12];
	union
	{
		unsigned char		size[84];
		VCA_POLYGON_F       polygon;                        // 多边形/直线/折线
		VCA_RECT_F          rect;                           // 矩形
		VCA_ROTATE_RECT_F	rotate_rect;	 				// 旋转矩形
	};

}VCA_REGION; //100 BYTE

/***********************************************************************************************************************
多模态传感器结构体定义
***********************************************************************************************************************/

//传感器种类
typedef enum _VCA_SENSOR_TYPE_
{
	VCA_SENSOR_LIDAR	= 1,                       // Lidar
	VCA_SENSOR_RADAR	= 2,					   // Radar
	VCA_SENSOR_END		= VCA_ENUM_END_V2
}VCA_SENSOR_TYPE;

// 毫米波雷达3D点信息
typedef struct _VCA_RADAR_POINT_INF_
{
	short					snr;                // 雷达信噪比，dB
	short					rcs;                // 雷达反射截面积	
	unsigned char			reserved[44];		// 保留44字节
}VCA_RADAR_PT_INF;  // 48字节


// 激光雷达3D点心  
typedef struct _VCA_LIDAR_POINT_INF_
{
	unsigned short			ring;				// 点云线束
	unsigned char			reflectivity;		// 点云反射强度，0-255 
	unsigned char			reserved[45];		// 保留45字节
}VCA_LIDAR_PT_INF;  // 48字节


//多模态传感器三维点信息 (浮点型)： 有3D空间点的同时，具有传感器的信息，如速度、时间、反射率等
typedef struct _VCA_3D_SENSOR_POINT_F_
{
	VCA_3D_POINT_F			point;				// 点云坐标，x，y，z，单位m
	VCA_3D_POINT_F			velocity;			// 点云速度, x，y，z，3个方向
	unsigned int			time_stamp;			// 时间戳（单位毫秒）
	unsigned char			reserved[20];		// 保留20字节
	union
	{
		unsigned char		inf_size[48];		// 不同传感器扩展信息，48个字节
		VCA_RADAR_PT_INF    radar_pt_inf;		// 毫米波雷达3D点信息
		VCA_LIDAR_PT_INF    lidar_pt_inf;		// 激光雷达3D点心   
		 
	}; 
	
}VCA_3D_SENSOR_POINT_F;  // 96字节

//三维点信息链表
typedef struct _VCA_3D_SENSOR_POINT_F_LIST_
{
	unsigned int			 pts_num;			// 数量                         
	unsigned int			 frame_id;			// 帧号  
	unsigned char			 sensor_type;		// 标识符(VCA_SENSOR_TYPE)，代表传感器种类，1：Lidar，2：Radar 
	unsigned char			 reserved[15];
	VCA_3D_SENSOR_POINT_F	*pts;				// 3D点信息     
} VCA_3D_SENSOR_PT_F_LIST;//32字节

// 长方体 (浮点型)
typedef struct _VCA_CUBE_F_
{
	VCA_3D_POINT_F			 center;			// 长方体中心点坐标，x，y，z，单位m
	VCA_3D_POINT_F			 rotation;			// 长方体旋转角, (pitch,yaw,roll)
	float					 length;     	    // 长方体长度，m
	float					 width;      		// 长方体宽度，m
	float					 height;     		// 长方体高度，m
	unsigned char			 reserved[12];      // 保留12字节
}VCA_CUBE_F; // 48字节

// 多面体 (浮点型)
typedef struct _VCA_POLYHEDRON_F_
{
	VCA_3D_POINT_F			point[VCA_PLHD_MAX_VERTEX_NUM];	 // 多面体3D顶点
	int						vertex_num;                      // 多面体顶点数
	unsigned char			reserved[4];                     // 保留4字节
}VCA_POLYHEDRON_F; // 392字节

// 3D 目标基本信息
typedef  struct _VCA_3D_OBJ_INFO_
{
	int                     valid;                // 是否有效
	int                     id;                   //（参考）ID号，不能确定时为0
	VCA_OBJ_TYPE            type;                 // 目标类型
	VCA_CUBE_F              cube;                 // 3D目标位置
	VCA_RECT_F              rect;                 // 图像目标位置
	unsigned short          confidence;           // 检测置信度[0-1000]
	unsigned char           reserved[18];         // 保留18字节
} VCA_3D_OBJ_INFO; // 96字节

// 3D 目标链表  
typedef struct _VCA_3D_OBJ_LIST_
{
	unsigned int			obj_num;              // 目标个数
	unsigned int			max_obj_num;          // 初始化最大个数
	VCA_3D_OBJ_INFO		   *p_obj;                // 每个目标信息
}VCA_3D_OBJ_LIST; // 16字节

/***********************************************************************************************************************
目标信息定义
***********************************************************************************************************************/

//目标基本信息
typedef struct _VCA_BLOB_BASIC_INFO_
{
	unsigned int            id;
	unsigned int		    blob_type;					   // 目标类型,VCA_OBJ_TYPE
	short					confidence;					   // 目标框置信度
	char                    reserved[14];
	VCA_REGION              region;					       // 目标位置区域
}VCA_BLOB_BASIC_INFO;    //100 BYTE


//属性置信度
typedef struct _VCA_ATTR_CONF_
{
	short                    attr_value;				  // 属性值
	short                    prob;						  // 属性置信度(归一化0到1000）
}VCA_ATTR_CONF;//4字节

//目标属性
typedef struct _VCA_ATTR_INFO_
{
	unsigned int           attr_type;                      // 属性类型
    short                  attr_value;                     // 属性值
    short                  attr_conf;                      // 属性置信度
	unsigned short         label_num;                      // 个数 
	VCA_ATTR_CONF          label_conf_list[16];                // 置信度链表（最大置信度为数组第一个值）
	char                   reserved[8];
}VCA_ATTR_INFO; // 16 BYTE


//特征点
typedef struct _VCA_KEY_POINT_
{
	unsigned int				 pt_type;                   // 特征点类型（参考各特征点算法索引）
	unsigned char				 reserved[12];
	VCA_POINT_F					 pt;                        // 特征点坐标
}VCA_KEY_POINT;


//OCR字符结构
#define  VCA_OCR_LEN			  128						// 字符串最大支持的字符数
#define  VCA_OCR_BYTE_WID		  3							// 单个字符所占的字节
typedef struct _VCA_OCR_CONTENT_
{
	unsigned short                char_symbol[VCA_OCR_LEN];						  // 预测类别
	char						  predict_string[VCA_OCR_LEN][VCA_OCR_BYTE_WID];  // 转换后的结果
	unsigned short                tr_symbol_crd[VCA_OCR_LEN];					  // 单字置信度
	unsigned char                 char_len;										  // 字符长度
	unsigned short                conf;											  // 整体置信度
	char						  reserved[64];									  // 保留字节
} VCA_OCR_CONTENT; // 964 BYTE

//目标附加信息类型
typedef enum _VCA_ADD_INFO_TYPE_
{
	VCA_OBJ_COMPONET_INFO = 0x0,						      // 目标组件附加信息VCA_BLOB_BASIC_INFO
	VCA_OBJ_KEY_POINT     = 0x1,                              // 关键点附加信息VCA_KEY_POINT
	VCA_OBJ_VEH_PLATE     = 0x2,                              // 车牌信息
	VCA_OBJ_ATTRIBUTE     = 0x3,                              // 目标属性VCA_ATTR
	VCA_OBJ_TRAJECTORY    = 0x4,                              // 目标轨迹VCA_POINT_F
	VCA_OBJ_MEDIAFRAME    = 0x5,                              // 多媒体数据VCA_MEDIA_FRAME
    VCA_OBJ_FEATUREMAP    = 0x6,                              // 目标附带特征图VCA_MAP
	VCA_OBJ_OCR_CONTENT   = 0x7,                              // 目标字符内容VCA_OCR_CONTENT
	VCA_OBJ_ADD_END = VCA_ENUM_END_V2
}VCA_ADD_INFO_TYPE;


//目标附加信息
typedef struct _VCA_INFO_
{
	unsigned int		  info_num;                        // 同一种类型的附加信息数量
	unsigned int          info_type;                       // 附加信息类型VCA_OBJ_ADD_INFO_TYPE
	unsigned int          info_size;                       // 附加信息结构尺寸
	unsigned char         model_id[VCA_MODEL_TYPE_LENGTH]; // 产生附加信息的模型
	unsigned int          reserved[7];
	void				 *info;                            // 附加信息内容
}VCA_INFO;


// BLOB类型枚举
typedef enum _VCA_BLOB_TYPE_
{
	VCA_BLOB_DET_OBJ     = 0x1,						        // 检测目标
	VCA_BLOB_TRK_OBJ     = 0x2,						        // 跟踪目标
	VCA_BLOB_CLS_OBJ     = 0x3,                             // 分类目标(不再使用)
	VCA_BLOB_TRIGGER_OBJ = 0x3,                             // 触发目标
	VCA_BLOB_ALERT_OBJ   = 0x4,                             // 报警目标
	VCA_BLOB_GLOBAL_OBJ  = 0x5,                             // 全局状态
	VCA_BLOB_REGION_OBJ  = 0x6,                             // 区域目标
	VCA_BLOB_CLS_OBJ_V2  = 0x7,                             // 分类目标
	VCA_BLOB_END         = VCA_ENUM_END_V2
}VCA_BLOB_TYPE;


typedef enum _VCA_OBJ_TRIGGER_TYPE_
{
	VCA_OBJ_TRIGGER_NONE = 0,                              // 目标未触发
	VCA_OBJ_TRIGGER_YES = 1,                               // 目标已触发（停止更新，进入下一分类或识别环节）
	VCA_OBJ_TRIGGER_END = VCA_ENUM_END_V2
}VCA_OBJ_TRIGGER_TYPE;

typedef enum _VCA_OBJ_UPDATE_TYPE_
{
	VCA_OBJ_UPDATE_NONE  = 0,                              // 不更新
	VCA_OBJ_UPDATE_ADD   = 1,                              // 更新当前帧信息，在原有基础上新增
	VCA_OBJ_UPDATE_COVER = 2,                              // 更新当前帧信息，覆盖时间最早的一帧信息
	VCA_OBJ_UPDATE_END = VCA_ENUM_END_V2
}VCA_OBJ_UPDATE_TYPE;

//目标触发信息
typedef struct _VCA_BLOB_STAT_INFO_
{
	unsigned char           b_trigger;					    // 目标触发报警标志，参考VCA_OBJ_TRIGGER_TYPE
	unsigned char           b_update;					    // 目标是否更新（更新背景图，子图等）参考VCA_OBJ_UPDATE_TYPE
	unsigned char           b_disapear;					    // 目标消失标志（目标消失，清空外部缓存）
	unsigned char           b_count;					    // 目标是否被计数过
	unsigned char           reserved[8];
	unsigned int            trigger_type;                   // 目标触发类型
}VCA_BLOB_STAT_INFO;//16BYTE

//完整目标
typedef  struct _VCA_BLOB_INFO_
{
	unsigned char           valid;						     // 是否有效
	unsigned char           visible;					     // 是否可见：1-可见，0-不可见
	unsigned char           rule_id;					     // 目标所属规则ID
	char                    reserved[13];

	VCA_BLOB_BASIC_INFO     obj_info;                        // 目标基本信息
	VCA_BLOB_STAT_INFO      obj_stat_info;                   // 目标状态信息

	unsigned int            add_info_num;                     // 附加信息数量
	VCA_INFO	           *add_info;                         // 附加信息,按VCA_OBJ_ADD_INFO_TYPE从队列中取值

}VCA_BLOB_INFO;//132BYTE + 8～16BYTE

typedef struct _VCA_BLOB_INFO_LIST_
{
	unsigned int          frame_num;                           // 该列表对应的图像帧号
	unsigned int          max_blob_num;                        // 列表中最大目标数量
	unsigned int          blob_num;						       // 目标数
	unsigned int          blob_source;                         // 目标来源,标志是检测、跟踪目标、复制目标VCA_BLOB_TYPE
	unsigned int          product_type;                        // 产品类型
    unsigned char         model_id[VCA_MODEL_TYPE_LENGTH];     // 目标的模型类型 (VCA_MODEL_TYPE<<128) + model_version
	VCA_BLOB_INFO        *obj;						           // 目标信息
	unsigned char         reserved[12];
} VCA_BLOB_INFO_LIST;

#ifdef __cplusplus
}
#endif 

#endif /* _HIK_VCA_BASE_H_ */

