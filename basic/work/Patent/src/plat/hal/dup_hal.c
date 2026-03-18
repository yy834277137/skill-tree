/*******************************************************************************
* dup_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年09月19日 Create
*
* Description : 硬件平台 Dup 模块
* Modification:
*******************************************************************************/

#include <sal.h>
#include <dspcommon.h>
//#include <platform_sdk.h>
#include <platform_hal.h>
#include "capbility.h"

//#include "../vpss/vpss_hisi.h"
//#include "vpss_hisi.h"
#include "dup_hal_inter.h"

#line __LINE__ "dup_hal.c"

/*****************************************************************************
                               宏定义
*****************************************************************************/

static INT32 dupHalDbLevel = DEBUG_LEVEL0;


/*****************************************************************************
                               全局变量
*****************************************************************************/

DUP_MOD_PRM g_stDupModPrm = 
{
    .dupMutex     = PTHREAD_MUTEX_INITIALIZER,
};
static  const DUP_PLAT_OPS * g_pstDupPlatOps = NULL;

/*****************************************************************************
                                函数
*****************************************************************************/


/*****************************************************************************
 函 数 名  : dup_mutexLock
 功能描述  : dup hal模块上锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年08月21日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_mutexLock( )
{
    pthread_mutex_t * pDupMutex = NULL;

    pDupMutex = &g_stDupModPrm.dupMutex;

    pthread_mutex_lock(pDupMutex);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : dup_mutexUnlock
 功能描述  : dup hal 层去锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年08月21日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_mutexUnlock( )
{
    pthread_mutex_t * pDupMutex = NULL;

    pDupMutex = &g_stDupModPrm.dupMutex;

    pthread_mutex_unlock(pDupMutex);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : dup_getFreeDupGroup
 功能描述  : 获取可用的 Vpss 模块
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月20日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_getFreeDupGroup(UINT32 * puiChn)
{
    int i = 0;

    dup_mutexLock();

    for (i = 0; i < DUP_CHN_NUM_MAX; i++)
    {
        /* 标记为1了，表示已被使用 */
        if (g_stDupModPrm.uiVpssGroupBeUsed & (1<<i))
        {
            continue;
        }
        else
        {
            /* 发现未使用的，返回通道号，并标记为已用 */
            g_stDupModPrm.uiVpssGroupBeUsed = g_stDupModPrm.uiVpssGroupBeUsed | (1 << i);
            *puiChn = i;
            break;
        }
    }

    /* DUP_CHN_NUM_MAX 个都被使用了，表示无可用的，返回错误，
        此情况就该扩大 DUP_CHN_NUM_MAX 的值
     */
    if (i == DUP_CHN_NUM_MAX)
    {
        *puiChn = 0;
        dup_mutexUnlock( );
        return SAL_FAIL;
    }

    dup_mutexUnlock( );

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : dup_putFreeDupGroup
* 描  述  : 
* 输  入  : - uiChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 dup_putFreeDupGroup(UINT32 uiChn)
{
    if (uiChn >= DUP_CHN_NUM_MAX)
    {
        DUP_LOGE("put free vpss group channel num is wrong!\n");
        return SAL_FAIL;
    }

    g_stDupModPrm.uiVpssGroupBeUsed = g_stDupModPrm.uiVpssGroupBeUsed & ~(1 << uiChn);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Dup_halVpssSetAttr
 功能描述  : vpss通道根据vi旋转设置属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :
    作    者   :
    修改内容   : 新生成函数
               : 本接口只用于测试，其他模块不要调用该接口
*****************************************************************************/
INT32 DupHal_drvVpssSetSizeAttr(UINT32 uiDupChn, UINT32 uiOutChn, UINT32 VpssWidth, UINT32 VpssHeight)
{
    DUP_HAL_CHN_PRM *pstHalVpssGrpPrm = NULL;
    UINT32 VpssGroupID = 0;
    INT32 s32Ret = SAL_SOK;
    PARAM_INFO_S stParamInfo;
    memset(&stParamInfo, 0, sizeof(stParamInfo));

    pstHalVpssGrpPrm = &g_stDupModPrm.stDupHalChnPrm[uiDupChn];
    VpssGroupID = pstHalVpssGrpPrm->uiVpssChn;

	if (uiDupChn >= DUP_CHN_NUM_MAX)
	{
	     DUP_LOGE("uiDupChn %d\n", uiDupChn);
		 return SAL_FAIL;
	}

    stParamInfo.enType = IMAGE_SIZE_CFG;
    stParamInfo.stImgSize.u32Width = VpssWidth;
    stParamInfo.stImgSize.u32Height = VpssHeight;
    s32Ret = g_pstDupPlatOps->setParam(VpssGroupID, uiOutChn, &stParamInfo);
    if (SAL_SOK != s32Ret)
    {
        DUP_LOGE("Vpss %d put Chn %d Frame Failed, %x !!!\n", VpssGroupID, uiOutChn, s32Ret);
        return SAL_FAIL;
    }

    DUP_LOGD("dupChan = %d,pstDupHalChnPrm->uiVpssChn = %d,videoChan = %d..{%d,%d}\n",uiDupChn,pstHalVpssGrpPrm->uiVpssChn,uiOutChn, VpssWidth, VpssHeight);
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : DupHal_drvVpssGetSizeAttr
 功能描述  : vpss获取通道属性，配置通道分辨率
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
           : 本接口只用于测试，其他模块不要调用该接口
*****************************************************************************/
INT32 DupHal_drvVpssGetSizeAttr(UINT32 uiGrp, UINT32 uiChn, UINT32 *VpssWidth, UINT32 *VpssHeight)
{
    UINT32 VpssGroupID = 0;
    INT32  s32Ret = SAL_SOK;
    DUP_HAL_CHN_PRM *pstHalVpssGrpPrm = NULL;
    PARAM_INFO_S stParamInfo;
    memset(&stParamInfo, 0, sizeof(stParamInfo));

	if (uiGrp >= DUP_CHN_NUM_MAX)
	{
	     DUP_LOGE("uiDupChn %d\n", uiGrp);
		 return SAL_FAIL;
	}

    pstHalVpssGrpPrm = &g_stDupModPrm.stDupHalChnPrm[uiGrp];
    VpssGroupID = pstHalVpssGrpPrm->uiVpssChn;

    stParamInfo.enType = IMAGE_SIZE_CFG;
    s32Ret = g_pstDupPlatOps->getParam(VpssGroupID, uiChn, &stParamInfo);
    if (SAL_SOK != s32Ret)
    {
        DUP_LOGE("Vpss %d put Chn %d Frame Failed, %x !!!\n", VpssGroupID, uiChn, s32Ret);
        return SAL_FAIL;
    }

    *VpssWidth = stParamInfo.stImgSize.u32Width;
    *VpssHeight = stParamInfo.stImgSize.u32Height;
    
	return s32Ret;
}






INT32 dup_hal_getChnFrame(UINT32 dupChan, UINT32 videoSourceChan, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32               iRet = 0;
	UINT32 VpssGroupID = 0;

	if (NULL == pstFrame || 0 == pstFrame->uiAppData)
	{
	     DUP_LOGE("Chn %d,  input param is null!!! %p, %p \n", dupChan, pstFrame, (NULL != pstFrame ? (VOID *)pstFrame->uiAppData : NULL));
         return SAL_FAIL;
	}
    VpssGroupID = dupChan;
    iRet = g_pstDupPlatOps->getFrame(VpssGroupID, videoSourceChan, pstFrame, 200);
    if (SAL_SOK != iRet)
    {
        DUP_LOGD("Vpss %d Get vpss Chn %d Frame Failed, %x !!!\n", VpssGroupID, videoSourceChan, iRet);
        return SAL_FAIL;
    }
	
    return SAL_SOK;
}

/* 临时代码 */
INT32 dup_hal_putChnFrame(UINT32 dupChan, UINT32 videoSourceChan, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32               iRet = 0;
	UINT32 VpssGroupID = 0;

	if (NULL == pstFrame || 0 == pstFrame->uiAppData)
	{
	     DUP_LOGE("Chn %d,  input param is null!!!\n", dupChan);
         return SAL_FAIL;
	}
    VpssGroupID = dupChan;
    iRet = g_pstDupPlatOps->releaseFrame(VpssGroupID, videoSourceChan, pstFrame);
    if (SAL_SOK != iRet)
    {
        DUP_LOGE("Vpss %d put Chn %d Frame Failed, %x !!!\n", VpssGroupID, videoSourceChan, iRet);
        return SAL_FAIL;
    }
	
    return SAL_SOK;
}


INT32 dup_hal_sendDupFrame(UINT32 dupChan,SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 iRet = 0;
	UINT32 VpssGroupID = 0;

	if (NULL == pstFrame || 0 == pstFrame->uiAppData)
	{
        DUP_LOGE("Chn %d, input param is null!!!\n", dupChan);
        return SAL_FAIL;
	}
    VpssGroupID = dupChan;
    iRet = g_pstDupPlatOps->sendFrame(VpssGroupID, pstFrame, 50);
    if (SAL_SOK != iRet)
    {
        DUP_LOGE("Vpss %d Send vpss Frame Failed, %x !!!\n", VpssGroupID, iRet);
        return SAL_FAIL;
    }
	
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : DupHal_drvInitVpssGroup
 功能描述  : 初始化 Vpss Group 通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史	   :
  1.日	  期   : 2018年07月02日
	作	  者   : wwq
	修改内容   : 新生成函数
*****************************************************************************/
INT32 dup_hal_getPhysChnNum(void)
{
    return g_pstDupPlatOps->getPhyChnNum();
}

INT32 dup_hal_getExtChnNum(void)
{
    return g_pstDupPlatOps->getExtChnNum();
}

INT32 dup_hal_setDupParam(UINT32 u32Grp, UINT32 u32Chn, PARAM_INFO_S *pstParamInfo)
{
    INT32 s32Ret      = 0;
    DUP_HAL_CHN_PRM *pstDupHalChnPrm = NULL;

    if (DUP_CHN_NUM_MAX <= u32Grp)
    {
        DUP_LOGE("Dup Hal No Support Chn %d !!!\n", u32Grp);
        return SAL_FAIL;
    }
    pstDupHalChnPrm = &g_stDupModPrm.stDupHalChnPrm[u32Grp];
        
    s32Ret = g_pstDupPlatOps->setParam(pstDupHalChnPrm->uiVpssChn, u32Chn, pstParamInfo);
    if (SAL_SOK != s32Ret)
    {
        DUP_LOGE("Vpss %d   Chn %d setParam Failed, %x !!!\n", pstDupHalChnPrm->uiVpssChn, u32Chn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

INT32 dup_hal_getDupParam(UINT32 u32Grp, UINT32 u32Chn, PARAM_INFO_S *pstParamInfo)
{
    UINT32 VpssGroupID = 0;
    INT32  s32Ret = SAL_SOK;
    DUP_HAL_CHN_PRM *pstHalVpssGrpPrm = NULL;

    if (u32Grp >= DUP_CHN_NUM_MAX)
	{
         DUP_LOGE("uiDupChn %d\n", u32Grp);
         return SAL_FAIL;
    }

    pstHalVpssGrpPrm = &g_stDupModPrm.stDupHalChnPrm[u32Grp];
    VpssGroupID = pstHalVpssGrpPrm->uiVpssChn;

    s32Ret = g_pstDupPlatOps->getParam(VpssGroupID, u32Chn, pstParamInfo);
    if (SAL_SOK != s32Ret)
    {
        DUP_LOGE("Vpss %d put Chn %d Frame Failed, %x !!!\n", VpssGroupID, u32Chn, s32Ret);
        return SAL_FAIL;
    }
    
	return s32Ret;
}


INT32 dup_hal_startChn(UINT32 u32Grp, UINT32 u32Chn)
{
    INT32 s32Ret      = 0;
    DUP_HAL_CHN_PRM *pstDupHalChnPrm = NULL;

    if (DUP_CHN_NUM_MAX <= u32Grp)
    {
        DUP_LOGE("Dup Hal No Support Chn %d !!!\n", u32Grp);
        return SAL_FAIL;
    }
    pstDupHalChnPrm = &g_stDupModPrm.stDupHalChnPrm[u32Grp];
        
    s32Ret = g_pstDupPlatOps->start(pstDupHalChnPrm->uiVpssChn, u32Chn);
    if (SAL_SOK != s32Ret)
    {
        DUP_LOGE("Vpss %d put Chn %d Frame Failed, %x !!!\n", pstDupHalChnPrm->uiVpssChn, u32Chn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

INT32 dup_hal_stopChn(UINT32 u32Grp, UINT32 u32Chn)
{
    INT32 s32Ret      = 0;
    DUP_HAL_CHN_PRM *pstDupHalChnPrm = NULL;

    if (DUP_CHN_NUM_MAX <= u32Grp)
    {
        DUP_LOGE("Dup Hal No Support Chn %d !!!\n", u32Grp);
        return SAL_FAIL;
    }
    pstDupHalChnPrm = &g_stDupModPrm.stDupHalChnPrm[u32Grp];
        
    s32Ret = g_pstDupPlatOps->stop(pstDupHalChnPrm->uiVpssChn, u32Chn);
    if (SAL_SOK != s32Ret)
    {
        DUP_LOGE("Vpss %d put Chn %d Frame Failed, %x !!!\n", pstDupHalChnPrm->uiVpssChn, u32Chn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}



INT32 dup_hal_deinitDupGroup(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm)
{
    INT32 s32Ret = 0;
    
    if (SAL_SOK != g_pstDupPlatOps->destroy(pstHalVpssGrpPrm))
    {
        DUP_LOGE("Init Vpss Group Failed !!!\n");

      //  return SAL_FAIL;      //liuxianying需要再确认修改
    }

    s32Ret = dup_putFreeDupGroup(pstHalVpssGrpPrm->uiVpssChn);

    return s32Ret;
}

INT32 dup_hal_initDupGroup(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm)
{
    UINT32 uiVpssGroup = 0;
    INT32  i           = 0;
    UINT32 u32dupPhysChnNum = 0;

    DUP_HAL_CHN_PRM      *pstDupHalChnPrm = NULL;
    
	DUP_HAL_CHN_INIT_ATTR stChnInitAttr = {0};

	if (NULL == pstHalVpssGrpPrm)
	{
		DUP_LOGE("prm == NULL \n");
		return SAL_FAIL;
	}
    if (SAL_FAIL == dup_getFreeDupGroup(&uiVpssGroup))
    {
        DUP_LOGE("Dup Hal Get Vpss Group %d Falied !!!\n", uiVpssGroup);
        return SAL_FAIL;
    }

    pstDupHalChnPrm = &g_stDupModPrm.stDupHalChnPrm[uiVpssGroup];

    memset(pstDupHalChnPrm, 0, sizeof(DUP_HAL_CHN_PRM));

    DUP_LOGD("uiVpssGroup %d \n", uiVpssGroup);

    pstDupHalChnPrm->uiVpssChn  = uiVpssGroup;
    pstHalVpssGrpPrm->uiVpssChn = uiVpssGroup;

    stChnInitAttr.uiOutChn = pstHalVpssGrpPrm->uiPhyChnNum + pstHalVpssGrpPrm->uiExtChnNum;
    for (i = 0; i <  pstHalVpssGrpPrm->uiPhyChnNum; i++)
    {
        stChnInitAttr.uiOutChnEnable[i] = pstHalVpssGrpPrm->uiChnEnable[i];
        /* 如果类似ss928平台vpss同一时刻只能有一个chn做放大，那么除了第0通道都需要配置为支持的vi分辨率中最小的分辨率 */
        stChnInitAttr.stOutChnAttr[i].uiOutChnW = pstHalVpssGrpPrm->uiGrpWidth;
        stChnInitAttr.stOutChnAttr[i].uiOutChnH = pstHalVpssGrpPrm->uiGrpHeight;
    }

    u32dupPhysChnNum = dup_hal_getPhysChnNum();
    for (i = 0; i < pstHalVpssGrpPrm->uiExtChnNum; i++)
    {
        stChnInitAttr.uiOutChnEnable[i+u32dupPhysChnNum] = pstHalVpssGrpPrm->uiChnEnable[i];
        
        /* 跟物理通道先保持一致的分辨率，由上层业务模块再对扩展通道配置分辨率 */
        stChnInitAttr.stOutChnAttr[i+u32dupPhysChnNum].uiOutChnW = 640;
        stChnInitAttr.stOutChnAttr[i+u32dupPhysChnNum].uiOutChnH = 336;
    }
    
    if (SAL_SOK != g_pstDupPlatOps->create(pstHalVpssGrpPrm, &stChnInitAttr))
    {
        DUP_LOGE("Init Vpss Group Failed !!!\n");

        return SAL_FAIL;
    }

    DUP_LOGI("Create Vpss Group Chn %d SUC!!!\n", uiVpssGroup);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Dup_HalSetDbLeave
* 描  述  : 
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void dup_hal_setDbLevel(INT32 level)
{
     dupHalDbLevel = (level > 0) ? level : 0;
	 DUP_LOGD("debugLevel %d\n",dupHalDbLevel);
}

/**
 * @function    dup_hal_scaleYuvRange
 * @brief       通过硬件模块将yuv范围转换为0~255，并根据需要做yuv垂直镜像, 最大支持尺寸：2048x1280
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
                            CHAR *pu8Src, CHAR *pu8Dst, BOOL bIfMirror)
{
	if (NULL == pstSrcSize || NULL == pstDstSize || NULL == pstSrcCropPrm || NULL == pstDstCropPrm || NULL == pu8Src || NULL == pu8Dst)
	{
        DUP_LOGE("Null pointer error, pstSrcSize:%p, pstDstSize:%p, pstSrcCropPrm:%p, pstDstCropPrm:%p, pu8Src:%p, pu8Dst:%p.\n", 
                 pstSrcSize, pstDstSize, pstSrcCropPrm, pstDstCropPrm, pu8Src, pu8Dst);
        return SAL_FAIL;
	}

    if (SAL_SOK != g_pstDupPlatOps->scaleYuvRange(pstSrcSize, pstDstSize, pstSrcCropPrm, pstDstCropPrm, pu8Src, pu8Dst, bIfMirror))
    {
        DUP_LOGE("scaleYuvRange failed, w:%u, h:%u, crop_w:%u, crop_h:%u mirror:%d.\n", 
                 pstSrcSize->width, pstSrcSize->height, pstSrcCropPrm->width, pstSrcCropPrm->height, bIfMirror);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : dup_hal_init
* 描  述  : 初始化dup模块
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 dup_hal_init(void)
{
    if (NULL == g_pstDupPlatOps)
    {
        g_pstDupPlatOps = dup_hal_register();
    }

    return SAL_SOK;
}


