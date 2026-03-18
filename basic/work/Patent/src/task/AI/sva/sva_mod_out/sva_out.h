/*******************************************************************************
* sva_out.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2020年6月8日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef __SVA_OUT_H__
#define __SVA_OUT_H__


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "platform_hal.h"

#ifndef SVA_NO_USE
#include "ia_common.h"
#endif

#include "xtrans_sys.h"
/* #include "xtrans_macro.h" */
#include "xtrans_device.h"
#include "xtrans_buf_proc.h"
#include "xtrans_port.h"
#include "xtrans_config.h"
#include "xtrans_mem.h"

#include "xtrans_link_dev.h"
#include "xtrans_msg.h"
#include "xtrans_thr.h"
#include "xtrans_mgr.h"
#include "xtrans_msgq.h"
#include "xtrans_mail.h"
#include "xtrans_list.h"

#include "xtrans_bussiness_dev.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define SVA_POST_MSG_LEN (3196)

/* #define SVA_INPUT_GAP_NUM               (2)//(5)       // 采集数据间隔帧数，如采集60fps，间隔2帧送入xsi */

#define SVA_OFFSET_BUF_LEN		(12)
#define SVA_PKG_FWD_BUF_NUM		(256)
#define SVA_MAX_PACKAGE_BUF_NUM (16)       /* 单画面最多缓存的包裹个数 */
#define SVA_PACKAGE_MAX_NUM  (16)          /* 单帧包裹分割最大个数 */
#define SVA_DET_PACKAGE_MAX_NUM  (32)      /* 单帧待检测包裹最大个数 */
#define SVA_NO_SUB_TYPE_IDX (0)            /* 没有子类 */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
#if 0
/* 锁 */
typedef struct tagSvaDrvMutexSt
{
    pthread_mutex_t lock;
} SVA_DRV_MUTEX_ST;
#endif

/*引擎功能类参数*/
typedef struct tagSvaAlertInPrm
{
	unsigned int bInit;                                     /* 应用层是否初始化，代表是否启用 */

    unsigned int sva_type;                                  /* 危险品类型 */

    unsigned int sva_key;                                   /* 检测开关 */

    unsigned int sva_color;                                 /* 画框颜色,也可以在显示部分做。 */

    unsigned char sva_name[SVA_ALERT_NAME_LEN];             /* 危险物名称 */

    unsigned int sva_alert_tar_cnt;                         /* 告警危险品个数阈值 */

} SVA_ALERT_IN_PRM;

typedef struct  tagSvaTargetPrm
{
    SVA_ALERT_EXT_TYPE enOsdExtType;                        /* OSD拓展信息展示类型 */
    unsigned int scaleLevel;                                /* OSD字体大小缩放等级(1-4,0为默认等级:1) */
    unsigned int confidence;
} SVA_TARGET_PRM;

typedef struct  tagSvaAltPkgPrm
{
    UINT32 uiColor;                                         /* 包裹危险级别标记颜色 */
    INT8 cPkgLevelStr[SVA_PKG_LEVEL_NAME_LEN];              /* 包裹级别名称 */
} SVA_ALERT_PKG_IN_PRM;

