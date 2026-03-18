/**
 * @file:   ppm_drv.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联驱动层头文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */
#ifndef _PPM_DRV_H_
#define _PPM_DRV_H_
/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "ppm_hal.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define PPM_MAX_QUEUE_BUF_NUM_V0	(6)
#define PPM_MAX_QUEUE_BUF_NUM_V1	(32) /* (32)//(6)   / * 同步方案V0版本使用同一个队列保存多源数据，V1版本将多路源数据队列分开便于后续逻辑同步处理 * / */
#define PPM_MAX_DET_OBJ_NUM			(16)
#define PPM_MAX_XSI_FRM_NUM			(2)
#define PPM_MAX_QUE_FRM_NUM			(PPM_MAX_XSI_FRM_NUM)
#define PPM_CALIB_E_IMG_SIZE            (1)         /* 图像尺寸不匹配 */
#define PPM_CALIB_E_NULL_POINT          (2)         /* 空指针 */ 
#define PPM_CALIB_E_IMG_NUM             (3)         /* 图像个数错误（[1, MCC_MAX_IMG_NUM]） */
#define PPM_CALIB_E_PROC_IN_TYEP        (4)         /* 输入参数类型错误 */
#define PPM_CALIB_E_PROC_OUT_TYEP       (5)         /* 输出参数类型错误 */
#define PPM_CALIB_E_CALIB_IMG_NUM       (6)         /* 缺少有效的标定板图像 */

#define PPM_HANDLE_MAX_NUM       (8)

/* 深度检测目标最大个数 */
#define PPM_DEPTH_MAX_DETECT_OBJ_NUM (16)
/* 深度信息队列的最大深度 */
#define PPM_DEPTH_INFO_MAX_QUEUE_SIZE (128)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef enum _PPM_TRI_CAM_CHN_E_
{
    PPM_TRI_CAM_LEFT_CHN = 0,                /* 三目相机左视角对应的VPSS拓展通道 */
    PPM_TRI_CAM_RIGHT_CHN,                   /* 三目相机右视角对应的VPSS拓展通道 */
    PPM_TRI_CAM_MIDDLE_CHN,                  /* 三目相机中路视角对应的VPSS拓展通道 */
    PPM_TRI_CAM_OUT_CHN_NUM,
} PPM_TRI_CAM_CHN_E;

typedef enum tagPpmQueType
{
    SINGLE_QUEUE_TYPE = 0,                  /* 单队列模型 */
    DOUBLE_QUEUE_TYPE,                      /* 双队列模型 */
    MAX_QUEUE_TYPE,
} PPM_QUE_TYPE_E;

typedef struct tagPpmQuePrm
{
    PPM_QUE_TYPE_E enType;             /* 队列模型 */

    UINT32 uiBufCnt;                  /* Buf个数 */
    UINT32 uiBufSize;                 /* 单个Buf大小 */
    DSA_QueHndl *pstFullQue;          /* 数据满队列 */
    DSA_QueHndl *pstEmptQue;          /* 数据空队列 */
} PPM_QUEUE_PRM;

/* 支持的关节类型 */
typedef enum _PPM_JOINT_TYPE_
{
	/* 左手腕 */
	LEFT_HAND_WRIST = 0,
	/* 左手肘 */
	LEFT_ELBOW,
	/* 左肩 */
	LEFT_SHOULDER,
	/* 头 */
	HEAD_POINT,
	/* 右肩 */
	RIGHT_SHOULDER,
	/* 右手肘 */
	RIGHT_ELBOW,
	/* 右手腕 */
	RIGHT_HAND_WRIST,

	PPM_JOINT_NUM,
} PPM_JOINT_TYPE_E;

/* 二维头肩框坐标 */
typedef struct _PPM_RECT_FLOAT_
{
	float x;
	float y;
	float w;
	float h;
} PPM_RECT_FLOAT_S;

/* 三维点坐标 */
typedef struct _PPM_POINT_3D_FLOAT_
{
	float x;
	float y;
	float z;
} PPM_POINT_3D_FLOAT_S;

/* 二维点坐标 */
typedef struct _PPM_POINT_2D_FLOAT_
{
	float x;
	float y;
} PPM_POINT_2D_FLOAT_S;

typedef struct _PPM_JOINT_INFO_
{
	/* 关节点 */
	BOOL bJointValid;
	/* 关节类型 */
	PPM_JOINT_TYPE_E u32JointType;
	/* 关节点评分 */
	float fJointScore;
	/* 关节点二维坐标 */
	PPM_POINT_2D_FLOAT_S st2dPoint;
	/* 关节点三维坐标 */
	PPM_POINT_3D_FLOAT_S st3dPoint;
} PPM_JOINT_INFO_S;

typedef struct _PPM_DEPTH_OBJ_INFO_
{
	/* 目标id */
	UINT32 u32Id;
	/* 目标是否有效 */
	BOOL bValid;
	/* 目标是否消失 */
	BOOL bDisapper;
	/* 目标高度，三维 */
	float fHeight;
	/* 头肩二维区域 */
	PPM_RECT_FLOAT_S stHeadRect;

	/* 关节点id */
	UINT32 u32JointId;
	/* 关节点个数 */
	UINT32 u32JointNum;
	/* 关节点信息 */
	PPM_JOINT_INFO_S astJointInfo[PPM_JOINT_NUM];
} PPM_DEPTH_OBJ_INFO_S;

