/**
 * @file:   ppm_drv.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联驱动层源文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"
#include "ppm_drv.h"
#include "dup_tsk.h"
#include "disp_tsk_api.h"
#include "system_prm_api.h"
#include "ia_common.h"
#include "platform_hal.h"
#include "jpeg_drv_api.h"
#include "dup_hal_api.h"
#include "vdec_tsk_api.h"

/* jpeg软解库相关头文件 */
#include "jdec_soft.h"
#include "jpgd_lib.h"

/* 软解jdec用到neon指令集 */
#include <arm_neon.h>

#include "plat_com_inter_hal.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define PPM_DRV_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            PPM_LOGE("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define PPM_DRV_CHECK_RET_NO_LOOP(ret, str) \
    { \
        if (ret) \
        { \
            PPM_LOGE("%s, ret: 0x%x \n", str, ret); \
        } \
    }

#define PPM_DRV_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            PPM_LOGE("%s \n", str); \
            goto loop; \
        } \
    }

#define PPM_DRV_CHECK_MOD_CHAN(chan, value) \
    { \
        if (chan > MAX_PPM_CHN_NUM - 1) \
        { \
            PPM_LOGE("Invalid mod chan %d \n", chan); \
            return value; \
        } \
    }


#define MAX_PPM_CHN_NUM	(1)           /* 人包关联通道个数 */

#define PPM_TRI_CAM_CHN_WIDTH	(2560)                          /* 三目相机第四码流通道宽度 */
#define PPM_TRI_CAM_CHN_HEIGHT	(1440)                          /* 三目相机第四码流通道高度 */

#define PPM_TRI_CAM_LEFT_CHN_WIDTH	(768)                      /* 三目相机左视角通道宽度 */
#define PPM_TRI_CAM_LEFT_CHN_HEIGHT (432)                      /* 三目相机左视角通道高度 */

#define PPM_TRI_CAM_RIGHT_CHN_WIDTH		(768)                  /* 三目相机右视角通道宽度 */
#define PPM_TRI_CAM_RIGHT_CHN_HEIGHT	(432)                  /* 三目相机右视角通道高度 */

#define PPM_TRI_CAM_MIDDLE_CHN_WIDTH	(1280)                  /* 三目相机中路视角通道宽度 */
#define PPM_TRI_CAM_MIDDLE_CHN_HEIGHT	(720)                  /* 三目相机中路视角通道高度 */

#define PPM_RK_TRI_CAM_MIDDLE_CHN_WIDTH	    (1920)                  /* RK:三目相机中路视角通道宽度 */
#define PPM_RK_TRI_CAM_MIDDLE_CHN_HEIGHT	(1080)              /* RK:三目相机中路视角通道高度 */

#define PPM_ISM_CHN_WIDTH	(1280)                      /* 安检机输出图片宽度 */
#define PPM_ISM_CHN_HEIGHT	(720)                       /* 安检机输出图片高度 */

#define PPM_FACE_WIDTH	(1920)                        /* 人脸相机输出图片宽度 */
#define PPM_FACE_HEIGHT (1080)                        /* 人脸相机输出图片高度 */

#define PPM_TRI_CAM_VPSS_CHN (0)                        /* 三目相机第四码流总画面 */

#define PPM_TRI_CAM_VPSS_CROP_W (1280)                  /* 三目相机全景图抠图宽度 */
#define PPM_TRI_CAM_VPSS_CROP_H (720)                   /* 三目相机全景图抠图高度 */

/* demo（引擎外部） 错误码 */
#define DEMO_ERR_FILE_OPEN			(0x10000000)           /* 文件打开错误 */
#define DEMO_ERR_FILE_READ			(0x10000001)           /* 文件读取错误 */
#define DEMO_ERR_FILE_SEEK			(0x10000002)           /* 文件seek错误 */
#define DEMO_ERR_FILE_SCANF			(0x10000003)           /* 文件scanf错误 */
#define DEMO_ERR_FILE_WRITE			(0x10000004)           /* 文件写取错误 */
#define DEMO_ERR_MEM_ALLOC			(0x10000010)           /* 内存申请错误 */
#define DEMO_ERR_INPUT_MEDIA		(0x10000020)           /* 内存申请错误 */
#define DEMO_ERR_THREAD_CREATE		(0x10000030)           /* 线程创建错误 */
#define DEMO_ERR_THREAD_SYNC		(0x10000031)           /* 线程同步错误 */
#define DEMO_ERR_NOT_FIND_PACK_OUT	(0x10000040)           /* 未找到包裹关联输出 */
#define DEMO_ERR_NOT_FIND_XSI_OUT	(0x10000041)           /* 未找到X光包裹输出 */
#define DEMO_ERR_NOT_FIND_FACE_OUT	(0x10000042)           /* 未找到人脸关联输出 */
#define DEMO_ERR_NOT_FIND_HKP_OUT	(0x10000043)           /* 未找到HKP输出 */
#define DEMO_ERR_NOT_FIND_OUT		(0x10000044)           /* 未找到输出 */
#define DEMO_ERR_RESULT_INVALID		(0x10000050)           /* 结果不符合预期 */

/* 引擎demo外部输入的帧数据路径 */
#define DEMO_LEFT_VIEW_PATH		("./mtme/input_data/zuotri_view_w768_h432.nv21")
#define DEMO_RIGHT_VIEW_PATH	("./mtme/input_data/youtri_view_w768_h432.nv21")
#define DEMO_MIDDLE_VIEW_PATH	("./mtme/input_data/src_dpt_w1280_h720.nv21")
#define DEMO_FACE_VIEW_PATH		("./mtme/input_data/face_orignal.mp4_1920_1080.nv21")
#define DEMO_X_DATA_VIEW_PATH	("./mtme/input_data/xsi_src.mp4_w1280_h720.nv21")

/* #define PPM_PROC_VPSS_GRP_ID	(5) */
#define PPM_JPEG_CB_NUM	(1)

#define PPM_VDEC_CHN_TRI_CAM	(11)      /* 三目相机第四码流对应的解码通道号，临时和应用约定IPC_1为三目相机 */
#define PPM_VDEC_CHN_FACE_CAM	(10)      /* 人脸相机主码流对应的解码通道号，人脸抓拍相机为IPC_2 */
#define PPM_VDEC_CHN_CALIB_JDEC (12)      /* 标定图片解码通道号 */

#define PPM_XSI_CHN_0_VPSS_GRP	(1)      /* 采集通道0对应的VPSS通道组 */
#define PPM_XSI_CHN_0_VPSS_CHN	(2)
#define PPM_XSI_CHN_1_VPSS_GRP	(3)      /* 采集通道1对应的VPSS通道组 */
#define PPM_XSI_CHN_1_VPSS_CHN	(2)

#define PPM_XSI_SEC_CHN_BUF_NUM (128)

/* 生成算法配置文件相关 */
#define PPM_ALG_CFG_LR_FILE_PATH		("./mtme/param_cfg/calib_export_DS-2XD8826HRD-R20200409AACHE32771669.txt") /* ("/home/config/") */
#define PPM_ALG_CFG_MR_FILE_PATH		("./mtme/param_cfg/mr_calib_export_DS-2XD8826HRD-R20200409AACHE32771669.txt") /* ("/home/config/") */
#define PPM_ALG_CFG_COM_LABEL_NUM		(11)       /* 双目相机通用标记个数 */
#define PPM_DOUBLE_CAM_CALIB_LEBEL_NUM	(3)   /* 相机标定标记个数 */
#define PPM_ALG_CFG_LEFT_LABEL_IDX		(0)
#define PPM_ALG_CFG_RIGHT_LABEL_IDX		(4)
#define PPM_ALG_CFG_COM_ROT_LABEL_IDX	(8)
#define PPM_ALG_CFG_COM_TRANS_LABEL_IDX (9)
#define PPM_ALG_CFG_COM_PROJ_LABEL_IDX	(10)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _PPM_ALG_CFG_INFO_
{
    CHAR tag[64];
    PPM_MATRIX_PRM_S stMatrix;
} PPM_ALG_CFG_INFO_S;

typedef enum _PPM_VIDEO_FRM_PTS_OP_
{
    PPM_FILL_PTS_OP = 0,
    PPM_NOT_FILL_PTS_OP,
    PPM_PTS_OP_NUM,
} PPM_VIDEO_FRM_PTS_OP_E;

typedef struct _PPM_CHN_PRM_
{
    UINT32 uiVpssChn;                              /* VPSS通道 */

    UINT32 uiCropX;                                /* 起始点X, 左右中三路视角需要对全景进行抠图处理 */
    UINT32 uiCropY;                                /* 起始点Y, 左右中三路视角需要对全景进行抠图处理 */

    UINT32 uiOutChnW;                              /* 输出通道宽 */
    UINT32 uiOutChnH;                              /* 输出通道高 */
} PPM_OUT_CHN_PRM;

typedef struct _PPM_JPEG_PRM_
{
    CHAR *pcJpegAddr;
    UINT32 uiJpegSize;
} PPM_JPEG_PRM;

typedef struct _PPM_BUF_INFO_
{
    BOOL bUsed;                                /* 使用标志位 */
    UINT32 uiIdx;                              /* buf索引号 */
    SYSTEM_FRAME_INFO stFrameInfo;             /* 帧数据 */
} PPM_BUF_INFO_S;

typedef struct _PPM_BUF_PRM_
{
    UINT32 uiIdx;                                                 /* 遍历索引 */

    UINT32 uiBufCnt;                                              /* 申请的缓存个数 */
    PPM_BUF_INFO_S stXsiSecChnYuvBuf[PPM_XSI_SEC_CHN_BUF_NUM];    /* 第二路X光数据buf内存 */
} PPM_BUF_PRM_S;

typedef struct _PPM_DEBUG_PRM_
{
    UINT32 uiIptCnt;                           /* 送入引擎数据次数 */
    UINT32 uiCbCnt;                            /* 引擎回调结果次数 */
    SAL_ThrHndl stDumpSyncDataHdl;             /* dump同步帧线程句柄 */
} PPM_DEBUG_PRM_S;

typedef enum _PPM_JDEC_PROC_MODE_
{
    PPM_JDEC_HW_MODE = 0,
    PPM_JDEC_SOFT_MODE = 1,
    PPM_JDEC_MODE_NUM,
} PPM_JDEC_PROC_MODE_E;

typedef enum _PPM_FRAME_MODE_
{
    PPM_TOP_FID_MODE = 0,               /* Top field. */
    PPM_BOTTOM_FID_MODE	= 1,            /* Bottom field. */
    PPM_PROGRESSIVE_FRAME_MODE = 2,     /* Progressive frame*/
    PPM_INTERLACED_FRAME_MODE = 3,      /* Interlaced frame */
    PPM_SEPARATE_FRAME_MODE	= 4,        /* Separete frame */
    PPM_FRAME_MODE_BUTT,
} PPM_FRAME_MODE_E;

/* YUV图像的信息扩展 */
typedef struct _PPM_YUV_FRAME_EX_
{
    UINT32 width;
    UINT32 height;
    UINT32 pitchY;
    UINT32 pitchUv;
    PUINT8 yTopAddr;
    PUINT8 yBotAddr;
    PUINT8 uTopAddr;
    PUINT8 uBotAddr;
    PUINT8 vTopAddr;
    PUINT8 vBotAddr;
    PPM_FRAME_MODE_E frameMode;
} PPM_YUV_FRAME_EX_S;

typedef enum _PPM_ALG_CFG_CAM_TYPE_
{
    TRI_FIRST_CAM = 0,
    TRI_SECOND_CAM,
    TRI_CALIB_CAM_NUM,
} PPM_ALG_CFG_CAM_TYPE_E;

typedef struct _PPM_JDEC_PROC_OUT_
{
    SYSTEM_FRAME_INFO *pstSysFrame;              /* 帧数据 */
    PPM_JDEC_PROC_MODE_E enProcMode;           /* jpeg解码方式，硬解or软解 */
} PPM_JDEC_PROC_OUT_S;

typedef struct _PPM_JDEC_PROC_IN_
{
    INT8 *pcJpegAddr;                          /* 图片地址 */
    UINT32 uiJpegLen;                          /* 图片长度 */
} PPM_JDEC_PROC_IN_S;

typedef struct _PPM_JDEC_PRM_
{
    UINT8 bDecChnStatus;                                       /* 编码通道是否已创建 */
    UINT32 uiJdecChn;                                          /* jpeg解码通道 */
    DupHandle hDupFront;                                       /* 解码通道对应的front dup通道 */
    DupHandle hDupRear;                                        /* 解码通道对应的rear dup通道 */

    PPM_JDEC_PROC_MODE_E enFaceProcMode[PPM_MAX_CALIB_JPEG_NUM];
    PPM_JPEG_PRM stFaceSysFrame[PPM_MAX_CALIB_JPEG_NUM];       /* 人脸相机解码帧buf */

    PPM_JDEC_PROC_MODE_E enTriProcMode[PPM_MAX_CALIB_JPEG_NUM];
    PPM_JPEG_PRM stTriSysFrame[PPM_MAX_CALIB_JPEG_NUM];        /* 三目相机解码帧buf */

    PPM_JDEC_PROC_MODE_E enFaceCamProcMode[PPM_MAX_FACE_CALIB_JPEG_NUM];
    SYSTEM_FRAME_INFO stFaceCamSysFrame[PPM_MAX_FACE_CALIB_JPEG_NUM];   /* 人脸相机解码帧buf */
} PPM_JDEC_PRM_S;

typedef struct _PPM_DEV_PRM_
{
    UINT32 uiChn;
    BOOL bChnOpen;                             /* 通道启用标志 */
    BOOL bOutChnInit;                          /* 输出通道初始化标志 */
    BOOL bNeedSync;                            /* 同步标志位 */

    SAL_ThrHndl stSyncThrHdl;                  /* 同步线程句柄 */
    SAL_ThrHndl stProcThrHdl;                  /* 处理线程句柄 */
    SAL_ThrHndl stIptThrHdl;                   /* 推帧线程句柄 */
    SAL_ThrHndl stJencThrHdl;                  /* 结果编图线程句柄 */

    VOID *pJpegChnInfo;
    PPM_JPEG_PRM stPkgJpegInfo[PPM_JPEG_CB_NUM];
    PPM_JPEG_PRM stXpkgJpegInfo[PPM_JPEG_CB_NUM];
    PPM_JPEG_PRM stFaceJpegInfo[PPM_JPEG_CB_NUM];

    PPM_JDEC_PRM_S stJdecPrm;                   /* jpeg解码yuv的参数 */

    SYSTEM_FRAME_INFO stTmpFrame[2];                          /* 临时帧数据结构体 */
    SYSTEM_FRAME_INFO stVdecWholeFrame;                       /* debug: 三目相机第四码流帧数据，YUV */

    VOID *paDupChnHandle[PPM_HANDLE_MAX_NUM];                 /* 三目相机分割通道句柄 */
    SYSTEM_FRAME_INFO stVdecFrame[PPM_TRI_CAM_OUT_CHN_NUM];   /* 三目相机左右中三路帧数据，YUV */

    PPM_YUV_QUE_PRM stYuvQueInfo;                             /* 待编码的YUV数据队列 */
    PPM_INPUT_DATA_QUE_PRM stIptQueInfo;                      /* 引擎送帧数据队列 */
    PPM_RSLT_QUE_PRM stRsltQueInfo;                           /* 结果数据队列 */

    /* PPM_SYNC_V1: 人包关联同步方案V1版本使用 */
    SAL_ThrHndl stTriThrHdl;                                  /* 三目相机取帧线程句柄 */
    SAL_ThrHndl stFaceThrHdl;                                 /* 人脸相机取帧线程句柄 */
    SAL_ThrHndl stXsiThrHdl;                                  /* X光包裹取帧线程句柄 */

    BOOL bTriFrmClr;                                          /* 清空三目相机队列数据 */
    BOOL bFaceFrmClr;                                         /* 清空人脸相机队列数据 */
    BOOL bXsiFrmClr;                                          /* 清空X光包裹队列数据 */

    PPM_TRI_YUV_QUE_PRM stTriYuvQueInfo;                      /* 三目相机yuv数据队列 */
    PPM_FACE_YUV_QUE_PRM stFaceYuvQueInfo;                    /* 人脸相机yuv数据队列 */
    PPM_XSI_YUV_QUE_PRM stXsiYuvQueInfo;                      /* X光包裹yuv数据队列 */

    PPM_BUF_PRM_S stXsiSecBufPrm;                             /* 第二路X光帧数据队列 */

	/* 三目深度相机返回的深度信息队列 */
	PPM_DEPTH_INFO_QUE_PRM stDepthInfoQueInfo;

	/* RK人包深度目标信息 */
	MTME_OBJ_INPUT_INFO_T stMtmeInputObjInfo;

    PPM_POS_INFO_S stPosInfo;                                 /* pos信息，用于转封装叠加智能帧 */
    PPM_DEBUG_PRM_S stDebugPrm;                               /* debug参数信息，目前仅有输入和回调计数 */
} PPM_DEV_PRM;

typedef struct _PPM_COM_PRM_
{
    BOOL bInit;                                    /* 模块初始化状态 */

    UINT32 uiChannelCnt;                           /* 启用的通道个数，该通道为模块逻辑通道，目前与引擎子模块一致 */
    PPM_DEV_PRM stPpmDevPrm[MAX_PPM_CHN_NUM];     /* 人包关联通道参数 */
} PPM_COM_PRM;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static PPM_COM_PRM g_stPpmCommPrm = {0};

extern INST_CFG_S stPpmVdecDupInstCfg;

/*
    三目相机多路码流输出参数，从左到右依次是输出{通道号，数据宽，数据高} 。
    注意: 当前仅全景图为物理通道输出，其他三路视角均是经过物理通道裁剪和拓展通道放缩后得到的。
 */
static PPM_OUT_CHN_PRM g_stPpmOutChnPrm[PPM_TRI_CAM_OUT_CHN_NUM] =
{
    /* 第四码流左视角 */
    {PPM_TRI_CAM_LEFT_CHN, 0, 0, PPM_TRI_CAM_LEFT_CHN_WIDTH, PPM_TRI_CAM_LEFT_CHN_HEIGHT},

    /* 第四码流右视角 */
    {PPM_TRI_CAM_RIGHT_CHN, PPM_TRI_CAM_VPSS_CROP_W, 0, PPM_TRI_CAM_RIGHT_CHN_WIDTH, PPM_TRI_CAM_RIGHT_CHN_HEIGHT},

    /* 第四码流中路视角 */
    {PPM_TRI_CAM_MIDDLE_CHN, 0, PPM_TRI_CAM_VPSS_CROP_H, PPM_RK_TRI_CAM_MIDDLE_CHN_WIDTH, PPM_RK_TRI_CAM_MIDDLE_CHN_HEIGHT},

    /* 第四码流的全景图 */
    /* {PPM_TRI_CAM_VPSS_CHN, 0, 0, PPM_TRI_CAM_CHN_WIDTH, PPM_TRI_CAM_CHN_HEIGHT}, */
};

static UINT32 uiDbgSwitchFlag = SAL_FALSE;              /* 输入输出计数使用 */
static CHAR TriCamComLabel[PPM_ALG_CFG_COM_LABEL_NUM][32] =
{
    "M_l:", "D_l:", "R_l:", "P_l:", "M_r:", "D_r:", "R_r:", "P_r:", "R:", "T:", "Q:"
};
static CHAR TriCamPrvtLabel[TRI_CAM_CALIB_PARAM_NUM][PPM_DOUBLE_CAM_CALIB_LEBEL_NUM][32] =
{
    {"focalLenth:", "OriImgWidth:", "OriImgHeight:"},
    {"rms:", "eame:", ""},
};

static UINT32 uiPpmModuleInitFlag = SAL_FALSE;

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Ppm_DrvSetDbgSwitch
 * @brief:      设置调试等级，目前仅用于打印数据个数定位泄露
 * @param[in]:  UINT32 flag
 * @param[out]: None
 * @return:     VOID
 */
VOID Ppm_DrvSetDbgSwitch(UINT32 flag)
{
    uiDbgSwitchFlag = flag;
    return;
}

/**
 * @function:   Ppm_DrvGetDevPrm
 * @brief:      获取通道参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static PPM_DEV_PRM *
 */
static PPM_DEV_PRM *Ppm_DrvGetDevPrm(UINT32 chan)
{
    PPM_DRV_CHECK_MOD_CHAN(chan, NULL);

    return &g_stPpmCommPrm.stPpmDevPrm[chan];
}

/**
 * @function:   Ppm_DrvGetVprocHandle
 * @brief:      保存vproc通道句柄
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvGetVprocHandle(UINT32 chan, UINT32 u32OutChn)
{
    PPM_DEV_PRM *pstDevPrm = NULL;

    if (u32OutChn >= PPM_HANDLE_MAX_NUM)
    {
        SAL_ERROR("invalid out chn %d, chan %d \n", u32OutChn, chan);
        goto err;
    }

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    return pstDevPrm->paDupChnHandle[u32OutChn];
err:
    return NULL;
}

/**
 * @function:   Ppm_DrvSetVprocHandle
 * @brief:      保存vproc通道句柄
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static PPM_DEV_PRM *
 */
INT32 Ppm_DrvSetVprocHandle(UINT32 chan, UINT32 u32OutChn, VOID *pDupChnHandle)
{
    PPM_DEV_PRM *pstDevPrm = NULL;

    if (u32OutChn >= PPM_HANDLE_MAX_NUM)
    {
        SAL_ERROR("invalid out chn %d, chan %d \n", u32OutChn, chan);
        goto err;
    }

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstDevPrm->paDupChnHandle[u32OutChn] = pDupChnHandle;

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetPosInfo
 * @brief:      获取pos信息
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_POS_INFO_S *pstPosInfo
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvGetPosInfo(UINT32 chan, PPM_POS_INFO_S *pstPosInfo)
{
    PPM_DEV_PRM *pstDevPrm = NULL;

    PPM_DRV_CHECK_PTR(pstPosInfo, err, "pstPosInfo == null!");

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    memcpy(pstPosInfo, &pstDevPrm->stPosInfo, sizeof(PPM_POS_INFO_S));

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Ppm_DrvSetCapABRegion
 * @brief      设置抓拍AB区域
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *pPrm   
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvSetCapABRegion(UINT32 chan, VOID *pPrm)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_REGION_INFO_S *pstRegionInfo = NULL;
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_POS_INFO_S *pstPosInfo = NULL;

	MTME_CAPTURE_REGION_T stCapRegionPrm = {0};

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pPrm, exit, "pPrm == null!");

	pstRegionInfo = (PPM_REGION_INFO_S *)pPrm;

	/* checker: AB区域配置需要依次下发两个区域 */
	if (2 != pstRegionInfo->uiSubRgnCnt)
	{
		PPM_LOGE("cap region and sub cnt != 2! chan %d \n", chan);
		return PPM_INVALID_REGION_PARAM;
	}

	/* pos码流中需要标识出抓拍AB区域 */
	{
	    pstDevPrm = Ppm_DrvGetDevPrm(0);
	    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

	    pstPosInfo = &pstDevPrm->stPosInfo;
	}

	/* Capture 节点A */
	{
		stCapRegionPrm.region_a.vertex_num = 4;
		stCapRegionPrm.region_a.point[0].x = pstRegionInfo->stRgnRect[0].x;
		stCapRegionPrm.region_a.point[0].y = pstRegionInfo->stRgnRect[0].y;
		stCapRegionPrm.region_a.point[1].x = pstRegionInfo->stRgnRect[0].x + pstRegionInfo->stRgnRect[0].width;
		stCapRegionPrm.region_a.point[1].y = pstRegionInfo->stRgnRect[0].y;
		stCapRegionPrm.region_a.point[2].x = pstRegionInfo->stRgnRect[0].x;
		stCapRegionPrm.region_a.point[2].y = pstRegionInfo->stRgnRect[0].y + pstRegionInfo->stRgnRect[0].height;
		stCapRegionPrm.region_a.point[3].x = pstRegionInfo->stRgnRect[0].x + pstRegionInfo->stRgnRect[0].width;
		stCapRegionPrm.region_a.point[3].y = pstRegionInfo->stRgnRect[0].y + pstRegionInfo->stRgnRect[0].height;
		
		PPM_LOGW("a-----[%f, %f], [%f, %f], [%f, %f], [%f, %f] \n",
				 stCapRegionPrm.region_a.point[0].x, stCapRegionPrm.region_a.point[0].y,
				 stCapRegionPrm.region_a.point[1].x, stCapRegionPrm.region_a.point[1].y,
				 stCapRegionPrm.region_a.point[2].x, stCapRegionPrm.region_a.point[2].y,
				 stCapRegionPrm.region_a.point[3].x, stCapRegionPrm.region_a.point[3].y);
		
		if ((stCapRegionPrm.region_a.point[0].x <= 1e-6)
			&& (stCapRegionPrm.region_a.point[0].y <= 1e-6)
			&& (stCapRegionPrm.region_a.point[1].x <= 1e-6)
			&& (stCapRegionPrm.region_a.point[1].y <= 1e-6))
		{
			PPM_LOGE("Invalid cap roi a \n");
			return PPM_INVALID_REGION_PARAM;
		}
		
		/* save cap_a region for pos info */
		pstPosInfo->stRuleRgnInfo.stCapARgn.x = pstRegionInfo->stRgnRect[0].x;
		pstPosInfo->stRuleRgnInfo.stCapARgn.y = pstRegionInfo->stRgnRect[0].y;
		pstPosInfo->stRuleRgnInfo.stCapARgn.width = pstRegionInfo->stRgnRect[0].width;
		pstPosInfo->stRuleRgnInfo.stCapARgn.height = pstRegionInfo->stRgnRect[0].height;
	}
	
	/* Capture 节点B */
	{
		stCapRegionPrm.region_b.vertex_num = 4;
		stCapRegionPrm.region_b.point[0].x = pstRegionInfo->stRgnRect[1].x;
		stCapRegionPrm.region_b.point[0].y = pstRegionInfo->stRgnRect[1].y;
		stCapRegionPrm.region_b.point[1].x = pstRegionInfo->stRgnRect[1].x + pstRegionInfo->stRgnRect[1].width;
		stCapRegionPrm.region_b.point[1].y = pstRegionInfo->stRgnRect[1].y;
		stCapRegionPrm.region_b.point[2].x = pstRegionInfo->stRgnRect[1].x;
		stCapRegionPrm.region_b.point[2].y = pstRegionInfo->stRgnRect[1].y + pstRegionInfo->stRgnRect[1].height;
		stCapRegionPrm.region_b.point[3].x = pstRegionInfo->stRgnRect[1].x + pstRegionInfo->stRgnRect[1].width;
		stCapRegionPrm.region_b.point[3].y = pstRegionInfo->stRgnRect[1].y + pstRegionInfo->stRgnRect[1].height;
		
		if ((stCapRegionPrm.region_b.point[0].x <= 1e-6)
			&& (stCapRegionPrm.region_b.point[0].y <= 1e-6)
			&& (stCapRegionPrm.region_b.point[1].x <= 1e-6)
			&& (stCapRegionPrm.region_b.point[1].y <= 1e-6))
		{
			PPM_LOGE("Invalid cap roi b \n");
			return PPM_INVALID_REGION_PARAM;
		}
		
		PPM_LOGW("b-----[%f, %f], [%f, %f], [%f, %f], [%f, %f] \n",
				 stCapRegionPrm.region_b.point[0].x, stCapRegionPrm.region_b.point[0].y,
				 stCapRegionPrm.region_b.point[1].x, stCapRegionPrm.region_b.point[1].y,
				 stCapRegionPrm.region_b.point[2].x, stCapRegionPrm.region_b.point[2].y,
				 stCapRegionPrm.region_b.point[3].x, stCapRegionPrm.region_b.point[3].y);
		
		/* save cap_b region for pos info */
		pstPosInfo->stRuleRgnInfo.stCapBRgn.x = pstRegionInfo->stRgnRect[1].x;
		pstPosInfo->stRuleRgnInfo.stCapBRgn.y = pstRegionInfo->stRgnRect[1].y;
		pstPosInfo->stRuleRgnInfo.stCapBRgn.width = pstRegionInfo->stRgnRect[1].width;
		pstPosInfo->stRuleRgnInfo.stCapBRgn.height = pstRegionInfo->stRgnRect[1].height;
	}
	
	s32Ret = Ppm_HalSetCapRegion(&stCapRegionPrm);
	PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_HalSetCapRegion failed!");
		
exit:
	return s32Ret;
}

/**
 * @function   Ppm_DrvSetFaceRoi
 * @brief      设置人脸ROI区域
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *pPrm   
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvSetFaceRoi(UINT32 chan, VOID *pPrm)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_REGION_INFO_S *pstRegionInfo = NULL;
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_POS_INFO_S *pstPosInfo = NULL;

	MTME_RECT_T stFaceRoi = {0};

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pPrm, exit, "pPrm == null!");

	pstRegionInfo = (PPM_REGION_INFO_S *)pPrm;
	if (1 != pstRegionInfo->uiSubRgnCnt)
	{
		PPM_LOGE("Invalid sub rgn cnt %d, chan %d \n", pstRegionInfo->uiSubRgnCnt, chan);
		return SAL_FAIL;
	}

	stFaceRoi.x = pstRegionInfo->stRgnRect[0].x;
	stFaceRoi.y = pstRegionInfo->stRgnRect[0].y;
	stFaceRoi.width = pstRegionInfo->stRgnRect[0].width;
	stFaceRoi.height = pstRegionInfo->stRgnRect[0].height;
	
	if (stFaceRoi.width <= 1e-6 || stFaceRoi.height <= 1e-6)
	{
		PPM_LOGE("Invalid face roi info, w %f, h %f \n", stFaceRoi.width, stFaceRoi.height);
		return PPM_INVALID_REGION_PARAM;
	}

	/* pos码流中需要标识出人脸ROI区域 */
	{
	    pstDevPrm = Ppm_DrvGetDevPrm(0);
	    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

	    pstPosInfo = &pstDevPrm->stPosInfo;

		pstPosInfo->stRuleRgnInfo.stFaceRgn.x = stFaceRoi.x;
		pstPosInfo->stRuleRgnInfo.stFaceRgn.y = stFaceRoi.y;
		pstPosInfo->stRuleRgnInfo.stFaceRgn.width = stFaceRoi.width;
		pstPosInfo->stRuleRgnInfo.stFaceRgn.height = stFaceRoi.height;
	}
	
	s32Ret = Ppm_HalSetMatchFaceRoi(&stFaceRoi);
	PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_HalSetMatchPackRoi failed!");
	
	PPM_LOGW("set face roi: [%f, %f], [%f, %f] \n", 
		     stFaceRoi.x, stFaceRoi.y, stFaceRoi.width, stFaceRoi.height);
		
exit:
	return s32Ret;
}

/**
 * @function   Ppm_DrvSetPkgCapRegion
 * @brief      设置可见光抓拍区域
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *pPrm   
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvSetPkgCapRegion(UINT32 chan, VOID *pPrm)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_REGION_INFO_S *pstRegionInfo = NULL;
	MTME_RECT_T stPackRoiInfo = {0};

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pPrm, exit, "pPrm == null!");

	pstRegionInfo = (PPM_REGION_INFO_S *)pPrm;

	if (1 != pstRegionInfo->uiSubRgnCnt)
	{
		PPM_LOGE("Invalid sub rgn cnt %d, chan %d \n", pstRegionInfo->uiSubRgnCnt, chan);
		return SAL_FAIL;
	}
	
	stPackRoiInfo.x = pstRegionInfo->stRgnRect[0].x;
	stPackRoiInfo.y = pstRegionInfo->stRgnRect[0].y;
	stPackRoiInfo.width = pstRegionInfo->stRgnRect[0].width;
	stPackRoiInfo.height = pstRegionInfo->stRgnRect[0].height;
	
	if (stPackRoiInfo.width <= 1e-6 || stPackRoiInfo.height <= 1e-6)
	{
		PPM_LOGE("Invalid pack roi info, w %f, h %f \n", stPackRoiInfo.width, stPackRoiInfo.height);
		return PPM_INVALID_REGION_PARAM;
	}
	
	s32Ret = Ppm_HalSetMatchPackRoi(&stPackRoiInfo);
	PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_HalSetMatchPackRoi failed!");
	
	PPM_LOGI("set pkg capture region: [%f, %f], [%f, %f] \n", 
		     stPackRoiInfo.x, stPackRoiInfo.y, stPackRoiInfo.width, stPackRoiInfo.height);
	
exit:
	return s32Ret;
}

/**
 * @function:   Ppm_DrvSetXsiMatchPrm
 * @brief:      配置xsi匹配参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetXsiMatchPrm(UINT32 chan, void *pPrm)
{
#if 0
    INT32 s32Ret = SAL_SOK;

    PPM_MOD_S *pstModPrm = NULL;
    PPM_SUB_MOD_S *pstSubModPrm = NULL;
    PPM_XSI_MATCH_PRM_S *pstXsiMatchPrm = NULL;

    MTME_XSI_MATCH_PARM_T stXsiPrm = {0};

    PPM_DRV_CHECK_SUB_MOD_ID(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pPrm, err, "pPrm == null!");

    pstXsiMatchPrm = (PPM_XSI_MATCH_PRM_S *)pPrm;

    if (pstXsiMatchPrm->fBeltMovSpeed <= 1e-6 || pstXsiMatchPrm->fBeltCenterDist <= 1e-6)
    {
        PPM_LOGE("invalid prm [%f, %f], chan %d \n", pstXsiMatchPrm->fBeltMovSpeed, pstXsiMatchPrm->fBeltCenterDist, chan);
        goto err;
    }

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, err, "pstModPrm == null!");

    pstSubModPrm = Ppm_HalGetSubModPrm(chan);
    PPM_DRV_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    if (!pstSubModPrm->uiUseFlag)
    {
        PPM_LOGE("sub mod %d is not used! \n", chan);
        goto err;
    }

    PPM_DRV_CHECK_PTR(pstSubModPrm->pHandle, err, "pHandle == null!");

    stXsiPrm.band_speed = pstXsiMatchPrm->fBeltMovSpeed;     /* m/s */
    stXsiPrm.center_dis = pstXsiMatchPrm->fBeltCenterDist;   /* m */

    s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstSubModPrm->pHandle, MTME_MATCH_XSI_NODE_ID, MTME_SET_XSI_MATCH_PARAM, &stXsiPrm, sizeof(stXsiPrm));
    PPM_DRV_CHECK_RET(s32Ret, err, "ICF_Set_config MTME_SET_XSI_MATCH_PARAM failed!");

    SAL_INFO("set xsi match prm end! speed %f, center %f, chan %d \n", stXsiPrm.band_speed, stXsiPrm.center_dis, chan);
    return SAL_SOK;
err:
    return SAL_FAIL;
#else
	PPM_LOGW("xsi match prm not support! return success! \n");
	return SAL_SOK;
#endif
}

/**
 * @function:   Ppm_DrvSetDetSensity
 * @brief:      匹配检测灵敏度配置
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetDetSensity(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MATCH_SENSITY_E enMatchSensity = 0;
    MTME_PACK_MATCH_SENSITIVITY_E enEngUsedSensity = 0;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pPrm, exit, "pPrm == null!");
	PPM_DRV_CHECK_RET(SAL_TRUE != Ppm_HalCheckSubModSts(PPM_SUB_MOD_MATCH), exit, "sub mod not init");

    enMatchSensity = *(PPM_MATCH_SENSITY_E *)pPrm;
    if (enMatchSensity >= SENSITY_LEVEL_NUM)
    {
        SAL_ERROR("invalid match sensity %d, chan %d \n", enMatchSensity, chan);
        return SAL_FAIL;
    }

    enEngUsedSensity = enMatchSensity + 1;     /* 引擎内部从1开始计数 */

	s32Ret = Ppm_HalSetMatchPackSensity(enEngUsedSensity);
    PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_HalSetMatchPackSensity failed!");

    SAL_INFO("set detect sensity[%d] end! chan %d \n", enMatchSensity, chan);
exit:
    return s32Ret;
}

/**
 * @function:   Ppm_DrvSetFaceMatchThresh
 * @brief:      设置人脸iou阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetFaceMatchThresh(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_FAIL;

    float fIouVal = 0.0;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pPrm, exit, "pPrm == null!");
	PPM_DRV_CHECK_RET(SAL_TRUE != Ppm_HalCheckSubModSts(PPM_SUB_MOD_MATCH), exit, "sub mod not init");

    fIouVal = *(float *)pPrm;
    PPM_LOGW("from app: fIouVal %f \n", fIouVal);

    if (fIouVal <= 1e-6)
    {
        PPM_LOGE("invalid fIouVal %f, chan %d \n", fIouVal, chan);
        return SAL_FAIL;
    }

	s32Ret = Ppm_HalSetMatchFaceIouThresh(fIouVal);
    PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_HalSetMatchFaceIouThresh failed!");

    PPM_LOGI("set face match threshhold value %f end! chan %d \n", fIouVal, chan);
exit:
    return s32Ret;
}

/**
 * @function    Ppm_DrvPrintfMatrix
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Ppm_DrvPrintfMatrix(PPM_MATRIX_PRM_S *pstMatrixInfo, char *name)
{
    PPM_LOGI("%s matrix Info: \n", name);

    if (NULL == pstMatrixInfo)
    {
        PPM_LOGE("pstMatrixInfo == null! \n");
        return SAL_FAIL;
    }

    if (pstMatrixInfo->uiRowCnt > PPM_MAX_ARRAY_SIZE || pstMatrixInfo->uiColCnt > PPM_MAX_ARRAY_SIZE)
    {
        PPM_LOGE("error rowcnt %d, colcnt %d ", pstMatrixInfo->uiRowCnt, pstMatrixInfo->uiColCnt);
        return SAL_FAIL;
    }

    PPM_LOGI("[%f, %f, %f, %f, %f, %f, %f, %f] \n[%f, %f, %f, %f, %f, %f, %f, %f] \n\[%f, %f, %f, %f, %f, %f, %f, %f] \n\[%f, %f, %f, %f, %f, %f, %f, %f] \n\
              [%f, %f, %f, %f, %f, %f, %f, %f] \n\[%f, %f, %f, %f, %f, %f, %f, %f] \n\[%f, %f, %f, %f, %f, %f, %f, %f] \n\[%f, %f, %f, %f, %f, %f, %f, %f] \n",
             pstMatrixInfo->fMatrix[0][0], pstMatrixInfo->fMatrix[0][1], pstMatrixInfo->fMatrix[0][2], pstMatrixInfo->fMatrix[0][3],
             pstMatrixInfo->fMatrix[0][4], pstMatrixInfo->fMatrix[0][5], pstMatrixInfo->fMatrix[0][6], pstMatrixInfo->fMatrix[0][7],
             pstMatrixInfo->fMatrix[1][0], pstMatrixInfo->fMatrix[1][1], pstMatrixInfo->fMatrix[1][2], pstMatrixInfo->fMatrix[1][3],
             pstMatrixInfo->fMatrix[1][4], pstMatrixInfo->fMatrix[1][5], pstMatrixInfo->fMatrix[1][6], pstMatrixInfo->fMatrix[1][7],
             pstMatrixInfo->fMatrix[2][0], pstMatrixInfo->fMatrix[2][1], pstMatrixInfo->fMatrix[2][2], pstMatrixInfo->fMatrix[2][3],
             pstMatrixInfo->fMatrix[2][4], pstMatrixInfo->fMatrix[2][5], pstMatrixInfo->fMatrix[2][6], pstMatrixInfo->fMatrix[2][7],
             pstMatrixInfo->fMatrix[3][0], pstMatrixInfo->fMatrix[3][1], pstMatrixInfo->fMatrix[3][2], pstMatrixInfo->fMatrix[3][3],
             pstMatrixInfo->fMatrix[3][4], pstMatrixInfo->fMatrix[3][5], pstMatrixInfo->fMatrix[3][6], pstMatrixInfo->fMatrix[3][7],
             pstMatrixInfo->fMatrix[4][0], pstMatrixInfo->fMatrix[4][1], pstMatrixInfo->fMatrix[4][2], pstMatrixInfo->fMatrix[4][3],
             pstMatrixInfo->fMatrix[4][4], pstMatrixInfo->fMatrix[4][5], pstMatrixInfo->fMatrix[4][6], pstMatrixInfo->fMatrix[4][7],
             pstMatrixInfo->fMatrix[5][0], pstMatrixInfo->fMatrix[5][1], pstMatrixInfo->fMatrix[5][2], pstMatrixInfo->fMatrix[5][3],
             pstMatrixInfo->fMatrix[5][4], pstMatrixInfo->fMatrix[5][5], pstMatrixInfo->fMatrix[5][6], pstMatrixInfo->fMatrix[5][7],
             pstMatrixInfo->fMatrix[6][0], pstMatrixInfo->fMatrix[6][1], pstMatrixInfo->fMatrix[6][2], pstMatrixInfo->fMatrix[6][3],
             pstMatrixInfo->fMatrix[6][4], pstMatrixInfo->fMatrix[6][5], pstMatrixInfo->fMatrix[6][6], pstMatrixInfo->fMatrix[6][7],
             pstMatrixInfo->fMatrix[7][0], pstMatrixInfo->fMatrix[7][1], pstMatrixInfo->fMatrix[7][2], pstMatrixInfo->fMatrix[7][3],
             pstMatrixInfo->fMatrix[7][4], pstMatrixInfo->fMatrix[7][5], pstMatrixInfo->fMatrix[7][6], pstMatrixInfo->fMatrix[7][7]);

    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvSetMatchMatrixPrm
 * @brief:      配置匹配矩阵参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetMatchMatrixPrm(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_SOK;
    PPM_MATCH_MATRIX_INFO_S *pstMatchMatrixInfo = NULL;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_RET(NULL == pPrm, err, "pPrm == null!");

    pstMatchMatrixInfo = (PPM_MATCH_MATRIX_INFO_S *)pPrm;
    s32Ret = Ppm_DrvPrintfMatrix(&pstMatchMatrixInfo->stFaceInterMatrixInfo, "face inter");
    PPM_DRV_CHECK_RET(s32Ret, err, "face inter matrixInfo is NULL !!!");
    s32Ret = Ppm_DrvPrintfMatrix(&pstMatchMatrixInfo->stTriCamInvMatrixInfo, "tri inter");
    PPM_DRV_CHECK_RET(s32Ret, err, "tri inter matrixInfo is NULL !!!");
    s32Ret = Ppm_DrvPrintfMatrix(&pstMatchMatrixInfo->stR_MatrixInfo, "R");
    PPM_DRV_CHECK_RET(s32Ret, err, "R matrix is NULL !!!");
    s32Ret = Ppm_DrvPrintfMatrix(&pstMatchMatrixInfo->stT_MatrixInfo, "T");
    PPM_DRV_CHECK_RET(s32Ret, err, "T matrix is NULL !!!");

    if (pstMatchMatrixInfo->fCamHeight <= 1e-6)
    {
        PPM_LOGE("cam height error \n");
        return SAL_FAIL;
    }

    s32Ret = Ppm_HalSetMatchMatrixPrm(chan, pstMatchMatrixInfo);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_HalSetMatchMatrixPrm  failed!");

    PPM_LOGI("cam height: %f \n", pstMatchMatrixInfo->fCamHeight);
    PPM_LOGI("set match matrix end! chan %d \n", chan);

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvSetFaceScoreFilter
 * @brief:      设置人脸评分阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetFaceScoreFilter(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_FAIL;

    float fScore = 0.0;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_RET(NULL == pPrm, exit, "pPrm == null!");
	PPM_DRV_CHECK_RET(SAL_TRUE != Ppm_HalCheckSubModSts(PPM_SUB_MOD_MATCH), exit, "sub mod not init");

    fScore = *(float *)pPrm;
    PPM_LOGW("from app: fScore %f \n", fScore);

    if (fScore <= 1e-6)
    {
        PPM_LOGE("invalid fScore %f, chan %d \n", fScore, chan);
        return SAL_FAIL;
    }

	s32Ret = Ppm_HalSetMatchFaceScoreFilter(fScore);
    PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_HalSetMatchFaceScoreFilter failed!");

    PPM_LOGI("set face score filter %f end! chan %d \n", fScore, chan);
exit:
    return s32Ret;
}

/**
 * @function:   Ppm_DrvAddCharToAlgCfgFile
 * @brief:      向指定文件写入字符（当前用于增加空格和换行）
 * @param[in]:  FILE *fp
 * @param[in]:  CHAR c
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvAddCharToAlgCfgFile(FILE *fp, CHAR c)
{
    INT32 r = SAL_SOK;

    r = fprintf(fp, "%c", c);
    PPM_DRV_CHECK_RET(r < 0, err, "fprintf failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvAddStringToAlgCfgFile
 * @brief:      向指定文件写入字符串
 * @param[in]:  FILE *fp
 * @param[in]:  CHAR *str
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvAddStringToAlgCfgFile(FILE *fp, CHAR *str)
{
    INT32 r = SAL_SOK;

    r = fprintf(fp, "%s\n", str);
    PPM_DRV_CHECK_RET(r < 0, err, "fprintf failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvAddMatrixToAlgCfgFile
 * @brief:      向指定文件写入矩阵参数
 * @param[in]:  FILE *fp
 * @param[in]:  PPM_ALG_CFG_INFO_S *pMatrixCfg
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvAddMatrixToAlgCfgFile(FILE *fp, PPM_ALG_CFG_INFO_S *pMatrixCfg)
{
    INT32 r = SAL_SOK;

    INT32 i = 0;
    INT32 j = 0;
    INT32 row = 0;
    INT32 col = 0;

    PPM_DRV_CHECK_RET(fp == NULL, err, "invalid fp!");
    PPM_DRV_CHECK_RET(pMatrixCfg == NULL, err, "invalid matrix!");

    r = fprintf(fp, "%s\n", pMatrixCfg->tag);
    PPM_DRV_CHECK_RET(r < 0, err, "fprintf failed!");

    row = pMatrixCfg->stMatrix.uiRowCnt;
    col = pMatrixCfg->stMatrix.uiColCnt;

    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            /* use PPM_ALG_CFG_MAX_MATRIX_COL_SIZE, cause PPM_ALG_CFG_INFO_S use this macro to define this 2-d array */
            r = fprintf(fp, "%lf", pMatrixCfg->stMatrix.fMatrix[i][j]);
            PPM_DRV_CHECK_RET(r < 0, err, "fprintf failed!");

            r = Ppm_DrvAddCharToAlgCfgFile(fp, ' ');
            PPM_DRV_CHECK_RET(r != SAL_SOK, err, "add_space failed!");

            if (j == col - 1)
            {
                r = Ppm_DrvAddCharToAlgCfgFile(fp, '\n');
                PPM_DRV_CHECK_RET(r != SAL_SOK, err, "add_change_line failed!");
            }
        }
    }

    r = Ppm_DrvAddCharToAlgCfgFile(fp, '\n');
    PPM_DRV_CHECK_RET(r != SAL_SOK, err, "add_change_line failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvCreateWrtMatrixSt
 * @brief:      组装写文件结构体
 * @param[in]:  CHAR *tag
 * @param[in]:  PPM_MATRIX_PRM_S *pstMatrix
 * @param[in]:  PPM_ALG_CFG_INFO_S *pstOutCfg
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvCreateWrtMatrixSt(CHAR *tag, PPM_MATRIX_PRM_S *pstMatrix, PPM_ALG_CFG_INFO_S *pstOutCfg)
{
    PPM_DRV_CHECK_RET(tag == NULL, err, "tag == null!");
    PPM_DRV_CHECK_RET(pstMatrix == NULL, err, "pstMatrix == null!");
    PPM_DRV_CHECK_RET(pstOutCfg == NULL, err, "pstOutCfg == null!");

    memset(pstOutCfg->tag, '\0', 64);
    sal_memcpy_s(pstOutCfg->tag, 64, tag, 32);
    sal_memcpy_s(&pstOutCfg->stMatrix, sizeof(PPM_MATRIX_PRM_S), pstMatrix, sizeof(PPM_MATRIX_PRM_S));

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvWrtLrPrvtPrm
 * @brief:      写入左右相机标定私有参数
 * @param[in]:  FILE *fp
 * @param[in]:  PPM_ALG_CFG_PRVT_LR_PRM_S *pstPrvtPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvWrtLrPrvtPrm(FILE *fp, PPM_ALG_CFG_PRVT_LR_PRM_S *pstPrvtPrm)
{
    INT32 s32Ret = SAL_SOK;

    CHAR tmpStr[64] = {0};

    PPM_DRV_CHECK_PTR(fp, err, "fp == null!");
    PPM_DRV_CHECK_PTR(pstPrvtPrm, err, "pstPrvtPrm == null!");

    /* 写入相机光圈参数 */
    memset(tmpStr, '\0', 64);
    sprintf(tmpStr, "%s %fmm", TriCamPrvtLabel[TRI_CAM_LR_CALIB_PARAM_TYPE][0], pstPrvtPrm->fFocalLen);
    s32Ret = Ppm_DrvAddStringToAlgCfgFile(fp, tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddStringToAlgCfgFile failed!");

    /* 写入图片宽度 */
    memset(tmpStr, '\0', 64);
    sprintf(tmpStr, "%s %d", TriCamPrvtLabel[TRI_CAM_LR_CALIB_PARAM_TYPE][1], 0);
    s32Ret = Ppm_DrvAddStringToAlgCfgFile(fp, tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddStringToAlgCfgFile failed!");

    /* 写入图片高度 */
    memset(tmpStr, '\0', 64);
    sprintf(tmpStr, "%s %d", TriCamPrvtLabel[TRI_CAM_LR_CALIB_PARAM_TYPE][2], 0);
    s32Ret = Ppm_DrvAddStringToAlgCfgFile(fp, tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddStringToAlgCfgFile failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvWrtMrPrvtPrm
 * @brief:      写入中右相机标定私有参数
 * @param[in]:  FILE *fp
 * @param[in]:  PPM_ALG_CFG_PRVT_MR_PRM_S *pstPrvtPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvWrtMrPrvtPrm(FILE *fp, PPM_ALG_CFG_PRVT_MR_PRM_S *pstPrvtPrm)
{
    INT32 s32Ret = SAL_SOK;

    CHAR tmpStr[64] = {0};

    PPM_DRV_CHECK_PTR(fp, err, "fp == null!");
    PPM_DRV_CHECK_PTR(pstPrvtPrm, err, "pstPrvtPrm == null!");

    /* 写入重投影误差参数 */
    memset(tmpStr, '\0', 64);
    sprintf(tmpStr, "%s %f", TriCamPrvtLabel[TRI_CAM_MR_CALIB_PARAM_TYPE][0], pstPrvtPrm->fRms);
    s32Ret = Ppm_DrvAddStringToAlgCfgFile(fp, tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddStringToAlgCfgFile failed!");

    /* 写入极线对齐误差参数 */
    memset(tmpStr, '\0', 64);
    sprintf(tmpStr, "%s %f", TriCamPrvtLabel[TRI_CAM_MR_CALIB_PARAM_TYPE][1], pstPrvtPrm->fEame);
    s32Ret = Ppm_DrvAddStringToAlgCfgFile(fp, tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddStringToAlgCfgFile failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvWrtDoubleCalibPrm
 * @brief:      写入三目相机中双目标定参数
 * @param[in]:  FILE *fp
 * @param[in]:  PPM_ALG_CFG_PRM_S *pstAlgCfgPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvWrtDoubleCalibPrm(FILE *fp, PPM_ALG_CFG_PRM_S *pstAlgCfgPrm)
{
    INT32 s32Ret = SAL_SOK;

    PPM_ALG_CFG_INFO_S stAlgCfgInfo = {0};

    PPM_DRV_CHECK_PTR(fp, err, "fp == null!");
    PPM_DRV_CHECK_PTR(pstAlgCfgPrm, err, "pstAlgCfgPrm == null!");

    /* 写入左相机到右相机的旋转矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[PPM_ALG_CFG_COM_ROT_LABEL_IDX], &pstAlgCfgPrm->stL2RCamRotMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    /* 写入右相机到左相机的转换矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[PPM_ALG_CFG_COM_TRANS_LABEL_IDX], &pstAlgCfgPrm->stL2RCamTransMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    /* 写入视差点的投影矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[PPM_ALG_CFG_COM_PROJ_LABEL_IDX], &pstAlgCfgPrm->stL2RCamProjectMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvWrtSingleCamParam
 * @brief:      写入单个相机参数
 * @param[in]:  FILE *fp
 * @param[in]:  PPM_ALG_CFG_CAM_TYPE_E enCamType
 * @param[in]:  PPM_ALG_CFG_CAM_PRM_S *pstSingleCamPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvWrtSingleCamParam(FILE *fp, PPM_ALG_CFG_CAM_TYPE_E enCamType, PPM_ALG_CFG_CAM_PRM_S *pstSingleCamPrm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiIdx = 0;
    PPM_ALG_CFG_INFO_S stAlgCfgInfo = {0};

    PPM_DRV_CHECK_PTR(fp, err, "fp == null!");
    PPM_DRV_CHECK_PTR(pstSingleCamPrm, err, "pstSingleCamPrm == null!");

    if (enCamType >= TRI_CALIB_CAM_NUM)
    {
        PPM_LOGE("invalid cam type %d \n", enCamType);
        return SAL_FAIL;
    }

    uiIdx = (enCamType == TRI_FIRST_CAM) ? PPM_ALG_CFG_LEFT_LABEL_IDX : PPM_ALG_CFG_RIGHT_LABEL_IDX;

    /* 写入内参矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[0 + uiIdx], &pstSingleCamPrm->stCamInterMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    /* 写入畸变矫正矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[1 + uiIdx], &pstSingleCamPrm->stCamDistortMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    /* 写入单目相机旋转矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[2 + uiIdx], &pstSingleCamPrm->stCamRotateMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    /* 写入单目相机投影矩阵 */
    memset(&stAlgCfgInfo, 0x00, sizeof(PPM_ALG_CFG_INFO_S));
    s32Ret = Ppm_DrvCreateWrtMatrixSt((CHAR *)TriCamComLabel[3 + uiIdx], &pstSingleCamPrm->stCamProjectMatrix, &stAlgCfgInfo);
    s32Ret |= Ppm_DrvAddMatrixToAlgCfgFile(fp, &stAlgCfgInfo);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Ppm_DrvAddMatrixToAlgCfgFile failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGenerateMrCfgFile
 * @brief:      生成mr双目标定文件
 * @param[in]:  PPM_ALG_CFG_PRM_S *pstAlgCfgPrm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_DrvGenerateMrCfgFile(PPM_ALG_CFG_PRM_S *pstAlgCfgPrm)
{
    INT32 s32Ret = SAL_SOK;

    CHAR tmpStr[128] = {0};
    FILE *fp = NULL;

    PPM_DRV_CHECK_PTR(pstAlgCfgPrm, err, "pstAlgCfgPrm == null!");

    memset(tmpStr, '\0', 128);
    s32Ret = sprintf(tmpStr, "hik_rm -rf %s", PPM_ALG_CFG_MR_FILE_PATH);
    PPM_DRV_CHECK_RET(s32Ret <= 0, err, "sprintf failed!");

    s32Ret = system(tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "system failed!");

    fp = fopen(PPM_ALG_CFG_MR_FILE_PATH, "a+");
    PPM_DRV_CHECK_PTR(fp, err, "fopen failed!");

    /* 写入左相机参数 */
    s32Ret = Ppm_DrvWrtSingleCamParam(fp, TRI_FIRST_CAM, &pstAlgCfgPrm->stFirstCamPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtSingleCamParam failed!");

    /* 写入右相机参数 */
    s32Ret = Ppm_DrvWrtSingleCamParam(fp, TRI_SECOND_CAM, &pstAlgCfgPrm->stSecodCamPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtSingleCamParam failed!");

    /* 写入双目标定参数 */
    s32Ret = Ppm_DrvWrtDoubleCalibPrm(fp, pstAlgCfgPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtDoubleCalibPrm failed!");

    /* 写入双目标定私有参数 */
    s32Ret = Ppm_DrvWrtMrPrvtPrm(fp, &pstAlgCfgPrm->stPrvtPrm.stMrPrvtPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtMrPrvtPrm failed!");

    fflush(fp);
    fclose(fp);
    fp = NULL;

    PPM_LOGI("generate mr alg cfg file end! \n");
    return SAL_SOK;
err:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGenerateLrCfgFile
 * @brief:      生成lr双目标定文件
 * @param[in]:  PPM_ALG_CFG_PRM_S *pstAlgCfgPrm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_DrvGenerateLrCfgFile(PPM_ALG_CFG_PRM_S *pstAlgCfgPrm)
{
    INT32 s32Ret = SAL_SOK;

    CHAR tmpStr[128] = {0};
    FILE *fp = NULL;

    PPM_DRV_CHECK_PTR(pstAlgCfgPrm, err, "pstAlgCfgPrm == null!");

    memset(tmpStr, '\0', 128);
    sprintf(tmpStr, "hik_rm -rf %s", PPM_ALG_CFG_LR_FILE_PATH);

    s32Ret = system(tmpStr);
    PPM_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "system failed!");

    fp = fopen(PPM_ALG_CFG_LR_FILE_PATH, "a+");
    PPM_DRV_CHECK_PTR(fp, err, "fopen failed!");

    /* 写入左相机参数 */
    s32Ret = Ppm_DrvWrtSingleCamParam(fp, TRI_FIRST_CAM, &pstAlgCfgPrm->stFirstCamPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtSingleCamParam failed!");

    /* 写入右相机参数 */
    s32Ret = Ppm_DrvWrtSingleCamParam(fp, TRI_SECOND_CAM, &pstAlgCfgPrm->stSecodCamPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtSingleCamParam failed!");

    /* 写入双目标定参数 */
    s32Ret = Ppm_DrvWrtDoubleCalibPrm(fp, pstAlgCfgPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtDoubleCalibPrm failed!");

    /* 写入双目标定私有参数 */
    s32Ret = Ppm_DrvWrtLrPrvtPrm(fp, &pstAlgCfgPrm->stPrvtPrm.stLrPrvtPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvWrtMrPrvtPrm failed!");

    fflush(fp);
    fclose(fp);
    fp = NULL;

    PPM_LOGI("generate lr alg cfg file end! \n");
    return SAL_SOK;
err:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return SAL_FAIL;
}

/**
 * @function    AlgCfgPrintf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Ppm_DrvAlgCfgPrintf(PPM_ALG_CFG_PRM_S *pstAlgCfgPrm)
{
    int s32Ret = 0;

    SAL_WARN("Ppm_DrvAlgCfgPrintf: tri calib type %d (0:lr, 1:mr)\n", pstAlgCfgPrm->enTriCamCalibType);

    SAL_WARN("Firstcamprm: inter rowxcol [%d, %d], distort rowxcol [%d, %d], rotate rowxcol [%d, %d], project rowxcol [%d, %d]\n",
             pstAlgCfgPrm->stFirstCamPrm.stCamInterMatrix.uiRowCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamInterMatrix.uiColCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamDistortMatrix.uiRowCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamDistortMatrix.uiColCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamRotateMatrix.uiRowCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamRotateMatrix.uiColCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamProjectMatrix.uiRowCnt,
             pstAlgCfgPrm->stFirstCamPrm.stCamProjectMatrix.uiColCnt);
    SAL_WARN("Secodcamprm: inter rowxcol [%d, %d], distort rowxcol [%d, %d], rotate rowxcol [%d, %d], project rowxcol [%d, %d]\n",
             pstAlgCfgPrm->stSecodCamPrm.stCamInterMatrix.uiRowCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamInterMatrix.uiColCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamDistortMatrix.uiRowCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamDistortMatrix.uiColCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamRotateMatrix.uiRowCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamRotateMatrix.uiColCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamProjectMatrix.uiRowCnt,
             pstAlgCfgPrm->stSecodCamPrm.stCamProjectMatrix.uiColCnt);
    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stFirstCamPrm.stCamInterMatrix, "face cam inter");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam inter matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stFirstCamPrm.stCamDistortMatrix, "\nface cam distort");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam distort matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stFirstCamPrm.stCamRotateMatrix, "\nface cam rotate");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam rotate matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stFirstCamPrm.stCamProjectMatrix, "\nface cam project");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam project matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stSecodCamPrm.stCamInterMatrix, "\nface cam inter");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam inter matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stSecodCamPrm.stCamDistortMatrix, "\nface cam distort");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam distort matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stSecodCamPrm.stCamRotateMatrix, "\nface cam rotate");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam rotate matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stSecodCamPrm.stCamProjectMatrix, "\nface cam project");
    PPM_DRV_CHECK_RET(s32Ret, err, "face cam project matrix is NULL !!!");

    SAL_WARN("L2R_cam_rot_matrix: row %d, col %d, trans row %d, col %d, project row %d, col %d \n",
             pstAlgCfgPrm->stL2RCamRotMatrix.uiRowCnt, pstAlgCfgPrm->stL2RCamRotMatrix.uiColCnt,
             pstAlgCfgPrm->stL2RCamTransMatrix.uiRowCnt, pstAlgCfgPrm->stL2RCamTransMatrix.uiColCnt,
             pstAlgCfgPrm->stL2RCamProjectMatrix.uiRowCnt, pstAlgCfgPrm->stL2RCamProjectMatrix.uiColCnt);
    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stL2RCamRotMatrix, "L2R cam rot matrix");
    PPM_DRV_CHECK_RET(s32Ret, err, "L2R cam rot matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stL2RCamTransMatrix, "\nL2R cam trans matrix");
    PPM_DRV_CHECK_RET(s32Ret, err, "L2R cam trans matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstAlgCfgPrm->stL2RCamProjectMatrix, "\nL2R cam project matrix");
    PPM_DRV_CHECK_RET(s32Ret, err, "L2R cam project matrix is NULL !!!");

    if (pstAlgCfgPrm->enTriCamCalibType)
    {
        SAL_WARN("rms %f, eame %f \n",
                 pstAlgCfgPrm->stPrvtPrm.stMrPrvtPrm.fRms, pstAlgCfgPrm->stPrvtPrm.stMrPrvtPrm.fEame);
    }
    else
    {
        SAL_WARN("focalLen %f, imgW %d, imgH %d\n",
                 pstAlgCfgPrm->stPrvtPrm.stLrPrvtPrm.fFocalLen,
                 pstAlgCfgPrm->stPrvtPrm.stLrPrvtPrm.uiOriImgW,
                 pstAlgCfgPrm->stPrvtPrm.stLrPrvtPrm.uiOriImgH);
    }

    return SAL_SOK;
err:
    return SAL_FAIL;

}

/**
 * @function:   Ppm_DrvGenerateAlgCfgFile
 * @brief:      生成算法依赖的配置文件(三目mr、lr标定参数)
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvGenerateAlgCfgFile(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    PPM_ALG_CFG_PRM_S *pstAlgCfgPrm = NULL;

    PPM_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstAlgCfgPrm = (PPM_ALG_CFG_PRM_S *)prm;
    s32Ret = Ppm_DrvAlgCfgPrintf(pstAlgCfgPrm);
    if (SAL_SOK != s32Ret)
    {
        PPM_LOGE("invalid cfg calib prm \n");
        goto err;
    }

    switch (pstAlgCfgPrm->enTriCamCalibType)
    {
        case TRI_CAM_LR_CALIB_PARAM_TYPE:
        {
            s32Ret = Ppm_DrvGenerateLrCfgFile(pstAlgCfgPrm);
            break;
        }
        case TRI_CAM_MR_CALIB_PARAM_TYPE:
        {
            s32Ret = Ppm_DrvGenerateMrCfgFile(pstAlgCfgPrm);
            break;
        }
        default:
        {
            s32Ret = SAL_FAIL;
            PPM_LOGW("invalid tri cam calib param type %d \n", pstAlgCfgPrm->enTriCamCalibType);
            break;
        }
    }

    PPM_LOGI("gengrate alg cfg file end! type %d \n", pstAlgCfgPrm->enTriCamCalibType);
    return s32Ret;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetVersion
 * @brief:      获取引擎版本号信息
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvGetVersion(VOID *prm)
{
#if 0
    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    PPM_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstPpmGlobalPrm->pModHandle, err, "global handle is not initialized!");

    if (strlen(pstPpmGlobalPrm->strIcfVer) <= 0)
    {
        PPM_LOGE("no version info \n");
        goto err;
    }

    memcpy((CHAR *)prm, pstPpmGlobalPrm->strIcfVer, PPM_MAX_ICF_VERSION_LEN);
    return SAL_SOK;
err:
    return SAL_FAIL;
#else
	PPM_LOGW("get version not support! return success! \n");
	return SAL_SOK;
#endif
}

/**
 * @function:   Ppm_DrvSetConfTh
 * @brief:      配置置信度
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetConfTh(UINT32 chan, void *pPrm)
{
#if 0
    INT32 s32Ret = SAL_FAIL;

    PPM_CONF_TH_PRM *pstConfThrs  = NULL;
	MTME_OBJ_CONFIDENCE_PARAM_T stObjConfPrm = {0};

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_RET(NULL == pPrm, err, "pPrm == null!");

    pstConfThrs = (PPM_CONF_TH_PRM *)pPrm;

	stObjConfPrm.enable = pstConfThrs->enable;
	stObjConfPrm.filter_threshold = pstConfThrs->fConfTh;

	s32Ret = Ppm_HalSetTrackObjConfThrs(&stObjConfPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_HalSetTrackObjConfThrs failed!");

    PPM_LOGI("chan %d, set confth enable %d, th %f end! \n", chan, pstConfThrs->enable, pstConfThrs->fConfTh);
    return SAL_SOK;

err:
    return SAL_FAIL;
#else
	PPM_LOGW("set conf thresh not support! return success! \n");
	return SAL_SOK;
#endif

}

/**
 * @function:   Ppm_DrvSetPosSwitch
 * @brief:      配置POS信息的开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetPosSwitch(void *pPrm)
{
    UINT32 *uiPosEnableFlag = NULL;
    PPM_DEV_PRM *pstDevPrm = NULL;

    uiPosEnableFlag = (UINT32*)pPrm;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    /* 危险物总开关 */
    if (*uiPosEnableFlag > 1)
    {
        SVA_LOGI("invaild enableflag %d \n", *uiPosEnableFlag);
        *uiPosEnableFlag = 1;
    }
    if (pstDevPrm->stPosInfo.bEnable == *uiPosEnableFlag)
    {
        SVA_LOGI("same pos switch %d \n", *uiPosEnableFlag);
        return SAL_SOK;
    }

    pstDevPrm->stPosInfo.bEnable = *uiPosEnableFlag;

    PPM_LOGW("set pos switch %d ! \n", *uiPosEnableFlag);
    return SAL_SOK;

err:
    return SAL_FAIL;
    
}


/**
 * @function:   Ppm_DrvMemFreeAligned
 * @brief:      释放内存
 * @param[in]:  VOID *pAlignBuf
 * @param[in]:  UINT32 alignment
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvMemFreeAligned(VOID *pAlignBuf, UINT32 alignment)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_DRV_CHECK_PTR(pAlignBuf, err, "pAlignBuf == null!");

    s32Ret = mem_hal_mmzFree(pAlignBuf, "PPM", "aligned mem");
    PPM_DRV_CHECK_RET(s32Ret, err, "mmz free failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvMemAllocAligned
 * @brief:      软解jdec申请aligned内存
 * @param[in]:  UINT32 size
 * @param[in]:  UINT32 alignment
 * @param[in]:  CHAR *pcName
 * @param[in]:  CHAR *pcZone
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvMemAllocAligned(UINT32 size, UINT32 alignment, CHAR *pcName, CHAR *pcZone)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pBuf = NULL;         /* 申请的mmz地址   mmz: |DataHead|Alignment|AlignBuf| */
    PhysAddr phyAddr = 0;

    /* 申请的内存大小不能为负和0, alignment必须为2的整次幂 */
    if (0 == size || (alignment & (alignment - 1)))
    {
        return NULL;
    }

    /* 申请mmz内存，前一级调用中已保证alignment至少为8个字节 */
    s32Ret = mem_hal_mmzAlloc(size + alignment, "PPM", "aligned mem", NULL, SAL_FALSE, &phyAddr, (VOID **)&pBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "mmz alloc failed!");

    return pBuf;
err:
    return NULL;
}

/**
 * @function:   Ppm_DrvMemFree
 * @brief:      释放内存
 * @param[in]:  VOID *pBuf
 * @param[in]:  UINT32 align
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvMemFree(VOID *pBuf, UINT32 align)
{
    UINT32 s32Ret = SAL_FAIL;

    /* 默认8字节对齐，与内存申请对应 */
    if (align < 8)
    {
        align = 8;
    }

    /*(align & align - 1)用于判断是否是2的N次幂*/
    if ((align & (align - 1)) || NULL == pBuf)
    {
        SAL_ERROR("param err, align:0x%x pBuf %p\n", align, pBuf);
        return SAL_FAIL;
    }

    s32Ret = Ppm_DrvMemFreeAligned(pBuf, align);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Free Aligned Mem Failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvMemAlloc
 * @brief:      软解jdec申请系统内存
 * @param[in]:  UINT32 size
 * @param[in]:  UINT32 align
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvMemAlloc(UINT32 size, UINT32 align)
{
    void *pVitrAddr = NULL;

    /* 对齐*/
    if (align < 8)
    {
        align = 8;
    }

    /*(align & align - 1)用于判断是否是2的N次幂*/
    if ((size == 0) || (align & (align - 1)))
    {
        PPM_LOGE("param err, size:0x%x, align:0x%x\n", size, align);
        return NULL;
    }

    pVitrAddr = Ppm_DrvMemAllocAligned(size, align, NULL, NULL);

    return pVitrAddr;
}

/**
 * @function:   Ppm_DrvJdecSoftFreeMem
 * @brief:      软解Jdec释放内存
 * @param[in]:  HKA_MEM_TAB *memTab
 * @param[in]:  UINT32 tabNum
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvJdecSoftFreeMem(HKA_MEM_TAB *memTab, UINT32 tabNum)
{
    UINT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;
    UINT32 size = 0;
    UINT32 uiFailcnt = 0;
    void *pBuf = NULL;
    HKA_MEM_ALIGNMENT align = HKA_MEM_ALIGN_4BYTE;   /* 默认4字节对齐 */

    /* 入参判断 */
    if ((NULL == memTab)
        || (0 == tabNum))
    {
        SAL_ERROR("Err memTab:%p,memTab:%d\n", memTab, tabNum);
        return SAL_FAIL;
    }

    for (i = 0; i < tabNum; i++)
    {
        size = memTab[i].size;
        align = memTab[i].alignment;
        pBuf = memTab[i].base;

        if (0 != size)
        {
            s32Ret = Ppm_DrvMemFreeAligned(pBuf, align);
            if (SAL_SOK != s32Ret)
            {
                uiFailcnt++;
                SAL_ERROR("Err i:%d size:%d, align:%d\n", i, size, align);
                continue;  /* 释放失败不返回，进行下一次释放 */
            }

            memTab[i].base = NULL;
        }
    }

    if (uiFailcnt > 0)
    {
        SAL_ERROR("Mem Free End! Fail %d times !\n", uiFailcnt);
    }

    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvJdecSoftAllocMem
 * @brief:      软件jdec申请内存表
 * @param[in]:  HKA_MEM_TAB *memTab
 * @param[in]:  UINT32 tabNum
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvJdecSoftAllocMem(HKA_MEM_TAB *memTab, UINT32 tabNum)
{
    UINT32 i = 0;
    UINT32 size = 0;
    void *pBase = NULL;
    HKA_MEM_ALIGNMENT align = HKA_MEM_ALIGN_4BYTE;

    /* 入参判断 */
    if ((NULL == memTab)
        || (0 == tabNum))
    {
        SAL_ERROR("Err memTab:%p,memTab:%d\n", memTab, tabNum);
        return SAL_FAIL;
    }

    for (i = 0; i < tabNum; i++)
    {
        size = memTab[i].size;
        align = memTab[i].alignment;

        if (size != 0)
        {
            pBase = Ppm_DrvMemAlloc(size, align);
            if (NULL == pBase)
            {
                SAL_ERROR("Err i:%d size:%d, align:%d\n", i, size, align);
                return SAL_FAIL;
            }

            memTab[i].base = pBase;
        }
        else
        {
            memTab[i].base = NULL;
        }
    }

    for (i = 0; i < HKA_MEM_TAB_NUM; i++)
    {
        PPM_LOGD("memTab: i %d size %u alignment %d base %p \n", i,
                 (UINT32)memTab[i].size,
                 (UINT32)memTab[i].alignment,
                 memTab[i].base);
    }

    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvI420ToNv21
 * @brief:      视频帧格式转换将I420的格式转成nv21
 * @param[in]:  PPM_YUV_FRAME_EX_S *pYuvFrm
 * @param[in]:  UINT32 yDstStride
 * @param[in]:  UINT32 uvDstStride
 * @param[in]:  PUINT8 pDstLumaAddr
 * @param[in]:  PUINT8 pDstChromaAddr
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvI420ToNv21(PPM_YUV_FRAME_EX_S *pYuvFrm,
                        UINT32 yDstStride,
                        UINT32 uvDstStride,
                        PUINT8 pDstLumaAddr,
                        PUINT8 pDstChromaAddr)
{
    UINT32 err = SAL_SOK;
    UINT32 index = 0;
/*    UINT32 index_w = 0; */
    UINT32 i = 0;
/*    UINT32 ylen = 0; */
/*    UINT32 uvlen = 0; */
    PUINT8 pChromaAddr_u = NULL;    /* 输入色度u首地址 */
    PUINT8 pChromaAddr_v = NULL;    /* 输入色度v首地址 */
    PUINT8 plumaTopAddr = NULL;     /* 输入源亮度顶场首地址 */
    PUINT8 plumaBotAddr = NULL;     /* 输入源亮度底场首地址 */
    PUINT8 plumadst_o = NULL;       /* 输出亮度偶数行首地址 */
    PUINT8 plumadst_e = NULL;       /* 输出亮度奇数行首地址 */
    PUINT8 pchromaTopAddr_u = NULL; /* 输入源色度u顶场首地址 */
    PUINT8 pchromaBotAddr_u = NULL; /* 输入源色度u底场首地址 */
    PUINT8 pchromaTopAddr_v = NULL; /* 输入源色度v顶场首地址 */
    PUINT8 pchromaBotAddr_v = NULL; /* 输入源色度v底场首地址 */
/*    PUINT8 pchromadst_o = NULL;     / * 输出亮度偶数行首地址 * / */
/*    PUINT8 pchromadst_e = NULL;     / * 输出亮度奇数行首地址 * / */
    uint8x16_t y_8x16;
    uint8x8_t u_8x8, v_8x8;
/*    uint8x8_t y_8x8_1, y_8x8_2; */
    uint8x16_t y_8x8_11; /* , y_8x8_21; */
    uint8x8x2_t tmp_8x8x2;
    uint8x16_t vu8x16;
/*    uint8x16_t y8x16; */
    UINT32 offset = 0;
    UINT32 width = 0;
    UINT32 height = 0;
    UINT32 srcYStride = 0;
    UINT32 srcUvStride = 0;

    /* 输入参数检查 */
    if ((NULL == pYuvFrm)
        || (0 == yDstStride)
        || (0 == uvDstStride)
        || (NULL == pDstLumaAddr)
        || (NULL == pDstChromaAddr))
    {
        SAL_ERROR("Err, pYuvFrm:%p,yDstStride:%d,uvDstStride:%d,pDstLumaAddr:%p,pDstChromaAddr:%p\n",
                  pYuvFrm, yDstStride, uvDstStride, pDstLumaAddr, pDstChromaAddr);

        return SAL_FAIL;
    }

    /* 待格式转换前的图像大小 */
    width = pYuvFrm->width;
    height = pYuvFrm->height;
    srcYStride = pYuvFrm->pitchY;
    srcUvStride = pYuvFrm->pitchUv;

    /* ylen = width * height; */
    /* uvlen = ylen >> 2; */

    do
    {
        if (PPM_INTERLACED_FRAME_MODE == pYuvFrm->frameMode)
        {
            if ((NULL == pYuvFrm->yTopAddr)
                || (NULL == pYuvFrm->uTopAddr)
                || (NULL == pYuvFrm->yBotAddr)
                || (NULL == pYuvFrm->uBotAddr))
            {
                SAL_ERROR("Err, yTopAddr:%p,uTopAddr:%p yBotAddr:%p uBotAddr:%p\n",
                          pYuvFrm->yTopAddr, pYuvFrm->uTopAddr, pYuvFrm->yBotAddr, pYuvFrm->uBotAddr);

                err = SAL_FAIL;
                break;
            }

            /* Y分量 顶场一行底场一行*/
            plumaTopAddr = pYuvFrm->yTopAddr;
            plumaBotAddr = pYuvFrm->yBotAddr;

            for (index = 0; index < height; index += 2)
            {
                plumadst_e = pDstLumaAddr + index * yDstStride;        /* 0 2 4 6 8...行 */
                plumadst_o = pDstLumaAddr + index * yDstStride + yDstStride; /* 1 3 5 7 9...行 */

                for (i = 0; i < width; i += 16)
                {
                    y_8x16 = vld1q_u8(plumaTopAddr);    /* 顶场读入16个像素 */
                    vst1q_u8(plumadst_e, y_8x16);        /* store 16个像素 */

                    y_8x16 = vld1q_u8(plumaBotAddr);   /* 底场读入16个像素 */
                    vst1q_u8(plumadst_o, y_8x16);       /* store 16个像素 */

                    plumaTopAddr += 16;
                    plumaBotAddr += 16;
                    plumadst_e += 16;
                    plumadst_o += 16;
                }
            }

            pchromaTopAddr_u = pYuvFrm->uTopAddr;               /* u 顶场 */
            pchromaTopAddr_v = pYuvFrm->vTopAddr;               /* v 顶场 */
            pchromaBotAddr_u = pYuvFrm->uBotAddr;               /* u 底场 */
            pchromaBotAddr_v = pYuvFrm->vBotAddr;               /* v 底场 */

            for (index = 0; index < (height >> 1); index += 2)
            {
                plumadst_e = pDstChromaAddr + index * uvDstStride;
                /* 取顶场VU交织 */
                for (i = 0; i < width; i += 16)
                {
                    u_8x8 = vld1_u8(pchromaTopAddr_u); /* 顶场u 读入8个像素 */
                    v_8x8 = vld1_u8(pchromaTopAddr_v); /* 顶场v 读入8个像素 */
                    tmp_8x8x2 = vzip_u8(v_8x8, u_8x8);  /* 交织 vu 得到 16个像素 */
                    vu8x16 = vcombine_u8(tmp_8x8x2.val[0], tmp_8x8x2.val[1]);

                    vst1q_u8(plumadst_e, vu8x16); /* store 16个像素 */

                    pchromaTopAddr_u += 8;
                    pchromaTopAddr_v += 8;
                    plumadst_e += 16;
                }

                plumadst_o = pDstChromaAddr + index * uvDstStride + uvDstStride;
                /* 取底场VU交织 */
                for (i = 0; i < width; i += 16)
                {
                    u_8x8 = vld1_u8(pchromaBotAddr_u); /* 底场u读入8个像素 */
                    v_8x8 = vld1_u8(pchromaBotAddr_v); /* 底场v读入8个像素 */
                    tmp_8x8x2 = vzip_u8(v_8x8, u_8x8); /* 交织 */
                    vu8x16 = vcombine_u8(tmp_8x8x2.val[0], tmp_8x8x2.val[1]);

                    /* store 数据 */
                    vst1q_u8(plumadst_o, vu8x16);
                    /* 地址累加 */
                    pchromaBotAddr_u += 8;
                    pchromaBotAddr_v += 8;
                    plumadst_o += 16;
                }
            }
        }
        else if (PPM_PROGRESSIVE_FRAME_MODE == pYuvFrm->frameMode)
        {
            if ((NULL == pYuvFrm->yTopAddr)
                || (NULL == pYuvFrm->uTopAddr)
                || (NULL == pYuvFrm->vTopAddr)
                || (width < yDstStride)
                || (width % 2u)
                || (yDstStride % 16u))
            {
                SAL_ERROR("Err, yTopAddr:%p,uTopAddr:%p vTopAddr:%p width:%d ,yStride:%d\n",
                          pYuvFrm->yTopAddr, pYuvFrm->uTopAddr, pYuvFrm->vTopAddr, width, yDstStride);

                err = SAL_FAIL;
                break;
            }

            /* 处理原则是nv21图像的缩放要求16字节对齐，源图像不满足，截取源图像中中间的部分，左右各裁剪一点 */

            /* 图像左边裁剪的偏移量 */
            offset = SAL_alignDown(((width - yDstStride) / 2), 4u); /* 4对齐 确保uv分量的偏移是2对齐 */

            /* y分量的指针先偏移好 */
            plumaTopAddr = pYuvFrm->yTopAddr + offset;

            /* Y分量每一次拷贝16个像素点(实际效率(每一次拷贝16个)和使用memcpy一次全部拷贝效率差不多) */
            for (index = 0; index < height; index++)
            {
                for (i = 0; i < yDstStride / 16; i++)
                {
                    y_8x8_11 = vld1q_u8(plumaTopAddr);      /* load 16个y分量像素值 */
                    vst1q_u8(pDstLumaAddr, y_8x8_11);       /* store 16个y分量像素值 */
                    pDstLumaAddr += 16;
                    plumaTopAddr += 16;
                }

                /* 图像左右边裁剪的偏移量 */
                plumaTopAddr += (srcYStride - yDstStride);
            }

            /* UV分量的指针先偏移好 */
            pChromaAddr_u = pYuvFrm->uTopAddr + offset / 2;
            pChromaAddr_v = pYuvFrm->vTopAddr + offset / 2;
            /* 注意YUV420颜色分量长度为一半,为兼容后续输出显示部分代码和Hisi解码输出保持一致将UV反置 */
            for (index = 0; index < height / 2; index++)
            {
                for (i = 0; i < uvDstStride / 16; i++)
                {
                    u_8x8 = vld1_u8(pChromaAddr_u); /* load 8个u分量像素值 */
                    v_8x8 = vld1_u8(pChromaAddr_v); /* load 8个v分量像素值 */
                    tmp_8x8x2 = vzip_u8(v_8x8, u_8x8); /* vu交织 */
                    vu8x16 = vcombine_u8(tmp_8x8x2.val[0], tmp_8x8x2.val[1]);

                    vst1q_u8(pDstChromaAddr, vu8x16);  /* store 16个vu分量数值 */
                    pChromaAddr_u += 8;
                    pChromaAddr_v += 8;
                    pDstChromaAddr += 16;
                }

                /* 图像左右边裁剪的偏移量 */
                pChromaAddr_u += srcUvStride - uvDstStride / 2;
                pChromaAddr_v += srcUvStride - uvDstStride / 2;
            }
        }
        else
        {
            SAL_ERROR("Err, frameMode:%d\n", pYuvFrm->frameMode);

            err = SAL_FAIL;
            break;
        }
    }
    while (0);

    return err;
}

/**
 * @function:   Ppm_DrvSoftWareJdec
 * @brief:      软解jdec
 * @param[in]:  PPM_JDEC_PROC_IN_S *pstProcIn
 * @param[in]:  HKAJPGD_ABILITY *pstAbility
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvSoftWareJdec(PPM_JDEC_PROC_IN_S *pstProcIn, HKAJPGD_ABILITY *pstAbility, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;
    UINT32 bufLen = 0;
    UINT32 decWidth = 0;
    UINT32 decHeight = 0;
    UINT32 pitch = 0;

    VOID *handle = NULL;
    PUINT8 pSrcY = NULL;
    PUINT8 pSrcV = NULL;
    PUINT8 pSrcU = NULL;
    VOID *pVirAddr = NULL;
    VOID *pConvertData[3] = {NULL};
    VOID *pOutBuf1 = NULL;          /* 解码的后的图像存储 */
    VOID *pOutBuf2 = NULL;          /* 格式转换后的图像 */

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    HKA_MEM_TAB memTab[HKA_MEM_TAB_NUM] = {0};
    PPM_YUV_FRAME_EX_S yuvFrame = {0};
    HKAJPGD_OUTPUT outParam = {0};
    HKAJPGD_STREAM stJpegData = {0};
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    PPM_DRV_CHECK_PTR(pstProcIn, err, "pstProcIn == null!");
    PPM_DRV_CHECK_PTR(pstAbility, err, "pstAbility == null!");
    PPM_DRV_CHECK_PTR(pstSysFrame, err, "pstSysFrame == null!");

    SAL_INFO("SOFT Jdec Entering! \n");

    /* 获取所需资源数量 */
    s32Ret = HKAJPGD_GetMemSize(pstAbility, memTab);
    if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
    {
        SAL_ERROR("Err ret:%#x\n", s32Ret);
        return SAL_FAIL;
    }

    /* 分配库所需资源 */
    s32Ret = Ppm_DrvJdecSoftAllocMem(memTab, HKA_MEM_TAB_NUM);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Ppm_DrvJdecSoftAllocMem failed! ret:%#x\n", s32Ret);
        return SAL_FAIL;
    }

    uiPicW = pstAbility->image_info.img_size.width;
    uiPicH = pstAbility->image_info.img_size.height;

    /* 创建库实例 */
    s32Ret = HKAJPGD_Create(pstAbility, memTab, &handle);
    if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
    {
        SAL_ERROR("Err ret:%#x\n", s32Ret);
        goto out_1;
    }

    bufLen = ((uiPicH / 32) + 1) * 32 * ((uiPicW / 32) + 1) * 32;

    /* 申请jpg解码的后的缓存 4表示最大用到的存储的分量 */
    pOutBuf1 = Ppm_DrvMemAlloc(bufLen * 4, 128);
    if (NULL == pOutBuf1)
    {
        SAL_ERROR("Mem Alloc Failed! \n");
        goto out_1;
    }

    outParam.image_out.data[0] = pOutBuf1;
    outParam.image_out.data[1] = outParam.image_out.data[0] + bufLen;
    outParam.image_out.data[2] = outParam.image_out.data[1] + bufLen;
    outParam.image_out.data[3] = outParam.image_out.data[2] + bufLen;

    stJpegData.stream_buf = (UINT8 *)pstProcIn->pcJpegAddr;
    stJpegData.stream_len = pstProcIn->uiJpegLen;

    /* process img_out的数据块 */
    s32Ret = HKAJPGD_Process(handle, (void *)&stJpegData, sizeof(HKAJPGD_STREAM), (void *)&outParam, sizeof(HKAJPGD_OUTPUT));
    if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
    {
        SAL_ERROR("Err ret:%#x\n", s32Ret);
        goto out_2;
    }
    else
    {
        /* jpg解码完后有多种格式，需要换成0x22111100处理 即为I420格式 */
        if (0x22111100 != pstAbility->image_info.pix_format)
        {
            /* 申请格式转换后的缓存 I420格式 */
            bufLen = ((uiPicH / 32) + 1) * 32 * ((uiPicW / 32) + 1) * 32;
            pOutBuf2 = Ppm_DrvMemAlloc(bufLen * 3 / 2, 128);
            if (NULL == pOutBuf2)
            {
                SAL_ERROR("Mem Alloc Failed! \n");
                goto out_2;
            }

            pConvertData[0] = pOutBuf2;                     /* Y分量 */
            pConvertData[1] = pConvertData[0] + bufLen;     /* U分量 */
            pConvertData[2] = pConvertData[1] + bufLen / 4; /* V分量 */

            /* 申请地址用着格式转化用的 */
            s32Ret = HKAJPGD_FormatConvert(handle, (void *)&outParam, (void **)pConvertData);
            if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
            {
                SAL_ERROR("Err ret:%#x\n", s32Ret);
                goto out_3;
            }

            pSrcY = pConvertData[0]; /* Y分量 */
            pSrcU = pConvertData[1]; /* U分量 */
            pSrcV = pConvertData[2]; /* V分量 */
        }
        else
        {
            pSrcY = outParam.image_out.data[0]; /* Y分量 */
            pSrcU = outParam.image_out.data[1]; /* U分量 */
            pSrcV = outParam.image_out.data[2]; /* V分量 */
        }

        /* 解码后的yuv图像宽和高都是2对齐，outParam.image_out.width和height是原图的大小 */
        decWidth = SAL_alignDown(outParam.image_out.width, 2u);
        decHeight = SAL_alignDown(outParam.image_out.height, 2u);

        /* 将yv12拷贝成nv21格式 */
        pitch = SAL_alignDown(decWidth, 16u);   /* 后面缩放模块要求pitch16对齐 格式转换后的图像要求16对齐，用优化的函数 宽和高要求是一样 */

        SAL_INFO("align w %d h %d pitch %d \n", decWidth, decHeight, pitch);

        /* 申请VB用于存放格式转换完成的yuv数据，该VB在本接口外释放 */
        bufLen = pitch * decHeight * 3 / 2;
        s32Ret = mem_hal_vbAlloc(bufLen, "PPM", "face_jdec_tmp_buf", NULL, SAL_TRUE, &stAllocVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("Alloc Vb Buf For Jdec Failed! \n");
            goto out_3;
        }

        /* 输出帧信息 */
        stVideoFrmBuf.frameParam.width = pitch;
        stVideoFrmBuf.frameParam.height = decHeight;
        stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
        stVideoFrmBuf.phyAddr[0] = stAllocVbInfo.u64PhysAddr;
        stVideoFrmBuf.phyAddr[1] = stAllocVbInfo.u64PhysAddr + pitch * decHeight;
        stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];
        stVideoFrmBuf.virAddr[0] = (PhysAddr)stAllocVbInfo.pVirAddr;
        stVideoFrmBuf.virAddr[1] = (PhysAddr)((CHAR *)stAllocVbInfo.pVirAddr + pitch * decHeight);
        stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
        stVideoFrmBuf.stride[0] = pitch;
        stVideoFrmBuf.stride[1] = pitch;
        stVideoFrmBuf.pts = 0;
        stVideoFrmBuf.poolId = stAllocVbInfo.u32PoolId;
		stVideoFrmBuf.vbBlk = stAllocVbInfo.u64VbBlk;
		stVideoFrmBuf.privateDate = stAllocVbInfo.u64VbBlk;    // 复用privateData传递vbBlk信息

        (VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSysFrame);

        memset(&yuvFrame, 0x0, sizeof(yuvFrame));
        yuvFrame.frameMode = PPM_PROGRESSIVE_FRAME_MODE;
        yuvFrame.yTopAddr = pSrcY;
        yuvFrame.uTopAddr = pSrcU;
        yuvFrame.vTopAddr = pSrcV;
        yuvFrame.width = decWidth;
        yuvFrame.height = decHeight;
        yuvFrame.pitchY = outParam.image_out.step[0];
        yuvFrame.pitchUv = outParam.image_out.step[1]; /* uv的值一样大 */

        /* 海思平台的格式要求 优化的函数时候16字节对齐，宽和pitch一致的情况 */
        s32Ret = Ppm_DrvI420ToNv21(&yuvFrame, stVideoFrmBuf.stride[0], stVideoFrmBuf.stride[1], \
                                   (PUINT8)stVideoFrmBuf.virAddr[0], (PUINT8)stVideoFrmBuf.virAddr[1]);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("Err: 0x%x!\n", s32Ret);
            goto out_4;
        }
    }

    /* MMZ内存释放 */
    (VOID)Ppm_DrvMemFree(pOutBuf2, 128);
    (VOID)Ppm_DrvMemFree(pOutBuf1, 128);
    (VOID)Ppm_DrvJdecSoftFreeMem(memTab, HKA_MEM_TAB_NUM);

    return SAL_SOK;

out_4:
    (VOID)mem_hal_vbFree(pVirAddr, "PPM", "face_jdec_tmp_buf", stAllocVbInfo.u32Size, stAllocVbInfo.u64VbBlk, stAllocVbInfo.u32PoolId);
out_3:
    (VOID)Ppm_DrvMemFree(pOutBuf2, 128);
out_2:
    (VOID)Ppm_DrvMemFree(pOutBuf1, 128);
out_1:
    (VOID)Ppm_DrvJdecSoftFreeMem(memTab, HKA_MEM_TAB_NUM);

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvHdWareJdec
 * @brief:      硬解Jpeg2Yuv
 * @param[in]:  PPM_JDEC_PROC_IN_S *pstProcIn
 * @param[in]:  HKAJPGD_ABILITY *pstAbility
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvHdWareJdec(PPM_JDEC_PROC_IN_S *pstProcIn, HKAJPGD_ABILITY *pstAbility, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;

    //JPEG_SNAP_CB_RESULT_ST stJpegInfo = {0};

    PPM_DRV_CHECK_PTR(pstProcIn, err, "pstProcIn == null!");
    PPM_DRV_CHECK_PTR(pstAbility, err, "pstAbility == null!");
    PPM_DRV_CHECK_PTR(pstSysFrame, err, "pstSysFrame == null!");

    uiPicW = pstAbility->image_info.img_size.width;
    uiPicH = pstAbility->image_info.img_size.height;

    SAL_INFO("HW Jdec Entering! w %d, h %d \n", uiPicW, uiPicH);

    /* 更新解码输出通道宽高 */
    uiPicW = SAL_align(uiPicW, 2);
    uiPicH = SAL_align(uiPicH, 2);

    s32Ret = vdec_tsk_PpmSetOutChnResolution(PPM_VDEC_CHN_CALIB_JDEC, 0, uiPicW, uiPicH);
    PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_ScaleJpegChn failed!");

    //stJpegInfo.cJpegAddr = (UINT8 *)pstProcIn->pcJpegAddr;
    //stJpegInfo.uiJpegSize = pstProcIn->uiJpegLen;

    /* jpg数据送解码 */
    s32Ret = vdec_tsk_SendJpegFrame(PPM_VDEC_CHN_CALIB_JDEC, (void *)pstProcIn->pcJpegAddr, pstProcIn->uiJpegLen);
    PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_SendJpegFrame failed!");

    /* 获取解码数据 */
    s32Ret = vdec_tsk_GetJpegYUVFrame(PPM_VDEC_CHN_CALIB_JDEC, 0, (VOID *)pstSysFrame->uiAppData);
    PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_GetJpegYUVFrame failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetImgInfo
 * @brief:      获取图片信息
 * @param[in]:  UINT8 *pcJpeg
 * @param[in]:  UINT32 uiJpegLen
 * @param[in]:  HKAJPGD_ABILITY *pstAbility
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetImgInfo(UINT8 *pcJpeg, UINT32 uiJpegLen, HKAJPGD_ABILITY *pstAbility)
{
    INT32 s32Ret = SAL_SOK;

    HKAJPGD_STREAM stJpegData = {0};

    PPM_DRV_CHECK_PTR(pcJpeg, err, "pcJpeg == null!");
    PPM_DRV_CHECK_PTR(pstAbility, err, "pstAbility == null!");

    stJpegData.stream_buf = (UINT8 *)pcJpeg;
    stJpegData.stream_len = uiJpegLen;

    s32Ret = HKAJPGD_GetImageInfo(&stJpegData, &pstAbility->image_info);
    if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
    {
        SAL_ERROR("HKAJPGD_GetImageInfo ERR! s32Ret:0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvJpeg2Yuv
 * @brief:      jpeg转yuv
 * @param[in]:  PPM_JDEC_PROC_IN_S *pstProcIn
 * @param[in]:  PPM_JDEC_PROC_OUT_S *pstProcOut
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvJpeg2Yuv(PPM_JDEC_PROC_IN_S *pstProcIn, PPM_JDEC_PROC_OUT_S *pstProcOut)
{
    INT32 s32Ret = SAL_SOK;

    BOOL bHwJDec = SAL_TRUE;
    BOOL bSoftJDec = SAL_FALSE;

    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;
    UINT32 uiProgressiveMode = 0;

    HKAJPGD_ABILITY stAbility = {0};

    PPM_DRV_CHECK_PTR(pstProcIn, err, "pstProcIn == null!");
    PPM_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    s32Ret = Ppm_DrvGetImgInfo((UINT8 *)pstProcIn->pcJpegAddr, pstProcIn->uiJpegLen, &stAbility);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetImgInfo failed!");

    uiPicW = stAbility.image_info.img_size.width;
    uiPicH = stAbility.image_info.img_size.height;
    uiProgressiveMode = stAbility.image_info.progressive_mode;

    if (uiPicW > 1920 || uiPicH > 1080)
    {
        PPM_LOGE("invalid jpeg info! w %d, h %d \n", uiPicW, uiPicH);
        return SAL_FAIL;
    }

    SAL_INFO("ability: progressive_mode %d num_components %d w %d h %d pix_format 0x%x watermark_enable %d \n",
             stAbility.image_info.progressive_mode,
             stAbility.image_info.num_components,
             stAbility.image_info.img_size.width,
             stAbility.image_info.img_size.height,
             stAbility.image_info.pix_format,
             stAbility.watermark_enable);

    SAL_INFO("get: w %d h %d mode %d \n", uiPicW, uiPicH, uiProgressiveMode);

    if (uiProgressiveMode)
    {
        bHwJDec = SAL_FALSE;
        bSoftJDec = SAL_TRUE;
    }

    if (bHwJDec)
    {
        pstProcOut->enProcMode = PPM_JDEC_HW_MODE;

        s32Ret = Ppm_DrvHdWareJdec(pstProcIn, &stAbility, pstProcOut->pstSysFrame);
        if (SAL_SOK != s32Ret)
        {
            PPM_LOGW("hard jdec failed! use soft jdec! \n");
            bSoftJDec = SAL_TRUE;
        }
    }

    if (bSoftJDec)
    {
        pstProcOut->enProcMode = PPM_JDEC_SOFT_MODE;

        s32Ret = Ppm_DrvSoftWareJdec(pstProcIn, &stAbility, pstProcOut->pstSysFrame);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvHdWareJdec failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvPutJdecSingleFrame
 * @brief:      释放jdec中间缓存（单帧处理）
 * @param[in]:  PPM_JDEC_PROC_MODE_E enProcMode
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvPutJdecSingleFrame(PPM_JDEC_PROC_MODE_E enProcMode, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 s32Ret = SAL_SOK;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    PPM_DRV_CHECK_PTR(pstSysFrame, err, "pstSysFrame == null!");

    (VOID)sys_hal_getVideoFrameInfo(pstSysFrame, &stVideoFrmBuf);

    /* VB资源释放，以免mmz内存泄露 */
    if (PPM_JDEC_HW_MODE == enProcMode)
    {
        s32Ret = vdec_tsk_PutVideoFrmaeMeth(PPM_VDEC_CHN_CALIB_JDEC, 0, pstSysFrame);
        PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_PutVdecFrame failed!");
    }
    else if (PPM_JDEC_SOFT_MODE == enProcMode)
    {
        s32Ret = mem_hal_vbFree((VOID *)stVideoFrmBuf.virAddr[0], 
			                    "PPM", 
			                    "face_jdec_tmp_buf", 
			                    stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height * 3 / 2,
			                    stVideoFrmBuf.privateDate,
			                    stVideoFrmBuf.poolId);
        PPM_DRV_CHECK_RET(s32Ret, err, "mem_hal_vbFree failed!");
    }
    else
    {
        /* DO NOTHING... */
        SAL_WARN("Invalid Jdec Mode %d \n", enProcMode);
        goto err;
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvFacePutJdecFrameData
 * @brief:      释放jdec的中间缓存
 * @param[in]:  UINT32 uiJpegCnt
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvFacePutJdecFrameData(UINT32 uiJpegCnt)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    PPM_DEV_PRM *pstDevPrm = NULL;

    if (uiJpegCnt > PPM_MAX_FACE_CALIB_JPEG_NUM)
    {
        PPM_LOGE("invalid jpeg cnt %d \n", uiJpegCnt);
        goto err;
    }

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    for (i = 0; i < uiJpegCnt; i++)
    {
        if (pstDevPrm->stJdecPrm.enFaceCamProcMode[i] < PPM_JDEC_MODE_NUM)
        {
            s32Ret = Ppm_DrvPutJdecSingleFrame(pstDevPrm->stJdecPrm.enFaceCamProcMode[i], &pstDevPrm->stJdecPrm.stFaceCamSysFrame[i]);
            PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "Ppm_DrvPutJdecSingleFrame failed!");
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvResetJdecPrm
 * @brief:      重置jdec全局参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvResetJdecPrm(VOID)
{
    UINT32 i = 0;
    PPM_DEV_PRM *pstDevPrm = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    for (i = 0; i < PPM_MAX_CALIB_JPEG_NUM; i++)
    {
        pstDevPrm->stJdecPrm.enFaceProcMode[i] = PPM_JDEC_MODE_NUM;
        pstDevPrm->stJdecPrm.enTriProcMode[i] = PPM_JDEC_MODE_NUM;
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvFaceResetJdecPrm
 * @brief:      重置jdec全局参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvFaceResetJdecPrm(VOID)
{
    UINT32 i = 0;
    PPM_DEV_PRM *pstDevPrm = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    for (i = 0; i < PPM_MAX_FACE_CALIB_JPEG_NUM; i++)
    {
        pstDevPrm->stJdecPrm.enFaceCamProcMode[i] = PPM_JDEC_MODE_NUM;
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetCalibPicInfo
 * @brief:      获取标定图片信息
 * @param[in]:  PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm
 * @param[in]:  MTME_MCC_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetCalibPicInfo(PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm, MTME_MCC_CABLI_IN_T *pstCalibIn)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_CALIB_PIC_INFO_S *pstCalibPicInfo = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    PPM_JDEC_PROC_IN_S stJdecProcIn = {0};
    PPM_JDEC_PROC_OUT_S stJdecProcOut = {0};

    PPM_DRV_CHECK_PTR(pstCalibIn, err1, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err1, "pstCalibInputPrm == null!");

    pstCalibPicInfo = &pstCalibInputPrm->stCalibPicInfo;

    pstCalibIn->nimages = pstCalibPicInfo->uiJpegCnt;
    pstCalibIn->width = pstCalibPicInfo->uiJpegW;
    pstCalibIn->height = pstCalibPicInfo->uiJpegH;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err1, "pstDevPrm == null!");

    if (pstCalibPicInfo->uiJpegCnt > PPM_MAX_CALIB_JPEG_NUM)
    {
        PPM_LOGE("invalid calib jpeg cnt %d \n", pstCalibPicInfo->uiJpegCnt);
        goto err1;
    }

    (VOID)Ppm_DrvResetJdecPrm();

    SYSTEM_FRAME_INFO pstFaceSysFrame = {0};
    SYSTEM_FRAME_INFO pstTriSysFrame = {0};
    (VOID)sys_hal_allocVideoFrameInfoSt(&pstFaceSysFrame);
    (VOID)sys_hal_allocVideoFrameInfoSt(&pstTriSysFrame);

#if 0 /* DEBUG: 使用引擎demo的相机内参数据和标定图片进行测试 */
    PhysAddr u64PhyAddr = 0;
    void *pVirAddr = NULL;

    s32Ret = mem_hal_mmzAlloc(1920*1080*2*10,
                                      "ia_common",
                                      (CHAR *)"ppm",
                                      NULL,
                                      SAL_TRUE,
                                      &u64PhyAddr,
                                      (VOID **)&pVirAddr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Alloc Mem failed! mode %d, type %d \n", 0, IA_HISI_MMZ_CACHE);
        return SAL_FAIL;
    }	
    void *image_face = pVirAddr;
	void *image_tre  = (char *)pVirAddr +1920*1080*10;
    
    FILE *cab_face = fopen("./ppm/face_data.y", "rb");
    if (NULL == cab_face)
    {
        printf("cab_face_error\n");
    }

    for (int j = 0;j < pstCalibPicInfo->uiJpegCnt; j++)
    {
        pstCalibIn->titlcam_img_y[j]  = image_face + j * 1920*1080;
        fread(pstCalibIn->titlcam_img_y[j], 1920* 1080, 1, cab_face);
    }
    fclose(cab_face);

    cab_face = fopen("./ppm/tre_data.y", "rb");
    if (NULL == cab_face)
    {
        printf("cab_face_error\n");
    }

    for (int j = 0;j < pstCalibPicInfo->uiJpegCnt; j++)
    {
        pstCalibIn->vertical_img_y[j] = image_tre + j * 1920*1080;
        fread(pstCalibIn->vertical_img_y[j], 1920* 1080, 1, cab_face);
    }
    fclose(cab_face);

    PPM_LOGW("debug: get img end! \n");
    return SAL_SOK;
#endif

    for (i = 0; i < pstCalibPicInfo->uiJpegCnt; i++)
    {
        /* 获取人脸相机标定jpeg解码yuv数据 */
        stJdecProcIn.pcJpegAddr = (INT8 *)pstCalibPicInfo->stFaceCamCalibJpeg.stPicAttr[i].pcJpegAddr;
        stJdecProcIn.uiJpegLen = pstCalibPicInfo->stFaceCamCalibJpeg.stPicAttr[i].uiPicLen;

        if (NULL == stJdecProcIn.pcJpegAddr
            || 0 == stJdecProcIn.uiJpegLen)
        {
            PPM_LOGE("face: jpeg addr == null! or jpeg data len == 0! i %d, %p, %d \n",
                     i, stJdecProcIn.pcJpegAddr, stJdecProcIn.uiJpegLen);
            goto err;
        }

        stJdecProcOut.pstSysFrame = &pstFaceSysFrame;

        s32Ret = Ppm_DrvJpeg2Yuv(&stJdecProcIn, &stJdecProcOut);
        pstDevPrm->stJdecPrm.enFaceProcMode[i] = stJdecProcOut.enProcMode;

        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvJpeg2Yuv failed!");

        (VOID)sys_hal_getVideoFrameInfo(stJdecProcOut.pstSysFrame, &stVideoFrmBuf);
        //sal_memcpy_s(pstDevPrm->stJdecPrm.stFaceSysFrame[i].pcJpegAddr, size, (VOID*)stVideoFrmBuf.virAddr[0], size);
        memset(pstDevPrm->stJdecPrm.stFaceSysFrame[i].pcJpegAddr, 0x00, 1920*1080);
        memcpy(pstDevPrm->stJdecPrm.stFaceSysFrame[i].pcJpegAddr, (VOID*)stVideoFrmBuf.virAddr[0], 1920*1080);
        pstCalibIn->titlcam_img_y[i] = (UINT8 *)pstDevPrm->stJdecPrm.stFaceSysFrame[i].pcJpegAddr;

        if (uiDbgSwitchFlag == 2)
        {
            UINT32 uiFrmSize = 0; /* 1920*1080*3/2; */
            CHAR face_path[64] = {'\0'};
            static unsigned int uiPicCnt = 0;
            uiFrmSize = stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height*3/2;
            SAL_WARN("get face size %d, %p, [%p, %p, %p], [%p, %p, %p], w %d, h %d, s[%d, %d, %d] \n",
                     uiFrmSize, pstCalibIn->titlcam_img_y[i],
                     (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1], (VOID *)stVideoFrmBuf.virAddr[2],
                     (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1], (VOID *)stVideoFrmBuf.phyAddr[2],
                     stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height,
                     stVideoFrmBuf.stride[0], stVideoFrmBuf.stride[1], stVideoFrmBuf.stride[2]);
            snprintf(face_path, 64,"/home/config/face_%d.nv21", uiPicCnt);
            Ppm_HalDebugDumpData((CHAR *)pstCalibIn->titlcam_img_y[i], face_path, uiFrmSize, 1);
            uiPicCnt++;
        }

        /* 获取三目相机标定jpeg解码yuv数据 */
        stJdecProcIn.pcJpegAddr = (INT8 *)pstCalibPicInfo->stTriCamCalibJpeg.stPicAttr[i].pcJpegAddr;
        stJdecProcIn.uiJpegLen = pstCalibPicInfo->stTriCamCalibJpeg.stPicAttr[i].uiPicLen;

        if (NULL == stJdecProcIn.pcJpegAddr
            || 0 == stJdecProcIn.uiJpegLen)
        {
            PPM_LOGE("tri: jpeg addr == null! or jpeg data len == 0! i %d, %p, %d \n",
                     i, stJdecProcIn.pcJpegAddr, stJdecProcIn.uiJpegLen);
            goto err;
        }

        stJdecProcOut.pstSysFrame = &pstTriSysFrame;

        s32Ret = Ppm_DrvJpeg2Yuv(&stJdecProcIn, &stJdecProcOut);
        pstDevPrm->stJdecPrm.enTriProcMode[i] = stJdecProcOut.enProcMode;

        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvJpeg2Yuv failed!");

        (VOID)sys_hal_getVideoFrameInfo(stJdecProcOut.pstSysFrame, &stVideoFrmBuf);
        memset(pstDevPrm->stJdecPrm.stTriSysFrame[i].pcJpegAddr, 0x00, 1920*1080);
        memcpy(pstDevPrm->stJdecPrm.stTriSysFrame[i].pcJpegAddr, (VOID*)stVideoFrmBuf.virAddr[0], 1920*1080);

        pstCalibIn->vertical_img_y[i] = (UINT8 *)pstDevPrm->stJdecPrm.stTriSysFrame[i].pcJpegAddr;

        if (uiDbgSwitchFlag == 2)
        {
            UINT32 uiFrmSize = 0;
            CHAR tri_path[64] = {'\0'};
            static unsigned int uiPicCnt1 = 0;

            uiFrmSize = stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height*3/2;
            SAL_WARN("get ppm size %d, %p, [%p, %p, %p], [%p, %p, %p], w %d, h %d, s[%d, %d, %d] \n",
                     uiFrmSize, pstCalibIn->vertical_img_y[i],
                     (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1], (VOID *)stVideoFrmBuf.virAddr[2],
                     (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1], (VOID *)stVideoFrmBuf.phyAddr[2],
                     stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height,
                     stVideoFrmBuf.stride[0], stVideoFrmBuf.stride[1], stVideoFrmBuf.stride[2]);
            snprintf(tri_path, 64,"/home/config/tri_%d.nv21", uiPicCnt1);

            Ppm_HalDebugDumpData((CHAR *)pstCalibIn->vertical_img_y[i], tri_path, uiFrmSize, 1);
            uiPicCnt1++;
        }

        if (pstDevPrm->stJdecPrm.enFaceProcMode[i] < PPM_JDEC_MODE_NUM)
        {
            s32Ret = Ppm_DrvPutJdecSingleFrame(pstDevPrm->stJdecPrm.enFaceProcMode[i], &pstFaceSysFrame);
            PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "Ppm_DrvPutJdecSingleFrame failed!");
            pstDevPrm->stJdecPrm.enFaceProcMode[i] = PPM_JDEC_MODE_NUM;
        }

        if (pstDevPrm->stJdecPrm.enTriProcMode[i] < PPM_JDEC_MODE_NUM)
        {
            s32Ret = Ppm_DrvPutJdecSingleFrame(pstDevPrm->stJdecPrm.enTriProcMode[i], &pstTriSysFrame);
            PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "Ppm_DrvPutJdecSingleFrame failed!");
            pstDevPrm->stJdecPrm.enFaceProcMode[i] = PPM_JDEC_MODE_NUM;
        }

        PPM_LOGW("proc %d face+tri pic \n", i);
    }

    PPM_LOGI("get jdec yuv end! cnt %d \n", pstCalibPicInfo->uiJpegCnt);
    return SAL_SOK;

err1:
    return PPM_GET_CALIB_PCIINFO_ERR;

err:
    if (pstDevPrm->stJdecPrm.enFaceProcMode[i] < PPM_JDEC_MODE_NUM)
    {
        s32Ret = Ppm_DrvPutJdecSingleFrame(pstDevPrm->stJdecPrm.enFaceProcMode[i], &pstFaceSysFrame);
        PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "Ppm_DrvPutJdecSingleFrame failed!");
        pstDevPrm->stJdecPrm.enFaceProcMode[i] = PPM_JDEC_MODE_NUM;
    }

    if (pstDevPrm->stJdecPrm.enTriProcMode[i] < PPM_JDEC_MODE_NUM)
    {
        s32Ret = Ppm_DrvPutJdecSingleFrame(pstDevPrm->stJdecPrm.enTriProcMode[i], &pstTriSysFrame);
        PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "Ppm_DrvPutJdecSingleFrame failed!");
        pstDevPrm->stJdecPrm.enFaceProcMode[i] = PPM_JDEC_MODE_NUM;
    }
    return PPM_GET_CALIB_PCIINFO_ERR;
}

/**
 * @function:   Ppm_DrvGetFaceCalibPicInfo
 * @brief:      获取人脸相机标定图片信息
 * @param[in]:  PPM_FACE_CALIB_INPUT_PRM_S *pstCalibInputPrm
 * @param[in]:  MTME_MCC_SV_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetFaceCalibPicInfo(PPM_FACE_CALIB_INPUT_PRM_S *pstCalibInputPrm, MTME_MCC_SV_CABLI_IN_T *pstCalibIn)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

#ifdef PPM_DUMP_YUV
    CHAR *face_path = "./ppm/face.nv21";
    CHAR *tri_path = "./ppm/tri.nv21";
#endif

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_FACE_CALIB_PIC_INFO_S *pstCalibPicInfo = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    PPM_JDEC_PROC_IN_S stJdecProcIn = {0};
    PPM_JDEC_PROC_OUT_S stJdecProcOut = {0};

    PPM_DRV_CHECK_PTR(pstCalibIn, err1, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err1, "pstCalibInputPrm == null!");

    pstCalibPicInfo = &pstCalibInputPrm->stCalibPicInfo;

    pstCalibIn->image_num = pstCalibPicInfo->uiJpegCnt;
    pstCalibIn->img_w = pstCalibPicInfo->uiJpegW;
    pstCalibIn->img_h = pstCalibPicInfo->uiJpegH;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err1, "pstDevPrm == null!");

    if (pstCalibPicInfo->uiJpegCnt > PPM_MAX_FACE_CALIB_JPEG_NUM)
    {
        PPM_LOGE("invalid calib jpeg cnt %d \n", pstCalibPicInfo->uiJpegCnt);
        goto err1;
    }

#if 0 /* DEBUG: 使用引擎demo的相机内参数据和标定图片进行测试 */
    FILE *fp = NULL;
    CHAR face_path[16][256] = {
        "./ppm/10.19.112.99_01_20210813171014245.nv21",
        "./ppm/10.19.112.99_01_20210813171032452.nv21",
        "./ppm/10.19.112.99_01_20210813171033683.nv21",
        "./ppm/10.19.112.99_01_20210813171051492.nv21",
        "./ppm/10.19.112.99_01_20210813171133292.nv21",
        "./ppm/10.19.112.99_01_20210813171145781.nv21",
        "./ppm/10.19.112.99_01_20210813171157908.nv21",
        "./ppm/10.19.112.99_01_20210813171219396.nv21",
        "./ppm/10.19.112.99_01_20210813171233269.nv21",
        "./ppm/10.19.112.99_01_20210813171249780.nv21"
    };

    for (i = 0; i < pstCalibPicInfo->uiJpegCnt; i++)
    {
        fp = fopen(face_path[i], "r+");
        CHAR *pFaceData0 = HAL_memAlloc(3 * 1920 * 1080, NULL, NULL);
        fread(pFaceData0, 1920 * 1080, 1, fp);
        cablic_in->imageData[i] = (UINT8 *)pFaceData0;
        fclose(fp);
    }

    PPM_LOGW("debug: get img end! \n");
    return SAL_SOK;
#endif

    for (i = 0; i < pstCalibPicInfo->uiJpegCnt; i++)
    {
        /* 获取人脸相机标定jpeg解码yuv数据 */
        stJdecProcIn.pcJpegAddr = (INT8 *)pstCalibPicInfo->stFaceCamCalibJpeg.stPicAttr[i].pcJpegAddr;
        stJdecProcIn.uiJpegLen = pstCalibPicInfo->stFaceCamCalibJpeg.stPicAttr[i].uiPicLen;

        if (NULL == stJdecProcIn.pcJpegAddr
            || 0 == stJdecProcIn.uiJpegLen)
        {
            PPM_LOGE("face: jpeg addr == null! or jpeg data len == 0! i %d, %p, %d \n",
                     i, stJdecProcIn.pcJpegAddr, stJdecProcIn.uiJpegLen);
            goto err1;
        }

        stJdecProcOut.pstSysFrame = &pstDevPrm->stJdecPrm.stFaceCamSysFrame[i];

        s32Ret = Ppm_DrvJpeg2Yuv(&stJdecProcIn, &stJdecProcOut);
        pstDevPrm->stJdecPrm.enFaceCamProcMode[i] = stJdecProcOut.enProcMode;

        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvJpeg2Yuv failed!");

        (VOID)sys_hal_getVideoFrameInfo(stJdecProcOut.pstSysFrame, &stVideoFrmBuf);

        pstCalibIn->image_data[i] = (UINT8 *)stVideoFrmBuf.virAddr[0];
        PPM_LOGW("cnt %d \n", i);

    }

    PPM_LOGI("get jdec yuv end! cnt %d \n", pstCalibPicInfo->uiJpegCnt);
    return SAL_SOK;
err1:
    return PPM_GET_CALIB_PCIINFO_ERR;

err:
    (VOID)Ppm_DrvFacePutJdecFrameData(pstCalibPicInfo->uiJpegCnt);
    (VOID)Ppm_DrvFaceResetJdecPrm();
    return PPM_GET_CALIB_PCIINFO_ERR;
}

/**
 * @function:   Ppm_DrvGetCalibBoardInfo
 * @brief:      获取标定板参数
 * @param[in]:  PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm
 * @param[in]:  MTME_MCC_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetCalibBoardInfo(PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm, MTME_MCC_CABLI_IN_T *pstCalibIn)
{
    PPM_CALIB_CALIB_BOARD_INFO_S *pstCalibBoardInfo = NULL;

    PPM_DRV_CHECK_PTR(pstCalibIn, err, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err, "pstCalibInputPrm == null!");

    pstCalibBoardInfo = &pstCalibInputPrm->stCalibBoardInfo;
    pstCalibIn->chessboard_size.width = pstCalibBoardInfo->uiHoriCnt;
    pstCalibIn->chessboard_size.height = pstCalibBoardInfo->uiVertCnt;
    pstCalibIn->square_size = pstCalibBoardInfo->fSquaLen;

    PPM_LOGI("get HCnt %d, VCnt %d, fSquaLen %f \n",
             pstCalibBoardInfo->uiHoriCnt, pstCalibBoardInfo->uiVertCnt, pstCalibBoardInfo->fSquaLen);
    return SAL_SOK;
err:
    return PPM_GET_CALIB_BOARD_INFO_ERR;
}

/**
 * @function:   Ppm_DrvGetFaceCalibBoardInfo
 * @brief:      获取人脸相机标定板参数
 * @param[in]:  PPM_FACE_CALIB_INPUT_PRM_S *pstCalibInputPrm
 * @param[in]:  MTME_MCC_SV_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetFaceCalibBoardInfo(PPM_FACE_CALIB_INPUT_PRM_S *pstCalibInputPrm, MTME_MCC_SV_CABLI_IN_T *pstCalibIn)
{
    PPM_CALIB_CALIB_BOARD_INFO_S *pstCalibBoardInfo = NULL;

    PPM_DRV_CHECK_PTR(pstCalibIn, err, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err, "pstCalibInputPrm == null!");

    pstCalibBoardInfo = &pstCalibInputPrm->stCalibBoardInfo;
    pstCalibIn->chessboard_width = pstCalibBoardInfo->uiHoriCnt;
    pstCalibIn->chessboard_height = pstCalibBoardInfo->uiVertCnt;
    pstCalibIn->chessboard_num = 1;
    pstCalibIn->squaresize = pstCalibBoardInfo->fSquaLen;

    PPM_LOGI("get VCnt %d, HCnt %d, boardNum %d, fSquaLen %f \n",
             pstCalibIn->chessboard_width, pstCalibIn->chessboard_height,
             pstCalibIn->chessboard_num, pstCalibIn->squaresize);
    return SAL_SOK;
err:
    return PPM_GET_CALIB_BOARD_INFO_ERR;
}

/**
 * @function:   Ppm_DrvGetFaceCalibInfo
 * @brief:      获取标定相机参数
 * @param[in]:  PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm
 * @param[in]:  MTME_MCC_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetFaceCalibInfo(PPM_FACE_CALIB_INPUT_PRM_S *pstCalibInputPrm, MTME_MCC_SV_CABLI_IN_T *pstCalibIn)
{
    PPM_FACE_CAM_INFO_S *pstCalibCamInfo = NULL;

    PPM_DRV_CHECK_PTR(pstCalibIn, err, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err, "pstCalibInputPrm == null!");

    pstCalibCamInfo = &pstCalibInputPrm->stCamInfo;
    pstCalibIn->focal_length = pstCalibCamInfo->uiFocalLength;
    pstCalibIn->pixel_size = pstCalibCamInfo->pixelSize;

    PPM_LOGI("get focallength %d, pixelSize %f \n", pstCalibIn->focal_length, pstCalibIn->pixel_size);
    return SAL_SOK;
err:
    return PPM_GET_MATRIX_INFO_ERR;
}

/**
 * @function:   Ppm_DrvGetMatrixInfo
 * @brief:      获取应用下发的标定矩阵参数
 * @param[in]:  PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm
 * @param[in]:  MTME_MCC_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetMatrixInfo(PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm, MTME_MCC_CABLI_IN_T *pstCalibIn)
{
    UINT32 i = 0;

    PPM_FACE_CALIB_MATRIX_PRM_S *pstFaceMatrixPrm = NULL;
    PPM_TRI_CALIB_MATRIX_PRM_S *pstTriMatrixPrm = NULL;

    PPM_DRV_CHECK_PTR(pstCalibIn, err, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err, "pstCalibInputPrm == null!");

    /* 填充相机内参和畸变矫正参数 */
    pstFaceMatrixPrm = &pstCalibInputPrm->stFaceCamCalibMatrixInfo;
    pstTriMatrixPrm = &pstCalibInputPrm->stTriCamCalibMatrixInfo;

    /* 人脸相机内参+畸变矫正参数 */
    for (i = 0; i < pstFaceMatrixPrm->stCamInterPrm.uiRowCnt; i++)
    {
        memcpy(pstCalibIn->tiltcam.inter_matrix[i],
               pstFaceMatrixPrm->stCamInterPrm.fMatrix[i],
               sizeof(double) * pstFaceMatrixPrm->stCamInterPrm.uiColCnt);
    }

    memcpy(pstCalibIn->tiltcam.dist_param,
           pstFaceMatrixPrm->stCamDistortMatrix.fMatrix[0],
           sizeof(double) * pstFaceMatrixPrm->stCamDistortMatrix.uiColCnt);


    /* 三目相机内参+畸变矫正参数 */
    for (i = 0; i < pstTriMatrixPrm->stCamInterPrm.uiRowCnt; i++)
    {
        memcpy(pstCalibIn->vertical_cam.inter_matrix[i],
               pstTriMatrixPrm->stCamInterPrm.fMatrix[i],
               sizeof(double) * pstTriMatrixPrm->stCamInterPrm.uiColCnt);
    }

    /* 三目相机的畸变矫正参数设置为0，引擎内部会通过读文件操作获取 */
    memcpy(pstCalibIn->vertical_cam.dist_param,
           pstTriMatrixPrm->stCamDistortMatrix.fMatrix[0],
           sizeof(double) * pstTriMatrixPrm->stCamDistortMatrix.uiColCnt);

    PPM_LOGI("get matrix prm end! \n");
    return SAL_SOK;
err:
    return PPM_GET_MATRIX_INFO_ERR;
}

/**
 * @function:   Ppm_DrvInputCalibData
 * @brief:      向引擎输入标定数据
 * @param[in]:  UINT32 chan
 * @param[in]:  MTME_MCC_CABLI_IN_T *cablic_in
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInputCalibData(UINT32 chan, MTME_MCC_CABLI_IN_T *pstCalibIn, PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;
    ICF_INPUT_DATA *pstCalibEngInputData = NULL;

    PPM_DRV_CHECK_PTR(pstCalibIn, err, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err, "pstCalibInputPrm == null!");

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, err, "pstModPrm == null!");

	pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_DRV_CHECK_PTR(pstMccCalibChnPrm, err, "pstMccCalibChnPrm == null!");

    pstMccCalibChnPrm->stIcfCalibPrm.stCalibIptPrm.uiCalibUseFlag = 1;
	pstMccCalibChnPrm->stIcfCalibPrm.stCalibIptPrm.uiSubChn = PPM_SUB_MOD_MCC_CALIB;

	pstCalibEngInputData = &pstMccCalibChnPrm->stCalibEngInputData;

    /* 读取灰度图 */
    pstCalibEngInputData->nBlobNum = 1;

    /* 原图 */
    pstCalibEngInputData->stBlobData[0].nShape[0] = pstCalibIn->width;
    pstCalibEngInputData->stBlobData[0].nShape[1] = pstCalibIn->height;
    pstCalibEngInputData->stBlobData[0].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21;
    pstCalibEngInputData->stBlobData[0].nFrameNum = 0;
    pstCalibEngInputData->stBlobData[0].nSpace = ICF_HISI_MEM_MMZ_WITH_CACHE;
    pstCalibEngInputData->stBlobData[0].pData = pstCalibIn;
    pstCalibEngInputData->pUseFlag[0] = (INT32 *)&pstMccCalibChnPrm->stIcfCalibPrm.stCalibIptPrm.uiCalibUseFlag;
    pstCalibEngInputData->pUserPtr = (VOID *)&pstMccCalibChnPrm->stIcfCalibPrm.stCalibIptPrm.uiSubChn;
    pstCalibEngInputData->nUserDataSize = sizeof(UINT32);

#if 1
    PPM_LOGW("w %d, h %d, cnt %d, tilt %p, vert %p, c %d, r %d, sq %f \n",
             pstCalibIn->width, pstCalibIn->height, pstCalibIn->nimages, pstCalibIn->titlcam_img_y[0], pstCalibIn->vertical_img_y[0],
             pstCalibIn->chessboard_size.height, pstCalibIn->chessboard_size.width, pstCalibIn->square_size);
    PPM_LOGW("face: [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]], [%f, %f, %f, %f, %f, %f, %f, %f] \n",
             pstCalibIn->tiltcam.inter_matrix[0][0], pstCalibIn->tiltcam.inter_matrix[0][1], pstCalibIn->tiltcam.inter_matrix[0][2],
             pstCalibIn->tiltcam.inter_matrix[1][0], pstCalibIn->tiltcam.inter_matrix[1][1], pstCalibIn->tiltcam.inter_matrix[1][2],
             pstCalibIn->tiltcam.inter_matrix[2][0], pstCalibIn->tiltcam.inter_matrix[2][1], pstCalibIn->tiltcam.inter_matrix[2][2],
             pstCalibIn->tiltcam.dist_param[0], pstCalibIn->tiltcam.dist_param[1], pstCalibIn->tiltcam.dist_param[2], pstCalibIn->tiltcam.dist_param[3],
             pstCalibIn->tiltcam.dist_param[4], pstCalibIn->tiltcam.dist_param[5], pstCalibIn->tiltcam.dist_param[6], pstCalibIn->tiltcam.dist_param[7]);
    PPM_LOGW("tri: [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]], [%f, %f, %f, %f, %f, %f, %f, %f] \n",
             pstCalibIn->vertical_cam.inter_matrix[0][0], pstCalibIn->vertical_cam.inter_matrix[0][1], pstCalibIn->vertical_cam.inter_matrix[0][2],
             pstCalibIn->vertical_cam.inter_matrix[1][0], pstCalibIn->vertical_cam.inter_matrix[1][1], pstCalibIn->vertical_cam.inter_matrix[1][2],
             pstCalibIn->vertical_cam.inter_matrix[2][0], pstCalibIn->vertical_cam.inter_matrix[2][1], pstCalibIn->vertical_cam.inter_matrix[2][2],
             pstCalibIn->vertical_cam.dist_param[0], pstCalibIn->vertical_cam.dist_param[1], pstCalibIn->vertical_cam.dist_param[2], pstCalibIn->vertical_cam.dist_param[3],
             pstCalibIn->vertical_cam.dist_param[4], pstCalibIn->vertical_cam.dist_param[5], pstCalibIn->vertical_cam.dist_param[6], pstCalibIn->vertical_cam.dist_param[7]);
#endif

    PPM_DRV_CHECK_PTR(pstModPrm->stIcfFuncP.Ppm_IcfInputData, err, "Ppm_IcfInputData == null!");

    s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfInputData(pstMccCalibChnPrm->stCreateHandle.pChannelHandle, 
		                                           pstCalibEngInputData, 
		                                           sizeof(ICF_INPUT_DATA));
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_IcfInputData failed!");

    return SAL_SOK;
err:
    return PPM_INPUT_DATA_ERR;
}

/**
 * @function:   Ppm_DrvInputFaceCalibData
 * @brief:      向引擎输入标定数据
 * @param[in]:  UINT32 chan
 * @param[in]:  MTME_MCC_SV_CABLI_IN_T *pstCalibIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInputFaceCalibData(UINT32 chan, MTME_MCC_SV_CABLI_IN_T *pstCalibIn, PPM_FACE_CALIB_INPUT_PRM_S *pstCalibInputPrm)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;
    ICF_INPUT_DATA *pstCalibEngInputData = NULL;

    PPM_DRV_CHECK_PTR(pstCalibIn, err1, "cablic_in == null!");
    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err1, "pstCalibInputPrm == null!");

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, err1, "pstModPrm == null!");

    pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_DRV_CHECK_PTR(pstFaceCalibChnPrm, err1, "pstFaceCalibChnPrm == null!");

    pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibIptPrm.uiCalibUseFlag = 1;
	pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibIptPrm.uiSubChn = PPM_SUB_MOD_FACE_CALIB;

 	pstCalibEngInputData = &pstFaceCalibChnPrm->stCalibEngInputData;

    /* 读取灰度图 */
    pstCalibEngInputData->nBlobNum = 1;

    /* 原图 */
    pstCalibEngInputData->stBlobData[0].nShape[0] = pstCalibIn->img_w;
    pstCalibEngInputData->stBlobData[0].nShape[1] = pstCalibIn->img_h;
    pstCalibEngInputData->stBlobData[0].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21;
    pstCalibEngInputData->stBlobData[0].nFrameNum = 0;
    pstCalibEngInputData->stBlobData[0].nSpace = ICF_HISI_MEM_MMZ_WITH_CACHE;
    pstCalibEngInputData->stBlobData[0].pData = pstCalibIn;
    pstCalibEngInputData->pUseFlag[0] = (INT32 *)&pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibIptPrm.uiCalibUseFlag;
    pstCalibEngInputData->pUserPtr = (VOID *)&pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibIptPrm.uiSubChn;
    pstCalibEngInputData->nUserDataSize = sizeof(UINT32);

#if 1
    PPM_LOGW("board w %d, h %d, bnum %d, sq %f, img w %d, h %d, cnt %d, len %x, pixelsize %f, imageData %p \n",
             pstCalibIn->chessboard_width, pstCalibIn->chessboard_height, pstCalibIn->chessboard_num, pstCalibIn->squaresize,
             pstCalibIn->img_w, pstCalibIn->img_h, pstCalibIn->image_num, pstCalibIn->focal_length, pstCalibIn->pixel_size,
             pstCalibIn->image_data[0]);
#endif

    PPM_DRV_CHECK_PTR(pstModPrm->stIcfFuncP.Ppm_IcfInputData, err1, "Ppm_IcfInputData == null!");

    s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfInputData(pstFaceCalibChnPrm->stCreateHandle.pChannelHandle,
		                                           pstCalibEngInputData, 
		                                           sizeof(ICF_INPUT_DATA));
    PPM_LOGW("face calib: s32ret %x \n", s32Ret);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_IcfInputData failed!");

    return SAL_SOK;

err:
    return PPM_INPUT_DATA_ERR;
err1:
    if (pstCalibInputPrm != NULL)
    {
        (VOID)Ppm_DrvFacePutJdecFrameData(pstCalibInputPrm->stCalibPicInfo.uiJpegCnt);
        (VOID)Ppm_DrvFaceResetJdecPrm();
    }

    return PPM_INPUT_DATA_ERR;
}

/**
 * @function:   Ppm_DrvCalibSyncProc
 * @brief:      标定结果同步接口
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvCalibSyncProc(UINT32 chan, PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm)
{
    PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;

    PPM_DRV_CHECK_PTR(pstCalibInputPrm, err, "pstCalibInputPrm == null!");

	pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_DRV_CHECK_PTR(pstMccCalibChnPrm, err, "pstMccCalibChnPrm == null!");

    sem_wait(&pstMccCalibChnPrm->stIcfCalibPrm.sem);
    return SAL_SOK;

err:
    return PPM_SYNC_PROC_ERR;
}

/**
 * @function:   Ppm_DrvFaceCalibSyncProc
 * @brief:      人脸相机标定结果同步接口
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvFaceCalibSyncProc(UINT32 chan)
{
    PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;

    pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_DRV_CHECK_PTR(pstFaceCalibChnPrm, err, "pstFaceCalibChnPrm == null!");

    sem_wait(&pstFaceCalibChnPrm->stIcfFaceCalibPrm.sem);

    return SAL_SOK;
err:
    return PPM_SYNC_PROC_ERR;
}

/**
 * @function:   Ppm_DrvPrintCalibRslt
 * @brief:      打印标定结果
 * @param[in]:  PPM_OUTPUT_CALIB_PRM_S *pstIcfCalibOutput
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Ppm_DrvPrintCalibRslt(PPM_CALIB_OUTPUT_PRM_S *pstCalibMatrix)
{
    PPM_DRV_CHECK_PTR(pstCalibMatrix, err, "pstIcfCalibOutput == null!");

#if 0  /* 调试打印，当前联调阶段打开，后续关闭 */
    return;
#endif

    PPM_LOGW("R = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
             pstCalibMatrix->stR_MatrixInfo.fMatrix[0][0],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[0][1],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[0][2],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[1][0],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[1][1],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[1][2],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[2][0],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[2][1],
             pstCalibMatrix->stR_MatrixInfo.fMatrix[2][2]);

    PPM_LOGW("T = [%f, %f, %f] \n",
             pstCalibMatrix->stT_MatrixInfo.fMatrix[0][0],
             pstCalibMatrix->stT_MatrixInfo.fMatrix[0][1],
             pstCalibMatrix->stT_MatrixInfo.fMatrix[0][2]);

    PPM_LOGW("face_inv = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[0][0],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[0][1],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[0][2],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[1][0],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[1][1],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[1][2],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[2][0],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[2][1],
             pstCalibMatrix->stFaceCamInvMatrixInfo.fMatrix[2][2]);

    PPM_LOGW("tri_inv = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[0][0],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[0][1],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[0][2],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[1][0],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[1][1],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[1][2],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[2][0],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[2][1],
             pstCalibMatrix->stTriCamInvMatrixInfo.fMatrix[2][2]);
err:
    return;
}

/**
 * @function:   Ppm_DrvGetCalibRslt
 * @brief:      获取标定结果
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_CALIB_OUTPUT_PRM_S *pstCalibOutputPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetCalibRslt(UINT32 chan, PPM_CALIB_OUTPUT_PRM_S *pstCalibOutputPrm)
{
    INT32 s32CalibSts = 0;

    PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pstCalibOutputPrm, err, "pstCalibOutputPrm == null!");

	pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_DRV_CHECK_PTR(pstMccCalibChnPrm, err, "pstMccCalibChnPrm == null!");

    if (pstMccCalibChnPrm->stIcfCalibPrm.stCalibOptPrm.fRmsVal >= 3.0)
    {
        PPM_LOGE("calib failed! rms %f \n", pstMccCalibChnPrm->stIcfCalibPrm.stCalibOptPrm.fRmsVal);
        return PPM_GET_RMS_ERR;
    }

    s32CalibSts = pstMccCalibChnPrm->stIcfCalibPrm.stCalibOptPrm.s32CalibSts;
    if (s32CalibSts != SAL_SOK)
    {
        switch (s32CalibSts)
        {
            case PPM_CALIB_E_IMG_SIZE:
            {
                return PPM_MCC_E_IMG_SIZE;
                break;
            }
            case PPM_CALIB_E_NULL_POINT:
            {
                return PPM_MCC_E_NULL_POINT;
                break;
            }
            case PPM_CALIB_E_IMG_NUM:
            {
                return PPM_MCC_E_IMG_NUM;
                break;
            }
            case PPM_CALIB_E_PROC_IN_TYEP:
            {
                return PPM_MCC_E_PROC_IN_TYEP;
                break;
            }
            case PPM_CALIB_E_PROC_OUT_TYEP:
            {
                return PPM_MCC_E_PROC_OUT_TYEP;
                break;
            }
            case PPM_CALIB_E_CALIB_IMG_NUM:
            {
                return PPM_MCC_E_CALIB_IMG_NUM;
                break;
            }
            default:
            {
                return PPM_MCC_E_CALIB_IMG_NUM;
                break;
            }
        }
    }

    memcpy(pstCalibOutputPrm,
           &pstMccCalibChnPrm->stIcfCalibPrm.stCalibOptPrm.stMatrixInfo,
           sizeof(PPM_CALIB_OUTPUT_PRM_S));

    (VOID)Ppm_DrvPrintCalibRslt(pstCalibOutputPrm);

    return SAL_SOK;

err:
    return PPM_GET_RMS_ERR;
}

/**
 * @function:   Ppm_DrvGetFaceCalibRslt
 * @brief:      获取标定结果
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_FACE_CALIB_OUTPUT_PRM_S *pstCalibOutputPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetFaceCalibRslt(UINT32 chan, PPM_FACE_CALIB_OUTPUT_PRM_S *pstCalibOutputPrm)
{
    INT32 s32CalibSts = SAL_SOK;

    INT32 i = 0;
    INT32 j = 0;

	PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_PTR(pstCalibOutputPrm, err, "pstCalibOutputPrm == null!");

    pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_DRV_CHECK_PTR(pstFaceCalibChnPrm, err, "pstFaceCalibChnPrm == null!");

    pstCalibOutputPrm->fRms = pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm.fRmsVal;

    /* 算法结果阈值rms值需小于3.0 */
    if (pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm.fRmsVal >= 3.0)
    {
        PPM_LOGE("calib failed! rms %f \n", pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm.fRmsVal);
        goto err;
    }

    s32CalibSts = pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm.s32CalibSts;
    if (s32CalibSts != SAL_SOK)
    {
        switch (s32CalibSts)
        {
            case PPM_CALIB_E_IMG_SIZE:
            {
                return PPM_MCC_E_IMG_SIZE;
                break;
            }
            case PPM_CALIB_E_NULL_POINT:
            {
                return PPM_MCC_E_NULL_POINT;
                break;
            }
            case PPM_CALIB_E_IMG_NUM:
            {
                return PPM_MCC_E_IMG_NUM;
                break;
            }
            case PPM_CALIB_E_PROC_IN_TYEP:
            {
                return PPM_MCC_E_PROC_IN_TYEP;
                break;
            }
            case PPM_CALIB_E_PROC_OUT_TYEP:
            {
                return PPM_MCC_E_PROC_OUT_TYEP;
                break;
            }
            case PPM_CALIB_E_CALIB_IMG_NUM:
            {
                return PPM_MCC_E_CALIB_IMG_NUM;
                break;
            }
            default:
            {
                return PPM_MCC_E_CALIB_IMG_NUM;
                break;
            }
        }
    }

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 3; j++)
        {
            pstCalibOutputPrm->fRMatrix[i][j] = pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm.stMatrixInfo.stR_MatrixInfo.fMatrix[i][j];
        }
    }

    pstCalibOutputPrm->fRMatrix[2][0] = 0;
    pstCalibOutputPrm->fRMatrix[2][1] = 0;
    pstCalibOutputPrm->fRMatrix[2][2] = 1;
    for (i = 0; i < 8; i++)
    {
        pstCalibOutputPrm->fTMatrix[i] = pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm.stMatrixInfo.stT_MatrixInfo.fMatrix[0][i];
    }

#if 1
    PPM_LOGW("face inter = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
             pstCalibOutputPrm->fRMatrix[0][0],
             pstCalibOutputPrm->fRMatrix[0][1],
             pstCalibOutputPrm->fRMatrix[0][2],
             pstCalibOutputPrm->fRMatrix[1][0],
             pstCalibOutputPrm->fRMatrix[1][1],
             pstCalibOutputPrm->fRMatrix[1][2],
             pstCalibOutputPrm->fRMatrix[2][0],
             pstCalibOutputPrm->fRMatrix[2][1],
             pstCalibOutputPrm->fRMatrix[2][2]);

    PPM_LOGW("face distor = [%f, %f, %f, %f, %f, %f, %f, %f] \n",
             pstCalibOutputPrm->fTMatrix[0],
             pstCalibOutputPrm->fTMatrix[1],
             pstCalibOutputPrm->fTMatrix[2],
             pstCalibOutputPrm->fTMatrix[3],
             pstCalibOutputPrm->fTMatrix[4],
             pstCalibOutputPrm->fTMatrix[5],
             pstCalibOutputPrm->fTMatrix[6],
             pstCalibOutputPrm->fTMatrix[7]);
#endif
    return SAL_SOK;

err:
    return PPM_GET_RMS_ERR;
}

/**
 * @function:   Ppm_DrvSetCalibPrm
 * @brief:      输入标定参数，获取标定结果
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetCalibPrm(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_FAIL;
	
    UINT32 i = 0;
	
    PPM_CALIB_PRM_S *pstCalibPrm = NULL;
    PPM_CALIB_INPUT_PRM_S *pstCalibInputPrm = NULL;
    PPM_CALIB_OUTPUT_PRM_S *pstCalibOutputPrm = NULL;

    MTME_MCC_CABLI_IN_T stCablicInPrm = {0};

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_RET(NULL == pPrm, err, "pPrm == null!");

    pstCalibPrm = (PPM_CALIB_PRM_S *)pPrm;
    pstCalibInputPrm = &pstCalibPrm->stCalibInputPrm;
    pstCalibOutputPrm = &pstCalibPrm->stCalibOutputPrm;

    PPM_LOGW("from app:chan %d, face inter RowCnt %d, ColCnt %d, distor RowCnt %d, ColCnt %d\ntri inter RowCnt %d, ColCnt %d, distor RowCnt %d, ColCnt %d\n",
             chan,
             pstCalibInputPrm->stFaceCamCalibMatrixInfo.stCamInterPrm.uiRowCnt,
             pstCalibInputPrm->stFaceCamCalibMatrixInfo.stCamInterPrm.uiColCnt,
             pstCalibInputPrm->stFaceCamCalibMatrixInfo.stCamDistortMatrix.uiRowCnt,
             pstCalibInputPrm->stFaceCamCalibMatrixInfo.stCamDistortMatrix.uiColCnt,
             pstCalibInputPrm->stTriCamCalibMatrixInfo.stCamInterPrm.uiRowCnt,
             pstCalibInputPrm->stTriCamCalibMatrixInfo.stCamInterPrm.uiColCnt,
             pstCalibInputPrm->stTriCamCalibMatrixInfo.stCamDistortMatrix.uiRowCnt,
             pstCalibInputPrm->stTriCamCalibMatrixInfo.stCamDistortMatrix.uiColCnt);

    s32Ret = Ppm_DrvPrintfMatrix(&pstCalibInputPrm->stFaceCamCalibMatrixInfo.stCamInterPrm, "face calib inter");
    PPM_DRV_CHECK_RET(s32Ret, err, "face calib inter matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstCalibInputPrm->stFaceCamCalibMatrixInfo.stCamDistortMatrix, "face calib distor");
    PPM_DRV_CHECK_RET(s32Ret, err, "face calib distor matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstCalibInputPrm->stTriCamCalibMatrixInfo.stCamInterPrm, "tri calib inter");
    PPM_DRV_CHECK_RET(s32Ret, err, "tri calib inter matrix is NULL !!!");

    s32Ret = Ppm_DrvPrintfMatrix(&pstCalibInputPrm->stTriCamCalibMatrixInfo.stCamDistortMatrix, "tri calib distor");
    PPM_DRV_CHECK_RET(s32Ret, err, "tri calib distor matrix is NULL !!!");

    PPM_LOGW("boardInfo: horiCnt %d, vertCnt %d, fsquaLen %f, jpegCnt %d\n",
             pstCalibInputPrm->stCalibBoardInfo.uiHoriCnt,
             pstCalibInputPrm->stCalibBoardInfo.uiVertCnt,
             pstCalibInputPrm->stCalibBoardInfo.fSquaLen,
             pstCalibInputPrm->stCalibPicInfo.uiJpegCnt);

    for(i = 0; i < pstCalibInputPrm->stCalibPicInfo.uiJpegCnt; i++)
    {
        PPM_LOGW("jpg:jpegCnt %d, wxh [%d, %d], tri_picLen %d, addr %p, face_piclen %d, addr %p\n",
             i,
             pstCalibInputPrm->stCalibPicInfo.uiJpegW,
             pstCalibInputPrm->stCalibPicInfo.uiJpegH,
             pstCalibInputPrm->stCalibPicInfo.stTriCamCalibJpeg.stPicAttr[i].uiPicLen,
             pstCalibInputPrm->stCalibPicInfo.stTriCamCalibJpeg.stPicAttr[i].pcJpegAddr,
             pstCalibInputPrm->stCalibPicInfo.stFaceCamCalibJpeg.stPicAttr[i].uiPicLen,
             pstCalibInputPrm->stCalibPicInfo.stFaceCamCalibJpeg.stPicAttr[i].pcJpegAddr);
    }

    /* 填充矩阵参数 */
    s32Ret = Ppm_DrvGetMatrixInfo(pstCalibInputPrm, &stCablicInPrm);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_DrvGetMatrixInfo failed!");

    /* 填充标定板相关参数 */
    s32Ret = Ppm_DrvGetCalibBoardInfo(pstCalibInputPrm, &stCablicInPrm);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_DrvGetCalibBoardInfo failed!");

    /* 填充标定图片数据 */
    s32Ret = Ppm_DrvGetCalibPicInfo(pstCalibInputPrm, &stCablicInPrm);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_DrvGetCalibPicInfo failed!");

    /* 标定信息输入引擎 */
    s32Ret = Ppm_DrvInputCalibData(chan, &stCablicInPrm, pstCalibInputPrm);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_DrvInputCalibData failed!");

    /* 异步结果进行同步 */
    s32Ret = Ppm_DrvCalibSyncProc(chan, pstCalibInputPrm);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_DrvCalibSyncProc failed!");

    /* 获取标定结果数据 */
    s32Ret = Ppm_DrvGetCalibRslt(chan, pstCalibOutputPrm);
    if (s32Ret != SAL_SOK)
    {
        PPM_LOGE("error s32Ret 0x%x \n", s32Ret);
        return s32Ret;
    }

    PPM_LOGI("set calib prm end! chan %d \n", chan);
    return SAL_SOK;

err:
    return s32Ret;
}

/**
 * @function:   Ppm_DrvSetFaceCalibPrm
 * @brief:      输入人脸相机标定参数，获取标定结果
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvSetFaceCalibPrm(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_SOK;

    PPM_FACE_CALIB_PRM_S *pstFaceCalibPrm = NULL;
    PPM_FACE_CALIB_INPUT_PRM_S *pstFaceCalibInputPrm = NULL;
    PPM_FACE_CALIB_OUTPUT_PRM_S *pstFaceCalibOutputPrm = NULL;

    MTME_MCC_SV_CABLI_IN_T stCalibIn = {0};

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);
    PPM_DRV_CHECK_RET(NULL == pPrm, err, "pPrm == null!");

    pstFaceCalibPrm = (PPM_FACE_CALIB_PRM_S *)pPrm;
    pstFaceCalibInputPrm = &pstFaceCalibPrm->stFaceCalibInputPrm;
    pstFaceCalibOutputPrm = &pstFaceCalibPrm->stFaceCalibOutputPrm;

    pstFaceCalibInputPrm->stCamInfo.uiFocalLength = MTME_MCC_STC_FL_4;
    pstFaceCalibInputPrm->stCamInfo.pixelSize = 2.9;

    PPM_LOGW("from app: chan %d, face focalLength %x, pixelSize %f, jpegCnt %d, wxh [%d, %d], RowCnt %d, ColCnt %d, fSquaLen %f \n",
             chan,
             pstFaceCalibInputPrm->stCamInfo.uiFocalLength,
             pstFaceCalibInputPrm->stCamInfo.pixelSize,
             pstFaceCalibInputPrm->stCalibPicInfo.uiJpegCnt,
             pstFaceCalibInputPrm->stCalibPicInfo.uiJpegW,
             pstFaceCalibInputPrm->stCalibPicInfo.uiJpegH,
             pstFaceCalibInputPrm->stCalibBoardInfo.uiHoriCnt,
             pstFaceCalibInputPrm->stCalibBoardInfo.uiVertCnt,
             pstFaceCalibInputPrm->stCalibBoardInfo.fSquaLen);

    /* 填充相机参数 */
    s32Ret = Ppm_DrvGetFaceCalibInfo(pstFaceCalibInputPrm, &stCalibIn);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "get face prm failed!");

    /* 填充标定板相关参数 */
    s32Ret = Ppm_DrvGetFaceCalibBoardInfo(pstFaceCalibInputPrm, &stCalibIn);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "get Calib Board Info failed!");

    /* 填充标定图片数据 */
    s32Ret = Ppm_DrvGetFaceCalibPicInfo(pstFaceCalibInputPrm, &stCalibIn);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "get calib pic info failed!");

    /* 标定信息输入引擎 */
    s32Ret = Ppm_DrvInputFaceCalibData(chan, &stCalibIn, pstFaceCalibInputPrm);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "input calib date failed!");

    /* 异步结果进行同步 */
    s32Ret = Ppm_DrvFaceCalibSyncProc(chan);
    PPM_DRV_CHECK_RET(s32Ret != SAL_SOK, err, "sync calib proc failed!");

    /* 获取标定结果数据 */
    s32Ret = Ppm_DrvGetFaceCalibRslt(chan, pstFaceCalibOutputPrm);
    if (s32Ret != SAL_SOK)
    {
        (VOID)Ppm_DrvFacePutJdecFrameData(pstFaceCalibInputPrm->stCalibPicInfo.uiJpegCnt);
        (VOID)Ppm_DrvFaceResetJdecPrm();
        PPM_LOGE("error s32Ret 0x%x \n", s32Ret);
        goto err;
    }

    /* 中间资源进行释放 */
    (VOID)Ppm_DrvFacePutJdecFrameData(pstFaceCalibInputPrm->stCalibPicInfo.uiJpegCnt);
    (VOID)Ppm_DrvFaceResetJdecPrm();

    PPM_LOGI("set face cam calib prm end! chan %d \n", chan);
    return SAL_SOK;

err:
    return s32Ret;
}

/* #ifdef PPM_SYNC_V0 */

/**
 * @function:   Ppm_DrvGetDupChnHandle
 * @brief:      获取特定实例对应的输出通道
 * @param[in]:  CHAR *pcInstName
 * @param[in]:  UINT32 u32OutChn
 * @param[out]: DUP_ChanHandle *phDupChnHdl
 * @return:     INT32
 */
static INT32 Ppm_DrvGetDupChnHandle(CHAR *pcInstName, CHAR *pcOutNodeName, DUP_ChanHandle *phDupChnHdl)
{
    INST_INFO_S *pstInstInfo = NULL;
    VOID *pDupChnHandle = NULL;

    PPM_DRV_CHECK_PTR(pcInstName, err, "pcInstName == null!");
    PPM_DRV_CHECK_PTR(pcOutNodeName, err, "pcOutNodeName == null!");
    PPM_DRV_CHECK_PTR(phDupChnHdl, err, "phDupChnHdl == null!");

    pstInstInfo = link_drvGetInst(pcInstName);
    PPM_DRV_CHECK_PTR(pstInstInfo, err, "get pstInstInfo == null!");

    pDupChnHandle = link_drvGetHandleFromNode(pstInstInfo, pcOutNodeName);
    PPM_DRV_CHECK_PTR(pDupChnHandle, err, "get pDupChnHandle == null!");

    *phDupChnHdl = *(DUP_ChanHandle *)pDupChnHandle;

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvPutChnFrame
 * @brief:      释放VPSS通道数据
 * @param[in]:  UINT32 uiVpssGrp
 * @param[in]:  UINT32 uiVpssChn
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrameInfo
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvPutChnFrame(UINT32 chan, SYSTEM_FRAME_INFO *pstSysFrameInfo)
{
    INT32 s32Ret = 0;

    CHAR acInstName[64] = {0};
    CHAR acOutNodeName[64] = {'\0'};

    DUP_ChanHandle hDupChnHdl = {0};
    DUP_COPY_DATA_BUFF stDupDataBuf = {0};

    PPM_DRV_CHECK_PTR(pstSysFrameInfo, err, "pstSysFrameInfo == null!");

    if (0x00 == pstSysFrameInfo->uiAppData)
    {
        SAL_ERROR("is null\n");
        goto err;
    }

    snprintf(acInstName, 64, "DUP_REAR_%d", chan);
    snprintf(acOutNodeName, 64, "out_sva");

    s32Ret = Ppm_DrvGetDupChnHandle(acInstName, acOutNodeName, &hDupChnHdl);
    PPM_DRV_CHECK_RET(s32Ret, err, "get dup chan handle failed!");

    stDupDataBuf.pstDstSysFrame = pstSysFrameInfo;

    s32Ret = hDupChnHdl.dupOps.OpDupPutBlit((Ptr) & hDupChnHdl, (Ptr) & stDupDataBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "get chan frame failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetChnFrame
 * @brief:      获取VPSS通道数据
 * @param[in]:  UINT32 uiVpssGrp
 * @param[in]:  UINT32 uiVpssChn
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrameInfo
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_DrvGetChnFrame(UINT32 chan,
                                SYSTEM_FRAME_INFO *pstSysFrameInfo,
                                PPM_VIDEO_FRM_PTS_OP_E enFillFlag,
                                BOOL bCopyData)
{
    INT32 s32Ret = 0;

    static SYSTEM_FRAME_INFO stSysFrameInfo = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    DUP_ChanHandle hDupChnHdl = {0};
    DUP_COPY_DATA_BUFF stDupDataBuf = {0};

    CHAR acInstName[64] = {'\0'};
    CHAR acOutNodeName[64] = {'\0'};

    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;

    PPM_DRV_CHECK_PTR(pstSysFrameInfo, err, "pstSysFrameInfo == null!");

    if (0x00 == pstSysFrameInfo->uiAppData)
    {
        SAL_ERROR("tmp buf err! \n");
        goto err;
    }

    if (0x00 == stSysFrameInfo.uiAppData)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stSysFrameInfo);
        PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_allocVideoFrameInfoSt failed!");
    }

    pstSysFrmInfo = (bCopyData ? &stSysFrameInfo : pstSysFrameInfo);

    snprintf(acInstName, 64, "DUP_REAR_%d", chan);
    snprintf(acOutNodeName, 64, "out_sva");

    s32Ret = Ppm_DrvGetDupChnHandle(acInstName, acOutNodeName, &hDupChnHdl);
    PPM_DRV_CHECK_RET(s32Ret, err, "get dup chan handle failed!");

    stDupDataBuf.pstDstSysFrame = pstSysFrmInfo;

    s32Ret = hDupChnHdl.dupOps.OpDupGetBlit((Ptr) & hDupChnHdl, (Ptr) & stDupDataBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "get chan frame failed!");

    s32Ret = sys_hal_getVideoFrameInfo(pstSysFrmInfo, &stVideoFrmBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_getVideoFrameInfo failed!");

    /* 获取上层应用码流同步后的时间戳，目前使用gettimeofday */
    stVideoFrmBuf.pts = (PPM_FILL_PTS_OP == enFillFlag) ? SAL_getTimeOfDayMs() : stVideoFrmBuf.pts;

    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSysFrmInfo);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_buildVideoFrame failed!");

    if (bCopyData)
    {
        s32Ret = Ia_TdeQuickCopy(&stSysFrameInfo, pstSysFrameInfo, 0, 0, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, SAL_FALSE);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("PPM: Tde Copy fail! ret: 0x%x \n", s32Ret);
            /* 失败不返回，需要释放VB */
        }

        s32Ret = hDupChnHdl.dupOps.OpDupPutBlit((Ptr) & hDupChnHdl, (Ptr) & stDupDataBuf);
        PPM_DRV_CHECK_RET(s32Ret, err, "release chan frame failed!");
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/* #endif */


/**
 * @function:   Ppm_DrvGetVdecFrm
 * @brief:      获取解码数据
 * @param[in]:  UINT32 uiVdecChn
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetVdecFrm(UINT32 u32VdecChn, UINT32 u32OutChn, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32Width, UINT32 u32Height)
{
    INT32 s32Ret = SAL_FAIL;

    CHAR acInstName[64] = {'\0'};
    CHAR *pcLinkOutNode[7] = {"out_disp0", "out_disp1", "out_jpeg", "out_ba", "out_ppm_left", "out_ppm_right", "out_ppm_mid"};

    DUP_ChanHandle hDupChnHdl = {0};

    static SYSTEM_FRAME_INFO stFrameInfo[16] = {0};    /* 人包关联最大的解码通道号为4，故此处使用5 */
    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    DUP_COPY_DATA_BUFF stDupDataBuf = {0};
    PARAM_INFO_S stDupChnParam = {0};

    if (!pstFrame)
    {
        SAL_ERROR("pstFrame == NULL! \n");
        return SAL_FAIL;
    }

    if (0x00 == stFrameInfo[u32VdecChn].uiAppData)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stFrameInfo[u32VdecChn]);
        PPM_DRV_CHECK_RET(s32Ret, err, "Sva_DrvGetFrameMem failed!");
    }

    snprintf(acInstName, 64, "DUP_VDEC_%d", u32VdecChn);

    s32Ret = Ppm_DrvGetDupChnHandle(acInstName, pcLinkOutNode[u32OutChn], &hDupChnHdl);
    PPM_DRV_CHECK_RET(s32Ret, err, "get dup chan handle failed!");

    stDupChnParam.enType = IMAGE_SIZE_CFG;
    s32Ret = hDupChnHdl.dupOps.OpDupGetBlitPrm((Ptr) & hDupChnHdl, (Ptr)&stDupChnParam);
    PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");

    if (stDupChnParam.stImgSize.u32Width != u32Width && stDupChnParam.stImgSize.u32Height != u32Height)
    {
        s32Ret = vdec_tsk_SetOutChnResolution(u32VdecChn, u32OutChn, u32Width, u32Height);
        PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_ScaleJpegChn failed!");
    }

    sal_memset_s(&stDupChnParam, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));
    stDupChnParam.enType = PIX_FORMAT_CFG;
    stDupChnParam.enPixFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = hDupChnHdl.dupOps.OpDupSetBlitPrm((Ptr) & hDupChnHdl, (Ptr)&stDupChnParam);
    PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");

    stDupDataBuf.pstDstSysFrame = &stFrameInfo[u32VdecChn];

    s32Ret = hDupChnHdl.dupOps.OpDupGetBlit((Ptr) & hDupChnHdl, (Ptr) & stDupDataBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "get chan frame failed!");

    s32Ret = Ia_TdeQuickCopy(&stFrameInfo[u32VdecChn],
                             pstFrame,
                             0, 0,
                             stFrameInfo[u32VdecChn].uiDataWidth, stFrameInfo[u32VdecChn].uiDataHeight, SAL_FALSE);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("PPM: Tde Copy fail! vdec %d, ret: 0x%x \n", u32VdecChn, s32Ret);
        goto putdata;
    }

    s32Ret = hDupChnHdl.dupOps.OpDupPutBlit((Ptr) & hDupChnHdl, (Ptr) & stDupDataBuf);;
    PPM_DRV_CHECK_RET(s32Ret, err, "put chan frame failed!");

    (VOID)sys_hal_getVideoFrameInfo(pstFrame, &stVideoFrmBuf);

    if (stVideoFrmBuf.frameParam.width != PPM_TRI_CAM_CHN_WIDTH || stVideoFrmBuf.frameParam.height != PPM_TRI_CAM_CHN_HEIGHT)
    {
        /* return SAL_FAIL; */
    }

    return SAL_SOK;

putdata:
    (VOID)hDupChnHdl.dupOps.OpDupPutBlit((Ptr) & hDupChnHdl, (Ptr) & stDupDataBuf);;
err:
    return SAL_FAIL;
}

static UINT32 uiIdx[5] = {0};
static PPM_COM_FRAME_INFO_ST *pstComFrm[2] = {NULL};

/**
 * @function    Ppm_DrvDumpNv21
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Ppm_DrvDumpNv21(VOID *pVir, UINT32 size, UINT32 type)
{
    FILE *fp = NULL;
    CHAR path[64] = {0};
    CHAR *pType[5] = {"left", "right", "middle","face", "tri"};
    CHAR *pPreFix = "/mnt/ppm/"; /* "./ppm_dump"; */

    sprintf(path, "%s/%d_%s.nv21", pPreFix, uiIdx[type], pType[type]);

    fp = fopen(path, "w+");
    if (NULL == fp)
    {
        PPM_LOGE("fopen failed! %s \n", pType[type]);
        return SAL_FAIL;
    }

    fwrite(pVir, size, 1, fp);
    fflush(fp);

    fclose(fp);
    fp = NULL;

    uiIdx[type]++;

    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvGetProcFrm
 * @brief:      获取裁剪放缩处理后的数据
 * @param[in]:  UINT32 uiVpssGrp
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvGetProcFrm(UINT32 chan, UINT32 u32OutChn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_DEV_PRM *pstDevPrm = NULL;
    DUP_ChanHandle *phDupChnHandle = NULL;

    static SYSTEM_FRAME_INFO stFrameInfo = {0};
    DUP_COPY_DATA_BUFF stDupCopyDataBuf = {0};
    DUP_ChanHandle *pDupChnHandle = NULL;

    PPM_DRV_CHECK_PTR(pstFrame, err, "pstFrame == null!");
    if (u32OutChn >= PPM_HANDLE_MAX_NUM)
    {
        SAL_ERROR("invalid out chn %d, chan %d \n", u32OutChn, chan);
        return SAL_FAIL;
    }

    if (0x00 == stFrameInfo.uiAppData)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stFrameInfo);
        PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_allocVideoFrameInfoSt failed!");
    }

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    phDupChnHandle = (DUP_ChanHandle *)pstDevPrm->paDupChnHandle[u32OutChn];

    stDupCopyDataBuf.pstDstSysFrame = &stFrameInfo;
    s32Ret = phDupChnHandle->dupOps.OpDupGetBlit(phDupChnHandle, &stDupCopyDataBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "get chn frame failed!");

    pDupChnHandle = (DUP_ChanHandle *)Ppm_DrvGetVprocHandle(0, u32OutChn);
    PPM_DRV_CHECK_PTR(pDupChnHandle, err, "pDupChnHandle == null!");

#if 0
    sal_memset_s(&stDupChnParam, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));
    stDupChnParam.enType = CACHE_FLUSH;
    stDupChnParam.MbSize = stVideoFrmBuf.privateDate;
    s32Ret = pDupChnHandle->dupOps.OpDupGetBlitPrm(pDupChnHandle, &stDupChnParam);
    PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");
#endif

    s32Ret = Ia_TdeQuickCopy(&stFrameInfo,
                             pstFrame,
                             0, 0,
                             stFrameInfo.uiDataWidth, stFrameInfo.uiDataHeight, SAL_FALSE);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("PPM: Tde Copy fail! ret: 0x%x \n", s32Ret);
        goto rls;
    }

#if 0
    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    s32Ret = sys_hal_getVideoFrameInfo(&stFrameInfo, &stVideoFrmBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_getVideoFrameInfo failed!");

    (VOID)Ppm_DrvDumpNv21((VOID *)stVideoFrmBuf.virAddr[0],
                          stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height * 3 / 2, u32OutChn);

    (VOID)sys_hal_getVideoFrameInfo(pstFrame, &stVideoFrmBuf);
    (VOID)Ppm_DrvDumpNv21((VOID *)stVideoFrmBuf.virAddr[0],
                          stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height * 3 / 2, u32OutChn);
#endif

    s32Ret = phDupChnHandle->dupOps.OpDupPutBlit(phDupChnHandle, &stDupCopyDataBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "put chn frame failed!");

    return SAL_SOK;

rls:
    s32Ret = phDupChnHandle->dupOps.OpDupPutBlit(phDupChnHandle, &stDupCopyDataBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "put chn frame failed!");

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetSplitFrame
 * @brief:      获取分割后的帧数据
 * @param[in]:  UINT32 chan
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrame
 * @param[out]: PPM_IPT_YUV_DATA_INFO *pstOutFrm
 * @return:     static INT32
 */
INT32 Ppm_DrvGetSplitFrame(UINT32 chan, SYSTEM_FRAME_INFO *pstFrame, PPM_IPT_YUV_DATA_INFO *pstOutFrm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    DUP_ChanHandle *phDupChnHandle = NULL;

    if (chan > MAX_PPM_CHN_NUM - 1 || NULL == pstFrame || NULL == pstOutFrm)
    {
        SAL_ERROR("invalid input arg! chan %d, %p, %p \n", chan, pstFrame, pstOutFrm);
        goto err;
    }

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    phDupChnHandle = (DUP_ChanHandle *)pstDevPrm->paDupChnHandle[0];
    s32Ret = dup_task_sendToDup(phDupChnHandle->dupModule, pstFrame);
    PPM_DRV_CHECK_RET(s32Ret, err, "dup_task_sendToDup failed!");

#if 0
    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    (VOID)sys_hal_getVideoFrameInfo(pstFrame, &stVideoFrmBuf);
    (VOID)Ppm_DrvDumpNv21((VOID *)stVideoFrmBuf.virAddr[0],
    stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height * 3 / 2, 4);
#endif

    for (i = 0; i < PPM_TRI_CAM_OUT_CHN_NUM; i++)
    {
        s32Ret = Ppm_DrvGetProcFrm(chan, i, &pstOutFrm->stTriViewYuv[i]);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetProcFrm failed!");

        PPM_LOGT("Get Split frame %d end! \n", i);
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvPackPlatVideoSt
 * @brief:      填充海思平台使用的视频帧数据结构体
 * @param[in]:  VCA_YUV_DATA *pstYuvData
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrmInfo
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_DrvPackPlatVideoSt(MTME_YUV_DATA_T *pstYuvData, SYSTEM_FRAME_INFO *pstSysFrmInfo)
{
    INT32 s32Ret = SAL_SOK;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    PPM_DRV_CHECK_PTR(pstYuvData, err, "pstYuvData == null!");
    PPM_DRV_CHECK_PTR(pstSysFrmInfo, err, "pstSysFrmInfo == null!");

    s32Ret = sys_hal_getVideoFrameInfo(pstSysFrmInfo, &stVideoFrmBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_getVideoFrameInfo failed!");

    stVideoFrmBuf.frameParam.width = pstYuvData->image_w;
    stVideoFrmBuf.frameParam.height = pstYuvData->image_h;
    stVideoFrmBuf.stride[0] = pstYuvData->image_w;
    stVideoFrmBuf.stride[1] = pstYuvData->image_w;
    stVideoFrmBuf.stride[2] = pstYuvData->image_w;
    stVideoFrmBuf.virAddr[0] = (PhysAddr)pstYuvData->y;
    stVideoFrmBuf.virAddr[1] = (PhysAddr)pstYuvData->v;
    stVideoFrmBuf.virAddr[2] = (PhysAddr)pstYuvData->u;

    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSysFrmInfo);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_buildVideoFrame failed!");

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvJudgeRsltExist
 * @brief:      判断本次回调数据中是否存在有效检测结果
 * @param[in]:  PPM_RSLT_INFO_S *pstRsltInfo
 * @param[out]: None
 * @return:     static BOOL
 */
static BOOL Ppm_DrvJudgeRsltExist(PPM_RSLT_INFO_S *pstRsltInfo)
{
    BOOL bFlag = SAL_FALSE;

    PPM_DRV_CHECK_PTR(pstRsltInfo, exit, "pstRsltInfo == null!");

    if (pstRsltInfo->stXsiRsltOut.uiXsiCnt > 0
        || pstRsltInfo->stPkgRsltOut.uiPkgCnt > 0
        || pstRsltInfo->stFaceRsltOut.uiFaceCnt > 0)
    {
        bFlag = SAL_TRUE;
    }

exit:
    return bFlag;
}

/**
 * @function:   Ppm_DrvPutFrameMem
 * @brief:      释放帧内存
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrmInfo
 * @param[out]: None
 * @return:     INT32
 */
static INT32 ATTRIBUTE_UNUSED Ppm_DrvPutFrameMem(SYSTEM_FRAME_INFO *pstSysFrmInfo)
{
    INT32 s32Ret = SAL_SOK;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    PPM_DRV_CHECK_PTR(pstSysFrmInfo, err, "pstSysFrmInfo == null!");

    if (0x00 == pstSysFrmInfo->uiAppData)
    {
        PPM_LOGE("pstSysFrmInfo not malloc st! \n");
        return SAL_FAIL;
    }

    s32Ret = sys_hal_getVideoFrameInfo(pstSysFrmInfo, &stVideoFrmBuf);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_allocVideoFrameInfoSt failed!");

    s32Ret = mem_hal_mmzFree((Ptr)stVideoFrmBuf.virAddr[0], "PPM", "frame mem");
    PPM_DRV_CHECK_RET(s32Ret, err, "mem_hal_mmzFree failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetFrameMem
 * @brief:      获取帧内存
 * @param[in]:  UINT32 u32Width
 * @param[in]:  UINT32 u32Height
 * @param[out]: SYSTEM_FRAME_INFO *pstSysFrmInfo
 * @return:     INT32
 */
static INT32 Ppm_DrvGetFrameMem(UINT32 u32Width, UINT32 u32Height, SYSTEM_FRAME_INFO *pstSysFrmInfo)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 u32FrmSize = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
	ALLOC_VB_INFO_S stAllocVbInfo = {0};

    PPM_DRV_CHECK_PTR(pstSysFrmInfo, err, "pstSysFrmInfo == null!");
    if (0x00 == pstSysFrmInfo->uiAppData)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(pstSysFrmInfo);
        PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_allocVideoFrameInfoSt failed!");
    }

    u32FrmSize = u32Width * u32Height * 3 / 2;

    s32Ret = mem_hal_vbAlloc(u32FrmSize, "PPM", "frame mem", NULL, SAL_FALSE, &stAllocVbInfo);
    PPM_DRV_CHECK_RET(s32Ret, err, "mem_hal_vbAlloc failed!");

    stVideoFrmBuf.poolId = stAllocVbInfo.u32PoolId;
	stVideoFrmBuf.vbBlk = stAllocVbInfo.u64VbBlk;

    stVideoFrmBuf.frameParam.width = u32Width;
    stVideoFrmBuf.frameParam.height = u32Height;
    stVideoFrmBuf.stride[0] = u32Width;
    stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[0];
    stVideoFrmBuf.stride[2] = stVideoFrmBuf.stride[0];

    stVideoFrmBuf.virAddr[0] = (PhysAddr)stAllocVbInfo.pVirAddr;
    stVideoFrmBuf.virAddr[1] = (PhysAddr)((CHAR *)stVideoFrmBuf.virAddr[0] + u32Width * u32Height);
    stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];

    stVideoFrmBuf.phyAddr[0] = (PhysAddr)stAllocVbInfo.u64PhysAddr;
    stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + u32Width * u32Height;
    stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

    stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSysFrmInfo);
    PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_allocVideoFrameInfoSt failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetPkgResult
 * @brief:      获取匹配的包裹信息
 * @param[in]:  VOID *pstOutput
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_DrvGetPkgResult(VOID *pstOutput, PPM_PKG_RSLT_S *pstPkgOut)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;    /* 结果计数 */
    UINT32 uiImgW = 0;
    UINT32 uiImgH = 0;
    UINT32 uiPkgId = 0;
    UINT32 uiFrmNum = 0;
    UINT32 uiObjNum = 0;
    UINT32 uiInputTime = 0;
    UINT32 uiCpyStartTime = 0;
    UINT32 uiOutputTime = 0;

    INT64L llFrmNum = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    VOID *pstYData = NULL;
    VOID *pstUData = NULL;
    VOID *pstVData = NULL;
    MTME_PACK_MATCH_OUT_T *pstPkgMatchOut = NULL;
    MTME_PACK_MATCH_INFO_T *pstPkgMatchInfo = NULL;
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_PKG_OBJ_INFO_S *pstPkgObjInfo = NULL;
    ICF_MEDIA_INFO *pstMediaInfo = NULL;
    PPM_INPUT_DATA_INFO *pstInputDataInfo = NULL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    XPACK_IMG stFrameDataXpackCopySrc = {0};
    XPACK_IMG stFrameDataXpackCopyDst = {0};

    pstPpmGlobalPrm = Ppm_HalGetModPrm();

    PPM_DRV_CHECK_PTR(pstOutput, exit, "pstOutput == null!");
    PPM_DRV_CHECK_PTR(pstPkgOut, exit, "pstPkgOut == null!");
    PPM_DRV_CHECK_RET(pstPpmGlobalPrm == NULL, exit, "Ppm_HalGetModPrm failed!");

    pstMediaInfo = (ICF_MEDIA_INFO *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    PPM_DRV_CHECK_PTR(pstMediaInfo, exit, "pstMediaInfo == null!");

    llFrmNum = (INT64L)pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum;
    PPM_LOGD("get frmNum result %lld \n", llFrmNum);

    pstInputDataInfo = (PPM_INPUT_DATA_INFO *)pstMediaInfo->stInputInfo.pUserPtr;
    uiInputTime = pstInputDataInfo->stUsrPrvtInfo.uiIptTime;

	/* 获取包裹关联结果 */
	pstPkgMatchOut	= (MTME_PACK_MATCH_OUT_T *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_MATCH_PACK_DATA);
    PPM_DRV_CHECK_PTR(pstPkgMatchOut, exit, "pstPkgMatchOut == null!");

    pstPkgOut->uiPkgCnt = pstPkgMatchOut->obj_num;

#ifdef DEBUG_DUMP_DATA
    char path[64] = {0};
    char *pData = NULL;

    UINT32 uiSize = 0;
#endif

    uiObjNum = pstPkgMatchOut->obj_num;
    if (uiObjNum > 0)
    {
        PPM_LOGI("pack_match_result num is %d\n", uiObjNum);
    }

    /* debug: 告警当前buf cnt不够 */
    if (uiObjNum > PPM_MAX_DET_OBJ_NUM)
    {
        PPM_LOGE("obj num > max buf cnt 16!!! \n");
    }

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    for (i = 0; i < uiObjNum && i < PPM_MAX_DET_OBJ_NUM; i++)
    {
        pstPkgMatchInfo = &pstPkgMatchOut->pack_match_info[i];
        pstPkgObjInfo = &pstPkgOut->stPkgObjInfo[i];

        pstPkgObjInfo->uiMatchId = pstPkgMatchInfo->id;

        if (0x00 == pstPkgObjInfo->stFrameInfo.uiAppData)
        {
            s32Ret = Ppm_DrvGetFrameMem(PPM_FACE_WIDTH, PPM_FACE_HEIGHT, &pstPkgObjInfo->stFrameInfo);
            PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_DrvGetFrameMem failed!");
        }

#ifdef DEBUG_CPU_CPY  /* debug */
        UINT32 uiSize = 0;

        s32Ret = sys_hal_getVideoFrameInfo(&pstPkgObjInfo->stFrameInfo, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

        uiSize = pstPkgMatchInfo->pack_img.image_w * pstPkgMatchInfo->pack_img.image_h * 3 / 2;

        memcpy((VOID *)stVideoFrmBuf.virAddr[0], pstPkgMatchInfo->pack_img.y, uiSize);

        pstPkgObjInfo->stFrameInfo.uiDataWidth = pstPkgMatchInfo->pack_img.image_w;
        pstPkgObjInfo->stFrameInfo.uiDataHeight = pstPkgMatchInfo->pack_img.image_h;

        SAL_WARN("pkg info: w %d, h %d \n", pstPkgObjInfo->stFrameInfo.uiDataWidth, pstPkgObjInfo->stFrameInfo.uiDataHeight);

        /* 清空旧数据 */
        memset((VOID *)stVideoFrmBuf.virAddr[0], 0x80, PPM_FACE_WIDTH * PPM_FACE_HEIGHT * 3 / 2);
#else
        //s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstDevPrm->stTmpFrame[0]);
        //PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_buildVideoFrame failed!");

        s32Ret = Ppm_DrvPackPlatVideoSt(&pstPkgMatchInfo->pack_img, &pstDevPrm->stTmpFrame[0]);
        PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_DrvPackPlatVideoSt failed!");

        s32Ret = sys_hal_getVideoFrameInfo(&pstDevPrm->stTmpFrame[0], &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

        PPM_LOGW("pack video st: w %d, h %d, s %d, vir %p, %p, phy %p, %p \n",
                 stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, stVideoFrmBuf.stride[0],
                 (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1],
                 (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1]);

        uiCpyStartTime = SAL_getCurMs();


		/*拷贝帧数据*/
        s32Ret = sys_hal_getVideoFrameInfo(&pstDevPrm->stTmpFrame[0], &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");
        
        stFrameDataXpackCopySrc.picPrm.u32Width = stVideoFrmBuf.frameParam.width;
        stFrameDataXpackCopySrc.picPrm.u32Height = stVideoFrmBuf.frameParam.height;
        stFrameDataXpackCopySrc.picPrm.u32Stride = stVideoFrmBuf.stride[0];
        stFrameDataXpackCopySrc.picPrm.phyAddr[0] = stVideoFrmBuf.phyAddr[0];
        stFrameDataXpackCopySrc.picPrm.VirAddr[0] = (VOID *)stVideoFrmBuf.virAddr[0];
        stFrameDataXpackCopySrc.picPrm.VirAddr[1] = (VOID *)stVideoFrmBuf.virAddr[1];
        stFrameDataXpackCopySrc.picPrm.enPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
        stFrameDataXpackCopySrc.picPrm.vbBlk = stVideoFrmBuf.vbBlk;

        s32Ret = sys_hal_getVideoFrameInfo(&pstPkgObjInfo->stFrameInfo, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");
        stFrameDataXpackCopyDst.picPrm.u32Width = pstPkgMatchInfo->pack_img.image_w;
        stFrameDataXpackCopyDst.picPrm.u32Stride = stVideoFrmBuf.frameParam.width;
        stFrameDataXpackCopyDst.picPrm.u32Height = pstPkgMatchInfo->pack_img.image_h;
        stFrameDataXpackCopyDst.picPrm.phyAddr[0] = stVideoFrmBuf.phyAddr[0];
        stFrameDataXpackCopyDst.picPrm.VirAddr[0] = (VOID *)stVideoFrmBuf.virAddr[0];
        stFrameDataXpackCopyDst.picPrm.VirAddr[1] = (VOID *)stVideoFrmBuf.virAddr[1];
        stFrameDataXpackCopyDst.picPrm.enPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
        stFrameDataXpackCopyDst.picPrm.vbBlk = stVideoFrmBuf.vbBlk;

        s32Ret = SAL_halDataCopy(&stFrameDataXpackCopySrc, &stFrameDataXpackCopyDst);
        PPM_DRV_CHECK_RET(s32Ret, exit, "HAL_bufQuickCopyVideo failed!");

        pstPkgObjInfo->stFrameInfo.uiDataWidth = pstPkgMatchInfo->pack_img.image_w;
        pstPkgObjInfo->stFrameInfo.uiDataHeight = pstPkgMatchInfo->pack_img.image_h;
#if 0
        s32Ret = Ia_TdeQuickCopy(&pstDevPrm->stTmpFrame[0],
                                 &pstPkgObjInfo->stFrameInfo,
                                 0, 0,
                                 stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.height, SAL_FALSE);
        PPM_DRV_CHECK_RET(s32Ret, exit, "HAL_bufQuickCopyVideo failed!");
#endif
#if 0
        char path[64] = {0};
        char *pData = NULL;
        
        UINT32 uiSize = 0;

        memset(path, 0x00, 64 * sizeof(char));

        sprintf(path, "/mnt/nfs/cb_%d_%d_pkg_%dx%d.nv21", pstPkgMatchInfo->frame_num,pstPkgMatchInfo->pack_id, pstPkgMatchInfo->pack_img.image_w, pstPkgMatchInfo->pack_img.image_h);

        pData = (VOID *)stVideoFrmBuf.virAddr[0];
        uiSize = pstPkgMatchInfo->pack_img.image_w * pstPkgMatchInfo->pack_img.image_h * 3 / 2;

        (VOID)Ppm_HalDebugDumpData((VOID *)pData, path, uiSize, 0);
        SAL_WARN("fwrite end! \n");

        memset(path, 0x00, 64 * sizeof(char));

        sprintf(path, "/mnt/nfs/cb_copy_%d_%d_pkg_%dx%d.nv21",pstPkgMatchInfo->frame_num, pstPkgMatchInfo->pack_id, pstPkgMatchInfo->pack_img.image_w, pstPkgMatchInfo->pack_img.image_h);
        s32Ret = sys_hal_getVideoFrameInfo(&pstPkgObjInfo->stFrameInfo, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

        pData = (VOID *)stVideoFrmBuf.virAddr[0];
        uiSize = pstPkgMatchInfo->pack_img.image_w * pstPkgMatchInfo->pack_img.image_h * 3 / 2;

        (VOID)Ppm_HalDebugDumpData((VOID *)pData, path, uiSize, 0);
        SAL_WARN("fwrite end! \n");
#endif
#endif

        s32Ret = sys_hal_getVideoFrameInfo(&pstPkgObjInfo->stFrameInfo, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

        SAL_WARN("pkg obj Info: w %d, h %d, s %d, vir %p, %p, phy %p, %p \n",
                 stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, stVideoFrmBuf.stride[0],
                 (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1],
                 (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1]);


#if 1  /* debug: log */
        uiPkgId = pstPkgMatchInfo->pack_id;
        uiFrmNum = pstPkgMatchInfo->frame_num;

        uiImgH = pstPkgMatchInfo->pack_img.image_h;
        uiImgW = pstPkgMatchInfo->pack_img.image_w;
        pstYData = pstPkgMatchInfo->pack_img.y;
        pstUData = pstPkgMatchInfo->pack_img.u;
        pstVData = pstPkgMatchInfo->pack_img.v;

        PPM_DRV_CHECK_PTR(pstYData, exit, "pstYData == null!");

        /* sprintf(szPkgPath, "%s/pack-match-obj%d-pack%d-f%d-w%d_h%d.nv21",DEMO_SAVE_DATA_PATH, */
        /*        uiObjId, uiPkgId, uiFrmNum, uiImgW, uiImgH); */

        uiOutputTime = SAL_getCurMs();

        PPM_LOGW("pack-match-obj: id %d, pack %d, frmNum %d, w %d, h %d, y %p, u %p, v %p, get ipt time %d, %d, cur %d, cost %d \n",
                 pstPkgObjInfo->uiMatchId, uiPkgId, uiFrmNum, uiImgW, uiImgH, pstYData, pstUData, pstVData, uiInputTime, uiCpyStartTime, uiOutputTime, uiOutputTime - uiInputTime);

        PPM_LOGW("pack-match-obj: frmNum %d, w %d, h %d \n",
                 uiFrmNum, pstPkgMatchInfo->pack_bg_img.image_w, pstPkgMatchInfo->pack_bg_img.image_h);
#endif
    }

    return SAL_SOK;

exit:
    return DEMO_ERR_NOT_FIND_PACK_OUT;
}

/**
 * @function:   Ppm_DrvGetFaceResult
 * @brief:      获取匹配的人脸信息
 * @param[in]:  void *pstOutput
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_DrvGetFaceResult(void *pstOutput, PPM_FACE_RSLT_S *pstFaceOut)
{
    INT32 s32Ret = SAL_SOK;

    INT32 i = 0;     /* 结果计数 */
    INT32 uiImgW = 0;
    INT32 uiImgH = 0;
    INT32 uiFrmNum = 0;
    INT32 uiObjNum = 0;
    UINT32 uiInputTime = 0;
    UINT32 uiCpyStartTime = 0;
    UINT32 uiOutputTime = 0;
    INT64L llFrmNum = 0;

    float fFaceScore = 0.0f;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    XPACK_IMG stFrameDataXpackCopySrc = {0};
    XPACK_IMG stFrameDataXpackCopyDst = {0};

    VOID *pstYData = NULL;
    VOID *pstUData = NULL;
    VOID *pstVData = NULL;
    MTME_FACE_MATCH_OUT_T *pstFaceMatchOut = NULL;
    MTME_FACE_MATCH_INFO_T *pstFaceMatchInfo = NULL;
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_FACE_OBJ_INFO_S *pstFaceObjInfo = NULL;
    ICF_MEDIA_INFO *pstMediaInfo = NULL;
    PPM_INPUT_DATA_INFO *pstInputDataInfo = NULL;
	MTME_MATCH_INFO_OUT_T *pstMatchOutInfo = NULL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_RET(pstPpmGlobalPrm == NULL, exit, "Ppm_HalGetModPrm failed!");

#ifdef DEBUG_DUMP_DATA
    char path[64] = {0};
    char *pData = NULL;

    UINT32 uiSize = 0;
#endif

    /* 此处无需添加入参判空，有效性由调用接口保证 */
    uiCpyStartTime = SAL_getCurMs();


    pstMediaInfo = (ICF_MEDIA_INFO *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    PPM_DRV_CHECK_PTR(pstMediaInfo, exit, "pstMediaInfo == null!");

    llFrmNum = (INT64L)pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum;
    PPM_LOGD("get frmNum result %lld \n", llFrmNum);

    pstInputDataInfo = (PPM_INPUT_DATA_INFO *)pstMediaInfo->stInputInfo.pUserPtr;
    uiInputTime = pstInputDataInfo->stUsrPrvtInfo.uiIptTime;

	/* 获取人脸关联结果 */
	pstMatchOutInfo	= (MTME_MATCH_INFO_OUT_T *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_MATCH_FACE_DATA);
	PPM_DRV_CHECK_PTR(pstMatchOutInfo, exit, "get match face out failed!");

	pstFaceMatchOut = &pstMatchOutInfo->face_match_data;

    uiObjNum = pstFaceMatchOut->obj_num;
    if (uiObjNum > 0)
    {
        PPM_LOGI("face_match_result num is %d\n", uiObjNum);
    }

    /* debug: 告警当前buf cnt不够 */
    if (uiObjNum > PPM_MAX_DET_OBJ_NUM)
    {
        PPM_LOGE("obj num > max buf cnt 16!!! \n");
    }

    pstFaceOut->uiFaceCnt = uiObjNum;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    for (i = 0; i < uiObjNum; i++)
    {
        pstFaceMatchInfo = &pstFaceMatchOut->face_match_info[i];
        pstFaceObjInfo = &pstFaceOut->stFaceObjInfo[i];

        pstFaceObjInfo->uiMatchId = pstFaceMatchInfo->id;

        if (0x00 == pstFaceObjInfo->stFrameInfo.uiAppData)
        {
            s32Ret = Ppm_DrvGetFrameMem(PPM_FACE_WIDTH, PPM_FACE_HEIGHT, &pstFaceObjInfo->stFrameInfo);
            PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_DrvGetFrameMem failed!");
        }

#ifdef DEBUG_CPU_CPY  /* debug */
        UINT32 uiSize = 0;

        s32Ret = sys_hal_getVideoFrameInfo(&pstFaceObjInfo->stFrameInfo, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

        uiSize = pstFaceMatchInfo->face_img.image_w * pstFaceMatchInfo->face_img.image_h * 3 / 2;

        memcpy((VOID *)stVideoFrmBuf.virAddr[0], pstFaceMatchInfo->face_img.y, uiSize);

        pstFaceObjInfo->stFrameInfo.uiDataWidth = pstFaceMatchInfo->face_img.image_w;
        pstFaceObjInfo->stFrameInfo.uiDataHeight = pstFaceMatchInfo->face_img.image_h;

        PPM_LOGW("face info: w %d, h %d \n", pstFaceObjInfo->stFrameInfo.uiDataWidth, pstFaceObjInfo->stFrameInfo.uiDataHeight);
#else
        /* 清空旧数据 */
        //s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstDevPrm->stTmpFrame[0]);
        //PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_buildVideoFrame failed!");

        s32Ret = Ppm_DrvPackPlatVideoSt(&pstFaceMatchInfo->face_img, &pstDevPrm->stTmpFrame[0]);
        PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_DrvPackPlatVideoSt failed!");

        s32Ret = sys_hal_getVideoFrameInfo(&pstDevPrm->stTmpFrame[0], &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

        PPM_LOGW("face video st: w %d, h %d, s0 %d, s1 %d, vir %p, %p, %p, phy %p, %p, %p \n",
                 stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, stVideoFrmBuf.stride[0], stVideoFrmBuf.stride[1],
                 (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1], (VOID *)stVideoFrmBuf.virAddr[2],
                 (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1], (VOID *)stVideoFrmBuf.phyAddr[2]);

#ifdef DEBUG_DUMP_DATA
        memset(path, 0x00, 64 * sizeof(char));

        sprintf(path, "/mnt/nfs/ppm/%d_face_img_%dx%d.nv21", pstFaceMatchInfo->id, pstFaceMatchInfo->face_img.image_w, pstFaceMatchInfo->face_img.image_h);

        pData = (VOID *)stVideoFrmBuf.virAddr[0];
        uiSize = pstFaceMatchInfo->face_img.image_w * pstFaceMatchInfo->face_img.image_h * 3 / 2;

        (VOID)Ppm_HalDebugDumpData((VOID *)pData, path, uiSize, 0);
        PPM_LOGW("fwrite end!111 \n");

        memset(path, 0x00, 64 * sizeof(char));

        sprintf(path, "/mnt/nfs/ppm/%d_face_bg_%dx%d.nv21", pstFaceMatchInfo->id, pstFaceMatchInfo->face_bg_img.image_w, pstFaceMatchInfo->face_bg_img.image_h);

        pData = (VOID *)pstFaceMatchInfo->face_bg_img.y;
        uiSize = pstFaceMatchInfo->face_bg_img.image_w * pstFaceMatchInfo->face_bg_img.image_h * 3 / 2;

        (VOID)Ppm_HalDebugDumpData((VOID *)pData, path, uiSize, 0);
        PPM_LOGW("fwrite end!222 \n");
#endif


		/*拷贝帧数据*/
        s32Ret = sys_hal_getVideoFrameInfo(&pstDevPrm->stTmpFrame[0], &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");
        
        stFrameDataXpackCopySrc.picPrm.u32Width = stVideoFrmBuf.frameParam.width;
        stFrameDataXpackCopySrc.picPrm.u32Height = stVideoFrmBuf.frameParam.height;
        stFrameDataXpackCopySrc.picPrm.u32Stride = stVideoFrmBuf.stride[0];
        stFrameDataXpackCopySrc.picPrm.phyAddr[0] = stVideoFrmBuf.phyAddr[0];
        stFrameDataXpackCopySrc.picPrm.VirAddr[0] = (VOID *)stVideoFrmBuf.virAddr[0];
        stFrameDataXpackCopySrc.picPrm.VirAddr[1] = (VOID *)stVideoFrmBuf.virAddr[1];
        stFrameDataXpackCopySrc.picPrm.enPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
        stFrameDataXpackCopySrc.picPrm.vbBlk = stVideoFrmBuf.vbBlk;

        s32Ret = sys_hal_getVideoFrameInfo(&pstFaceObjInfo->stFrameInfo, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");
        stFrameDataXpackCopyDst.picPrm.u32Width = pstFaceMatchInfo->face_img.image_w;
        stFrameDataXpackCopyDst.picPrm.u32Stride = stVideoFrmBuf.frameParam.width;
        stFrameDataXpackCopyDst.picPrm.u32Height = pstFaceMatchInfo->face_img.image_h;
        stFrameDataXpackCopyDst.picPrm.phyAddr[0] = stVideoFrmBuf.phyAddr[0];
        stFrameDataXpackCopyDst.picPrm.VirAddr[0] = (VOID *)stVideoFrmBuf.virAddr[0];
        stFrameDataXpackCopyDst.picPrm.VirAddr[1] = (VOID *)stVideoFrmBuf.virAddr[1];
        stFrameDataXpackCopyDst.picPrm.enPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
        stFrameDataXpackCopyDst.picPrm.vbBlk = stVideoFrmBuf.vbBlk;

        s32Ret = SAL_halDataCopy(&stFrameDataXpackCopySrc, &stFrameDataXpackCopyDst);
        PPM_DRV_CHECK_RET(s32Ret, exit, "HAL_bufQuickCopyVideo failed!");

        pstFaceObjInfo->stFrameInfo.uiDataWidth  = pstFaceMatchInfo->face_img.image_w;
        pstFaceObjInfo->stFrameInfo.uiDataHeight = pstFaceMatchInfo->face_img.image_h;

#if 0
        s32Ret = Ia_TdeQuickCopy(&pstDevPrm->stTmpFrame[0],
                                 &pstFaceObjInfo->stFrameInfo,
                                 0, 0,
                                 stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.height, SAL_TRUE);
        PPM_DRV_CHECK_RET(s32Ret, exit, "HAL_bufQuickCopyVideo failed!");
#endif

#endif

    s32Ret = sys_hal_getVideoFrameInfo(&pstFaceObjInfo->stFrameInfo, &stVideoFrmBuf);
    PPM_DRV_CHECK_RET(s32Ret, exit, "sys_hal_getVideoFrameInfo failed!");

    PPM_LOGW("face obj info: w %d, h %d, s0 %d, s1 %d, vir %p, %p, %p, phy %p, %p, %p \n",
             stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, stVideoFrmBuf.stride[0], stVideoFrmBuf.stride[1],
             (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1], (VOID *)stVideoFrmBuf.virAddr[2],
             (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1], (VOID *)stVideoFrmBuf.phyAddr[2]);

#if 1  /* debug: 调试打印 */
        uiFrmNum = pstFaceMatchInfo->frame_num;
        fFaceScore = pstFaceMatchInfo->face_score;

        uiImgH = pstFaceMatchInfo->face_img.image_h;
        uiImgW = pstFaceMatchInfo->face_img.image_w;
        pstYData = pstFaceMatchInfo->face_img.y;
        pstUData = pstFaceMatchInfo->face_img.u;
        pstVData = pstFaceMatchInfo->face_img.v;

        PPM_DRV_CHECK_PTR(pstYData, exit, "pstYData == null!");

        /* sprintf(face_path,"%s/face-match-obj%d-pack%d-f%d-w%d_h%d.nv21",DEMO_SAVE_DATA_PATH, */
        /*       obj_id, pack_id, frame_get, uiImgW, img_h); */

        uiOutputTime = SAL_getCurMs();

        PPM_LOGW("face-match-obj: id %d, score %f, frmNum %d, w %d, h %d, y %p, u %p, v %p, get ipt time %d, %d, cur %d, cost %d, [%d]\n",
                 pstFaceObjInfo->uiMatchId, fFaceScore, uiFrmNum, uiImgW, uiImgH, pstYData,
                 pstUData, pstVData, uiInputTime, uiCpyStartTime, uiOutputTime, uiOutputTime - uiInputTime,
                 uiOutputTime - uiCpyStartTime);

        PPM_LOGW("face-match-obj: frmNum %d, w %d, h %d \n",
                 uiFrmNum, pstFaceMatchInfo->face_bg_img.image_w, pstFaceMatchInfo->face_bg_img.image_h);
#endif
    }

    return SAL_SOK;

exit:
    return DEMO_ERR_NOT_FIND_FACE_OUT;
}

#define PPM_LOG_POS_RECORD_MASK (0)
#define PPM_LOG_POS_TRK_MASK	(1)
#define PPM_LOG_DUMP_JPG_MASK	(2)
#define PPM_LOG_DUMP_SYNC_DATA	(3)

static UINT32 u32PosInfoFlag = SAL_FALSE;

/**
 * @function    Ppm_DrvSetPosInfoFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Ppm_DrvSetPosInfoFlag(UINT32 flag)
{
    u32PosInfoFlag = flag;
    return;
}

/**
 * @function    Ppm_DrvJudgePrintPosInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
BOOL Ppm_DrvJudgePrintPosInfo(VOID)
{
    return u32PosInfoFlag;
}

/**
 * @function:   Ppm_DrvFillPosInfo
 * @brief:      填充pos信息
 * @param[in]:  VOID *pstOutput
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvFillPosInfo(VOID *pstOutput)
{
#if 1
    UINT32 i = 0;
    UINT32 k = 0;
    UINT32 cnt = 0;
    float fPointRectHalf = 3.0 / 1920.0;
    float fPointRectW = 6.0 / 1920.0;
    UINT32 uiHeadFlag = 0;
    UINT32 uiTimeStart = 0;
    UINT32 uiTime1 = 0;
    UINT32 uiTimeEnd = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_POS_INFO_S *pstPosInfo = NULL;
    MTME_RECT_T *pstObjRect = NULL;    /* 目标头肩框 */
    MTME_HTR_JOINTS_T *pstHikJoint_T = NULL;
    MTME_POINT_T *pstPoint2d = NULL;
    MTME_RELATION_INFO_CPY_T *pstRelInfo = NULL;
    ICF_MEDIA_INFO *pstMediaInfo = NULL;
	MTME_MATCH_INFO_OUT_T *pstFaceOutInfo = NULL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_RET(pstPpmGlobalPrm == NULL, err, "Ppm_HalGetModPrm failed!");

    MTME_POINT_T stCenterPoint = {0};                             /* 中心点信息 */

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstPosInfo = &pstDevPrm->stPosInfo;

    uiTimeStart  = SAL_getCurMs();

    if (!pstDevPrm->stPosInfo.bEnable)
    {
        return SAL_SOK;
    }

    pstMediaInfo = (ICF_MEDIA_INFO *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    PPM_DRV_CHECK_PTR(pstMediaInfo, err, "pstMediaInfo == null!");

    pstPosInfo->uiReserved[0] = (UINT32)(pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum);

    if (0 == pstPosInfo->u64PTS)
    {
        /* SAL_WARN("warn pts == 0! \n"); */
    }

    /* 三目相机: 获取关节点信息 */
    pstFaceOutInfo = (MTME_MATCH_INFO_OUT_T *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_MATCH_FACE_DATA);
    pstRelInfo = &pstFaceOutInfo->pos_data;

    for (i = 0; i < MTME_MAX_RECORED_OBJ_NUM; i++)
    {
        if (SAL_TRUE != pstRelInfo->target_list.target_info[i].target_valid)
        {
            continue;
        }

        if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_POS_RECORD_MASK) & 0x1)
        {
            SAL_ERROR("id %d, cross_status %d, capture_status %d, cross_result %d, rect_iou %f\n",
                      pstRelInfo->target_list.target_info[i].target_id, 
                      pstRelInfo->target_list.capture_info[i].cross_status, 
                      pstRelInfo->target_list.capture_info[i].capture_status,
                      pstRelInfo->target_list.capture_info[i].cross_result,
                      pstRelInfo->target_list.match_face_info[i].rect_iou);
            printf("get map point: ");
            for (k = 0; k < pstRelInfo->target_list.match_face_info[i].head_rect_map.vertex_num; k++)
            {
                printf("[%f, %f] ", pstRelInfo->target_list.match_face_info[i].head_rect_map.point[k].x, 
                                    pstRelInfo->target_list.match_face_info[i].head_rect_map.point[k].y);
            }
            printf("\n");
        }
        pstObjRect = &(pstRelInfo->target_list.capture_info[i].obj_ret);

        sprintf(pstPosInfo->stJointsRectInfo.str[cnt], "id: %d,%d,%d,%d,%d,%d", 
                pstRelInfo->target_list.target_info[i].target_id,
                pstRelInfo->target_list.match_pack_info[i].trigger_cnt,
                pstRelInfo->target_list.match_pack_info[i].remove_cnt,
                pstRelInfo->target_list.match_pack_info[i].flag_package_num,
                pstRelInfo->target_list.capture_info[i].cross_status,
                pstRelInfo->target_list.capture_info[i].capture_status);

        /* SAL_ERROR("get tri string %s \n", pstPosInfo->stJointsRectInfo.str[cnt]); */

        pstPosInfo->stJointsRectInfo.stJointsRect[cnt].x = pstObjRect->x;
        pstPosInfo->stJointsRectInfo.stJointsRect[cnt].y = pstObjRect->y;
        pstPosInfo->stJointsRectInfo.stJointsRect[cnt].width = pstObjRect->width;
        pstPosInfo->stJointsRectInfo.stJointsRect[cnt++].height = pstObjRect->height;

        uiHeadFlag = 0;   /* 每个目标头flag */
        for (k = 0; k < pstRelInfo->target_list.capture_info[i].joint_num; k++)
        {
            pstHikJoint_T = &pstRelInfo->target_list.capture_info[i].joint_point.joints[k];
            if (MTME_HTR_JOINTS_VISIBLE == pstHikJoint_T->state)
            {
                pstPoint2d = &pstHikJoint_T->point_2d;

                pstPosInfo->stJointsRectInfo.stJointsRect[cnt].x = pstPoint2d->x - fPointRectHalf;
                pstPosInfo->stJointsRectInfo.stJointsRect[cnt].y = pstPoint2d->y - fPointRectHalf;
                pstPosInfo->stJointsRectInfo.stJointsRect[cnt].width = fPointRectW;
                pstPosInfo->stJointsRectInfo.stJointsRect[cnt++].height = fPointRectW;

                /* 获取头肩中心点 */
                if (pstHikJoint_T->type == MTME_HTR_JOINTS_HEAD)
                {
                    stCenterPoint.x = pstPoint2d->x;
                    stCenterPoint.y = pstPoint2d->y;
                    uiHeadFlag = 1;
                }
            }

            if (0 == uiHeadFlag)
            {
                stCenterPoint.x = pstObjRect->x + pstObjRect->width / 2;
                stCenterPoint.y = pstObjRect->y + pstObjRect->height / 2;
            }

            pstPosInfo->stJointsRectInfo.stJointsRect[cnt].x = stCenterPoint.x - fPointRectHalf;
            pstPosInfo->stJointsRectInfo.stJointsRect[cnt].y = stCenterPoint.y - fPointRectHalf;
            pstPosInfo->stJointsRectInfo.stJointsRect[cnt].width = fPointRectW;
            pstPosInfo->stJointsRectInfo.stJointsRect[cnt++].height = fPointRectW;
        }
    }

    /* 记录三目相机需要叠加的目标框个数 */
    pstPosInfo->stJointsRectInfo.uiJointsCnt = cnt;
    uiTime1  = SAL_getCurMs();

    /* 人脸相机: 获取人脸和关键点信息 */
    cnt = 0;
    if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_POS_TRK_MASK) & 0x1)
    {
        if (pstRelInfo->face_trk.face_num > 0)
        {
            PPM_LOGD("face_pos: face_num %d \n", pstRelInfo->face_trk.face_num);
        }
    }

    for (i = 0; i < pstRelInfo->face_trk.face_num; i++)
    {
        pstObjRect = &(pstRelInfo->face_trk.det_out[i]);

        /* 人脸矩形框 */
        sprintf(pstPosInfo->stFaceRectInfo.str[cnt], "id: %d", pstRelInfo->face_trk.face_id[i]);

        sprintf(pstPosInfo->stJointsRectInfo.str[cnt], "id: %d,%d,%d,%d,%d,%d", 
                        pstRelInfo->target_list.target_info[i].target_id,
                        pstRelInfo->target_list.match_pack_info[i].trigger_cnt,
                        pstRelInfo->target_list.match_pack_info[i].remove_cnt,
                        pstRelInfo->target_list.match_pack_info[i].flag_package_num,
                        pstRelInfo->target_list.capture_info[i].cross_status,
                        pstRelInfo->target_list.capture_info[i].capture_status);

        /* SAL_ERROR("get face string %s \n", pstPosInfo->stFaceRectInfo.str[cnt]); */

        pstPosInfo->stFaceRectInfo.stFaceRect[cnt].x = pstObjRect->x;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt].y = pstObjRect->y;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt].width = pstObjRect->width;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt++].height = pstObjRect->height;

        if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_POS_TRK_MASK) & 0x1)
        {
            PPM_LOGD("face_pos: trk111---i %d, face_id %d, cnt %d, score %f, [%f, %f] [%f, %f] \n",
                     i, pstRelInfo->face_trk.face_id[i], cnt, pstRelInfo->face_trk.face_score[i],
                     pstObjRect->x, pstObjRect->y, pstObjRect->width, pstObjRect->height);
        }

        /* 人脸几何中心 */
        stCenterPoint.x = pstObjRect->x + pstObjRect->width / 2;
        stCenterPoint.y = pstObjRect->y + pstObjRect->height / 2;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt].x = stCenterPoint.x - fPointRectHalf;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt].y = stCenterPoint.y - fPointRectHalf;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt].width = fPointRectW;
        pstPosInfo->stFaceRectInfo.stFaceRect[cnt++].height = fPointRectW;

        PPM_LOGD("face_pos: trk222---i %d, cnt %d, [%f, %f] [%f, %f] \n", i, cnt,
                 stCenterPoint.x - fPointRectHalf, stCenterPoint.y - fPointRectHalf,
                 fPointRectW, fPointRectW);
    }

    PPM_LOGD("face_pos: face_num %d \n", pstRelInfo->face_trk.face_num);

    /* 绘制人脸预测框 */
    for (i = 0; i < MTME_MAX_RECORED_OBJ_NUM; i++)
    {
        if (pstRelInfo->target_list.match_face_info[i].face_rect_fore.height > 0.0f)
        {
            /* 预测的人脸框 */
            pstObjRect = &(pstRelInfo->target_list.match_face_info[i].face_rect_fore);

            pstPosInfo->stFaceRectInfo.stFaceRect[cnt].x = pstObjRect->x;
            pstPosInfo->stFaceRectInfo.stFaceRect[cnt].y = pstObjRect->y;
            pstPosInfo->stFaceRectInfo.stFaceRect[cnt].width = pstObjRect->width;
            pstPosInfo->stFaceRectInfo.stFaceRect[cnt++].height = pstObjRect->height;

            /* 映射头顶点 */
            for (k = 0; k < pstRelInfo->target_list.match_face_info[i].head_rect_map.vertex_num; k++)
            {
                pstPoint2d = &(pstRelInfo->target_list.match_face_info[i].head_rect_map.point[k]);

                pstPosInfo->stFaceRectInfo.stFaceRect[cnt].x = pstPoint2d->x / PPM_FACE_WIDTH - fPointRectHalf;
                pstPosInfo->stFaceRectInfo.stFaceRect[cnt].y = pstPoint2d->y / PPM_FACE_WIDTH - fPointRectHalf;
                pstPosInfo->stFaceRectInfo.stFaceRect[cnt].width = fPointRectW;
                pstPosInfo->stFaceRectInfo.stFaceRect[cnt++].height = fPointRectW;
            }
        }
    }

    /* 记录人脸相机需要叠加的目标框个数 */
    pstPosInfo->stFaceRectInfo.uiFaceCnt = cnt;
    uiTimeEnd  = SAL_getCurMs();

    PPM_LOGD("save pos info: joints[%d], face[%d] \n",
             pstPosInfo->stJointsRectInfo.uiJointsCnt, pstPosInfo->stFaceRectInfo.uiFaceCnt);
    PPM_LOGD("pos:joints %d, face %d, get ipt time %d, %d, %d, pkg %d, cost %d\n",
                 pstPosInfo->stJointsRectInfo.uiJointsCnt, pstPosInfo->stFaceRectInfo.uiFaceCnt,
                 uiTimeStart, uiTime1, uiTimeEnd, uiTime1 - uiTimeStart,
                 uiTimeEnd - uiTimeStart);
    return SAL_SOK;
err:
    return DEMO_ERR_NOT_FIND_PACK_OUT;
#else
	PPM_LOGD("pos not support! return success! \n");
	return SAL_SOK;
#endif
}

/**
 * @function:   Ppm_DrvMccCalibCbFunc
 * @brief:      跨相机mcc标定结果回调函数
 * @param[in]:  INT32 nNodeID
 * @param[in]:  INT32 nCallBackType
 * @param[in]:  VOID *pstOutput
 * @param[in]:  UINT32 nSize
 * @param[in]:  VOID *pUsr
 * @param[in]:  INT32 nUserSize
 * @param[out]: None
 * @return:     VOID
 */
VOID Ppm_DrvMccCalibCbFunc(INT32 nNodeID, 
                           INT32 nCallBackType, 
                           VOID *pstOutput, 
                           UINT32 nSize, 
                           VOID *pUsr, 
                           INT32 nUserSize)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 chan = 0;
    UINT64 uiFrmNum = 0;

    PPM_MOD_S *pstModPrm = NULL;
    PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;
    ICF_MEDIA_INFO *pstMediaInfo = NULL;
    MTME_MCC_CABLI_OUT_T *pstCalibOut = NULL;
    PPM_OUTPUT_CALIB_PRM_S *pstIcfCalibOutput = NULL;

    pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_DRV_CHECK_PTR(pstMccCalibChnPrm, calib_end, "pstMccCalibChnPrm == null!");

    pstIcfCalibOutput = &pstMccCalibChnPrm->stIcfCalibPrm.stCalibOptPrm;

    /* 默认本次标定结果为失败 */
    pstIcfCalibOutput->s32CalibSts = SAL_FAIL;

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, calib_end, "pstModPrm == null!");

    pstMediaInfo = (ICF_MEDIA_INFO *)pstModPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    PPM_DRV_CHECK_PTR(pstMediaInfo, calib_end, "pstMediaInfo == null!");

    uiFrmNum = (UINT64)pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum;
    chan = *(UINT32 *)pstMediaInfo->stInputInfo.pUserPtr;

	/* checker */
	if (chan != PPM_SUB_MOD_MCC_CALIB)
	{
		PPM_LOGE("invalid user chan %d! \n", chan);
		goto calib_end;
	}

    PPM_LOGW("get frmNum %llu, chan %d \n", uiFrmNum, chan);

    s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfGetPackageState(pstOutput);
    PPM_DRV_CHECK_RET(s32Ret, calib_end, "ICF_Package_GetState failed!");

    pstCalibOut = (MTME_MCC_CABLI_OUT_T *)pstModPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_CABLIC_DATA);
    PPM_DRV_CHECK_RET(NULL == pstCalibOut, calib_end, "calib ICF_GetDataPtrFromPkg failed!");

    /* 拷贝本次标定结果状态 */
    pstIcfCalibOutput->s32CalibSts = pstCalibOut->cablic_status;
    if (SAL_SOK != pstIcfCalibOutput->s32CalibSts)
    {
        PPM_LOGE("cablic_err, 0x%x \n", pstIcfCalibOutput->s32CalibSts);
        goto calib_end;
    }

    PPM_LOGI("ywn beforetrans: R = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
           pstCalibOut->R_matrix[0][0],
           pstCalibOut->R_matrix[0][1],
           pstCalibOut->R_matrix[0][2],
           pstCalibOut->R_matrix[1][0],
           pstCalibOut->R_matrix[1][1],
           pstCalibOut->R_matrix[1][2],
           pstCalibOut->R_matrix[2][0],
           pstCalibOut->R_matrix[2][1],
           pstCalibOut->R_matrix[2][2]);
	
    /* 拷贝标定结果-R矩阵 */
    pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiRowCnt = 3;
    pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiColCnt = 3;
    for (i = 0; i < pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiRowCnt; i++)
    {
        for (j = 0; j < pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiColCnt; j++)
        {
            pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[j][i] = pstCalibOut->R_matrix[i][j];
        }
    }

    PPM_LOGI("ywn aftertrans: R = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[0][0],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[0][1],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[0][2],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[1][0],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[1][1],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[1][2],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[2][0],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[2][1],
           pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[2][2]);

    /*		memcpy(pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[i],
               cablic_out->R_matrix[i],
               sizeof(double) * 3);*/

    /* 拷贝标定结果-T矩阵 */
    pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.uiRowCnt = 1;
    pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.uiColCnt = 3;
    memcpy(pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.fMatrix[0],
           pstCalibOut->T_matrix,
           sizeof(double) * 3);

    /* 拷贝标定结果-RMS */
    pstIcfCalibOutput->fRmsVal = pstCalibOut->rms;

    /* 拷贝标定结果-人脸相机内参逆矩阵 */
    pstIcfCalibOutput->stMatrixInfo.stFaceCamInvMatrixInfo.uiRowCnt = 3;
    pstIcfCalibOutput->stMatrixInfo.stFaceCamInvMatrixInfo.uiColCnt = 3;
    for (i = 0; i < pstIcfCalibOutput->stMatrixInfo.stFaceCamInvMatrixInfo.uiRowCnt; i++)
    {
        memcpy(pstIcfCalibOutput->stMatrixInfo.stFaceCamInvMatrixInfo.fMatrix[i],
               pstCalibOut->face_inv_matrix[i],
               sizeof(double) * 3);
    }

    /* 拷贝标定结果-三目相机内参逆矩阵 */
    pstIcfCalibOutput->stMatrixInfo.stTriCamInvMatrixInfo.uiRowCnt = 3;
    pstIcfCalibOutput->stMatrixInfo.stTriCamInvMatrixInfo.uiColCnt = 3;
    for (i = 0; i < pstIcfCalibOutput->stMatrixInfo.stTriCamInvMatrixInfo.uiRowCnt; i++)
    {
        memcpy(pstIcfCalibOutput->stMatrixInfo.stTriCamInvMatrixInfo.fMatrix[i],
               pstCalibOut->thr_inv_matrix[i],
               sizeof(double) * 3);
    }

    PPM_LOGI("get calib result, rms %f \n", pstIcfCalibOutput->fRmsVal);

    /* 打印标定结果 */
    (VOID)Ppm_DrvPrintCalibRslt(&pstIcfCalibOutput->stMatrixInfo);

calib_end:
    sem_post(&pstMccCalibChnPrm->stIcfCalibPrm.sem);
    return;
}

/**
 * @function:   Ppm_DrvFaceCalibCbFunc
 * @brief:      标定结果回调函数
 * @param[in]:  INT32 nNodeID
 * @param[in]:  INT32 nCallBackType
 * @param[in]:  VOID *pstOutput
 * @param[in]:  UINT32 nSize
 * @param[in]:  VOID *pUsr
 * @param[in]:  INT32 nUserSize
 * @param[out]: None
 * @return:     VOID
 */
VOID Ppm_DrvFaceCalibCbFunc(INT32 nNodeID, 
                            INT32 nCallBackType, 
                            VOID *pstOutput, 
                            UINT32 nSize,
                            VOID *pUsr, 
                            INT32 nUserSize)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 chan = 0;
    UINT64 uiFrmNum = 0;

    PPM_MOD_S *pstModPrm = NULL;
    ICF_MEDIA_INFO *pstMediaInfo = NULL;
    MTME_MCC_SV_CABLI_OUT_T *pstCalibOut = NULL;
    PPM_OUTPUT_CALIB_PRM_S *pstIcfCalibOutput = NULL;
	PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;

    pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_DRV_CHECK_PTR(pstFaceCalibChnPrm, calib_end, "pstFaceCalibChnPrm == null!");

    pstIcfCalibOutput = &pstFaceCalibChnPrm->stIcfFaceCalibPrm.stCalibOptPrm;

    /* 默认本次标定结果为失败 */
    pstIcfCalibOutput->s32CalibSts = SAL_FAIL;

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, calib_end, "pstModPrm == null!");

    pstMediaInfo = (ICF_MEDIA_INFO *)pstModPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    PPM_DRV_CHECK_PTR(pstMediaInfo, calib_end, "pstMediaInfo == null!");

    uiFrmNum = (UINT64)pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum;
    chan = *(UINT32 *)pstMediaInfo->stInputInfo.pUserPtr;

	/* checker */
	if (chan != PPM_SUB_MOD_FACE_CALIB)
	{
		PPM_LOGE("invalid user chan %d! \n", chan);
		goto calib_end;
	}

    PPM_LOGW("get frmNum %llu, chan %d \n", uiFrmNum, chan);

    s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfGetPackageState(pstOutput);
    PPM_DRV_CHECK_RET(s32Ret, calib_end, "ICF_Package_GetState failed!");

    pstCalibOut = (MTME_MCC_SV_CABLI_OUT_T *)pstModPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_SVCABLIC_DATA);
    PPM_DRV_CHECK_RET(NULL == pstCalibOut, calib_end, "calib ICF_GetDataPtrFromPkg failed!");

    /* 拷贝本次标定结果状态 */
    pstIcfCalibOutput->s32CalibSts = pstCalibOut->cablic_status;
    if (SAL_SOK != pstIcfCalibOutput->s32CalibSts)
    {
        PPM_LOGE("cablic_err, 0x%x \n", pstIcfCalibOutput->s32CalibSts);
        goto calib_end;
    }

    PPM_LOGI("face inter matrix = [[%f, %f, %f] [%f, %f, %f] [%f, %f, %f]] \n",
             pstCalibOut->cam_internal_mat[0][0],
             pstCalibOut->cam_internal_mat[0][1],
             pstCalibOut->cam_internal_mat[0][2],
             pstCalibOut->cam_internal_mat[1][0],
             pstCalibOut->cam_internal_mat[1][1],
             pstCalibOut->cam_internal_mat[1][2],
             pstCalibOut->cam_internal_mat[2][0],
             pstCalibOut->cam_internal_mat[2][1],
             pstCalibOut->cam_internal_mat[2][2]);

    PPM_LOGI("face dist matrix = [%f, %f, %f, %f, %f, %f, %f, %f] \n",
             pstCalibOut->dist_coeffs[0],
             pstCalibOut->dist_coeffs[1],
             pstCalibOut->dist_coeffs[2],
             pstCalibOut->dist_coeffs[3],
             pstCalibOut->dist_coeffs[4],
             pstCalibOut->dist_coeffs[5],
             pstCalibOut->dist_coeffs[6],
             pstCalibOut->dist_coeffs[7]);

    /* 拷贝标定结果-RMS */
    pstIcfCalibOutput->fRmsVal = pstCalibOut->rms;

    /* 拷贝标定结果-人脸相机内参逆矩阵 */
    pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiRowCnt = 3;
    pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiColCnt = 3;
    for (i = 0; i < pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiRowCnt; i++)
    {
        for (j = 0; j < pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.uiColCnt; j++)
        {
            pstIcfCalibOutput->stMatrixInfo.stR_MatrixInfo.fMatrix[i][j] = pstCalibOut->cam_internal_mat[i][j];
        }
    }

    /* 拷贝标定结果-人脸相机内参畸变系数 */
    pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.uiRowCnt = 1;
    pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.uiColCnt = 8;
    for (i = 0; i < pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.uiColCnt; i++)
    {
        pstIcfCalibOutput->stMatrixInfo.stT_MatrixInfo.fMatrix[0][i] = pstCalibOut->dist_coeffs[i];
    }

    PPM_LOGI("get calib result, rms %f, chan %d \n", pstIcfCalibOutput->fRmsVal, chan);

calib_end:
    sem_post(&pstFaceCalibChnPrm->stIcfFaceCalibPrm.sem);
    return;
}

/**
 * @function   Ppm_DrvGetRsltEmptQueBuf
 * @brief      获取结果空队列空闲缓存
 * @param[in]  UINT32 chan                    
 * @param[in]  PPM_RSLT_INFO_S **ppstRsltBuf  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_DrvGetRsltEmptQueBuf(UINT32 chan, PPM_RSLT_INFO_S **ppstRsltBuf)
{
	INT32 s32Ret = SAL_FAIL;
	
    PPM_DEV_PRM *pstDevPrm = NULL;
    DSA_QueHndl *pstEmptQue = NULL;       /* 数据空队列 */
	PPM_RSLT_INFO_S *pstTmpRsltBuf = NULL;

    PPM_DRV_CHECK_PTR(ppstRsltBuf, exit, "ppstRsltBuf == null!");

	pstDevPrm = Ppm_DrvGetDevPrm(chan);
	PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    pstEmptQue = pstDevPrm->stRsltQueInfo.stRsltQuePrm.pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, exit, "pstEmptQue == null!");

    (VOID)DSA_QueGet(pstEmptQue, (VOID **)&pstTmpRsltBuf, SAL_TIMEOUT_FOREVER);
    PPM_DRV_CHECK_PTR(pstTmpRsltBuf, exit, "pstTmpRsltBuf == null!");

	/* 清空缓存数据 */
	memset(pstTmpRsltBuf, 0x00, sizeof(PPM_RSLT_INFO_S));

	*ppstRsltBuf = pstTmpRsltBuf;
	s32Ret = SAL_SOK;
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_DrvPutRsltEmptQueBuf
 * @brief      释放空队列结果缓存
 * @param[in]  UINT32 chan                  
 * @param[in]  PPM_RSLT_INFO_S *pstRsltBuf  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_DrvPutRsltEmptQueBuf(UINT32 chan, PPM_RSLT_INFO_S *pstRsltBuf)
{
	INT32 s32Ret = SAL_FAIL;
	
    PPM_DEV_PRM *pstDevPrm = NULL;
    DSA_QueHndl *pstEmptQue = NULL;       /* 数据空队列 */

    PPM_DRV_CHECK_PTR(pstRsltBuf, exit, "pstRsltBuf == null!");

	pstDevPrm = Ppm_DrvGetDevPrm(chan);
	PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    pstEmptQue = pstDevPrm->stRsltQueInfo.stRsltQuePrm.pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, exit, "pstEmptQue == null!");

    (VOID)DSA_QuePut(pstEmptQue, (VOID *)pstRsltBuf, SAL_TIMEOUT_NONE);

exit:
	return s32Ret;
}

/**
 * @function   Ppm_DrvGetRsltFullQueBuf
 * @brief      获取结果满队列空闲缓存
 * @param[in]  UINT32 chan                    
 * @param[in]  PPM_RSLT_INFO_S **ppstRsltBuf  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_DrvGetRsltFullQueBuf(UINT32 chan, PPM_RSLT_INFO_S **ppstRsltBuf)
{
	INT32 s32Ret = SAL_FAIL;
	
    PPM_DEV_PRM *pstDevPrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;       /* 数据满队列 */
	PPM_RSLT_INFO_S *pstTmpRsltBuf = NULL;

    PPM_DRV_CHECK_PTR(ppstRsltBuf, exit, "ppstRsltBuf == null!");

	pstDevPrm = Ppm_DrvGetDevPrm(chan);
	PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    pstFullQue = pstDevPrm->stRsltQueInfo.stRsltQuePrm.pstFullQue;
    PPM_DRV_CHECK_PTR(pstFullQue, exit, "pstFullQue == null!");

    (VOID)DSA_QueGet(pstFullQue, (VOID **)&pstTmpRsltBuf, SAL_TIMEOUT_FOREVER);
    PPM_DRV_CHECK_PTR(pstTmpRsltBuf, exit, "pstTmpRsltBuf == null!");

	*ppstRsltBuf = pstTmpRsltBuf;
	s32Ret = SAL_SOK;
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_DrvPutRsltFullQueBuf
 * @brief      释放满队列结果缓存
 * @param[in]  UINT32 chan                  
 * @param[in]  PPM_RSLT_INFO_S *pstRsltBuf  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_DrvPutRsltFullQueBuf(UINT32 chan, PPM_RSLT_INFO_S *pstRsltBuf)
{
	INT32 s32Ret = SAL_FAIL;
	
    PPM_DEV_PRM *pstDevPrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;       /* 数据满队列 */

    PPM_DRV_CHECK_PTR(pstRsltBuf, exit, "pstRsltBuf == null!");

    pstRsltBuf->uiReverse[0] = SAL_getCurMs();   /* 记录耗时 */

	pstDevPrm = Ppm_DrvGetDevPrm(chan);
	PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    pstFullQue = pstDevPrm->stRsltQueInfo.stRsltQuePrm.pstFullQue;
    PPM_DRV_CHECK_PTR(pstFullQue, exit, "pstFullQue == null!");

    (VOID)DSA_QuePut(pstFullQue, (VOID *)pstRsltBuf, SAL_TIMEOUT_NONE);
	s32Ret = SAL_SOK;

exit:
	return s32Ret;
}

/**
 * @function   Ppm_DrvMatchPosRsltCbFunc
 * @brief      关联匹配通道-POS回调处理
 * @param[in]  INT32 nNodeID        
 * @param[in]  INT32 nCallBackType  
 * @param[in]  VOID *pstOutput      
 * @param[in]  UINT32 nSize         
 * @param[in]  VOID *pUsr           
 * @param[in]  INT32 nUserSize      
 * @param[out] None
 * @return     VOID
 */
VOID Ppm_DrvMatchPosRsltCbFunc(INT32 nNodeID, 
	                           INT32 nCallBackType, 
	                           VOID *pstOutput, 
	                           UINT32 nSize, 
	                           VOID *pUsr, 
	                           INT32 nUserSize)
{
	INT32 s32Ret = SAL_FAIL;

	/* szl_ppm_todo: 此处仅处理人脸POS，后续需要考虑metadata中解析的深度信息(关节点等) */



    s32Ret = Ppm_DrvFillPosInfo(pstOutput);
    PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_DrvFillPosInfo failed!");
	
exit:
	return;
}

/**
 * @function   Ppm_DrvPrMatchFaceOut
 * @brief      调试接口：打印关联匹配人脸抓拍结果
 * @param[in]  VOID *pstOutput  
 * @param[out] None
 * @return     static VOID
 */
static INT32 Ppm_DrvPrMatchFaceOut(VOID *pstOutput)
{
	INT32 s32Ret = SAL_FAIL;

	UINT32 i = 0;
	
	PPM_MOD_S *pstPpmGlobalPrm = NULL;
	MTME_MATCH_INFO_OUT_T *pstMatchOutInfo = NULL;
	MTME_FACE_MATCH_OUT_T *pstMatchFaceOut = NULL;

	//return;

	/* 获取hal层全局参数 */
	pstPpmGlobalPrm = Ppm_HalGetModPrm();
	PPM_DRV_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

	/* 获取当前帧状态，如果出错则直接丢弃 */
	s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageState(pstOutput);
	PPM_DRV_CHECK_RET(s32Ret, exit, "get package state failed!");

	/* 获取人脸关联结果 */
	pstMatchOutInfo	= (MTME_MATCH_INFO_OUT_T *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_MATCH_FACE_DATA);
	PPM_DRV_CHECK_PTR(pstMatchOutInfo, exit, "get match face out failed!");

	pstMatchFaceOut = &pstMatchOutInfo->face_match_data;
	
    if (pstMatchFaceOut->obj_num != 0)
    {
        PPM_LOGI("get match face obj_num: %d \n", pstMatchFaceOut->obj_num);
    }

	for (i = 0; i < pstMatchFaceOut->obj_num; i++)
	{
		PPM_LOGI("face: i %d, ID:%d, frmNum %d, rect: [%f, %f] [%f, %f], score %f \n", i,
				pstMatchFaceOut->face_match_info[i].id,
				pstMatchFaceOut->face_match_info[i].frame_num,
				pstMatchFaceOut->face_match_info[i].face_rect.x,
				pstMatchFaceOut->face_match_info[i].face_rect.y,
				pstMatchFaceOut->face_match_info[i].face_rect.width,
				pstMatchFaceOut->face_match_info[i].face_rect.height,
				pstMatchFaceOut->face_match_info[i].face_score);
	}

	/* szl_ppm_todo: 增加图像dump功能 */
    if (pstMatchFaceOut->obj_num == 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
exit:	
    return SAL_FAIL;
}

/**
 * @function   Ppm_DrvMatchFaceRsltCbFunc
 * @brief      关联匹配通道-人脸抓拍回调处理
 * @param[in]  INT32 nNodeID        
 * @param[in]  INT32 nCallBackType  
 * @param[in]  VOID *pstOutput      
 * @param[in]  UINT32 nSize         
 * @param[in]  VOID *pUsr           
 * @param[in]  INT32 nUserSize      
 * @param[out] None
 * @return     VOID
 */
VOID Ppm_DrvMatchFaceRsltCbFunc(INT32 nNodeID, 
	                            INT32 nCallBackType, 
	                            VOID *pstOutput, 
	                            UINT32 nSize, 
	                            VOID *pUsr, 
	                            INT32 nUserSize)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_RSLT_INFO_S *pstRsltInfo = NULL;

	/* debug: 打印关联匹配人脸抓拍结果，默认关闭 */
	s32Ret = Ppm_DrvPrMatchFaceOut(pstOutput);
    if (SAL_SOK != s32Ret)
    {
        goto exit;
    }

	/* 将包裹关联结果填入后处理队列 */
	s32Ret = Ppm_DrvGetRsltEmptQueBuf(0, &pstRsltInfo);
    PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvGetRsltEmptQueBuf failed!");

    s32Ret = Ppm_DrvGetFaceResult(pstOutput, &pstRsltInfo->stFaceRsltOut);
    PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvGetFaceResult failed!");

	s32Ret = Ppm_DrvPutRsltFullQueBuf(0, pstRsltInfo);
    PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvPutRsltFullQueBuf failed!");

	goto exit;

reuse:
	if (pstRsltInfo)
	{
		(VOID)Ppm_DrvPutRsltEmptQueBuf(0, pstRsltInfo);
		pstRsltInfo = NULL;
	}
	
exit:
	return;
}

/**
 * @function   Ppm_DrvPrMatchPackOut
 * @brief      调试接口：打印关联匹配可见光包裹结果
 * @param[in]  VOID *pstOutput  
 * @param[out] None
 * @return     static VOID
 */
static INT32 Ppm_DrvPrMatchPackOut(VOID *pstOutput)
{
	INT32 s32Ret = SAL_FAIL;
	
	UINT32 i = 0;
	
	PPM_MOD_S *pstPpmGlobalPrm = NULL;
	MTME_PACK_MATCH_OUT_T *pstMatchPackOut = NULL;
    ICF_MEDIA_INFO_V2 *pstMediaInfo = NULL;
    PPM_INPUT_DATA_INFO *pstInputDataInfo = NULL;

	//return;

	/* 获取hal层全局参数 */
	pstPpmGlobalPrm = Ppm_HalGetModPrm();
	PPM_DRV_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

	/* 获取当前帧状态，如果出错则直接丢弃 */
	s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageState(pstOutput);
	PPM_DRV_CHECK_RET(s32Ret, exit, "get package state failed!");

	/* 获取包裹关联结果 */
	pstMatchPackOut	= (MTME_PACK_MATCH_OUT_T *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, MTME_MATCH_PACK_DATA);
	PPM_DRV_CHECK_PTR(pstMatchPackOut, exit, "get match pack out failed!");
	
    if (pstMatchPackOut->obj_num != 0)
    {
        PPM_LOGI("get match pack obj_num: %d \n", pstMatchPackOut->obj_num);
    }
	
	for (i = 0; i < pstMatchPackOut->obj_num; i++)
	{
		PPM_LOGI("pack: i %d, ID:%d, pack_id: %d, frmNum %d \n", i,
				pstMatchPackOut->pack_match_info[i].id,
				pstMatchPackOut->pack_match_info[i].pack_id,
				pstMatchPackOut->pack_match_info[i].frame_num);
	}

    /* 修改透传引擎参数 */
    pstMediaInfo  = (ICF_MEDIA_INFO_V2  *)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    PPM_DRV_CHECK_PTR(pstMediaInfo, exit, "get match pack out failed!");

    pstInputDataInfo = (PPM_INPUT_DATA_INFO *)pstMediaInfo->stInputInfo.pUserPtr;
    pstInputDataInfo->bUsed = SAL_FALSE;

	/* szl_ppm_todo: 增加图像dump功能 */
    if (pstMatchPackOut->obj_num == 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
exit:
    return SAL_FAIL;
}

/**
 * @function   Ppm_DrvMatchPackRsltCbFunc
 * @brief      关联匹配通道-可见光包裹回调处理
 * @param[in]  INT32 nNodeID        
 * @param[in]  INT32 nCallBackType  
 * @param[in]  VOID *pstOutput      
 * @param[in]  UINT32 nSize         
 * @param[in]  VOID *pUsr           
 * @param[in]  INT32 nUserSize      
 * @param[out] None
 * @return     VOID
 */
VOID Ppm_DrvMatchPackRsltCbFunc(INT32 nNodeID, 
	                            INT32 nCallBackType, 
	                            VOID *pstOutput, 
	                            UINT32 nSize, 
	                            VOID *pUsr, 
	                            INT32 nUserSize)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_RSLT_INFO_S *pstRsltInfo = NULL;

	/* debug: 打印关联匹配包裹结果，默认关闭 */
	s32Ret = Ppm_DrvPrMatchPackOut(pstOutput);
    if (SAL_SOK != s32Ret)
    {
        goto exit;
    }

	/* 将包裹关联结果填入后处理队列 */
	s32Ret = Ppm_DrvGetRsltEmptQueBuf(0, &pstRsltInfo);
    PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvGetRsltEmptQueBuf failed!");

    s32Ret = Ppm_DrvGetPkgResult(pstOutput, &pstRsltInfo->stPkgRsltOut);
    PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvGetPkgResult failed!");

	s32Ret = Ppm_DrvPutRsltFullQueBuf(0, pstRsltInfo);
    PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvPutRsltFullQueBuf failed!");

	goto exit;

reuse:	
	if (pstRsltInfo)
	{
		(VOID)Ppm_DrvPutRsltEmptQueBuf(0, pstRsltInfo);
		pstRsltInfo = NULL;
	}
	
exit:
	return;
}

/**
 * @function:   Ppm_DrvInitEngInputPrm
 * @brief:      初始化引擎输入参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitEngInputPrm(VOID)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;

    PPM_SUB_MOD_S *pstSubModPrm = NULL;
    PPM_INPUT_DATA_INFO *pstInputDataInfo = NULL;

#if 0
    UINT32 uiResoTab[DMEO_SRC_DIM][2] =
    {
        {DEMO_FACE_WIDTH, DEMO_FACE_HEIGHT},
		{DEMO_TRI_VISION_WIDTH, DEMO_TRI_VISION_HEIGHT},
    };
#endif

    for (i = 0; i < g_stPpmCommPrm.uiChannelCnt; i++)
    {
        pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_MATCH);
        PPM_DRV_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

        pstSubModPrm->stIptDataPrm.uiIdx = 0;
        pstSubModPrm->stIptDataPrm.uiCnt = PPM_MAX_INPUT_DATA_NUM;
        for (j = 0; j < pstSubModPrm->stIptDataPrm.uiCnt; j++)
        {
            pstInputDataInfo = &pstSubModPrm->stIptDataPrm.stInputDataInfo[j];

            pstInputDataInfo->bUsed = SAL_FALSE;
            pstInputDataInfo->uiIdx = j;

			/* 填充人脸图像数据信息 */
			pstSubModPrm->stIptDataPrm.stMtmeInputData[j].face_image.image_w = DEMO_FACE_WIDTH;
			pstSubModPrm->stIptDataPrm.stMtmeInputData[j].face_image.image_h = DEMO_FACE_HEIGHT;
			pstSubModPrm->stIptDataPrm.stMtmeInputData[j].face_image.format = VCA_IMG_YUV_NV21;

			/* 填充三目可见光图像数据信息 */
			pstSubModPrm->stIptDataPrm.stMtmeInputData[j].vertical_image.image_w = DEMO_TRI_VISION_WIDTH;
			pstSubModPrm->stIptDataPrm.stMtmeInputData[j].vertical_image.image_h = DEMO_TRI_VISION_HEIGHT;
			pstSubModPrm->stIptDataPrm.stMtmeInputData[j].vertical_image.format = VCA_IMG_YUV_NV21;
			
            pstInputDataInfo->stIptDataInfo.nBlobNum = 1;//DMEO_SRC_DIM;       /* 多源数据 */
            for (k = 0; k < pstInputDataInfo->stIptDataInfo.nBlobNum; k++)
            {
                pstInputDataInfo->stIptDataInfo.stBlobData[k].nShape[0] = DEMO_FACE_WIDTH;//uiResoTab[k][0];
                pstInputDataInfo->stIptDataInfo.stBlobData[k].nShape[1] = DEMO_FACE_HEIGHT;//uiResoTab[k][1];
                pstInputDataInfo->stIptDataInfo.stBlobData[k].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21;
                pstInputDataInfo->stIptDataInfo.stBlobData[k].nFrameNum = 0;
                pstInputDataInfo->stIptDataInfo.stBlobData[k].nSpace = ICF_RN_MEM_MMZ_CMA_WITH_CACHE;
                pstInputDataInfo->stIptDataInfo.stBlobData[k].pData = &pstSubModPrm->stIptDataPrm.stMtmeInputData[j];
            }
        }
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitOutChnPrm
 * @brief:      初始化输出通道参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitOutChnPrm(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 uiCropX = 0;
    UINT32 uiCropY = 0;
    UINT32 uiCropW = PPM_TRI_CAM_VPSS_CROP_W;
    UINT32 uiCropH = PPM_TRI_CAM_VPSS_CROP_H;

    DUP_ChanHandle *pDupChnHandle = NULL;

    PARAM_INFO_S stDupChnParam = {0};

    pDupChnHandle = (DUP_ChanHandle *)Ppm_DrvGetVprocHandle(0, i);
    PPM_DRV_CHECK_PTR(pDupChnHandle, err, "pDupChnHandle == null!");


    for (i = 0; i < PPM_TRI_CAM_OUT_CHN_NUM; i++)
    {
        uiCropX = g_stPpmOutChnPrm[i].uiCropX;
        uiCropY = g_stPpmOutChnPrm[i].uiCropY;

        pDupChnHandle = (DUP_ChanHandle *)Ppm_DrvGetVprocHandle(0, i);
        PPM_DRV_CHECK_PTR(pDupChnHandle, err, "pDupChnHandle == null!");

        /* 配置物理通道裁剪属性 */
        {
	        sal_memset_s(&stDupChnParam, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));

	        stDupChnParam.enType = CHN_CROP_CFG;
            
	        stDupChnParam.stCrop.u32CropEnable = SAL_TRUE;
	        stDupChnParam.stCrop.enCropType = CROP_ORIGN;
	        stDupChnParam.stCrop.u32X = uiCropX;
	        stDupChnParam.stCrop.u32Y = uiCropY;
	        stDupChnParam.stCrop.u32W = uiCropW;
	        stDupChnParam.stCrop.u32H = uiCropH;

	        s32Ret = pDupChnHandle->dupOps.OpDupSetBlitPrm(pDupChnHandle, &stDupChnParam);
	        PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");

#if 1
	        sal_memset_s(&stDupChnParam, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));
            stDupChnParam.enType = IMAGE_SIZE_CFG;
            stDupChnParam.stImgSize.u32Width = g_stPpmOutChnPrm[i].uiOutChnW;
            stDupChnParam.stImgSize.u32Height = g_stPpmOutChnPrm[i].uiOutChnH;

	        s32Ret = pDupChnHandle->dupOps.OpDupSetBlitPrm(pDupChnHandle, &stDupChnParam);
	        PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");
#endif

	        sal_memset_s(&stDupChnParam, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));
            stDupChnParam.enType = PIX_FORMAT_CFG;
            stDupChnParam.enPixFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

	        s32Ret = pDupChnHandle->dupOps.OpDupSetBlitPrm(pDupChnHandle, &stDupChnParam);
	        PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");

	        s32Ret = pDupChnHandle->dupOps.OpDupStartBlit(pDupChnHandle);
	        PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn enable switch failed!");

           SAL_INFO("i %d, x %d, y %d, w %d, h %d, dst[w %d, h %d] \n", i, uiCropX, uiCropY, uiCropW, uiCropH, g_stPpmOutChnPrm[i].uiOutChnW,g_stPpmOutChnPrm[i].uiOutChnH);

        }

#if 0
        /* 配置拓展通道宽高属性 */
        {
	        pDupChnHandle = (DUP_ChanHandle *)Ppm_DrvGetVprocHandle(0, i + dup_hal_getPhysChnNum());
	        PPM_DRV_CHECK_PTR(pDupChnHandle, err, "pDupChnHandle == null!");

	        sal_memset_s(&stDupChnParam, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));

	        stDupChnParam.enType = IMAGE_SIZE_EXT_CFG;
	        stDupChnParam.stImgSize.u32Width = g_stPpmOutChnPrm[i].uiOutChnW;
	        stDupChnParam.stImgSize.u32Height = g_stPpmOutChnPrm[i].uiOutChnH;

	        s32Ret = pDupChnHandle->dupOps.OpDupSetBlitPrm(pDupChnHandle, &stDupChnParam);
	        PPM_DRV_CHECK_RET(s32Ret, err, "set dup chn crop param failed!");

	        SAL_INFO("i %d, set ext chn [%d x %d] \n", i, stDupChnParam.stImgSize.u32Width, stDupChnParam.stImgSize.u32Height);
        }
#endif
    }

    SAL_INFO("%s end! \n", __func__);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDestroyVdecChn
 * @brief:      销毁解码通道
 * @param[in]:  UINT32 u32Chan
 * @param[in]:  UINT32 u32VdecChn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDestroyVdecChn(UINT32 u32Chan, UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;

    PPM_DEV_PRM *pstDevPrm = NULL;

    pstDevPrm = &g_stPpmCommPrm.stPpmDevPrm[u32Chan];

    /* TODO: 当前解码通道默认绑定后级DUP，故此处传入vdec和dup绑定状态为SAL_TRUE */
    s32Ret = vdec_tsk_DestroyVdec(u32VdecChn, SAL_TRUE, pstDevPrm->stJdecPrm.hDupFront, pstDevPrm->stJdecPrm.hDupRear);
    PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_DestroyVdec failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvCreateVdecChn
 * @brief:      创建解码通道
 * @param[in]:  UINT32 u32Chan
 * @param[in]:  UINT32 u32VdecChn
 * @param[in]:  VDEC_PRM *pstVdecCreatePrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvCreateVdecChn(UINT32 u32Chan, UINT32 u32VdecChn, VDEC_PRM *pstVdecCreatePrm)
{
    INT32 s32Ret = SAL_SOK;
    PPM_DEV_PRM *pstDevPrm = NULL;

    PPM_DRV_CHECK_PTR(pstVdecCreatePrm, err, "alloc mmz failed!");

    s32Ret = vdec_tsk_CreateVdec(u32VdecChn, pstVdecCreatePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "vdec_tsk_CreateVdec failed!");

    pstDevPrm = &g_stPpmCommPrm.stPpmDevPrm[u32Chan];

    s32Ret = dup_task_vdecDupCreate(&stPpmVdecDupInstCfg, u32VdecChn, &pstDevPrm->stJdecPrm.hDupFront, &pstDevPrm->stJdecPrm.hDupRear, pstVdecCreatePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "dup_task_vdecDupCreate failed!");

    s32Ret = Vdec_task_PpmDupChnHandleCreate(u32VdecChn, (void*)&pstDevPrm->stJdecPrm.hDupFront);
    PPM_DRV_CHECK_RET(s32Ret, err, "Vdec_task_vdecDupChnHandleCreate failed!");

    if (SAL_SOK != (s32Ret = vdec_hal_VdecStart(u32VdecChn)))
    {
        VDEC_LOGE("chn %d start failed ret 0x%x\n", u32VdecChn, s32Ret);
        return SAL_FAIL;
    }

    SAL_INFO("ppm create vdec chn %d \n", u32VdecChn);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDeinitGlobalVar
 * @brief:      去初始化业务层全局变量
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDeinitGlobalVar(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;

    for (i = 0; i < g_stPpmCommPrm.uiChannelCnt; i++)
    {
        if (i >= MAX_PPM_CHN_NUM)
        {
            PPM_LOGE("invalid i %d \n", i);
            continue;
        }

        pstDevPrm = Ppm_DrvGetDevPrm(i);
        PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

        for (j = 0; j < PPM_JPEG_CB_NUM; j++)
        {
            s32Ret = mem_hal_mmzFree(pstDevPrm->stPkgJpegInfo[j].pcJpegAddr, "PPM", "ppm_pkg_jpeg");
            PPM_DRV_CHECK_RET(s32Ret, err, "free mmz failed!");

            pstDevPrm->stPkgJpegInfo[j].pcJpegAddr = NULL;

#if 0
            s32Ret = mem_hal_mmzFree(pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr, "PPM", "ppm_xsi_jpeg");
            PPM_DRV_CHECK_RET(s32Ret, err, "free mmz failed!");

            pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr = NULL;
#endif

            s32Ret = mem_hal_mmzFree(pstDevPrm->stFaceJpegInfo[j].pcJpegAddr, "PPM", "ppm_face_jpeg");
            PPM_DRV_CHECK_RET(s32Ret, err, "free mmz failed!");

            pstDevPrm->stFaceJpegInfo[j].pcJpegAddr = NULL;
        }

        for (j = 0; j < 2; j++)
        {
            if (0x00 != pstDevPrm->stTmpFrame[j].uiAppData)
            {
                (VOID)sys_hal_rleaseVideoFrameInfoSt(&pstDevPrm->stTmpFrame[j]);
            }
        }

        /* jpeg 2 yuv相关参数初始化 */
        for (j = 0; j < PPM_MAX_CALIB_JPEG_NUM; j++)
        {
            pstDevPrm->stJdecPrm.enFaceProcMode[j] = PPM_JDEC_MODE_NUM;
            s32Ret = mem_hal_mmzFree(&pstDevPrm->stJdecPrm.stFaceSysFrame[j].pcJpegAddr, "PPM", "ppm_calib_jpeg");
            PPM_DRV_CHECK_RET(s32Ret, err, "free mmz failed!");
            memset(pstDevPrm->stJdecPrm.stFaceSysFrame[j].pcJpegAddr, 0x00, 1920*1080);

            pstDevPrm->stJdecPrm.enTriProcMode[j] = PPM_JDEC_MODE_NUM;
            s32Ret = mem_hal_mmzFree(&pstDevPrm->stJdecPrm.stTriSysFrame[j].pcJpegAddr, "PPM", "ppm_calib_jpeg");
            PPM_DRV_CHECK_RET(s32Ret, err, "free mmz failed!");
            memset(pstDevPrm->stJdecPrm.stTriSysFrame[j].pcJpegAddr, 0x00, 1920*1080);
        }

        /* jpeg 2 yuv相关参数初始化 */
        for (j = 0; j < PPM_MAX_FACE_CALIB_JPEG_NUM; j++)
        {
            pstDevPrm->stJdecPrm.enFaceCamProcMode[j] = PPM_JDEC_MODE_NUM;
            (VOID)sys_hal_rleaseVideoFrameInfoSt(&pstDevPrm->stJdecPrm.stFaceCamSysFrame[j]);
        }

        (VOID)Ppm_DrvDestroyVdecChn(i, PPM_VDEC_CHN_CALIB_JDEC);

#if 0  /* unused */
        for (j = 0; j < PPM_TRI_CAM_OUT_CHN_NUM; j++)
        {
            pstSysFrm = &g_stPpmCommPrm.stPpmDevPrm[i].stVdecFrame[j];

            if (0x00 == pstSysFrm->uiAppData)
            {
                SAL_WARN("buf is deinit! \n");
                continue;
            }

            (VOID)Ppm_DrvPutFrameMem(pstSysFrm);
            SAL_WARN("disp get frame mem end! chan %d, j %d \n", i, j);
        }

#endif
        memset(&g_stPpmCommPrm.stPpmDevPrm[i], 0x00, sizeof(PPM_DEV_PRM));
    }

    g_stPpmCommPrm.uiChannelCnt = 0;
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetCurMs
 * @brief:      获取当前系统时间ms
 * @param[in]:  DEBUG_LEVEL_E enLevel
 * @param[in]:  UINT32 *puiTime
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Ppm_DrvGetCurMs(DEBUG_LEVEL_E enLevel, UINT32 *puiTime)
{
    if (enLevel > SAL_getModLogLevel(MOD_AI_PPM))
    {
        return;
    }

    *puiTime = SAL_getCurMs();
    return;
}

/**
 * @function:   Ppm_DrvPutQueData
 * @brief:      将数据放入队列
 * @param[in]:  DSA_QueHndl *pstQueHdl
 * @param[in]:  VOID *pData
 * @param[in]:  UINT32 uiTimeOut
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Ppm_DrvPutQueData(DSA_QueHndl *pstQueHdl, VOID *pData, UINT32 uiTimeOut)
{
    UINT32 time0 = 0;
    UINT32 time1 = 0;

    (VOID)Ppm_DrvGetCurMs(DEBUG_LEVEL4, &time0);
    DSA_QuePut(pstQueHdl, pData, uiTimeOut);
    (VOID)Ppm_DrvGetCurMs(DEBUG_LEVEL4, &time1);

    PPM_LOGT("put que data cost %d ms. \n", time1 - time0);
    return;
}

/**
 * @function:   Ppm_DrvGetQueData
 * @brief:      获取队列数据
 * @param[in]:  DSA_QueHndl *pstQueHdl
 * @param[in]:  VOID **ppData
 * @param[in]:  UINT32 uiTimeOut
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Ppm_DrvGetQueData(DSA_QueHndl *pstQueHdl, VOID **ppData, UINT32 uiTimeOut)
{
    UINT32 time0 = 0;
    UINT32 time1 = 0;

    (VOID)Ppm_DrvGetCurMs(DEBUG_LEVEL4, &time0);
    DSA_QueGet(pstQueHdl, ppData, uiTimeOut);
    (VOID)Ppm_DrvGetCurMs(DEBUG_LEVEL4, &time1);

    PPM_LOGT("put que data cost %d ms. \n", time1 - time0);
    return;
}

/**
 * @function:   Ppm_DrvDeInitQue
 * @brief:      去初始化队列参数（通用）
 * @param[in]:  PPM_QUEUE_PRM *pstQuePrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDeInitQue(PPM_QUEUE_PRM *pstQuePrm)
{
    INT32 s32Ret = SAL_SOK;

    PPM_DRV_CHECK_PTR(pstQuePrm, err, "pstQuePrm == null!");

    if (NULL != pstQuePrm->pstEmptQue)
    {
        s32Ret = DSA_QueDelete(pstQuePrm->pstEmptQue);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("error !!!\n");
        }
    }

    (VOID)Ppm_HalMemFree(pstQuePrm->pstEmptQue);

    if (NULL != pstQuePrm->pstFullQue)
    {
        s32Ret = DSA_QueDelete(pstQuePrm->pstFullQue);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("error !!!\n");
        }
    }

    (VOID)Ppm_HalMemFree(pstQuePrm->pstFullQue);

    SAL_INFO("deinit Que end! \n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitQue
 * @brief:      初始化队列参数（通用）
 * @param[in]:  PPM_QUEUE_PRM *pstQuePrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitQue(PPM_QUEUE_PRM *pstQuePrm)
{
    PPM_DRV_CHECK_PTR(pstQuePrm, err, "pstQuePrm == null!");

    if (pstQuePrm->enType >= MAX_QUEUE_TYPE
        || 0 == pstQuePrm->uiBufCnt
        || 0 == pstQuePrm->uiBufSize)
    {
        SAL_ERROR("Invalid input args: type %d, buf_cnt %d, buf_size %d \n",
                  pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);
        return SAL_FAIL;
    }

    pstQuePrm->pstEmptQue = Ppm_HalMemAlloc(sizeof(DSA_QueHndl), SAL_FALSE, "que handle");
    PPM_DRV_CHECK_PTR(pstQuePrm->pstEmptQue, err, "mem alloc failed!");

    if (SAL_SOK != DSA_QueCreate(pstQuePrm->pstEmptQue, pstQuePrm->uiBufCnt))
    {
        SAL_ERROR("error !!!\n");
        goto err;
    }

    if (DOUBLE_QUEUE_TYPE == pstQuePrm->enType)
    {
        pstQuePrm->pstFullQue = Ppm_HalMemAlloc(sizeof(DSA_QueHndl), SAL_FALSE, "que handle");
        PPM_DRV_CHECK_PTR(pstQuePrm->pstFullQue, err, "mem alloc failed!");

        if (SAL_SOK != DSA_QueCreate(pstQuePrm->pstFullQue, pstQuePrm->uiBufCnt))
        {
            SAL_ERROR("error !!!\n");
            goto err;
        }
    }

    SAL_INFO("init Que end! type %d, cnt %d, size %d \n", pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);
    return SAL_SOK;

err:
    (VOID)Ppm_DrvDeInitQue(pstQuePrm);
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDeinitRsltQue
 * @brief:      去初始化结果队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDeinitRsltQue(UINT32 chan)
{
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_RSLT_QUE_PRM *pstRsltQueInfo = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstRsltQueInfo = &pstDevPrm->stRsltQueInfo;

    (VOID)Ppm_DrvDeInitQue(&pstRsltQueInfo->stRsltQuePrm);

    SAL_INFO("Deinit Que Result End! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitRsltQue
 * @brief:      初始化结果队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitRsltQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_RSLT_QUE_PRM *pstRsltQueInfo = NULL;
    PPM_QUEUE_PRM *pstRsltQuePrm = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstRsltQueInfo = &pstDevPrm->stRsltQueInfo;

    pstRsltQuePrm = &pstRsltQueInfo->stRsltQuePrm;

    pstRsltQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstRsltQuePrm->uiBufCnt = PPM_MAX_QUEUE_BUF_NUM_V0;
    pstRsltQuePrm->uiBufSize = sizeof(PPM_RSLT_INFO_S);

    s32Ret = Ppm_DrvInitQue(pstRsltQuePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQue failed!");

    pstEmptQue = pstRsltQuePrm->pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstRsltQuePrm->uiBufCnt; i++)
    {
        s32Ret = DSA_QuePut(pstEmptQue, (void *)&pstRsltQueInfo->stRsltInfo[i], SAL_TIMEOUT_NONE);
        PPM_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    SAL_INFO("Init Que Result End! chan %d, type %d, cnt %d, size %d \n",
             chan, pstRsltQuePrm->enType, pstRsltQuePrm->uiBufCnt, pstRsltQuePrm->uiBufSize);

    return SAL_SOK;
err:
    (VOID)Ppm_DrvDeinitRsltQue(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDeInitYuvQue
 * @brief:      去初始化YUV队列
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_TRI_CAM_CHN_E enCamChn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDeInitYuvQue(UINT32 chan)
{
    UINT32 i = 0;
    UINT32 j = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_YUV_QUE_PRM *pstYuvQueInfo = NULL;
    PPM_QUEUE_PRM *pstYuvQuePrm = NULL;
    PPM_FRAME_INFO_ST *pstYuvFrame = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstYuvQueInfo = &pstDevPrm->stYuvQueInfo;
    PPM_DRV_CHECK_RET(!pstYuvQueInfo, exit, "pstYuvQueInfo == null!");

    pstYuvQuePrm = &pstYuvQueInfo->stYuvQuePrm;
    if (pstYuvQuePrm)
    {
        for (i = 0; i < pstYuvQuePrm->uiBufCnt; i++)
        {
            pstYuvFrame = &pstYuvQueInfo->stYuvFrame[i];

            /* 三目相机YUV帧数据内存 */
            (VOID)Ppm_DrvPutFrameMem(&pstYuvFrame->stTriCamFrame);

            /* 安检机输出帧数据内存 */
            for (j = 0; j < PPM_MAX_QUE_FRM_NUM; j++)
            {
                (VOID)Ppm_DrvPutFrameMem(&pstYuvFrame->stXpkgFrame[j]);
            }

            /* 人脸帧数据内存 */
            (VOID)Ppm_DrvPutFrameMem(&pstYuvFrame->stFaceFrame);
        }

        (VOID)Ppm_DrvDeInitQue(pstYuvQuePrm);
    }

exit:
    SAL_INFO("deinit Que Yuv Frm End! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitFaceYuvQue
 * @brief:      初始化人脸相机YUV队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitFaceYuvQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiW = 0;
    UINT32 uiH = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_FACE_YUV_QUE_PRM *pstYuvQueInfo = NULL;
    PPM_QUEUE_PRM *pstYuvQuePrm = NULL;
    PPM_COM_FRAME_INFO_ST *pstYuvFrame = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstYuvQueInfo = &pstDevPrm->stFaceYuvQueInfo;

    pstYuvQuePrm = &pstYuvQueInfo->stYuvQuePrm;

    pstYuvQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstYuvQuePrm->uiBufCnt = PPM_MAX_QUEUE_BUF_NUM_V1;
    pstYuvQuePrm->uiBufSize = sizeof(PPM_COM_FRAME_INFO_ST);

    s32Ret = Ppm_DrvInitQue(pstYuvQuePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQue failed!");

    pstEmptQue = pstYuvQuePrm->pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstYuvQuePrm->uiBufCnt; i++)
    {
        pstYuvFrame = &pstYuvQueInfo->stYuvFrame[i];
        PPM_DRV_CHECK_PTR(pstYuvFrame, err, "pstYuvFrame == null!");

        pstYuvFrame->uiFrmCnt = 1;    /* 暂定人脸只有一张数据用于处理 */
        for (j = 0; j < pstYuvFrame->uiFrmCnt; j++)
        {
            uiW = PPM_FACE_WIDTH;
            uiH = PPM_FACE_HEIGHT;
            s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstYuvFrame->stFrame[j]);
            PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");
        }

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);
        PPM_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    SAL_INFO("Init TriCam Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
             chan, pstYuvQuePrm->enType, pstYuvQuePrm->uiBufCnt, pstYuvQuePrm->uiBufSize);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitTriYuvQue
 * @brief:      初始化三目相机YUV队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitTriYuvQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiW = 0;
    UINT32 uiH = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_TRI_YUV_QUE_PRM *pstYuvQueInfo = NULL;
    PPM_QUEUE_PRM *pstYuvQuePrm = NULL;
    PPM_COM_FRAME_INFO_ST *pstYuvFrame = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstYuvQueInfo = &pstDevPrm->stTriYuvQueInfo;

    pstYuvQuePrm = &pstYuvQueInfo->stYuvQuePrm;

    pstYuvQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstYuvQuePrm->uiBufCnt = PPM_MAX_QUEUE_BUF_NUM_V1;
    pstYuvQuePrm->uiBufSize = sizeof(PPM_COM_FRAME_INFO_ST);

    s32Ret = Ppm_DrvInitQue(pstYuvQuePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQue failed!");

    pstEmptQue = pstYuvQuePrm->pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstYuvQuePrm->uiBufCnt; i++)
    {
        pstYuvFrame = &pstYuvQueInfo->stYuvFrame[i];
        PPM_DRV_CHECK_PTR(pstYuvFrame, err, "pstYuvFrame == null!");

        pstYuvFrame->uiFrmCnt = 1;    /* 暂定三目只有一张数据用于处理 */
        for (j = 0; j < pstYuvFrame->uiFrmCnt; j++)
        {
            uiW = PPM_TRI_CAM_CHN_WIDTH;
            uiH = PPM_TRI_CAM_CHN_HEIGHT;
            s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstYuvFrame->stFrame[j]);
            PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");
        }

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);
        PPM_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    SAL_INFO("Init TriCam Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
             chan, pstYuvQuePrm->enType, pstYuvQuePrm->uiBufCnt, pstYuvQuePrm->uiBufSize);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDeInitInputDataQue
 * @brief:      去初始引擎输入数据队列
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_TRI_CAM_CHN_E enCamChn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDeInitInputDataQue(UINT32 chan)
{
    UINT32 i = 0;
    UINT32 j = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_INPUT_DATA_QUE_PRM *pstIptQueInfo = NULL;
    PPM_QUEUE_PRM *pstIptQuePrm = NULL;
    PPM_IPT_YUV_DATA_INFO *pstIptData = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstIptQueInfo = &pstDevPrm->stIptQueInfo;
    pstIptQuePrm = &pstIptQueInfo->stIptQuePrm;

    for (i = 0; i < pstIptQuePrm->uiBufCnt; i++)
    {
        pstIptData = &pstIptQueInfo->stIptYuvData[i];

        /* 三目相机YUV帧数据内存 */
        for (j = 0; j < PPM_TRI_CAM_OUT_CHN_NUM; j++)
        {
            (VOID)Ppm_DrvPutFrameMem(&pstIptData->stTriViewYuv[j]);
        }

        /* 安检机输出帧数据内存 */
        /* for (j = 0; j < PPM_MAX_QUE_FRM_NUM; j++) */
        {
            (VOID)Ppm_DrvPutFrameMem(&pstIptData->stXpkgYuv[0]);
        }

        /* 人脸帧数据内存 */
        (VOID)Ppm_DrvPutFrameMem(&pstIptData->stFaceYuv);
    }

    (VOID)Ppm_DrvDeInitQue(pstIptQuePrm);

    SAL_INFO("deinit input data Que End! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitInputDataQue
 * @brief:      初始化引擎输入数据队列
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_TRI_CAM_CHN_E enCamChn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitInputDataQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;

    UINT32 uiW = 0;
    UINT32 uiH = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_INPUT_DATA_QUE_PRM *pstIptQueInfo = NULL;
    PPM_QUEUE_PRM *pstIptQuePrm = NULL;
    PPM_IPT_YUV_DATA_INFO *pstIptYuvData = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstIptQueInfo = &pstDevPrm->stIptQueInfo;

    pstIptQuePrm = &pstIptQueInfo->stIptQuePrm;

    pstIptQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstIptQuePrm->uiBufCnt = PPM_MAX_QUEUE_BUF_NUM_V0;
    pstIptQuePrm->uiBufSize = sizeof(PPM_IPT_YUV_DATA_INFO);

    s32Ret = Ppm_DrvInitQue(pstIptQuePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQue failed!");

    pstEmptQue = pstIptQuePrm->pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstIptQuePrm->uiBufCnt; i++)
    {
        pstIptYuvData = &pstIptQueInfo->stIptYuvData[i];
        PPM_DRV_CHECK_PTR(pstIptYuvData, err, "pstIptYuvData == null!");

        /* 三目相机YUV帧数据内存 */
        for (j = 0; j < PPM_TRI_CAM_OUT_CHN_NUM; j++)
        {
            uiW = g_stPpmOutChnPrm[j].uiOutChnW;
            uiH = g_stPpmOutChnPrm[j].uiOutChnH;

            s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstIptYuvData->stTriViewYuv[j]);
            PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");

            SAL_WARN("disp get frame mem end! chan %d, j %d, w %d, h %d \n", i, j, uiW, uiH);
        }

        /* 安检机输出帧数据内存 */
        /* for(j = 0; j < PPM_MAX_QUE_FRM_NUM; j++) */
        {
            uiW = PPM_ISM_CHN_WIDTH;
            uiH = PPM_ISM_CHN_HEIGHT;
            s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstIptYuvData->stXpkgYuv[0]);   /* 此处只有通道1的X光数据申请内存 */
            PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");
        }

        /* 人脸帧数据内存 */
        uiW = PPM_FACE_WIDTH;
        uiH = PPM_FACE_HEIGHT;
        s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstIptYuvData->stFaceYuv);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstIptYuvData, SAL_TIMEOUT_NONE);
        PPM_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    SAL_INFO("Init Input Data Que End! chan %d, type %d, cnt %d, size %d \n",
             chan, pstIptQuePrm->enType, pstIptQuePrm->uiBufCnt, pstIptQuePrm->uiBufSize);
    return SAL_SOK;

err:
    (VOID)Ppm_DrvDeInitInputDataQue(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitYuvQue
 * @brief:      初始化YUV队列
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_TRI_CAM_CHN_E enCamChn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitYuvQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    UINT32 uiW = 0;
    UINT32 uiH = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_YUV_QUE_PRM *pstYuvQueInfo = NULL;
    PPM_QUEUE_PRM *pstYuvQuePrm = NULL;
    PPM_FRAME_INFO_ST *pstYuvFrame = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstYuvQueInfo = &pstDevPrm->stYuvQueInfo;

    pstYuvQuePrm = &pstYuvQueInfo->stYuvQuePrm;
    pstYuvQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstYuvQuePrm->uiBufCnt = PPM_MAX_QUEUE_BUF_NUM_V1;
    pstYuvQuePrm->uiBufSize = sizeof(PPM_FRAME_INFO_ST);

    s32Ret = Ppm_DrvInitQue(pstYuvQuePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQue failed!");

    pstEmptQue = pstYuvQuePrm->pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstYuvQuePrm->uiBufCnt; i++)
    {
        pstYuvFrame = &pstYuvQueInfo->stYuvFrame[i];
        PPM_DRV_CHECK_PTR(pstYuvFrame, err, "pstYuvFrame == null!");

        /* 三目相机YUV帧数据内存 */
        uiW = PPM_TRI_CAM_CHN_WIDTH;
        uiH = PPM_TRI_CAM_CHN_HEIGHT;
        s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstYuvFrame->stTriCamFrame);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");

#if 0
        /* 安检机输出帧数据内存 */
        /* for(j = 0; j < PPM_MAX_QUE_FRM_NUM; j++) */
        {
            uiW = PPM_ISM_CHN_WIDTH;
            uiH = PPM_ISM_CHN_HEIGHT;
            s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstYuvFrame->stXpkgFrame[0]);   /* 此处只有通道1的X光数据申请内存 */
            PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");
        }
#endif

        /* 人脸帧数据内存 */
        uiW = PPM_FACE_WIDTH;
        uiH = PPM_FACE_HEIGHT;
        s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstYuvFrame->stFaceFrame);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);
        PPM_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

#if 0
    /* 初始化用于第二路X光数据存放的内存 */
    pstDevPrm->stXsiSecBufPrm.uiIdx = 0;
    pstDevPrm->stXsiSecBufPrm.uiBufCnt = PPM_XSI_SEC_CHN_BUF_NUM;
    for (i = 0; i < PPM_XSI_SEC_CHN_BUF_NUM; i++)
    {
        uiW = PPM_ISM_CHN_WIDTH;
        uiH = PPM_ISM_CHN_HEIGHT;

        pstDevPrm->stXsiSecBufPrm.stXsiSecChnYuvBuf[i].uiIdx = i;
        pstDevPrm->stXsiSecBufPrm.stXsiSecChnYuvBuf[i].bUsed = SAL_FALSE;
        s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstDevPrm->stXsiSecBufPrm.stXsiSecChnYuvBuf[i].stFrameInfo);    /* 此处只有通道1的X光数据申请内存 */
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");
    }

#endif

    SAL_INFO("Init Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
             chan, pstYuvQuePrm->enType, pstYuvQuePrm->uiBufCnt, pstYuvQuePrm->uiBufSize);
    return SAL_SOK;

err:
    (VOID)Ppm_DrvDeInitYuvQue(chan);
    return SAL_FAIL;
}

/**
 * @function   Ppm_DrvDeinitDepthInfoQue
 * @brief      去初始化深度信息结果队列
 * @param[in]  UINT32 chan  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_DrvDeinitDepthInfoQue(UINT32 chan)
{
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_DEPTH_INFO_QUE_PRM *pstQueInfo = NULL;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQueInfo = &pstDevPrm->stDepthInfoQueInfo;

    (VOID)Ppm_DrvDeInitQue(&pstQueInfo->stDepthQuePrm);

    PPM_LOGI("Deinit depth Que End! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitDepthInfoQue
 * @brief:      初始化深度结果队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitDepthInfoQue(UINT32 chan)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_DEPTH_INFO_QUE_PRM *pstQueInfo = NULL;
    PPM_QUEUE_PRM *pstQuePrm = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    PPM_DRV_CHECK_MOD_CHAN(chan, SAL_FAIL);

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQueInfo = &pstDevPrm->stDepthInfoQueInfo;

    pstQuePrm = &pstQueInfo->stDepthQuePrm;

    pstQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstQuePrm->uiBufCnt = PPM_DEPTH_INFO_MAX_QUEUE_SIZE;
    pstQuePrm->uiBufSize = sizeof(PPM_DEPTH_INFO_S);

    s32Ret = Ppm_DrvInitQue(pstQuePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQue failed!");

    pstEmptQue = pstQuePrm->pstEmptQue;
    PPM_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstQuePrm->uiBufCnt; i++)
    {
        s32Ret = DSA_QuePut(pstEmptQue, (void *)&pstQueInfo->astDepthInfo[i], SAL_TIMEOUT_NONE);
        PPM_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    PPM_LOGI("Init Depth Info Que End! chan %d, type %d, cnt %d, size %d \n",
             chan, pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);

    return SAL_SOK;
err:
    (VOID)Ppm_DrvDeinitDepthInfoQue(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDeinitQue
 * @brief:      去初始化数据队列
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvDeinitQueData(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;

    for (i = 0; i < g_stPpmCommPrm.uiChannelCnt; i++)
    {
        pstDevPrm = Ppm_DrvGetDevPrm(i);
        PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

        s32Ret = Ppm_DrvDeInitYuvQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvDeinitYuvQue failed!");

        s32Ret = Ppm_DrvDeInitInputDataQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvDeInitInputDataQue failed!");

        s32Ret = Ppm_DrvDeinitDepthInfoQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvDeinitDepthInfoQue failed!");
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitQue
 * @brief:      初始化数据队列
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitQueData(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    for (i = 0; i < g_stPpmCommPrm.uiChannelCnt; i++)
    {
        s32Ret = Ppm_DrvInitYuvQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitYuvQue failed!");

        s32Ret = Ppm_DrvInitInputDataQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitYuvQue failed!");

#ifndef PPM_SYNC_V0
        s32Ret = Ppm_DrvInitTriYuvQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitTriYuvQue failed!");

        s32Ret = Ppm_DrvInitFaceYuvQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitFaceYuvQue failed!");
#endif

        s32Ret = Ppm_DrvInitRsltQue(i);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitYuvQue failed!");

#if 1   /* 人包相机迁移项目适配，初始化深度信息结果队列 */
		s32Ret = Ppm_DrvInitDepthInfoQue(i);
		PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitDepthInfoQue failed!");
#endif
    }

    return SAL_SOK;

err:
    (VOID)Ppm_DrvDeinitQueData();
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvSendJenc
 * @brief:      Jpeg编码
 * @param[in]:  UINT32 uiChn
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[in]:  PPM_JPEG_PRM *pstPicInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvSendJenc(VOID *pJpegChnInfo, SYSTEM_FRAME_INFO *pstSysFrame, PPM_JPEG_PRM *pstPicInfo)
{
    INT32 s32Ret = SAL_SOK;

    JPEG_COMMON_ENC_PRM_S stJpegEncPrm = {0};
    CROP_S stCropPrm = {0};

    PPM_DRV_CHECK_PTR(pJpegChnInfo, err, "pJpegChnInfo == null!");
    PPM_DRV_CHECK_PTR(pstSysFrame, err, "pstSysFrame == null!");
    PPM_DRV_CHECK_PTR(pstPicInfo, err, "pstPicInfo == null!");

    sal_memcpy_s(&stJpegEncPrm.stSysFrame, sizeof(SYSTEM_FRAME_INFO), pstSysFrame, sizeof(SYSTEM_FRAME_INFO));
    stJpegEncPrm.pOutJpeg = (INT8 *)pstPicInfo->pcJpegAddr;

    stCropPrm.u32CropEnable = SAL_TRUE;
    stCropPrm.u32X = 0;
    stCropPrm.u32Y = 0;
    stCropPrm.u32W = SAL_alignDown(pstSysFrame->uiDataWidth + 2, 4);
    stCropPrm.u32H = SAL_alignDown(pstSysFrame->uiDataHeight + 2, 4);

    /* jpeg编图: 无叠框信息 */
    s32Ret = jpeg_drv_cropEnc(pJpegChnInfo, &stJpegEncPrm, &stCropPrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "jpeg_drv_cropEnc failed!");

    pstPicInfo->uiJpegSize = stJpegEncPrm.outSize;

    SAL_WARN("crop flag %d, x %d, y %d, w %d, h %d \n", stCropPrm.u32CropEnable,
             stCropPrm.u32X, stCropPrm.u32Y, stCropPrm.u32W, stCropPrm.u32H);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvCalThrFps
 * @brief:      计算线程帧率
 * @param[in]:  UINT64 *ptime000
 * @param[in]:  UINT64 *ptime111
 * @param[in]:  UINT32 *pfps
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvCalThrFps(UINT64 *ptime000, UINT64 *ptime111, UINT32 *pfps, const char *str)
{
    INT32 s32Ret = SAL_SOK;

    PPM_DRV_CHECK_PTR(ptime000, exit, "ptr null!");
    PPM_DRV_CHECK_PTR(ptime111, exit, "ptr null!");
    PPM_DRV_CHECK_PTR(pfps, exit, "ptr null!");

    /* 统计线程帧率 */
    if ((*ptime111 - *ptime000 >= 990) && (*pfps > 0))
    {
        SVA_LOGW("%s: fps %d, gap %llu. \n", str, *pfps, *ptime111 - *ptime000);
        *pfps = 0;
    }

    if (*pfps == 0)
    {
        s32Ret = sys_hal_GetSysCurPts(ptime000);
        if (s32Ret != 0)
        {
            *ptime000 = SAL_getCurMs();
            SVA_LOGW("error\n");
        }
        else
        {
            *ptime000 = (*ptime000 / 1000);
        }
    }

    s32Ret = sys_hal_GetSysCurPts(ptime111);
    if (s32Ret != 0)
    {
        *ptime111 = SAL_getCurMs();
        SVA_LOGW("error\n");
    }
    else
    {
        *ptime111 = (*ptime111 / 1000);
    }

exit:
    return SAL_SOK;
}

#define PPM_SYNC_FRM_NUM	(3)                          /* 待同步的数据源个数 */
#define PPM_INPUT_FPS		(20)                         /* 待控制的帧率 */
#define PPM_SYNC_GAP_MS		(1000 / PPM_INPUT_FPS)       /* 帧间隔，ms */

static UINT64 gap = 100;     /* 同步要求100ms */
static BOOL bFlag = SAL_FALSE;

typedef struct _DBG_SYNC_PRM_
{
    BOOL flag;                  /* 结果是否有效 */
    SYSTEM_FRAME_INFO *pFrm;    /* 帧信息 */
    UINT64 pts;
} DBG_SYNC_PRM;

/******************************************************************
   Function:   set_sync_gap
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
VOID set_sync_gap(BOOL bChgFlag, UINT64 in_gap)
{
    gap = in_gap;
    bFlag = bChgFlag;

    SAL_WARN("set flag %d, sync gap %llu \n", bFlag, gap);
    return;
}

/**
 * @function:   debug_get_frame
 * @brief:      调试接口：获取帧数据
 * @param[in]:  UINT32 chan
 * @param[in]:  SYSTEM_FRAME_INFO *pFrm
 * @param[in]:  UINT64 *pts
 * @param[out]: None
 * @return:     static INT32
 */
INT32 debug_get_frame(UINT32 chan, SYSTEM_FRAME_INFO *pFrm, UINT64 *pts)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 time0 = 0;
    UINT32 time1 = 0;

    UINT32 uiChnTab[2] = {PPM_VDEC_CHN_TRI_CAM, PPM_VDEC_CHN_FACE_CAM};   /* 解码通道号表 */

#ifdef DEBUG_SYNC_DATA
    UINT64 tmp[3] = {50000, 40000, 16667};    /* 分别模拟三路视频的帧率，20fps、25fps、60fps */

    time0 = SAL_getCurMs();

    s32Ret = Ppm_DrvGetVdecFrm(PPM_VDEC_CHN_TRI_CAM, 3, pFrm, PPM_TRI_CAM_CHN_WIDTH, PPM_TRI_CAM_CHN_HEIGHT);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetVdecFrm failed!");

    time1 = SAL_getCurMs();

    *pts += tmp[chan];
#else

    time0 = SAL_getCurMs();

    if (chan < 2)
    {
        s32Ret = Ppm_DrvGetVdecFrm(uiChnTab[chan], 3, pFrm, PPM_TRI_CAM_CHN_WIDTH, PPM_TRI_CAM_CHN_HEIGHT);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetVdecFrm failed!");
    }
    else if (2 == chan)
    {
        /* szl_todo: 入参临时调整，待后续删除 */
        s32Ret = Ppm_DrvGetChnFrame(0, pFrm, PPM_FILL_PTS_OP, 0);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetVdecFrm failed!");
    }
    else
    {
        SAL_ERROR("invalid chan %d \n", chan);
        return SAL_FAIL;
    }

    time1 = SAL_getCurMs();
#endif

    PPM_LOGT("chan %d get frame end! pts %llu, cost %d. \n", chan, *pts, time1 - time0);
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   judge_need_clean
 * @brief:      判断是否存在无效阈值阻塞同步，若存在进行清空处理
 * @param[in]:  UINT64 pts
 * @param[in]:  UINT64 *domain
 * @param[out]: None
 * @return:     static BOOL
 */
static BOOL judge_need_clean(UINT64 pts, UINT64 *domain)
{
    BOOL bNeedClr = SAL_FALSE;

    if (pts < domain[0])
    {
        if (domain[0] - pts > 60000)
        {
            bNeedClr = SAL_TRUE;
        }
    }
    else if (pts > domain[0])
    {
        if (pts - domain[0] > 60000)
        {
            bNeedClr = SAL_TRUE;
        }
    }

    return bNeedClr;
}

/**
 * @function:   judge_dur_domain
 * @brief:      调试接口：判断是否在合理域之内
 * @param[in]:  UINT64 pts
 * @param[in]:  UINT64 *domain
 * @param[out]: None
 * @return:     static BOOL
 */
static BOOL judge_dur_domain(UINT64 pts, UINT64 *domain)
{
    BOOL flag = SAL_FALSE;

    if ((domain[0] == 0 || domain[1] == 0)
        || (pts <= domain[1] && pts >= domain[0]))
    {
        flag = SAL_TRUE;
    }

    return flag;
}

/******************************************************************
   Function:   get_arr
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static VOID get_arr(DBG_SYNC_PRM *pSync, UINT64 *pOutArr, UINT32 *pCnt)
{
    UINT32 i = 0;
    UINT32 cnt = 0;

    /* cal valid pts */
    for (i = 0; i < PPM_SYNC_FRM_NUM; i++)
    {
        if (!pSync[i].flag)
        {
            continue;
        }

        cnt++;
        pOutArr[i] = pSync[i].pts;
    }

    *pCnt = cnt;
    return;
}

/******************************************************************
   Function:   sort_arr
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static VOID sort_arr(UINT64 *pOutArr)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT64 tmp = 0;

    /* sort */
    for (i = 0; i < PPM_SYNC_FRM_NUM - 1; i++)
    {
        for (j = 0; j < PPM_SYNC_FRM_NUM - i - 1; j++)
        {
            if (pOutArr[j] > pOutArr[j + 1])
            {
                tmp = pOutArr[j];
                pOutArr[j] = pOutArr[j + 1];
                pOutArr[j + 1] = tmp;
            }
        }
    }

    return;
}

/**
 * @function:   update_domain
 * @brief:      更新限制域
 * @param[in]:  DBG_SYNC_PRM *pSync
 * @param[in]:  UINT64 *domain
 * @param[out]: None
 * @return:     static VOID
 */
static VOID update_domain(DBG_SYNC_PRM *pSync, UINT64 *domain)
{
    UINT32 cnt = 0;
    UINT64 tmp_arr[PPM_SYNC_FRM_NUM] = {0};

    (VOID)get_arr(pSync, tmp_arr, &cnt);

    (VOID)sort_arr(tmp_arr);

    /* cal domain */
    if (cnt > 0)
    {
        if (1 == cnt)
        {
            domain[0] = tmp_arr[PPM_SYNC_FRM_NUM - cnt] - gap;
            domain[1] = tmp_arr[PPM_SYNC_FRM_NUM - cnt] + gap;
        }
        else if (2 == cnt)
        {
            domain[0] = tmp_arr[PPM_SYNC_FRM_NUM - cnt] - gap;
            domain[1] = tmp_arr[PPM_SYNC_FRM_NUM - cnt + 1] + gap;

            if (domain[0] > domain[1])
            {
                SAL_ERROR("err domain: (%llu, %llu), %llu, %llu, %llu \n",
                          domain[0], domain[1], pSync[0].pts, pSync[1].pts, pSync[2].pts);
            }
        }
        else
        {
            PPM_LOGT("get three frame success! %d \n", cnt);
        }
    }
    else
    {
        memset(domain, 0x00, 2 * sizeof(UINT64));
        SAL_ERROR("cnt == 0!!!!!!!!!!!!!!!! \n");
    }

    PPM_LOGT("new domain: %llu, %llu \n", domain[0], domain[1]);
    return;
}

/******************************************************************
   Function:   get_max_min
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static VOID get_max_min(UINT64 *pArr, UINT64 *pMin, UINT32 *pMinId, UINT64 *pMax, UINT32 *pMaxId)
{
    UINT32 i = 0;
    UINT64 min = -1;
    UINT64 max = 0;
    UINT32 min_id = 0;
    UINT32 max_id = 0;

    for (i = 0; i < PPM_SYNC_FRM_NUM; i++)
    {
        if (0 == pArr[i])
        {
            continue;
        }

        if (pArr[i] < min)
        {
            min = pArr[i];
            min_id = i;
        }

        if (pArr[i] > max)
        {
            max = pArr[i];
            max_id = i;
        }
    }

    *pMin = min;
    *pMax = max;
    *pMinId = min_id;
    *pMaxId = max_id;

    return;
}

/******************************************************************
   Function:   debug_monitor
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static BOOL debug_monitor(UINT64 pts, UINT64 std)
{
    /* debug: 用来监控是否存在时间戳和阈值相差太大的情况 */
    UINT64 ullGapTmp = 0;
    BOOL bFlag = SAL_FALSE;

    if (0 != pts && 0 != std)
    {
        ullGapTmp = ((pts > std) ? (pts - std) : (std - pts));

        if (ullGapTmp > 1000)
        {
            SAL_ERROR("debug: timep and domain farrrrrrrrrrrrrrr! %llu, %llu \n", pts, std);

            bFlag = SAL_TRUE;
        }
    }

    return bFlag;
}

/******************************************************************
   Function:   rls_first_frm
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static VOID rls_first_frm(UINT32 idx, UINT64 pts, DBG_SYNC_PRM *pSync)
{
    UINT32 i = 0;
    UINT32 cnt = 0;
    UINT64 arr[PPM_SYNC_FRM_NUM] = {0};

    (VOID)get_arr(pSync, arr, &cnt);

    if (cnt > 0)
    {
        if (cnt == 1)
        {
            for (i = 0; i < PPM_SYNC_FRM_NUM; i++)
            {
                if (0 == arr[i])
                {
                    continue;
                }

                break;
            }

            if (i >= PPM_SYNC_FRM_NUM)
            {
                SAL_ERROR("errrrrrrr! \n");
                return;
            }

            if (pts < pSync[i].pts)
            {
                PPM_LOGT("idx %d unused! \n", idx);
            }
            else
            {
                /* 删除旧的pts */
                pSync[i].flag = SAL_FALSE;
                pSync[i].pts = 0;

                /* 使能新的pts，并在后续更新domain */
                pSync[idx].flag = SAL_TRUE;
                pSync[idx].pts = pts;
                PPM_LOGT("idx %d unused! \n", i);
            }
        }
        else if (cnt <= 3)
        {
            UINT64 min = 0;
            UINT64 max = 0;
            UINT32 min_id = 0;
            UINT32 max_id = 0;

            (VOID)get_max_min(arr, &min, &min_id, &max, &max_id);

            if (pts < min)
            {
                if (SAL_TRUE == debug_monitor(pts, min))   /* debug */
                {
                    SAL_ERROR("%s: farrrrrrrr! %d \n", __func__, min_id);
                }

                if (SAL_TRUE == debug_monitor(pts, max))   /* debug */
                {
                    SAL_ERROR("%s: farrrrrrrr! %d \n", __func__, max_id);
                }

                pSync[idx].flag = SAL_FALSE;
                pSync[idx].pts = 0;
                PPM_LOGT("idx %d unused! \n", idx);
            }
            else if (pts > max)
            {
                if (SAL_TRUE == debug_monitor(pts, pSync[min_id].pts))   /* debug */
                {
                    SAL_ERROR("%s: farrrrrrrr! %d \n", __func__, min_id);
                }

                /* 当新获取的帧时间戳大于已有最大值时，考虑到本接口的前置条件是当前时间戳在有效区间之外。
                   故删除已有的所有时间戳且仅保留当前这一帧的时间戳pts */
                pSync[min_id].flag = SAL_FALSE;
                pSync[min_id].pts = 0;

                pSync[max_id].flag = SAL_FALSE;
                pSync[max_id].pts = 0;

                pSync[idx].flag = SAL_TRUE;
                pSync[idx].pts = pts;

                PPM_LOGT("idx %d unused! \n", min_id);
            }
        }
        else
        {
            SAL_ERROR("invalid cnt! %d \n", cnt);
            return;
        }
    }

    PPM_LOGT("__rls idx %d, pts %llu \n", idx, pts);
    return;
}

/******************************************************************
   Function:   get_sleep_ms
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static VOID get_sleep_ms(UINT32 time0, UINT32 time1, UINT32 *pSleepMs)
{
    UINT32 ms = 0;

    if (time1 < time0 + 50)
    {
        ms = 50 + time0 - time1;
    }

    *pSleepMs = ms;
    return;
}

/**
 * @function:   Ppm_DrvGetPts
 * @brief:      获取帧数据结构中的时间戳
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrm
 * @param[in]:  UINT64 *pu64Pts
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Ppm_DrvGetPts(SYSTEM_FRAME_INFO *pstFrm, UINT64 *pu64Pts)
{
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    PPM_DRV_CHECK_PTR(pstFrm, exit, "pstFrm == null!");
    PPM_DRV_CHECK_PTR(pu64Pts, exit, "pu64Pts == null!");

    (VOID)sys_hal_getVideoFrameInfo(pstFrm, &stVideoFrmBuf);

    *pu64Pts = stVideoFrmBuf.pts;

exit:
    return;
}

/* 同步使用的结构体 */
UINT64 domain[2] = {0};                                  /* 每次获取帧数据前的限制域，基于pts。需及时更新 */
UINT64 tmp_pts[PPM_SYNC_FRM_NUM] = {0, 0, 0};            /* 时间戳，中间变量 */
DBG_SYNC_PRM stDbgArr[PPM_SYNC_FRM_NUM] = {0};           /* 同步全局参数 */

/*UINT64 auiDbgErrCnt[PPM_SYNC_FRM_NUM][2][128] = {0};*/     /* 调试信息，统计出错索引值对应的时间戳 */
UINT32 auiDbgErr[PPM_SYNC_FRM_NUM] = {0};                /* 调试信息，统计出错的索引值 */

/******************************************************************
   Function:   clear_tmp_result
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static VOID clear_tmp_result(VOID)
{
    memset(domain, 0x00, 2 * sizeof(UINT64));    /* 清空限制域的值，便于下一次获取匹配帧 */
    memset(stDbgArr, 0x00, PPM_SYNC_FRM_NUM * sizeof(DBG_SYNC_PRM));
    memset(tmp_pts, 0x00, PPM_SYNC_FRM_NUM * sizeof(UINT64));
    /*memset(auiDbgErrCnt, 0x00, PPM_SYNC_FRM_NUM * 2 * 128 * sizeof(UINT64));*/
    memset(auiDbgErr, 0x00, PPM_SYNC_FRM_NUM * sizeof(UINT32));

    return;
}

#ifdef PPM_SYNC_V0

/**
 * @function:   Ppm_DrvSyncThread
 * @brief:      多源数据同步线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvSyncThread_V0(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT64 time000 = 0;
    UINT64 time111 = 0;
    UINT32 fps = 0;
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;

    UINT32 time_start = 0;
    UINT32 time_end = 0;

    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 sleep_ms = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    PPM_DEV_PRM *pstDevPrm = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;
    PPM_QUEUE_PRM *pstQuePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    PPM_FRAME_INFO_ST *pstPpmFrmInfo = NULL;

    pstDevPrm = (PPM_DEV_PRM *)prm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQuePrm = pstDevPrm->pstYuvQueInfo->pstYuvQuePrm;
    pstFullQue = pstQuePrm->pstFullQue;     /* 满队列 */
    pstEmptQue = pstQuePrm->pstEmptQue;     /* 空队列 */

    pstSysFrameInfo = &pstDevPrm->stVdecWholeFrame;
    if (0x00 == pstSysFrameInfo->uiAppData)
    {
        s32Ret = Ppm_DrvGetFrameMem(2560, 1440, &pstDevPrm->stVdecWholeFrame);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");

        SAL_WARN("get whole frame buf end! w %d, h %d \n", 2560, 1440);
    }

    SAL_SET_THR_NAME();

    while (SAL_TRUE)
    {
        if (!pstDevPrm->bChnOpen)
        {
            usleep(200 * 1000);
            continue;
        }

        /* get free queue buf */
        (VOID)Ppm_DrvGetQueData(pstEmptQue, (VOID **)&pstPpmFrmInfo, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstPpmFrmInfo, reuse, "get queue data failed! pstPpmFrmInfo == null!");

        time_start = SAL_getCurMs();

frm_1:
        /* 调试配置接口，用于调整同步间隔，默认100ms */
        if (SAL_TRUE == bFlag)
        {
            clear_tmp_result();
            bFlag = SAL_FALSE;
        }

        /* 帧率统计 */
        (VOID)Ppm_DrvCalThrFps(&time000, &time111, &fps, __func__);

        if (!stDbgArr[0].flag)
        {
            /* Get Frame */
            s32Ret = debug_get_frame(0, &pstPpmFrmInfo->stTriCamFrame, &tmp_pts[0]);
            PPM_DRV_CHECK_RET(s32Ret, frm_1, "get frm 1 failed!");

            if (0)
            {
                /* 比对当前帧是否在限制域中 */
                if (SAL_TRUE != judge_dur_domain(tmp_pts[0], domain))
                {
                    rls_first_frm(0, tmp_pts[0], stDbgArr);
                    update_domain(stDbgArr, domain);

                    auiDbgErrCnt[0][0][auiDbgErr[0]] = (SAL_getCurMs()) - time_start;
                    auiDbgErrCnt[0][1][auiDbgErr[0]++] = tmp_pts[0];
                    goto frm_1;
                }
            }

            stDbgArr[0].flag = SAL_TRUE;
            stDbgArr[0].pts = tmp_pts[0];

            PPM_LOGT("0 in valid domain! (%llu, %llu) %llu \n", domain[0], domain[1], tmp_pts[0]);
            time1 = SAL_getCurMs();

            /* 更新限制域 */
            update_domain(stDbgArr, domain);
        }

        if (!stDbgArr[1].flag)
        {
            /* Get Frame */
            s32Ret = debug_get_frame(1, &pstPpmFrmInfo->stFaceFrame, &tmp_pts[1]);
            PPM_DRV_CHECK_RET(s32Ret, frm_1, "get frm 2 failed!");

            if (0)
            {
                /* 比对当前帧是否在限制域中 */
                if (SAL_TRUE != judge_dur_domain(tmp_pts[1], domain))
                {
                    rls_first_frm(1, tmp_pts[1], stDbgArr);
                    update_domain(stDbgArr, domain);

                    auiDbgErrCnt[1][0][auiDbgErr[1]] = (SAL_getCurMs()) - time_start;
                    auiDbgErrCnt[1][1][auiDbgErr[1]++] = tmp_pts[1];
                    goto frm_1;
                }
            }

            PPM_LOGT("1 in valid domain! (%llu, %llu) %llu \n", domain[0], domain[1], tmp_pts[1]);
            time2 = SAL_getCurMs();

            stDbgArr[1].flag = SAL_TRUE;
            stDbgArr[1].pts = tmp_pts[1];

            /* 更新限制域 */
            update_domain(stDbgArr, domain);
        }

        if (!stDbgArr[2].flag)
        {
            /* Get Frame */
            s32Ret = debug_get_frame(2, &g_stPpmCommPrm.pstPpmDevPrm[0]->stTmpFrame[1], &tmp_pts[2]);
            PPM_DRV_CHECK_RET(s32Ret, frm_1, "get frm 3 failed!");

            if (0)
            {
                /* 比对当前帧是否在限制域中 */
                if (SAL_TRUE != judge_dur_domain(tmp_pts[2], domain))
                {
                    rls_first_frm(2, tmp_pts[2], stDbgArr);
                    update_domain(stDbgArr, domain);

                    auiDbgErrCnt[2][0][auiDbgErr[2]] = (SAL_getCurMs()) - time_start;
                    auiDbgErrCnt[2][1][auiDbgErr[2]++] = tmp_pts[2];

                    if (SAL_TRUE != stDbgArr[2].flag)
                    {
                        (VOID)Ppm_DrvPutChnFrame(0, &g_stPpmCommPrm.pstPpmDevPrm[0]->stTmpFrame[1]);
                    }

                    goto frm_1;
                }
            }

            stDbgArr[2].flag = SAL_TRUE;
            stDbgArr[2].pts = tmp_pts[2];

            PPM_LOGT("2 in valid domain! (%llu, %llu) %llu \n", domain[0], domain[1], tmp_pts[2]);
        }

        (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.pstPpmDevPrm[0]->stTmpFrame[1], &stVideoFrmBuf);

        if (SAL_SOK != Ia_TdeQuickCopy(&g_stPpmCommPrm.pstPpmDevPrm[0]->stTmpFrame[1],
                                       &pstPpmFrmInfo->stXpkgFrame,
                                       0, 0, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, SAL_FALSE))
        {
            SAL_ERROR("PPM: Tde Copy fail! w %d, h %d \n", stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height);
            goto reuse;
        }

        time3 = SAL_getCurMs();

        PPM_LOGI("get sync frm end! pts: %llu, %llu, %llu, time0 %d, time1 %d, time2 %d \n",
                 stDbgArr[0].pts, stDbgArr[1].pts, stDbgArr[2].pts, time1 - time_start, time2 - time_start, time3 - time_start);

#if 1   /* debug: 统计同步时各个部分调用的次数和对应的时间戳 */
        for (i = 0; i < PPM_SYNC_FRM_NUM; i++)
        {
            PPM_LOGD("i %d \n", i);
            for (j = 0; j < auiDbgErr[i] / 16 + 1; j++)
            {
                k = j * 16;
                PPM_LOGD("%d, %d: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu \n", j, k,
                         auiDbgErrCnt[i][0][k], auiDbgErrCnt[i][0][k + 1], auiDbgErrCnt[i][0][k + 2], auiDbgErrCnt[i][0][k + 3], auiDbgErrCnt[i][0][k + 4], auiDbgErrCnt[i][0][k + 5], auiDbgErrCnt[i][0][k + 6], auiDbgErrCnt[i][0][k + 7],
                         auiDbgErrCnt[i][0][k + 8], auiDbgErrCnt[i][0][k + 9], auiDbgErrCnt[i][0][k + 10], auiDbgErrCnt[i][0][k + 11], auiDbgErrCnt[i][0][k + 12], auiDbgErrCnt[i][0][k + 13], auiDbgErrCnt[i][0][k + 14], auiDbgErrCnt[i][0][k + 15]);

                PPM_LOGD("%d, %d: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu \n", j, k,
                         auiDbgErrCnt[i][1][k], auiDbgErrCnt[i][1][k + 1], auiDbgErrCnt[i][1][k + 2], auiDbgErrCnt[i][1][k + 3], auiDbgErrCnt[i][1][k + 4], auiDbgErrCnt[i][1][k + 5], auiDbgErrCnt[i][1][k + 6], auiDbgErrCnt[i][1][k + 7],
                         auiDbgErrCnt[i][1][k + 8], auiDbgErrCnt[i][1][k + 9], auiDbgErrCnt[i][1][k + 10], auiDbgErrCnt[i][1][k + 11], auiDbgErrCnt[i][1][k + 12], auiDbgErrCnt[i][1][k + 13], auiDbgErrCnt[i][1][k + 14], auiDbgErrCnt[i][1][k + 15]);
            }
        }

#endif

        /* 送入推帧线程的yuv数据队列中 */
        (VOID)Ppm_DrvPutQueData(pstFullQue, (VOID *)pstPpmFrmInfo, SAL_TIMEOUT_NONE);
        pstPpmFrmInfo = NULL;

        fps++;

        /* clear result */
        clear_tmp_result();

        time_end = SAL_getCurMs();

        /* 增加帧率控制 */
        get_sleep_ms(time_start, time_end, &sleep_ms);
        if (0 != sleep_ms)
        {
            usleep(sleep_ms * 1000);
        }

        (VOID)Ppm_DrvPutChnFrame(0, &g_stPpmCommPrm.pstPpmDevPrm[0]->stTmpFrame[1]);

        PPM_LOGT("------------------proc time(%d), sleep(%d) \n", time_end - time_start, sleep_ms);
        continue;

reuse:
        /* recycle queue buf */
        if (pstPpmFrmInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pstPpmFrmInfo, SAL_TIMEOUT_NONE);
        }

        pstPpmFrmInfo = NULL;
    }

err:
    return NULL;

}

#else

/**
 * @function:   Ppm_DrvClrQueData
 * @brief:      清空队列数据
 * @param[in]:  PPM_QUEUE_PRM *pstQuePrm
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Ppm_DrvClrQueData(PPM_QUEUE_PRM *pstQuePrm)
{
    VOID *pData = NULL;

    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    PPM_DRV_CHECK_PTR(pstQuePrm, exit, "pstQuePrm == null!");

    pstFullQue = pstQuePrm->pstFullQue;
    pstEmptQue = pstQuePrm->pstEmptQue;

    while (DSA_QueGetQueuedCount(pstFullQue))
    {
        (VOID)Ppm_DrvGetQueData(pstFullQue, (VOID **)&pData, SAL_TIMEOUT_NONE);

        if (pData)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pData, SAL_TIMEOUT_NONE);
            pData = NULL;
        }
    }

exit:
    SAL_WARN("clear queue data end! \n");
    return;
}

/**
 * @function:   Ppm_DrvGetFreeBuf
 * @brief:      获取空闲buf
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 *puiIdx
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_DrvGetFreeBuf(UINT32 chan, UINT32 *puiIdx)
{
    UINT32 i = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_BUF_PRM_S *pstBufPrm = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstBufPrm = &pstDevPrm->stXsiSecBufPrm;

    for (i = pstBufPrm->uiIdx; i < pstBufPrm->uiIdx + pstBufPrm->uiBufCnt; i++)
    {
        if (pstBufPrm->stXsiSecChnYuvBuf[i % pstBufPrm->uiBufCnt].bUsed)
        {
            continue;
        }

        pstBufPrm->stXsiSecChnYuvBuf[i % pstBufPrm->uiBufCnt].bUsed = SAL_TRUE;  /* 使用中 */
        break;
    }

    if (i >= pstBufPrm->uiIdx + pstBufPrm->uiBufCnt)
    {
        SAL_ERROR("get free buf failed! chan %d \n", chan);
        return SAL_FAIL;
    }

/*    SAL_WARN("get free idx %d, %p \n", i % pstBufPrm->uiBufCnt, &pstBufPrm->stXsiSecChnYuvBuf[i % pstBufPrm->uiBufCnt]); */
    pstBufPrm->uiIdx = (pstBufPrm->uiIdx + 1) % pstBufPrm->uiBufCnt;    /* 缓存索引递增 */
    *puiIdx = i % pstBufPrm->uiBufCnt;

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvGetTriFrameThread
 * @brief:      三目相机取帧线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvGetTriFrameThread(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT64 time000 = 0;
    UINT64 time111 = 0;
    UINT32 fps = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    UINT32 time5 = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    PPM_QUEUE_PRM *pstQuePrm = NULL;
    PPM_COM_FRAME_INFO_ST *pstComFrmInfo = NULL;

    pstDevPrm = (PPM_DEV_PRM *)prm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQuePrm = &pstDevPrm->stTriYuvQueInfo.stYuvQuePrm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstQuePrm == null!");

    pstFullQue = pstQuePrm->pstFullQue;     /* 满队列 */
    pstEmptQue = pstQuePrm->pstEmptQue;     /* 空队列 */

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind(0);

    while (SAL_TRUE)
    {
        time0 = SAL_getCurMs();
        if (!pstDevPrm->bChnOpen)
        {
            if (pstDevPrm->bTriFrmClr)
            {
                Ppm_DrvClrQueData(pstQuePrm);
                pstDevPrm->bTriFrmClr = SAL_FALSE;

                SAL_WARN("tri queue clear end! \n");
            }

            usleep(200 * 1000);
            continue;
        }

        time1 = SAL_getCurMs();

        (VOID)Ppm_DrvCalThrFps(&time000, &time111, &fps, __func__);

        time2 = SAL_getCurMs();

        (VOID)Ppm_DrvGetQueData(pstEmptQue, (VOID **)&pstComFrmInfo, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstComFrmInfo, reuse, "pstComFrmInfo == null!");

        time3 = SAL_getCurMs();

        if (time3 - time2 > 60)
        {
            SAL_ERROR("get tri empty queue cost %d > 60ms! \n", time3 - time2);
        }

        /* Get Frame */
        s32Ret = Ppm_DrvGetVdecFrm(PPM_VDEC_CHN_TRI_CAM, 3, &pstComFrmInfo->stFrame[0], PPM_TRI_CAM_CHN_WIDTH, PPM_TRI_CAM_CHN_HEIGHT);
        PPM_DRV_CHECK_RET(s32Ret, reuse, "tri: Ppm_DrvGetVdecFrm failed!");

        time4 = SAL_getCurMs();

#if 0
        SAL_VideoFrameBuf stVideoFrmBuf = {0};
        (VOID)sys_hal_getVideoFrameInfo(&pstComFrmInfo->stFrame[0], &stVideoFrmBuf);
        (VOID)Ppm_DrvDumpNv21((VOID *)stVideoFrmBuf.virAddr[0],
                              stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height * 3 / 2, 4);
#endif

        (VOID)Ppm_DrvPutQueData(pstFullQue, (VOID *)pstComFrmInfo, SAL_TIMEOUT_NONE);
        pstComFrmInfo = NULL;

        time5 = SAL_getCurMs();

        if (time5 - time0 > 50)
        {
            PPM_LOGD("tri---timeeeeeeee: %d, %d, %d, %d, %d \n", time1 - time0, time2 - time1, time3 - time2, time4 - time3, time5 - time4);
        }

        fps++;

        continue;

reuse:
        if (pstComFrmInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pstComFrmInfo, SAL_TIMEOUT_NONE);
            pstComFrmInfo = NULL;
        }
    }

err:
    return NULL;
}

/**
 * @function:   Ppm_DrvGetFaceFrameThread
 * @brief:      人脸相机取帧线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvGetFaceFrameThread(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT64 time000 = 0;
    UINT64 time111 = 0;
    UINT32 fps = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    UINT32 time5 = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    PPM_QUEUE_PRM *pstQuePrm = NULL;
    PPM_COM_FRAME_INFO_ST *pstComFrmInfo = NULL;

    pstDevPrm = (PPM_DEV_PRM *)prm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQuePrm = &pstDevPrm->stFaceYuvQueInfo.stYuvQuePrm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstQuePrm == null!");

    pstFullQue = pstQuePrm->pstFullQue;     /* 满队列 */
    pstEmptQue = pstQuePrm->pstEmptQue;     /* 空队列 */

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind(0);

    while (SAL_TRUE)
    {
        time0 = SAL_getCurMs();

        if (!pstDevPrm->bChnOpen)
        {
            if (pstDevPrm->bFaceFrmClr)
            {
                Ppm_DrvClrQueData(pstQuePrm);
                pstDevPrm->bFaceFrmClr = SAL_FALSE;

                SAL_WARN("face queue clear end! \n");
            }

            usleep(200 * 1000);
            continue;
        }

        time1 = SAL_getCurMs();


        (VOID)Ppm_DrvCalThrFps(&time000, &time111, &fps, __func__);
        time2 = SAL_getCurMs();


        (VOID)Ppm_DrvGetQueData(pstEmptQue, (VOID **)&pstComFrmInfo, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstComFrmInfo, reuse, "pstComFrmInfo == null!");
        time3 = SAL_getCurMs();

        if (time3 - time2 > 60)
        {
            SAL_ERROR("get face empty queue cost %d > 60ms! \n", time3 - time2);
        }

        /* Get Frame */
        s32Ret = Ppm_DrvGetVdecFrm(PPM_VDEC_CHN_FACE_CAM, 3, &pstComFrmInfo->stFrame[0], PPM_FACE_WIDTH, PPM_FACE_HEIGHT);
        PPM_DRV_CHECK_RET(s32Ret, reuse, "face: Ppm_DrvGetVdecFrm failed!");
        time4 = SAL_getCurMs();

#if 0
        VIDEO_FRAME_INFO_S *pTmp = NULL;

        pTmp = (VIDEO_FRAME_INFO_S *)pstComFrmInfo->stFrame[0].uiAppData;
        SAL_WARN("face: get pts %llu \n", pTmp->stVFrame.u64PTS);
#endif
#if 0

        SAL_VideoFrameBuf stVideoFrmBuf = {0};
        (VOID)sys_hal_getVideoFrameInfo(&pstComFrmInfo->stFrame[0], &stVideoFrmBuf);
        (VOID)Ppm_DrvDumpNv21((VOID *)stVideoFrmBuf.virAddr[0],
                              stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height * 3 / 2, 3);
#endif

        (VOID)Ppm_DrvPutQueData(pstFullQue, (VOID *)pstComFrmInfo, SAL_TIMEOUT_NONE);
        pstComFrmInfo = NULL;
        time5 = SAL_getCurMs();

        fps++;

        if (time5 - time0 > 50)
        {
            PPM_LOGD("face---timeeeeeeee: %d, %d, %d, %d, %d \n", time1 - time0, time2 - time1, time3 - time2, time4 - time3, time5 - time4);
        }

        continue;

reuse:
        if (pstComFrmInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pstComFrmInfo, SAL_TIMEOUT_NONE);
            pstComFrmInfo = NULL;
        }
    }

err:
    return NULL;
}

/**
 * @function:   Ppm_DrvJudgeNeedSync
 * @brief:      判断当前队列状态是否需要同步
 * @param[in]:  DSA_QueHndl *pstTriFullQue
 * @param[in]:  DSA_QueHndl *pstFaceFullQue
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_DrvJudgeNeedSync(DSA_QueHndl *pstTriFullQue, DSA_QueHndl *pstFaceFullQue)
{
    BOOL bNeedSync = SAL_FALSE;
    UINT32 uiTriFullQueCnt = 0;
    UINT32 uiFaceFullQueCnt = 0;

    uiTriFullQueCnt = DSA_QueGetQueuedCount(pstTriFullQue);
    uiFaceFullQueCnt = DSA_QueGetQueuedCount(pstFaceFullQue);

    if (uiTriFullQueCnt > 0 && uiFaceFullQueCnt > 0)
    {
        bNeedSync = SAL_TRUE;
    }

    return bNeedSync;
}

/* static VOID *pData[2] = {NULL}; */
static UINT32 uiSyncCnt = 10;

/**
 * @function    Ppm_DrvSetSyncGapCnt
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Ppm_DrvSetSyncGapCnt(UINT32 cnt)
{
    uiSyncCnt = cnt;
    return;
}

/**
 * @function    Ppm_DrvDumpSyncDataThread
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID *Ppm_DrvDumpSyncDataThread(VOID *prm)
{
    UINT32 i = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SAL_SET_THR_NAME();

    for (i = 0; i < 2; i++)
    {
        if (NULL == pstComFrm[i])
        {
            continue;
        }

        (VOID)sys_hal_getVideoFrameInfo(&pstComFrm[i]->stFrame[0], &stVideoFrmBuf);
        (VOID)Ppm_DrvDumpNv21((VOID *)stVideoFrmBuf.virAddr[0],
                              stVideoFrmBuf.stride[0] * stVideoFrmBuf.frameParam.height * 3 / 2,
                              i);
    }

    SAL_thrExit(NULL);
    return NULL;
}

/* 第一次同步上之后深度信息队列为空时，超时等待的次数 */
#define PPM_DRV_AFTER_FIRST_SYNC_WAIT_DEPTH_NUM (3)

/* bit proc flag, low to high, 0x1: print raw cjson, 0x2: enable depth sync, 0x4: enable pos dump, 0x8: enable depth print */
static UINT32 u32PrMetaJson = 2;

/* 深度信息同步时间间隔，默认300ms */
static UINT32 u32DepthSyncGap = 50;

static BOOL bDepthFirstSyncFlag = SAL_FALSE;
static BOOL bFFFlag = SAL_FALSE;
//static UINT32 u32DepthSyncWaitCnt = 0;

VOID Ppm_DrvSetDepthSyncGap(UINT32 gap)
{
	u32DepthSyncGap = gap;
	PPM_LOGI("set depth sync gap %d \n", u32DepthSyncGap);
}

VOID Ppm_DrvSetMetaCtlFlag(UINT32 flag)
{
	u32PrMetaJson = flag;
}

/**
 * @function:   Ppm_DrvSyncThread
 * @brief:      多源数据同步线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvSyncThread(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT64 time000 = 0;
    UINT64 time111 = 0;
    UINT32 fps = 0;

    /*UINT32 LastNum[2] = {0};*/
	INT64L s64Tmp[4] = {0};   /*0:三目时戳, 1:人脸时戳, 2:三目深度信息时戳, 3:三目深度信息时戳比三目时戳大的阈值,4:三目深度信息时戳比三目时戳小的阈值, 用于保存匹配上的三目、人脸相机帧时间戳 */
    UINT32 uiSyncDataCnt = 0;
    UINT32 bClrFlag = SAL_TRUE;

#if 1
    UINT32 time_start = 0;
    UINT32 time_end = 0;
    UINT32 time_last = 0;
    UINT32 sleep_ms = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time3_1 = 0;
    UINT32 time4 = 0;
    UINT32 time4_1 = 0;
    UINT32 time5 = 0;
    UINT32 time5_0 = 0;
    UINT32 time5_1 = 0;
    UINT32 time6 = 0;
#endif

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_COM_FRAME_INFO_ST *pstComFrmInfo[2] = {NULL, NULL};   /* 三路源数据，分别代表三目相机、人脸相机 */
    PPM_FRAME_INFO_ST *pstPpmFrmInfo = NULL;

    /* 三目相机队列参数 */
    PPM_QUEUE_PRM *pstTriQuePrm = NULL;
    DSA_QueHndl *pstTriFullQue = NULL;
    DSA_QueHndl *pstTriEmptQue = NULL;

    /* 人脸相机队列参数 */
    PPM_QUEUE_PRM *pstFaceQuePrm = NULL;
    DSA_QueHndl *pstFaceFullQue = NULL;
    DSA_QueHndl *pstFaceEmptQue = NULL;

#if 0
    /* X光包裹队列参数 */
    PPM_QUEUE_PRM *pstXsiQuePrm = NULL;
    DSA_QueHndl *pstXsiFullQue = NULL;
    DSA_QueHndl *pstXsiEmptQue = NULL;
#endif

    /* 同步结果队列参数 */
    PPM_QUEUE_PRM *pstYuvQuePrm = NULL;
    DSA_QueHndl *pstYuvFullQue = NULL;
    DSA_QueHndl *pstYuvEmptQue = NULL;

	/* 深度信息队列参数 */
	BOOL bMatch = SAL_FALSE;
    PPM_QUEUE_PRM *pstDepthInfoQuePrm = NULL;
    DSA_QueHndl *pstDepthFullQue = NULL;
    DSA_QueHndl *pstDepthEmptQue = NULL;
	PPM_DEPTH_INFO_S *pstDepthInfo = NULL;
	
	PPM_DEPTH_INFO_S stMatchDepthInfo = {0};

    pstDevPrm = (PPM_DEV_PRM *)prm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    /* 三目相机队列参数 */
    pstTriQuePrm = &pstDevPrm->stTriYuvQueInfo.stYuvQuePrm;
    PPM_DRV_CHECK_PTR(pstTriQuePrm, err, "pstTriQuePrm == null!");

    pstTriFullQue = pstTriQuePrm->pstFullQue;     /* 满队列 */
    pstTriEmptQue = pstTriQuePrm->pstEmptQue;     /* 空队列 */

    /* 人脸相机队列参数 */
    pstFaceQuePrm = &pstDevPrm->stFaceYuvQueInfo.stYuvQuePrm;
    PPM_DRV_CHECK_PTR(pstFaceQuePrm, err, "pstFaceQuePrm == null!");

    pstFaceFullQue = pstFaceQuePrm->pstFullQue;     /* 满队列 */
    pstFaceEmptQue = pstFaceQuePrm->pstEmptQue;     /* 空队列 */

#if 0
    /* X光包裹队列参数 */
    pstXsiQuePrm = &pstDevPrm->stXsiYuvQueInfo.stYuvQuePrm;
    PPM_DRV_CHECK_PTR(pstFaceQuePrm, err, "pstXsiQuePrm == null!");

    pstXsiFullQue = pstXsiQuePrm->pstFullQue;   /* 满队列 */
    pstXsiEmptQue = pstXsiQuePrm->pstEmptQue;   /* 空队列 */
#endif

    /* 结果数据队列参数 */
    pstYuvQuePrm = &pstDevPrm->stYuvQueInfo.stYuvQuePrm;
    PPM_DRV_CHECK_PTR(pstYuvQuePrm, err, "pstYuvQuePrm == null!");

    pstYuvFullQue = pstYuvQuePrm->pstFullQue;     /* 满队列 */
    pstYuvEmptQue = pstYuvQuePrm->pstEmptQue;     /* 空队列 */

	pstDepthInfoQuePrm = &pstDevPrm->stDepthInfoQueInfo.stDepthQuePrm;
	pstDepthFullQue = pstDepthInfoQuePrm->pstFullQue;
	pstDepthEmptQue = pstDepthInfoQuePrm->pstEmptQue;

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind(0);

    while (SAL_TRUE)
    {
#if 0
        if (SAL_TRUE != Ppm_DrvJudgeNeedSync(pstTriFullQue, pstFaceFullQue))
        {
            usleep(200 * 1000);

            SAL_WARN("%s: need sync! \n", __func__);
            continue;
        }

#endif

        if (!pstDevPrm->bChnOpen)
        {
            usleep(100 * 1000);

            if (bClrFlag)
            {
                pstDevPrm->bTriFrmClr = SAL_TRUE;
                pstDevPrm->bFaceFrmClr = SAL_TRUE;
                pstDevPrm->bXsiFrmClr = SAL_TRUE;

                bClrFlag = SAL_FALSE;

                SAL_WARN("sync thr proc clear end! \n");
            }

            continue;
        }

        bClrFlag = SAL_TRUE;

        time0 = SAL_getCurMs();

        /* 帧率统计 */
        (VOID)Ppm_DrvCalThrFps(&time000, &time111, &fps, __func__);

        time1 = SAL_getCurMs();

        /* get free queue buf */
        (VOID)Ppm_DrvGetQueData(pstYuvEmptQue, (VOID **)&pstPpmFrmInfo, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstPpmFrmInfo, reuse_1, "get queue data failed! pstPpmFrmInfo == null!");

        time_start = SAL_getCurMs();

        /* 帧率控制 */
        get_sleep_ms(time_last, time_start, &sleep_ms);
        time_last = time_start;

		/* 避免线程占用率过高，增加休眠让出cpu资源，维持在20fps */
		if (0 != sleep_ms)
		{
			usleep(sleep_ms * 1000);
			PPM_LOGD("==========sleep %d ms \n", sleep_ms);
		}

sync:
        if (!stDbgArr[0].flag)
        {
            if (!pstDevPrm->bChnOpen)
            {
                goto clean;
            }

            (VOID)Ppm_DrvGetQueData(pstTriFullQue, (VOID **)&pstComFrmInfo[0], SAL_TIMEOUT_FOREVER);
            PPM_DRV_CHECK_PTR(pstComFrmInfo[0], reuse_2, "get tri que data failed! pstComFrmInfo[0] == null!");

			(VOID)Ppm_DrvGetPts(&pstComFrmInfo[0]->stFrame[0], &tmp_pts[0]);

            if (!pstDevPrm->bNeedSync)
            {
                if (SAL_TRUE != judge_dur_domain(tmp_pts[0], domain))
                {
                    if (judge_need_clean(tmp_pts[0], domain))
                    {
                        SAL_ERROR("0: pts %llu, domain[%llu, %llu] \n", tmp_pts[0], domain[0], domain[1]);
                        goto clean;
                    }

                    rls_first_frm(0, tmp_pts[0], stDbgArr);
                    update_domain(stDbgArr, domain);

                    if (!stDbgArr[0].flag && pstComFrmInfo[0])
                    {
                        (VOID)Ppm_DrvPutQueData(pstTriEmptQue, (VOID *)pstComFrmInfo[0], SAL_TIMEOUT_NONE);
                        pstComFrmInfo[0] = NULL;
                    }

                    if (!stDbgArr[1].flag && pstComFrmInfo[1])
                    {
                        (VOID)Ppm_DrvPutQueData(pstFaceEmptQue, (VOID *)pstComFrmInfo[1], SAL_TIMEOUT_NONE);
                        pstComFrmInfo[1] = NULL;
                    }

#if 0
                    auiDbgErrCnt[0][0][auiDbgErr[0]] = (SAL_getCurMs()) - time_start;
                    auiDbgErrCnt[0][1][auiDbgErr[0]++] = tmp_pts[0];

                    if (LastNum[0] < auiDbgErr[0])
                    {
                        SAL_ERROR("dbg:0 cur %d, Lastnum %d, errcnt[%llu, %llu] \n",
                                  auiDbgErr[0], LastNum[0], auiDbgErrCnt[0][0][LastNum[0]], auiDbgErrCnt[0][1][LastNum[0]]);
                    }

                    LastNum[0] = auiDbgErr[0];
#endif
                    goto sync;
                }

                stDbgArr[0].flag = SAL_TRUE;
                stDbgArr[0].pts = tmp_pts[0];

                PPM_LOGD("0 get valid result! \n");
                update_domain(stDbgArr, domain);
            }
        }

        time2 = SAL_getCurMs();

        if (!stDbgArr[1].flag)
        {
            if (!pstDevPrm->bChnOpen)
            {
                goto clean;
            }

            (VOID)Ppm_DrvGetQueData(pstFaceFullQue, (VOID **)&pstComFrmInfo[1], SAL_TIMEOUT_FOREVER);
            PPM_DRV_CHECK_PTR(pstComFrmInfo[1], reuse_3, "get face que data failed! pstComFrmInfo[1] == null!");

			(VOID)Ppm_DrvGetPts(&pstComFrmInfo[1]->stFrame[0], &tmp_pts[1]);

            if (!pstDevPrm->bNeedSync)
            {
                if (SAL_TRUE != judge_dur_domain(tmp_pts[1], domain))
                {
                    if (judge_need_clean(tmp_pts[1], domain))
                    {
                        SAL_ERROR("1: pts %llu, domain[%llu, %llu] \n", tmp_pts[1], domain[0], domain[1]);
                        goto clean;
                    }

                    rls_first_frm(1, tmp_pts[1], stDbgArr);
                    update_domain(stDbgArr, domain);

                    if (!stDbgArr[0].flag && pstComFrmInfo[0])
                    {
                        (VOID)Ppm_DrvPutQueData(pstTriEmptQue, (VOID *)pstComFrmInfo[0], SAL_TIMEOUT_NONE);
                        pstComFrmInfo[0] = NULL;
                    }

                    if (!stDbgArr[1].flag && pstComFrmInfo[1])
                    {
                        (VOID)Ppm_DrvPutQueData(pstFaceEmptQue, (VOID *)pstComFrmInfo[1], SAL_TIMEOUT_NONE);
                        pstComFrmInfo[1] = NULL;
                    }

#if 0
                    auiDbgErrCnt[1][0][auiDbgErr[1]] = (SAL_getCurMs()) - time_start;
                    auiDbgErrCnt[1][1][auiDbgErr[1]++] = tmp_pts[1];
                    if (LastNum[1] < auiDbgErr[1])
                    {
                        SAL_ERROR("dbg:1 cur %d, Lastnum %d, errcnt[%llu, %llu] \n",
                                  auiDbgErr[1], LastNum[1], auiDbgErrCnt[1][0][LastNum[1]], auiDbgErrCnt[1][1][LastNum[1]]);
                    }

                    LastNum[1] = auiDbgErr[1];
#endif
                    goto sync;
                }

                stDbgArr[1].flag = SAL_TRUE;
                stDbgArr[1].pts = tmp_pts[1];

                PPM_LOGD("1 get valid result! \n");
                update_domain(stDbgArr, domain);
            }
        }

        time3 = SAL_getCurMs();

        /* debug: 用来规避获取帧数据时间太长，前一个通道获取的数据已经很旧，直接对已有数据进行释放重新获取 */
        if (time3 - time2 > 100)
        {
            SAL_ERROR("clear result and reuse! \n");
            goto clean;
        }


        if (!pstDevPrm->bNeedSync)
        {
            PPM_LOGW("============================ get sync Data!!!!!!!!!!!!! tri %d, face %d, %llu, %llu, %llu \n",
                     DSA_QueGetQueuedCount(pstTriFullQue), DSA_QueGetQueuedCount(pstFaceFullQue), tmp_pts[0], tmp_pts[1], tmp_pts[2]);
            pstDevPrm->bNeedSync = SAL_TRUE;

            if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_DUMP_SYNC_DATA) & 0x1)
            {
                pstComFrm[0] = pstComFrmInfo[0];
                pstComFrm[1] = pstComFrmInfo[1];
                (VOID)SAL_thrCreate(&pstDevPrm->stDebugPrm.stDumpSyncDataHdl, Ppm_DrvDumpSyncDataThread, SAL_THR_PRI_DEFAULT, 0, NULL);
                (VOID)SAL_thrDetach(&pstDevPrm->stDebugPrm.stDumpSyncDataHdl);
            }
        }

        uiSyncDataCnt++;

        if (uiSyncDataCnt >= uiSyncCnt)
        {
            pstDevPrm->bNeedSync = SAL_FALSE;
            uiSyncDataCnt = 0;
        }

		/* 保存三目和人脸的时间戳 */
		s64Tmp[0] = tmp_pts[0];
		s64Tmp[1] = tmp_pts[1];

        (VOID)clear_tmp_result();   /* 清空临时调试结果 */

        if (NULL == pstComFrmInfo[0] || NULL == pstComFrmInfo[1])
        {
            SAL_ERROR("ERRRRRRRRRR------------%p, %p, %p \n", pstComFrmInfo[0], pstComFrmInfo[1], pstPpmFrmInfo);
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
            SAL_ERROR("=================== ERRRRRRRRRRRRRRRRRR ===================== \n");
			
            sleep(1);
            goto clean;
        }
        time3_1 = SAL_getCurMs();

		/* match tri frame pts with depth info from result queue */
		if (u32PrMetaJson & 0x2)
		{
			bMatch = SAL_FALSE;
			do
			{
				pstDepthInfo = NULL;  // reset
				if (SAL_SOK != DSA_QuePeek(pstDepthFullQue, (VOID **)&pstDepthInfo)
					|| NULL == pstDepthInfo)
				{
					/* 预同步成功后，若深度信息队列为空，增加一个超时逻辑避免多线程异步引入的频繁触发预同步缓存的问题 */
					bDepthFirstSyncFlag = SAL_FALSE;
					bFFFlag = SAL_FALSE;
					break;
				}

				PPM_LOGD("111: get que peek %p \n", pstDepthInfo);
				s64Tmp[2] = (INT64L)pstDepthInfo->u64Timestamp;

				/* 第一次同步处理 */
				if (!bDepthFirstSyncFlag)
				{
					/* 深度信息时间戳比三目小超过1000ms or 深度信息时间戳比三目大5000ms时，判定时间差异常 */
					if (s64Tmp[2] + 1000 < s64Tmp[0] || s64Tmp[2] > s64Tmp[0] + 5000)
					{
						PPM_LOGE("gap too big! depth %llu, tri %llu \n", s64Tmp[2], s64Tmp[0]);

						/* 清空深度信息队列 */
						Ppm_DrvClrQueData(pstDepthInfoQuePrm);

						/* 清空三目图像队列 */
						Ppm_DrvClrQueData(pstTriQuePrm);

						/* 清空人脸图像队列 */
						Ppm_DrvClrQueData(pstFaceQuePrm);

						PPM_LOGE("clr que data end! \n");
						break;
					}

                    /* 当三目时戳比深度信息时间戳大，需要持续缓存深度数据后开启同步，由第一次预同步的差值为依据 */
					if (s64Tmp[2] < s64Tmp[0])
					{
						if (DSA_QueGetQueuedCount(pstDepthFullQue) < 21)
						{
							usleep(10*1000);
							PPM_LOGD("---buffer depth full queue! cnt %d, depth %llu, tri %llu \n", 
								     DSA_QueGetQueuedCount(pstDepthFullQue), s64Tmp[2], s64Tmp[0]);
							continue;
						}
					}
					else if (s64Tmp[2] > s64Tmp[0])  /* 若三目时间戳比深度小，那么就丢弃缓存的三目帧数据，直到满足预同步的差值 */
					{
						if (!bFFFlag)
						{
							s64Tmp[3] = (s64Tmp[2] - s64Tmp[0]) / 50;
							bFFFlag = SAL_TRUE;
						}

						if (DSA_QueGetQueuedCount(pstDepthFullQue) < s64Tmp[3])
						{
							PPM_LOGD("+++buffer depth full queue! cnt %d, depth %llu, tri %llu \n", 
								     DSA_QueGetQueuedCount(pstDepthFullQue), s64Tmp[2], s64Tmp[0]);
							goto clean;
						}
					}

					bDepthFirstSyncFlag = SAL_TRUE;
					PPM_LOGD("++++++++++++++++++++++++++++++++ first sync success! \n");
				}

				/* 增加三目和深度的时间戳同步，以一帧误差作为基准并增加过补偿 */
				{
					if (s64Tmp[2] > s64Tmp[0]-u32DepthSyncGap/2 && s64Tmp[2] < s64Tmp[0]+u32DepthSyncGap/2)
					{
						bMatch = SAL_TRUE;
						PPM_LOGD("222: match success! tri %llu, depth %llu, gap %lld, thr %u, full que cnt %u \n", 
								  s64Tmp[0], s64Tmp[2], s64Tmp[2]-s64Tmp[0], u32DepthSyncGap, DSA_QueGetQueuedCount(pstDepthFullQue));

						DSA_QueGet(pstDepthFullQue, (VOID **)&pstDepthInfo, SAL_TIMEOUT_FOREVER);
						memcpy(&stMatchDepthInfo, pstDepthInfo, sizeof(PPM_DEPTH_INFO_S));
						DSA_QuePut(pstDepthEmptQue, (VOID *)pstDepthInfo, SAL_TIMEOUT_NONE);

						break;
					}
					/* 若深度信息时间戳比三目时间戳小超过300ms以上，需要丢弃 */
					else if (s64Tmp[2] < s64Tmp[0]-u32DepthSyncGap/2)
					{
						PPM_LOGD("333: ---------- delete old depth %llu, tri %llu \n", s64Tmp[2], s64Tmp[0]);
					
						DSA_QueGet(pstDepthFullQue, (VOID **)&pstDepthInfo, SAL_TIMEOUT_NONE);
						DSA_QuePut(pstDepthEmptQue, (VOID *)pstDepthInfo, SAL_TIMEOUT_NONE);

						usleep(10*1000);
					}
					/* 否则跳出循环，释放当前三目帧，并等待下一次匹配 */
					else
					{
						PPM_LOGW("444: ---------- depth %llu largerrrrr tri %llu \n", s64Tmp[2], s64Tmp[0]);
						break;
					}
				}
			} while(1);

			/* continue loop if match failed */
			if (!bMatch)
			{
				PPM_LOGD("match depth info failed! depth %llu, tri %llu \n", pstDepthInfo ? s64Tmp[2] : 0, s64Tmp[0]);
				goto clean;
			}

			memcpy(&pstPpmFrmInfo->stDepthInfo, &stMatchDepthInfo, sizeof(PPM_DEPTH_INFO_S));
			PPM_LOGD("========= get sync depth info! \n");
        }

        time4 = SAL_getCurMs();

#if 1
        /* 拷贝三目相机帧数据 */
        s32Ret = Ia_TdeQuickCopy(&pstComFrmInfo[0]->stFrame[0], &pstPpmFrmInfo->stTriCamFrame,
                                 0, 0, PPM_TRI_CAM_CHN_WIDTH, PPM_TRI_CAM_CHN_HEIGHT, SAL_FALSE);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("PPM: Tde Copy tri frame fail! ret: 0x%x \n", s32Ret);
            goto clean;
        }


        /* recycle queue buf */
        (VOID)Ppm_DrvPutQueData(pstTriEmptQue, (VOID *)pstComFrmInfo[0], SAL_TIMEOUT_NONE);
        pstComFrmInfo[0] = NULL;

        time4_1 = SAL_getCurMs();

        /* 拷贝人脸相机帧数据 */
        s32Ret = Ia_TdeQuickCopy(&pstComFrmInfo[1]->stFrame[0], &pstPpmFrmInfo->stFaceFrame,
                                 0, 0, PPM_FACE_WIDTH, PPM_FACE_HEIGHT, SAL_FALSE);
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("PPM: Tde Copy face frame fail! ret: 0x%x \n", s32Ret);
            goto clean;
        }

        time5 = SAL_getCurMs();

        /* recycle queue buf */
        (VOID)Ppm_DrvPutQueData(pstFaceEmptQue, (VOID *)pstComFrmInfo[1], SAL_TIMEOUT_NONE);
        pstComFrmInfo[1] = NULL;

        time5_0 = SAL_getCurMs();

        time5_1 = SAL_getCurMs();

#endif

        time_end = SAL_getCurMs();

        /* 送入推帧线程的yuv数据队列中 */
        (VOID)Ppm_DrvPutQueData(pstYuvFullQue, (VOID *)pstPpmFrmInfo, SAL_TIMEOUT_NONE);
        pstPpmFrmInfo = NULL;

        fps++;

        time6 = SAL_getCurMs();

        if (time5_1 - time3_1 > 60)
        {
            PPM_LOGW("%s: %d, %d, %d, %d, %d, sync %d, %d, %d, %d, %d, %d, %d, sleep %d \n", __func__,
                     time1 - time0, time_start - time1, time2 - time_start, time3 - time_start, time3_1 - time_start, time4 - time3_1,
                     time4_1 - time4, time5 - time4, time5_0 - time5, time5_1 - time5, time_end - time5, time6 - time_end, sleep_ms);
        }

        continue;

clean:
        (VOID)clear_tmp_result();   /* 清空临时调试结果 */

        if (!stDbgArr[0].flag && pstComFrmInfo[0])
        {
            (VOID)Ppm_DrvPutQueData(pstTriEmptQue, (VOID *)pstComFrmInfo[0], SAL_TIMEOUT_NONE);
            pstComFrmInfo[0] = NULL;
        }

        if (!stDbgArr[1].flag && pstComFrmInfo[1])
        {
            (VOID)Ppm_DrvPutQueData(pstFaceEmptQue, (VOID *)pstComFrmInfo[1], SAL_TIMEOUT_NONE);
            pstComFrmInfo[1] = NULL;
        }

        (VOID)clear_tmp_result();   /* 清空临时调试结果 */

reuse_3:

        /* recycle queue buf */
        if (pstComFrmInfo[1])
        {
            (VOID)Ppm_DrvPutQueData(pstFaceEmptQue, (VOID *)pstComFrmInfo[1], SAL_TIMEOUT_NONE);
            pstComFrmInfo[1] = NULL;
        }

reuse_2:
        /* recycle queue buf */
        if (pstComFrmInfo[0])
        {
            (VOID)Ppm_DrvPutQueData(pstTriEmptQue, (VOID *)pstComFrmInfo[0], SAL_TIMEOUT_NONE);
            pstComFrmInfo[0] = NULL;
        }

reuse_1:
        /* recycle queue buf */
        if (pstPpmFrmInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstYuvEmptQue, (VOID *)pstPpmFrmInfo, SAL_TIMEOUT_NONE);
            pstPpmFrmInfo = NULL;
        }
    }

err:
    return NULL;

}

#endif

/**
 * @function:   Ppm_DrvJencThread
 * @brief:      结果编图处理线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvJencThread(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiXsiJpgCnt = 0;
    UINT32 uiEndTime = 0;
    UINT32 uiCbStartTime = 0;
    UINT32 uiCbEndTime = 0;

    CHAR path[64] = {0};
    CHAR *pPrefix = "/home/config";

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_QUEUE_PRM *pstQuePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;          /* 数据满队列 */
    DSA_QueHndl *pstEmptQue = NULL;          /* 数据空队列 */
    PPM_RSLT_INFO_S *pstRsltInfo = NULL;
    PPM_PKG_RSLT_S *pstPkgRslt = NULL;
    PPM_XSI_RSLT_S *pstXsiRslt = NULL;
    PPM_FACE_RSLT_S *pstFaceRslt = NULL;
    PPM_DSP_OBJ_INFO *pstObjOutInfo = NULL;

    STREAM_ELEMENT stStreamEle = {0};
    PPM_DSP_OUT stPpmDspOut = {0};
    SAL_VideoFrameParam stEncCreatePrm = {0};

    pstDevPrm = (PPM_DEV_PRM *)prm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQuePrm = &pstDevPrm->stRsltQueInfo.stRsltQuePrm;
    PPM_DRV_CHECK_PTR(pstQuePrm, err, "pstQuePrm == null!");

    pstFullQue = pstQuePrm->pstFullQue;
    pstEmptQue = pstQuePrm->pstEmptQue;

    stEncCreatePrm.width = 1920;
    stEncCreatePrm.height = 1080;
    stEncCreatePrm.quality = 99;
    stEncCreatePrm.encodeType = MJPEG;
    stEncCreatePrm.fps = 1;
    stEncCreatePrm.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = jpeg_drv_createChn((VOID **)&pstDevPrm->pJpegChnInfo, &stEncCreatePrm);
    PPM_DRV_CHECK_RET(s32Ret, err, "jpeg_drv_createChn failed!");

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind(0);

    while (SAL_TRUE)
    {
        (VOID)Ppm_DrvGetQueData(pstFullQue, (VOID **)&pstRsltInfo, SAL_TIMEOUT_FOREVER);

        SAL_DEBUG("get rslt!!! \n");

        sal_memset_s(&stStreamEle, sizeof(STREAM_ELEMENT), 0x00, sizeof(STREAM_ELEMENT));
        sal_memset_s(&stPpmDspOut, sizeof(PPM_DSP_OUT), 0x00, sizeof(PPM_DSP_OUT));

        /* pkg obj enc */
        pstPkgRslt = &pstRsltInfo->stPkgRsltOut;
        stPpmDspOut.stPpmPkgOut.pkg_cnt = pstPkgRslt->uiPkgCnt;

        SAL_INFO("get pkg cnt %d \n", pstPkgRslt->uiPkgCnt);

        for (i = 0; i < pstPkgRslt->uiPkgCnt; i++)
        {
            pstObjOutInfo = &stPpmDspOut.stPpmPkgOut.stPkgObjInfo[i];
            pstObjOutInfo->match_id = pstPkgRslt->stPkgObjInfo[i].uiMatchId;

            memset(pstDevPrm->stPkgJpegInfo[i].pcJpegAddr, 0x00, 1920 * 1080 * 4);

            s32Ret = Ppm_DrvSendJenc(pstDevPrm->pJpegChnInfo, &pstPkgRslt->stPkgObjInfo[i].stFrameInfo, &pstDevPrm->stPkgJpegInfo[i]);
            PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvSendJenc failed!");

            pstObjOutInfo->stCbPicInfo.jpeg_cnt = 1;   /* 暂时单一目标信息仅编一张图 */
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiWidth = pstPkgRslt->stPkgObjInfo[i].stFrameInfo.uiDataWidth;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiHeight = pstPkgRslt->stPkgObjInfo[i].stFrameInfo.uiDataHeight;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].cJpegAddr = (VOID *)pstDevPrm->stPkgJpegInfo[0].pcJpegAddr;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiJpegSize = pstDevPrm->stPkgJpegInfo[0].uiJpegSize;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].ullTimePts = pstRsltInfo->stPkgRsltOut.uiJpegPts;

            if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_DUMP_JPG_MASK) & 0x1)
            {
                memset(path, 0x00, 64);
                sprintf(path, "%s/%d_pkg_%d.jpg", pPrefix, pstObjOutInfo->match_id, i);
                (VOID)Ppm_HalDebugDumpData((VOID *)pstDevPrm->stPkgJpegInfo[0].pcJpegAddr,
                                           path,
                                           pstDevPrm->stPkgJpegInfo[0].uiJpegSize,
                                           0);
            }

            SAL_WARN("pkg: i %d, %p, size %d, ullTimePts %lld \n", i,
                     pstObjOutInfo->stCbPicInfo.stJpegResult[0].cJpegAddr,
                     pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiJpegSize,
                     pstObjOutInfo->stCbPicInfo.stJpegResult[0].ullTimePts);
        }

        /* xsi obj enc */
        uiXsiJpgCnt = 0;
        pstXsiRslt = &pstRsltInfo->stXsiRsltOut;
        stPpmDspOut.stPpmXpkgOut.x_pkg_cnt = pstXsiRslt->uiXsiCnt;

        SAL_INFO("get xsi cnt %d \n", pstXsiRslt->uiXsiCnt);
        for (i = 0; i < pstXsiRslt->uiXsiCnt; i++)
        {
            pstObjOutInfo = &stPpmDspOut.stPpmXpkgOut.stXpkgObjInfo[i];
            pstObjOutInfo->match_id = pstXsiRslt->stXsiObjInfo[i].uiMatchId;

            /* 当前X光包裹回调两路数据给上层 */
            for (j = 0; j < 1; j++)
            {
                SAL_INFO("i %d, j %d, %d \n", i, j, PPM_MAX_XSI_FRM_NUM);
                memset(pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr, 0x00, 1920 * 1080 * 4);

                s32Ret = Ppm_DrvSendJenc(pstDevPrm->pJpegChnInfo, &pstXsiRslt->stXsiObjInfo[i].stFrameInfo[j], &pstDevPrm->stXpkgJpegInfo[j]);
                PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvSendJenc failed!");

                pstObjOutInfo->stCbPicInfo.stJpegResult[uiXsiJpgCnt].uiWidth = pstXsiRslt->stXsiObjInfo[i].stFrameInfo[j].uiDataWidth;
                pstObjOutInfo->stCbPicInfo.stJpegResult[uiXsiJpgCnt].uiHeight = pstXsiRslt->stXsiObjInfo[i].stFrameInfo[j].uiDataHeight;
                pstObjOutInfo->stCbPicInfo.stJpegResult[uiXsiJpgCnt].cJpegAddr = (VOID *)pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr;
                pstObjOutInfo->stCbPicInfo.stJpegResult[uiXsiJpgCnt].uiJpegSize = pstDevPrm->stXpkgJpegInfo[j].uiJpegSize;

                SAL_WARN("xsi: j %d, %p, size %d \n", j,
                         pstObjOutInfo->stCbPicInfo.stJpegResult[uiXsiJpgCnt].cJpegAddr, pstObjOutInfo->stCbPicInfo.stJpegResult[uiXsiJpgCnt].uiJpegSize);

                if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_DUMP_JPG_MASK) & 0x1)
                {
                    memset(path, 0x00, 64);
                    sprintf(path, "%s/%d_xsi_%d_%d.jpg", pPrefix, pstObjOutInfo->match_id, i, j);
                    (VOID)Ppm_HalDebugDumpData((VOID *)pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr,
                                               path,
                                               pstDevPrm->stXpkgJpegInfo[j].uiJpegSize,
                                               0);
                }

                uiXsiJpgCnt++;
            }

            /* 当前调试版本仅开放第一路，待后续版本更新 */
            pstObjOutInfo->stCbPicInfo.jpeg_cnt = uiXsiJpgCnt;
        }

        /* face obj enc */
        pstFaceRslt = &pstRsltInfo->stFaceRsltOut;
        stPpmDspOut.stPpmFaceOut.face_cnt = pstFaceRslt->uiFaceCnt;

        SAL_INFO("get face cnt %d \n", pstFaceRslt->uiFaceCnt);

        for (i = 0; i < pstFaceRslt->uiFaceCnt; i++)
        {
            pstObjOutInfo = &stPpmDspOut.stPpmFaceOut.stFaceObjInfo[i];
            pstObjOutInfo->match_id = pstFaceRslt->stFaceObjInfo[i].uiMatchId;

            memset(pstDevPrm->stFaceJpegInfo[i].pcJpegAddr, 0x00, 1920 * 1080 * 4);

#ifdef DEBUG_DUMP_DATA
            char path[64] = {0};
            char *pData = NULL;

            UINT32 uiSize = 0;
            SAL_VideoFrameBuf stVideoFrmBuf = {0};

            memset(path, 0x00, 64 * sizeof(char));

            (VOID)sys_hal_getVideoFrameInfo(&pstFaceRslt->stFaceObjInfo[i].stFrameInfo, &stVideoFrmBuf);

            sprintf(path, "/mnt/nfs/ppm/%d_jpeg_face_%dx%d.nv21",
                    pstFaceRslt->stFaceObjInfo[i].uiMatchId, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height);

            pData = (VOID *)stVideoFrmBuf.virAddr[0];
            uiSize = stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height * 3 / 2;

            (VOID)Ppm_HalDebugDumpData((VOID *)pData, path, uiSize, 0);

            SAL_WARN("fwrite end! \n");
#endif

            s32Ret = Ppm_DrvSendJenc(pstDevPrm->pJpegChnInfo, &pstFaceRslt->stFaceObjInfo[i].stFrameInfo, &pstDevPrm->stFaceJpegInfo[i]);
            PPM_DRV_CHECK_RET(s32Ret, reuse, "Ppm_DrvSendJenc failed!");

            pstObjOutInfo->stCbPicInfo.jpeg_cnt = 1;   /* 暂时单一目标信息仅编一张图 */
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiWidth = pstFaceRslt->stFaceObjInfo[i].stFrameInfo.uiDataWidth;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiHeight = pstFaceRslt->stFaceObjInfo[i].stFrameInfo.uiDataHeight;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].cJpegAddr = (VOID *)pstDevPrm->stFaceJpegInfo[i].pcJpegAddr;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiJpegSize = pstDevPrm->stFaceJpegInfo[i].uiJpegSize;
            pstObjOutInfo->stCbPicInfo.stJpegResult[0].ullTimePts = pstRsltInfo->stFaceRsltOut.uiJpegPts;

            SAL_WARN("face: i %d, %p, size %d, ullTimePts %lld \n", i,
                     pstObjOutInfo->stCbPicInfo.stJpegResult[0].cJpegAddr,
                     pstObjOutInfo->stCbPicInfo.stJpegResult[0].uiJpegSize,
                     pstObjOutInfo->stCbPicInfo.stJpegResult[0].ullTimePts);

            if ((Ppm_DrvJudgePrintPosInfo() >> PPM_LOG_DUMP_JPG_MASK) & 0x1)
            {
                memset(path, 0x00, 64);
                sprintf(path, "%s/%d_face_%d.jpg", pPrefix, pstObjOutInfo->match_id, i);
                (VOID)Ppm_HalDebugDumpData((VOID *)pstDevPrm->stFaceJpegInfo[i].pcJpegAddr,
                                           path,
                                           pstDevPrm->stFaceJpegInfo[i].uiJpegSize,
                                           0);
            }
        }

        SAL_getDateTime_tz(&stStreamEle.absTime);

        uiEndTime = SAL_getCurMs();

        SAL_WARN("to app: ppm result before cb! %d, %d, %d, get ipt time %d, cur %d, jenc cost %d \n",
                 pstPkgRslt->uiPkgCnt, pstXsiRslt->uiXsiCnt, pstFaceRslt->uiFaceCnt, pstRsltInfo->uiReverse[0], uiEndTime, uiEndTime - pstRsltInfo->uiReverse[0]);

        uiCbStartTime = SAL_getCurMs();

        stStreamEle.type = STREAM_ELEMENT_PPM_IMG;
        SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stPpmDspOut, sizeof(PPM_DSP_OUT));

        uiCbEndTime = SAL_getCurMs();
        SAL_WARN("cb func cost %d! start %d, end %d \n", uiCbEndTime - uiCbStartTime, uiCbStartTime, uiCbEndTime);

reuse:
        if (pstRsltInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pstRsltInfo, SAL_TIMEOUT_NONE);
        }

        pstRsltInfo = NULL;
    }

err:
    return NULL;
}

/**
 * @function:   Ppm_DrvIptThread
 * @brief:      推帧处理线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvIptThread(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 id = 0;
    UINT32 time_start = 0;
    UINT32 time0 = 0;
    UINT32 time_end = 0;

    UINT32 uiTimeTmp = 0;    /* 用于统计引擎回调图片慢的问题，作为时间戳传入 */
    UINT64 time000 = 0;
    UINT64 time111 = 0;
    UINT32 fps = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
	
    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_SUB_MOD_S *pstSubModPrm = NULL;
    ICF_INPUT_DATA *pstInputData = NULL;

    /* 推帧数据队列参数 */
    PPM_QUEUE_PRM *pstQuePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    PPM_IPT_YUV_DATA_INFO *pstIptData = NULL;
	MTME_INPUT_T *pstMtmeInputData = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;
	MTME_INPUT_T *pstMtmeInputInfo = NULL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    pstPpmGlobalPrm = Ppm_HalGetModPrm();

    pstDevPrm = (PPM_DEV_PRM *)prm;
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    pstQuePrm = &pstDevPrm->stIptQueInfo.stIptQuePrm;
    pstFullQue = pstQuePrm->pstFullQue;     /* 满队列 */
    pstEmptQue = pstQuePrm->pstEmptQue;     /* 空队列 */

    pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_MATCH);
    PPM_DRV_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

	pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_DRV_CHECK_PTR(pstMatchChnPrm, err, "pstMatchChnPrm == null!");

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind(3);

    while (SAL_TRUE)
    {
        (VOID)Ppm_DrvGetQueData(pstFullQue, (VOID **)&pstIptData, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstIptData, reuse, "queue data errrrrrrr!");

        /* 帧率统计 */
        (VOID)Ppm_DrvCalThrFps(&time000, &time111, &fps, __func__);

        time_start = SAL_getCurMs();

        if (SAL_SOK != Ppm_HalGetFreeIptData(0, &id))
        {
            SAL_ERROR("get free input data failed! \n");
            goto reuse;
        }

        pstInputData = &pstSubModPrm->stIptDataPrm.stInputDataInfo[id].stIptDataInfo;

		pstMtmeInputData = (MTME_INPUT_T *)pstInputData->stBlobData[0].pData;
        PPM_DRV_CHECK_PTR(pstMtmeInputData, reuse_1, "sys_hal_getVideoFrameInfo failed!");

		/* 填充人脸的图像数据 */
        s32Ret = sys_hal_getVideoFrameInfo(&pstIptData->stFaceYuv, &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_getVideoFrameInfo failed!");

		pstMtmeInputData->face_image.y = (UINT8 *)stVideoFrmBuf.virAddr[0];
		pstMtmeInputData->face_image.u = (UINT8 *)pstMtmeInputData->face_image.y + DEMO_FACE_WIDTH * DEMO_FACE_HEIGHT;
		pstMtmeInputData->face_image.v = (UINT8 *)pstMtmeInputData->face_image.u;
        pstMtmeInputData->face_image.image_w = DEMO_FACE_WIDTH;
        pstMtmeInputData->face_image.image_h = DEMO_FACE_HEIGHT;
        pstMtmeInputData->face_image.format  = VCA_IMG_YUV_NV21;

		/* 填充私有信息 */
        pstSubModPrm->stIptDataPrm.stInputDataInfo[id].stUsrPrvtInfo.uiIptPts[0] = stVideoFrmBuf.pts;

		/* 填充三目可见光的图像数据 */
        s32Ret = sys_hal_getVideoFrameInfo(&pstIptData->stTriViewYuv[PPM_TRI_CAM_MIDDLE_CHN], &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, reuse_1, "sys_hal_getVideoFrameInfo failed!");

		pstMtmeInputData->vertical_image.y = (UINT8 *)stVideoFrmBuf.virAddr[0];
		pstMtmeInputData->vertical_image.u = (UINT8 *)pstMtmeInputData->vertical_image.y + DEMO_TRI_VISION_WIDTH * DEMO_TRI_VISION_HEIGHT;
		pstMtmeInputData->vertical_image.v = (UINT8 *)pstMtmeInputData->vertical_image.u;
        pstMtmeInputData->vertical_image.image_w = DEMO_TRI_VISION_WIDTH;
        pstMtmeInputData->vertical_image.image_h = DEMO_TRI_VISION_HEIGHT;
        pstMtmeInputData->vertical_image.format  = VCA_IMG_YUV_NV21;

		/* 填充私有信息 */
        pstSubModPrm->stIptDataPrm.stInputDataInfo[id].stUsrPrvtInfo.uiIptPts[1] = stVideoFrmBuf.pts;

		/* 填充帧序号和时间戳 */
        pstInputData->stBlobData[0].nFrameNum = pstSubModPrm->uiFrameIdx;
        pstInputData->stBlobData[0].nTimeStamp = pstSubModPrm->uiFrameIdx * 50;

	    /* 填充前端相机发送来的深度信息，包括7个关节点和深度头肩 */
		pstMtmeInputInfo = (MTME_INPUT_T *)pstInputData->stBlobData[0].pData;
		memcpy(&pstMtmeInputInfo->obj_info, &pstIptData->stMtmeInputObjInfo, sizeof(MTME_OBJ_INPUT_INFO_T));

#if 0
        /* 安检机输出帧数据 */
        s32Ret = sys_hal_getVideoFrameInfo(&pstIptData->stXpkgYuv[0], &stVideoFrmBuf);
        PPM_DRV_CHECK_RET(s32Ret, err, "sys_hal_getVideoFrameInfo failed!");

        pstInputData->stBlobData[uiIdx].pData = (VOID *)stVideoFrmBuf.virAddr[0];
        pstInputData->stBlobData[uiIdx].nFrameNum = pstSubModPrm->uiFrameIdx;
        pstInputData->stBlobData[uiIdx++].nTimeStamp = pstSubModPrm->uiFrameIdx * 80;
#endif

        if (uiDbgSwitchFlag == 1)
        {
            char path[64] = {0};
            UINT32 uiResoTab[DMEO_SRC_DIM][2] =
            {
                {DEMO_FACE_WIDTH, DEMO_FACE_HEIGHT},
				{DEMO_TRI_VISION_WIDTH, DEMO_TRI_VISION_HEIGHT},
            };
			
            char *type[DMEO_SRC_DIM] = {"face", "mid"};
            char *path_prefix = "/home/config/"; /* "/home/config/"; */

			/* face, 1080P */
            snprintf(path, 64, "%s/dbg_%s.nv21", path_prefix, type[0]);
            (VOID)Ppm_HalDebugDumpData((VOID *)pstMtmeInputData->face_image.y, path,
                                       uiResoTab[0][0] * uiResoTab[0][1] * 3 / 2, 1);

			/* tri_vision, 1080P */
			snprintf(path, 64, "%s/dbg_%s.nv21", path_prefix, type[1]);
			(VOID)Ppm_HalDebugDumpData((VOID *)pstMtmeInputData->vertical_image.y, path,
									   uiResoTab[1][0] * uiResoTab[1][1] * 3 / 2, 1);
        }

        /* 填充私有数据，目前包括时间戳和X光第二路数据 */
        uiTimeTmp = SAL_getCurMs();
        pstSubModPrm->stIptDataPrm.stInputDataInfo[id].stUsrPrvtInfo.uiIptTime = uiTimeTmp;

        pstInputData->pUserPtr = (VOID *)&pstSubModPrm->stIptDataPrm.stInputDataInfo[id];
        pstInputData->nUserDataSize = sizeof(PPM_INPUT_DATA_INFO);

        time0 = SAL_getCurMs();
        if (1)
        {
            s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfInputData(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
				                                                 (VOID *)pstInputData, 
				                                                 sizeof(ICF_INPUT_DATA));
            PPM_DRV_CHECK_RET(s32Ret, reuse_1, "ICF_Input_data failed!");

            pstDevPrm->stDebugPrm.uiIptCnt++;
        }
        else
        {
            usleep(20 * 1000);
            pstSubModPrm->stIptDataPrm.stInputDataInfo[id].bUsed = SAL_FALSE;
        }

        pstSubModPrm->uiFrameIdx++;
        fps++;

        if (pstIptData)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pstIptData, SAL_TIMEOUT_NONE);
            pstIptData = NULL;
        }

        time_end = SAL_getCurMs();

        if ((time_end - time0 >= 60)) /*  */
        {
            SAL_ERROR("%s: %d, %d, total cost %d ms \n", __func__,
                      time0 - time_start, time_end - time0, time_end - time_start);
        }

        continue;

reuse_1:
        pstSubModPrm->stIptDataPrm.stInputDataInfo[id].bUsed = SAL_FALSE;
reuse:
        if (pstIptData)
        {
            (VOID)Ppm_DrvPutQueData(pstEmptQue, (VOID *)pstIptData, SAL_TIMEOUT_NONE);
            pstIptData = NULL;
        }
    }

err:
    return NULL;
}

BOOL Ppm_DrvPrDepthPos(VOID)
{
	return (!!(u32PrMetaJson & 0x8));
}

static VOID Ppm_DrvConvtInnerDepthInfo(IN PPM_DEPTH_INFO_S *pstDepthInfo,
	                                   OUT MTME_OBJ_INPUT_INFO_T *pstMtmeInputObjInfo)
{
	UINT32 i = 0, j = 0;
	
	pstMtmeInputObjInfo->obj_num = pstDepthInfo->u32DetObjNum;

	if (Ppm_DrvPrDepthPos())
	{
		PPM_LOGD("+++++++++ pos: obj_num %d \n", pstMtmeInputObjInfo->obj_num);
	}

	for(i = 0; i < pstMtmeInputObjInfo->obj_num; i++)
	{
		pstMtmeInputObjInfo->obj_id[i] = pstDepthInfo->astDetObjInfo[i].u32Id;
		pstMtmeInputObjInfo->obj_valid[i] = pstDepthInfo->astDetObjInfo[i].bValid;
		pstMtmeInputObjInfo->obj_disappear[i] = pstDepthInfo->astDetObjInfo[i].bDisapper;
		pstMtmeInputObjInfo->obj_height[i] = pstDepthInfo->astDetObjInfo[i].fHeight;
	
		// head
		pstMtmeInputObjInfo->obj_headrect[i].x = pstDepthInfo->astDetObjInfo[i].stHeadRect.x;
		pstMtmeInputObjInfo->obj_headrect[i].y = pstDepthInfo->astDetObjInfo[i].stHeadRect.y;
		pstMtmeInputObjInfo->obj_headrect[i].width = pstDepthInfo->astDetObjInfo[i].stHeadRect.w;
		pstMtmeInputObjInfo->obj_headrect[i].height = pstDepthInfo->astDetObjInfo[i].stHeadRect.h;
	
		if (Ppm_DrvPrDepthPos())
		{
			PPM_LOGD("i %d, id %d, valid %d, disappear %d, height %f, head[%f, %f, %f, %f] \n", i,
					 pstMtmeInputObjInfo->obj_id[i], pstMtmeInputObjInfo->obj_valid[i], 
					 pstMtmeInputObjInfo->obj_disappear[i], pstMtmeInputObjInfo->obj_height[i],
					 pstMtmeInputObjInfo->obj_headrect[i].x, pstMtmeInputObjInfo->obj_headrect[i].y,
					 pstMtmeInputObjInfo->obj_headrect[i].width, pstMtmeInputObjInfo->obj_headrect[i].height);
		}
		
		// joints
		pstMtmeInputObjInfo->obj_joinmsg[i].id = pstDepthInfo->astDetObjInfo[i].u32JointId;
	
		if (Ppm_DrvPrDepthPos())
		{
			PPM_LOGD("joint id %d, joint num %d \n", pstMtmeInputObjInfo->obj_joinmsg[i].id, pstDepthInfo->astDetObjInfo[i].u32JointNum);
		}
		
		for(j = 0; j < pstDepthInfo->astDetObjInfo[i].u32JointNum; j++)
		{
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].type = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].u32JointType;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].score = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].fJointScore;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].state = 1;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.x = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].st2dPoint.x;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.y = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].st2dPoint.y;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.x = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].st3dPoint.x;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.y = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].st3dPoint.y;
			pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.z = pstDepthInfo->astDetObjInfo[i].astJointInfo[j].st3dPoint.z;
	
			if (Ppm_DrvPrDepthPos())
			{
				PPM_LOGD("joints: type %d, score %f, state %d, 2d[%f, %f], 3d[%f, %f, %f] \n",
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].type,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].score,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].state,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.x,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.y,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.x,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.y,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.z);
			}
		}
	}

	return;
}

/**
 * @function   Ppm_DrvDbgCbDumpNv21PlusDepth
 * @brief      素材采集（回调人脸、三目可见光和深度信息）
 * @param[in]  PPM_IPT_YUV_DATA_INFO *pstIptData  
 * @param[out] None
 * @return     static VOID
 */
static VOID Ppm_DrvDbgCbDumpNv21PlusDepth(PPM_IPT_YUV_DATA_INFO *pstIptData)
{
	INT32 s32Ret = SAL_FAIL;
	
    /* 人包迁移项目素材采集使用，后续基线化整理 */
	UINT32 u32Offset = 0;
	static char *pDumpData = NULL;
	UINT32 u32FaceSize = 1920*1080*3/2;
	UINT32 u32TriSize = 1920*1080*3/2;
	UINT32 u32SingleDataSize = u32FaceSize + u32TriSize + sizeof(MTME_OBJ_INPUT_INFO_T);
	static UINT64 u64PhyDbg = 0;
	UINT32 u32PosTime[3] = {0};

	static VOID *pVirDbg = NULL;
	
	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	STREAM_ELEMENT stStreamEle = {0};
	SVA_DSP_POS_OUT stSvaDspPosOut = {0};
	static SYSTEM_FRAME_INFO stTmpCpyFrm = {0};
	
	if (stTmpCpyFrm.uiAppData == 0x00)
	{
		(VOID)sys_hal_allocVideoFrameInfoSt(&stTmpCpyFrm);
	}

	/* debug: write sync data into file, including face yuv, tri yuv and depth info, for engine debugging */
	if (u32PrMetaJson & 0x4)
	{
		PPM_LOGW("write file enter! \n");

		if (NULL == pDumpData)
		{
			s32Ret = mem_hal_mmzAlloc(u32SingleDataSize, "ppm", "debug", NULL, 1, &u64PhyDbg, (VOID **)&pVirDbg);
			PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "mmz alloc failed!");
			
			pDumpData = pVirDbg;
		}

		{
			u32PosTime[0] = SAL_getCurMs();

			/* copy face yuv */
			stVideoFrmBuf.frameParam.width = DEMO_FACE_WIDTH;
			stVideoFrmBuf.frameParam.height = DEMO_FACE_HEIGHT;
			stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = DEMO_FACE_WIDTH;
			stVideoFrmBuf.phyAddr[0] = u64PhyDbg;
			stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height;
			stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

			(VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, &stTmpCpyFrm);

			s32Ret = Ia_TdeQuickCopy(&pstIptData->stFaceYuv, &stTmpCpyFrm, 0, 0, DEMO_FACE_WIDTH, DEMO_FACE_HEIGHT, 0);
			PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "tde copy failed!");
			
			u32Offset += u32FaceSize;

			/* copy tri yuv */
			stVideoFrmBuf.frameParam.width = DEMO_TRI_VISION_WIDTH;
			stVideoFrmBuf.frameParam.height = DEMO_TRI_VISION_HEIGHT;
			stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = DEMO_TRI_VISION_WIDTH;
			stVideoFrmBuf.phyAddr[0] = u64PhyDbg + u32FaceSize;
			stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height;
			stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

			(VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, &stTmpCpyFrm);

			s32Ret = Ia_TdeQuickCopy(&pstIptData->stTriViewYuv[PPM_TRI_CAM_MIDDLE_CHN], &stTmpCpyFrm, 0, 0, DEMO_TRI_VISION_WIDTH, DEMO_TRI_VISION_HEIGHT, 0);
			PPM_DRV_CHECK_RET_NO_LOOP(s32Ret, "tde copy failed!");

			u32Offset += u32TriSize;	
		
			memcpy((char *)pDumpData + u32Offset, &pstIptData->stMtmeInputObjInfo, sizeof(MTME_OBJ_INPUT_INFO_T));				
			u32Offset += sizeof(MTME_OBJ_INPUT_INFO_T);

			/* 填充回调数据类型 */
			stStreamEle.type = STREAM_ELEMENT_SVA_POS;

			/* 填充回调数据地址和长度信息 */
			stSvaDspPosOut.pcPosData = pDumpData;
			stSvaDspPosOut.uiDataLen = u32SingleDataSize;

			u32PosTime[1] = SAL_getCurMs(); 			
			
			SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stSvaDspPosOut, sizeof(SVA_DSP_POS_OUT));

			u32PosTime[2] = SAL_getCurMs();

			PPM_LOGE("++++++++ cpy cost %d ms, cb cost %d ms \n", u32PosTime[1] - u32PosTime[0], u32PosTime[2] - u32PosTime[1]);
			u32Offset = 0;
		}
	}
}

/**
 * @function:   Ppm_DrvProcThread
 * @brief:      分割数据处理线程
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Ppm_DrvProcThread(VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 time_start = 0;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time_end = 0;
    UINT32 i = 0;

    UINT64 time000 = 0;
    UINT64 time111 = 0;
    UINT32 fps = 0;

    PPM_DEV_PRM *pstDevPrm = NULL;

    PPM_QUEUE_PRM *pstYuvQuePrm = NULL;
    DSA_QueHndl *pstYuvFullQue = NULL;
    DSA_QueHndl *pstYuvEmptQue = NULL;
    PPM_FRAME_INFO_ST *pstPpmFrmInfo = NULL;

    PPM_QUEUE_PRM *pstIptDataQuePrm = NULL;
    DSA_QueHndl *pstIptDataFullQue = NULL;
    DSA_QueHndl *pstIptDataEmptQue = NULL;
    PPM_IPT_YUV_DATA_INFO *pstIptData = NULL;
	
	pstDevPrm = (PPM_DEV_PRM *)prm;
	PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    /* yuv数据队列 */
    pstYuvQuePrm = &pstDevPrm->stYuvQueInfo.stYuvQuePrm;
    pstYuvFullQue = pstYuvQuePrm->pstFullQue;     /* 满队列 */
    pstYuvEmptQue = pstYuvQuePrm->pstEmptQue;     /* 空队列 */

    /* ipt数据队列 */
    pstIptDataQuePrm = &pstDevPrm->stIptQueInfo.stIptQuePrm;
    pstIptDataFullQue = pstIptDataQuePrm->pstFullQue;     /* 满队列 */
    pstIptDataEmptQue = pstIptDataQuePrm->pstEmptQue;     /* 空队列 */

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind(0);

    while (SAL_TRUE)
    {
        (VOID)Ppm_DrvGetQueData(pstYuvFullQue, (VOID **)&pstPpmFrmInfo, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstPpmFrmInfo, reuse, "queue data errrrrrrr!");

        /* 帧率统计 */
        (VOID)Ppm_DrvCalThrFps(&time000, &time111, &fps, __func__);

        time_start = SAL_getCurMs();

        (VOID)Ppm_DrvGetQueData(pstIptDataEmptQue, (VOID **)&pstIptData, SAL_TIMEOUT_FOREVER);
        PPM_DRV_CHECK_PTR(pstIptData, reuse_1, "queue data errrrrrrr!");

        /* 三目相机第四码流进行帧数据分割 */
        s32Ret = Ppm_DrvGetSplitFrame(pstDevPrm->uiChn, &pstPpmFrmInfo->stTriCamFrame, pstIptData);
        PPM_DRV_CHECK_RET(s32Ret, reuse_1, "Ppm_DrvGetSplitFrame failed!");

        time0 = SAL_getCurMs();

        /* 拷贝人脸帧数据 */
        s32Ret = Ia_TdeQuickCopy(&pstPpmFrmInfo->stFaceFrame,
                                 &pstIptData->stFaceYuv,
                                 0, 0,
                                 PPM_FACE_WIDTH, PPM_FACE_HEIGHT, SAL_FALSE);
        PPM_DRV_CHECK_RET(s32Ret, reuse_1, "Ia_TdeQuickCopy face frame data failed!");

        time1 = SAL_getCurMs();

		/* 深度信息按照引擎的头文件进行结构化输出 */
		Ppm_DrvConvtInnerDepthInfo(&pstPpmFrmInfo->stDepthInfo, &pstIptData->stMtmeInputObjInfo);

        if (pstIptData->stMtmeInputObjInfo.obj_num)
        {
            for(i = 0; i < pstIptData->stMtmeInputObjInfo.obj_num; i++)
            {
                if (pstIptData->stMtmeInputObjInfo.obj_height[i] < 1.0)
                {
                    goto reuse_1;
                }
            }
        }
		/* 素材采集 */
		Ppm_DrvDbgCbDumpNv21PlusDepth(pstIptData);

#if 0
        /* 拷贝X光帧数据 */
        s32Ret = Ia_TdeQuickCopy(&pstPpmFrmInfo->stXpkgFrame[0],
                                 &pstIptData->stXpkgYuv[0],
                                 0, 0,
                                 PPM_ISM_CHN_WIDTH, PPM_ISM_CHN_HEIGHT);
        PPM_DRV_CHECK_RET(s32Ret, reuse_1, "Ia_TdeQuickCopy xsi frame data failed!");
#endif
        time2 = SAL_getCurMs();

        fps++;

        if (pstPpmFrmInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstYuvEmptQue, (VOID *)pstPpmFrmInfo, SAL_TIMEOUT_NONE);
            pstPpmFrmInfo = NULL;
        }

        if (pstIptData)
        {
            (VOID)Ppm_DrvPutQueData(pstIptDataFullQue, (VOID *)pstIptData, SAL_TIMEOUT_NONE);
            pstIptData = NULL;
        }

        time_end = SAL_getCurMs();

        if (time_end - time_start > 50)
        {
            PPM_LOGD("%s: %d, %d, %d, %d, total cost %d ms \n", __func__,
                     time0 - time_start, time1 - time0, time2 - time1, time_end - time2, time_end - time_start);
        }

        continue;

reuse_1:
        if (pstIptData)
        {
            (VOID)Ppm_DrvPutQueData(pstIptDataEmptQue, (VOID *)pstIptData, SAL_TIMEOUT_NONE);
            pstIptData = NULL;
        }

reuse:
        if (pstPpmFrmInfo)
        {
            (VOID)Ppm_DrvPutQueData(pstYuvEmptQue, (VOID *)pstPpmFrmInfo, SAL_TIMEOUT_NONE);
            pstPpmFrmInfo = NULL;
        }
    }

err:
    return NULL;
}

/**
 * @function:   Ppm_DrvInitThread
 * @brief:      初始化线程资源
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitThread(UINT32 chan)
{
    PPM_DEV_PRM *pstDevPrm = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    /* 同步线程 */
#ifdef PPM_SYNC_V0
    SAL_thrCreate(&pstDevPrm->stSyncThrHdl, Ppm_DrvSyncThread_V0, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);
#else
    SAL_thrCreate(&pstDevPrm->stTriThrHdl, Ppm_DrvGetTriFrameThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);
    SAL_thrCreate(&pstDevPrm->stFaceThrHdl, Ppm_DrvGetFaceFrameThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);

#ifdef PPM_SYNC_V2
    /* SAL_thrCreate(&pstDevPrm->stXsiThrHdl, Ppm_DrvGetXsiFrameThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm); */
#endif

    SAL_thrCreate(&pstDevPrm->stSyncThrHdl, Ppm_DrvSyncThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);
#endif

    /* 处理线程，主要实现三目数据分割+放缩 */
    SAL_thrCreate(&pstDevPrm->stProcThrHdl, Ppm_DrvProcThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);

    /* 推帧线程，主要实现给引擎推帧 */
    SAL_thrCreate(&pstDevPrm->stIptThrHdl, Ppm_DrvIptThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);

    /* 结果处理线程，主要实现对相关帧数据进行编图 */
    SAL_thrCreate(&pstDevPrm->stJencThrHdl, Ppm_DrvJencThread, SAL_THR_PRI_DEFAULT, 0, pstDevPrm);

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvInitGlobalVar
 * @brief:      初始化业务层全局变量
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_DrvInitGlobalVar(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiSize = 0;
    UINT64 u64PhyAddr = 0;
    UINT32 uiW = 1920;
    UINT32 uiH = 1080;

    PPM_DEV_PRM *pstDevPrm = NULL;

    if (g_stPpmCommPrm.uiChannelCnt > 0)
    {
        SAL_WARN("drv layer global var is initialized! \n");
        return SAL_FAIL;
    }

    g_stPpmCommPrm.uiChannelCnt = MAX_PPM_CHN_NUM;

    for (i = 0; i < g_stPpmCommPrm.uiChannelCnt; i++)
    {
        pstDevPrm = &g_stPpmCommPrm.stPpmDevPrm[i];

        uiSize = 1920 * 1080 * 4;
        for (j = 0; j < PPM_JPEG_CB_NUM; j++)
        {
            s32Ret = mem_hal_mmzAlloc(uiSize, "PPM", "ppm_pkg_jpeg", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstDevPrm->stPkgJpegInfo[j].pcJpegAddr);
            PPM_DRV_CHECK_PTR(pstDevPrm->stPkgJpegInfo[j].pcJpegAddr, err, "alloc mmz failed!");
            PPM_DRV_CHECK_RET(s32Ret, err, "alloc mmz failed!");

#if 0
            s32Ret = mem_hal_mmzAlloc(uiSize, "PPM", "ppm_xsi_jpeg", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr);
            PPM_DRV_CHECK_PTR(pstDevPrm->stXpkgJpegInfo[j].pcJpegAddr, err, "alloc mmz failed!");
#endif

            s32Ret = mem_hal_mmzAlloc(uiSize, "PPM", "ppm_face_jpeg", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstDevPrm->stFaceJpegInfo[j].pcJpegAddr);
            PPM_DRV_CHECK_PTR(pstDevPrm->stFaceJpegInfo[j].pcJpegAddr, err, "alloc mmz failed!");
            PPM_DRV_CHECK_RET(s32Ret, err, "alloc mmz failed!");
        }
#if 1
        for (j = 0; j < 2; j++)
        {
                uiW = PPM_RK_TRI_CAM_MIDDLE_CHN_WIDTH;
                uiH = PPM_RK_TRI_CAM_MIDDLE_CHN_HEIGHT;
                s32Ret = Ppm_DrvGetFrameMem(uiW, uiH, &pstDevPrm->stTmpFrame[j]);
                PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");
        }
#else
        for (j = 0; j < 2; j++)
        {
            if (0x00 == pstDevPrm->stTmpFrame[j].uiAppData)
            {
                (VOID)sys_hal_allocVideoFrameInfoSt(&pstDevPrm->stTmpFrame[j]);
                PPM_DRV_CHECK_PTR(pstDevPrm->stTmpFrame[j].uiAppData, err, "mem alloc failed!");
            }
        }
#endif
        /* jpeg 2 yuv相关参数初始化 */
        uiSize = 1920 * 1080;
        for (j = 0; j < PPM_MAX_CALIB_JPEG_NUM; j++)
        {
            pstDevPrm->stJdecPrm.enFaceProcMode[j] = PPM_JDEC_MODE_NUM;
            s32Ret = mem_hal_mmzAlloc(uiSize, "PPM", "ppm_calib_jpeg", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstDevPrm->stJdecPrm.stFaceSysFrame[j].pcJpegAddr);
            PPM_DRV_CHECK_PTR(&pstDevPrm->stJdecPrm.stFaceSysFrame[j], err, "alloc mmz failed!");
            PPM_DRV_CHECK_RET(s32Ret, err, "alloc mmz failed!");

            pstDevPrm->stJdecPrm.enTriProcMode[j] = PPM_JDEC_MODE_NUM;
            s32Ret = mem_hal_mmzAlloc(uiSize, "PPM", "ppm_calib_jpeg", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstDevPrm->stJdecPrm.stTriSysFrame[j].pcJpegAddr);
            PPM_DRV_CHECK_PTR(&pstDevPrm->stJdecPrm.stTriSysFrame[j], err, "alloc mmz failed!");
            PPM_DRV_CHECK_RET(s32Ret, err, "alloc mmz failed!");
        }

        /* jpeg 2 yuv相关参数初始化 */
        for (j = 0; j < PPM_MAX_FACE_CALIB_JPEG_NUM; j++)
        {
            pstDevPrm->stJdecPrm.enFaceCamProcMode[j] = PPM_JDEC_MODE_NUM;
            (VOID)sys_hal_allocVideoFrameInfoSt(&pstDevPrm->stJdecPrm.stFaceCamSysFrame[j]);
        }

        g_stPpmCommPrm.stPpmDevPrm[i].uiChn = i;
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function   Ppm_DrvGetDepthInfoFromQueue
 * @brief      从深度信息结果队列中获取结果
 * @param[in]  UINT32 chan                     
 * @param[in]  PPM_DEPTH_INFO_S *pstDepthInfo  
 * @param[out] None
 * @return     static INT32
 */
static INT32 ATTRIBUTE_UNUSED Ppm_DrvGetDepthInfoFromQueue(UINT32 chan, PPM_DEPTH_INFO_S *pstDepthInfo)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_DEPTH_INFO_QUE_PRM *pstDepthInfoQue = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    PPM_DEPTH_INFO_S *pstQueBuf = NULL;

    /* checker */
    PPM_DRV_CHECK_RET(NULL == pstDepthInfo, exit, "pstDepthInfo == null!");

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    /* get depth info queue */
    pstDepthInfoQue = &pstDevPrm->stDepthInfoQueInfo;
    
    pstFullQue = pstDepthInfoQue->stDepthQuePrm.pstFullQue;
    pstEmptQue = pstDepthInfoQue->stDepthQuePrm.pstEmptQue;
    if (NULL == pstFullQue || NULL == pstEmptQue)
    {
        PPM_LOGE("queue err! chan %d, full %p, empt %p \n", chan, pstFullQue, pstEmptQue);
        goto exit;
    }

    Ppm_DrvGetQueData(pstFullQue, (VOID **)&pstQueBuf, SAL_TIMEOUT_NONE);
    if (pstQueBuf)
    {
        /* get depth info from result queue */
        memcpy(pstDepthInfo, pstQueBuf, sizeof(PPM_DEPTH_INFO_S));
        Ppm_DrvPutQueData(pstEmptQue, pstQueBuf, SAL_TIMEOUT_NONE);
    }
    else
    {
        PPM_LOGW("depth info full queue empty! \n");
    }
    
    s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/**
 * @function   Ppm_DrvPutDepthInfoIntoQueue
 * @brief      将解封装获取到的深度结构化信息送入结果队列
 * @param[in]  UINT32 chan                     
 * @param[in]  PPM_DEPTH_INFO_S *pstDepthInfo  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_DrvPutDepthInfoIntoQueue(UINT32 chan, PPM_DEPTH_INFO_S *pstDepthInfo)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_DEV_PRM *pstDevPrm = NULL;
    PPM_DEPTH_INFO_QUE_PRM *pstDepthInfoQue = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    PPM_DEPTH_INFO_S *pstQueBuf = NULL;

    /* checker */
    PPM_DRV_CHECK_RET(NULL == pstDepthInfo, exit, "pstDepthInfo == null!");

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

    /* get depth info queue */
    pstDepthInfoQue = &pstDevPrm->stDepthInfoQueInfo;
    
    pstFullQue = pstDepthInfoQue->stDepthQuePrm.pstFullQue;
    pstEmptQue = pstDepthInfoQue->stDepthQuePrm.pstEmptQue;
    if (NULL == pstFullQue || NULL == pstEmptQue)
    {
        PPM_LOGE("queue err! chan %d, full %p, empt %p \n", chan, pstFullQue, pstEmptQue);
        goto exit;
    }

    /* put depth info into result queue */
    Ppm_DrvGetQueData(pstEmptQue, (VOID **)&pstQueBuf, SAL_TIMEOUT_NONE);

    if (pstQueBuf)
    {
        /* put depth info into result queue */
        memcpy(pstQueBuf, pstDepthInfo, sizeof(PPM_DEPTH_INFO_S));  
        Ppm_DrvPutQueData(pstFullQue, pstQueBuf, SAL_TIMEOUT_NONE);
    }
    else
    {
        PPM_LOGW("no free queue buf! \n");
    }
    
    s32Ret = SAL_SOK;
    
exit:
    return s32Ret;
}

/**
 * @function   Ppm_DrvGenerateDepthInfo
 * @brief      从json数据中格式化生成深度信息
 * @param[in]  VOID *pJsonData        
 * @param[in]  UINT32 u32DataLen  
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_DrvGenerateDepthInfo(VOID *pJsonData, UINT32 u32DataLen)
{
	INT32 s32Ret = SAL_FAIL;

	UINT32 i = 0;
	UINT32 j = 0;

	PPM_DEV_PRM *pstDevPrm = NULL;
    cJSON *pstRoot = NULL;
	cJSON *pstMetaData = NULL;
	cJSON *pstCameratePointData = NULL;
	cJSON *pstTargetData = NULL;
	cJSON *pstHeadPolyData = NULL;
	cJSON *pstHead2DRectData = NULL;
	cJSON *pstHeadCctData = NULL;
	cJSON *pstJointPolyData = NULL;
	cJSON *pstJointArrData = NULL;
	cJSON *pstJointData = NULL;
	cJSON *pstItem = NULL;

	PPM_DEPTH_INFO_S stDepthInfo = {0};

	if (NULL == pJsonData || 0 == u32DataLen)
	{
		PPM_LOGE("invalid prm! pJsonData %p, u32DataLen %d \n", pJsonData, u32DataLen);
		goto exit;
	}

	PPM_LOGD("============== GENErate depth info enter! \n");
	
	/* transfer to cjson format */
    pstRoot = cJSON_Parse(pJsonData);
    if (!pstRoot)
    {
        PPM_LOGE("Error before: [%s]\n", cJSON_GetErrorPtr());
        goto exit;
    }

	/* cJSON_Print中会对pstRoot的内存进行释放 */
	CHAR *pstOutPara = cJSON_Print(pstRoot);

#if 0  /* 调试接口，将负载数据写文件临时保留，后续删除 */
	{
		static FILE *fp = NULL;
		static int flag_cb = 1;

		if (flag_cb)
		{
			if (!fp)
			{
				fp = fopen("./depth_info_cb.json", "a+");
			}
			fwrite(pstOutPara, 1, strlen(pstOutPara), fp);
			fflush(fp);
			//fclose(fp);
		}

		if (flag_cb > 0)
		{
			//flag_cb--;
		}
		else
		{
			SAL_LOGE("++++++++++++++++++	dump rtp data end! \n");
		}		
		
		//RECODE_LOGW("no ppm proc cb func! \n");	
	}
#endif

    // debug, print as string via serial output 
    if (u32PrMetaJson & 0x1)
    {
		printf("get json data: \n");
		printf("%s \n\n\n", pstOutPara);
		printf("debug print json data end! \n");

		free(pstOutPara);
		pstOutPara = NULL;

		cJSON_Delete(pstRoot);
		pstRoot = NULL;
		return SAL_SOK;
    }

	free(pstOutPara);
	pstOutPara = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(0);
    PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

	/* 若人包模块未开启，不解析深度信息，直接返回成功 */
    if (SAL_TRUE != pstDevPrm->bChnOpen)
    {
		cJSON_Delete(pstRoot);
		pstRoot = NULL;

		return SAL_SOK;
	}

	/* metadata */
    pstMetaData = cJSON_GetObjectItem(pstRoot, "MetaData");
	PPM_DRV_CHECK_PTR(pstMetaData, exit, "get metadata section failed!");

	/* get timestamps */
	{
	    pstItem = cJSON_GetObjectItem(pstMetaData, "timeStamp");
		PPM_DRV_CHECK_PTR(pstItem, exit, "get timestamp failed!");

		stDepthInfo.u64Timestamp = (UINT64)pstItem->valuedouble;
		PPM_LOGD("get timestamp %llu \n", (UINT64)pstItem->valuedouble);
	}
	
    pstCameratePointData = cJSON_GetObjectItem(pstMetaData, "CameraPoint");
	PPM_DRV_CHECK_PTR(pstCameratePointData, exit, "get camerate point section failed!");

	/* number of detected obj */
	stDepthInfo.u32DetObjNum = cJSON_GetArraySize(pstCameratePointData);
	PPM_LOGD("get target array size %d \n", stDepthInfo.u32DetObjNum);

	for(i = 0; i < stDepthInfo.u32DetObjNum; i++)
	{
		pstTargetData = cJSON_GetArrayItem(pstCameratePointData, i);
		PPM_DRV_CHECK_PTR(pstTargetData, exit, "get target data section failed!");

		PPM_LOGD("--- get det obj %d \n", i);
		
		/* get target id */
		{
		    pstItem = cJSON_GetObjectItem(pstTargetData, "objId");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].u32Id = (UINT32)atoi(pstItem->valuestring);
			PPM_LOGD("get target id %d \n", stDepthInfo.astDetObjInfo[i].u32Id);
		}

		/* get target valid */
		{
		    pstItem = cJSON_GetObjectItem(pstTargetData, "isvalid");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].bValid = (UINT32)pstItem->valueint;
			PPM_LOGD("get target valid %d \n", pstItem->valueint);
		}

		/* get target track complete flag */
		{
		    pstItem = cJSON_GetObjectItem(pstTargetData, "traComplete");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].bDisapper = (UINT32)pstItem->valueint;
			PPM_LOGD("get target track complete %d \n", pstItem->valueint);
		}

		pstHeadPolyData = cJSON_GetObjectItem(pstTargetData, "HeadPolygon");
		PPM_DRV_CHECK_PTR(pstHeadPolyData, exit, "get head polygon failed!");	
		
		/* get head 2d rect */
		{
			pstHead2DRectData = cJSON_GetObjectItem(pstHeadPolyData, "Rect");
			PPM_DRV_CHECK_PTR(pstHead2DRectData, exit, "get head 2d rect failed!");

		    pstItem = cJSON_GetObjectItem(pstHead2DRectData, "x");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].stHeadRect.x = (float)pstItem->valuedouble;
			PPM_LOGD("get 2d head x %f \n", pstItem->valuedouble);
			
		    pstItem = cJSON_GetObjectItem(pstHead2DRectData, "y");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].stHeadRect.y = (float)pstItem->valuedouble;
			PPM_LOGD("get 2d head y %f \n", pstItem->valuedouble);

		    pstItem = cJSON_GetObjectItem(pstHead2DRectData, "width");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].stHeadRect.w = (float)pstItem->valuedouble;
			PPM_LOGD("get 2d head w %f \n", pstItem->valuedouble);

		    pstItem = cJSON_GetObjectItem(pstHead2DRectData, "height");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

			stDepthInfo.astDetObjInfo[i].stHeadRect.h = (float)pstItem->valuedouble;
			PPM_LOGD("get 2d head h %f \n", pstItem->valuedouble);
		}
		
		/* get target height */
		{
		    pstHeadCctData = cJSON_GetObjectItem(pstHeadPolyData, "cct");
			PPM_DRV_CHECK_PTR(pstHeadCctData, exit, "get head cct failed!");

		    pstItem = cJSON_GetObjectItem(pstHeadCctData, "z");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get head cct failed!");

			stDepthInfo.astDetObjInfo[i].fHeight = pstItem->valueint;
			PPM_LOGD("get head cct, z %d \n", pstItem->valueint);
		}
		
		/* get 7 joint info, including joint type, validation, score, 3d ordinated location */
		{
		    pstJointPolyData = cJSON_GetObjectItem(pstTargetData, "GesturePolygon");
			PPM_DRV_CHECK_PTR(pstJointPolyData, exit, "get joint polydata failed!");
			
		    pstItem = cJSON_GetObjectItem(pstJointPolyData, "id");
			PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");
			
			stDepthInfo.astDetObjInfo[i].u32JointId = (UINT32)atoi(pstItem->valuestring);
			PPM_LOGD("get joint id %d \n", stDepthInfo.astDetObjInfo[i].u32JointId);

		    pstJointArrData = cJSON_GetObjectItem(pstJointPolyData, "JoinMsg");
			PPM_DRV_CHECK_PTR(pstJointArrData, exit, "get joint array failed!");

			/* number of detected joints */
			stDepthInfo.astDetObjInfo[i].u32JointNum = cJSON_GetArraySize(pstJointArrData);
			PPM_LOGD("get joint array size %d \n", stDepthInfo.u32DetObjNum);

			for(j = 0; j < stDepthInfo.astDetObjInfo[i].u32JointNum; j++)
			{
				pstJointData = cJSON_GetArrayItem(pstJointArrData, j);
				PPM_DRV_CHECK_PTR(pstJointData, exit, "get joint data failed!");

				PPM_LOGD("get joint %d \n", j);

				pstItem = cJSON_GetObjectItem(pstJointData, "gestureType");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

				stDepthInfo.astDetObjInfo[i].astJointInfo[j].u32JointType = pstItem->valueint;
				PPM_LOGD("type %d \n", pstItem->valueint);

				pstItem = cJSON_GetObjectItem(pstJointData, "gestureScore");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

				stDepthInfo.astDetObjInfo[i].astJointInfo[j].fJointScore = (float)pstItem->valuedouble;
				PPM_LOGD("Score %f \n", pstItem->valuedouble);

				pstItem = cJSON_GetObjectItem(pstJointData, "gestureValid");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

				stDepthInfo.astDetObjInfo[i].astJointInfo[j].bJointValid = pstItem->valueint;
				PPM_LOGD("Valid %d \n", pstItem->valueint);

				pstItem = cJSON_GetObjectItem(pstJointData, "x");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

				stDepthInfo.astDetObjInfo[i].astJointInfo[j].st3dPoint.x = (float)pstItem->valuedouble;
				PPM_LOGD("x %f \n", pstItem->valuedouble);
				
				pstItem = cJSON_GetObjectItem(pstJointData, "y");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

				stDepthInfo.astDetObjInfo[i].astJointInfo[j].st3dPoint.y = (float)pstItem->valuedouble;
				PPM_LOGD("y %f \n", pstItem->valuedouble);
				
				pstItem = cJSON_GetObjectItem(pstJointData, "z");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");

				stDepthInfo.astDetObjInfo[i].astJointInfo[j].st3dPoint.z = (float)pstItem->valuedouble;
				PPM_LOGD("z %f \n", pstItem->valuedouble);

				pstItem = cJSON_GetObjectItem(pstJointData, "x2d");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");
				
				stDepthInfo.astDetObjInfo[i].astJointInfo[j].st2dPoint.x = (float)pstItem->valuedouble;
				PPM_LOGD("x2d %f \n", pstItem->valuedouble);
				
				pstItem = cJSON_GetObjectItem(pstJointData, "y2d");
				PPM_DRV_CHECK_PTR(pstItem, exit, "get item failed!");
				
				stDepthInfo.astDetObjInfo[i].astJointInfo[j].st2dPoint.y = (float)pstItem->valuedouble;
				PPM_LOGD("y2d %f \n", pstItem->valuedouble);
			}
		}
	}

	free(pstRoot);
	pstRoot = NULL;
	
	PPM_LOGD("+++++++++++++++++++++++++ GENErate depth info end! \n");

	/* put depth info into result queue, currently only support one channel, default use idx 0 */
	s32Ret = Ppm_DrvPutDepthInfoIntoQueue(0, &stDepthInfo);
	PPM_DRV_CHECK_RET(s32Ret, exit, "Ppm_DrvPutDepthInfoIntoQueue failed!");
		
exit:
	return s32Ret;
}


/**
 * @function:   Ppm_DrvDisableChannel
 * @brief:      模块通道关闭
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvDisableChannel(UINT32 chan)
{
    PPM_DEV_PRM *pstDevPrm = NULL;

    PPM_DRV_CHECK_RET(SAL_TRUE != g_stPpmCommPrm.bInit, err, "ppm drv has not finished inition!");

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    if (SAL_TRUE != pstDevPrm->bChnOpen)
    {
        PPM_LOGI("chan %d is disabled! return success! \n", chan);
        return SAL_SOK;
    }

    pstDevPrm->bChnOpen = SAL_FALSE;

    SAL_INFO("ppm chan %d deinit end! \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvEnableChannel
 * @brief:      模块通道开启
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvEnableChannel(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    PPM_DEV_PRM *pstDevPrm = NULL;

    PPM_DRV_CHECK_RET(SAL_TRUE != g_stPpmCommPrm.bInit, err, "ppm drv has not finished inition!");

    pstDevPrm = Ppm_DrvGetDevPrm(chan);
    PPM_DRV_CHECK_PTR(pstDevPrm, err, "pstDevPrm == null!");

    if (SAL_TRUE == pstDevPrm->bChnOpen)
    {
        PPM_LOGI("chan %d is enabled! return success! \n", chan);
        return SAL_SOK;
    }

    if (!pstDevPrm->bOutChnInit)
    {
        s32Ret = Ppm_DrvInitOutChnPrm();
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitOutChnPrm failed!");
    }

    /* 清空队列数据 */
    (VOID)Ppm_DrvClrQueData(&pstDevPrm->stFaceYuvQueInfo.stYuvQuePrm);
    (VOID)Ppm_DrvClrQueData(&pstDevPrm->stTriYuvQueInfo.stYuvQuePrm);
    /* (VOID)Ppm_DrvClrQueData(&pstDevPrm->stXsiYuvQueInfo.stYuvQuePrm); */

    pstDevPrm->bNeedSync = SAL_FALSE;
    pstDevPrm->bChnOpen = SAL_TRUE;

    /* 清空调试计数信息 */
    memset(&pstDevPrm->stDebugPrm, 0x00, sizeof(PPM_DEBUG_PRM_S));

    SAL_INFO("ppm chan %d init end! \n", chan);
    return SAL_SOK;

err:
    (VOID)Ppm_DrvDisableChannel(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvModuleDeinit
 * @brief:      模块资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvModuleDeinit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    PPM_MOD_S *pstModPrm = NULL;

    PPM_DRV_CHECK_RET(SAL_TRUE != g_stPpmCommPrm.bInit, err, "ppm drv has not finished inition!");

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, err, "pstModPrm == null!");

    if (!pstModPrm->uiInitFlag)
    {
        PPM_LOGI("ppm module is deinitialized! return success! \n");
        return SAL_SOK;
    }

    s32Ret = Ppm_HalDeinit();
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_HalInit failed!");

    uiPpmModuleInitFlag = SAL_FALSE;
    SAL_INFO("ppm module deinit end! \n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvSetProcModeThread
 * @brief:      配置模式切换
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
static void *Ppm_DrvSetModeInitThread()
{
    INT32 s32Ret = SAL_SOK;

    SAL_SET_THR_NAME();

    s32Ret = Ppm_HalInit();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Ppm_HalInit Failed! \n");
        goto EXIT;
    }

    PPM_LOGI("Ppm_HalInit End! \n");

EXIT:

    SAL_thrExit(NULL);
    return NULL;
}

/**
 * @function:   Ppm_DrvModeInitThread
 * @brief:      模块资源初始化线程
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_DrvModeInitThread()
{
    SAL_ThrHndl Handle;

    /* hcnn库中存在1M多的栈内存使用，故此处创建2M的线程栈用于模式切换 */
    if (SAL_SOK != SAL_thrCreate(&Handle, Ppm_DrvSetModeInitThread, SAL_THR_PRI_DEFAULT, 2 * 1024 * 1024, NULL))
    {
        SVA_LOGE("Create Engine Init Thread Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SAL_thrDetach(&Handle))
    {
        SVA_LOGE("pthread detach failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvModuleInit
 * @brief:      模块资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvModuleInit(VOID)
{
    INT32 s32Ret = SAL_SOK;
    INT32 i = 0;
    UINT32 uiTimeOut = 300;              /* 300 * 100ms = 30s */
    PPM_MOD_S *pstModPrm = NULL;
    VDEC_PRM stVdecCreatePrm = {0};
    PPM_DEV_PRM *pstDevPrm = NULL;

    PPM_DRV_CHECK_RET(SAL_TRUE != g_stPpmCommPrm.bInit, err, "ppm drv has not finished inition!");

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, err, "pstModPrm == null!");
	
    (void)Ppm_HalSetProcFlag(SAL_FALSE);

    if (pstModPrm->uiInitFlag)
    {
        PPM_LOGI("ppm module is initialized! return success! \n");
        return SAL_SOK;
    }

    if (uiPpmModuleInitFlag)
    {
        PPM_LOGI("second ppm module init! return success! initflag %d \n", pstModPrm->uiInitFlag);
        return SAL_SOK;
    }

    if (g_stPpmCommPrm.uiChannelCnt == 0)
    {
        SAL_WARN("drv layer global var is initialized! \n");
        return SAL_FAIL;
    }

    for (i = 0; i < g_stPpmCommPrm.uiChannelCnt; i++)
    {
        pstDevPrm = &g_stPpmCommPrm.stPpmDevPrm[i];
        if (SAL_FALSE == pstDevPrm->stJdecPrm.bDecChnStatus)
        {
            stVdecCreatePrm.decWidth = PPM_FACE_WIDTH;
            stVdecCreatePrm.decHeight = PPM_FACE_HEIGHT;
            stVdecCreatePrm.encType = MJPEG;
            s32Ret = Ppm_DrvCreateVdecChn(0, PPM_VDEC_CHN_CALIB_JDEC, &stVdecCreatePrm);
            PPM_DRV_CHECK_RET(s32Ret, err, "vdec_hal_VdecCreate failed!");
            pstDevPrm->stJdecPrm.uiJdecChn = PPM_VDEC_CHN_CALIB_JDEC;
            pstDevPrm->stJdecPrm.bDecChnStatus = SAL_TRUE;
        }
    }

    if (SAL_SOK != Ppm_DrvModeInitThread())
    {
        PPM_LOGE("Ppm_DrvModeInitThread failed! \n");
        return SAL_FAIL;
    }

    while (!Ppm_HalGetInitFlag() && uiTimeOut > 0)
    {
        usleep(100 * 1000);

        if (uiTimeOut > 0)
        {
            uiTimeOut--;
        }
    }

    /* s32Ret = Ppm_HalInit(); */
    /* PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_HalInit failed!"); */
    uiPpmModuleInitFlag = SAL_TRUE;

    SAL_INFO("ppm module init end! \n");
    return SAL_SOK;

err:
    (VOID)Ppm_DrvModuleDeinit();
    return SAL_FAIL;
}

/**
 * @function:   Ppm_DrvDeinit
 * @brief:      人包关联业务层去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvDeinit(VOID)
{
    if (SAL_TRUE != g_stPpmCommPrm.bInit)
    {
        SAL_WARN("%s is Deinited! return success! \n", __func__);
        return SAL_SOK;
    }

    (VOID)Ppm_HalDeinit();
    (VOID)Ppm_DrvDeinitQueData();
    (VOID)Ppm_DrvDeinitGlobalVar();

    g_stPpmCommPrm.bInit = SAL_FALSE;
    return SAL_SOK;
}

/**
 * @function:   Ppm_DrvGetMmzUsageInfo
 * @brief:      将各个步骤消耗的mmz内存信息打印出来
 * @param[in]:  CHAR *funcName
 * @param[out]: None
 * @return:     VOID
 */
static VOID Ppm_DrvGetMmzUsageInfo(CHAR *funcName)
{
#ifndef MEM_DEBUG
    return;
#endif

    printf("%s_mem_info: \n", funcName);
    system("cat /proc/media-mem |grep total");
    printf("\n");

    return;
}

/**
 * @function:   Ppm_DrvInit
 * @brief:      人包关联业务层初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_DrvInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    if (SAL_TRUE == g_stPpmCommPrm.bInit)
    {
        SAL_WARN("%s is inited! return success! \n", __func__);
        return SAL_SOK;
    }

    Ppm_DrvGetMmzUsageInfo("before Ppm_DrvInitGlobalVar! \n");

    s32Ret = Ppm_DrvInitGlobalVar();
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitGlobalVar failed!");

    Ppm_DrvGetMmzUsageInfo("after Ppm_DrvInitGlobalVar! \n");

    s32Ret = Ppm_DrvInitQueData();
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitQueData failed!");

    Ppm_DrvGetMmzUsageInfo("after Ppm_DrvInitQueData! \n");

    s32Ret = Ppm_HalInitGlobalVar();
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_HalInitGlobalVar failed!");

#if 0
    s32Ret = Ppm_HalInit();
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_HalInit failed!");
#endif

    Ppm_DrvGetMmzUsageInfo("after Ppm_HalInit! \n");

    s32Ret = Ppm_DrvInitEngInputPrm();
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitEngInputPrm failed!");

    Ppm_DrvGetMmzUsageInfo("after Ppm_DrvInitEngInputPrm! \n");

    s32Ret = Ppm_DrvInitThread(0);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvInitThread failed!");

    Ppm_DrvGetMmzUsageInfo("after Ppm_DrvInitThread! \n");

    g_stPpmCommPrm.bInit = SAL_TRUE;
    return SAL_SOK;

err:
    (VOID)Ppm_DrvDeinit();
    return SAL_FAIL;
}

/**
 * @function    Ppm_DrvPrintDbgPrm
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Ppm_DrvPrintDbgPrm(VOID)
{
    PPM_DEV_PRM *pstDevPrm = NULL;

    pstDevPrm = Ppm_DrvGetDevPrm(0);

    SAL_ERROR("DEBUG: input cnt %d, cb cnt %d \n",
              pstDevPrm->stDebugPrm.uiIptCnt, pstDevPrm->stDebugPrm.uiCbCnt);
    return;
}

#if 1 /* debug */

/* 调试接口，打印pos信息 */
VOID Ppm_DrvParseDepthPosInfo(MTME_OBJ_INPUT_INFO_T *pstMtmeInputObjInfo)
{
	UINT32 i = 0, j = 0;

	return;
	
	PPM_LOGW("============================= get mtme obj info! \n");
	
	for(i = 0; i < pstMtmeInputObjInfo->obj_num; i++)
	{		
		{
			PPM_LOGW("i %d, id %d, valid %d, disappear %d, height %f, head[%f, %f, %f, %f] \n", i,
					 pstMtmeInputObjInfo->obj_id[i], pstMtmeInputObjInfo->obj_valid[i], 
					 pstMtmeInputObjInfo->obj_disappear[i], pstMtmeInputObjInfo->obj_height[i],
					 pstMtmeInputObjInfo->obj_headrect[i].x, pstMtmeInputObjInfo->obj_headrect[i].y,
					 pstMtmeInputObjInfo->obj_headrect[i].width, pstMtmeInputObjInfo->obj_headrect[i].height);
		}
		
		// joints		
		{
			PPM_LOGW("joint id %d \n", pstMtmeInputObjInfo->obj_joinmsg[i].id);
		}
		
		for(j = 0; j < MTME_MAX_HTR_HSG_NUM; j++)
		{		
			if (pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].state)
			{
				PPM_LOGW("joints: type %d, score %f, state %d, 2d[%f, %f], 3d[%f, %f, %f] \n",
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].type,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].score,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].state,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.x,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.y,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.x,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.y,
						 pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_3d.z);
			}
		}

		PPM_LOGW("\n");
	}
}

static VOID Ppm_DrvDrawRectCPU(CHAR *pcFrmDataY, UINT32 uiFrmW, UINT32 uiFrmH,
                               UINT32 uiRectX, UINT32 uiRectY, UINT32 uiRectW, UINT32 uiRectH)
{
    UINT32 i = 0;

    CHAR *tmp1 = NULL;
    CHAR *tmp2 = NULL;

    if (uiRectX + uiRectW >= uiFrmW)
    {
        uiRectW = uiFrmW - 1 - uiRectX;
    }

    if (uiRectY + uiRectH >= uiFrmH)
    {
        uiRectH = uiFrmH - 1 - uiRectY;
    }

	PPM_LOGW("uiFrmW %d, uiFrmH %d, uiRectX %d, uiRectY %d, uiRectW %d, uiRectH %d \n",
		     uiFrmW, uiFrmH, uiRectX, uiRectY, uiRectW, uiRectH);

    tmp1 = pcFrmDataY + uiRectY * uiFrmW + uiRectX;
    tmp2 = pcFrmDataY + (uiRectY + uiRectH) * uiFrmW + uiRectX;

    for (i = 0; i < uiRectW; i++)
    {
        *tmp1 = *tmp2 = 0;
        tmp1++;
        tmp2++;
    }

    tmp1 = pcFrmDataY + uiRectY * uiFrmW + uiRectX;
    tmp2 = pcFrmDataY + uiRectY * uiFrmW + uiRectX + uiRectW;

    for (i = 0; i < uiRectH; i++)
    {
        *tmp1 = *tmp2 = 0;
        tmp1 += uiFrmW;
        tmp2 += uiFrmW;
    }

    return;
}

#define FLAOT2INT(f, s) ((UINT32)(f*s))

/* 调试接口，画头肩和关节点框 */
VOID Ppm_DrvDrawDepthBox(VOID *pData, MTME_OBJ_INPUT_INFO_T *pstMtmeInputObjInfo)
{
	int i = 0, j = 0;
	
	for (i = 0; i < pstMtmeInputObjInfo->obj_num; i++)
	{
		if (!pstMtmeInputObjInfo->obj_valid[i])
		{
			PPM_LOGE("obj not valid! i %d, obj_num %d \n", i, pstMtmeInputObjInfo->obj_num);
			continue;
		}
		
		// draw head rect
		Ppm_DrvDrawRectCPU((CHAR *)pData, 1920, 1080, 
		                   FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].x, 1920), 
		                   FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].y, 1080),
		                   FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].width, 1920), 
		                   FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].height, 1080));

		PPM_LOGI(" x %f, y %f, w %f, h %f, [%d, %d] [%d, %d] \n",
			     pstMtmeInputObjInfo->obj_headrect[i].x,
			     pstMtmeInputObjInfo->obj_headrect[i].y,
			     pstMtmeInputObjInfo->obj_headrect[i].width,
			     pstMtmeInputObjInfo->obj_headrect[i].height,
			     FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].x, 1920),
			     FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].y, 1080),
			     FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].width, 1920),
			     FLAOT2INT(pstMtmeInputObjInfo->obj_headrect[i].height, 1080));


		// draw joints rect
		for (j = 0; j < 7; j++)
		{
			Ppm_DrvDrawRectCPU((CHAR *)pData, 1920, 1080, 
			                   FLAOT2INT(pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.x, 1920), 
			                   FLAOT2INT(pstMtmeInputObjInfo->obj_joinmsg[i].joints[j].point_2d.y, 1080),
			                   30, 
			                   30);
		}
	}
	
	return;
}

/* 调试接口，用于解析通过H5导出的视频+深度信息素材 */
VOID Ppm_DrvParsePos(VOID)
{
	CHAR *path = "../master/pos/yuvpos_2.bin";
	CHAR *path1 = "../master/pos/yuvpos_2_output.bin";
	
	FILE *fp = NULL;
	FILE *fp_dst = NULL;

	UINT32 u32ReadSize = 0;
//	UINT32 u32YuvSize = 1920*1080*3;
	UINT32 u32SingleYuvSize = 1920*1080*3/2;

	VOID *pVir = NULL;
	PPM_DEV_PRM *pstDevPrm = NULL;
	MTME_OBJ_INPUT_INFO_T *pstMtmeInputObjInfo = NULL;

	pstDevPrm = Ppm_DrvGetDevPrm(0);
	PPM_DRV_CHECK_PTR(pstDevPrm, exit, "pstDevPrm == null!");

	pstMtmeInputObjInfo = &pstDevPrm->stMtmeInputObjInfo;

	fp = fopen(path, "rb");
	if (NULL == fp)
	{
		PPM_LOGE("fopen failed! %s \n", path);
		goto exit;
	}

	fp_dst = fopen(path1, "a+");
	if (NULL == fp_dst)
	{
		PPM_LOGE("fopen failed! %s \n", path1);
		goto exit;
	}

	PPM_LOGE("parse pos enter! \n");
	
	while(0 == fseek(fp, u32SingleYuvSize, SEEK_CUR))
	{
		if (!pVir)
		{
			pVir = SAL_memMalloc(u32SingleYuvSize, "PPM", "ppm_pos");
			if (!pVir)
			{
				PPM_LOGE("malloc failed! size %d \n", u32SingleYuvSize);
				goto exit;
			}
			
			memset(pVir, 0x00, u32SingleYuvSize);
		}

		// read tri yuv
		u32ReadSize = fread(pVir, 1, u32SingleYuvSize, fp);
		if (u32SingleYuvSize != u32ReadSize)
		{
			PPM_LOGE("fread failed! u32ReadSize %d \n", u32ReadSize);
			goto exit;
		}

		// read depth info
		u32ReadSize = fread(pstMtmeInputObjInfo, 1, sizeof(MTME_OBJ_INPUT_INFO_T), fp);
		if (sizeof(MTME_OBJ_INPUT_INFO_T) != u32ReadSize)
		{
			PPM_LOGE("fread failed! u32ReadSize %d \n", u32ReadSize);
			goto exit;
		}

		// print depth info
		Ppm_DrvParseDepthPosInfo(pstMtmeInputObjInfo);

		// draw box
		Ppm_DrvDrawDepthBox(pVir, pstMtmeInputObjInfo);

		// write file
		fwrite(pVir, u32SingleYuvSize, 1, fp_dst);
		fflush(fp_dst);
	}

exit:
	if (pVir)
	{
		SAL_memfree(pVir, "PPM", "ppm_pos");
		pVir = NULL;
	}

	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if (fp_dst)
	{
		fclose(fp_dst);
		fp_dst = NULL;
	}

	PPM_LOGE("parse pos end! \n");
	return;
}

VOID ppm_debug_dump_depth(VOID)
{
	INT32 s32Ret = SAL_FAIL;
	
	FILE *fp = NULL;
	char path[64] = {0};
	static UINT32 u32Cnt = 0;
	UINT32 u32Offset = 0;
	UINT32 u32ReadSize = 0;

	UINT64 u64PhyDbg = 0;
	VOID *pVirDbg = NULL;

	UINT64 u64SrcPhyDbg = 0;
	VOID *pSrcVirDbg = NULL;
	SYSTEM_FRAME_INFO stTmpSrcCpyFrm = {0};

	UINT32 u32SingleDataSize = 1920*1080*3 + sizeof(PPM_DEPTH_INFO_S);

	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	SYSTEM_FRAME_INFO stTmpCpyFrm = {0};

	s32Ret = mem_hal_mmzAlloc(u32SingleDataSize, "ppm", "debug", NULL, 0, &u64PhyDbg, (VOID **)&pVirDbg);
	PPM_DRV_CHECK_RET(s32Ret, exit1, "mmz alloc failed!");
	
	PPM_LOGW("++++++++++++ mmz alloc end! %llu, %p \n", u64PhyDbg, pVirDbg);

#if 1  /* open debug yuv 1080P, build frame as SYSTEM_FRAME_INFO */
	s32Ret = mem_hal_mmzAlloc(1920*1080*3/2, "ppm", "debug", NULL, 0, &u64SrcPhyDbg, (VOID **)&pSrcVirDbg);
	PPM_DRV_CHECK_RET(s32Ret, exit1, "mmz alloc failed!");

	PPM_LOGW("++++++++++++ mmz src alloc end! %llu, %p \n", u64SrcPhyDbg, pSrcVirDbg);

	fp = fopen("./yuv/0.yuv", "rb+");
	if (NULL == fp)
	{
		goto exit1;
	}
	
	u32ReadSize = fread(pSrcVirDbg, 1, 1920*1080*3/2, fp);
	if (1920*1080*3/2 != u32ReadSize)
	{
		PPM_LOGE("fread size err! %d \n", u32ReadSize);
		goto exit1;
	}
	
	fclose(fp);
	fp = NULL;

	(VOID)sys_hal_allocVideoFrameInfoSt(&stTmpSrcCpyFrm);

	stVideoFrmBuf.frameParam.width = 1920;
	stVideoFrmBuf.frameParam.height = 1080;
	stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = 1920;

	stVideoFrmBuf.phyAddr[0] = u64SrcPhyDbg;
	stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height;
	stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

	(VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, &stTmpSrcCpyFrm);
#endif

	memset(pVirDbg, 0x00, u32SingleDataSize);
	
	(VOID)sys_hal_allocVideoFrameInfoSt(&stTmpCpyFrm);

#if 1  /* fixme: 图像数据拷贝使用硬件接口替换，优化cpu处理耗时 */

#if 1			/* copy face yuv */
	stVideoFrmBuf.frameParam.width = 1920;
	stVideoFrmBuf.frameParam.height = 1080;
	stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = 1920;
	stVideoFrmBuf.phyAddr[0] = u64PhyDbg;
	stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height;
	stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

	(VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, &stTmpCpyFrm);

	s32Ret = Ia_TdeQuickCopy(&stTmpSrcCpyFrm, &stTmpCpyFrm, 0, 0, 1920, 1080, 0);
	PPM_DRV_CHECK_RET(s32Ret, exit, "tde copy failed!");
	
#else
	(VOID)sys_hal_getVideoFrameInfo(&pstIptData->stFaceYuv, &stVideoFrmBuf);
	memcpy((char *)pDumpData + u32Offset, (VOID *)stVideoFrmBuf.virAddr[0], u32FaceSize);
#endif
	u32Offset += 1920*1080*3/2;

#if 1			/* copy tri yuv */
	stVideoFrmBuf.frameParam.width = 1920;
	stVideoFrmBuf.frameParam.height = 1080;
	stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = 1920;
	stVideoFrmBuf.phyAddr[0] = u64PhyDbg + 1920*1080*3/2;
	stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height;
	stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

	(VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, &stTmpCpyFrm);

	s32Ret = Ia_TdeQuickCopy(&stTmpSrcCpyFrm, &stTmpCpyFrm, 0, 0, 1920, 1080, 0);	
	PPM_DRV_CHECK_RET(s32Ret, exit, "tde copy failed!");
	
#else
	(VOID)sys_hal_getVideoFrameInfo(&pstIptData->stTriViewYuv[PPM_TRI_CAM_MIDDLE_CHN], &stVideoFrmBuf);
	memcpy((char *)pDumpData + u32Offset, (VOID *)stVideoFrmBuf.virAddr[0], u32TriSize);	
#endif
	u32Offset += 1920*1080*3/2;	

#if 1
	sprintf(path, "./%d_face_1920x1080_tri_1920x1080_depth.dump", u32Cnt);
	fp = fopen(path, "wb+");
	if (NULL == fp)
	{
		goto exit;
	}

	fwrite(pVirDbg, u32Offset, 1, fp);
	fflush(fp);

	fclose(fp);
	fp = NULL;

	PPM_LOGW("==== dump %s end! \n", path);

	/* 重置，并开始下一轮loop写入 */
	u32Offset = 0;
	u32Cnt++;
#endif

#endif 

exit:
	(VOID)sys_hal_rleaseVideoFrameInfoSt(&stTmpCpyFrm);

exit1:
	if (fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if (NULL != pVirDbg)
	{
		(VOID)mem_hal_mmzFree(pVirDbg, "ppm", "debug");
	}

	if (NULL != pSrcVirDbg)
	{
		(VOID)mem_hal_mmzFree(pSrcVirDbg, "ppm", "debug");
	}	
}

/**
 * @function:   test_dump_proc_frame
 * @brief:      调试接口: 获取各通道帧
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 test_dump_proc_frame(VOID)
{
#if 1  /* 获取三目-左目通道帧 */
    UINT32 uiSize = 768 * 432 * 3 / 2;
    char path[64] = "./test_left_768x432.nv21";

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[0], &stVideoFrmBuf);

    (VOID)Ppm_HalDebugDumpData((VOID *)stVideoFrmBuf.virAddr[0], path, uiSize, 0);
    SAL_WARN("fwrite end! \n");
#endif


#if 1  /* 获取三目-右目通道帧 */
    uiSize = 768 * 432 * 3 / 2;
    sprintf(path, "./test_right_768x432.nv21");

    (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[1], &stVideoFrmBuf);

    (VOID)Ppm_HalDebugDumpData((VOID *)stVideoFrmBuf.virAddr[0], path, uiSize, 0);
    SAL_WARN("fwrite end! \n");
#endif


#if 1  /* 获取三目-中路视角通道帧 */
    uiSize = 1280 * 720 * 3 / 2;
    sprintf(path, "./test_mid_1280x720.nv21");

    (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[2], &stVideoFrmBuf);

    (VOID)Ppm_HalDebugDumpData((VOID *)stVideoFrmBuf.virAddr[0], path, uiSize, 0);
    SAL_WARN("fwrite end! \n");
#endif

    return SAL_SOK;
}

/**
 * @function:   test_get_vdec_frame
 * @brief:      调试接口: 获取各通道帧
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 test_get_vdec_frame(VOID)
{
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

#if 1  /* 获取三目-左目通道帧 */
    if (SAL_SOK != Ppm_DrvGetVdecFrm(0, PPM_TRI_CAM_LEFT_CHN + 4, &g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[0], PPM_TRI_CAM_LEFT_CHN_WIDTH, PPM_TRI_CAM_LEFT_CHN_HEIGHT))
    {
        SAL_ERROR("ppm: get vdec frm failed! \n");
        return SAL_FAIL;
    }

    SAL_WARN("get vdec frm end! 0 \n");

    UINT32 uiSize = 768 * 432 * 3 / 2;
    char path[64] = "./test_left_768x432.nv21";

    (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[0], &stVideoFrmBuf);

    (VOID)Ppm_HalDebugDumpData((VOID *)stVideoFrmBuf.virAddr[0], path, uiSize, 0);
    SAL_WARN("fwrite end! \n");
#endif


#if 1  /* 获取三目-右目通道帧 */
    if (SAL_SOK != Ppm_DrvGetVdecFrm(0, PPM_TRI_CAM_RIGHT_CHN + 4, &g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[1], PPM_TRI_CAM_RIGHT_CHN_WIDTH, PPM_TRI_CAM_RIGHT_CHN_HEIGHT))
    {
        SAL_ERROR("ppm: get vdec frm failed! \n");
        return SAL_FAIL;
    }

    SAL_WARN("get vdec frm end! 1 \n");

    uiSize = 768 * 432 * 3 / 2;
    sprintf(path, "./test_right_768x432.nv21");

    (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[1], &stVideoFrmBuf);

    (VOID)Ppm_HalDebugDumpData((VOID *)stVideoFrmBuf.virAddr[0], path, uiSize, 0);
    SAL_WARN("fwrite end! \n");
#endif


#if 1  /* 获取三目-中路视角通道帧 */
    if (SAL_SOK != Ppm_DrvGetVdecFrm(0, PPM_TRI_CAM_MIDDLE_CHN + 4, &g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[2], PPM_TRI_CAM_MIDDLE_CHN_WIDTH, PPM_TRI_CAM_MIDDLE_CHN_HEIGHT))
    {
        SAL_ERROR("ppm: get vdec frm failed! \n");
        return SAL_FAIL;
    }

    SAL_WARN("get vdec frm end! 2 \n");

    uiSize = 1280 * 720 * 3 / 2;
    sprintf(path, "./test_mid_1280x720.nv21");

    (VOID)sys_hal_getVideoFrameInfo(&g_stPpmCommPrm.stPpmDevPrm[0].stVdecFrame[2], &stVideoFrmBuf);

    (VOID)Ppm_HalDebugDumpData((VOID *)stVideoFrmBuf.virAddr[0], path, uiSize, 0);
    SAL_WARN("fwrite end! \n");
#endif

    return SAL_SOK;
}

/**
 * @function:   input_data
 * @brief:      输入demo输入数据(来源于引擎)
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 input_data(VOID)
{
#if 0
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    INT32 input_idx = 0;
    INT32 frameNum = 0;
    INT32 loopNum = 0;
    INT32 img_size = 0;
    INT32 useFlag = 1;
    /* INT32 buf_size = 0; */
    ICF_INPUT_DATA engInput[INPUT_BUF_NUM] = {0};
    UINT32 uiResoTab[DMEO_SRC_DIM][2] =
    {
        {DEMO_LEFT_IMG_WIDTH, DEMO_LEFT_IMG_HEIGHT},
        {DEMO_RIGHT_IMG_WIDTH, DEMO_RIGHT_IMG_HEIGHT},
        {DEMO_SRC_WIDTH, DEMO_SRC_HEIGHT},
        {DEMO_FACE_WIDTH, DEMO_FACE_HEIGHT},
        {DEMO_SRC_WIDTH, DEMO_SRC_HEIGHT}
    };

    FILE *fp_l_yuv_in = NULL;
    FILE *fp_r_yuv_in = NULL;
    FILE *fp_m_yuv_in = NULL;
    FILE *fp_face_yuv_in = NULL;
    FILE *fp_xsi_yuv_in = NULL;
    VOID *p_src_img = NULL;
    DEMO_MEM_TAB demo_mem_tab = {0};
    PPM_SUB_MOD_S *pstSubModPrm = NULL;
    PPM_MOD_S *pstModPrm = NULL;
    UINT64 u64PhyAddr = 0;
    VOID *pVirAddr = NULL;

    img_size = DEMO_FACE_HEIGHT * DEMO_FACE_WIDTH * 3 / 2 * DMEO_SRC_DIM;    /* 按照最大内存开辟 */

    demo_mem_tab.size = img_size;
    demo_mem_tab.alignment = DEMO_MEM_ALIGN_128BYTE;
    demo_mem_tab.space = DEMO_HISI_MEM_MALLOC; /* DEMO_HISI_MEM_MMZ_WITH_CACHE; // hisi平台内存类型 */
    printf("====================== hisi plat\n");

    /* nRet = alloc_memory(&demo_mem_tab); */
    /* ICF_CHECK_ERROR(nRet != 0, nRet); */

    if (SAL_SOK != mem_hal_mmzAlloc(img_size, "PPM", "input_mem", NULL, SAL_TRUE, &u64PhyAddr, (void **)&pVirAddr))
    {
        PPM_LOGE("mmz alloc cached failed! \n");
        return SAL_FAIL;
    }

    demo_mem_tab.base = pVirAddr; /* malloc(img_size); */

    p_src_img = (void *)demo_mem_tab.base;

    /* thread_param->blob_num = DMEO_SRC_DIM; */
    /* 初始化engInput结构体 */
    for (i = 0; i < INPUT_BUF_NUM; i++)
    {
        j = 0;
        engInput[i].nBlobNum = DMEO_SRC_DIM;
        for (j = 0; j < DMEO_SRC_DIM; j++)
        {
            /* 原图 */
            engInput[i].stBlobData[j].nShape[0] = uiResoTab[j][0];
            engInput[i].stBlobData[j].nShape[1] = uiResoTab[j][1];
            engInput[i].stBlobData[j].eBlobFormat = ICF_INPUT_FORMAT_YUV_NV21;
            engInput[i].stBlobData[j].nFrameNum = 0;
            engInput[i].stBlobData[j].nSpace = ICF_HISI_MEM_MMZ_WITH_CACHE;
            engInput[i].stBlobData[j].pData = p_src_img;
            p_src_img = p_src_img + (uiResoTab[j][0] * uiResoTab[j][1]) * 3 / 2;

            engInput[i].pUseFlag[j] = SAL_memMalloc(sizeof(int), "PPM", "ppm_input");
            memset(engInput[i].pUseFlag[j], 0x00, sizeof(int));
        }
    }

    /* 打开文件，获取各个图像源，加偏置 */
    fp_l_yuv_in = fopen(DEMO_LEFT_VIEW_PATH, "rb");
    PPM_DRV_CHECK_PTR(fp_l_yuv_in, err, "fp_l_yuv_in == null!");

    fp_r_yuv_in = fopen(DEMO_RIGHT_VIEW_PATH, "rb");
    PPM_DRV_CHECK_PTR(fp_r_yuv_in, err, "fp_r_yuv_in == null!");

    fp_m_yuv_in = fopen(DEMO_MIDDLE_VIEW_PATH, "rb");
    PPM_DRV_CHECK_PTR(fp_m_yuv_in, err, "fp_m_yuv_in == null!");

    fp_face_yuv_in = fopen(DEMO_FACE_VIEW_PATH, "rb");
    PPM_DRV_CHECK_PTR(fp_face_yuv_in, err, "fp_face_yuv_in == null!");

    fp_xsi_yuv_in = fopen(DEMO_X_DATA_VIEW_PATH, "rb");
    PPM_DRV_CHECK_PTR(fp_xsi_yuv_in, err, "fp_xsi_yuv_in == null!");

    pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_MATCH);
    PPM_DRV_CHECK_PTR(pstSubModPrm, err, "pSubModHandle == null!");

    pstModPrm = Ppm_HalGetModPrm();
    PPM_DRV_CHECK_PTR(pstModPrm, err, "pstModPrm == null!");

    while (loopNum < DEMO_FRAME_NUM)
    {
        /* while(1) */
        /* 准备数据 */

        memcpy(engInput[input_idx].pUseFlag[0], &useFlag, sizeof(INT32));
        s32Ret = fread(engInput[input_idx].stBlobData[0].pData, DEMO_LEFT_IMG_HEIGHT * DEMO_LEFT_IMG_WIDTH, 1, fp_l_yuv_in);
        PPM_DRV_CHECK_RET(s32Ret <= 0, err, "fread failed 0!");

        memcpy(engInput[input_idx].pUseFlag[1], &useFlag, sizeof(INT32));
        s32Ret = fread(engInput[input_idx].stBlobData[1].pData, DEMO_RIGHT_IMG_HEIGHT * DEMO_RIGHT_IMG_WIDTH, 1, fp_r_yuv_in);
        PPM_DRV_CHECK_RET(s32Ret <= 0, err, "fread failed 1!");

        memcpy(engInput[input_idx].pUseFlag[2], &useFlag, sizeof(INT32));
        s32Ret = fread(engInput[input_idx].stBlobData[2].pData, DEMO_SRC_HEIGHT * DEMO_SRC_WIDTH * 3 / 2, 1, fp_m_yuv_in);
        PPM_DRV_CHECK_RET(s32Ret <= 0, err, "fread failed 2!");

        memcpy(engInput[input_idx].pUseFlag[3], &useFlag, sizeof(INT32));
        s32Ret = fread(engInput[input_idx].stBlobData[3].pData, DEMO_FACE_WIDTH * DEMO_FACE_HEIGHT * 3 / 2, 1, fp_face_yuv_in);
        PPM_DRV_CHECK_RET(s32Ret <= 0, err, "fread failed 3!");

        memcpy(engInput[input_idx].pUseFlag[4], &useFlag, sizeof(INT32));
        s32Ret = fread(engInput[input_idx].stBlobData[4].pData, DEMO_SRC_HEIGHT * DEMO_SRC_WIDTH * 3 / 2, 1, fp_xsi_yuv_in);
        PPM_DRV_CHECK_RET(s32Ret <= 0, err, "fread failed 4!");

        /* *engInput[input_idx].pUseFlag[0] = 1;// 如果不置位会有奇怪的错误，比如段错误，cannot find graph 1 */
        engInput[input_idx].stBlobData[0].nFrameNum = frameNum;
        engInput[input_idx].stBlobData[0].nTimeStamp = frameNum * 80;
        engInput[input_idx].stBlobData[1].nFrameNum = frameNum;
        engInput[input_idx].stBlobData[1].nTimeStamp = frameNum * 80;
        engInput[input_idx].stBlobData[2].nFrameNum = frameNum;
        engInput[input_idx].stBlobData[2].nTimeStamp = frameNum * 80;
        engInput[input_idx].stBlobData[3].nFrameNum = frameNum;
        engInput[input_idx].stBlobData[3].nTimeStamp = frameNum * 80;
        engInput[input_idx].stBlobData[4].nFrameNum = frameNum;
        engInput[input_idx].stBlobData[4].nTimeStamp = frameNum * 80;

        printf("=========================================== input, framenum %d\n", frameNum);

        s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfInputData(pstSubModPrm->pHandle, &engInput[input_idx], sizeof(ICF_INPUT_DATA));
        if (SAL_SOK != s32Ret)
        {
            printf("cannot input data!!! return 0x%x\n", s32Ret);
            goto err;
        }

        /* input_idx = (input_idx+1)%INPUT_BUF_NUM; */

        /* 每两帧处理一次 */
        /* frameNum += 2; */
        loopNum++;
        frameNum++;

        usleep(SLEEPTIME_US);
        /* usleep(SLEEPTIME_US * 4); */
    }

err:
    if (NULL != fp_l_yuv_in)
    {
        fclose(fp_l_yuv_in);
        fp_l_yuv_in = NULL;
    }

    if (NULL != fp_r_yuv_in)
    {
        fclose(fp_r_yuv_in);
        fp_r_yuv_in = NULL;
    }

    if (NULL != fp_m_yuv_in)
    {
        fclose(fp_m_yuv_in);
        fp_m_yuv_in = NULL;
    }

    if (NULL != fp_face_yuv_in)
    {
        fclose(fp_face_yuv_in);
        fp_face_yuv_in = NULL;
    }

    if (NULL != fp_xsi_yuv_in)
    {
        fclose(fp_xsi_yuv_in);
        fp_xsi_yuv_in = NULL;
    }

    /* 释放buffer内存 */
    /* nRet = free_memory(&demo_mem_tab); */
    /* ICF_CHECK_ERROR(nRet != 0, nRet); */
    (VOID)mem_hal_mmzFree((void *)pVirAddr, "PPM", "input_mem");
#endif
    return SAL_SOK;
}

/* debug: 验证解码2560x1440, 可行 */

/**
 * @function:   test_app
 * @brief:      调试接口入口
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 test_app(VOID)
{
    INT32 s32Ret = SAL_SOK;

    static BOOL bFirstFlag = SAL_TRUE;

    if (bFirstFlag)
    {
        s32Ret = Ppm_DrvInit();
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("ppm drv init end! \n");
            return SAL_FAIL;
        }

        bFirstFlag = SAL_FALSE;
    }

#ifdef TEST_PROC_FRM
    if (g_stPpmCommPrm.stPpmDevPrm[0].stVdecWholeFrame.uiAppData == 0x00)
    {
        s32Ret = Ppm_DrvGetFrameMem(2560, 1440, &g_stPpmCommPrm.stPpmDevPrm[0].stVdecWholeFrame);
        PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetFrameMem failed!");

        SAL_WARN("get whole frame buf end! w %d, h %d \n", 2560, 1440);
    }

    if (SAL_SOK != Ppm_DrvGetVdecFrm(0, 2, &g_stPpmCommPrm.stPpmDevPrm[0].stVdecWholeFrame, 2560, 1440))
    {
        SAL_ERROR("ppm: get vdec frm failed! \n");
        return SAL_FAIL;
    }

    SAL_ERROR("get vdec frame end! \n");

#if 0
    SAL_ERROR("bofore proc frame! \n");
    s32Ret = Ppm_DrvGetSplitFrame(0, &g_stPpmCommPrm.pstPpmDevPrm[0]->stVdecWholeFrame);
    PPM_DRV_CHECK_RET(s32Ret, err, "Ppm_DrvGetSplitFrame failed!");
#endif

    /* (VOID)test_dump_proc_frame(); */
    SAL_ERROR("dump data end! \n");

    return SAL_SOK;
#endif

#ifdef TEST_VDEC_FRM
    (VOID)test_get_vdec_frame();
#endif

    /* return SAL_SOK; */

#ifdef TEST_ENG_COHEN
    (VOID)input_data();
#endif

    return SAL_SOK;

#ifdef TEST_PROC_FRM
err:
    return SAL_FAIL;
#endif
}

#endif

