#pragma once

#ifndef _LIBXRAY_DEF_H_
#define _LIBXRAY_DEF_H_

/* 自研AI-XSP */
#include "ai_xsp_interface.h"

#define XRAY_LIB_MTAB_NUM                   8       // 内存表长度
#define XRAY_LIB_MAX_IMGENHANCE_LENGTH      4500    // 整图图像处理时，最大允许传入的图像长度
#define XRAY_LIB_MAX_RTPROCESS_LENGTH       125     // 实时出图能设置的单次最大传入列数
#define XRAY_LIB_MAX_FILTER_KERNEL_LENGTH   36      // 滤波核最大半径， 每次传入的最大列数+MAX_FILTER_KERNEL_LENGTH 是分段处理时需要开辟的临时内存长度
#define XRAY_LIB_MAX_TCPROCESS_LENGTH       240     // 实时测试体增强缓存的最大列数
#define XRAY_LIB_MAX_CACHED_RT_SLICE_NUM    10      // 实时处理算法内部缓存的最多条带数，可通过XRAYLIB_PARAM_TCSTRIPENUM设置，最小（默认）为1，最大不超过该值
#define XRAY_LIB_MAX_TIP_IMAGE_HEIGHT       720     // TIP图像最大长度
#define XRAY_LIB_MAX_TIP_IMAGE_WIDTH        720     // TIP图像最大宽度
#define XRAY_LIB_MIN_TIP_IMAGE_HEIGHT       32      // TIP图像最小长度
#define XRAY_LIB_MIN_TIP_IMAGE_WIDTH        32      // TIP图像最小宽度
#define XRAY_LIB_MAX_SEGNUM                 10000   // 有机物和难穿透识别时，最大识别区域数量
#define XRAY_LIB_MAX_CONCEREDNUMBER         40      // 单次返回最大可疑有机物框的数量
#define XRAY_LIB_MAX_RESIZE_FACTOR          2       // 流水线处理之间临时内存所需支持的最大缩放系数
#define XRAY_LIB_RESIZE_MULTIPLE            2       // 缩放处理后图像高度需满足的像素倍数
#define XRAY_LIB_MAX_RT_SLICE_HEIGHT        200     // 单次条带最大高度
#define XRAY_LIB_MAX_PATH                   4096    // 读取exe路径时最大的字符数
#define XRAY_LIB_MAX_NAME                   100     // 文件名称最大的字符数
#define XRAY_LIB_GRAYLEVEL                  64      // XRAYLIB_GRAYLEVEL（可变吸收率）等级数量
#define XRAY_LIB_BRIGHTNESSLEVEL            100     // XRAYLIB_BRIGHTNESS（增亮/减暗）等级数量


//错误代码定义
/****************************************************************************************
*                      常规错误码定义预留字节    0x80000100 ~ 0x800001FF
 ****************************************************************************************/
#define XRAY_LIB_OK                             0x80000100      //成功
#define XRAY_LIB_EMPTY_DATA                     0x80000101      //传入数据为空
#define XRAY_LIB_FINDEXEPATH_FAIL               0x80000102      //寻找当前可执行程序文件夹失败
#define XRAY_LIB_OPENFILE_FAIL                  0x80000103      //打开文件失败
#define XRAY_LIB_NULLPTR                        0x80000104      //指针为空
#define XRAY_LIB_PARAM_WIDTH_ERROR              0x80000105      //传入的宽度不符初始化时的设置
#define XRAY_LIB_IMAGELENGTH_OVERFLOW           0x80000106      //传入图像太大
#define XRAY_LIB_ENERGYMODE_ERROR               0x80000107      //能量模式解析错误
#define XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR    0x80000108      //更新空气本底模式错误
#define XRAY_LIB_VISUALVIEW_ERROR               0x80000109      //视角编号错误
#define XRAY_LIB_IMAGESIZE_ZERO                 0x8000010A      //图像尺寸为0
#define XRAY_LIB_NOACCESS                       0x8000010B      //没有执行权限
#define XRAY_LIB_INVALID_PARAM                  0x8000010C      //无效的参数
#define XRAY_LIB_READFILESIZE_ERROR             0x8000010D      //读取文件大小不符
#define XRAY_LIB_TIPSIZE_ERROR                  0x8000010E      //传入TIP图像文件尺寸超过限制
#define XRAY_LIB_TIPSIZE_TOOSMALL               0x8000010F      //传入TIP图像尺寸太小
#define XRAY_LIB_TIP_WORKING                    0x80000110      //TIP正在进行中 
#define XRAY_LIB_DEVICETYPE_ERROR               0x80000111      //设备类型错误
#define XRAY_LIB_GEOCALITABLE_ERROR             0x80000112      //几何校正表缺失
#define XRAY_LIB_MAXRTHEIGHT_OVERFLOW           0x80000113      //实时成像单次传入上限值太大
#define XRAY_LIB_RTCALIRATIO_ERROR              0x80000114      //实时更新比例值错误
#define XRAY_LIB_RTCACHING                      0x80000115      //实时条带处理正在缓存邻域中
#define XRAY_LIB_TIPWIDTH_ERROR					0x80000116		//计算TIP插入位置错误
#define XRAY_LIB_FLATDETCALITABLE_ERROR         0X80000117      //探测器平铺校正表缺失
#define XRAY_LIB_DENOISINGINTENSITY_ERROR		0x80000118		//设置降噪强度失败
#define XRAY_LIB_DETFUSIONCALITABLE_ERROR		0x80000119		//设置探测板融合校正失败
#define XRAY_LIB_INITIALVERIFICATION_ERROR      0x8000011A      //算法库校验失败
#define XRAY_LIB_INI_ERROR                      0x8000011B      //读取ini文件失败
#define XRAY_LIB_CNN_MODEL_OVERFLOW             0x8000011C      //存储的CNN模型数量溢出
#define XRAY_LIB_DETECTORTYPE_ERROR             0x8000011D      //探测板类型错误
#define XRAY_LIB_VALUETYPE_ERROR                0x8000011E      //SCSG类型错误
#define XRAY_LIB_SOURCETYPE_ERROR               0x8000011F      //射线源类型错误
#define XRAY_LIB_IMAGETYPE_ERROR                0x80000120      //图像输出类型错误
#define XRAY_LIB_DEVICEPARA_ERROR	            0x80000121      //机型射线源探测板视角类型等参数配置不支持
#define XRAY_LIB_MEM_ERROR                      0x80000122      //内存分配错误
#define XRAY_LIB_MIX_CURVE_INVALID              0x80000123      //没有有效的混合曲线

// AI_XSP错误定义
/****************************************************************************************
*                    AI_XSP错误码定义预留字节    0x80000300 ~ 0x800003FF
 ****************************************************************************************/
#define XRAY_LIB_AI_XSP_INIT_ERROR              0x80000300      //AI_XSP初始化失败
#define XRAY_LIB_AI_XSP_CHANNEL_ERROR           0x80000301      //AI_XSP通道数设置越界
#define XRAY_LIB_AI_XSP_RUN_ERROR               0x80000302      //AI_XSP处理数据错误
#define XRAY_LIB_AI_XSP_GET_ERROR               0x80000303      //AI_XSP获取数据错误
#define XRAY_LIB_AI_XSP_HEIGHT_OVERFLOW         0x80000304      //AI_XSP输入高度溢出
#define XRAY_LIB_AI_XSP_INTEGRAL_TIME_ERR       0x80000305      //AI_XSP模型积分时间错误
#define XRAY_LIB_AI_XSP_SPEED_GEAR_ERR          0x80000306      //AI_XSP模型速度档位错误
#define XRAY_LIB_AI_XSP_RELEASE_ERR             0x80000307      //AI_XSP资源释放错误
#define XRAY_LIB_AI_XSP_ASYN_PENDING 			0x80000308   	//AI_XSP异步任务正在处理中
#define XRAY_LIB_AI_XSP_ASYN_TIMEOUT 			0x80000309   	//AI_XSP异步任务获取结果超时
#define XRAY_LIB_AI_XSP_ASYN_ERROR 				0x8000030A     	//AI_XSP异步任务处理异常
#define XRAY_LIB_AI_XSP_ASYN_NOT_FOUND 			0x8000030B 		//AI_XSP异步任务不存在
#define XRAY_LIB_AI_XSP_ASYN_INPUT_ERR 			0x8000030C   	//AI_XSP异步任务输入异常
#define XRAY_LIB_AI_XSP_ASYN_INIT_ERR 			0x8000030D     	//AI_XSP异步初始化异常

 // XMat错误定义
 /****************************************************************************************
 *                    XMat错误码定义预留字节    0x80000400 ~ 0x800004FF
 ****************************************************************************************/
#define XRAY_LIB_XMAT_INVALID                   0x80000400      //无效的矩阵 
#define XRAY_LIB_XMAT_DAT_NULL                  0x80000401      //矩阵数据为NULL
#define XRAY_LIB_XMAT_SIZE_ERR                  0x80000402      //矩阵尺寸不匹配
#define XRAY_LIB_XMAT_TYPE_ERR                  0x80000403      //矩阵类型错误
#define XRAY_LIB_XMAT_ERINITIAL                 0x80000404      //重复初始化
#define XRAY_LIB_XMAT_MEM_OVERFLOW              0x80000405      //矩阵reshape超过最大内存
#define XRAY_LIB_XMAT_SIZE_MINUS                0x80000406      //矩阵尺寸设置小于等于0
#define XRAY_LIB_XMAT_TYPE_UNSUPPORTED          0x80000407      //矩阵类型不支持


//返回码定义
typedef unsigned int XRAY_LIB_HRESULT;

//标志位信息定义
typedef unsigned long long XRAYLIB_FLAG;

#ifndef VOID
#define VOID   void
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifdef LIBXRAY_X86
  #define LIBXRAY_NULL nullptr
#else
  #define LIBXRAY_NULL NULL
#endif

////TestDemo使用的转换类型
typedef enum _XRAY_LIB_IMAGEFORMAT_
{
	XRAYLIB_FORMAT_BMP = 0x00000000,
	XRAYLIB_FORMAT_JPG,
	XRAYLIB_FORMAT_PNG,
	XRAYLIB_FORMAT_TIFF,

	XRAYLIB_FORMAT_MIN_VALUE = XRAYLIB_FORMAT_BMP,
	XRAYLIB_FORMAT_MAX_VALUE = XRAYLIB_FORMAT_TIFF,
}XRAY_LIB_IMAGEFORMAT;

typedef struct _XRAYLIB_RGBA_
{
	unsigned char bRCompent;
	unsigned char bGCompent;
	unsigned char bBCompent;
	unsigned char bAplha;
}XRAYLIB_RGBA;