typedef struct _PPM_DEPTH_INFO_
{
	/* 检测到的目标个数 */
	UINT32 u32DetObjNum;
	/* 目标信息 */
	PPM_DEPTH_OBJ_INFO_S astDetObjInfo[PPM_DEPTH_MAX_DETECT_OBJ_NUM];
	
	/* 全局时间戳 */
	UINT64 u64Timestamp;
} PPM_DEPTH_INFO_S;

typedef struct tagPpmDepthInfoQuePrmSt
{
    PPM_QUEUE_PRM stDepthQuePrm;
    PPM_DEPTH_INFO_S astDepthInfo[PPM_DEPTH_INFO_MAX_QUEUE_SIZE];
} PPM_DEPTH_INFO_QUE_PRM;

typedef struct tagPpmFrmInfoSt
{
    SYSTEM_FRAME_INFO stTriCamFrame;                          /* 三目相机帧数据 */
    SYSTEM_FRAME_INFO stXpkgFrame[PPM_MAX_QUE_FRM_NUM];       /* 安检机输出帧数据 */
    SYSTEM_FRAME_INFO stFaceFrame;                            /* 人脸帧数据 */
	PPM_DEPTH_INFO_S stDepthInfo;
    UINT32 uiReserved[4];                                     /* 保留位 */
} PPM_FRAME_INFO_ST;

typedef struct tagPpmYuvQuePrmSt
{
    PPM_QUEUE_PRM stYuvQuePrm;
    PPM_FRAME_INFO_ST stYuvFrame[PPM_MAX_QUEUE_BUF_NUM_V1];
} PPM_YUV_QUE_PRM;

typedef struct _PPM_IPT_YUV_DATA_
{
    SYSTEM_FRAME_INFO stTriViewYuv[PPM_TRI_CAM_OUT_CHN_NUM];          /* 三视角数据，依次为左右中 */
    SYSTEM_FRAME_INFO stXpkgYuv[PPM_MAX_QUE_FRM_NUM];
    SYSTEM_FRAME_INFO stFaceYuv;

	/* RK人包深度目标信息 */
	MTME_OBJ_INPUT_INFO_T stMtmeInputObjInfo;
} PPM_IPT_YUV_DATA_INFO;

typedef struct tagPpmIptDataQuePrmSt
{
    PPM_QUEUE_PRM stIptQuePrm;
    PPM_IPT_YUV_DATA_INFO stIptYuvData[PPM_MAX_QUEUE_BUF_NUM_V0];
} PPM_INPUT_DATA_QUE_PRM;

#if 1  /* PPM_SYNC_V1版本增加的两路IPC队列参数 */
typedef struct tagPpmComFrmInfoSt
{
    UINT32 uiFrmCnt;                                           /* 帧数据个数 */
    SYSTEM_FRAME_INFO stFrame[PPM_MAX_QUE_FRM_NUM];            /* 帧数据 */
    UINT32 uiReserved[4];                                      /* 保留位 */
} PPM_COM_FRAME_INFO_ST;

typedef struct tagPpmTriYuvQuePrmSt
{
    PPM_QUEUE_PRM stYuvQuePrm;
    PPM_COM_FRAME_INFO_ST stYuvFrame[PPM_MAX_QUEUE_BUF_NUM_V1];
} PPM_TRI_YUV_QUE_PRM;

typedef struct tagPpmFaceYuvQuePrmSt
{
    PPM_QUEUE_PRM stYuvQuePrm;
    PPM_COM_FRAME_INFO_ST stYuvFrame[PPM_MAX_QUEUE_BUF_NUM_V1];
} PPM_FACE_YUV_QUE_PRM;

typedef struct tagPpmXsiYuvQuePrmSt
{
    PPM_QUEUE_PRM stYuvQuePrm;
    PPM_COM_FRAME_INFO_ST stYuvFrame[PPM_MAX_QUEUE_BUF_NUM_V1];
} PPM_XSI_YUV_QUE_PRM;
#endif

typedef struct tagPpmFaceObjSt
{
    UINT32 uiMatchId;                              /* 匹配ID */
    SYSTEM_FRAME_INFO stFrameInfo;                 /* 帧数据 */
} PPM_FACE_OBJ_INFO_S;

typedef struct tagPpmFaceRsltSt
{
    UINT32 uiFaceCnt;                                          /* 人脸个数 */
    UINT64 uiJpegPts;                                          /* 人脸时间戳 */
    PPM_FACE_OBJ_INFO_S stFaceObjInfo[PPM_MAX_DET_OBJ_NUM];    /* 人脸目标信息 */
} PPM_FACE_RSLT_S;

typedef struct tagPpmPkgObjSt
{
    UINT32 uiMatchId;                             /* 匹配ID */
    SYSTEM_FRAME_INFO stFrameInfo;                /* 帧数据 */
} PPM_PKG_OBJ_INFO_S;

