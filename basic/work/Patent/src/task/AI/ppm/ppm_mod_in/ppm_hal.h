/**
 * @file:   ppm_hal.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联hal层头文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */
#ifndef _PPM_HAL_H_
#define _PPM_HAL_H_

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"
#include "dspcommon.h"

#include "system_common_api.h"

#include "ia_common.h"
#include "ppm_out.h"

#include "vca_base.h"

#include "alg_mtme_type.h"
#include "icf_version_compat.h"

#include "ICF_base_v2.h"
#include "ICF_error_code_v2.h"
#include "ICF_Interface_v2.h"
#include "ICF_toolkit_v2.h"
#include "ICF_type_v2.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
//#define PPM_SUB_MOD_MAX_NUM (1)               /* 人包关联模块最大子模块个数 */

#define DEMO_FRAME_NUM	(100)

/* RK版本输入引擎的图像数据仅有两种，人脸1080P，三目可见光1080P */
#define DMEO_SRC_DIM	(2)//(5)                /* 输入数据维数 */

#define INPUT_BUF_NUM	(1)
#define SLEEPTIME_US	(500000)

#define DEMO_XSI_WIDTH			(1280)
#define DEMO_XSI_HEIGHT			(720)
#define DEMO_SRC_WIDTH			(1280)
#define DEMO_SRC_HEIGHT			(720)
#define DEMO_FACE_WIDTH			(1920)   /* 人脸NV21尺寸 */
#define DEMO_FACE_HEIGHT		(1080)
#define DEMO_LEFT_IMG_WIDTH		(768)
#define DEMO_LEFT_IMG_HEIGHT	(432)
#define DEMO_RIGHT_IMG_WIDTH	(768)
#define DEMO_RIGHT_IMG_HEIGHT	(432)
#define DEMO_TRI_VISION_WIDTH	(1920)   /* 三目可见光NV21尺寸，RK版本新增 */
#define DEMO_TRI_VISION_HEIGHT	(1080)

#define PPM_MAX_INPUT_DATA_NUM	(128)
#define PPM_MAX_ICF_VERSION_LEN (256)

#define PPM_IPT_PTS_LEN (2)

#ifdef MMZ_ZONE
#define ST_ZONE ("ppm st")
#else
#define ST_ZONE (NULL)
#endif

typedef enum _PPM_SUB_MOD_TYPE_
{
	/* 关联通道 */
	PPM_SUB_MOD_MATCH = 0,
	/* 跨相机标定 */
	PPM_SUB_MOD_MCC_CALIB = 1,
	/* 人脸单目相机内参标定 */
	PPM_SUB_MOD_FACE_CALIB = 2,

	/* 人包模块当前支持的最大引擎处理通道数 */
	PPM_SUB_MOD_MAX_NUM,
} PPM_SUB_MOD_TYPE_E;

typedef enum PpmcfgJsonFile
{
    PPM_ALG_CFG_PATH = 0,                   /* AlgCfgPath */
    PPM_TASK_CFG_PATH = 1,                  /* TaskCfgPath */
    PPM_TOOLKIT_CFG_PATH = 2,               /* ToolkitCfgPath */
    PPM_PARAM_CFG_PATH = 3,              /* AppParamCfgPath */
} PPM_CFG_JSONFILE_E;

typedef enum _PPM_JSON_CLASS_
{
    PPM_JSON_CLASS_SECURITY_DETECT = 0,
    PPM_JSON_CLASS_NUM,
} PPM_JSON_CLASS_E;

typedef enum _PPM_JSON_CHG_CMD_
{
    CHANGE_RESOLUTION = 0,
    CHANGE_DPT_PARAM,
    CHANGE_CMD_NUM,
} PPM_JSON_CHANGE_CMD_E;

typedef enum _PPM_JSON_SUB_CLASS_
{
    PPM_JSON_SUB_CLASS_HEIGHT,
    PPM_JSON_SUB_CLASS_INCLINE,
    PPM_JSON_SUB_CLASS_PITCH,
    PPM_JSON_SUB_CLASS_NUM,
} PPM_JSON_SUB_CLASS_E;
typedef enum _PPM_JSON_CLASS_TYPE_
{
    PPM_JSON_FIRST_CLASS = 0,
    PPM_JSON_SECOND_CLASS,
    PPM_JSON_CLASS_TYPE_NUM,
} PPM_JSON_CLASS_TYPE_E;

