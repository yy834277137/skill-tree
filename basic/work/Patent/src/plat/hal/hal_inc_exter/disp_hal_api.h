/*******************************************************************************
* disp_hal_api.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年1月23日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _DISP_HAL_DRV_H_
#define _DISP_HAL_DRV_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "sal.h"
#include "dspcommon.h"
#include "vgs_hal_api.h"
#include "bt_timing.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define DISP_MEM_NAME "disp"

#define DISP_HAL_MAX_DEV_NUM (4)

#define DISP_HAL_MAX_CHN_NUM (16)

#define DISP_HAL_XDATA_CNT (10)     /* 智能信息最大保存个数 */

#define DISP_OSD_SHOW_NUM            (6)
#define DISP_OSD_UPPER_AREA_RATIO    (13)
#define DISP_OSD_DOWN_AREA_RATIO     (87)
#define MAX_ARTICLE_TYPE    (3)         /* 物品种类类型智能 TIP 有机物 */

#define DISP_DEBUG_CHN_TIME_NUM (100) /* 显示通道调试时间信息*/

/* 通道属性<旋转、镜像、翻转、裁剪等等 > */
typedef struct tagDispChnPrm
{
    UINT32 uiRotate;                                /* 旋转 0(原始),1(旋转90°),2(旋转180°),3(旋转270°) */
    UINT32 uiMirror;                                /* 镜像 */
    UINT32 uiFlip;                                  /* 翻转 */
    float uiRatio;                                  /* 视频层分辨率和设备层分辨率比例 */

    UINT32 uiCrop;                                  /* 裁剪开关 */
    UINT32 uiCropX;                                 /* 裁剪x坐标 */
    UINT32 uiCropY;                                 /* 裁剪y坐标 */
    UINT32 uiCropW;                                 /* 裁剪宽 */
    UINT32 uiCropH;                                 /* 裁剪高 */
} DISP_CHN_PRM;

/* 显示输入源属性 */
typedef struct tagDispChnIn
{
    UINT32 uiUseFlag;                               /* 使能标识 */

    UINT32 uiModId;                                 /* 输入模块模块名字 */

    UINT32 uiChn;                                   /* 输入模块通道号 */

    PhysAddr uiHandle;                              /* 输出模块信息，DUP模块时是DUP输出handle，VDEC时是解码通道 */

    UINT32 uiAiFlag;                                /* 是否叠加智能信息，无叠加信息的可以忽略此参数 */
    UINT32 g_AiDispSwitch;                          /*安检机显示智能开关*/

    UINT32 uiAiFlagDisappear;                       /* 是否需要隐藏智能信息*/
    UINT32 uitipFlagDisappear;                      /* 是否需要隐藏TIP信息*/
    UINT32 uiorgnaciFlagDisappear;                  /* 是否需要隐藏有机物信息*/

    UINT32 frameNum;                                /* 上次帧序号*/

    UINT32 bClearOsdBuf;                            /* 清空OSD缓存标志 */
} DISP_CHN_IN_COMMON;

typedef struct tagDispEnlarge
{
    FLOAT32 globalratio;                           /* 全局放大比例*/
    UINT32 defaultenlargesign;                    /* 默认全局放大标志, 1显示智能画框 0不显示智能画框 */
    UINT32 globalchn;                              /* 全局放大通道号*/
    INT32 partchn;                                 /*局部放大窗口号*/
    UINT32 uiDisappear;                            /* 全局放大或者局部放大是否显示智能信息 1不显示 0显示*/
    UINT32 uiCropX;                                /* 裁剪x坐标 */
    UINT32 uiCropY;                                /* 裁剪y坐标 */
    UINT32 uiCropW;                                /* 裁剪宽 */
    UINT32 uiCropH;                                /* 裁剪高 */
} DISP_CHN_ENLARGE_PRM;

/*
    其他危险品数据保存
 */
typedef struct tagDispOrgSvaPrm
{
   UINT32  orgNamecnt;//有机物名字修改次数   
   UINT32  orgScaleLevel;//有机物缩放等级
   UINT32  orgBackColor; //有机物背景色
   UINT32  orgDrawType;//有机物画线类型   
   UINT8   orgname[SVA_ALERT_NAME_LEN];//有机物名称
   UINT32  orgfGAlpha;
   UINT32  orgBgAlpha;
   UINT32  orgnumfGAlpha;
   UINT32  orgnumBgAlpha;
   UINT32  unpenNamecnt;//难穿透名字修改次数   
   UINT32  unpenScaleLevel;//难穿透缩放等级
   UINT32  unpenBackColor; //难穿透背景色
   UINT32  unpenDrawType;//难穿透画线类型   
   UINT8   unpenname[SVA_ALERT_NAME_LEN];//难穿透名称
   UINT32  unpenfGAlpha;
   UINT32  unpenBgAlpha;
   UINT32  unpennumfGAlpha;
   UINT32  unpennumBgAlpha;
   UINT32  tipNamecnt;//tip名字修改次数
   UINT32  tipScaleLevel;//tip缩放等级
   UINT32  tipBackColor; //tip背景色
   UINT32  tipDrawType;//tip画线类型
   UINT8   tipname[SVA_ALERT_NAME_LEN];//tip名称
   UINT32  tipfGAlpha;
   UINT32  tipBgAlpha;
   UINT32  tipnumfGAlpha;
   UINT32  tipnumBgAlpha;
}DISP_ORG_SVAPRM;

