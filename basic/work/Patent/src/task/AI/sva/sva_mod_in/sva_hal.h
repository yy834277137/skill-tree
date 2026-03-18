/*******************************************************************************
* sva_hal.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月14日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef __SVA_HAL_H__
#define __SVA_HAL_H__

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "plat_com_inter_hal.h"

#include "ia_common.h"
#include "icf_common.h"

#include "chip_dual_hal.h"
#include "dup_tsk_api.h"
#include "sva_out.h"
#include "capbility.h"

#include "alg_xsie_type.h"    /* 包含xsie引擎的应用层数据结构 */
#include "alg_error_code.h"

#include "sts.h"      /* 智能模块维护子模块 */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/* 安检机从xpack拷贝的包裹图像数据最大宽，像素，140100机型AiYuv宽度最大设置为1600，与1280等比例，后续全部按照最大申请 */
#define ISM_SVA_COPY_WIDTH	(1600)//(1280)
/* 安检机从xpack拷贝的包裹图像数据最大高，像素，140100机型AiYuv高度为1280 */
#define ISM_SVA_COPY_HEIGHT	(1280)//(1024)

#define SVA_MODULE_WIDTH	(1280)
#define SVA_MODULE_HEIGHT	(1024)
#define SVA_JPEG_MAX_SIZE   (256)

#define SVA_BUF_WIDTH_MAX   (1600)
#define SVA_BUF_HEIGHT_MAX   (1280)


#define SVA_POST_MSG_LEN		(3196)
#define SVA_MODULE_COPY_WIDTH	(1280)

/* 此四个值，XSI算法库所使用区域配置 */
#ifdef DSP_ISA

#define INPUT_HEIGHT		(0.74f)             /* 处理范围高度（设置来屏蔽水印、操作界面录像等） */
#define INPUT_WIDTH			(0.995f)             /* 处理范围宽度（设置来屏蔽水印、操作界面录像等） */
#define INPUT_Y				(0.08f)             /* 处理范围左上角Y坐标（设置来屏蔽水印、操作界面录像等） */
#define INPUT_X				(0.005f)             /* 处理范围左上角X坐标（设置来屏蔽水印、操作界面录像等） */

#elif defined DSP_ISM

#define INPUT_HEIGHT		(1.0f)             /* 处理范围高度（设置来屏蔽水印、操作界面录像等） */
#define INPUT_WIDTH			(1.0f)             /* 处理范围宽度（设置来屏蔽水印、操作界面录像等） */
#define INPUT_Y				(0.0f)             /* 处理范围左上角Y坐标（设置来屏蔽水印、操作界面录像等） */
#define INPUT_X				(0.0f)             /* 处理范围左上角X坐标（设置来屏蔽水印、操作界面录像等） */

#endif

#define XSI_DEV_MAX			(2)
#define ENG_DEV_MAX			(XSI_DEV_MAX)       /* 引擎通道暂时和DSP逻辑通道数保持一致，实际引擎能力不局限于2路 */
#define SVA_INPUT_GAP_NUM	(5)                   /* 采集数据间隔帧数，如采集60fps，间隔5帧送入xsi */

#ifdef DEBUG_SVA_DUMP
#define SVA_INPUT_BUF_LEN (60)                     /* 缓存的帧数 */
#else
#define SVA_INPUT_BUF_LEN (6)                     /* 缓存的帧数 */
#endif

#define SVA_INPUT_BUF_NUM (4)                     /* 最大缓存个数 */



#define SVA_XSI_VIDEO_MEM_SIZE	(0)          /* 引擎初始化为视频分析申请550M内存，后续资源使用有变化可更改 */
#define SVA_XSI_BA_MEM_SIZE		(50 * 1024 * 1024)       /* 引擎初始化为行为分析申请50M内存，后续资源使用有变化可更改 */
#define SVA_XSI_FACE_MEM_SIZE	(0)           /* 引擎初始化为行为分析申请50M内存，后续资源使用有变化可更改 */

#define SVAOUT_CNT (24)                          /* 存放算法处理结果的最大个数;  */

#define DUMP_YUV_MEM_SIZE (10*1024*1024)                     /* 默认保存大小为50M */
#define DUMP_ALG_YUV_PATH ("./xsi/dump_yuv_alg.nv21")        /* 文件保存路径，从片or单芯片 */
#define DUMP_SEND_YUV_PATH ("./xsi/dump_yuv_send.nv21")      /* 文件保存路径，主片 */

/* dsp demo中图像写文件到可执行文件目录下，release组件中图像写到/home/config路径下 */
#ifdef _DBG_MODE_
#define INPUT_IMG_DUMP_PATH "."
#else
#define INPUT_IMG_DUMP_PATH "/home/config"
#endif


