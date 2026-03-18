/*******************************************************************************
* icf_hal.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2022年3月18日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef __ICF_HAL_H__
#define __ICF_HAL_H__

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "plat_com_inter_hal.h"
#include "ia_common.h"

#include "icf_common.h"
#include "icf_version_compat.h"

/* 此处包含的当前版本icf的所有头文件及引擎应用层头文件 */
#include "ICF_error_code_v2.h"
#include "ICF_Interface_v2.h"
#include "ICF_toolkit_v2.h"
#include "ICF_type_v2.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define ICF_MAX_INNER_CHN_NUM		(2)         /* 底层引擎支持的最大通道数，xsie暂定2个，与ICF_INNER_CHN_INFO里的pHandle对应 */
#define ICF_MAX_INNER_CHN_BUF_NUM	(16)        /* 送入引擎的数据有两种处理模式: 1.拷贝模式，2.非拷贝模式。对于非拷贝模式送入的结构体需要是全局的，引擎内部仅做送入内部流转的操作，后续结果回调时会将结构体返回 */

/* 运行时加载动态库符号，此处仅获取函数地址 */
/* typedef int (*ICF_GET_RUNTIME_LIMIT_F)(const ICF_LIMIT_INPUT *pLimitInput, ICF_LIMIT_OUTPUT *pLimitOutput); */
typedef int (*ICF_INIT_F)(void *pInitParam, unsigned int nInitParamSize, void *pInitOutParam, unsigned int nInitOutParamSize);
typedef int (*ICF_FINIT_F)(void *pFinitInParam, unsigned int nFinitInParamSize);
typedef int (*ICF_LOAD_MODEL_F)(void *pLMInParam, unsigned int nLMInParamSize, void *pLMOutParam, unsigned int nLMOutParamSize);
typedef int (*ICF_UNLOAD_MODEL_F)(void *pULMInParam, unsigned int nULMInParamSize);
typedef int (*ICF_CREATE_F)(void *pCreateParam, unsigned int nCreateParamSize, void *pCreateHandle, unsigned int nHandleSize);
typedef int (*ICF_DESTROY_F)(void *pCreateHandle, unsigned int nHandleSize);
typedef int (*ICF_INPUT_DATA_F)(void *pChannelHandle, void *pInputData, unsigned int nDataSize);
typedef int (*ICF_SET_CONFIG_F)(void *pChannelHandle, void *pConfigParam, unsigned int nConfigParamSize);
typedef int (*ICF_GET_CONFIG_F)(void *pChannelHandle, void *pConfigParam, unsigned int nConfigParamSize);
typedef int (*ICF_SET_CALLBACK_F)(void *pChannelHandle, void *pCBParam, unsigned int nCBParamSize);
typedef int (*ICF_GET_VERSION_F)(void *pEngineVersionParam, int nSize);
typedef int (*ICF_APP_GET_VERSION_F)(void *pVersionParam, int nSize);
typedef int (*ICF_GET_PACKAGE_STATE)(void *pPackageHandle);
typedef void * (*ICF_GET_PACKAGE_DATA_PTR)(void *pPackageHandle, int nDataType);

typedef struct _ICF_LIB_FUNC_P_
{
    ICF_INIT_F IcfInit;
    ICF_FINIT_F IcfFinit;
    ICF_LOAD_MODEL_F IcfLoadModel;
    ICF_UNLOAD_MODEL_F IcfUnloadModel;
    ICF_CREATE_F IcfCreate;
    ICF_DESTROY_F IcfDestroy;
    ICF_SET_CONFIG_F IcfSetConfig;
    ICF_GET_CONFIG_F IcfGetConfig;
    ICF_SET_CALLBACK_F IcfSetCallback;
    ICF_GET_VERSION_F IcfGetVersion;
    ICF_APP_GET_VERSION_F IcfAppGetVersion;               /* 获取app版本号接口来自libalg.so，其他均是libicf.so中获取 */
    ICF_INPUT_DATA_F IcfInputData;
    ICF_GET_PACKAGE_STATE IcfGetPackageStatus;
    ICF_GET_PACKAGE_DATA_PTR IcfGetPackageDataPtr;
} ICF_LIB_FUNC_P;

typedef struct _ICF_LIB_SYM_INFO_
{
    VOID *pIcfLibHandle;              /* dlopen icf 句柄 */
    VOID *pXsieLibHandle;             /* dlopen xsie 句柄 */

    ICF_LIB_FUNC_P stIcfLibApi;       /* icf动态库支持解析的函数符号，通过dlopen打开 */
} ICF_LIB_SYM_INFO;

typedef struct _ICF_INNER_CHN_BUF_
{
    BOOL bBusy;                                /* 是否空闲 */
    ICF_INPUT_DATA stIcfInputDataBuf;          /* 送入引擎的数据，与底层icf框架版本一致 */
} ICF_INNER_CHN_BUF;

typedef struct _ICF_INNER_CHN_INFO_
{
    ICF_CREATE_HANDLE stCreateHandle; /* 通道句柄 */
    UINT32 u32HdlIdx;                 /* 用于区分结果回调通道 */

    /* fixme: 此处仅针对xsie进行处理，增加blobNum的外部业务属性，不具备通用性 */
    UINT32 u32BlobNum;

    UINT32 uiR_idx;                   /* 取索引，避免O(n)时间 */
    ICF_INNER_CHN_BUF astIcfInputDataBuf[ICF_MAX_INNER_CHN_BUF_NUM];

    /* todo: 考虑到icf可能兼容不同智能模块，需要将外部智能业务用到的数据结构进行映射管理，可以使用hash table */

} ICF_INNER_CHN_INFO;

typedef struct _ICF_INNER_INFO_
{
    ICF_INIT_HANDLE stIcfHandle;       /* icf全局句柄 */

    ICF_MODEL_PARAM stModelParam[2];      /* 模型参数 */
    ICF_MODEL_HANDLE stModelHandle[2];    /* 模型句柄信息 */

    ICF_APP_PARAM_INFO stAppParam[2];

	/* 智能模块注册进来的内存申请/释放接口，函数指针 */
	ENGINE_COMM_MEM_ALLOC_CB_FUNC pMemAllocFunc;
	ENGINE_COMM_MEM_RLS_CB_FUNC pMemFreeFunc;

    UINT32 u32ChnNum;                 /* 开启的引擎处理通道个数 */
    ICF_INNER_CHN_INFO astInnerChnInfo[ICF_MAX_INNER_CHN_NUM];
} ICF_INNER_INFO;

#endif

