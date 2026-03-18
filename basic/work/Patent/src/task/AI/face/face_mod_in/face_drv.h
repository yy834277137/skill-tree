/*******************************************************************************
* face_drv.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : zongkai5 <zongkai5@hikvision.com>
* Version: V2.0.0  2021年11月1日 Create
*
* Description :
* Modification:
*******************************************************************************/

#ifndef _FACE_DRV_H_
#define _FACE_DRV_H_


/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
#include "sal.h"
/* #include "system_common.h" */
#include <dspcommon.h>
#include <platform_hal.h>
#include "jdec_soft.h"
#include "jpeg_drv.h"
#include "face_hal.h"

/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */
#define FACE_MAX_CHAN			(3)
#define FACE_MAX_PRM_BUFFER		(6)
#define FACE_MAX_YUV_BUF_NUM	(6)
#define FACE_MAX_PIC_FRAME_NUM	(4)
#define MAX_JPEG_WIDTH			(1920)
#define MAX_JPEG_HEIGHT			(1080)
#define MAX_JPEG_SIZE			(4 * 4096 * 4096)

/* 单个选帧图像中最多的人脸个数 */
#define FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME (MAX_CAP_FACE_NUM)

/* 人脸抓拍业务线中节点1的最大队列深度 */
#define FACE_MAX_VIDEO_CAP_SELECT_QUE_DEPTH	(6)


/* 错误返回 */
#define ICF_CHECK_ERROR(sts, ret)                                                                           \
    {                                                                                                           \
        if (sts)                                                                                                \
        {                                                                                                       \
            FACE_LOGE("\nerror !!!: %s, %s, line: %d, return 0x%x\n\n", __FILE__, __FUNCTION__, __LINE__, ret);    \
            return (ret);                                                                                       \
        }                                                                                                       \
    }
/* ========================================================================== */
/*                          数据结构区									   */
/* ========================================================================== */
typedef enum tagFaceJdecMode
{
    FACE_JDEC_HW_MODE = 0,
    FACE_JDEC_SOFT_MODE = 1,
    FACE_JDEC_MODE_NUM,
} FACE_JDEC_MODE_E;

typedef enum tagFaceErrCode
{
    FACE_RESOLUTION_OVER_SIZE = 0,
    FACE_REG_ERR_MODE_NUM,
} FACE_JDEC_ERR_CODE_E;

typedef struct tagFaceRegRslt
{
    FACE_JDEC_ERR_CODE_E eErrCode;
    FACE_JDEC_MODE_E eJdecMode;
} FACE_REG_RESULT_S;

/* 人脸配置参数*/
typedef union tagFaceConfigParam
{
    /* 算法需要每帧更新的参数 */
    float uiThreshold;                          /* 检测阈值 */
    SVA_RECT_F rect;                            /* 检测框 */
} FACE_CONFIG_PARAM_U;

/* 人脸功能类参数*/
typedef struct tagFaceProcessIn
{
    /* 算法需要每帧更新的参数 */
    float faceLoginThreshold;                           /* 人脸登录检测阈值 */
    float faceCapThreashold;                            /* 人脸抓拍检测阈值参数，szl_face_todo: unused */
    SVA_RECT_F faceLoginRect;                           /* 人脸登录检测框，szl_face_todo: unused */
    SVA_RECT_F faceCapDetRect;                          /* 人脸抓拍检测区域，szl_face_todo: unused */
} FACE_PROCESS_IN;

/* jpeg编码信息 */
typedef struct tagFaceJpegVencPrmSt
{
    INT8 *pPicJpeg[FACE_MAX_PIC_FRAME_NUM];                   /* jpeg文件地址 */

    UINT8 bDecChnStatus;               /* 编码通道是否已创建 */

    UINT32 uiPicJpegSize;             /* jpeg大小 */

    void *pstJpegChnInfo;             /* jpeg通道信息 */
} FACE_JPEG_VENC_PRM;