//
/***************************************************************************************************
* 使用的结构体和枚举类型
***************************************************************************************************/
/* 
* 安检机机型大类，0~3位表示SG、SC、CA(民航)子型号，第8~11位表示机型大类 
* (与DSP保持一致, 修改需同DSP同步)
*/
typedef enum _XRAY_DEVICETYPE_
{
	XRAY_DEVICETYPE_NUD = 0x0,
	/* 6550 */
	XRAY_DEVICETYPE_6550_SC = 0x100,
	XRAY_DEVICETYPE_6550_SG = 0x101,
	XRAY_DEVICETYPE_6550_CA = 0x102,
	/* 5030 */
	XRAY_DEVICETYPE_5030_SC = 0x200,
	XRAY_DEVICETYPE_5030_SG = 0x201,
	XRAY_DEVICETYPE_5030_CA = 0x202,
	/* 100100 */
	XRAY_DEVICETYPE_100100_SC = 0x300,
	XRAY_DEVICETYPE_100100_SG = 0x301,
	XRAY_DEVICETYPE_100100_CA = 0x302,
	/* 4233 */
	XRAY_DEVICETYPE_4233_SC = 0x400,
	XRAY_DEVICETYPE_4233_SG = 0x401,
	XRAY_DEVICETYPE_4233_CA = 0x402,
	/* 140100 */
	XRAY_DEVICETYPE_140100_SC = 0x500,
	XRAY_DEVICETYPE_140100_SG = 0x501,
	XRAY_DEVICETYPE_140100_CA = 0x502,

	/* 6040 */
	XRAY_DEVICETYPE_6040_SC = 0x600,
	XRAY_DEVICETYPE_6040_SG = 0x601,
	XRAY_DEVICETYPE_6040_CA = 0x602,

	XRAY_DEVICETYPE_MIN_VALUE = XRAY_DEVICETYPE_NUD,
	XRAY_DEVICETYPE_MAX_VALUE = XRAY_DEVICETYPE_6040_CA,
}XRAY_DEVICETYPE;

/* 探测板类型(与DSP保持一致, 修改需同DSP同步) */
typedef enum _XRAY_DETECTORNAME_
{
	XRAY_DT = 1,					  /*DT探测板*/
	XRAY_SUNFY,						  /*尚飞探测板*/
	XRAY_IRAY,						  /*奕瑞探测板*/
	XRAY_HAMAMATSU,					  /*滨松探测板*/
	XRAY_TYW,						  /*同源微探测板*/
	XRAY_VIDETECT,					  /*中威晶源探测板*/
	XRAY_RAYIN,						  /*睿影自研探测板*/
	XRAY_XRAYDV_DTCA,				  /*DT探测板，民航专用*/
	XRAY_RAYIN_QIPAN,				  /*睿影自研奇攀探测板，奇攀的X-sensor，睿影的底板*/
	XRAY_RAYIN_DIGITAL_WEIYING,       /*自研数字微影ASIC(GOS)*/
	XRAY_RAYIN_DIGITAL_DDC264,        /*自研数字DDC264ASIC(民航使用GOS)*/
	XRAY_RAYIN_ANALOG_NEW,			  /*自研新模拟(碘化铯)*/
	XRAY_RAYIN_MCD_HS,		 		  /*尚飞0.8mm对应探测板*/
	XRAY_RAYIN_DIGITAL_32PIXEL,		  /*自研数字32像素探测板*/
	XRAY_RAYIN_BACKSCATTER_DSB,		  /* 背散射, 采传命名的DSB, 不知道什么意思, 后面修正下 */

	XRAY_DETECTORTYPE_MAX_VALUE,
	XRAY_DETECTORTYPE_MIN_VALUE = XRAY_DT,
}XRAY_DETECTORNAME;

/* 每块探测板所含像素点数量 */
typedef enum _XRAY_DETECTOR_PIXEL_
{
	XRAY_DETECTOR_32PIXEL  = 32,	/* 每块探测板含32个像素点 */
	XRAY_DETECTOR_64PIXEL  = 64,	/* 每块探测板含64个像素点 */
	XRAY_DETECTOR_128PIXEL  = 128,	/* 每块探测板含128个像素点 */

	XRAY_DETECTOR_PIXEL_MIN_VALUE =  XRAY_DETECTOR_32PIXEL,
	XRAY_DETECTOR_PIXEL_MAX_VALUE = XRAY_DETECTOR_128PIXEL,
}XRAY_DETECTOR_PIXEL;

/*
* 射线源型号，24~31位表示厂商，第16~23位表示源型号
* (与DSP保持一致, 修改需同DSP同步)
*/
typedef enum _XRAY_SOURCE_TYPE_E_
{
	/* 射线源厂商-机电院 */
	XRAYLIB_SOURCE_JDY_T160 = 0x00000000,   //机电院T160，6550
	XRAYLIB_SOURCE_JDY_T140 = 0x00010000,   //机电院T140，5030
	XRAYLIB_SOURCE_JDY_T80 = 0x00020000,   //机电院T80，5030
	XRAYLIB_SOURCE_HIK_T120 = 0x00030000,   //机电院海康T120定制，4233
	XRAYLIB_SOURCE_JDY_T120 = 0x00040000,   //机电院T120，5030D
	XRAYLIB_SOURCE_JDY_T160YT = 0x00050000,   //机电院160YT
	XRAYLIB_SOURCE_JDY_T2050 = 0x00060000,   //机电院T2050，140100·VH
	XRAYLIB_SOURCE_JDY_T140RT = 0x00070000,   //机电院T140RT
	XRAYLIB_SOURCE_JDY_TM80 = 0x000A0000,   //机电院io控制T80
											/* 射线源厂商-凯威信达 */
	XRAYLIB_SOURCE_KVXD_X160 = 0x01000000,   //凯威信达T160
	/* 射线源厂商-超群 */
	XRAYLIB_SOURCE_CQ_T200 = 0x03010000,   //超群T200，波特率57600
	/* 射线源厂商-博思得 */
	XRAYLIB_SOURCE_BSD_T160 = 0x04000000,   //博思得T160
	XRAYLIB_SOURCE_BSD_T140 = 0x04010000,   //博思得T140
	XRAYLIB_SOURCE_BSD_T80 = 0x04020000,   //博思得B80
	/* 射线源厂商-spellman */
	XRAYLIB_SOURCE_SPM_S180 = 0x05000000,   //S180
	XRAYLIB_SOURCE_SPM_S180L = 0x05020000,				//S180降本
	/* 射线源厂商-深圳力能时代*/
	XRAYLIB_SOURCE_LIOENERGY_LXB80 = 0x06000000,   //力能80
	/* 射线源厂商-睿影*/
	XRAYLIB_SOURCE_RAYIN_XSD180 = 0x07000000,			//睿影自研射线源180，SD-XSD180DP200-0.8-E03-CA	90-180KV 0.5-1.11mA	0.8mm
	XRAYLIB_SOURCE_RAYIN_XSD160 = 0x07010000,			//睿影自研射线源160
	XRAYLIB_SOURCE_RAYIN_XSD160L = 0x07020000,			//睿影自研射线源180KV的射线源用成160
	/* 射线源厂商-通用型号-算法内部测试 */
	XRAYLIB_SOURCE_NORMAL,                        //算法内部测试 枚举最大值

	XRAY_SOURCETYPE_MIN_VALUE = XRAYLIB_SOURCE_JDY_T160,
	XRAY_SOURCETYPE_MAX_VALUE = XRAYLIB_SOURCE_NORMAL,
} XRAYLIB_SOURCE_TYPE_E;


/* 视角 */
typedef enum _XRAY_LIB_VISUAL_
{
	XRAYLIB_VISUAL_MAIN = 0x00000000,  //主视角
	XRAYLIB_VISUAL_AUX,                //辅视角
	XRAYLIB_VISUAL_SINGLE,             //单视角

	XRAYLIB_VISUAL_MIN_VALUE = XRAYLIB_VISUAL_MAIN,
	XRAYLIB_VISUAL_MAX_VALUE = XRAYLIB_VISUAL_SINGLE,
}XRAY_LIB_VISUAL;

/* 算法开关 */
typedef enum _XRAY_LIB_ONOFF_
{
	XRAYLIB_OFF = 0x00000000,
	XRAYLIB_ON,

	XRAYLIB_ONOFF_MIN_VALUE = XRAYLIB_OFF,
	XRAYLIB_ONOFF_MAX_VALUE = XRAYLIB_ON,
}XRAY_LIB_ONOFF;

/* 内存分配结构体 */
typedef struct _XRAY_LIB_MEM_TAB_
{
    unsigned long    size;       /* 以BYTE为单位的内存大小*/
    void*           base;        /* 分配出的内存指针 */
}XRAY_LIB_MEM_TAB;

/* 算法库能力集 */
typedef struct _XRAY_LIB_ABILITY_
{
	int   nDetectorWidth;
	int   nMaxHeightRealTime; 
	int	  nMaxHeightEntire;			/*整图处理最大条带高度*/
	int   nPerDetectorPixel;		/* 每块探测板宽度(XRAY_DETECTOR_PIXEL) */
	float fResizeScale;
	int	  nPipeLineMode;
	int   nUseAI;
	int   nEnergyMode;
	int   nUseSeg;
	int	  nUseTc;
}XRAY_LIB_ABILITY;

///版本信息
typedef struct _XRAYLIB_XRY_VERSION_
{
    unsigned int major;    //主版本号，模块有较大改动如增加功能或架构发生变换等时修改
	unsigned int minor;    //子版本号，功能有增加及变动时
	unsigned int revision; //修订版本号，Bug修复或功能优化
	unsigned int year;     
	unsigned int month;
	unsigned int day;
}XRAYLIB_XRY_VERSION;

/* 版本类型 */
typedef enum
{
	XRAYLIB_VERSION_IMAGING_ALG = 0x00000000, 	// 成像算法版本 
	XRAYLIB_VERSION_EXPLOSIVE_DETECT_ALG1, 		// 爆炸物检测算法1版本 
	XRAYLIB_VERSION_EXPLOSIVE_DETECT_ALG2, 		// 爆炸物检测算法2版本 
	XRAYLIB_VERSION_LOWPENETRATION_DETECT_ALG,	// 难穿透检测算法版本

	XRAYLIB_VERSION_MIN_VALUE = XRAYLIB_VERSION_IMAGING_ALG,
	XRAYLIB_VERSION_MAX_VALUE = XRAYLIB_VERSION_LOWPENETRATION_DETECT_ALG,

}XRAY_LIB_VERSION_TYPE;