/*引擎功能类参数*/
typedef struct tagSvaProcessIn
{
    /* 算法需要每帧更新的参数 */
    SVA_RECT_F rect;                                        /* 主视角检测区域 */

    SVA_ALERT_IN_PRM alert[SVA_MAX_ALARM_TYPE];             /* 危险品类型，包括xsi模型(0~63)和ai模型(64~95) */

    SVA_ALERT_LEVEL_E enAlertLevel[SVA_MAX_ALARM_TYPE];     /* 危险级别(成都定制) */

    SVA_ALERT_PKG_IN_PRM stPkgPrm[SVA_ALERT_LEVEL_NUM];     /* 包裹参数(成都定制) */

    SVA_ORIENTATION_TYPE enDirection;                       /* 传送带方向 */

    UINT32 sva_name_cnt;                                    /* 危险物名称改变计数,每改变一次，值改变 */

    UINT32 sva_color_cnt;                                   /* 危险物颜色改变计数,每改变一次，值改变 */

    float fScale;                                           /* 缩放因子, unused */

    UINT32 uiPkgSensity;                                    /* 包裹分割灵敏度(1~10)，默认值4 */

    UINT32 uiPkgSplitFilter;                                /* 包裹分割过滤阈值，默认值50 */

    float fZoomInOutVal;                                    /* 放大缩小配置参数，默认值0.01，越大检准，越小高检出，建议根据场景+-0.01微调 */

    UINT32 uiPkSwitch;                                      /* PK模式是否打开，0表示正常模式灵敏度包括置信度使用算法内置参数，1表示支持外部基于算法结果进行自定义过滤 */

    UINT32 uiSprayFilterSwitch;                             /* 喷灌过滤开关，通过目标色域检测基于原先喷灌结果进行二次过滤，降低喷灌误报率 */

    UINT32 uiGapUpFlag;                                     /* 包裹左右预留像素更新标志 */
    UINT32 uiGapPixel;                                      /* 包裹左右预留像素大小 */

    SVA_TARGET_PRM stTargetPrm;                             /* 危险物目标相关配置参数，应用触发动态更新 */

    SVA_BORDER_TYPE_E drawType;                             /* 叠框类型，0 osd画在目标框上, 1 osd画在上下通过线和目标框连接 */

    AI_PACK_PRM stAiPrm[2];                                 /*AI 透传数据,用于后续包裹数据绑定*/

} SVA_PROCESS_IN;

/* 算法POS信息结构体 */
typedef struct tagSvaPostInfo
{
    unsigned char xsi_pos_buf[SVA_POST_MSG_LEN];            /* pos信息指针 */
    unsigned int xsi_pos_size;                              /* pos信息结构体大小 */
} SVA_POS_INFO;

/* XSI报警目标 */
typedef struct tagSvaAlert
{
    unsigned int ID;                                        /* ID */

    unsigned int type;                                      /* 目标类型 */

	unsigned int merge_type;                                /* 目标大类合并后的标识类别，用于显示模块包裹合并展示 */

	UINT32 u32SubTypeIdx;                                   /* 大类中细分子类的索引值，用于回溯区分与OSD叠加小数位展示。R8维护新增违禁品大类合并需求 */

    unsigned int alarm_flg;                                 /* 目标的报警标志位，TRUE为报警，FALSE为不报警 */

    unsigned int color;                                     /* 画框颜色,也可以在显示部分做。 */

    UINT32 confidence;                                      /* 目标置信度 (0~1000) */

    UINT32 visual_confidence;                               /* 视觉置信度，用于客户端显示置信度。目前初定 visual_confidence = confidence/2.0+0.50.   (0-1000) */

    SVA_RECT_F rect;                                        /* 目标框 */

    BOOL bOsdShow;                                          /* OSD是否需要展示 */
    
    BOOL bRectShow;                                         /* 目标框是否叠加显示 */

	/* 是否为双视角包裹匹配合并展示目标 */
	BOOL bAnotherViewTar;

    UINT32 uiPkgId;                                         /* 对应的包裹ID */

    unsigned char sva_name[SVA_ALERT_NAME_LEN];             /* 危险物名称 */

    float offset;                                           /* 偏移量 */

    UINT32 cnt;                                             /*当前type有多少个*/
} SVA_TARGET;
typedef struct _SVA_XSIE_PACKAGE_T_
{    
    unsigned int                        PakageID;                                   // 包裹ID
    unsigned int                        PackLifeFrame;                              // 包裹从开始到当前帧所经历的帧数
    SVA_RECT_F                          PackageRect;                                // 包裹目标框
    float                               PackageForwardLocat;                        // 包裹前沿位置输出
    BOOL                                PackageValid;                               // 包裹是否有效，1：包裹已经分割完成，有效----0：包裹未分割完成，无效
	
	/* 注意: 历史包裹标记在包裹分割结果出来之后，包裹在画面整个过程中都是1 */
    BOOL                                IsHistoryPackage;                           // 该位置包裹是否成为了历史包裹，1：是，该包裹已成为历史包裹，该位置可被复用---
                                                                                    // -0：否，仍是新包裹，需要继续维护
}SVA_XSIE_PACKAGE_T;