typedef enum tagDispClenStatus
{
    DISP_CLEAN_STATUS_START = 1,                    /* 开始清屏 */
    DISP_CLEAN_STATUS_CLEANING,                     /* 正在清屏 */
    DISP_CLEAN_STATUS_IDLE,                         /* 空闲状态 */
    DISP_CLEAN_STATUS_BUTT,
} DISP_CLEAN_STATUS_E;

/*
    显示相关配置，可作为平台特殊设置接口方便添加
 */
typedef enum
{
    DISP_CFG_NONE = 0x0,
    DISP_SCALING_ALGORITHM_CFG = 0x01,     /*rk平台设置缩放算法*/

    DISP_CFG_BUTT,
} DISP_CFG_TYPE_E;

typedef struct
{
    UINT64 u64TimeEntry;
    UINT64 u64TimePrcStart;
    UINT32 uiUseFlag;
    UINT32 uiModId;
    UINT64 u64TimeGetStart;
    UINT64 u64TimeGetEnd;
    UINT64 u64TimeGetTimeOut;
    UINT64 u64TimeSendStart;
    UINT64 u64TimeSendEnd;
    UINT64 u64TimeExit;
} DISP_DEBUG_CHN_TIME;

/* 显示通道属性 */
typedef struct tagDispChn
{
    UINT32 uiEnable;                                /* 通道使能开关 */
    UINT32 srcStatus;                               /* 通道数据源状态用于判断是否获取用户图片 */

    UINT32 uiChn;                                   /* 通道号 */

    UINT32 uiLayerNo;                               /* 视频层编号，一般与设备号相同 */

    UINT32 uiDevNo;                                 /* 设备号 */

    UINT32 uiNoSignal;                              /* 发送无视频信号图片标志，产品不需要此项，可以忽略 */

    UINT32 flag;                                    /* 线程同步使用 */
    UINT32 uiX;                                     /* 坐标X */
    UINT32 uiY;                                     /* 坐标Y */
    UINT32 uiW;                                     /* 通道长 */
    UINT32 uiH;                                     /* 通道宽 */
    UINT32 uiFps;                                   /* 帧率 */
    UINT32 uiColor;                                 /* 底色 */
    UINT32 uiLayer;                                 /* Android专用 */
    UINT32 u32Priority;
    UINT32 uiScaleAlgo;                             /* 仅RK平台有效，1表示新算法，0表示旧算法*/
    unsigned int bDispBorder;                       /* 是否显示边框*/
    unsigned int BorDerColor;                       /* 边框颜色 */
    unsigned int BorDerLineW;                       /* 边框线框 */
    UINT32 useMode;                                 /* 使用模式0 放大镜，1缩略图*/
    DISP_CLEAN_STATUS_E enCleanStatus;              /* 清屏状态 */
    DISP_CHN_ENLARGE_PRM enlargeprm;                /* 放大参数*/
    DISP_ORG_SVAPRM orgPrm;                              /*TIP / 有机物 信息配置*/
    /* 此处根据需求可以增加旋转镜像等属性变量 */
    DISP_CHN_PRM stChnPrm;

    DISP_CHN_IN_COMMON stInModule;                  /* 通道输入模块 */
    DISPFLICKER_PRM aiflicker[MAX_ARTICLE_TYPE];                      /* 通道智能信息闪烁*/
    VGS_ARTICLE_LINE_TYPE articlelinetype;                        /*通道框画线类型*/
    SAL_ThrHndl stChnThrHandl;                      /* 通道线程 */

    void *mChnMutexHdl;                             /* 通道信号量*/

    UINT32 u32DbgTimeIdx;                           /* 时间调试信息索引*/
    DISP_DEBUG_CHN_TIME stDbgTime[DISP_DEBUG_CHN_TIME_NUM];  /* 时间调试信息*/
} DISP_CHN_COMMON;

