/*******************************************************************************
* ba_hal.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年9月5日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _BA_HAL_H_
#define _BA_HAL_H_

/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "system_common_api.h"
#include "ia_common.h"

/* 此处包含的当前版本icf的所有头文件及引擎应用层头文件 */
#include "ICF_Interface_v2.h"
#include "ICF_error_code_v2.h"
#include "ICF_toolkit_v2.h"
#include "ICF_type_v2.h"

/* 行为分析相关头文件 */

#include "alg_error_code.h"
#include "alg_sba_type.h"



/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */
#define BA_SUB_MOD_MAX_NUM	(4)             /* 行为分析子模块最大个数，当前只用到一个子模块 */
#define BA_VERSION_INFO_LEN (256)           /* 版本信息字符串最大长度 */

#define BA_DEFAULT_SENSITY	(1)            /* 默认灵敏度，当前仅对在离岗生效。将灵敏度和持续时间进行映射处理，引擎不支持直接配置灵敏度 */
#define BA_MAX_SENSITY		(5)
#define BA_MIN_SENSITY		(BA_DEFAULT_SENSITY)

#define XBA_DEV_MAX			(2)
#define BA_ENG_DEV_MAX		(XSI_DEV_MAX)       /* 引擎通道暂时和DSP逻辑通道数保持一致，实际引擎能力不局限于2路 */


#define BA_HAL_CHECK_CHAN(chan, value)		{if (chan > (XBA_DEV_MAX - 1)) {SVA_LOGE("Chan (Illegal parameters)\n"); return (value); }}
#define BA_HAL_CHECK_PRM(ptr, value)		{if (!ptr) {SVA_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}
#define BA_HAL_CHECK_RETURN(ptr, value)	{if (ptr) {SVA_LOGE("ptr 0x%x\n", ptr); return (value); }}


/* ========================================================================== */
/*                          数据结构区								                                                                    */
/* ========================================================================== */
typedef void (*BA_CALL_BACK_FUNC)(INT32 nNodeID,
                                  INT32 nCallBackType,
                                  VOID *pstOutput,
                                  UINT32 nSize,
                                  VOID *pUsr,
                                  INT32 nUserSize);

typedef enum tagBaCfgType
{
    SENSITY_TYPE = 0,
    BA_CONFIG_TYPE_NUM,
} BA_CONFIG_TYPE_E;

typedef struct _BA_RECT_I_
{
    int   x;         //矩形左上角X轴坐标
    int   y;         //矩形左上角Y轴坐标
    int   width;     //矩形宽度
    int   height;    //矩形高度
} BA_RECT_I;

typedef struct tagBaObjInfo
{
    UINT32 uiObjectNum;
    BA_RECT_I Rect[64];     /* 暂定单张图中最多有64个目标 */
} BA_OBJ_DET_RESULT;

typedef struct tagXbaSubModPrm
{
    UINT32 uiUseFlag;             /* 子模块是否启用 */
    ICF_CREATE_HANDLE stHandle;                /* 子模块句柄 */
	ICF_MODEL_HANDLE stModelHandle;           /* 子模块模型句柄 */
    CHAR acVerInfo[BA_VERSION_INFO_LEN];     /* 子模块版本信息长度 */
    UINT32 uiReserved[4];         /* 预留字段 */
} BA_SUB_MOD_S;


/* 运行时加载动态库符号，此处仅获取函数地址 */
//typedef int (*BA_ICF_GET_RUNTIME_LIMIT_F)(const ICF_LIMIT_INPUT *pLimitInput, ICF_LIMIT_OUTPUT *pLimitOutput);
typedef int (*BA_ICF_INIT_F)(void                  *pInitParam,
                             unsigned int           nInitParamSize,
                             void                  *pInitOutParam,
                             unsigned int           nInitOutParamSize);

typedef int (*BA_ICF_FINIT_F)(void                *pFinitInParam,
                              unsigned int         nFinitInParamSize);


typedef int (*BA_ICF_LOAD_MODEL_F)(void          *pLMInParam,
                                   unsigned int   nLMInParamSize,
                                   void          *pLMOutParam,
                                   unsigned int   nLMOutParamSize);