typedef struct tagSvaInsEncRslt
{
    UINT32 uiInsEncFlag;                                    /* 即刻送入编图的标志 */
    SVA_RECT_F stInsPkgRect;                                /* 即时编图包裹位置信息 */

    BOOL bRemainValid;
    SVA_RECT_F stCutRemainPkgRect;                          /* pkgValid出现的那一帧剩余未裁剪的包裹部分 */
} SAV_INS_ENC_RSLT_S;

typedef struct _SVA_SLICE_OUT_
{
	/* 分片是否为包裹开始 */
	BOOL bStartFlag;
	/* 分片是否为包裹结束 */
	BOOL bEndFlag;
	/* 分片所属包裹ID */
	UINT32 u32PkgId;
	/* 当前包裹是否做完OBD */
	UINT32 u32PkgValid;

	/* 包裹前沿X位置开始计算，即时编图包裹位置信息，用于计算分片内的目标坐标映射 */
    SVA_RECT_F stCutTotalPkgRect;                           
	/* 分片处理结果 */
	SAV_INS_ENC_RSLT_S stSliceInsRsltBuf;
} SVA_SLICE_OUT_S;

typedef struct _SVA_PERSION_PKG_MATCH_PRM_
{
	/* 包裹前沿 */
	float fPkgfwd;
	/* 绝对时间 */
    DATE_TIME absTime;
} SVA_PERSION_PKG_MATCH_PRM_S;

typedef struct _SVA_PKG_FALSE_DET_RLST_
{
	/* 误检包裹个数 */
	UINT32 u32FalseDetPkgCnt;
	/* 误检包裹id，数组当前深度为16 */
	UINT32 au32FalseDetPkgId[SVA_MAX_PACKAGE_BUF_NUM];
} SVA_PKG_FALSE_DET_RLST_S;

/* XSI包裹报警信息 */
typedef struct  tagSvaPackAlert
{
    unsigned int package_valid;                              /* 包裹是否有效，1：有效，0：无效 */

	unsigned int package_id;                                 /* 当前pkg_valid的包裹id */

	unsigned int package_match_index;                        /* 双视角包裹匹配索引 */

    unsigned int up_pkg_idx;                                 /* 待更新的包裹缓存索引 */

    unsigned int candidate_flag;                             /*  记录当前帧是否可能是候选正样本（一般可能存在刀枪等罕见样本）,用于样本回收。 */

    float fPkgFwdLoc;                                        /* 包裹前边沿坐标 */

    float fRealPkgY;                                         /* 包裹真实上边沿，用于整包裁剪 */

    float fRealPkgH;                                         /* 包裹真实高度 */

    BOOL bSplitStart;                                        /* 是否为分片序列的开始 */

    BOOL bSplitEnd;                                          /* 当前valid是否为分片序列的结束 */

	unsigned int u32SplitPackageId;                          /* 包裹分割完成时的包裹id */

    SVA_RECT_F package_loc;                                  /* 包裹位置 */

	/* 误检包裹结果信息，当前集中判图要求将误检包裹回调用于清空客户端多余误检分片 */
	SVA_PKG_FALSE_DET_RLST_S stFalseDetPkgRslt;

    unsigned int         MainViewPackageNum;                   /* 包裹分割结果 */

    SVA_XSIE_PACKAGE_T   MainViewPackageLoc[SVA_PACKAGE_MAX_NUM];   /* 主路包裹信息 */

	unsigned int		 OriginalPackageNum;				   /* 引擎回调的原始包裹个数 */

	SVA_XSIE_PACKAGE_T	 OriginalPackageLoc[SVA_PACKAGE_MAX_NUM];	/* 引擎回调的原始包裹信息 */

	/* 包裹前沿X位置开始计算，即时编图包裹位置信息，用于计算分片内的目标坐标映射，非集中判图不生效 */
    SVA_RECT_F stCutTotalPkgRect;                           

	/* 分片组缓存参数，单帧最大支持16个包裹同时进行集中判图分片处理 */
	UINT32 u32SliceInsRsltNum;
	SVA_SLICE_OUT_S astSliceInsRslt[SVA_PACKAGE_MAX_NUM];

	/* 人包精准关联参数: 包裹出现及分包完成的前沿+绝对时间信息 */
    SVA_PERSION_PKG_MATCH_PRM_S stPkgStartInfo;
    SVA_PERSION_PKG_MATCH_PRM_S stPkgEndInfo;
} SVA_PACK_ALERT;