#ifdef  WIN32
#include <time.h>
typedef clock_t TIME_STRUCT;
#elif defined (LINUX)
#include <sys/time.h>
typedef struct timeval TIME_STRUCT;
#endif

#ifdef CUDA
#include <cuda_runtime.h>
typedef cudaEvent_t CUDA_TIME_STRUCT;
#endif

#define SVA_GET_ABS(a, b)	(a < b ? b - a : a - b)
#define SVA_GET_MAX(a, b)	(a < b ? b : a)

typedef void (*SVACALLBACK)(INT32 nNodeID,
                            INT32 nCallBackType,
                            void *pstOutput,
                            UINT32 nSize,
                            void *pUsr,
                            INT32 nUserSize);       /* cb func from drv layer to hal layer */
/***************************** IN *****************************************************/
/* 锁 */
typedef struct tagSvaDrvMutexSt
{
    pthread_mutex_t lock;
} SVA_DRV_MUTEX_ST;

typedef enum _SVA_JSON_CLASS_
{
    SVA_JSON_CLASS_SECURITY_DETECT = 0,
    SVA_JSON_CLASS_NUM,
} SVA_JSON_CLASS_E;


typedef enum _SVA_ISA_JSON_SUB_CLASS_
{
    SVA_JSON_SUB_CLASS_WORK_TYPE = 0,
    SVA_JSON_SUB_CLASS_AI_TYPE,
    SVA_JSON_SUB_CLASS_PACK_CONG_TH,
    SVA_JSON_SUB_CLASS_PD_GAP,
    SVA_JSON_SUB_CLASS_SMALL_PACK_TH,
    SVA_JSON_SUB_CLASS_PD_SPLIT_THRESH,
    SVA_JSON_SUB_CLASS_CLS_BATCH_NUM,
    SVA_JSON_SUB_CLASS_MAIN_OBD_MODEL_NUM,
    SVA_JSON_SUB_CLASS_MAIN_OBD_MODEL_PATH,
    SVA_JSON_SUB_CLASS_SIDE_OBD_MODEL_NUM,
    SVA_JSON_SUB_CLASS_SIDE_OBD_MODEL_PATH,
    SVA_JSON_SUB_CLASS_MAIN_PD_MODEL_NUM,
    SVA_JSON_SUB_CLASS_MAIN_PD_MODEL_PATH,
    SVA_JSON_SUB_CLASS_SIDE_PD_MODEL_NUM,
    SVA_JSON_SUB_CLASS_SIDE_PD_MODEL_PATH,
    SVA_JSON_SUB_CLASS_MAIN_AI_MODEL_NUM,
    SVA_JSON_SUB_CLASS_MAIN_AI_MODEL_PATH,
    SVA_JSON_SUB_CLASS_SIDE_AI_MODEL_NUM,
    SVA_JSON_SUB_CLASS_SIDE_AI_MODEL_PATH,
    SVA_JSON_SUB_CLASS_CLS_MODEL_NUM,
    SVA_JSON_SUB_CLASS_CLS_MODEL_PATH,
    SVA_JSON_SUB_CLASS_POS_WRITE_FLAG,
    SVA_JSON_SUB_CLASS_NUM,
} SVA_ISA_JSON_SUB_CLASS_E;

typedef enum _SVA_ISA_JSON_CHG_CMD_
{
    CHANGE_RESOLUTION = 0,
    CHANGE_WORK_TYPE,
    CHANGE_AI_TYPE,                  /*修改Model Enable Switch*/
    CHANGE_PACK_CONF_THRESH,         /*修改AI Model 路径*/
    CHANGE_PD_GAP,                   /*修改AI Model 路径*/
    CHANGE_SMALL_PACK_THRESH,        /*修改AI Model 路径*/
    CHANGE_PD_SPLIT_THRESH,          /*修改AI Model 路径*/
    CHANGE_CLS_BATCH_NUM,            /*修改AI Model 路径*/
    CHANGE_MAIN_AI_MODEL_PATH,       /*修改AI Model 路径*/
    CHANGE_SIDE_AI_MODEL_PATH,       /*修改AI Model 路径*/
    CHANGE_MAIN_OBD_MODEL_PATH,      /*修改AI Model 路径*/
    CHANGE_SIDE_OBD_MODEL_PATH,      /*修改AI Model 路径*/
    CHANGE_MAIN_PD_MODEL_PATH,       /*修改AI Model 路径*/
    CHANGE_SIDE_PD_MODEL_PATH,       /*修改AI Model 路径*/
    CHANGE_CLS_MODEL_PATH,           /*修改分类模型 路径*/
    CHANGE_POS_WRITE_FLAG,           /* 修改pos信息是否写入的标志位  */
    CHANGE_CMD_NUM,
} SVA_ISA_JSON_CHANGE_CMD_E;