typedef struct _PPM_JSON_MOD_PRM_
{
    PPM_JSON_CLASS_TYPE_E enClassType;
    UINT32 uiClassId;

    union
    {
        UINT32 uiVal;
        CHAR *pcVal;
    };
} PPM_JSON_MOD_PRM_S;

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _PPM_USER_PRVT_INFO_
{
    UINT32 uiIptTime;                          /* 引擎推帧时间戳，用于计算耗时统计 */
    SYSTEM_FRAME_INFO stSysFrmInfo;            /* 第二路X光包裹数据 */
    UINT64 uiIptPts[PPM_IPT_PTS_LEN];          /* 输入通道时间戳暂时缓存 */

    UINT32 uiReserved[8];                      /* 预留 */
} PPM_USER_PRVT_INFO_S;

typedef struct _PPM_IPT_DATA_INFO_
{
    BOOL bUsed;
    UINT32 uiIdx;

    ICF_INPUT_DATA stIptDataInfo;      /* 引擎接口入参 */
    PPM_USER_PRVT_INFO_S stUsrPrvtInfo;                       /* 用户私有信息用于传入引擎框架并传回 */
} PPM_INPUT_DATA_INFO;

typedef struct _PPM_IPT_DATA_PRM_
{
    UINT32 uiIdx;

    UINT32 uiCnt;
	MTME_INPUT_T stMtmeInputData[PPM_MAX_INPUT_DATA_NUM];
    PPM_INPUT_DATA_INFO stInputDataInfo[PPM_MAX_INPUT_DATA_NUM];      /* 引擎接口入参 */
} PPM_INPUT_DATA_PRM_S;

typedef struct _PPM_INPUT_CALIB_PRM_
{
    UINT32 uiCalibUseFlag;             /* 标定参数，使用标志 */
    UINT32 uiSubChn;                   /* 对应graph的通道号 */
} PPM_INPUT_CALIB_PRM_S;

typedef struct _PPM_OUTPUT_CALIB_PRM_
{
    INT32 s32CalibSts;

    double fRmsVal;
    PPM_CALIB_OUTPUT_PRM_S stMatrixInfo;
} PPM_OUTPUT_CALIB_PRM_S;

typedef struct _PPM_ICF_CALIB_PRM_
{
    sem_t sem;                            /* 同步信号量 */
    PPM_INPUT_CALIB_PRM_S stCalibIptPrm;  /* 标定输入 */
    PPM_OUTPUT_CALIB_PRM_S stCalibOptPrm; /* 标定输出 */
} PPM_ICF_CALIB_PRM_S;

typedef struct _PPM_ICF_FACE_CALIB_PRM_
{
    sem_t sem;                            /* 同步信号量 */
    PPM_INPUT_CALIB_PRM_S stCalibIptPrm;  /* 标定输入 */
    PPM_OUTPUT_CALIB_PRM_S stCalibOptPrm; /* 标定输出 */
} PPM_ICF_FACE_CALIB_PRM_S;

typedef struct _PPM_SUB_MOD_FACE_CALIB_PRM_
{
	/* 引擎通道句柄，用于人脸单目相机标定 */
	ICF_CREATE_HANDLE stCreateHandle;
	/* 模型参数 */
	ICF_MODEL_PARAM	stModelParam;
	/* 模型句柄 */
	ICF_MODEL_HANDLE stModelHandle;
	/* 标定输入引擎的中间变量 */
	ICF_INPUT_DATA stCalibEngInputData;

	/* 人脸单目相机标定参数 */
    PPM_ICF_CALIB_PRM_S stIcfFaceCalibPrm;
} PPM_SUB_MOD_FACE_CALIB_PRM_S;

typedef struct _PPM_SUB_MOD_MCC_CALIB_PRM_
{
	/* 引擎通道句柄，用于跨相机标定 */
	ICF_CREATE_HANDLE stCreateHandle;
	/* 模型参数 */
	ICF_MODEL_PARAM	stModelParam;
	/* 模型句柄 */
	ICF_MODEL_HANDLE stModelHandle;
	/* 标定输入引擎的中间变量 */
	ICF_INPUT_DATA stCalibEngInputData;
	
	/* 跨相机标定参数 */
    PPM_ICF_CALIB_PRM_S stIcfCalibPrm;
} PPM_SUB_MOD_MCC_CALIB_PRM_S;