/* 降噪模式选择*/
typedef enum _XRAY_LIB_DENOISING_MODE_
{
	XRAYLIB_DENOISING_NONE = 0x00000000, // 不降噪
	XRAYLIB_DENOISING_MODE0,             // 快速双边滤波降噪
	XRAYLIB_DENOISING_MODE1,             // 基于OPENVINO的CNN去噪

	XRAYLIB_DENOISING_MIN_VALUE = XRAYLIB_DENOISING_NONE,
	XRAYLIB_DENOISING_MAX_VALUE = XRAYLIB_DENOISING_MODE1,

}XRAYLIB_DENOISING_MODE;

/*降噪强度*/
typedef enum _XRAY_LIB_DENOISING_INTENSITY_
{
	XRAYLIB_DENOISING_INTENSITY_NONE	 = 0x00000000, // 不降噪
	XRAYLIB_DENOISING_INTENSITY_LOW,                   // 低强度降噪
	XRAYLIB_DENOISING_INTENSITY_SUBLOW,                // 次低强度降噪
	XRAYLIB_DENOISING_INTENSITY_MEDIUM,                // 中强度降噪
	XRAYLIB_DENOISING_INTENSITY_SUBHIGH,               // 次高强度降噪
	XRAYLIB_DENOISING_INTENSITY_HIGH,                  // 高强度降噪

	XRAYLIB_DENOISING_MIN_INTENSITY = XRAYLIB_DENOISING_INTENSITY_NONE,
	XRAYLIB_DENOISING_MAX_INTENSITY = XRAYLIB_DENOISING_INTENSITY_HIGH,

}XRAYLIB_DENOISING_INTENSITY;

/*伽马变换无级调节图像亮度*/
typedef enum _XRAYLIB_GAMMA_MODE_
{
	XRAYLIB_GAMMA_OPEN = 0x00000000,   //打开伽马变换
	XRAYLIB_GAMMA_CLOSE,               //关闭伽马变换

	XRAYLIB_GAMMA_MIN_VALUE = XRAYLIB_GAMMA_OPEN,
	XRAYLIB_GAMMA_MAX_VALUE = XRAYLIB_GAMMA_CLOSE,
}XRAYLIB_GAMMA_MODE;

typedef enum _XRAYLIB_FILEPATH_MODE_
{
	XRAYLIB_FILEPATH_OPPOSITE = 0x00000000,
	XRAYLIB_FILEPATH_ABSOLUTE,

	XRAYLIB_FILEPATH_MODE_MIN_VALUE = XRAYLIB_FILEPATH_OPPOSITE,
	XRAYLIB_FILEPATH_MODE_MAX_VAULE = XRAYLIB_FILEPATH_ABSOLUTE,
}XRAYLIB_FILEPATH_MODE;

///指定更新满载和本底数据的方式
typedef enum _XRAYLIB_UPDATE_ZEROANDFULL_MODE_
{
     XRAYLIB_UPDATEZERO = 0x00000000,
	 XRAYLIB_UPDATEFULL,

	 XRAYLIB_UPDATE_ZEROANDFULL_MIN = XRAYLIB_UPDATEZERO,
	 XRAYLIB_UPDATE_ZEROANDFULL_MAX = XRAYLIB_UPDATEFULL,
}XRAYLIB_UPDATE_ZEROANDFULL_MODE;

////TIP时返回的状态
typedef enum _XRAYLIB_TIP_STATUS_
{
	XRAYLIB_TIP_NONE = 0x00000000,
	XRAYLIB_TIP_WORKING,
	XRAYLIB_TIP_END,
	XRAYLIB_TIP_FAILED, 				//如果空间不够插入，返回错误后后续不再视图插入
	XRAYLIB_TIP_INTERRUPT,				//插入过程中包裹边界不够，返回错误
	XRAYLIB_TIP_ZIDENTIFY_FAILED,
	XRAYLIB_TIP_MERGE_FAILED,
	XRAYLIB_TIP_DEFAULTENHANCE_FAILED,

	XRAYLIB_TIP_STATUS_MIN_VALUE = XRAYLIB_TIP_NONE,
	XRAYLIB_TIP_STATUS_MAX_VALUE = XRAYLIB_TIP_DEFAULTENHANCE_FAILED,
}XRAYLIB_TIP_STATUS;

typedef enum _XRAYLIB_TIP_FLAG_
{
	XRAYLIB_TIPPROC_FALSE = 0x00000000,
	XRAYLIB_TIPPROC_TRUE,

	XRAYLIB_TIP_FLAG_MIN_VALUE = XRAYLIB_TIPPROC_FALSE,
	XRAYLIB_TIP_FLAG_MAX_VALUE = XRAYLIB_TIPPROC_TRUE,
}XRAYLIB_TIP_FLAG;

typedef struct _XRAYLIB_PACKAGE_POS_
{
	////X是时间方向；Y是探测器方向
	int nPackNum; //最大设置为2个(16列里2个包裹)
	int nPackageXStart1;
	int nPackageXEnd1;
	int nPackageYStart1;
	int nPackageYEnd1;
	int nPackageXStart2;
	int nPackageXEnd2;
	int nPackageYStart2;
	int nPackageYEnd2;
}XRAYLIB_PACKAGE_POS;

////算法图像处理结构体
typedef struct _XRAYLIB_IMG_PARAM_
{
	int img_width_in;    //传入图像宽度（探测器方向像素数）
	int img_height_in;   //传入图像高度（采集列数），该高度包含上下邻域

	int img_height_ntop; //上邻域高度，可为0，该区域不输出
	int img_height_nbotm;//下邻域高度，可为0，该区域不输出

	int img_width_out;   //传出图像宽度
	int img_height_out;  //传出图像高度

	int rgb_width_out;   //RGB图像实际宽度
	int rgb_height_out;  //RGB图像高度（考虑到RGB图像会转置，所以宽高和归一化后图像等并不相同）
	int rgb_width_stride_need; //传入值，指定RGB图像传出时需要的宽度

	int nRGBStrideW;   ///输入的RGB内存实际宽度（探测器方向）
	int nRGBStrideH;   ///输入的RGB内存实际长度（出图方向）
	
	int yuv_width_out;   //YUV图像实际宽度
	int yuv_height_out;  //YUV图像高度（考虑到YUV图像会转置，所以宽高和归一化后图像等并不相同）
	int yuv_width_stride_need; //传入值，指定YUV图像传出时需要的宽度

	int nYUVStrideW;   ///输入的YUV内存实际宽度（探测器方向）
	int nYUVStrideH;   ///输入的YUV内存实际长度（出图方向）

	int nLowGray;        //本次是否有难穿透区，0为否，1为是  

	//输入的原始采集板数据
	unsigned short* pOrgHighData;  //探测器采集到的原始高能数据
	unsigned short* pOrgLowData;   //探测器采集到的原始低能数据

	//校正和材料分辨后输出的图像
	unsigned short* pCaliHighData; //归一化后高能数据
	unsigned short* pCaliLowData;  //归一化后低能数据
	unsigned char* pZData;         //原子序数图
	unsigned short* pMergeData;    //高低能合成灰度图

	//TIP
	XRAYLIB_TIP_STATUS TipStatus;
	/////TIP开启下，同时返回增加了TIP效果的图像，输出
	unsigned short* pTipedCaliHighData; //归一化后增加TIP信息高能数据
	unsigned short* pTipedCaliLowData;  //归一化后增加TIP信息低能数据
	unsigned char* pTipedZData;         //增加TIP信息原子序数图
	int nTipedPosW;                     //探测器方向TIP图像插入起始位置

	unsigned char* pAiYUV;         //AI路YUV图像，永远不会有图像处理和TIP
	////输出的RGB图像,以下接口使用一路即可。
	////原接口，R和G\B路不分离
	unsigned char* pDispRGB;       //显示用RGB图像(如果有TIP，会有TIP图像在内）
	////输出的YUV图像,以下接口使用一路即可。
	////原接口，Y和UV路不分离
	unsigned char* pDispYUV;       //显示用YUV图像(如果有TIP，会有TIP图像在内）
	////改变后，R和G、B路分离
	unsigned char* pAiY;
	unsigned char* pAiUV;
    unsigned char* pDispY;
	unsigned char* pDispUV;
	unsigned char* pDispR;

	XRAYLIB_PACKAGE_POS packpos;   //包裹位置

}XRAYLIB_IMG_PARAM;

// 研究院设备参数
typedef struct _DEV_DATA_T
{
	int                     dev_type;              // 处理核心类型
	int                     dev_id;                // 处理核心编号
	float                   per;                   // 算力占用比例
}DEV_DATA_T;

//研究院调度器管理设备结构体
typedef struct _SCHEDULER_MGR_PARAM_T
{
	DEV_DATA_T  dev_data[60];     // 设备数组
	int               dev_num;             // 设备数量
	int               priority;            // 进程优先级
	int               state;               // 是否输出状态统计信息
} SCHEDULER_MGR_PARAM_T;

// 研究院设备类型
typedef enum _DEV_TYPE_E
{
	DEV_ID0 = 0,
	DEV_ID1,
	DEV_ID2,
	DEV_ID3,
	DEV_ID4,
	DEV_ID5,
	DEV_ID6,
	DEV_ID7,
	DEV_ID8,
	DEV_ID9
} DEV_TYPE_E;

//自研AI-XSP调度参数
typedef struct _RAYIN_AI_MGR_
{
	SYSTEM_CORE_MASK  aiSrNpuCore;    //超分降噪模型使用核心id
	SYSTEM_CORE_MASK  aiSegNpuCore;   //语义分割模型使用核心id
} RAYIN_AI_MGR;

// AI-XSP初始化参数
typedef struct _XRAYLIB_AI_PARAM_
{
	// AI-XSP图像参数
	int AI_width_in;                                  // 传入AI-XSP图像宽度（探测器方向像素数）
	int AI_height_in;                                 // 传入AI-XSP图像高度（采集列数）

	int AI_width_out;                                 // AI-XSP输出图像宽度
	int AI_height_out;                                // AI-XSP输出图像高度

	int AI_channels_in;                               // AI-XSP输入通道数
	int AI_channels_out;                              // AI-XSP输出通道数

	int neighborLen;                                  // 输入扩边高度

	SYSTEM_PLAT_E systemPlat;                         // 硬件平台
	SYSTEM_PROCESS_TYPE_E systemProcessType;          // 推理选项
	SYSTEM_PROCESS_PRECISION_E systemProcessPrecision;// 推理精度


	int plat_mode;                                    // 0 自研AI-XSP 1 研究院AI-XSP 目前支持x86和rk模式的切换
	SCHEDULER_MGR_PARAM_T  scheduler_param;           // 研究院环境初始化参数，开放给DSP初始化
	RAYIN_AI_MGR rayinSchedulerParam;                 // 自研环境初始化参数，开放给DSP初始化 (NPU/GPU设备指定)

	int nChannel;                                     // 模型通道号 用于不同视角初始化 0-2
	int nUseAI;                                       // AI_XSP使用开关 0关闭 1打开
}XRAYLIB_AI_PARAM;