/*传入修改Json结构体*/
typedef struct tagIaIsaCfgParaJson
{
    SVA_ISA_JSON_CHANGE_CMD_E enChgType;    /*修改类型*/
    SVA_PROC_MODE_E enWorkType;             /*work 类型*/
    UINT32 uiWidth;
    UINT32 uiHeight;
    UINT32 uiBasicModelNum;
    UINT32 uiAiModelNum;
    UINT32 uiAiEnable;                      /*模型使能开关*/
    double uiPackConfTh;
    UINT32 uiPackGap;
    double uiSmallPackTh;
    UINT32 uiPdSplitTh;
    UINT32 uiClsBatchNum;
    UINT32 uiObdModelNum;
    UINT32 uiPdModelNum;
	UINT32 uiClsModelNum;
	CHAR *pcClsModelPath[SVA_CLS_MODEL_MAX_NUM];        /* 分类模型路径 */
    CHAR *pcAiModelPath[SVA_AI_MODEL_MAX_NUM];          /*开放平台AI模型路径*/
    CHAR *pcModelPath[ASM_MODEL_NUM];                   /*安检机模型路径*/
    CHAR *pcObdModelPath[SVA_DET_MODEL_MAX_NUM];        /*安检机模型路径*/
    CHAR *pcPdModelPath[SVA_PD_MODEL_MAX_NUM];          /*安检机模型路径*/

    UINT32 uiPosWriteFlag;                              /*修改pos信息是否写入的标志位*/
} IA_ISA_UPDATA_CFG_JSON_PARA;

typedef enum _SVA_ISM_JSON_SUB_CLASS_
{
    ISM_SVA_JSON_SUB_CLASS_RESO_WIDTH = 0,
    ISM_SVA_JSON_SUB_CLASS_RESO_HEIGHT,
    ISM_SVA_JSON_SUB_CLASS_WORK_TYPE,
    ISM_SVA_JSON_SUB_CLASS_AI_TYPE,
    ISM_SVA_JSON_SUB_CLASS_MAIN_VIEW_MODEL_NUM,
    ISM_SVA_JSON_SUB_CLASS_MAIN_VIEW_MODEL_PATH,
    ISM_SVA_JSON_SUB_CLASS_SUB_VIEW_MODEL_NUM,
    ISM_SVA_JSON_SUB_CLASS_SUB_VIEW_MODEL_PATH,
    ISM_SVA_JSON_SUB_CLASS_AI_MODEL_NUM,
    ISM_SVA_JSON_SUB_CLASS_AI_MODEL_PATH,
    ISM_SVA_JSON_SUB_CLASS_NUM,
} SVA_ISM_JSON_SUB_CLASS_E;

typedef enum _SVA_ISM_JSON_CHG_CMD_
{
    ISM_CHANGE_RESOLUTION = 0,
    ISM_CHANGE_WORK_TYPE,
    ISM_CHANGE_AI_TYPE,        /*修改Model Enable Switch*/
    ISM_CHANGE_AI_MODEL_PATH,       /*修改AI Model 路径*/
    ISM_CHANGE_MAIN_MODEL_PATH,          /*修改安检机模型(单双能模型) 路径*/
    ISM_CHANGE_SIDE_MODEL_PATH,          /*修改安检机模型(单双能模型) 路径*/
    ISM_CHANGE_CMD_NUM,
} SVA_ISM_JSON_CHANGE_CMD_E;

/*dspdebug 中选择下载不同地方的yuv图片，用于调试维护，可同时开启多个位置的图片下载*/
typedef enum
{
    SVA_DUMP_YUV_COPY_XPACK       = 0,     /*从xpack中copy出来的yuv图片*/
    SVA_DUMP_YUV_ENGINE,                   /*送入引擎的yuv图片*/
    SVA_DUMP_YUV_XPACK,                    /*从xpack中直接获取的yuv图片(包含9个包裹图片的大缓冲区)*/
    SVA_DUMP_YUV_SLAVE_REC,                /*从片接收的YUV图像*/
    SVA_DUMP_YUV_NUM,                      /*支持下载yuv图片位置的个数*/
} SVA_DUMP_YUV_MODE;