/*jpg编码数据队列格式*/
typedef struct tagFaceJpegInfoSt
{
    /* 背景图 */
    SYSTEM_FRAME_INFO stFrame;

    /*抓图人脸数量*/
    UINT32 u32FaceNum;

    /*人脸抠图坐标,用于jpg抠图,不上传给应用*/
    FACE_RECT stFaceRec[FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME];

    /*人脸特征数据,最多支持3张人脸特征数据*/
    char acFaceFeature[FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME][FACE_FEATURE_LENGTH];

    /*人脸属性信息*/
    FACE_ATTRIBUTE_DSP_OUT astFaceAttrubute[FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME];

    /* 保留位 */
    UINT32 uiReserved[4];
} FACE_FRAME_INFO_ST;

/*人脸队列类型*/
typedef enum tagFaceQueType
{
    SINGLE_QUEUE_TYPE = 0,                  /* 单队列模型 */
    DOUBLE_QUEUE_TYPE,                      /* 双队列模型 */
    MAX_QUEUE_TYPE,
} FACE_QUE_TYPE_E;

/*人脸抓拍队列参数*/
typedef struct tagFaceQuePrm
{
    FACE_QUE_TYPE_E enType;           /* 队列模型 */
    UINT32 uiBufCnt;                  /* Buf个数 */
    UINT32 uiBufSize;                 /* 单个Buf大小 */
    DSA_QueHndl *pstFullQue;          /* 数据满队列 */
    DSA_QueHndl *pstEmptQue;          /* 数据空队列 */
} FACE_QUEUE_PRM;

/*人脸抓拍YUV缓存参数*/
typedef struct tagFaceYuvQuePrmSt
{
    FACE_QUEUE_PRM pstFaceYuvQuePrm;
    FACE_FRAME_INFO_ST *pstFaceYuvFrame[FACE_MAX_YUV_BUF_NUM];
} FACE_YUV_QUE_PRM;

typedef struct _FACE_VIDEO_CAP_SELECT_QUE_DATA_
{
    /* 背景图 */
    SYSTEM_FRAME_INFO stFrame;

    /*抓图人脸数量*/
    UINT32 u32FaceNum;

    /*人脸抠图坐标,用于jpg抠图,不上传给应用*/
    FACE_RECT stFaceRec[FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME];

    /* 保留位 */
    UINT32 uiReserved[4];
} FACE_VIDEO_CAP_SELECT_QUE_DATA_S;

typedef struct _FACE_VIDEO_CAP_SELECT_QUE_PRM_
{
    FACE_QUEUE_PRM stFaceSelQuePrm;
    FACE_VIDEO_CAP_SELECT_QUE_DATA_S stFaceSelInfo[FACE_MAX_VIDEO_CAP_SELECT_QUE_DEPTH];
} FACE_VIDEO_CAP_SELECT_QUE_PRM_S;

typedef struct _FACE_VIDEO_LOGIN_SELECT_QUE_DATA_
{
    /* 背景图 */
    SYSTEM_FRAME_INFO stFrame;

    /*抓图人脸数量*/
    UINT32 u32FaceNum;

    /*人脸抠图坐标,用于jpg抠图,不上传给应用*/
    FACE_RECT stFaceRec[FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME];

    /* 保留位 */
    UINT32 uiReserved[4];
} FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S;

typedef struct _FACE_VIDEO_LOGIN_SELECT_QUE_PRM_
{
    FACE_QUEUE_PRM stFaceSelQuePrm;
    FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S stFaceSelInfo[FACE_MAX_VIDEO_CAP_SELECT_QUE_DEPTH];
} FACE_VIDEO_LOGIN_SELECT_QUE_PRM_S;

/*人脸转YUV内部缓存数据结构参数*/
typedef struct tagFaceToYuvMem
{
    UINT32 u32PoolId;
    UINT64 u64VbBlk;
    void *pVirAddr;         /*虚拟地址*/
    PhysAddr uiPhyAddr;     /*物理地址*/
    UINT32 bufLen;          /*缓存长度*/
} FACE_YUV_MEM_PRM;