typedef struct _SVA_PKG_FWD_CORCT_INFO_
{
    BOOL bPkgFwdNeedCorct;                                  /* 包裹前沿需要更新的标志 */

    UINT32 uiFrmNum;                                        /* 帧号 */
    float fSavedPkgFwd;                                     /* 用于矫正包裹前沿的基础前沿信息 */
} SVA_PKG_FWD_CORCT_INFO_S;

typedef struct _SVA_RSLT_BUF_INFO_
{
    float fLastFwdXBuf;                                     /* 缓存上一帧包裹前沿X位置，只在新的包裹出现时使用，用于过滤跳变的包裹前沿坐标 */

    UINT32 uiFwdBufIdx;                                     /* 缓存历史包裹写入的当前索引，向前寻找第一个非跳变值 */
    UINT32 u32FrmNum[SVA_PKG_FWD_BUF_NUM];                  /* 缓存历史帧号，用于后续包裹前沿坐标匹配计算 */
    float fLastOffsetBuf[SVA_PKG_FWD_BUF_NUM];              /* 缓存历史包裹前沿X位置，只在新的包裹出现时使用，用于过滤跳变的包裹前沿坐标 */
} SVA_RSLT_BUF_INFO_S;

typedef struct tagSvaInsEncPrm
{
    /* V1 */
    BOOL bCurPkgEnd;                                        /* 是否以获取到当前包裹信息 */
    UINT32 uiCalFrmNum;                                     /* 计算是否需要截图的帧数统计, 该数值与采集帧率相关 */
    float fStartCutGap;                                     /* 裁图起始的宽度(浮点) */
    float fCutWidth;                                        /* 截图宽度(浮点) */
    float fStepCutGap;                                      /* 总共需要截图的宽度 */

    /* V2 */
    float fPkgFwdX;                                         /* 包裹前沿X位置，相对完整图片坐标 */
    UINT32 uiExistCnt;                                      /* unused, 记录连续出现正常的次数 */
    float fCutTotalW;                                       /* 距离当前包裹前沿裁剪的总宽度(浮点) */
    BOOL bNewPkg;                                           /* 新包裹出现标志位 */
    BOOL bPkgProcFlag;                                      /* 新包裹出现后该包裹处理周期标志 */
    BOOL bForceSplitProcFlag;                               /* 强制包裹分割处理标志，1代表当前处在强制分割包裹状态中，下一个valid为强制分割剩下的部分 */
    BOOL bLastPkgValid;                                     /* 上一个包裹有效位 */
    float bLastPkgRealFwd;                                  /* 上一帧实际的包裹前沿位置 */
    SVA_RECT_F stCutTotalPkgRect;                           /* 包裹前沿X位置开始计算，即时编图包裹位置信息 */

    SVA_PKG_FWD_CORCT_INFO_S stPkgFwdCorctInfo;             /* 包裹前沿更新信息 */
    SVA_RSLT_BUF_INFO_S stRsltBufInfo;                      /* 结果缓存信息 */
    SAV_INS_ENC_RSLT_S stInsEncRslt;                        /* 即刻编图的目标相关结果 */
} SAV_INS_ENC_PRM_S;

#if 0
typedef struct _SVA_PKG_BUF_
{
    UINT32 uiRIdx;                                          /* 包裹缓存读索引 */
    UINT32 uiWIdx;                                          /* 包裹缓存写索引 */
    UINT32 uiStartUpFlag;                                   /* 是否开始更新坐标标记 */
    UINT32 uiGapPixel[SVA_MAX_PACKAGE_BUF_NUM];             /* 包裹左右边沿的留白像素 */
    UINT32 uiTarCnt[SVA_MAX_PACKAGE_BUF_NUM];               /* 包裹内目标个数 */
    SVA_RECT_F stPkgLocInfo[SVA_MAX_PACKAGE_BUF_NUM];       /* 包裹缓存信息，当前最多有16个 */
} SVA_PKG_BUF_S;
#endif