/*传入修改Json结构体*/
typedef struct tagIaIsmCfgParaJson
{
    SVA_ISM_JSON_SUB_CLASS_E enChgType;    /*修改类型*/
    SVA_PROC_MODE_E enWorkType;             /*work 类型*/
    UINT32 uiWidth;
    UINT32 uiHeight;
    UINT32 uiBasicModelNum;
    UINT32 uiAiModelNum;
    UINT32 uiAiEnable;                      /*模型使能开关*/
    CHAR *pcAiModelPath[AI_MODEL_NUM];       /*开放平台AI模型路径*/
    CHAR *pcModelPath[ASM_MODEL_NUM];         /*安检机模型路径*/
} IA_ISM_UPDATA_CFG_JSON_PARA;



/* 算法处理结果缓存池，用于与显示模块同步*/
typedef struct _SVA_OUT_POOL_T
{
    UINT32 w_idx;
    UINT32 r_idx;
    UINT32 count;
    SVA_PROCESS_OUT svaOut[SVAOUT_CNT];
} SVA_OUT_POOL;

typedef struct _XSI_DEV_USR_PRM_T
{
    UINT32 uiEngHdlIdx;              /* 引擎句柄索引 */

    UINT32 devChn;                   /* 业务模块通道号 */
    UINT32 syncIdx;
    UINT32 timeRef;
    UINT32 uiInputTime;              /* 调试使用: 计算算法耗时使用 */
    UINT32 imgW;                     /* 包裹分割后有效图像的宽度 */
    UINT32 imgH;                     /* 包裹分割后有效图像的高度 */
    UINT64 offsetX;                  /* 图片模式偏移坐标X */
    UINT64 pts;                      /* 目标框偏移使用的时间戳 */
    UINT64 syncPts;                  /* 上层使用用于同步原始数据和智能结果 */
    AI_PACK_PRM stAiPrm;             /* 外接透传数据结构*/
} XSI_DEV_USR_PRM_S;

/* 设备通道循环Buff结构体 */
typedef struct _XSI_DEV_BUFF_T
{
	/* buf是否占用中 */
    UINT32 buffUse;
	/* 通道号 */
    UINT32 devChn;
	/* 内存信息 */
	IA_MEM_PRM_S stMemBuf;
	/* 用户透传参数 */
    XSI_DEV_USR_PRM_S stUsrPrm;
	/* 保留字段 */
    UINT32 uiReserve[4];
} XSI_DEV_BUFF;


/* POS信息输出结构 */
typedef struct _XSI_DEV_T
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
} XSI_DEV;

typedef struct _XSIE_USER_PRVT_INFO_
{
    SVA_PROC_MODE_E mode;
    UINT32 uiDirection;
    long long nRealFrameNum;                            /* 帧号 */
    XSI_DEV_USR_PRM_S stUsrPrm[2];

    VOID *pstInputAddInfo;    /* SVA_INPUT_DATA */
} XSIE_USER_PRVT_INFO;

typedef struct _SVA_INPUT_DATA_
{
    BOOL bUseFlag;                                             /* 空闲标记 */

    XSIE_USER_PRVT_INFO stXsieUsrPrvtInfo;                     /* szl_todo: 增加DSP前缀，避免符号冲突，xsie用户私有信息 */
    XSIE_SECURITY_INPUT_PARAM_T stXsieAlgPrm;                  /* xsie算法层使用的配置参数，如灵敏度等 */
    ENGINE_COMM_INPUT_DATA_INFO stInputDataAddInfo;               /* 送入引擎层的数据，通用结构 */
} SVA_INPUT_DATA;

typedef struct _ENG_CHANNEL_T_
{
/*    ICF_APP_PARAM_INFO stAppParam;                             / * 引擎应用层参数 * / */

    UINT32 auiIptIdx2BufIdMap[SVA_INPUT_BUF_LEN];              /* 输入数据索引TO缓存数据索引的映射表 */

    VOID *pHandle;    /* szl_todo: 引擎通道句柄，与icf_common联系起来 */
    ENGINE_COMM_INPUT_DATA_INFO stInputDataAddInfo[SVA_INPUT_BUF_LEN];
    SVA_INPUT_DATA astSvaInputData[SVA_INPUT_BUF_LEN];
} ENG_CHANNEL_PRM_S;

typedef struct _XSI_COMMON_T
{
    UINT8 version[256];                                /* 版本信息 */
    void *alg_encrypt_hdl;                             /* 解密句柄 */
    UINT32 uiChannelNum;                               /* 初始化通道数 */
    ENG_CHANNEL_PRM_S stEngChnPrm[ENG_DEV_MAX];        /* 引擎通道参数 */
    XSI_DEV xsi_dev[XSI_DEV_MAX];                      /* 设备通道相关参数 */
    UINT32 uiIptEngStuckFlag;                          /* 引擎送帧接口卡住标记 */
    UINT32 uiInputEngDataNum;
    UINT32 uiInputEngDataSuccNum;
    UINT32 uiInputEngDataFailNum;

    UINT32 uiInputEngCnt;                              /* 引擎输入帧数 */
} XSI_COMMON;

