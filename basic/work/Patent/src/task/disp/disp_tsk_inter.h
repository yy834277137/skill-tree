/*******************************************************************************
 * disp_tsk_inter.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : heshengyuan <heshengyuan@hikvision.com>
 * Version: V1.0.0  2018年10月22日 Create
 *
 * Description : DISP是一个摄像头采集后图像管理控制模块，内部包含如下功能:
                 包含: VI数据的采集、数据管理

                 流程:
                 DISP模块创建( 输入VI的视频参数 ，一个模块调用一次接口)
                    |
                   \|/
                 获取一个disp通道(获取到disp通道及数据接口)
                    |
                   \|/
                 拿到该disp通道的数据接口能力
                    |
                   \|/
                 循环使用 OpDispGetBlit/OpDispPutBlit
                    |
                   \|/
                 销毁整个DISP模块
 * Modification:
*******************************************************************************/

#ifndef _DISP_TSK_INTERFACE_H_
#define _DISP_TSK_INTERFACE_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>

#include <linux/fb.h>
#include <platform_hal.h>
#include <dspcommon.h>
#include "disp_tsk_api.h"
#include "svp_dsp_drv_api.h"
#include "color_space.h"

#define DISP_MAX_DEV_NUM	(2)
#ifdef DISP_VIDEO_FUSION
#define MIN_CROP_WIDTH  16
#else
#define MIN_CROP_WIDTH  64
#endif

#define DISP_SVA_DEV_MAX	(2)


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define DISP_MAX_BUFFER      (5)



/* 句柄类型 */
typedef unsigned int       DispHandle;         /* 统用句柄类型 */


/* 指针类型定义 */
typedef void *             Ptr;         /* 指针类型 */

typedef enum
{
    DISP_OPS_FLAG_NONE       =  0x00,

    /* 拿到该标志后，意味着获取到DISP通道的数据输出方式是队列方式，需要成对使用disp接口。 */
    DISP_OPS_FLAG_QUE        =  0x01,

    /* 拿到该标志后，意味着获取到DISP通道的数据输出方式是块拷贝方式，调用disp接口一次即拿到数据。 */
    DISP_OPS_FLAG_COPY       =  0x02,

    /* 拿到该标志后，意味着获取到DISP通道的数据输出方式是海思平台的绑定方式 */
    DISP_OPS_FLAG_BIND       =  0x03,
} DISP_CreateFlag;


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
typedef struct _DISP_TAR_CNT_INFO_
{
	/* 分别表示两个视角的目标类别个数 */
	SVA_PKG_TAR_CNT_S astViewTarCnt[DISP_SVA_DEV_MAX];
} DISP_TAR_CNT_INFO;

typedef struct
{
    UINT32 mod;
    UINT32 modChn;
    UINT32 chn;
}DISP_BIND_PRM;

/* 图形界面信息 */
typedef struct tagDispFbAttrSt
{
    INT8   *cAddr;   /* FB 地址   */
    UINT32  uiSize;  /* FB 的大小 */
}DISP_FB_ATTR_ST;


/* 视频层信息 */
typedef struct tagDispVideoLayerAttrSt
{
    VIDEO_OUTPUT_DEV_E  eVoDev; /* 显示输出设备类型 暂时只关注分辨率 */

    int videoLayerWidth;       /* 视频层宽度 */
    int videoLayerHeight;      /* 视频层高度 */
    int videoLayerFps;         /* 视频层刷新帧率 */
}DISP_VIDEO_LAYER_ATTR_ST;


/* 显示模块初始化信息 */
typedef struct tagDispInitAttrSt
{
    UINT32                    uiVideoLayerCnt;     /* 视频层个数，即显示设备个数 */
    DISP_VIDEO_LAYER_ATTR_ST  stVideoLayerAttr[2]; /* 配置下的视频层属性 */
}DISP_INIT_ATTR_ST;