typedef struct tagSvaPkgTarCnt
{
    unsigned int uiTarCnt[SVA_MAX_PACKAGE_BUF_NUM][SVA_MAX_ALARM_TYPE];              /* 危险品出现次数 */    
} SVA_PKG_TAR_CNT_S;


/* 
   智能处理输出信息
   注1: 当前该结构体作为对外传输使用的结构体，无需通信传输的结构不可以放在此处。尤其是中间变量需要在外部进行处理
   注2: xtrans组件目前针对智能结果传输使用的共享内存，当该结构体数据较大时传输帧率无法满足75fps，当前结构体大小:[27232]Bytes
*/
typedef struct tagSvaProcessOut
{
    unsigned int frame_num;                                 /* 帧号 */

    unsigned int reaultCnt;                                 /* 结果计数 */

    unsigned int uiTimeRef;                                 /* 帧序号 */

    unsigned int xsi_out_type;                              /* 关联模式下返回的数据是主视角0，还是侧视角1，后续删除 */

    float frame_offset;                                     /* 帧间隔，送入智能前后两帧偏移量 */

    float single_frame_offset;                              /* 帧间隔，显示模块用于坐标偏移，单位为1 timeRef */

    BOOL bPkgForceSplit;                                    /* 当前包裹是否为强制分割 */

    BOOL bZoomValid;                                        /* 当前画面是否存在放大缩小 */

    BOOL bFastMovValid;                                     /* 当前画面是否存在快速移动，回拉or前拉 */

    BOOL bSplitMode;                                        /* 集中判图模式是否开启 */

    SVA_PACK_ALERT packbagAlert;                            /* 包裹报警信息 */

    unsigned int target_num;                                /* 主视角报警数 */

#if 0
  /* 该结构体在智能模块作为中间过滤目标个数处理，不作为传输给外部模块的数据，独立成SVA_PKG_TAR_CNT_S */
    unsigned int uiTarCnt[SVA_MAX_PACKAGE_BUF_NUM][SVA_MAX_ALARM_TYPE];              /* 危险品出现次数 */
#endif

    SVA_TARGET target[SVA_XSI_MAX_ALARM_NUM];               /* 报警目标 */

#ifdef DSP_ISM
   /* szl_dbg: 临时注释，安检机使用的raw_target注释掉 */
    SVA_TARGET raw_target[SVA_XSI_MAX_ALARM_NUM];           /*报警目标，相对于原始包裹数据的坐标框*/
#endif

    SAV_INS_ENC_RSLT_S stInsEncRslt;                        /* 即刻编图的目标相关结果 */

    unsigned int frame_sync_idx;                            /* 同步帧索引值 */

    UINT64 frame_stamp;                                     /* 时间戳 */

    SVA_POS_INFO pos_info;                                  /* pos信息 */

    UINT32 uiOffsetX;                                       /* X偏移量 */

    UINT32 uiImgW;                                          /* 有效图像宽度 */ /*yyz: 安检机里的是分片宽度 */

    UINT32 uiImgH;                                          /* 有效图像高度 */ /*yyz: 6550安检机里即为640 */

    UINT32 uiSourceImgW;                                    /* 原始图像宽度 */

    UINT32 uiSourceImgH;                                    /* 原始图像高度 */

    AI_PACK_PRM aipackinfo;                                 /* 智能透传结构体*/

    /* SVA_DRV_MUTEX_ST    stOutLock;                          //锁 */
} SVA_PROCESS_OUT;

typedef struct tagSvaAiYuvPackPrm
{
    UINT32 padding_top;          /* 显示图像顶部padding高 */
    UINT32 padding_bottom;       /* 显示图像底部padding高 */
    ROTATE_MODE rotate;          /* 显示图像相对RAW的旋转角度 */
    MIRROR_MODE mirror;          /* 显示图像相对RAW的镜像类型 */
} SVA_YUV_PACK_PRM;