/* 编图叠框信息 */
typedef struct _SVA_OSD_JPG_PRM_
{
    UINT32 uiUpTarNum;                          /* 包裹上方的违禁品总个数 */
    UINT32 uiDownTarNum;                        /* 包裹下方的违禁品总个数 */
    UINT32 uiUpLen;                             /* 包裹上方的长度 */
    UINT32 uiDownLen;                           /* 包裹下方的长度 */
    UINT32 uiOsdWidth;                          /* 违禁品OSD的宽度 */
    UINT32 uiOsdHeight;                         /* 违禁品OSD的高度 */
} SVA_OSD_JPG_PRM_S;

/*******************************************************************************
* 函数名  : Sva_HalGetDev
* 描  述  : 获取算法结果全局变量
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
XSI_COMMON *Sva_HalGetXsiCommon(void);

/*******************************************************************************
* 函数名  : Sva_HalVcaeStart
* 描  述  : 开启引擎通道
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeStart(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_HalVcaeStop
* 描  述  : 停止引擎
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeStop(UINT32 chan);

#if 1   /* TODO: save in drv layer */

/*******************************************************************************
* 函数名  : Sva_HalJudgeBackPull
* 描  述  : 判断是否是回拉状态
* 输  入  : - chan : 通道号
*         : - pPrm : 帧数据
*         : - pFlag: 标志位(判断是否回拉)
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 判断是否为回拉状态，若是返回1的标志位
*******************************************************************************/
INT32 Sva_HalJudgeBackPull(UINT32 chan, void *pPrm, void *pBpFlag);

/*******************************************************************************
* 函数名  : Sva_HalJudgeForwPull
* 描  述  : 判断是否是前拉状态
* 输  入  : - chan : 通道号
*         : - pPrm : 帧数据
*         : - pFlag: 标志位(判断是否回拉)
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 判断是否为前拉状态，若是返回1的标志位
*******************************************************************************/
INT32 Sva_HalJudgeForwPull(UINT32 chan, void *pPrm, void *pFpFlag);

#endif

/*******************************************************************************
* 函数名  : Sva_HalGetSvaOut
* 描  述  : 获取智能结果信息
* 输  入  : - chan     : 通道号
*         : - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetSvaOut(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut);

/*******************************************************************************
* 函数名  : Sva_HalSetVcaePrm
* 描  述  : 设置引擎参数
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 模式切换需要退出引擎资源(句柄等)，然后设置模式后重新创建引擎句柄
*******************************************************************************/
INT32 Sva_HalSetVcaePrm(void *prm);