// 3-6色成像模式
typedef enum _XRAYLIB_COLORSIMAGING_MODE_
{
	XRAYLIB_COLORSIMAGING_3S = 0x00000000,            /* 3色成像 */
	XRAYLIB_COLORSIMAGING_6S,                         /* 6色成像 */

	XRAYLIB_COLORSIMAGING_MIN_VALUE = XRAYLIB_COLORSIMAGING_3S,
	XRAYLIB_COLORSIMAGING_MAX_VALUE = XRAYLIB_COLORSIMAGING_6S,
}XRAYLIB_COLORSIMAGING_SEL;

// 流水线级数定义
typedef enum _XRAYLIB_PIPELINE_MODE_
{
	// 未知流水线模式
	XRAYLIB_PIPELINE_MODE_UNKNOWN = 0x0,

	// 0x1-0xFF: 一段流水
	XRAYLIB_PIPELINE_MODE1A = 0x1,
	XRAYLIB_PIPELINE_MODE1B = 0x2,
	XRAYLIB_PIPELINE_MODE1C = 0x3,

	// 0x101-0x1FF: 二段流水
	XRAYLIB_PIPELINE_MODE2A = 0x101,
	XRAYLIB_PIPELINE_MODE2B = 0x102,
	XRAYLIB_PIPELINE_MODE2C = 0x103,

	// 0x201-0x2FF：三段流水
	XRAYLIB_PIPELINE_MODE3A = 0x201,
	XRAYLIB_PIPELINE_MODE3B = 0x202,
	XRAYLIB_PIPELINE_MODE3C = 0x203,

	XRAYLIB_PIPELINE_MIN_VALUE = XRAYLIB_PIPELINE_MODE1A,
	XRAYLIB_PIPELINE_MAX_VALUE = XRAYLIB_PIPELINE_MODE3C,

} XRAYLIB_PIPELINE_MODE;

// 矩形区域（整型），以像素为单位
typedef struct _XRAYLIB_RECT_
{
	unsigned int x;         // 矩形左上角X轴坐标
	unsigned int y;         // 矩形左上角Y轴坐标
	unsigned int width;     // 矩形宽度
	unsigned int height;    // 矩形高度
} XRAYLIB_RECT;

// 实时条带处理的包裹分割参数
typedef struct _XRAYLIB_RTPROC_PACKAGE_
{
	unsigned int nPackNum;				// 最大支持2个
	XRAYLIB_RECT stPackPos[2];		    // 包裹矩形区域
} XRAYLIB_RTPROC_PACKAGE;

// TIP
typedef struct _XRAYLIB_RTPROC_TIP_PARAM_
{
	unsigned short* pTipedCaliHighData; //归一化后增加TIP信息高能数据
	unsigned short* pTipedCaliLowData;  //归一化后增加TIP信息低能数据
	unsigned char* pTipedZData;         //增加TIP信息原子序数图
	int nTipedPosW;                     //探测器方向TIP图像插入起始位置
} XRAYLIB_RTPROC_TIP_PARAM;

// 转存 TIP 结构体
typedef struct _XRAYLIB_DUMPPROC_TIP_PARAM_
{
	unsigned short* pTipedCaliHighData; //归一化后增加TIP信息高能数据
	unsigned short* pTipedCaliLowData;  //归一化后增加TIP信息低能数据
	unsigned char* pTipedZData;         //增加TIP信息原子序数图
	int nHeight;                        //TIP图像过包方向高度
	int nWidth;                         //TIP图像探测器方向高度
	int nTipedPosX;                     //过包方向TIP图像插入起始位置
	int nTipedPosY;                     //探测器方向TIP图像插入起始位置
	XRAYLIB_TIP_FLAG enTipflag;         //当前转存处理是否需要TIP
} XRAYLIB_DUMPPROC_TIP_PARAM;

// 图像格式
typedef enum _XRAYLIB_IMG_FORMAT_
{
	XRAYLIB_IMG_UNKNOWN = 0x00000000,      // 未定义数据格式

	//												// |         type         |        store memory idx        |             stride             |
	// XRAY RAW (0x1 ~ 0xFF)
	XRAYLIB_IMG_RAW_L		    = 0x00000001,		// |        UINT16        |              D[0]              |              S[0]              |
	XRAYLIB_IMG_RAW_LH		    = 0x00000002,		// |        UINT16        |         L:D[0], H:D[1]         |         L:S[0], H:S[1]         |
	XRAYLIB_IMG_RAW_LHZ8	    = 0x00000003,		// |  LH:UINT16, Z:UINT8  |     L:D[0], H:D[1], Z:D[2]     |     L:S[0], H:S[1], Z:S[2]     |
	XRAYLIB_IMG_RAW_LHZ16	    = 0x00000004,		// |  LH:UINT16, Z:UINT16 |     L:D[0], H:D[1], Z:D[2]     |     L:S[0], H:S[1], Z:S[2]     |
	XRAYLIB_IMG_RAW_MERGE       = 0x00000005,		// |        UINT16        |              D[0]              |              S[0]              |
	XRAYLIB_IMG_RAW_MASK        = 0x00000006,		// |         UINT8        |              D[0]              |              S[0]              |

	// YUV (0x101 ~ 0x1FF)
	XRAYLIB_IMG_YUV_NV21		= 0x00000101,		// | Y:UINT16, VU:UINT8*2 |         Y:D[0], VU:D[1]        |         Y:S[0], VU:S[1]        |
	
	// RGB (0x201 ~ 0x2FF)
	XRAYLIB_IMG_RGB_ARGB_P4     = 0x00000201,	    // |        UINT8         | R:D[0], G:D[1], B:D[2], A:D[3] | R:S[0], G:S[1], B:S[2], A:S[3] |
	XRAYLIB_IMG_RGB_ARGB_C4     = 0x00000202,		// |     BGRA:UINT8*4     |            BGRA:D[0]           |            BGRA:S[0]           |
	
	XRAYLIB_IMG_RGB_ARGB_P3     = 0x00000203,		// |        UINT8         |     R:D[0], G:D[1], B:D[2]     |     R:S[0], G:S[1], B:S[2]     |
	XRAYLIB_IMG_RGB_ARGB_C3     = 0x00000204,		// |      BGR:UINT8*3     |            BGR:D[0]            |             BGR:S[0]           |

	XRAYLIB_IMG_FORMAT_MIN_VALUE = XRAYLIB_IMG_RAW_L,
	XRAYLIB_IMG_FORMAT_MAX_VALUE = XRAYLIB_IMG_RGB_ARGB_C3,
} XRAYLIB_IMG_FORMAT;

// 条带数据图像参数（格式/宽度/高度/行间距/存储地址）
typedef struct _XRAYLIB_IMAGE_SPEC_
{
	XRAYLIB_IMG_FORMAT format;  // 图像格式
	unsigned int width;         // 图像宽度
	unsigned int height;        // 图像高度
	unsigned int stride[4];     // 行间距
	void *pData[4];             // 数据存储地址，使用外部指定的内存时，平面i的可写内存容量会根据stride[i]*height计算得到
} XRAYLIB_IMAGE;

// MODE2A类型流水线定义
typedef struct _XRAYLIB_PIPELINE_PARAM_MODE2A_
{
	struct _pipeline_2a_idx0_ // 1级流水
	{
		XRAYLIB_IMAGE in_xraw;                  // RAW_LH         输入，原始低能、高能 
		XRAYLIB_IMAGE out_calilhz;              // RAW_LHZ8       输出，归一化后低能、高能、Z8              （含其他图像处理）  
		XRAYLIB_IMAGE out_calilhz_ori;          // RAW_LHZ16      输出，归一化的低能、低能、Z16             （用于危险品检测、液体识别）
		XRAYLIB_IMAGE out_mask;                 // RAW_MASK       输出，条带中包裹区域Mask
		XRAYLIB_RTPROC_PACKAGE out_pack[XRAY_LIB_MAX_CACHED_RT_SLICE_NUM+1]; // 输出，条带中包裹区域，坐标基于out_calilhz，[0]为out_flag对应条带中包裹区域，
                                                                             // [n]为in_flag对应条带中包裹区域，数组中实际有效元素个数为n+1
		XRAYLIB_FLAG  in_flag;                  //                输入，条带标志
		XRAYLIB_FLAG  out_flag;                 //                输出，条带标志
	} idx0;
	struct _pipeline_2a_idx1_ // 2级流水
	{
		XRAYLIB_IMAGE in_calilhz;				// RAW_LHZ8       输入，1级流水输出的归一化后低能、高能、Z8
		XRAYLIB_IMAGE in_calilhz_ori;           // RAW_LHZ16      输入，1级流水输出的归一化后低能、高能、Z16（未经图像处理及分辨率缩放）
		XRAYLIB_IMAGE in_mask;                  // RAW_MASK       输入，idx0流水输出的out_mask回传
		XRAYLIB_RTPROC_PACKAGE in_pack[XRAY_LIB_MAX_CACHED_RT_SLICE_NUM+1]; // 输入，idx0流水输出的out_pack回传
		XRAYLIB_IMAGE out_calilhz;				// RAW_LHZ8       输出，后处理的归一化后低能、高能、Z8
		XRAYLIB_IMAGE out_calilhz_ori;		    // RAW_LHZ16      输出，后处理的归一化后低能、高能、Z16
		XRAYLIB_IMAGE out_merge;				// RAW_MERGE      输出，高低能合成灰度图
		XRAYLIB_IMAGE out_disp;					//                输出，显示图像
		XRAYLIB_IMAGE out_ai;					// YUV_NV21       输出，AI图像
		XRAYLIB_RTPROC_PACKAGE packpos;			//                输出，条带中包裹区域，坐标基于out_calilhz
		XRAYLIB_TIP_STATUS TipStatus;           //                输出，TIP状态返回
		XRAYLIB_RTPROC_TIP_PARAM TipParam;      //                输出，TIP开启下，返回插入TIP的图像
	} idx1;

}XRAYLIB_PIPELINE_PARAM_MODE2A;