typedef struct _PPM_SUB_MOD_MATCH_PRM_
{
	/* 引擎通道句柄，用于关联匹配 */
	ICF_CREATE_HANDLE stCreateHandle;
	/* 模型参数 */
	ICF_MODEL_PARAM	stModelParam;
	/* 模型句柄 */
	ICF_MODEL_HANDLE stModelHandle;
} PPM_SUB_MOD_MATCH_PRM_S;

typedef union _PPM_SUB_MOD_CHN_PRM_
{
	/* 关联匹配通道参数 */
	PPM_SUB_MOD_MATCH_PRM_S stMatchChnPrm;
	/* 跨相机标定通道参数 */
	PPM_SUB_MOD_MCC_CALIB_PRM_S stMccCalibChnPrm;
	/* 人脸单目相机标定通道参数 */
	PPM_SUB_MOD_FACE_CALIB_PRM_S stFaceCalibChnPrm;
} PPM_SUB_MOD_CHN_PRM_U;

typedef struct _PPM_SUB_MOD_
{
	/* 子模块是否启用，szl_ppm_todo: 命名规范调整 */
    UINT32 uiUseFlag;                  
	/* 帧索引 */
    UINT32 uiFrameIdx;                 
	/* 外部维护引擎输入参数队列 */
    PPM_INPUT_DATA_PRM_S stIptDataPrm; 
	/* 引擎通道全局参数 */
	PPM_SUB_MOD_CHN_PRM_U uSubModChnPrm;

    UINT32 uiReserved[4];              /* 预留字段 */
} PPM_SUB_MOD_S;


/*传入修改Json结构体*/
typedef struct tagPpmCfgParaJson
{
    PPM_JSON_CHANGE_CMD_E enChgType;        /*修改类型*/
    double fCamHeight;                               /* 双目相机架设高度 */
    double fInclineAngle;                            /* 倾斜角度 */
    double fPitchAngle;                              /* 俯仰角度 */
    CHAR *pcAiModelPath[AI_MODEL_NUM];       /*开放平台AI模型路径*/
    CHAR *pcModelPath[ASM_MODEL_NUM];         /*安检机模型路径*/
} PPM_UPDATA_CFG_JSON_PARA;

/* 运行时加载动态库符号，此处仅获取函数地址 */
typedef int (*PPM_ICF_INIT_F)(void *pInitParam, unsigned int nInitParamSize, void *pInitOutParam, unsigned int nInitOutParamSize);
typedef int (*PPM_ICF_FINIT_F)(void *pFinitInParam, unsigned int nFinitInParamSize);
typedef int (*PPM_ICF_LOAD_MODEL_F)(void *pLMInParam, unsigned int nLMInParamSize, void *pLMOutParam, unsigned int nLMOutParamSize);
typedef int (*PPM_ICF_UNLOAD_MODEL_F)(void *pULMInParam, unsigned int nULMInParamSize);
typedef int (*PPM_ICF_CREATE_F)(void *pCreateParam, unsigned int nCreateParamSize, void *pCreateHandle, unsigned int nHandleSize);
typedef int (*PPM_ICF_DESTROY_F)(void *pCreateHandle, unsigned int nHandleSize);
typedef int (*PPM_ICF_INPUT_DATA_F)(void *pChannelHandle, void *pInputData, unsigned int nDataSize);
typedef int (*PPM_ICF_SET_CONFIG_F)(void *pChannelHandle, void *pConfigParam, unsigned int nConfigParamSize);
typedef int (*PPM_ICF_GET_CONFIG_F)(void *pChannelHandle, void *pConfigParam, unsigned int nConfigParamSize);
typedef int (*PPM_ICF_SET_CALLBACK_F)(void *pChannelHandle, void *pCBParam, unsigned int nCBParamSize);
typedef int (*PPM_ICF_GET_VERSION_F)(void *pEngineVersionParam, int nSize);
typedef int (*PPM_ICF_APP_GET_VERSION_F)(void *pVersionParam, int nSize);
typedef int (*PPM_ICF_GET_PACKAGE_STATE)(void *pPackageHandle);
typedef void * (*PPM_ICF_GET_PACKAGE_DATA_PTR)(void *pPackageHandle, int nDataType);

#if 0
typedef int (*PPM_ICF_GET_RUNTIME_LIMIT_F)(const ICF_LIMIT_INPUT *pLimitInput, ICF_LIMIT_OUTPUT *pLimitOutput);
typedef int (*PPM_ICF_SUB_FUNC_F)(void *pCreateHandle, int nNodeID, void *pInputData, unsigned int nDataSize);