typedef int (*BA_ICF_UNLOAD_MODEL_F)(void        *pULMInParam,
                                     unsigned int nULMInParamSize);

typedef int (*BA_ICF_CREATE_F)(void           *pCreateParam,
                               unsigned int    nCreateParamSize,
                               void           *pCreateHandle,
                               unsigned int    nHandleSize);

typedef int (*BA_ICF_DESTROY_F)(void         *pCreateHandle,
                                unsigned int  nHandleSize);

typedef int (*BA_ICF_INPUT_DATA_F)(void            *pChannelHandle,
                                   void            *pInputData,
                                   unsigned int     nDataSize);

typedef int (*BA_ICF_SUB_FUNC_F)(void            *pChannelHandle,
                                 void            *pInputData,
                                 unsigned int     nInputDataSize,
                                 void           **pOutputData,
                                 unsigned int    *pOutputDataSize);

typedef int (*BA_ICF_SET_CONFIG_F)(void          *pChannelHandle,
                                   void          *pConfigParam,
                                   unsigned int   nConfigParamSize);

typedef int (*BA_ICF_GET_CONFIG_F)(void          *pChannelHandle,
                                   void          *pConfigParam,
                                   unsigned int   nConfigParamSize);

typedef int (*BA_ICF_SET_CALLBACK_F)(void         *pChannelHandle,
                                     void         *pCBParam,
                                     unsigned int  nCBParamSize);

typedef int (*BA_ICF_GET_VERSION_F)(void *pEngineVersionParam,
                                    int   nSize);
 
//typedef int (*BA_ICF_APP_GET_VERSION_F)(void *pVersionParam,
//                                       int   nSize);


//typedef int (*BA_ICF_SET_CORE_AFFINITY)(void *pInitHandle, void *pCoreAffinityInfo);
//typedef int (*BA_ICF_GET_MEMPOOL_STATUS)(void *pInitHandle);

//typedef int (*BA_ICF_MEMPOOL_ALLOC)(const char *file, const char *func, int line, void *pInitHandle, ICF_MEM_TAB *pMemTab);
//typedef int (*BA_ICF_MEMPOOL_FREE)(const char *file, const char *func, int line, void *pInitHandle, void *pMemAddr, ICF_MEM_TYPE_E nSpace);

typedef int (*BA_ICF_GET_PACKAGE_STATE)(void *pPackageHandle);
typedef void * (*BA_ICF_GET_PACKAGE_DATA_PTR)(void *pPackageHandle, int nDataType);



typedef struct _BA_ICF_FUNC_P_
{
    //BA_ICF_GET_RUNTIME_LIMIT_F Ba_IcfGetRunTimeLimit;
    BA_ICF_INIT_F Ba_IcfInit;
    BA_ICF_FINIT_F Ba_IcfFinit;
    BA_ICF_LOAD_MODEL_F Ba_IcfLoadModel;
    BA_ICF_UNLOAD_MODEL_F Ba_IcfUnloadModel;
    BA_ICF_CREATE_F Ba_IcfCreate;
    BA_ICF_DESTROY_F Ba_IcfDestroy;
	BA_ICF_INPUT_DATA_F Ba_IcfInputData;
	BA_ICF_SUB_FUNC_F Ba_IcfSubFunc;
    BA_ICF_SET_CONFIG_F Ba_IcfSetConfig;
    BA_ICF_GET_CONFIG_F Ba_IcfGetConfig;
    BA_ICF_SET_CALLBACK_F Ba_IcfSetCallback;
    BA_ICF_GET_VERSION_F Ba_IcfGetVersion;
    //BA_ICF_APP_GET_VERSION_F Ba_IcfAppGetVersion;               /* 获取app版本号接口来自libalg.so，其他均是libicf.so中获取 */
    
    
    BA_ICF_GET_PACKAGE_STATE Ba_IcfGetPackageStatus;
    BA_ICF_GET_PACKAGE_DATA_PTR Ba_IcfGetPackageDataPtr;

    //BA_ICF_SET_CORE_AFFINITY Ba_IcfSetCoreAffinity;
    //BA_ICF_GET_MEMPOOL_STATUS Ba_IcfGetMemPoolStatus;

    //BA_ICF_MEMPOOL_ALLOC Ba_IcfMemPoolAlloc;
    //BA_ICF_MEMPOOL_FREE Ba_IcfMemPoolFree;
} BA_ICF_FUNC_P;