// MODE3A类型流水线联合变量定义
typedef struct _XRAYLIB_PIPELINE_PARAM_MODE3A_
{
	struct _pipeline_3a_idx0_ // 1级流水
	{
		XRAYLIB_IMAGE in_xraw;                  // RAW_LH         输入，原始低能、高能
		XRAYLIB_IMAGE out_calilhz;              // RAW_LHZ8       输出，归一化后低能、高能、Z8              （含其他图像处理，预留两倍缩放内存）
		XRAYLIB_IMAGE out_calilhz_ori;          // RAW_LHZ16      输出，归一化的低能、高能、Z16             （用于危险品检测、液体识别）
		XRAYLIB_IMAGE out_mask;                 // RAW_MASK       输出，条带中包裹区域Mask，图像尺寸与out_calilhz一致
		XRAYLIB_RTPROC_PACKAGE out_pack[XRAY_LIB_MAX_CACHED_RT_SLICE_NUM+1]; // 输出，条带中包裹区域，坐标基于out_calilhz，[0]为out_flag对应条带中包裹区域，
                                                                             // [n]为in_flag对应条带中包裹区域，数组中实际有效元素个数为n+1
		XRAYLIB_FLAG in_flag;                   //                输入，条带标志
		XRAYLIB_FLAG out_flag;                  //                输出，条带标志
	} idx0;
	struct _pipeline_3a_idx1_ // 2级流水
	{
		XRAYLIB_IMAGE in_calilhz;               // RAW_LHZ8       输入，1级流水输出的归一化后低能、高能、Z8 （预留两倍缩放内存）
		XRAYLIB_IMAGE out_calilhz;              // RAW_LHZ8       输出，AI处理的归一化后低能、高能、Z8     （预留两倍缩放内存）
	} idx1;
	struct _pipeline_3a_idx2_ // 3级流水
	{
		XRAYLIB_IMAGE in_calilhz;				// RAW_LHZ8       输入，idx1流水输出的归一化后低能、高能、Z8 （预留两倍缩放内存）
		XRAYLIB_IMAGE in_calilhz_ori;           // RAW_LHZ16      输入，idx0流水输出的归一化后低能、高能、Z16
		XRAYLIB_IMAGE in_mask;                  // RAW_MASK       输入，idx0流水输出的out_mask回传
		XRAYLIB_RTPROC_PACKAGE in_pack[XRAY_LIB_MAX_CACHED_RT_SLICE_NUM+1]; // 输入，idx0流水输出的out_pack回传
		XRAYLIB_IMAGE out_calilhz;				// RAW_LHZ8       输出，后处理的归一化后低能、高能、Z8
		XRAYLIB_IMAGE out_calilhz_ori;		    // RAW_LHZ16      输出，后处理的归一化后低能、高能、Z16     （未经过AI路的降噪及分辨率缩放）
		XRAYLIB_IMAGE out_merge;				// RAW_MERGE      输出，高低能合成灰度图
		XRAYLIB_IMAGE out_disp;					//                输出，显示图像
		XRAYLIB_IMAGE out_ai;					// YUV_NV21       输出，AI图像
		XRAYLIB_RTPROC_PACKAGE packpos;			//                输出，条带中包裹区域，坐标基于out_calilhz
		XRAYLIB_TIP_STATUS TipStatus;           //                输出，TIP状态返回
		XRAYLIB_RTPROC_TIP_PARAM TipParam;      //                输出，TIP开启下，返回插入TIP的图像
	} idx2;
} XRAYLIB_PIPELINE_PARAM_MODE3A;

// MODE3B类型流水线联合变量定义(背散射使用)
typedef struct _XRAYLIB_PIPELINE_PARAM_MODE3B_
{
	struct _pipeline_3b_idx0_ // 1级流水
	{
		XRAYLIB_IMAGE in_xraw;                  // RAW(uint16)		            输入，原始背散数据
		XRAYLIB_IMAGE out_calilhz;              // RAW(u16 + u16)		        输出，预处理完成 (保留降噪的数据和AIXSP预处理结束后的数据)
		XRAYLIB_FLAG  in_flag;                  //                				输入，条带标志
		XRAYLIB_FLAG  out_flag;                 //                				输出，条带标志
	} idx0;
	struct _pipeline_3b_idx1_ // 2级流水
	{
		XRAYLIB_IMAGE in_calilhz;               // RAW(u16 + u16)     输入，1级流水输出的送AIXSP做降噪的数据
		XRAYLIB_IMAGE out_calilhz;              // RAW(u16 + u16)     输出，AI降噪处理后的数据
	} idx1;
	struct _pipeline_3b_idx2_ // 3级流水
	{
		XRAYLIB_IMAGE in_calilhz;				// RAW(u16 + u16)    输入，(AIXSP处理后的数据和降噪的数据)
		XRAYLIB_IMAGE out_calilhz;				// RAW(u16 + u16)    输出，后处理背景滤除后的NLMeans降噪数据和AIXSP降噪数据
		XRAYLIB_IMAGE out_merge;				// RAW(u16)          输出，增强后数据
		XRAYLIB_IMAGE out_disp;					// RAW_LH            输出，显示图像RGB
	} idx2;
} XRAYLIB_PIPELINE_PARAM_MODE3B;
/* 流水线相关结构体及枚举类型定义结束 */

////几何校正表文件头
typedef struct _XRAYLIB_GEOCALI_HEADER_
{
	XRAY_DEVICETYPE devicetype;      //设备类型
	unsigned short usPixelNumber;    //像素数
	unsigned short usIndexPerPixel;  //表中每个像素维度使用的数据量
}XRAYLIB_GEOCALI_HEADER;

/////TIP数据结构体
typedef struct _XRAYLIB_TIP_IMG_
{
	int nImageHeight;
	int nImageWidth;
	unsigned short* pCaliHighData;
	unsigned short* pCaliLowData;
	unsigned char* pZData;
}XRAYLIB_TIP_IMG;

// 矩形包围盒
typedef struct _XRAYLIB_BOX_
{
	int nLeft;
	int nRight;
	int nTop;
	int nBottom;
}XRAYLIB_BOX;

typedef struct _XRAYLIB_CONCERED_AREA_
{
	int nNum;
	XRAYLIB_BOX CMRect[XRAY_LIB_MAX_CONCEREDNUMBER];
}XRAYLIB_CONCERED_AREA;

////获取RGB或YUV图像的模式（使用高低能归一化图像or使用融合灰度图）
typedef enum _XRAYLIB_IMG_CHANNEL_
{
    XRAYLIB_IMG_CHANNEL_HIGHLOW = 0x00000000,    /* 高低能通道*/
    XRAYLIB_IMG_CHANNEL_MERGE,                   /* 合成灰度图*/

	XRAYLIB_IMG_CHANNEL_MIN_VALUE = XRAYLIB_IMG_CHANNEL_HIGHLOW,
	XRAYLIB_IMG_CHANNEL_MAX_VALUE = XRAYLIB_IMG_CHANNEL_MERGE,
}XRAYLIB_IMG_CHANNEL; 

///指定设置参数函数处理的类型，是图像处理还是参数值设置
typedef enum _XRAYLIB_SETCONFIG_CHANNEL_ 
{
     XRAYLIB_SETCONFIG_IMAGEORDER = 0x00000000, //图像处理请求
	 XRAYLIB_SETCONFIG_PARAMCHANGE,             //改变参数请求

	 XRAYLIB_SETCONFIG_CHANNEL_MIN_VALUE = XRAYLIB_SETCONFIG_IMAGEORDER,
	 XRAYLIB_SETCONFIG_CHANNEL_MAX_VALUE = XRAYLIB_SETCONFIG_PARAMCHANGE,
}XRAYLIB_SETCONFIG_CHANNEL;


/**************图像设置相关******************/
///图像设置-指定图像能量模式
typedef enum _XRAYLIB_ENERGY_MODE_ 
{
     XRAYLIB_ENERGY_DUAL	= 0x00000000, //双能
	 XRAYLIB_ENERGY_HIGH,                 //单高
	 XRAYLIB_ENERGY_LOW,                  //单低
	 XRAYLIB_ENERGY_SCATTER,			  //背散射模式
	 XRAYLIB_ENERGY_UNKNOWN,              //增加unknown模式，作为库内构造函数值，防止外部不指定模式

	 XRAYLIB_ENERGY_MODE_MIN_VALUE = XRAYLIB_ENERGY_DUAL,
	 XRAYLIB_ENERGY_MODE_MAX_VALUE = XRAYLIB_ENERGY_UNKNOWN,
}XRAYLIB_ENERGY_MODE;

///图像设置-指定获取YUV和RGB图像时的色彩模式
typedef enum _XRAYLIB_COLOR_MODE_
{
    XRAYLIB_DUALCOLOR = 0x00000000,         /* 彩色模式*/
    XRAYLIB_GRAY,                           /* 黑白模式*/
    XRAYLIB_ORGANIC,                        /* 有机物剔除模式*/
    XRAYLIB_ORGANIC_PLUS,                   /* 有机物剔除+ 模式*/
    XRAYLIB_INORGANIC,                      /* 无机物剔除模式*/
    XRAYLIB_INORGANIC_PLUS,                 /* 无机物剔除+ 模式*/
    XRAYLIB_Z789,				            /* 可疑有机物增强模式，原子序数7，8，9*/
    XRAYLIB_Z7,				                /* 可疑有机物增强模式，原子序数7*/
    XRAYLIB_Z8,				                /* 可疑有机物增强模式，原子序数8*/
    XRAYLIB_Z9,				                /* 可疑有机物增强模式，原子序数9*/

	XRAYLIB_COLOR_MIN_VALUE = XRAYLIB_DUALCOLOR,
	XRAYLIB_COLOR_MAX_VALUE = XRAYLIB_Z9,
} XRAYLIB_COLOR_MODE;

////双能颜色表选择
typedef enum _XRAYLIB_DUALCOLORTABLE_SEL_
{
	XRAYLIB_DUALCOLORTABLE_SEL1 = 0x00000000,		/* 颜色表1，Fiscan */
	XRAYLIB_DUALCOLORTABLE_SEL2,					/* 颜色表2，Default */
	XRAYLIB_DUALCOLORTABLE_SEL3,					/* 颜色表3，Nuctech */
	XRAYLIB_DUALCOLORTABLE_SEL4,					/* 颜色表4，Antianxia */
	XRAYLIB_DUALCOLORTABLE_PSEUDO,					/* 伪彩颜色表 */
	XRAYLIB_DUALCOLORTABLE_NUM,						/* 颜色表数量 */

	XRAYLIB_DUALCOLORTABLE_MIN_VALUE = XRAYLIB_DUALCOLORTABLE_SEL1,
	XRAYLIB_DUALCOLORTABLE_MAX_VALUE = XRAYLIB_DUALCOLORTABLE_PSEUDO,
}XRAYLIB_DUALCOLORTABLE_SEL;

