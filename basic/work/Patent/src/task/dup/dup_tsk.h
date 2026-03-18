
#ifndef _DUP_TSK_H_
#define _DUP_TSK_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "dup_tsk_api.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

/* DUP最大个数 */
#define DUP_MAX_OBJ           (32)

#define DUP_TSK_MAX_OUT_CHN   (4+4+3+3) //vpss grp 0 /2有三个通道可用，vpss grp 1/3有4个可用

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/*****************************************************************************
                            drv
*****************************************************************************/

/* dup drv chn 属性 */
typedef struct tagDupDrvChnCtrlPrm
{
    HAL_VPSS_GRP_PRM  stHalVpssGrpPrm0;      /* 用于处理单独建模命令数据的 vpss 信息目前先用VPSS */
    HAL_VPSS_GRP_PRM  stHalVpssGrpPrm1;      /* 用于处理单独建模命令数据的 vpss 信息目前先用VPSS , 重构之后改结构体可以删除*/
}DUP_DRV_CHN_CTRL_PRM;

/*****************************************************************************
                            tsk
*****************************************************************************/
/* dup Chn 属性 */
typedef struct tagDupObjChnPrm
{
    /* 输出队列数,由上层配置下来 */
    UINT32 uiModChn;
    UINT32 uiOutChn; /* 暂时不用 */

    SAL_ThrHndl stThrHandl;

    /* 帧率统计信息 */
    UINT32 uiCnt;
    struct timeval timeval;
    UINT32 startTime;
    UINT32 endTime;

    UINT32 uiDupCount[DUP_MAX_BUFFER];
    UINT32 uiDupWIdx;
    UINT32 uiDupRIdx[DUP_TSK_MAX_OUT_CHN];

    pthread_mutex_t DupMutex;

    DSA_QueHndl pstInFullQue;
    DSA_QueHndl pstInEmptQue;

    SYSTEM_FRAME_INFO  astInQue[DUP_MAX_BUFFER];

    /* 输出通道号 */
    UINT32          uiOutCnt;
    DUP_ChanHandle *pChanHandle[DUP_TSK_MAX_OUT_CHN]; 

    DUP_DRV_CHN_CTRL_PRM stDupChnCTrlPrm;
    INST_INFO_S *pstInst;
} DUP_OBJ_PRM;

/* Dup obj 管理结构体 */
typedef struct tagDupObjCOMMON{
    UINT32 uiObjCnt;                            /* 已创建个数 */
    pthread_mutex_t lock;
    UINT64 u64DupObjBeUsed;
    DUP_OBJ_PRM *pstDupObj[DUP_MAX_OBJ];
} DUP_OBJ_COMMON;

typedef struct tagDupVpssPrm{
    UINT32 width;
	UINT32 height; 
} DUP_VPSS_PRM;


#endif