typedef struct tagSvaAiYuvSrcPrm
{
    SVA_YUV_PACK_PRM aiyuvpack;      /* 智能使用的yuv信息*/
    AI_PACK_PRM aipackinfo;          /* 智能透传结构体*/
} SVA_YUV_SRC_PRM;

/*智能安检机数据输入结构体- 安检机使用*/
typedef struct tagSvaPackInData
{
    SVA_YUV_SRC_PRM uiSvaPackPrm;               /*传入AI 的数据透传数据结构体*/
    SYSTEM_FRAME_INFO *uiSvaFrame;              /*智能AI yuv 帧数据*/

} SVA_PACKAGE_DATE_IN;


/*智能安检机输出结果结构体- 安检机使用*/
typedef struct tagSvaProcessOutData
{
    AI_PACK_PRM uiSvaPackPrm;             /* 输出的透传数据*/
    SVA_PROCESS_OUT uiSvaProcessRes;      /* 检测结果输出*/
} SVA_PROCESS_OUT_DATA;


typedef enum cfgJsonFile
{
    ALG_CFG_PATH = 0,                   /* AlgCfgPath */
    TASK_CFG_PATH = 1,                  /* TaskCfgPath */
    TOOLKIT_CFG_PATH = 2,               /* ToolkitCfgPath */
    APPPARAM_CFG_PATH = 3,              /* AppParamCfgPath */

} SVA_CFG_JSONFILE_E;

/* 用于主从芯片通信，返回智能结果 */
typedef struct tagSvaSyncResult
{
    UINT32 chan;
    SVA_PROC_MODE_E enProcMode;
    SVA_PROCESS_OUT stProcOut;
} SVA_SYNC_RESULT_S;

typedef struct tagSvaOffsetPrm
{
    UINT32 uiTimeRef;                 /* 帧序号 */
    float fOffset;                    /* 帧间隔 */
    float fStdValue;                  /* 当前的偏移经验值，统计得出 */
} SVA_OFFSET_PRM_S;

typedef struct tagSvaOffsetBufTab
{
    UINT32 uiIdx;                                        /* index of current buf table */
    UINT32 uiMaxLen;                                     /* max length of buf table */
    SVA_OFFSET_PRM_S stOffsetPrm[SVA_OFFSET_BUF_LEN];    /* offset prm of buf table */
} SVA_OFFSET_BUF_TAB_S;

/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaOut
* 描  述  : 获取当前智能检测结果
* 输  入  : - chan: 通道号
*         : - *pOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetSvaOut(UINT32 chan, SVA_PROCESS_OUT *pOut);

/**
 * @function:   Sva_DrvPrClassMap
 * @brief:      打印大类映射表
 * @param[in]:  None
 * @param[out]: None
 * @return:     INT32
 */
VOID Sva_DrvPrClassMap(VOID);

/*******************************************************************************
* 函数名  : Sva_DrvGetCfgParam
* 描  述  : 获取外部配置的智能模块参数
* 输  入  : - chan: 通道号
*         : - *pOut: 外部配置的智能参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetCfgParam(UINT32 chan, SVA_PROCESS_IN *pOut);

VOID Sva_HalPrIptUseFlag(VOID);

/*******************************************************************************
* 函数名  : Sva_drvGetResult
* 描  述  : 获取智能处理结果
* 输  入  : - chan    通道号
* 输  出  : - pResult 智能处理结果
* 返回值  : 成功:SAL_SOK
*         : 失败:SAL_FAIL
*******************************************************************************/
INT32 Sva_drvGetResult(UINT32 chan, void *pResult, UINT32 *pFlag, UINT32 *pAiTimeRef);

#ifndef SVA_NO_USE