/////图像设置-穿透模式，高穿、低穿or正常
typedef enum _XRAYLIB_PENETRATION_MODE_
{
    XRAYLIB_PENETRATION_NORMAL = 0x00000000,         /* 正常*/
    XRAYLIB_PENETRATION_LOWPENE,                     /* 低穿*/
    XRAYLIB_PENETRATION_HIGHPENE,                    /* 高穿*/

    XRAYLIB_PENETRATION_MODE_MIN_VALUE = XRAYLIB_PENETRATION_NORMAL,
	XRAYLIB_PENETRATION_MODE_MAX_VALUE = XRAYLIB_PENETRATION_HIGHPENE,
}XRAYLIB_PENETRATION_MODE; 

/////图像设置-增强模式，边增、超增or正常
typedef enum _XRAYLIB_ENHANCE_MODE_
{
	XRAYLIB_ENHANCE_NORAML = 0x00000000,        /* 正常*/
    XRAYLIB_ENHANCE_SUPER = 0x00000001,         /* 超增*/
    XRAYLIB_ENHANCE_EDGE1 = 0x00000002,         /* 边缘增强1*/
	XRAYLIB_SPECIAL_LOCALENHANCE = 0x00000003,  /* 局增 */
	XRAYLIB_ENHANCE_MODE_MIN_VALUE = XRAYLIB_ENHANCE_NORAML,
	XRAYLIB_ENHANCE_MODE_MAX_VALUE = XRAYLIB_SPECIAL_LOCALENHANCE,
} XRAYLIB_ENHANCE_MODE; 

//图像设置-指定获取YUV及RGB图像时是否反显
typedef enum _XRAYLIB_INVERSE_MODE_
{
	XRAYLIB_INVERSE_NONE = 0x00000000,  //不反色显示
    XRAYLIB_INVERSE_INVERSE,            //反色显示

	XRAYLIB_INVERSE_MIN_VALUE = XRAYLIB_INVERSE_NONE,
	XRAYLIB_INVERSE_MAX_VALUE = XRAYLIB_INVERSE_INVERSE,
}XRAYLIB_INVERSE_MODE;

//图像设置-选择单能颜色表
typedef enum _XRAYLIB_SINGLECOLORTABLE_SEL_
{
	XRAYLIB_SINGLECOLORTABLE_SEL1 = 0x00000000,
	XRAYLIB_SINGLECOLORTABLE_SEL2,

	XRAYLIB_SINGLECOLORTABLE_MIN_VALUE = XRAYLIB_SINGLECOLORTABLE_SEL1,
	XRAYLIB_SINGLECOLORTABLE_MAX_VALUE = XRAYLIB_SINGLECOLORTABLE_SEL2,
}XRAYLIB_SINGLECOLORTABLE_SEL;

//图像设置-抗锯齿模式，打开or关闭
typedef enum _XRAYLIB_ANTIALIASING_MODE_
{
	XRAYLIB_ANTIALIASING_CLOSE = 0x00000000,       /* 抗锯齿关闭*/
	XRAYLIB_ANTIALIASING_OPEN,                     /* 抗锯齿打开*/

	XRAYLIB_ANTIALIASING_MIN_VALUE = XRAYLIB_ANTIALIASING_CLOSE,
	XRAYLIB_ANTIALIASING_MAX_VALUE = XRAYLIB_ANTIALIASING_OPEN,
}XRAYLIB_ANTIALIASING_MODE;

//背散射图像模式：正常模式、指标模式
typedef enum _XRAYLIB_BSIMAGE_MODE_
{
	XRAYLIB_BSIMAGE_NORMAL = 0x00000000,       /* 正常模式*/
	XRAYLIB_BSIMAGE_INDEX = 0x00000001,        /* 指标模式*/

	XRAYLIB_BSIMAGE_MIN_VALUE = XRAYLIB_BSIMAGE_NORMAL,
	XRAYLIB_BSIMAGE_MAX_VALUE = XRAYLIB_BSIMAGE_INDEX,
}XRAYLIB_BSIMAGE_MODE;

/////////////////////////////图像设置标识值
typedef enum _XRAYLIB_CONFIG_IMAGE_KEY_
{
	XRAYLIB_ENERGY = 0x00000000,      //能量模式
	XRAYLIB_COLOR,       //彩色模式
	XRAYLIB_PENETRATION, //穿透模式
	XRAYLIB_ENHANCE,     //增强模式
	XRAYLIB_INVERSE,     //反色模式
	XRAYLIB_GRAYLEVEL,   //可变吸收率 范围0 - （XRAY_LIB_GRAYLEVEL-1），传入-1时还原
	XRAYLIB_BRIGHTNESS,  //增亮或减暗，范围0 - XRAY_LIB_BRIGHTNESSLEVEL, （默认0-101，50为正常显示，增加是增亮，减小是变暗）
	XRAYLIB_SINGLECOLORTABLE, //单能颜色表切换
	XRAYLIB_DUALCOLORTABLE,   //双能颜色表切换
	XRAYLIB_COLORSIMAGING,    //3色或6色切换

	XRAYLIB_CONFIG_IMAGE_KEY_MIN_VALUE = XRAYLIB_ENERGY,
	XRAYLIB_CONFIG_IMAGE_KEY_MAX_VALUE = XRAYLIB_COLORSIMAGING,
}XRAYLIB_CONFIG_IMAGE_KEY;
/*********************************************/

/**************参数设置相关******************/
///////////////////////参数设置-图像翻转枚举型
typedef enum _XRAYLIB_IMG_ROTATE_
{
	XRAYLIB_MOVE_RIGHT = 0x00000000,            //实时向右滚屏(默认)
	XRAYLIB_MOVE_LEFT,                          //实时向左滚屏

	XRAYLIB_ENTIRE_MOVE_RIGHT,                  //回拉(转存)向右滚屏
	XRAYLIB_ENTIRE_MOVE_LEFT,                   //回拉(转存)向左滚屏

	XRAYLIB_ROTATE_MIN_VALUE = XRAYLIB_MOVE_RIGHT,
	XRAYLIB_ROTATE_MAX_VALUE = XRAYLIB_ENTIRE_MOVE_LEFT,
}XRAYLIB_IMG_ROTATE;

//////////////////////参数设置-图像镜像枚举类型
typedef enum _XRAYLIB_IMG_MIRROR_
{
	XRAYLIB_MIRROR_NONE = 0x00000000,                 //无镜像
	XRAYLIB_MIRROR_UPDOWN,                            //上下镜像

	XRAYLIB_MIRROR_MIN_VALUE = XRAYLIB_MIRROR_NONE,
	XRAYLIB_MIRROR_MAX_VALUE = XRAYLIB_MIRROR_UPDOWN,
}XRAYLIB_IMG_MIRROR;

//////////////////////参数设置-扫描模式枚举类型
typedef enum _XRAYLIB_SCAN_MODE_
{
	XRAYLIB_SCAN_MODE_NORMAL = 0x00000000,              //正常扫描模式
	XRAYLIB_SCAN_MODE_FORCE,                            //强扫模式

	XRAYLIB_SCAN_MODE_MIN_VALUE = XRAYLIB_SCAN_MODE_NORMAL,
	XRAYLIB_SCAN_MODE_MAX_VALUE = XRAYLIB_SCAN_MODE_FORCE,
}XRAYLIB_SCAN_MODE;

//////////////////////参数设置-默认增强-后续开放几种不同的默认增强方式和参数
typedef enum _XRAYLIB_DEFAULT_ENHANCE_
{
	XRAYLIB_DEFAULTENHANCE_CLOSE = 0x00000000,
	XRAYLIB_DEFAULTENHANCE_MODE0,
	XRAYLIB_DEFAULTENHANCE_MODE1,
	XRAYLIB_DEFAULTENHANCE_MODE2,
	XRAYLIB_DEFAULTENHANCE_MODE3,

	XRAYLIB_DEFAULTENHANCE_MIN_VALUE = XRAYLIB_DEFAULTENHANCE_CLOSE,
	XRAYLIB_DEFAULTENHANCE_MAX_VALUE = XRAYLIB_DEFAULTENHANCE_MODE3,
}XRAYLIB_DEFAULT_ENHANCE;

//////////////////////参数设置-融合模式-
typedef enum _XRAYLIB_FUSION_MODE_
{
	XRAYLIB_FUSION_DEFAULT = 0x00000000, //原始融合
	XRAYLIB_FUSION_DEFAULT2,             //融合方式二   
	XRAYLIB_FUSION_MIN_VALUE = XRAYLIB_FUSION_DEFAULT,
	XRAYLIB_FUSION_MAX_VALUE = XRAYLIB_FUSION_DEFAULT2,
}XRAYLIB_FUSION_MODE;

// 参数设置-可疑物报警灵敏度
typedef enum _XRAYLIB_CONCERNEDM_SENSITIVITY_
{
	XRAYLIB_CONCERNEDMSENS_LOW = 0x00000000,
	XRAYLIB_CONCERNEDMSENS_MEDIUM,
	XRAYLIB_CONCERNEDMSENS_HIGH,

	XRAYLIB_CONCERNEDMSENS_MIN_VALUE = XRAYLIB_CONCERNEDMSENS_LOW,
	XRAYLIB_CONCERNEDMSENS_MAX_VALUE = XRAYLIB_CONCERNEDMSENS_HIGH,
}XRAYLIB_CONCERNEDM_SENSITIVITY;

//////////////////////参数设置-可疑物报警灵敏度
typedef enum _XRAYLIB_LOWPENE_SENSITIVITY_
{
	XRAYLIB_LOWPENESENS_LOW = 0x00000000,
	XRAYLIB_LOWPENESENS_MEDIUM,
	XRAYLIB_LOWPENESENS_HIGH,

	XRAYLIB_LOWPENESENS_MIN_VALUE = XRAYLIB_LOWPENESENS_LOW,
	XRAYLIB_LOWPENESENS_MAX_VALUE = XRAYLIB_LOWPENESENS_HIGH,
}XRAYLIB_LOWPENE_SENSITIVITY;

