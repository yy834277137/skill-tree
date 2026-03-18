/**
 * @file   capt_hal_api.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台采集模块头文件
 * @author liuxianying
 * @date   2021/8/6
 * @note
 * @note \n History
   1.日    期: 2021/8/6
     作    者: liuxianying
     修改历史: 创建文件
 */

#ifndef _CAPT_HAL_H_
#define _CAPT_HAL_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"

#include "platform_hal.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/* 采集模块两个通道 */
#define CAPT_CHN_NUM_MAX        (2)
#define ISP_CHN_NUM_MAX         (CAPT_CHN_NUM_MAX)
#define CAPT_MEM_NAME "capt"

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 输入的分辨率信息 */
typedef struct
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
} CAPT_RESOLUTION_S;

/* Capt Chn 属性 */
typedef struct tagCaptChnAttrSt
{
    UINT32 uiCaptWid;  /* 视频宽 */
    UINT32 uiCaptHei;  /* 视频高 */
    UINT32 uiCaptFps;  /* 视频帧率 */
} CAPT_CHN_ATTR;


/* 通道 状态属性 */
typedef struct tagCaptHalChnStatusSt
{
    UINT32 uiIntCnt; /* 中断数 */
    UINT32 FrameLoss; /* 丢帧数 */
    UINT32 captStatus;/* 采集状态0:视频正常,1:无输入，2:输入不支持 */
}CAPT_HAL_CHN_STATUS;


/* Capt Chn 用户图片信息 */
typedef struct tagCaptChnUserPicPrm
{
    UINT32 uiW;
    UINT32 uiH;

    PUINT8 pucAddr;
    UINT32 uiSize;
} CAPT_CHN_USER_PIC;

/* ISP 通道属性 */
typedef struct tagIspChnAttr
{
    UINT32 uiIspChnWid;  /* 视频宽 */
    UINT32 uiIspChnHei;  /* 视频高 */
    UINT32 uiIspChnFps;  /* 视频帧率 */
    UINT32 uiWdrMod;     /* WDR 模式 */
} ISP_CHAN_ATTR;

/* ISP 模块创建属性 */
typedef struct tagIspCreateAttr
{
    UINT32        uiChnCnt;
    ISP_CHAN_ATTR stIspChanAttr[ISP_CHN_NUM_MAX];
} ISP_CREATE_ATTR;

/* 通道旋转属性 */
typedef enum DSP_ROTATION_E
{
    CAPT_ROTATION_0   = 0,
    CAPT_ROTATION_90  = 1,
    CAPT_ROTATION_180 = 2,
    CAPT_ROTATION_270 = 3,
    CAPT_ROTATION_BUTT
} CAPT_ROTATION;

/* 输入的接口 */
typedef enum
{
    CAPT_CABLE_VGA,
    CAPT_CABLE_HDMI,
    CAPT_CABLE_DVI,
    CAPT_CABLE_BUTT,
} CAPT_CABLE_E;

/* 输入的状态 */
typedef struct
{
    CAPT_CABLE_E enCable;
    CAPT_RESOLUTION_S stRes;
    BOOL bCableConnected;             /* 是否有线缆接入 */
    BOOL bVideoDetected;              /* 是否有检测到视频，及时检测到线缆接入，也有可能无视频 */
} CAPT_STATUS_S;


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */


/**
 * @function   capt_hal_drvGetUnLock
 * @brief      采集资源解锁
 * @param[in]  UINT32 chnId  通道号
 * @param[out] None
 * @return     static INT32
 */
INT32 capt_hal_drvSetFlag(UINT32 chnId, UINT32 flag);

/**
 * @function   capt_hal_drvGetLock
 * @brief      采集资源加锁
 * @param[in]  UINT32 chnId  通道号
 * @param[out] None
 * @return     static INT32
 */
INT32 capt_hal_drvGetFlag(UINT32 chnId);

/**
 * @function   capt_hal_drvSetWDRPrm
 * @brief      采集模块 配置 VI 设备宽动态属性
 * @param[in]  UINT32 uiChn     通道信息
 * @param[in]  UINT32 uiEnable  是否使能
 * @param[out] None
 * @return     INT32   SAL_SOK
 *                     SAL_FAIL
 */
INT32 capt_hal_drvSetWDRPrm(UINT32 uiChn, UINT32 uiEnable);

/**
 * @function   capt_hal_getCaptUserPicStatue
 * @brief      采集模块使用用户图片，用于无信号时编码
 * @param[in]  UINT32 uiChn  
 * @param[out] None
 * @return     UINT32 SAL_SOK
 *                    SAL_FAIL
 */
UINT32 capt_hal_getCaptUserPicStatue(UINT32 uiChn);