typedef struct tagBaModPrm
{
    UINT32 uiInitFlag;         /* 模块初始化状态 */
    ICF_INIT_HANDLE stModHandle;          /* 模块句柄 */
    VOID *pEncryptHandle;      /* 加解密句柄 */
    UINT32 uiSubModCnt;        /* 子模块个数 */
    BA_SUB_MOD_S *pstBaSubMod[BA_SUB_MOD_MAX_NUM];      /* 子模块参数 */
    VOID *pIcfLibHandle;                               /* libicf.so的加载句柄 */
    VOID *pIcfAlgHandle;                               /* libalg.so的加载句柄 */
    BA_ICF_FUNC_P stIcfFuncP;                         /* ICF框架动态运行库函数，通过dlopen打开 */
} BA_MOD_S;

/* POS信息输出结构 */
#if 0
typedef struct _XBA_DEV_T
{
    UINT32 xsi_status;                                         /* 智能检测通道状态 */
    UINT32 uiFrameNum;                                         /* 帧序号 */
    UINT32 uiSyncId;                                           /* 双视角同步索引序号 */
    UINT32 reaultCnt;                                          /* 智能结果更新计数 */
    SVA_PROCESS_OUT stSvaOut;                                  /* 智能模块对外结果信息 */
    SVA_OUT_POOL stSvaOutPool;                                 /* 智能结果缓存池，用于显示帧匹配 */
    SVA_PROCESS_OUT stHalTmpOut;                               /* 局部申请结构体太大，coverity会报错，所以搞一个全局的，6 不 6，耶 不 耶 */
    XSI_DEV_BUFF stBuff[SVA_INPUT_BUF_LEN];                    /* 帧缓存数据 */
    SYSTEM_FRAME_INFO stSysFrame;                              /* 帧数据，用于送入引擎的数据进行TDE拷贝 */

    void *mOutMutexHdl;                                        /* 智能结果同步信号量*/
    void *mChnMutexHdl;                                        /* 通道信号量*/
} XBA_DEV;
#endif

#if 0
typedef struct _BA_SUB_MOD_PRM_
{
    UINT32 uiUseFlag;                  /* 子模块是否启用 */
    VOID *pHandle;                     /* 子模块句柄 */
    VOID *pCalibHandle;                /* 标定句柄 */
    VOID *pFaceCalibHandle;            /* 人脸相机标定句柄  */
    UINT32 uiFrameIdx;                 /* 帧索引 */

    PPM_ICF_CALIB_PRM_S stIcfCalibPrm;
    PPM_ICF_FACE_CALIB_PRM_S stIcfFaceCalibPrm;
    PPM_INPUT_DATA_PRM_S stIptDataPrm; /* 外部维护引擎输入参数队列 */
    UINT32 uiReserved[4];              /* 预留字段 */
} BA_SUB_MOD_S;


typedef struct _XBA_COMMON_T
{
#if 0
    UINT8 version[256];                                /* 版本信息 */
    void *alg_encrypt_hdl;                             /* 解密句柄 */
    UINT32 uiChannelNum;                               /* 初始化通道数 */
    ENG_CHANNEL_PRM_S stEngChnPrm[BA_ENG_DEV_MAX];        /* 引擎通道参数 */
    XBA_DEV xsi_dev[XBA_DEV_MAX];                      /* 设备通道相关参数 */

    IA_PRODUCT_TYPE_E enProductType;                   /* 产品类型，szl_todo: 后续删除产品类型依赖 */
#endif
    VOID *pIcfLibHandle;                               /* libicf.so的加载句柄 */
//    VOID *pIcfAlgHandle;                               /* libalg.so的加载句柄 */
    BA_ICF_FUNC_P stIcfFuncP;                         /* ICF框架动态运行库函数，通过dlopen打开 */ 
}XBA_COMMON;

#endif


/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */

/**
 * @function:   ba_HalGetInitFlag
 * @brief:      获取模式初始化状态标志
 * @param[in]:  VOID
 * @param[out]: 无
 * @return:     UINT32
*/
UINT32 Ba_HalGetInitFlag(void);