//////////////////////爆炸物检测算法选择
typedef enum _XRAYLIB_EXPLOSIVEDETECTION_MODE_
{
	XRAYLIB_EXPLOSIVEDETECTION_1 = 0x00000000,  //简单场景爆炸物检测
	XRAYLIB_EXPLOSIVEDETECTION_2,               //复杂场景爆炸物检测
	XRAYLIB_EXPLOSIVEDETECTION_MIN_VALUE = XRAYLIB_EXPLOSIVEDETECTION_1,
	XRAYLIB_EXPLOSIVEDETECTION_MAX_VALUE = XRAYLIB_EXPLOSIVEDETECTION_2,
}XRAYLIB_EXPLOSIVEDETECTION_MODE;

/////////////////////////////参数设置标识值
typedef enum _XRAYLIB_CONFIG_PARAM_KEY_
{
	//////图像校正参数
	XRAYLIB_PARAM_WHITENUP = 0x00000000,				    //	0x00000000,		置白参数，上端置白像素数（int型）
	XRAYLIB_PARAM_WHITENDOWN,				                //	0x00000001,		置白参数，下端置白像素数（int型）
	XRAYLIB_PARAM_BACKGROUNDTHRESHOLED,	                    //	0x00000002,		置白参数，背景阈值（0~65535，int型）
	XRAYLIB_PARAM_BACKGROUNDGRAY,			                //	0x00000003,		置白参数，背景统一设置灰度值（0~65535，int型）
	XRAYLIB_PARAM_RTUPDATEAIRRATIO,		                    //	0x00000004,		实时更新空气参数，实时更新空气比例（0.0~1.0，float型）
	XRAYLIB_PARAM_AIRTHRESHOLDHIGH,		                    //	0x00000005,		实时更新空气参数，高能判断是否空气阈值（0~65535，int型）
	XRAYLIB_PARAM_AIRTHRESHOLDLOW,			                //	0x00000006,		实时更新空气参数，低能判断是否空气阈值（0~65535，int型）
	XRAYLIB_PARAM_RTUPDATE_LOWGRAY_RATIO,	                //	0x00000007,		低穿透区不一致性校正表实时更新比例,（0.0~1.0，float型）
	XRAYLIB_PARAM_PACKAGEGRAY,				                //	0x00000008,		包裹分割参数，包裹阈值（0~65535，int型）
	XRAYLIB_PARAM_IMAGEWIDTH,				                //	0x00000009,		校正表宽度（该功能已不再使用）
	XRAYLIB_PARAM_COLDHOTTHRESHOLED,		                //	0x0000000a,		冷热源参数阈值（0~800，int型）
	XRAYLIB_PARAM_BELTGRAINLIMIT,                           //	0x0000000b,		皮带纹去除参数

	///图像处理参数
	XRAYLIB_PARAM_DUAL_FILTERKERNEL_LENGTH = 0x10000000,	//	0x10000000,		双能分辨时滤波核长度（int型，一般1~5，太大影响速度）
	XRAYLIB_PARAM_DUALFILTER_RANGE,					        //	0x10000001,		双能分辨时滤波范围(一般0.1~0.15，float型）
	XRAYLIB_PARAM_DUALDISTINGUISH_GRAYUP ,				    //	0x10000002,		双能分类灰度上限，超过此值为灰色（0~65535，int型）
	XRAYLIB_PARAM_DUALDISTINGUISH_GRAYDOWN ,			    //	0x10000003,		双能分类灰度下限，低于此值为灰色（0~65535，int型）
	XRAYLIB_PARAM_DUALDISTINGUISH_NONEINORGANIC,		    //	0x10000004,		无机物分类上限，超过此值统一置为有机物（0~65535，int型）
	XRAYLIB_PARAM_DEFAUL_ENHANCE,						    //	0x10000005,		默认增强开关
	XRAYLIB_PARAM_DEFAULTENHANCE_INTENSITY,			        //	0x10000006,		默认增强强度（float型） 
	XRAYLIB_PARAM_EDGE1ENHANCE_INTENSITY,				    //	0x10000007,		边缘增强强度（float型）
	XRAYLIB_PARAM_SUPERENHANCE_INTENSITY,				    //	0x10000008,		超级增强强度（float型）
	XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDUP,			    //	0x10000009,		局增灰度值上限（0~65535，int型）
	XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDDOWN,		        //	0x1000000a,		局增灰度值下限（0~65535，int型）
	XRAYLIB_PARAM_SPECIALENHANCE_STRETCHGRAYUP,		        //	0x1000000b,		局增灰度拉伸上限（0~65535，int型）
	XRAYLIB_PARAM_SPECIALENHANCE_STRETCHGRAYDOWN,		    //	0x1000000c,		局增灰度拉伸下限（0~65535，int型）
	XRAYLIB_PARAM_CALILOWGRAY_THRESHOLD,				    //	0x1000000d,		局增时，低灰度不一致校正归一化值（0~65535，int型）
	XRAYLIB_PARAM_DENOISING_INTENSITY,				        //	0x1000000e,		降噪强度
	XRAYLIB_PARAM_COMPOSITIVE_RATIO,					    //	0x1000000f, 	对比度调整（该功能已不再使用）
	XRAYLIB_PARAM_DENOISING_MODE,					        //	0x10000010, 	降噪模式
	XRAYLIB_PARAM_ANTIALIASING_MODE,					    //	0x10000011, 	抗锯齿模式
	XRAYLIB_PARAM_AIDEVICE_MODE,					        //	0x10000012, 	AI降噪模式选择（CPU/GPU）
	XRAYLIB_PARAM_HEIGHT_RESIZE_FACTOR,				        //	0x10000013, 	图像高度拉伸系数(float)时间方向
	XRAYLIB_PARAM_WIDTH_RESIZE_FACTOR,					    //	0x10000014, 	图像宽度拉伸系数(float)探测器方向
	XRAYLIB_PARAM_FUSION_MODE,					            //	0x10000015, 	融合模式
	XRAYLIB_PARAM_BACKGROUNDCOLOR,                          //	0x10000016, 	图像背景色设置（| A 24~32bit | R 16~23bit | G 8~15bit | B 0~7bit |，int型）
	XRAYLIB_PARAM_STRETCHRATIO,                             //	0x10000017, 	图像拉伸比例设置（0.8~1.0，float型）
	XRAYLIB_PARAM_GAMMA_MODE,					            //	0x10000018, 	GAMMA模式开关（该功能已不再使用）
	XRAYLIB_PARAM_GAMMA_INTENSITY,				            //	0x10000019, 	GAMMA增强强度（该功能已不再使用）
	XRAYLIB_PARAM_SIGMA_NOISES,				                //	0x1000001a, 	传统XSP与AIXSP加权强度（float型,0.0~1.0）
	XRAYLIB_PARAM_HIGH_PENE_INTENSITY,				        //	0x1000001b, 	高穿与正常融合强度（float型）
	XRAYLIB_PARAM_LOW_PENE_INTENSITY,				        //	0x1000001c, 	低穿与正常融合强度（float型）
	XRAYLIB_PARAM_MERGE_BASELINE,                           //	0x1000001d, 	融合图基线拉伸值（该功能已不再使用）
	XRAYLIB_PARAM_TEST_MODE,								//	0x1000001e, 	测试体类型（0~2，int型）
	XRAYLIB_PARAM_TCSTRIPENUM,						        //	0x1000001f, 	实时处理缓存条带数量（1~XRAT_LIB_MAX_CACHED_RT_SLICE_NUM，int型）
	XRAYLIB_PARAM_TCENHANCE_INTENSITY,						//	0x10000020, 	测试体增强等级（0-100 int型）
	XRAYLIB_PARAM_LUMINANCE_INTENSITY,						//	0x10000021, 	设置图像参数亮度（0-100 int型）
	XRAYLIB_PARAM_CONTRANST_INTENSITY,						//	0x10000022, 	设置图像参数对比度(0-100 int型）
	XRAYLIB_PARAM_SHARPNESS_INTENSITY,						//	0x10000023, 	设置图像参数锐度（0-100 int型）
	XRAYLIB_PARAM_LOW_COMPENSATION,							//	0x10000024, 	难穿区补偿（0-100 int型）
	XRAYLIB_PARAM_HIGH_SENSITIVITY,							//	0x10000025, 	易穿区敏感度（0-100 int型）
	XRAYLIB_PARAM_MIXTURE_COLOR_RANGEDOWN,					//	0x10000026, 	混合物颜色范围下限（0-100，int型，默认值50），高于50压缩混合物色域，扩大有机物色域
	XRAYLIB_PARAM_MIXTURE_COLOR_RANGEUP,					//	0x10000027, 	混合物颜色范围上限（0-100，int型，默认值50），高于50扩大混合物色域，压缩无机物色域
	
	///功能类参数
	XRAYLIB_PARAM_ROTATE = 0x20000000,						//	0x20000000,		图像翻转方式
	XRAYLIB_PARAM_MIRROR,						            //	0x20000001,		图像上下镜像
	XRAYLIB_PARAM_SCAN_MODE,					            //	0x20000002,		扫描模式，XRAYLIB_SCAN_MODE，设置强扫时包裹分割功能失效
	XRAYLIB_PARAM_MATERIAL_ADJUST,				            //	0x20000003,		颜色微调，直接作用于输出的原子序数（float型，-10.0 ~ 10.0）
	XRAYLIB_PARAM_XRAYDOSE_CALI,				            //	0x20000004,		剂量波动修正（原有外部传入参数修正空气模板，算法内部已弃用）
	XRAYLIB_PARAM_GEOMETRIC_CALI,				            //	0x20000005,		几何校正开关
	XRAYLIB_PARAM_GEOMETRIC_INVERSE,						//	0x20000006,		几何校正是否置反
	XRAYLIB_PARAM_LOWPENETRATION_PROMPT,		            //	0x20000007,		低穿透报警
	XRAYLIB_PARAM_LOWPENETRATION_SENSTIVTY,	                //	0x20000008,		低穿透报警灵敏度
	XRAYLIB_PARAM_LOWPENETHRESHOLD,			                //	0x20000009,		低穿透报警阈值（0~65535，int型）
	XRAYLIB_PARAM_LOWPENEWARNTHRESHOLD,		                //	0x2000000a,		低穿透停带阈值（0~1.0，float型）
	XRAYLIB_PARAM_CONCERDMATERIAL_PROMPT,		            //	0x2000000b,		可疑有机物报警开关
	XRAYLIB_PARAM_CONCERDMATERIAL_SENSTIVTY,	            //	0x2000000c,		可疑有机物灵敏度
	XRAYLIB_PARAM_FLATDETMETRIC_CALI,                       //	0x2000000d,		探测器平铺校正开关
	XRAYLIB_PARAM_SPEED,                                    //	0x2000000e,		安检机速度（0~3，float型）
	XRAYLIB_PARAM_RT_HEIGHT,                                //	0x2000000f,		实时条带高度设置，用于重新初始化CNN模型
	XRAYLIB_PARAM_SPEEDGEAR,                                //	0x20000010,		实时速度档位设置，用于重新初始化NIEE模型
	XRAYLIB_PARAM_COLDANDHOT_CALI,                          //	0x20000011,		冷热源校正开关, 开启时冷热源不再根据时间做判断
	XRAYLIB_PARAM_DOSE_CALI,                                //	0x20000012,		剂量波动修正开关
	XRAYLIB_PARAM_SEG_AI,                                   //	0x20000013,		语义分割开关
	XRAYLIB_PARAM_DRUGMATERIAL_PROMPT,			            //	0x20000014,		毒品检测报警开关
	XRAYLIB_PARAM_EXPLOSIVEMATERIAL_PROMPT,	                //	0x20000015,		爆炸物检测报警开关
	XRAYLIB_PARAM_CURVE_STATE,                              //	0x20000016,		曲线状态获取开关（XRAYLIB_OFF为默认曲线，XRAYLIB_ON为自动标定曲线， int）
	XRAYLIB_PARAM_EUTEST_PROMPT,			                //	0x20000017,		欧标测试体&美标测试体局增开关（默认XRAYLIB_ON）
	XRAYLIB_PARAM_HOMOFILTER_MODE,						    //	0x20000018,		同态滤波开关
	XRAYLIB_PARAM_LOGSAVEMODE,								//	0x20000019,		日志保存模式
	XRAYLIB_PARAM_LOGLEVEL,									//	0x2000001a,		日志打印等级
	XRAYLIB_PARAM_TIMECOST_SHOW,							//  0x2000001b,		耗时统计打印
	XRAYLIB_PARAM_EXPLOSIVEDETECTION_MODE,					//  0x2000001c,		爆炸物检测模式
	XRAYLIB_PARAM_GEOMETRIC_CALI_RATIO,						//  0x2000001d,		几何校正比例(0~100, int型, 100完全畸变校正, 0无畸变矫正, 算法默认配置值等效60)
}XRAYLIB_CONFIG_PARAM_KEY;
/*********************************************/