/*******************************************************************************
* 函数名  : Sva_HalGetVersion
* 描  述  : 获取版本信息
* 输  入  : - prm: 版本信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetVersion(VOID *prm);

/*******************************************************************************
* 函数名  : Sva_HalGetDev
* 描  述  : 获取设备全局变量
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 设备全局变量
*******************************************************************************/
XSI_DEV *Sva_HalGetDev(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_HalSetDbLevel
* 描  述  : 设置日志打印级别
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalSetDbLevel(UINT32 level);

/*******************************************************************************
* 函数名  : Sva_HalGetFramePTS
* 描  述  : 获取帧信息中的时间戳
* 输  入  : - pu64Pts: 时间戳
*         : - pstSystemFrameInfo: 帧数据
* 输  出  : - pu64Pts: 时间戳
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFramePTS(UINT64 *pu64Pts, SYSTEM_FRAME_INFO *pstSysFrameInfo);

/*******************************************************************************
* 函数名  : Sva_HalGetDbgLevel
* 描  述  :
* 输  入  : - void:
* 输  出  : 无
* 返回值  : sva模块的调试打印级别
*******************************************************************************/
UINT32 Sva_HalGetDbgLevel(void);

/*******************************************************************************
* 函数名  : Sva_HalGetFrame
* 描  述  : 获取帧数据
* 输  入  : - pDupChnHandle: dup通道handle
*         : - pstSysFrameInfo: 帧信息
* 输  出  : - pstSysFrameInfo: 帧信息
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFrame(DUP_ChanHandle *pDupChnHandle, SYSTEM_FRAME_INFO *pstSysFrameInfo);

/*******************************************************************************
* 函数名  : Sva_HalPutFrame
* 描  述  : 获取帧数据
* 输  入  : - pDupChnHandle: dup通道handle
*         : - pstSysFrameInfo: 帧信息
* 输  出  : - pstSysFrameInfo: 帧信息
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalPutFrame(DUP_ChanHandle *pDupChnHandle, SYSTEM_FRAME_INFO *pstSysFrameInfo);

/*******************************************************************************
* 函数名  : Sva_HalVcaeSyncPutData
* 描  述  : 给引擎推帧接口
* 输  入  : - pstSysFrameInfo: 帧信息
*         : - pstSvaIn: 算法使用的参数
*         : - pChnSts: 通道状态
*         : - uiMainViewChn: 主视角通道
*         : - enProcMode: 处理模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeSyncPutData(SYSTEM_FRAME_INFO *pstSysFrameInfo[XSI_DEV_MAX],
                             SVA_PROCESS_IN *pstSvaIn,
                             UINT32 *puiChnSts,
                             UINT32 *puiXsiSts,
                             UINT32 uiMainViewChn,
                             SVA_PROC_MODE_E enProcMode);

/*******************************************************************************
* 函数名  : Sva_HalVcaeSyncImgmodePutData
* 描	 述  : 图片模型下给引擎推帧接口
* 输	 入  : - pstSysFrameInfo: 帧信息
*			: - pstSvaIn: 算法使用的参数
*			: - pChnSts: 通道状态
*			: - uiMainViewChn: 主视角通道
*			: - enProcMode: 处理模式
* 输	 出  : 无
* 返回值  : SAL_SOK  : 成功
*			  SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalVcaeSyncImgmodePutData(SYSTEM_FRAME_INFO *pstSysFrameInfo,
                                    SVA_PROCESS_IN *pstSvaIn,
                                    UINT32 *puiChnSts,
                                    UINT32 uiMainViewChn,
                                    SVA_PROC_MODE_E enProcMode,
                                    UINT32 uiW, UINT32 uiH);

/**
 * @function:   Sva_HalYuv2Bmp
 * @brief:      yuv转bmp
 * @param[in]:  SYSTEM_FRAME_INFO *pstSrcFrameInfo
 * @param[in]:  SYSTEM_FRAME_INFO *pstDstFrameInfo
 * @param[in]:  SVA_PROCESS_IN *pstIn
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 bSampleCollect
 * @param[in]:  INT8 *pcBmp
 * @param[in]:  UINT32 *pBmpSize
 * @param[in]:  UINT32 uiWidth
 * @param[in]:  UINT32 uiHeight
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalYuv2Bmp(SYSTEM_FRAME_INFO *pstSrcFrameInfo,
                     SYSTEM_FRAME_INFO *pstDstFrameInfo,
                     SVA_PROCESS_IN *pstIn,
                     SVA_PROCESS_OUT *pstOut,
                     UINT32 bSampleCollect,
                     INT8 *pcBmp,
                     UINT32 *pBmpSize,
                     UINT32 uiWidth,
                     UINT32 uiHeight,
                     UINT32 bCached);

/*******************************************************************************
* 函数名  : Sva_halDrawInit
* 描  述  : 初始化画框和OSD
* 输  入  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalDrawInit(UINT32 u32Dev, DRAW_MOD_E enLineMode, DRAW_MOD_E enOsdMode);

/**
 * @function:   Sva_HalTransTarLoc
 * @brief:      目标坐标转换
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 *puiRealTarNum
 * @param[in]:  SVA_TARGET *pstRealTarArr
 * @param[out]: None
 * @return:     static VOID
 */
VOID Sva_HalTransTarLoc(SVA_PROCESS_OUT *pstOut, UINT32 *puiRealTarNum, SVA_TARGET *pstRealTarArr);

/*******************************************************************************
* 函数名  : Sva_HalJpegDrawLine
* 描  述  : jpeg画线
* 输  入  : - chan            : 通道号
*         : - pstJpegFrame    : jpeg帧数据
*         : - pstIn           : 智能配置参数
*         : - pstOut          : 智能结果信息
*         : - uiJpegEncChn    : 编码通道
*         : - pcJpeg          : jpeg数据
*         : - pJpegSize       : jpeg大小
*         : - uiWith          : 图片宽度
*         : - uiHeight        : 图片高度
* 输  出  : - pcJpeg          : jpeg数据
*         : - pcBmp           : bmp数据
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalJpegDrawLine(UINT32 chan,
                          SYSTEM_FRAME_INFO *pstJpegFrame,
                          CROP_S *pstCropPrm,
                          SVA_PROCESS_IN *pstIn,
                          SVA_PROCESS_OUT *pstOut,
                          void *jpgHdl,
                          INT8 *pcJpeg,
                          UINT32 *pJpegSize,
                          UINT32 uiWith,
                          UINT32 uiHeight,
                          SVA_PROC_MODE_E mode,
                          SVA_OSD_JPG_PRM_S *pstOsdJpgPrm);

/*******************************************************************************
* 函数名  : Sva_HalJpegCalcTargetNum
* 描  述  : jpeg画线
* 输  入  : - chan            : 通道号
*         : - pstJpegFrame    : jpeg帧数据
*         : - pstIn           : 智能配置参数
*         : - pstOut          : 智能结果信息
*         : - uiJpegEncChn    : 编码通道
*         : - pcJpeg          : jpeg数据
*         : - pJpegSize       : jpeg大小
*         : - uiWith          : 图片宽度
*         : - uiHeight        : 图片高度
* 输  出  : - pcJpeg          : jpeg数据
*         : - pcBmp           : bmp数据
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalJpegCalcTargetNum(UINT32 chan, SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstOut, SVA_OSD_JPG_PRM_S *stSvaPicTarPrm);

/*******************************************************************************
* 函数名  : Sva_HalUpDateOut
* 描  述  : 更新智能结果
* 输  入  : - chan     : 通道号
*         : - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalUpDateOut(UINT32 chan, SVA_PROCESS_OUT *pstDrvOut);

/*******************************************************************************
* 函数名:   Sva_HalGetFromPool
* 描  述   :   获取智能结果信息
* 输  入   :   - chan     : 通道号
*                 :   - uiTimeRef: 用于匹配的帧号
*                 :   - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*                      SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFromPool(UINT32 chan, UINT32 uiTimeRef, SVA_PROCESS_OUT *pstSvaOut);

/*******************************************************************************
* 函数名:  Sva_HalSaveToPool
* 描  述   :  把SVA结果保持到缓冲池
* 输  入  : - chan     : 通道号
*                  - pstDrvOutSrc : 需要保存的SVA信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalSaveToPool(UINT32 chan, SVA_PROCESS_OUT *pstDrvOutSrc);

/*******************************************************************************
* 函数名:  Sva_HalClrPoolResult
* 描  述   :  把SVA结果保持到缓冲池
* 输  入  : - chan     : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalClrPoolResult(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_HalEngineInit
* 描  述  : 引擎资源初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalEngineInit(void);

/*******************************************************************************
* 函数名  : Sva_HalRegCbFunc
* 描  述  : 注册结果回调函数
* 输  入  : SVACALLBACK p   结果回调函数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalRegCbFunc(SVACALLBACK p);


/*******************************************************************************
* 函数名  : Sva_HalPrintCfgInfo
* 描  述  : 异常返回时调试打印
* 输  入  : 无
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalPrintCfgInfo(VOID);

/*******************************************************************************
* 函数名  : Sva_HalSetInitMode
* 描  述  : 设置初始化模式
* 输  入  : - mode: 模式
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_HalSetInitMode(SVA_PROC_MODE_E mode);

/*******************************************************************************
* 函数名  : Sva_HalGetAlgDbgFlag
* 描  述  : 获取算法调试开关
* 输  入  : 无
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
UINT32 Sva_HalGetAlgDbgFlag(void);

/**
 * @function:   Sva_HalDebugDumpData
 * @brief:      sva模块dump数据接口
 * @param[in]:  INT8 *pData
 * @param[in]:  INT8 *pPath
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     void
 */
void Sva_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiFileCtl);

