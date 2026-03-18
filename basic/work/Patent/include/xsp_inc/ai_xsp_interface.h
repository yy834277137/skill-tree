/******************************************************************************

   Copyright (C), 2022, ArashVision

******************************************************************************
   File Name       : ai_xsp_interface.h
   Version         : V1.0
   Author          : zongkai5
   Created         : 2022-3-16
   Description     : 作为后续引擎推理的接口整理,给算法调用,由于不同平台&
                     不用硬件，提供给算法的统一接口，将底层环境依赖和依赖库
                     和算法做隔离
******************************************************************************/
#ifndef _AI_XSP_INTERFACE_
#define _AI_XSP_INTERFACE_

#ifdef __cplusplus
extern "C" {
#endif

/*接口版本号*/
#define AI_XSP_MAJOR_VERSION	1
#define AI_XSP_SUB_VERSION		0
#define AI_XSP_REVISION_VERSION 0

#define AI_XSP_VERSION_YEAR		2022
#define AI_XSP_VERSION_MONTH	3
#define AI_XSP_VERSION_DAY		15

#define AI_XSP_MAX_CHAN         3    /*最多支持模型通道数*/

/*硬件平台*/
typedef enum _SYSTME_PLAT_
{
    SYS_SIHI_PLAT = 0,               /*海思平台*/
    SYS_X86_PLAT = 1,                /*x86平台*/
    SYS_NT_PLAT = 2,                 /*NT平台*/
    SYS_RK_PLAT = 3,                 /*RK平台*/
    SYS_PLAT_NUM
} SYSTEM_PLAT_E;


/*推理选项*/
typedef enum _SYSTEM_PROCESS_TYPE_
{
    ARM_HARD_WARE_CORE = 0,          /*ARM平台CNN加速核推理*/
    X86_CPU = 1,                     /*x86平台CPU加速核推理*/
    X86_INTEL_GPU = 2,               /*x86平台核显加速核推理*/
    X86_NVIDIA_GPU = 3,              /*x86平台独显加速核推理*/
    SYS_PROCESS_NUM
} SYSTEM_PROCESS_TYPE_E;

/*模型输入数据类型,用于申请输入buff缓存用*/
typedef enum _SYSTEM_DATA_TYPE_
{
    TENSOR_FLOAT32 = 0,                            /* data type is float32. */
    TENSOR_FLOAT16,                                /* data type is float16. */
    TENSOR_INT8,                                   /* data type is int8. */
    TENSOR_UINT8,                                  /* data type is uint8. */
    TENSOR_INT16,                                  /* data type is int16. */
    TENSOR_UINT16,                                 /* data type is uint16. */
    TENSOR_INT32,                                  /* data type is int32. */
    TENSOR_UINT32,                                 /* data type is uint32. */
    TENSOR_INT64,                                  /* data type is int64. */
    TENSOR_BOOL,

    TENSOR_TYPE_MAX,
}SYSTEM_INPUT_DATA_TYPE, SYSTEM_OUTPUT_DATA_TYPE;


/*推理精度选项,分别为 8-bit  16-bit 32-bit*/
typedef enum _SYSTEM_PROCESS_PRECISION_
{
    PROCESS_UINT8 = 0,              /*8-bit unsigned integer value推理*/
    PROCESS_UINT12 = 1,             /*12-bit unsigned integer value推理*/
    PROCESS_UINT16 = 2,             /*16-bit unsigned integer value推理*/
    PROCESS_UINT32 = 3,             /*32-bit unsigned integer value推理*/
    PROCESS_FP16 = 4,               /*16-bit floating point value推理*/
    PROCESS_FP32 = 5,               /*32-bit floating point value推理*/
    PROCESS_TYPE_NUM
} SYSTEM_PROCESS_PRECISION_E;

/* 指定工作的NPU/GPU核心(ARM/x86平台) */
typedef enum _SYSTEM_CORE_MASK {
    SYSTEM_NPU_CORE_0 = 1,                                 /* run on NPU core 0. */
    SYSTEM_NPU_CORE_1 = 2,                                 /* run on NPU core 1. */
    SYSTEM_NPU_CORE_2 = 4,                                 /* run on NPU core 2. */
    SYSTEM_GPU_CORE_0 = 0,                                 /* run on GPU core 0. */
    SYSTEM_GPU_CORE_1 = 1,                                 /* run on GPU core 1. */

    SYSTEM_CORE_UNDEFINED = 255,
} SYSTEM_CORE_MASK;

/*模型平台相关参数结构体*/
typedef struct _AI_MODEL_PLAT_PRM_
{
    /* pass through mode, for rknn_set_io_mem interface.
       if TRUE, the buf data is passed directly to the input node of the rknn model
                without any conversion. the following variables do not need to be set.
       if FALSE, the buf data is converted into an input consistent with the model
                 according to the following type and fmt. so the following variables
                 need to be set.*/
    char Input_Pass_Through;                          /* input pass through mode 仅RK平台有效 */
    char Output_Pass_Through;                         /* output pass through mode 仅RK平台有效 */
    const char *pModelPath;                           /*模型路径*/
	SYSTEM_PLAT_E eSysTemplat;                        /*模型适用平台*/
	SYSTEM_INPUT_DATA_TYPE sSysInputDataType;         /*模型输入数据类型*/
    SYSTEM_OUTPUT_DATA_TYPE sSysOutputDataType;       /*模型输输出数据类型*/
	SYSTEM_PROCESS_TYPE_E eSysProcessType;            /*模型推理选项*/
	SYSTEM_PROCESS_PRECISION_E eSysPrecisionType;     /*模型推理量化精度*/
    SYSTEM_CORE_MASK eSysCoreMask;                    /*设置运行的NPU/GPU核心*/
}SYSTEM_MODEL_PLAT_PRM;

/*模型内部分辨率相关参数结构体*/
typedef struct _AI_MODEL_INSIDE_PRM_
{
    int inputChnSize;                  /*模型输入通道数*/
	int inputWid;                      /*模型输入通道wid*/
	int inputHei;                      /*模型输入通道hei*/
    int outputChnSize;                 /*模型输出通道数*/
	int outputWid;                     /*模型输出通道wid*/
	int outputHei;                     /*模型输出通道hei*/	
}SYSTEM_MODEL_INSIDE_PRM;


/**@fn       AI_XSP_Init
 * @brief    算法初始化推理引擎接口
 * @param1   chan                        [in]     - chan模型通道号
 * @param2   pModelPlatPrm          [in]     - 模型平台参数
 * @param3   pModelInsidePrm       [in]     - 模型内部参数
 * @return   错误码
 * @note
 */
int AI_XSP_Init(int chan,SYSTEM_MODEL_PLAT_PRM *pModelPlatPrm,SYSTEM_MODEL_INSIDE_PRM *pModelInsidePrm);

/**@fn       AI_XSP_DeInit
 * @brief    算法去初始化接口
 * @brief    主要用于资源释放
 * @return   错误码 or  SAL_SOK
 * @note
 */
int AI_XSP_DeInit(int chan);

/**@fn       AI_XSP_RunProcess
 * @brief    算法推理接口, 主要用于数据推理处理
 * @param1   chan                    [in]     - 通道号
 * @param2   pdata                   [in]     - 需要处理的数据
 * @param3   length                  [in]     - 需要处理的数据长度,和初始化的数据得对齐
 * @return   错误码
 * @note
 */
int AI_XSP_RunProcess(int chan ,void *pdata, int length);


/**@fn       AI_XSP_GetResult
 * @brief    算法推理结果获取接口
 * @param1   pResult                  [out]    - 结果数据地址
 * @param2   pResultLen               [out]    - 结果数据长度
 * @return   错误码
 * @note
 */
int AI_XSP_GetResult(int chan ,void **pResult, int *pResultLen);

/**@fn       AI_XSP_GetImgDecision
 * @brief     输入输出分辨率获取接口
 * @param1   pInImgWid               [out]    - 输入图像wid
 * @param2   pInImgHei                [out]    - 输入图像hei
 * @param3   pOutImgWid             [out]    - 输出图像wid
 * @param4   pOutImgHei              [out]    - 输出图像hei
 * @return   错误码
 * @note
 */
int AI_XSP_GetModelPrm(int chan,SYSTEM_MODEL_INSIDE_PRM *pModelPrm);

/**@fn       AI_XSP_GetVersion
 * @brief    应用版本获取接口
 * @param1   pVersionParam            [out]   - 封装层版本结构体
 * @param2   nSize                    [in]    - 结构体大小
 * @return   错误码
 * @note
 */
int AI_XSP_GetVersion(void *pVersionParam,
                      int nSize);

#ifdef __cplusplus
}
#endif
#endif