/* 视频层属性 */
typedef struct tagDispLayer
{
    UINT32 uiLayerWith;                             /* 长 */
    UINT32 uiLayerHeight;                           /* 宽 */
    UINT32 uiLayerFps;                              /* 帧率 */
    UINT32 uiLayerNo;                               /* 视频层编号，一般与设备号相同 */
    SAL_VideoDataFormat enInputSalPixelFmt;         /* 视频层的输入像素格式 */
    UINT32 uiChnCnt;                                /* 记录当前正在使能的通道个数 */
    UINT32 uiMaxChnCnt;                             /* 记录初始化时最大通道个数,只配置一次，后续不能改变 */
    UINT32 runFlg[2];                               /* 输入在两个输出上显示的标志 */
    DISP_CHN_COMMON *pstVoChn[DISP_HAL_MAX_CHN_NUM];
} DISP_LAYER_COMMON;

/* 显示设备属性 */
typedef struct tagDispDev
{
    UINT32 uiStatus;                                /* 显示设备使能状态 */
    UINT32 uiDevWith;                               /* 长 */
    UINT32 uiDevHeight;                             /* 宽 */
    UINT32 uiDevFps;                                /* 视频输出设备刷新帧率 */
    UINT32 uiDevNo;                                 /* 设备号 */
    UINT32 uiBgcolor;                               /* 背景色 */
    VIDEO_OUTPUT_DEV_E eVoDev;                      /* 显示输出设备类型 */
    DISP_LAYER_COMMON szLayer;                      /* 视频层信息 */
} DISP_DEV_COMMON;

typedef struct tagDispMagnifier
{
    UINT32 enable;
    UINT32 viChn;
    UINT32 voDev;
    UINT32 voChn;
    void *mChnMutexHdl;                        /* 通道信号量*/
    SAL_ThrHndl stChnThrHandl;                /* 通道线程 */
} DISP_MAGNIFIER_COMMON;


/* 显示全局结构体 */
typedef struct tagDispModule
{
    UINT32 uiDevMaxCnt;                              /* 记录初始化时最设备个数,只配置一次，后续不能改变 */
    void *mMutexHdl;                                 /* 通道信号量*/
    DISP_DEV_COMMON *pstDevObj[DISP_HAL_MAX_DEV_NUM]; /* 设备个数 */
} DISP_MODULE_COMMON;

typedef struct tagClearSvaType
{
    UINT32 svaclear;
    UINT32 tip;         /* 1 清TIP */
    UINT32 orgnaic;    /* 1清有机物 */
} DISP_CLEAR_SVA_TYPE;

/* 坐标矫正全局参数 */
typedef struct tagDispOsdCoordinateCorrect
{
    UINT32 uiUpOsdFlag;         /* 用来记录连线模式下，OSD是否花在画面上部 */
    UINT32 uiUpStartY;          /* 画面上不进行OSD叠加的起始Y值 */
    UINT32 uiTmpOsdY;
    FLOAT32 f32LastRightX;
    FLOAT32 f32UpRightX;
    UINT32 uiDownRightX; /* 记录屏幕下方标签最右端X值 */
    UINT32 uiDownLowY;   /* 记录屏幕下方标签最下端Y值 */
    UINT32 uiAiOffsetH;
    UINT32 uiAiCropH;
    
    /* 以下变量用于十字框选叠框类型使用 */
    UINT32 uiPkgLeftX;            /* 包裹左边沿X坐标 */
    UINT32 uiPkgUpY;              /* 包裹上沿Y坐标 */
    UINT32 uiLastY;               /* 上一次叠加OSD时的Y坐标 */
    UINT32 uiTmpOsdLen;           /* 计算单列最长OSD长度, 中间变量 */
    UINT32 uiMaxOsdLen;           /* 当前最长的OSD，用于换列展示 */

    /* 用于7.1号新增叠框类型上部不够放向下排列 SVA_OSD_TYPE_CROSS_NO_RECT_TYPE和SVA_OSD_TYPE_CROSS_RECT_TYPE */
    BOOL bDownArranged;
    BOOL bOsdPstOutFlag;            /* 0:预览osd, 1:编图JPG osd */

    UINT32 uiPicW;                  /* 图像的宽 */
    UINT32 uiPicH;                  /* 图像的高*/
    UINT32 uiSourceW;               /* 真实包裹的宽*/
    UINT32 uiSourceH;               /* 真实包裹的高*/
    UINT32 uiUpTarNum;              /* 包裹上方的危险品个数 */
    UINT32 uiDownTarNum;            /* 包裹下方的危险品个数 */
    UINT32 uiChan;
} DISP_OSD_COORDINATE_CORRECT;

/* OSD坐标矫正单个区域的参数 */
typedef struct tagDispOsdCorrectPrm
{
    UINT32 u32X;
    UINT32 u32Y;
    UINT32 u32Width;
    UINT32 u32Height;
    SVA_BORDER_TYPE_E enBorderType;
} DISP_OSD_CORRECT_PRM_S;


/* ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// */


/* ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// */

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : disp_hal_putFrameMem
* 描  述  : 释放帧内存
* 输  入  : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_putFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo);


/**
 * @fn      disp_hal_clearFrameMem
 * @brief   清除Frame数据，置帧图像为纯色
 * 
 * @param   [IN] pstSystemFrameInfo 帧数据
 * @param   [IN] au32BgColor 背景色，YUV格式分别为Y、U、V，ARGB格式分别为B、G、R、A
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS disp_hal_clearFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 au32BgColor[4]);


/*******************************************************************************
* 函数名  : disp_hal_getFrameMem
* 描  述  : 申请帧内存
* 输  入  : - imgW              :
*         : - imgH              :
*         : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_getFrameMem(UINT32 imgW, UINT32 imgH, SYSTEM_FRAME_INFO *pstSystemFrameInfo);


/*******************************************************************************
* 函数名  : disp_hal_voInterface
* 描  述  : 设置VO显示接口
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_voInterface(DISP_DEV_COMMON *pDispDev);

/*******************************************************************************
* 函数名  : disp_hal_getHdmiEdid
* 描  述  : 获取芯片hdmi EDID
* 输  入  : - u32HdmiId: 芯片edid号
*       : - pu8Buff:edid信息
*       : - pu32Len:edid信息长度
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_getHdmiEdid(UINT32 u32HdmiId, UINT8 *pu8Buff, UINT32 *pu32Len);

/*******************************************************************************
* 函数名  : disp_hal_putNoSignalPicFrame
* 描  述  : 无视频数据时填充视频帧信息
* 输  入  : - videoFrame:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_putNoSignalPicFrame(SAL_VideoFrameBuf *videoFrame,VOID *pFrame);

/*******************************************************************************
* 函数名  : disp_hal_sendFrame
* 描  述  : 将数据送至vo通道
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_sendFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec);

INT32 disp_hal_pullOutFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec);

/*******************************************************************************
* 函数名  : disp_drvDeinitStartingup
* 描  述  : 开机时显示反初始化
* 输  入  : - uiDev  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_deInitStartingup(UINT32 uiDev);

/*******************************************************************************
* 函数名  : disp_hal_disableChn
* 描  述  : 禁止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_disableChn(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_enableChn
* 描  述  : 使能vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_enableChn(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_startChn
* 描  述  : 开始vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_startChn(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_stopChn
* 描  述  : 停止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_stopChn(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_setChnPos
* 描  述  : 设置vo参数（放大镜和小窗口）
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_setChnPos(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_deleteDev
* 描  述  : 删除显示层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_deleteDev(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_createLayer
* 描  述  : 创建视频层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_createLayer(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_clearVoBuf
* 描  述  : 清除vo缓存数据
* 输  入  : - uiLayerNo  : Vo设备号
*         : - uiVoChn  : Vo通道号
          : - bFlag    : 选择模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_clearVoBuf(UINT32 uiLayerNo, UINT32 uiVoChn, UINT32 bFlag);

/*******************************************************************************
* 函数名  : disp_hal_getVoChnFrame
* 描  述  : 获取vo输出帧数据
* 输  入  : VoLayer：视频层号
*       ：- VoChn: 通道号
*       : - pstFrame    :显示数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_getVoChnFrame(UINT32 VoLayer, UINT32 VoChn, SYSTEM_FRAME_INFO *pstFrame);

/*******************************************************************************
* 函数名  : disp_hal_setVoLayerCsc
* 描  述  : 设置视频层图像输出效果
* 输  入  : - uiChn: VO通道
*       : - prm: 视频层参数信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_setVoLayerCsc(UINT32 uiChn, VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 创建显示设备
* 输  入  : - pDispDev:设备参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_createDev(DISP_DEV_COMMON *pDispDev);

/*******************************************************************************
* 函数名  : disp_hal_modInit
* 描  述  : 显示模块初始化
* 输  入  : VOID *prm
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_modInit(VOID *prm);

/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 初始化disp hal层函结构体
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_Init(VOID);

/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 初始化disp hal层函结构体
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Disp_halSetVoChnPriority(VOID *prm);
/*******************************************************************************
* 函数名  : disp_hal_enableDispWbc
* 描  述  : 回写操作使能开启关闭操作
* 输  入  : - VoLayer    *pParam
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_enableDispWbc(UINT32 VoLayer,unsigned int *pParam);
/*******************************************************************************
* 函数名  : disp_hal_getDispWbc
* 描  述  : 回写操作
* 输  入  : VoLayer:
* 输  出  : pstWbcSources
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
******************************************************************************/
INT32 disp_hal_getDispWbc(UINT32 VoWbc,VOID *prm);



#endif


