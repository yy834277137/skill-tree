/**
 * @file   disp_tsk_api.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  显示tsk对外接口
 * @author dsp
 * @date   2022/5/13
 * @note
 * @note \n History
   1.日    期: 2022/5/13
     作    者: dsp
     修改历史: 创建文件
 */

#ifndef _DISP_TSK_H_
#define _DISP_TSK_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <platform_hal.h>
#include "sal.h"
#include "dspcommon.h"
#include "sva_out.h"


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define DISP_OSD_MAX_OBJ_NUM SVA_XSI_MAX_ALARM_NUM

#define DISP_OSD_MAX_CHAN_NUM DISP_HAL_MAX_DEV_NUM * DISP_HAL_MAX_CHN_NUM + 2 + 4 /* 加2路save画智能信息和2路转存 */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
typedef struct tagOsdHalALignInfo
{
    UINT32 alignAddr;
    UINT32 alignStried;
    UINT32 rev[2];
} OSD_HAL_ALIGN_INFO;

/*
    检测区域
 */
typedef struct tagDispSvaRectF
{
    UINT32 x;                           /* 矩形左上角X轴坐标 */
    UINT32 y;                           /* 矩形左上角Y轴坐标 */
    UINT32 width;                       /* 矩形宽度 */
    UINT32 height;                      /* 矩形高度 */
    FLOAT32 f32EndX;                    /* 矩形右上角X轴浮点坐标 */
    FLOAT32 f32StartXFract;
    FLOAT32 f32EndXFract;
} DISP_SVA_RECT_F;

typedef struct tagDispSvaOutPrm
{
    UINT32 bhave;               /* 是否存在结果 */
    UINT32 bNew;               /* 是否是新的结果 */
    UINT64 needDelayShowTime;              /* 需要延时显示 */
    UINT32 TipOrgShow;               /* tip 或者有机物是否需要显示 1 有机物 2 TIP */
    SVA_PROCESS_OUT svaPrm;
    DISP_SVA_RECT_F svaRect[SVA_XSI_MAX_ALARM_NUM];     /* 是svaPrm中target的整数坐标 */
} DISP_SVA_OUT_PRM;

typedef struct tagDispSvaOutCtl
{
    UINT32 svaCnt;
    UINT32 direction;
    UINT32 reaultCnt;                /* 结果计数 */
    UINT32 color_cnt;
    UINT32 name_cnt;
    UINT32 uiAiTargetCnt;
    INT32 frameNumdef;               /* 帧序号差值 */
    INT32 u32DispTimeRef;               /* 保存当前显示帧号以及前一个显示画面帧号 */
    INT32 u32DispLastTimeRef;             /* 保存前一个显示画面帧号 */
    INT32 u32AiTimeRef;              /* 智能信息延时 */
    INT32 u32refurbish;              /* 包裹停止时,智能信息是否需要刷新 1：刷新 */
    INT32 total[MAX_ARTICLE_TYPE];                  /* 当前屏幕显示各种类型智能信息个数 */
    DISP_SVA_OUT_PRM svaOut[DISP_HAL_XDATA_CNT];

    /* 显示通道的分辨率 */
    UINT32 width;
    UINT32 height;
} DISP_SVA_OUT_CTL;

typedef struct tagDispDebugCtl
{
    void *no_signal_frame;
    void *vo_frame;
    void *no_signal_frame_before;
} DISP_DEBUG_CTL;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   CmdProc_dispCmdProc
 * @brief      显示控制指令（应用层交互）
 * @param[in]  HOST_CMD cmd
 * @param[in]  UINT32 chan
 * @param[in]  void *prm   参数有可能为NULL
 * @param[out] None
 * @return     INT32
 */
INT32 CmdProc_dispCmdProc(HOST_CMD cmd, UINT32 chan,void *prm);

/**
 * @function   disp_tsk_moduleInit
 * @brief      显示模块注册
 * @param[in]  void
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tsk_moduleInit(void);

/**
 * @function   disp_tsk_deInitStartingup
 * @brief      开机时反初始化显示
 * @param[in]  UINT32 uiDev
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tsk_deInitStartingup(UINT32 uiDev);

/**
 * @function   disp_tsk_setDbLevel
 * @brief      设置调试打印等级
 * @param[in]  INT32 level
 * @param[out] None
 * @return     void
 */
void disp_tsk_setDbLevel(INT32 level);

/**
 * @fn      disp_tskShowVoWin
 * @brief   查看VO设备上的窗口属性
 *
 * @param   [IN] u32VoDev VO设备号
 */
void disp_tskShowVoWin(UINT32 u32VoDev);

/**
 * @function   disp_tskShowVoTime
 * @brief      查看VO设备上环节时间信息
 * @param[in]  UINT32 u32VoDev  vo设备号
 * @param[out] None
 * @return     void
 */
void disp_tskShowVoTime(UINT32 u32VoDev);

/**
 * @function    disp_hal_setVoCommonPrm
 * @brief   显示通用设置接口
 * @param[in]    enType 设置类型
 *               prm设置参数
 * @param[out]
 * @return SAL_SOK  : 成功
 *         SAL_FAIL : 失败
 */
INT32 disp_hal_setVoCommonPrm(DISP_CFG_TYPE_E enType, VOID *prm);


/**
 * @function   disp_tsk_dump
 * @brief      显示调试
 * @param[in]  INT32 vodev
 * @param[in]  INT32 vochn
 * @param[in]  INT32 arg0    显示信号源
 * @param[in]  INT32 dumpcnt 循环显示
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tsk_dump(INT32 vodev, INT32 vochn, INT32 arg0, INT32 dumpcnt, CHAR chDumpDir[]);

#endif /* _DISP_TSK_H_ */

