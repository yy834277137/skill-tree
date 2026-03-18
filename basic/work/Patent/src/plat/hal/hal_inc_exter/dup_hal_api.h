/*******************************************************************************
* dup_hal.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年09月19日 Create
*
* Description : 硬件平台 Dup 模块
* Modification:
*******************************************************************************/
#ifndef _DUP_HAL_H_

#define _DUP_HAL_H_

#include "sal.h"

#include "dup_common_api.h"

/*****************************************************************************
                            宏定义
*****************************************************************************/
/* Dup 模块最多支持通道数 */
#define DUP_CHN_NUM_MAX          (32)

#define DUP_CHN_OUT_CHN_NUM_MAX  (DUP_CHN_NUM_MAX)

#define ENABLED                  (1)
#define DISABLED                 (0)



/*****************************************************************************
                            结构体定义
*****************************************************************************/

/* 通道 输出通道 属性 */
typedef struct tagDupHalOutChnAttrSt
{
    UINT32 uiOutChnW;                /* 输出通道图像 宽 */
    UINT32 uiOutChnH;                /* 输出通道图像 高 */
    // UINT32 uiOutChnFps;               /* 输出通道图像 帧率, 暂时不使用不用控制帧率 */
}DUP_HAL_OUT_CHN_ATTR;


/* 通道 输出通道 属性 */
typedef struct tagDupHalChnAddrSt
{
    PhysAddr phyAddr;                /* 物理地址 */
    void*    virAddr;                /* 虚拟地址 */
	UINT32   memSize;                /* 地址空间总大小 */
	UINT32   poolId;
}DUP_HAL_CHN_ADDR;


/* 通道 创建属性 */
typedef struct tagDupHalChnInitAttrSt
{
    UINT32 uiMaxW;   /* 当前通道的最大 宽 */
    UINT32 uiMaxH;   /* 当前通道的最大 高 */
    UINT32 uiOutChn;                              /* 输出通道个数 */
    UINT32 uiOutChnEnable[DUP_CHN_OUT_CHN_NUM_MAX];   /* 输出通道使能情况 */
    DUP_HAL_OUT_CHN_ATTR stOutChnAttr[DUP_CHN_OUT_CHN_NUM_MAX]; /* 每个输出通道属性 */
}DUP_HAL_CHN_INIT_ATTR;

/* 通道 创建属性 */
typedef struct tagDupHalFrameOutChnAttrSt
{
    UINT32 uiChnIdx;                       /* 当前通道序号 */
    DUP_HAL_OUT_CHN_ATTR stOutChnAttr;     /* 通道的属性   */
}DUP_HAL_FRAME_OUT_CHN_ATTR;

/* 通道 属性 */
typedef struct tagDupHalChnPrmSt
{
    UINT32 uiVpssChn;         /* 当前VPSS组通道号 */

    UINT32 uiPhyChnNum;       /* 物理通道个数 */
    UINT32 uiExtChnNum;       /* 扩展通道个数 */

}DUP_HAL_CHN_PRM;


/* vpss group 属性 */
typedef struct tagHalVpssGrpPrmSt
{
    UINT32 uiVpssChn;         /* 当前通道号 */
	UINT32 uiGrpWidth;
	UINT32 uiGrpHeight;
    VPSS_DEV_TYPE_E enVpssDevType;  /* vpss模块使用的硬件类型，vpe/gpu/rga */
    SAL_VideoDataFormat enInputSalPixelFmt; //输入的像素格式
    SAL_VideoDataFormat enOutputSalPixelFmt; //输出的像素格式

    UINT32 uiPhyChnNum;       /* 物理通道个数 */
    UINT32 uiExtChnNum;       /* 扩展通道个数 */
    UINT32 uiChnEnable[DUP_CHN_NUM_MAX];
    UINT32 uiChnDepth;
    UINT32 u32BlkCnt;
    UINT32 u32PoolID;
    UINT32 u32PoolSize;
}HAL_VPSS_GRP_PRM;

/* 模块 属性 */
typedef struct tagDupModPrmSt
{
    /* vpss 通道属性 */
    pthread_mutex_t dupMutex;
    UINT32          uiVpssGroupBeUsed;
    DUP_HAL_CHN_PRM stDupHalChnPrm[DUP_CHN_NUM_MAX];
}DUP_MOD_PRM;



/*****************************************************************************
 函 数 名  : DupHal_drvVpssSetChnAttr
 功能描述  : vpss设置通道属性，配置通道分辨率，裁剪
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
INT32 DupHal_drvVpssGetSizeAttr(UINT32 uiGrp, UINT32 uiChn, UINT32 *VpssWidth, UINT32 *VpssHeight);
INT32 DupHal_drvVpssSetSizeAttr(UINT32 uiDupChn, UINT32 uiOutChn, UINT32 VpssWidth, UINT32 VpssHeight);
INT32 DupHal_drvGetFreeVpssGroup(UINT32 * puiChn);
INT32 Hal_putFreeVpssGroup(UINT32 uiChn);
INT32 dup_hal_getChnFrame(UINT32 dupChan, UINT32 videoSourceChan, SYSTEM_FRAME_INFO *pstFrame);
INT32 dup_hal_putChnFrame(UINT32 dupChan, UINT32 videoSourceChan, SYSTEM_FRAME_INFO *pstFrame);
INT32 DupHal_drvVpssSetGrpCrop(UINT32 uiGrpChn, UINT32 uiOutChn, UINT32 bCropEnable,INT32 uiX,INT32 uiY,INT32 uiW,INT32 uiH);
INT32 dup_hal_sendDupFrame(UINT32 dupChan,SYSTEM_FRAME_INFO *pstFrame);


void dup_hal_setDbLevel(INT32 level);
INT32 dup_hal_getPhysChnNum(void);
INT32 dup_hal_getExtChnNum(void);
INT32 dup_hal_initDupGroup(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm);
INT32 dup_hal_deinitDupGroup(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm);
INT32 dup_hal_setDupParam(UINT32 u32Grp, UINT32 u32Chn, PARAM_INFO_S *pstParamInfo);
INT32 dup_hal_getDupParam(UINT32 u32Grp, UINT32 u32Chn, PARAM_INFO_S *pstParamInfo);
INT32 dup_hal_startChn(UINT32 u32Grp, UINT32 u32Chn);
INT32 dup_hal_stopChn(UINT32 u32Grp, UINT32 u32Chn);

/**
 * @function    vpe_nt9833x_scaleYuvRange
 * @brief       通过硬件SCA模块将yuv范围转换为0~255，并根据需要做yuv垂直镜像，最大支持尺寸：2048x1280
 * @param[in]   SAL_VideoSize *pstSrcSize    原图尺寸
 * @param[in]   SAL_VideoSize *pstDstSize    目标图尺寸
 * @param[in]   SAL_VideoCrop *pstSrcCropPrm 原图中进行yc伸张区域
 * @param[in]   SAL_VideoCrop *pstDstCropPrm 目标图中存放yc伸张后结果区域
 * @param[in]   CHAR          *pstSrc        原图数据
 * @param[out]  CHAR          *pstDst        目标图数据
 * @param[in]   BOOL          bIfMirror      是否镜像
 * @return
 */
INT32 dup_hal_scaleYuvRange(SAL_VideoSize *pstSrcSize, SAL_VideoSize *pstDstSize, 
                            SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm, 
                            CHAR *pu8Src, CHAR *pu8Dst, BOOL bIfMirror);

INT32 dup_hal_init(void);






#endif /* _DUP_HAL_H_ */