/* 安检人脸算法模块参数 */
typedef struct tagFacePrmSt
{
    UINT32 chan;

    UINT32 bUpFrmThStart;                        /* 是否更新抓拍通道码流,用于控制是否抓拍 */

    UINT32 bLoginFlag;                           /* 是否触发人脸登录，用于停止更新码流线程，与bUpFrmFlag配对使用 */

    UINT32 bUpFrmStopFlag;                       /* 保持解码码流缓存数据最新，用于人脸登录 */

    UINT32 uiJpegVdecChn;                        /* 图片解码通道 */

    UINT32 uiFrameFaceLogVdecChn;                /* 人脸登录码流解码通道 */

    UINT32 uiFrameFaceCapVdecChn;                /*人脸抓拍码流解码通道*/

    UINT32 uiChnStatus;                          /* 通道创建状态 */

    FACE_YUV_MEM_PRM stFaceYuvMem;               /*人脸转YUV软编码拷贝内存*/

    FACE_PROCESS_IN stIn;                        /* 上层可配参数 */

    FACE_JPEG_VENC_PRM stJpegVenc;                       /*人脸jpg编码信息*/

    /* 人脸登录业务线第一个节点队列，存放选帧图像数据和人脸区域 */
    FACE_VIDEO_LOGIN_SELECT_QUE_PRM_S stFaceVideoLoginSelQueInfo;

    /* 人脸抓拍业务线第一个节点队列，存放选帧图像数据和人脸区域 */
    FACE_VIDEO_CAP_SELECT_QUE_PRM_S stFaceVideoCapSelQueInfo;

    /* 人脸抓拍业务线第二个节点队列，存放来自于节点一的结果+人脸属性和建模数据 */
    FACE_YUV_QUE_PRM stFaceYuvQueInfo;

    SYSTEM_FRAME_INFO stRegisterFrameInfo;              /* 帧数据，人脸注册使用 */

    SYSTEM_FRAME_INFO stRegTmpFrameInfo;              /* 帧数据，人脸注册四字节对齐使用 */

    SYSTEM_FRAME_INFO stLoginFrameInfo;                 /* 帧数据，人脸登录使用 */

    SYSTEM_FRAME_INFO stCapFrameInfo;                   /* 帧数据，人脸抓拍使用 */

    DSA_QueHndl stPrmFullQue;                    /* 设置参数数据满队列 */

    DSA_QueHndl stPrmEmptQue;                    /* 设置参数数据空队列 */

    SAL_ThrHndl stFaceLoginFeatThrHandl;                /* 人脸登录送入建模节点线程 */

    SAL_ThrHndl stFaceGetFrameHandle;                   /*人脸抓拍获取帧数据线程handle*/

    SAL_ThrHndl stFaceCapAttrFeatThrHandl;                  /*人脸抓拍业务线基于人脸选帧结果获取人脸属性和建模数据 线程*/

    SAL_ThrHndl stFaceCapProcThrHandl;                  /*人脸抓拍算法处理线程*/

    void *mChnMutexHdl;                          /* 通道信号量*/
} FACE_DEV_PRM;

/* 安检算法人脸模块全局参数 */
typedef struct tagFaceCommonSt
{
    UINT32 uiPublic;                            /* 全局参数初始化状态 */
    UINT32 uiDevCnt;                            /* 算法通道个数 */
    FACE_DEV_PRM stFace_dev[4];                 /* 算法通道，人脸处理预留四个通道 */
} FACE_COMMON;

/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : Face_DrvGetDev
* 描  述  : 获取全局变量
* 输  入  : chan: 通道号
* 输  出  : 无
* 返回值  : 人脸模块全局变量
*******************************************************************************/
FACE_DEV_PRM *Face_DrvGetDev(UINT32 chan);