typedef struct tagPpmPkgRsltSt
{
    UINT32 uiPkgCnt;                                          /* 可见光包裹个数 */
    UINT64 uiJpegPts;                                         /* 可见光时间戳 */
    PPM_PKG_OBJ_INFO_S stPkgObjInfo[PPM_MAX_DET_OBJ_NUM];     /* 可见光包裹目标信息 */
} PPM_PKG_RSLT_S;

typedef struct tagPpmXsiObjSt
{
    UINT32 uiMatchId;                                         /* 匹配ID */
    SYSTEM_FRAME_INFO stFrameInfo[PPM_MAX_XSI_FRM_NUM];       /* 帧数据，X光最多有两路视角数据 */
} PPM_XSI_OBJ_INFO_S;

typedef struct tagPpmXsiRsltSt
{
    UINT32 uiXsiCnt;                                           /* 安检机包裹个数 */
    PPM_XSI_OBJ_INFO_S stXsiObjInfo[PPM_MAX_DET_OBJ_NUM];      /* 安检机包裹目标信息 */
} PPM_XSI_RSLT_S;

typedef struct tagPpmRsltInfoSt
{
    PPM_XSI_RSLT_S stXsiRsltOut;                     /* 安检机包裹结果 */
    PPM_PKG_RSLT_S stPkgRsltOut;                     /* 可见光包裹结果 */
    PPM_FACE_RSLT_S stFaceRsltOut;                   /* 人脸结果 */

    UINT32 uiReverse[8];                             /* 保留字段 */
} PPM_RSLT_INFO_S;

typedef struct tagPpmRsltQuePrmSt
{
    PPM_QUEUE_PRM stRsltQuePrm;
    PPM_RSLT_INFO_S stRsltInfo[PPM_MAX_QUEUE_BUF_NUM_V0];
} PPM_RSLT_QUE_PRM;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
/**
 * @function:   Ppm_DrvSetPosSwitch
 * @brief:      配置POS信息的开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetPosSwitch(void *pPrm);

/**
 * @function:   Ppm_DrvSetConfTh
 * @brief:      配置置信度
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetConfTh(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvGetVersion
 * @brief:      获取引擎版本号信息
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvGetVersion(VOID *prm);

/**
 * @function:   Ppm_DrvSetVprocHandle
 * @brief:      保存vproc通道句柄
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static PPM_DEV_PRM *
 */
INT32 Ppm_DrvSetVprocHandle(UINT32 chan, UINT32 u32OutChn, VOID *pDupChnHandle);

/**
 * @function:   Ppm_DrvGenerateAlgCfgFile
 * @brief:      生成算法依赖的配置文件(三目mr、lr标定参数)
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvGenerateAlgCfgFile(VOID *prm);

/**
 * @function:   Ppm_DrvSetCalibPrm
 * @brief:      输入标定参数，获取标定结果
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetCalibPrm(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvSetFaceCalibPrm
 * @brief:      输入人脸相机标定参数，获取标定结果
 * @param[in]:  UINT32 chan  
 * @param[in]:  void *pPrm   
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetFaceCalibPrm(UINT32 chan, void *pPrm);

/**
 * @function   Ppm_DrvSetCapABRegion
 * @brief      设置抓拍AB区域
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *pPrm   
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvSetCapABRegion(UINT32 chan, VOID *pPrm);

/**
 * @function   Ppm_DrvSetFaceRoi
 * @brief      设置人脸ROI区域
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *pPrm   
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvSetFaceRoi(UINT32 chan, VOID *pPrm);

/**
 * @function   Ppm_DrvSetPkgCapRegion
 * @brief      设置可见光抓拍区域
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *pPrm   
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvSetPkgCapRegion(UINT32 chan, VOID *pPrm);

/**
 * @function:   Ppm_DrvSetFaceScoreFilter
 * @brief:      设置人脸评分阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetFaceScoreFilter(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvSetMatchMatrixPrm
 * @brief:      配置匹配矩阵参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetMatchMatrixPrm(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvSetFaceMatchThresh
 * @brief:      设置人脸iou阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetFaceMatchThresh(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvSetDetSensity
 * @brief:      匹配检测灵敏度配置
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetDetSensity(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvSetXsiMatchPrm
 * @brief:      配置xsi匹配参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetXsiMatchPrm(UINT32 chan, void *pPrm);

/**
 * @function:   Ppm_DrvDisableChannel
 * @brief:      模块通道关闭
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvDisableChannel(UINT32 chan);

/**
 * @function:   Ppm_DrvEnableChannel
 * @brief:      模块通道开启
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvEnableChannel(UINT32 chan);

/**
 * @function:   Ppm_DrvModuleDeinit
 * @brief:      模块资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvModuleDeinit(VOID);

/**
 * @function:   Ppm_DrvModuleInit
 * @brief:      模块资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvModuleInit(VOID);

/**
 * @function:   Ppm_DrvDeinit
 * @brief:      人包关联业务层去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvDeinit(VOID);

/**
 * @function:   Ppm_DrvInit
 * @brief:      人包关联业务层初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvInit(VOID);

#endif