/**
 * @function:   Ba_HalGetIcfCfg
 * @brief:      获取当前配置
 * @param[in]:  UINT32 uiIdx
 * @param[in]:  SBAE_SENSCTRL_PARAMS_T *pstSensityPrm
 * @param[in]:  SBAE_APP_ALERT_CONFIG_T *pstAppAlertPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalGetIcfCfg(UINT32 uiIdx, SBAE_DBA_PROC_PARAMS_T *pstSensityPrm, SBAE_DBA_PROC_PARAMS_T *pstAppAlertPrm);

/**
 * @function:   Ba_HalSetIcfCfg
 * @brief:      设置配置参数
 * @param[in]:  UINT32 uiIdx
 * @param[in]:  BA_CONFIG_TYPE_E enCfgType
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalSetIcfCfg(UINT32 uiIdx, BA_CONFIG_TYPE_E enCfgType, void *prm);


/**
 * @function:   Ba_HalSetCbFunc
 * @brief:      注册回调函数
 * @param[in]:  UINT32 uiIdx
 * @param[in]:  BA_CALL_BACK_FUNC pFunc
 * @param[out]: None
 * @return:     VOID *
 */
VOID *Ba_HalSetCbFunc(UINT32 uiIdx, BA_CALL_BACK_FUNC pFunc);

/**
 * @function:   Ba_HalEngineDeInit
 * @brief:      引擎资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalEngineDeInit(VOID);

/**
 * @function:   Ba_HalEngineInit
 * @brief:      引擎资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalEngineInit(VOID);

/**
 * @function:   Ba_HalGetSubModPrm
 * @brief:      获取子模块参数信息
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     BA_SUB_MOD_S *
 */
BA_SUB_MOD_S *Ba_HalGetSubModPrm(UINT32 uiIdx);

/**
 * @function:   Ba_HalGetModPrm
 * @brief:      获取模块参数信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     BA_MOD_S *
 */
BA_MOD_S *Ba_HalGetModPrm(VOID);

/**
 * @function:   Ba_HalPrint
 * @brief:      调试信息打印（用于跟踪流程是否走通）
 * @param[in]:  CHAR *pcInfo
 * @param[out]: None
 * @return:     VOID *
 */
VOID *Ba_HalPrint(CHAR *pcInfo);

/**
 * @function:   Ba_HalFree
 * @brief:      释放一段malloc内存
 * @param[in]:  VOID *p
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ba_HalFree(VOID *p);

/**
 * @function:   Ba_HalMalloc
 * @brief:      申请一段malloc内存
 * @param[in]:  VOID **pp
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ba_HalMalloc(VOID **pp, UINT32 uiSize);

/**
 * @function:   Ba_HalDebugDumpData
 * @brief:      Ba模块dump数据接口
 * @param[in]:  CHAR *pData
 * @param[in]:  CHAR *pPath
 * @param[in]:  UINT32 uiSize
 * @param[in]:  UINT32 uiStopFlg
 * @param[out]: None
 * @return:     void
 */
void Ba_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiStopFlg);

/**
 * @function:   Ba_HalSetDbLevel
 * @brief:      设置行为分析日志调试等级
 * @param[in]:  INT32 level
 * @param[out]: None
 * @return:     void
 */
void Ba_HalSetDbLevel(INT32 level);

/**
 * @function:   Ba_HalGetDbLevel
 * @brief:      获取行为分析调试日志等级
 * @param[in]:  void
 * @param[out]: None
 * @return:     UINT32
 */
UINT32 Ba_HalGetDbLevel(void);

/*******************************************************************************
* 函数名  : Ba_HalGetDev
* 描  述  : 获取算法结果全局变量
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
BA_MOD_S *Ba_HalGetXbaCommon(void);

/**
 * @function:   Ba_HalInitRtld
 * @brief:      ba模块加载动态库符号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalInitRtld(VOID);

/**
 * @function:   Ba_HalInit
 * @brief:      HAL层资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalInit(VOID);

/**
 * @function:   Ba_HalDeinit
 * @brief:      HAL层资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalDeinit(VOID);



#endif