/*****************************************************************************
 函 数 名  : capt_hal_getCaptBuffer
 功能描述  : Camera采集通道获取数据缓冲区
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史	   :
  1.日	  期   : 2016年12月13日
	作	  者   : wwq
	修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_setCaptPos(UINT32 viChn, POSITION_MODE posInfo, UINT32 offset);

/*****************************************************************************
 函 数 名  : capt_hal_initCaptInterface
 功能描述  : 初始化采集接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_reSetCaptInterface(UINT32 uiChn, CAPT_RESOLUTION_S *stRes);

/*****************************************************************************
 函 数 名  : capt_hal_initCaptInterface
 功能描述  : 初始化采集接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_initCaptInterface(UINT32 uiChn);

/*****************************************************************************
 函 数 名  : capt_hal_setCaptUserPic
 功能描述  : 采集模块配置用户图片，用于无信号时编码
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_setCaptUserPic(UINT32 uiDev, CAPT_CHN_USER_PIC *pstUserPic);


/*****************************************************************************
 函 数 名  : capt_hal_enableCaptUserPic
 功能描述  : 使能采集模块使用用户图片，用于无信号时编码
 输入参数  : -uiChn 输入设备号
         : -uiEnble 是否使能
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_enableCaptUserPic(UINT32 uiChn, UINT32 uiEnble);

/*****************************************************************************
 函 数 名  : capt_hal_setCaptRotate
 功能描述  : 采集模块采集通道旋转属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_setCaptRotate(UINT32 uiChn, UINT32 eRotate);

/*****************************************************************************
 函 数 名  : capt_hal_getCaptPipeId
 功能描述  : 获取采集pipi ID
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_getCaptPipeId(UINT32 uiChn, INT32 *pPipeId);

/*****************************************************************************
 函 数 名  : capt_hal_getCaptStatus
 功能描述  : 查询采集通道状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_getCaptStatus(UINT32 uiChn, CAPT_HAL_CHN_STATUS *pstStatus);

/*****************************************************************************
 函 数 名  : capt_hal_getCaptBuffer
 功能描述  : Camera采集通道获取数据缓冲区
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_getCaptBuffer(UINT32 uiChn, UINT32 *uiFrameInfo);

/*****************************************************************************
 函 数 名  : capt_hal_putCaptBuffer
 功能描述  : Camera采集通道释放数据缓冲区
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史     :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容    : 新生成函数
*****************************************************************************/
INT32 capt_hal_putCaptBuffer(UINT32 uiChn, UINT32 uiFrameInfo);

/*****************************************************************************
 函 数 名  : capt_hal_checkCaptBuffer
 功能描述  : Camera采集通道查询是否有数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_checkCaptBuffer(UINT32 uiChn);

/*****************************************************************************
 函 数 名 : capt_hal_captGetFrame
 功能描述 : 输入获取帧数据
 输入参数   :   uiDev 输入通道
 输出参数   :   pstFrame 帧数据
 返 回 值 : 成功:SAL_SOK
            失败:SAL_FAIL
*****************************************************************************/
INT32 capt_hal_captGetFrame(UINT32 uiDev, SYSTEM_FRAME_INFO *pstFrame);

/*****************************************************************************
 函 数 名 : capt_hal_captRealseFrame
 功能描述 : 输入释放帧数据
 输入参数   :   uiDev 输入通道
            pstFrame 帧数据
 输出参数   : 无
 返 回 值 : 成功:SAL_SOK
            失败:SAL_FAIL
*****************************************************************************/
INT32 capt_hal_captRealseFrame(UINT32 uiDev, SYSTEM_FRAME_INFO *pstFrame);

/*****************************************************************************
 函 数 名 : capt_hal_captIsOffset
 功能描述 : 计算图像是否有偏移
 输入参数   :   uiChn 输入通道
 输出参数   : 无
 返 回 值 : 无
*****************************************************************************/
BOOL capt_hal_captIsOffset(UINT32 uiDev);

/*****************************************************************************
 函 数 名  : capt_hal_stopCaptDev
 功能描述  : Camera采集通道停止
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_stopCaptDev(UINT32 uiChn);

/*****************************************************************************
 函 数 名  : capt_hal_startCaptDev
 功能描述  : Camera采集通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_startCaptDev(UINT32 uiDev);


/*****************************************************************************
 函 数 名  : capt_hal_createCaptDev
 功能描述  : 重新初始化dev
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_reInitCaptDev(UINT32 uiChn);

/*****************************************************************************
 函 数 名  : capt_hal_createCaptDev
 功能描述  : Camera采集通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_createCaptDev(UINT32 uiChn, CAPT_CHN_ATTR *pstChnAttr);

/*****************************************************************************
 函 数 名  : capt_hal_releaseCaptDev
 功能描述  : Camera采集通道销毁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_releaseCaptDev(UINT32 uiChn);


/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 初始化disp hal层函结构体
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_hal_Init(VOID);



#endif