/**
 * @function:   Sva_HalGetPdProcGap
 * @brief:      获取pd处理间隔
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     UINT32
 */
UINT32 Sva_HalGetPdProcGap(VOID);

/**
 * @function:   Sva_HalSetSlaveProcGap
 * @brief:      设置从片处理帧间隔
 * @param[in]:  UINT32 uiProcGap
 * @param[out]: None
 * @return:     VOID
 */
VOID Sva_HalSetSlaveProcGap(UINT32 uiProcGap);

/*******************************************************************************
* 函数名  : Sva_HalModifyCfgFile
* 描  述  : 修改配置文件中特定配置项
* 输  入  : pUpdataJson     需要修改的json内容结构体
*
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_IsaHalModifyCfgFile(IA_ISA_UPDATA_CFG_JSON_PARA *pstUpJsonPrm);

UINT32 Sva_HalPrintGapTimeMs(UINT32 t0, UINT32 t1);

/**
 * @function:   Sva_HalSetTarSizeFilterCfg
 * @brief:      设置目标尺寸过滤参数
 * @param[in]:  VOID *pHandle
 * @param[in]:  SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalSetTarSizeFilterCfg(VOID *pHandle, SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm);

/**
 * @function:   Sva_DrvJudgeOffsetEqualZero
 * @brief:      判断包裹偏移量是否等于0
 * @param[in]:  float fIn
 * @param[out]: None
 * @return:     BOOL
 */
BOOL Sva_DrvJudgeOffsetEqualZero(float fIn);