/**
 * @function:   Sva_HalSetCfgPrm
 * @brief:      设置安检智能配置参数
 * @param[in]:  cJSON *pstItem
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_HalSetCfgPrm(cJSON *pstItem);

#endif

/*******************************************************************************
* 函数名  : Sva_HalUpDateSvaResult
* 描  述  : 更新算法结果
* 输  入  : - pstSrc: 拷贝源变量
*         : - pstDst: 拷贝目的变量
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalCpySvaResult(SVA_PROCESS_OUT *pstSrc, SVA_PROCESS_OUT *pstDst);

/*******************************************************************************
* 函数名  : Sva_HalClearSvaOut
* 描  述  : 清空智能结果信息
* 输  入  : - chan : 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalClearSvaOut(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_HalJpegDrawAiInfo
* 描  述  : 将智能信息画在yuv上
* 输  入  : - chan        : 通道号
*         : - pstJpegFrame: jpeg帧数据
*         : - pstOut      : 智能结果信息
*         : - uiWith      : 宽度
*         : - uiHeight    : 高度
*         : - uiModule    : 模块
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalJpegDrawAiInfo(UINT32 chan,
                            SYSTEM_FRAME_INFO *pstJpegFrame,
                            SVA_PROCESS_IN *pstIn,
                            SVA_PROCESS_OUT *pstOut,
                            UINT32 uiWith,
                            UINT32 uiHeight,
                            UINT32 uiModule);

/*******************************************************************************
* 函数名  : Sva_HalSetAlgDbgFlag
* 描  述  : 设置算法调试开关
* 输  入  : UINT32 : uiFlag
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
void Sva_HalSetAlgDbgFlag(UINT32 uiFlag);

/*******************************************************************************
* 函数名  : Sva_HalSetAlgDbgLevel
* 描  述  : 设置算法调试等级
* 输  入  : UINT32 : uiDbgLevel
* 输  出  : 无
* 返回值  : 成功: SAL_SOK
            失败: SAL_FAIL
*******************************************************************************/
void Sva_HalSetAlgDbgLevel(UINT32 uiDbgLevel);

/*******************************************************************************
* 函数名:   Sva_HalGetFromPoolWithPts
* 描  述   :   根据PTS从缓存池获取智能结果信息
* 输  入   :   - chan     : 通道号
*          :   - pRidx: 读索引指针
*          :   - u64Pts: 用于匹配的PTS
*          :   - pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*                      SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_HalGetFromPoolWithPts(UINT32 chan, UINT32 *pRidx, UINT64 u64Pts, SVA_PROCESS_OUT *pstSvaOut);

/*******************************************************************************
* 函数名  : Sva_HalGetViSrcRate
* 描  述  : 获取VI的输入帧率
* 输  入  : 无
* 输  出  : 无
* 返回值  : vi帧率
*******************************************************************************/
INT32 Sva_HalGetViSrcRate(VOID);

/*******************************************************************************
* 函数名  : Sva_HalSetDbgCnt
* 描  述  : 设置框滞留时间(算法内部使用)
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
VOID Sva_HalSetDbgCnt(UINT32 dbgCnt);

/**
 * @function:   Sva_DrvSetJpegPosSwitch
 * @brief:      设置Jpeg图片增加Pos信息开关
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetJpegPosSwitch(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaFromPool
* 描  述  : 获取当前智能检测结果
* 输  入  : - chan: 通道号
*                : - uiTimeRef: 搜索时用于匹配的帧号
*                : - *pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetSvaFromPool(UINT32 chan, UINT32 uiTimeRef, SVA_PROCESS_OUT *pstSvaOut);

/*******************************************************************************
* 函数名  : Sva_DrvSetDbLevel
* 描  述  : 设置日志打印级别
* 输  入  : - level: 日志级别
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_DrvSetDbLevel(UINT32 level);

VOID Sva_DrvTransSetPrtFlag(BOOL bFlag);

/*******************************************************************************
* 函数名  : Sva_DrvDbgSaveJpg
* 描  述  : 设置保存图片信息开关
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_DrvDbgSaveJpg(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_DrvSetDCtestLevel
* 描  述  : 设置双芯片调试开关
* 输  入  : uiFlag         : 调试总开关
*         : uiYuvFlag      : 导出yuv调试开关
*         : uiInputFrmRate : 数据输入帧率
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_DrvSetDCtestLevel(UINT32 uiFlag, UINT32 uiYuvFlag, UINT32 uiInputFrmRate);

/*******************************************************************************
* 函数名  : Sva_DrvGetDCtestLevel
* 描  述  : 获取双芯片调试开关
* 输  入  : idx : 开关索引值
* 输  出  : 无
* 返回值  : 索引值对应的开关状态
*******************************************************************************/
UINT32 Sva_DrvGetDCtestLevel(UINT32 idx);

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaOffsetBufTab
* 描  述  : 获取通道全局变量
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 成功: 返回偏移量的缓存表指针
*           失败: 返回NULL
*******************************************************************************/
SVA_OFFSET_BUF_TAB_S *Sva_DrvGetSvaOffsetBufTab(UINT32 chan);