typedef int (*PPM_ICF_SET_CORE_AFFINITY)(void *pInitHandle, void *pCoreAffinityInfo);
typedef int (*PPM_ICF_GET_MEMPOOL_STATUS)(void *pInitHandle);
#endif

typedef struct _PPM_ICF_FUNC_P_
{
    PPM_ICF_INIT_F Ppm_IcfInit;
    PPM_ICF_FINIT_F Ppm_IcfFinit;
    PPM_ICF_CREATE_F Ppm_IcfCreate;
    PPM_ICF_DESTROY_F Ppm_IcfDestroy;
    PPM_ICF_SET_CONFIG_F Ppm_IcfSetConfig;
    PPM_ICF_GET_CONFIG_F Ppm_IcfGetConfig;
    PPM_ICF_SET_CALLBACK_F Ppm_IcfSetCallback;
    PPM_ICF_GET_VERSION_F Ppm_IcfGetVersion;
    PPM_ICF_APP_GET_VERSION_F Ppm_IcfAppGetVersion;
    PPM_ICF_INPUT_DATA_F Ppm_IcfInputData;
	PPM_ICF_LOAD_MODEL_F Ppm_IcfLoadModel;
	PPM_ICF_UNLOAD_MODEL_F Ppm_IcfUnloadModel;
    PPM_ICF_GET_PACKAGE_STATE Ppm_IcfGetPackageState;
    PPM_ICF_GET_PACKAGE_DATA_PTR Ppm_IcfGetPackageDataPtr;

#if 0
    PPM_ICF_GET_RUNTIME_LIMIT_F Ppm_IcfGetRunTimeLimit;
    PPM_ICF_SUB_FUNC_F Ppm_IcfSubFunc;
    PPM_ICF_SET_CORE_AFFINITY Ppm_IcfSetCoreAffinity;
    PPM_ICF_GET_MEMPOOL_STATUS Ppm_IcfGetMemPoolStatus;
#endif
} PPM_ICF_FUNC_P;

typedef struct _PPM_MOD_PRM_
{
    UINT32 uiInitFlag;         /* 模块初始化状态 */

#if 1
	ICF_INIT_HANDLE stInitHandle;
#else
    VOID *pModHandle;          /* 模块句柄 */
#endif

    VOID *pEncryptHandle;      /* 加解密句柄 */

	/* 子通道个数 */
    UINT32 uiSubModCnt;        
	/* 子通道参数 */
    PPM_SUB_MOD_S stPpmSubMod[PPM_SUB_MOD_MAX_NUM];
	
    CHAR strIcfVer[PPM_MAX_ICF_VERSION_LEN];           /* icf版本号 */

    VOID *pIcfLibHandle;                               /* libIcfPpm_V1.2.5.so的加载句柄 */
    VOID *pAlgHandle;                                  /* libMtmeAlg.so的加载句柄 */
    PPM_ICF_FUNC_P stIcfFuncP;                         /* ICF框架动态运行库函数，通过dlopen打开 */
} PPM_MOD_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/
typedef VOID (*PPM_CALL_FUNC)(INT32 nNodeID, INT32 nCallBackType, VOID *pstOutput, UINT32 nSize, VOID *pUsr, INT32 nUserSize);

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Ppm_DrvSetPackSensity
 * @brief:      设置可见光包裹的检测灵敏度
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     PPM_MOD_S *
 */
VOID Ppm_DrvSetPackSensity(UINT32 sensity);

/**
 * @function:   Ppm_HalGetInitFlag
 * @brief:      获取模块资源初始化状态
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     PPM_MOD_S *
 */
UINT32 Ppm_HalGetInitFlag(void);

/**
 * @function:   Ppm_HalSetProcFlag
 * @brief:      设置模块资源初始化状态
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     PPM_MOD_S *
 */
void Ppm_HalSetProcFlag(UINT32 uiFlag);

/**
 * @function:   Ppm_HalGetModPrm
 * @brief:      获取模块全局参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     PPM_MOD_S *
 */
PPM_MOD_S *Ppm_HalGetModPrm(VOID);

/**
 * @function:   Ppm_HalGetSubModPrm
 * @brief:      获取子模块全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     PPM_SUB_MOD_S *
 */