/**
 * @function:   Sva_HalJudgeStill
 * @brief:      判断包裹是否静止
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  BOOL *pStillFlag
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalJudgeStill(UINT32 chan, SVA_PROCESS_OUT *pstOut, BOOL *pStillFlag);

/**
 * @function:   Sva_HalSetTarSizeFilterCfg
 * @brief:      设置目标尺寸过滤参数
 * @param[in]:  VOID *pHandle
 * @param[in]:  SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalSetTarSizeFilterCfg(VOID *pHandle, SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm);

/**
 * @function:   Sva_HalGetDefConf
 * @brief:      设置算法检测置信度
 * @param[in]:  UINT32 chan
 * @param[out]: SVA_ALERT_CONF *pstSetAlertConf
 * @return:     INT32
 */
INT32 Sva_HalSetAlertConf(UINT32 chan, SVA_ALERT_CONF *pstSetAlertConf);

/**
 * @function:   Sva_HalGetNormalConf
 * @brief:      获取当前置信度(0~100)
 * @param[in]:  UINT32 chan
 * @param[out]: INT32 *uiDefNormalConfTab
 * @return:     INT32
 */
INT32 Sva_HalGetNormalConf(UINT32 chan, INT32 *uiDefNormalConfTab);

/**
 * @function:   Sva_HalGetDefPkConf
 * @brief:      获取默认pk置信度(0~100)
 * @param[in]:  UINT32 chan
 * @param[out]: INT32 *uiDefPkConfTab
 * @return:     INT32
 */
INT32 Sva_HalGetDefPkConf(UINT32 chan, INT32 *uiDefPkConfTab);

/**
 * @function:   Sva_HalGetDefConf
 * @brief:      从默认置信度表中获取默认置信度(0~100)
 * @param[in]:  UINT32 chan
 * @param[out]: SVA_DSP_DEF_CONF_OUT *pstXsiDefConf
 * @return:     INT32
 */
INT32 Sva_HalGetDefConf(UINT32 chan, SVA_DSP_DEF_CONF_OUT *pstXsiDefConf);



/**
 * @function:   Sva_HalGetViInputFrmRate
 * @brief:      获取VI输入帧率
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     UINT32
 */
UINT32 Sva_HalGetViInputFrmRate(VOID);

/**
 * @function    Sva_HalGetIptEngStuckFlag
 * @brief       获取引擎送帧接口是否卡住标记
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetIptEngStuckFlag(void);

UINT32 Sva_HalGetIptEngNum(void);

UINT32 Sva_HalGetIptEngSuccNum(void);

UINT32 Sva_HalGetIptEngFailNum(void);

/**
 * @function    Sva_HalGetInputEngCnt
 * @brief       获取引擎送帧个数
 * @param[in]
 * @param[out]
 * @return
 */
UINT32 Sva_HalGetInputEngCnt(void);

/**
 * @function    Sva_DrvDumpRecvYuvData
 * @brief       dump从片接收到的yuv数据(默认仅主视角)
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalDumpRecvYuvData(VOID *pVir, UINT32 uiSize, CHAR *pDumpPath);

/**
 * @function    Sva_HalResetDbgInfo
 * @brief       清空调试信息
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalResetDbgInfo(void);

/**
 * @function    Sva_HalRlsPrvtUserInfo
 * @brief       释放输入的用户附加信息
 * @param[in]   VOID *pXsieUserPrvtInfo
 * @param[out]  none
 * @return
 */
INT32 Sva_HalRlsPrvtUserInfo(VOID *pXsieUserPrvtInfo);

/**
 * @function:   Sva_HalGetIcfCommFuncP
 * @brief:      获取icf回调函数指针
 * @param[in]:  None
 * @param[out]: None
 * @return:     VOID
 */
ENGINE_COMM_FUNC_P *Sva_HalGetIcfCommFuncP(VOID);

/**
 * @function:   engine_comm_register
 * @brief:      注册通用接口函数
 * @param[in]:  None
 * @param[out]: None
 * @return:     ENGINE_COMM_FUNC_P *
 */
ENGINE_COMM_FUNC_P *engine_comm_register(VOID);
/**
 * @function   Sva_HalDumpInputImg
 * @brief      dump送入引擎的图像(当前仅安检机图片模式使用)
 * @param[in]  VOID *pImgVir  
 * @param[in]  CHAR *pHead    
 * @param[in]  UINT32 frmNum  
 * @param[out] None
 * @return     static VOID
 */
VOID Sva_HalDumpInputImg(VOID *pImgVir, UINT32 wihgt, UINT32 hight, CHAR *pHead, UINT32 chan, UINT64 frmNum, SVA_DUMP_YUV_MODE mode);

#endif