/***************设置函数参数结构体*************/
//////参数设置结构体,图像处理通道
typedef struct _XRAYLIB_CONFIG_IMAGE_
{
	XRAYLIB_CONFIG_IMAGE_KEY key;
	void* value;   //////设置图像处理类型,传入对应模式的枚举指针
}XRAYLIB_CONFIG_IMAGE;

//////参数设置结构体,参数设置通道
typedef struct _XRAYLIB_CONFIG_PARAM_
{
	XRAYLIB_CONFIG_PARAM_KEY key;
	int numeratorValue;
	int denominatorValue; ///分别是分子和分母，float型使用两者相除表示。整型设置分母等于1即可
}XRAYLIB_CONFIG_PARAM;

/***************校正表*************/
//////获取校正表用结构体
typedef struct _XRAYLIB_CALI_TABLE_
{
	int nCaliTableHeight;  ///探测器排数，目前均为单排探测器，强制设为1；为以后可能拓展考虑
	int nCaliWidth;   ///探测器方向列数，即 探测器板卡数*每个探测器板卡晶体数。该值可从算法库内获取。
	XRAYLIB_ENERGY_MODE EnergyMode;
	XRAYLIB_UPDATE_ZEROANDFULL_MODE UpdateMode;
	////////空气、本底的内存大小均为nCaliTableHeight*nCaliWidth；
	unsigned short* pCaliTableAirHigh;
	unsigned short* pCaliTableAirLow;
	unsigned short* pCaliTableBackgroundHigh;
	unsigned short* pCaliTableBackgroundLow;
}XRAYLIB_CALI_TABLE;

typedef enum _XRAY_LIB_REGONMODE_
{
	REGONMODE_Z789		= 0x00000000,
	REGONMODE_LOWPENE,
	REGONMODE_EXPLOSIVE,
	REGONMODE_DRUG,

	REGONMODE_MIN_VALUE = REGONMODE_Z789,
	REGONMODE_MAX_VALUE = REGONMODE_DRUG,
}XRAY_LIB_REGONMODE;

//////设备类型相关信息
typedef struct _XRAY_DEVICEINFO_
{
	XRAY_DEVICETYPE xraylib_devicetype;        //设备类型
	XRAY_LIB_VISUAL xraylib_visualNo;          //视角信息
	XRAYLIB_ENERGY_MODE xraylib_energymode;    //能量模式
	XRAY_DETECTORNAME xraylib_detectortype;    //探测器类型
	XRAY_DETECTORNAME xraylib_transmittertype; //传输板类型
	XRAYLIB_SOURCE_TYPE_E xraylib_sourcetype;  //射线源类型
}XRAY_DEVICEINFO;

//实时速度档位
typedef enum _XRAY_SPEEDGEAR_
{
	XRAYLIB_SPEED_LOW = 0x00000000,     //出图速度 低
	XRAYLIB_SPEED_MEDIUM,  //出图速度 中
	XRAYLIB_SPEED_HIGH,    //出图速度 高
	XRAYLIB_SPEED_SHIGH,   //出图速度 超高
	XRAYLIB_SPEED_UHIGH,   //出图速度 极高
	XRAYLIB_SPEED_SUHIGH,  //出图速度 超级高

	XRAYLIB_SPEED_MIN_VALUE = XRAYLIB_SPEED_LOW,
	XRAYLIB_SPEED_MAX_VALUE = XRAYLIB_SPEED_SUHIGH,
}XRAY_SPEEDGEAR;

//曲线类别
typedef enum _XRAY_RCURVE_TYPE_
{
	XRAYLIB_CURVE_NORMAL = 0x00000000,     // 常规曲线
	XRAYLIB_CURVE_MIXTURE,                 // 混合曲线

	XRAYLIB_CURVE_TYPE_MIN_VALUE = XRAYLIB_CURVE_NORMAL,
	XRAYLIB_CURVE_TYPE_MAX_VALUE = XRAYLIB_CURVE_MIXTURE,
}XRAY_RCURVE_TYPE;

//测试体类型
typedef enum _XRAY_TESTMODE_
{
	XRAYLIB_TESTMODE_CN = 0x00000000,     //国标测试体（通用场景）
	XRAYLIB_TESTMODE_EUR,				  //欧标测试体（通用场景）
	XRAYLIB_TESTMODE_USA,				  //美标测试体（通用场景）
	XRAYLIB_TESTMODE_CN_CHECK,            //国标测试体（验收场景）

	XRAYLIB_TESTMODE_MIN_VALUE = XRAYLIB_TESTMODE_CN,
	XRAYLIB_TESTMODE_MAX_VALUE = XRAYLIB_TESTMODE_CN_CHECK,
}XRAY_TESTMODE;

typedef struct _XRAYLIB_TUNE_PARAMS_
{
	int nParamGrayValueLL;                     //需要调整参数所在的灰度区间（闭）[nParamGrayValueLL,nParamGrayValueUL]
	int nParamGrayValueUL;     
	int nParamRValueLL;                        //需要调整参数所在的原子序数区间（闭）[nParamRValueLL,nParamRValueUL]
	int nParamRValueUL;      
	int nParamOffset;                          //参数调整值
}XRAYLIB_TUNE_PARAMS;

// 三色R曲线调整结构体,high对应薄区域，low对应厚区域，e.g. nAlGrayHighFactor=50,则Al的薄区域增强系数为50，厚区域不变
typedef struct _XRAYLIB_RCURVE_ADJUST_PARAMS_
{
	int nAlGrayHighFactor;            			// 薄铝调整系数，范围[0-100],默认50，下同
	int nAlGrayLowFactor;						// 厚铝调整系数
	int nCGrayHighFactor;            			// 薄碳调整系数，保留变量，初始化为50 
	int nCGrayLowFactor;                   		// 厚碳调整系数，保留变量，初始化为50	
	int nFeGrayHighFactor;            			// 薄铁调整系数
	int nFeGrayLowFactor;            			// 厚铁调整系数
}XRAYLIB_RCURVE_ADJUST_PARAMS;

typedef struct _XRAYLIB_MATERIAL_COLOR_PARAMS_
{
	float fTest5RatioCN;                         // 国标test5区域增强强度
	int nGrayTuneNum;                            // 灰度调整区域个数
	int nRTuneNum;                               // 原子序数调整区域个数
	XRAYLIB_TUNE_PARAMS stGrayValue[3];          // 灰度调整
	XRAYLIB_TUNE_PARAMS stRValue[3];             // 原子序数调整
	XRAYLIB_RCURVE_ADJUST_PARAMS stRCurveAdjust; // R曲线调整系数
}XRAYLIB_MATERIAL_COLOR_PARAMS;

//XSPTool单张灰度图处理校正开关
typedef struct _XRAYLIB_XSP_CALI_
{
	XRAY_LIB_ONOFF enDoseCaliMertric;					    // 剂量波动矫正开关
	XRAY_LIB_ONOFF enCaliMertric;							// 归一化矫正开关
	XRAY_LIB_ONOFF enFlatDetMertric;					    // 探测器平铺校正开关
	XRAY_LIB_ONOFF enGeoMertric;						    // 几何校正开关
	XRAY_LIB_ONOFF enStretchMertric;						// 拉伸校正开关
	XRAY_LIB_ONOFF enBeltRemoveMetric;						// 皮带纹去除开关
	XRAY_LIB_ONOFF enDtecMergeMetric;						// 探测器边界融合开关
}XRAYLIB_XSP_CALI;

//XSPTool单张灰度图处理降噪增强开关
typedef struct _XRAYLIB_XSP_ENHANCE_
{
	XRAY_LIB_ONOFF enFusionMode1;
	XRAY_LIB_ONOFF enFusionMode2;
	XRAYLIB_GAMMA_MODE enGammaMode;
	XRAYLIB_DEFAULT_ENHANCE enEnhanceMode;
	XRAY_LIB_ONOFF enCompositiveMode;
	XRAY_LIB_ONOFF enBeepsMode;
	XRAY_LIB_ONOFF enEdgeEnhanceMode;
	XRAY_LIB_ONOFF enSuperEnhanceMode;
	XRAY_LIB_ONOFF enSuperEnhanceMode2;
	XRAY_LIB_ONOFF enLocalEnhanceMode;
	XRAY_LIB_ONOFF enAiyuvMode;
	XRAY_LIB_ONOFF enCalRValueMode;
}XRAYLIB_XSP_ENHANCE;

#endif