PPM_SUB_MOD_S *Ppm_HalGetSubModPrm(UINT32 chan);

/**
 * @function   Ppm_HalCheckSubModSts
 * @brief      查询子模块通道是否已初始化
 * @param[in]  PPM_SUB_MOD_TYPE_E enChnIdx  
 * @param[out] None
 * @return     BOOL
 */
BOOL Ppm_HalCheckSubModSts(PPM_SUB_MOD_TYPE_E enChnIdx);

/**
 * @function   Ppm_HalGetSubModChnPrm
 * @brief      获取子模块通道全局参数
 * @param[in]  PPM_SUB_MOD_TYPE_E enChnIdx  
 * @param[out] None
 * @return     VOID *
 */
VOID *Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_TYPE_E enChnIdx);

/**
 * @function   Ppm_HalSetMatchPackSensity
 * @brief      配置关联通道可见光包裹抓拍灵敏度
 * @param[in]  MTME_PACK_MATCH_SENSITIVITY_E enSensity  
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchPackSensity(MTME_PACK_MATCH_SENSITIVITY_E enSensity);

/**
 * @function   Ppm_HalSetMatchPackRoi
 * @brief      配置关联通道可见光包裹抓拍roi
 * @param[in]  MTME_RECT_T *pstPackRoiInfo 
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchPackRoi(MTME_RECT_T *pstPackRoiInfo);

/**
 * @function   Ppm_HalSetMatchFaceIouThresh
 * @brief      配置关联通道人脸IOU阈值
 * @param[in]  float fMatchFaceIou
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchFaceIouThresh(float fMatchFaceIou);

/**
 * @function   Ppm_HalSetMatchFaceRoi
 * @brief      配置关联通道人脸抓拍roi
 * @param[in]  MTME_RECT_T *pstFaceRoiInfo
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchFaceRoi(MTME_RECT_T *pstFaceRoiInfo);

/**
 * @function   Ppm_HalSetMatchFaceScoreFilter
 * @brief      配置关联通道人脸评分过滤
 * @param[in]  float fFilterScore
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchFaceScoreFilter(float fFilterScore);

/**
 * @function:   Ppm_HalSetCapRegion
 * @brief:      设定抓拍AB区域
 * @param[in]:  MTME_CAPTURE_REGION_T *pstCapRegion
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalSetCapRegion(MTME_CAPTURE_REGION_T *pstCapRegion);

/**
 * @function:   Ppm_HalSetMatchMatrixPrm
 * @brief:      设置关联匹配矩阵参数
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_MATCH_MATRIX_INFO_S *pstMatchMatrixInfo
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalSetMatchMatrixPrm(UINT32 chan, PPM_MATCH_MATRIX_INFO_S *pstMatchMatrixInfo);

/**
 * @function:   Ppm_HalGetFreeIptData
 * @brief:      获取空闲的input节点
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 *puiIdx
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalGetFreeIptData(UINT32 chan, UINT32 *puiIdx);

/**
 * @function:   Ppm_HalInitGlobalVar
 * @brief:      初始化全局变量
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_HalInitGlobalVar(VOID);

/**
 * @function:   Ppm_HalDebugDumpData
 * @brief:      ppm模块dump数据接口
 * @param[in]:  INT8 *pData
 * @param[in]:  INT8 *pPath
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     void
 */
void Ppm_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiFlag);

/**
 * @function:   Ppm_HalMemFree
 * @brief:      PPM模块内存释放
 * @param[in]:  VOID *pVir
 * @param[out]: None
 * @return:     VOID
 */
VOID Ppm_HalMemFree(VOID *pVir);

/**
 * @function:   Ppm_HalMemAlloc
 * @brief:      PPM模块内存申请
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     VOID *
 */
VOID *Ppm_HalMemAlloc(UINT32 uiSize, BOOL bCache, CHAR *pcMemName);

/**
 * @function:   Ppm_HalFree
 * @brief:      释放一段malloc内存
 * @param[in]:  VOID *p
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_HalFree(VOID *p);

/**
 * @function:   Ppm_HalMalloc
 * @brief:      申请一段malloc内存
 * @param[in]:  VOID **pp
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_HalMalloc(VOID **pp, UINT32 uiSize);

/**
 * @function:   Ppm_HalDeinit
 * @brief:      HAL层资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalDeinit(VOID);

/**
 * @function:   Ppm_HalInit
 * @brief:      HAL层资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalInit(VOID);

#endif

