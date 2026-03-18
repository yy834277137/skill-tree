#ifndef _CAPT_TSK_H_

#define _CAPT_TSK_H_

#include "sal.h"

#include <platform_hal.h>

#include "capt_tsk_inter.h"


/* 暂定最大两个采集口，也就两个obj */
#define CAPT_MAX_OBJ       (2)
#define MAX_CAPTURE_CHAN_NUM  (2)

/*****************************************************************************
                            结构体定义
*****************************************************************************/
/* capt Chn 属性 */
typedef struct tagCaptObjChnPrm
{
    UINT32 uiModChn;
    UINT32 uiOutChn;

    SAL_ThrHndl stThrHandl;

    /* 帧率统计信息 */
    UINT32 uiCnt;
    struct timeval timeval;
    UINT32 startTime;
    UINT32 endTime;

    pthread_mutex_t mutex;

    /* 输出通道号 */
    UINT32           uiOutCnt;
    CAPT_ChanHandle *pChanHandle;
} CAPT_OBJ_PRM;


/* Capt obj 管理结构体 */
typedef struct tagCaptpObjCOMMON{
    UINT32 uiObjCnt;                            /* 已创建个数 */
    pthread_mutex_t lock;
    CAPT_OBJ_PRM *pstObj[CAPT_MAX_OBJ];
} CAPT_OBJ_COMMON;

/* Capt Chn 信息 */
typedef struct tagCaptChnPrm
{
    UINT32              uiChn;                  /* 当前通道号       */
    UINT32              uiStart;                /* 通道开启状态     */
    UINT32              captStatus;/* 采集状态0:视频正常,1:无输入，2:输入不支持 */
    UINT32              isOpenWdr;              /* 通道宽动态开启状态 */
    CAPT_CHN_ATTR       stCaptChnAttr;          /* 通道属性         */
    SAL_ThrHndl         videoDetectHndl;        /* 视频信号检测线程 */
    SAL_ThrHndl         videoCaptHndl;          /* 视频采集检测线程 */

    UINT32              uiVideoDectedStatus;    /* 视频检测状态 0表示停止 1表示开启 */
    pthread_cond_t      vicond;
    pthread_mutex_t     viMutex;
} CAPT_CHN_PRM;


/* 采集模块状态结构体 */
typedef struct tagCaptDrvMdStatusSt
{
    UINT32      uiCaptNum;                          /* 采集通道个数                      */
    PhysAddr    uiCaptChnPrm[MAX_CAPTURE_CHAN_NUM]; /* 数组下标表示采集通道 CAPT_CHN_PRM */
}CAPT_DRV_STATUS_S;


/*****************************************************************************
 函 数 名  : CmdProc_captCmdProc
 功能描述  : DSP 模块capt的处理命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年10月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 CmdProc_captCmdProc(HOST_CMD cmd, UINT32 uiChn, void *prm);


/*****************************************************************************
 函 数 名  : capt_tsk_init
 功能描述  : capt 模块初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 capt_tsk_init(void);

/*****************************************************************************
 函 数 名  : capt_tskGetChnPrm
 功能描述  : Capt 模块数据的获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 capt_tskGetChnPrm(CAPT_ChanHandle * pUsrArgs, CAPT_OBJ_PRM **ppstObjPrm);



#endif