/* DISP通道创建时指定的参数，和获取到的disp通道及接口能力 */
typedef struct
{
    /* 使用 disp 通道消费者的名字 */
    const INT8             *pName;
    /* 数据的来源 */
    UINT32                  modId;
    /* 数据来源的handle handle中并不能标识数据的来源 */
    UINT32                  handle;
    /* DISP 模块的句柄，初始化 DISP 模块通道时得到，外面使用 disp 通道时不需要更新此成员 */
    DispHandle              module;

    INT32                   reserved[4];
} DISP_ChanCreate;

/* DISP 通道创建成功后获取到的 disp 通道及接口能力。 */
typedef struct
{
    /* 魔数,用于校验句柄有效性。*/
    UINT32                  nMgicNum;

    /* 数据的来源 */
    UINT32                  modId;
    /* 数据来源的handle handle中并不能标识数据的来源 */
    UINT32                  handle;
    
    /* 本 Chan 所属 Module */
    DispHandle              module;
    
} DISP_ChanHandle;

/**
 * @function:   Sva_HalGetRealOut
 * @brief:      获取真实输出
 * @param[in]:  SVA_PROCESS_IN *pstIn
 * @param[out]: SVA_PROCESS_OUT *pstOut
 * @return:     INT32
 */
INT32 disp_osdGetRealOut(UINT32 chan, SVA_PROCESS_IN * pstIn, SVA_PROCESS_OUT * pstOut, DISP_TAR_CNT_INFO *pstPkgCntInfo);

/**
 * @function:   Sva_HalInitCocrtInfo
 * @brief:      初始化矫正参数，按照画面中所有的包裹进行初始化 
 * @param[in]:  DISP_CHN_COMMON *pDispChn
 * @param[in]:  UINT32 uiPicW
 * @param[in]:  UINT32 uiPicH
 * @param[in]:  SVA_PROCESS_IN *pstIn
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  DISP_OSD_COORDINATE_CORRECT *pCorctPtrArr
 * @param[in]:  UINT32 uiCnt
 * @return:     INT32
 */
INT32 disp_osdInitCocrtInfo(DISP_CHN_COMMON *pDispChn,
                            UINT32 uiPicW, 
                            UINT32 uiPicH, 
                            SVA_PROCESS_IN *pstIn, 
                            SVA_PROCESS_OUT *pstOut, 
                            DISP_OSD_COORDINATE_CORRECT *pCorctPtrArr);

INT32 disp_osdPstOutcorrect(DISP_OSD_COORDINATE_CORRECT *osd_correct, 
                                          DISP_OSD_CORRECT_PRM_S *pstOsdPrm, 
                                          DISP_SVA_RECT_F *pstSvaRect,
                                          SVA_PROCESS_OUT *pstOut,
                                          SVA_TARGET *pstTarInfo,
                                          DISP_TAR_CNT_INFO *pstPkgCntInfo);

INT32 disp_osdAiDrawFrame(VOID *prm, SYSTEM_FRAME_INFO *pstSystemFrame, SVA_PROCESS_OUT *pstOut, SVA_PROCESS_IN *pstIn, DISP_TAR_CNT_INFO *pstPkgCntInfo);
INT32 disp_osdClearSvaResult(UINT32 chan, DISP_CLEAR_SVA_TYPE *clearprm);
void disp_osdFlickCheck(DISP_CHN_COMMON *pDispChn);
INT32 disp_osdSetDispFlickPrm(UINT32 uiDev, VOID *prm);
INT32 disp_osdSetLineTypePrm(UINT32 uiDev, VOID *prm);
INT32 disp_osdSvaPrmChange(INT32 chan, SVA_PROCESS_IN *pstIn, INT32 colorFlag, INT32 nameflg);
INT32 disp_osdEnableVoAiFlag(UINT32 uiVoDev, VOID *prm);
INT32 disp_osdSetVoAiSwitch(UINT32 vodev, void *prm);
INT32 disp_osdSetDispLineWidthPrm(UINT32 uiDev, VOID *prm);
INT32 disp_osdDrawInit(UINT32 u32Dev, UINT32 u32Chn, DRAW_MOD_E enLineMode, DRAW_MOD_E enOsdMode);
INT32 disp_osdTest(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

#endif