/*******************************************************************************
* 函数名  : Face_DrvSetDetRegion
* 描  述  : 设置检测区域
* 输  入  : chan  : 通道号
*         : pParam: 区域信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 Face_DrvSetDetRegion(UINT32 chan, void *pParam);


/*******************************************************************************
* 函数名  : Face_DrvGetVersion
* 描  述  : 获取算法版本号
* 输  入  : pParam: 版本信息
* 输  出  : pParam: 版本信息
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvGetVersion(void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvClearDataBase
* 描  述  : 清空本地数据库
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 Face_DrvClearDataBase(void);

/*******************************************************************************
* 函数名  : Face_DrvUpdateDataBase
* 描  述  : 更新本地数据库信息
* 输  入  : chan  : 通道号
*         : pParam: 更新数据库的参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvUpdateDataBase(UINT32 chan, void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvUpdateCompLib
* 描  述  : 更新人脸比对库
* 输  入  : chan  : 通道号
*         : pParam: 更新比对库使用参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvUpdateCompLib(UINT32 chan, void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvJudgeFeatData
* 描  述  : 判断待注册人脸是否已注册
* 输  入  : chan  : 通道号
*         : pParam: 特征数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvJudgeFeatData(UINT32 chan, void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvUserRegister
* 描  述  : 人脸注册
* 输  入  : chan  : 通道号
*         : pParam: 人脸注册相关信息
* 输  出  : pParam: 注册成功后返回特征数据
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvUserRegister(UINT32 chan, void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvUserLogin
* 描  述  : 人脸登录
* 输  入  : chan  : 通道号
*         : pParam: 人脸登录使用的参数
* 输  出  : pParam: 登录成功后返回人脸ID
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvUserLogin(UINT32 chan, void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvSetConfig
* 描  述  : 人脸登录
* 输  入  : chan  : 通道号
*         : pParam: 配置参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvSetConfig(UINT32 chan, void *pParam);

/*******************************************************************************
* 函数名  : Face_DrvModuleInit
* 描  述  : 人脸模块初始化
* 输  入  : - chan: 通道号
*         : - pPrm: 参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*			 SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvModuleInit(UINT32 chan, void *pPrm);

/*******************************************************************************
* 函数名  : Face_DrvModuleDeInit
* 描  述  : 人脸模块去初始化
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*			 SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvModuleDeInit(UINT32 chan);

/*******************************************************************************
* 函数名  : Face_DrvDataBaseInit
* 描  述  : 本地数据库初始化
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*			SAL_FAIL : 失败
*******************************************************************************/
UINT32 Face_DrvDataBaseInit(VOID);

/*******************************************************************************
* 函数名  : Face_DrvDeInit
* 描  述  : 人脸模块资源退出
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*			SAL_FAIL : 失败
*******************************************************************************/
UINT32 Face_DrvDeInit(VOID);

/*******************************************************************************
* 函数名  : Face_DrvInit
* 描  述  :
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*			 SAL_FAIL : 失败
*******************************************************************************/
INT32 Face_DrvInit(VOID);

/**
 * @function   Face_DrvStartCap
 * @brief      开启人脸抓拍命令
 * @param[in]  UINT32 chan
 * @param[out] None
 * @return     INT32
 */
INT32 Face_DrvStartCap(UINT32 chan);

/**
 * @function   Face_DrvStopCap
 * @brief      关闭人脸抓拍命令
 * @param[in]  UINT32 chan
 * @param[out] None
 * @return     INT32
 */
INT32 Face_DrvStopCap(UINT32 chan);

/**
 * @function   Face_DrvCompareFeatureData
 * @brief      人脸特征数据比对接口
 * @param[in]  UINT32 chan
 * @param[in]  void *pParam
 * @param[out] None
 * @return     INT32
 */
INT32 Face_DrvCompareFeatureData(UINT32 chan, void *pParam);

/**
 * @function   Face_DrvGetFeatureVersionRst
 * @brief      用于特征数据版本获取接口
 * @param[in]  void *pFaceCmpData
 * @param[out] None
 * @return     INT32
 */
INT32 Face_DrvGetFeatureVersionRst(void *pFaceCmpData);

#endif