/**
 * @function:   Sva_DrvClearXpackRslt
 * @brief:      外部模块清空全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvClearXpackRslt(UINT32 chan);

UINT32 Sva_DrvGetInputGapNum(void);
VOID Sva_DrvSetSplitDbgSwitch(UINT32 u32Flag);
VOID Sva_HalSetDumpYuvCnt(UINT32 chan, UINT32 cnt, UINT32 mode);

/*******************************************************************************
* 函数名  : Sva_HalGetIcfParaJson
* 描  述  : 获取相应的配置文件路径
* 输  入  : - cfg_num: 文件
* 输  出  : 无
* 返回值  : 对应的json文件路径
*******************************************************************************/
CHAR *Sva_HalGetIcfParaJson(SVA_CFG_JSONFILE_E cfg_num);

/*******************************************************************************
* 函数名  : Sva_tskSetMirrorChnPrm
* 描  述  : 设置镜像通道参数
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_tskSetMirrorChnPrm(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_tskSetScaleChnPrm
* 描  述  : 设置缩放通道参数
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_tskSetScaleChnPrm(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_tskDevInit
* 描  述  : 任务设备初始化
* 输  入  : - chan: 通道
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_tskDevInit(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_tskInit
* 描  述  : 模块tsk层初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_tskInit(void);

#if 1

/*******************************************************************************
* 函数名  : CmdProc_svaCmdProc
* 描  述  : 应用层命令字交互接口
* 输  入  : cmd : 数据分发句柄
*         : chan: 通道号
*         : prm : 参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 CmdProc_svaCmdProc(HOST_CMD cmd, UINT32 chan, void *prm);

INT32 Sva_DrvDelPackResFromEngine(UINT32 chan);

INT32 Sva_DrvGetPackResFromEngine(UINT32 chan, SVA_PROCESS_OUT_DATA *pstSvaProcessDataToXpack);

INT32 Sva_DrvSendAiDataToEngine(UINT32 chan, SVA_PACKAGE_DATE_IN *pstSvaAiData);

SYSTEM_FRAME_INFO *Sva_DrvGetSvaNodeFrame(UINT32 chan);

INT32 Sva_DrvGetRunState(UINT32 chan,UINT32 *pChnStatus, UINT32 *pUiSwitch);

/**
 * @function   Sva_DrvSetDualPkgMatchThresh
 * @brief      设置包裹匹配阈值
 * @param[in]  float fMatchThresh  
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvSetDualPkgMatchThresh(float fMatchThresh);

/**
 * @function   Sva_DrvChgPosWriteFlag
 * @brief      设置保存POS信息开关
 * @param[in]  uiFlag: 开关
 * @param[out] None
 * @return     INT32
 */
VOID Sva_DrvChgPosWriteFlag(UINT32 uiFlag);

/**
 * @function   Sva_DrvPrintDpmDebugSts
 * @brief      打印包裹匹配模块内部调试信息
 * @param[in]  None
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvPrintDpmDebugSts(VOID);

/**
 * @function    Sva_HalSetDumpYuvFlag
 * @brief       设置yuv的dump开关
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_HalSetDumpYuvFlag(BOOL bFlag);

VOID Sva_DrvGetQueDbgInfo(void);

void debug_dump_set_flag(int flag);

VOID Sva_DrvSetPrOutInfoFlag(UINT32 u32Flag);

/*******************************************************************************
* 函数名  : Sva_DrvGetSyncMainChan
* 描  述  : 设置同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  : 
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
UINT32 Sva_DrvGetSyncMainChan();

/*******************************************************************************
* 函数名  : Sva_DrvGetAlgMode
* 描  述  : 设置同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  : 
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
UINT32 Sva_DrvGetAlgMode();
#endif

#endif

