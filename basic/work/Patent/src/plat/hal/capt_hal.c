/**
 * @file   capt_hal.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台采集模块
 * @author liuxianying
 * @date   2021/8/6
 * @note
 * @note \n History
   1.日    期: 2021/8/6
     作    者: liuxianying
     修改历史: 创建文件
 */
 
#include "sal.h"
#include "dspcommon.h"
#include "platform_hal.h"
#include "capbility.h"
#include "hal_inc_inter/capt_hal_inter.h"
#include "stream_bits_info_def.h"



/*****************************************************************************
                               宏定义
*****************************************************************************/
#line __LINE__ "capt_hal.c"




/*****************************************************************************
                               结构体定义
*****************************************************************************/
typedef struct tagAdvDrvInfoSt
{
    void         *MutexHandle;       /* 互斥锁handle */
} ADV_DRV_INFO;


/*****************************************************************************
                               全局变量
*****************************************************************************/

static ADV_DRV_INFO gstAdvInfo[CAPT_CHN_NUM_MAX] = {0};
static UINT32       gVipipeCaptChangeFlg[CAPT_CHN_NUM_MAX] ={0};

static CAPT_PLAT_OPS *pcaptHalOps = NULL;
static VIDEO_MATRIX_INFO       g_stVideoMatrixInfo;
UINT32 gVpssChnWidth  = 1920;
UINT32 gVpssChnHeight = 1200;
static CAPT_ROTATION gHI_Rotate   = CAPT_ROTATION_0;


/*****************************************************************************
                                函数
*****************************************************************************/

/**
 * @function   capt_hal_drvGetLock
 * @brief      采集资源加锁
 * @param[in]  UINT32 chnId  通道号
 * @param[out] None
 * @return     static INT32
 */
static INT32 capt_hal_drvGetLock(UINT32 chnId)
{

    if (chnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("chnId %d\n",chnId);
        return SAL_FAIL;
    }

    return SAL_mutexLock(gstAdvInfo[chnId].MutexHandle);
}

/**
 * @function   capt_hal_drvGetUnLock
 * @brief      采集资源解锁
 * @param[in]  UINT32 chnId  通道号
 * @param[out] None
 * @return     static INT32
 */
static INT32 capt_hal_drvGetUnLock(UINT32 chnId)
{

    if (chnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("chnId %d\n",chnId);
        return SAL_FAIL;
    }
    
    return SAL_mutexUnlock(gstAdvInfo[chnId].MutexHandle);
}

/**
 * @function   capt_hal_drvGetFlag
 * @brief      采集标志获取
 * @param[in]  UINT32 chnId  通道号
 * @param[out] None
 * @return     INT32
 */
INT32 capt_hal_drvGetFlag(UINT32 chnId)
{
    if (chnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("chnId %d\n",chnId);
        chnId = 0;
    }
	
    return gVipipeCaptChangeFlg[chnId];
}

/**
 * @function   capt_hal_drvSetFlag
 * @brief      设置采集标志
 * @param[in]  UINT32 chnId  通道号
 * @param[in]  UINT32 flag   采集标志
 * @param[out] None
 * @return     INT32
 */
INT32 capt_hal_drvSetFlag(UINT32 chnId, UINT32 flag)
{
    if (chnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("chnId %d\n",chnId);
        chnId = 0;
    }

    capt_hal_drvGetLock(chnId);
    gVipipeCaptChangeFlg[chnId] = flag;
    capt_hal_drvGetUnLock(chnId);

    return SAL_SOK;
}


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
INT32 capt_hal_setCaptPos(UINT32 viChn, POSITION_MODE posInfo, UINT32 offset)
{
    SAL_UNUSED(viChn);
    SAL_UNUSED(posInfo);
    SAL_UNUSED(offset);
	
	return SAL_SOK;
}


/**
 * @function   capt_hal_drvSetWDRPrm
 * @brief      采集模块 配置 VI 设备宽动态属性
 * @param[in]  UINT32 uiChn     通道信息
 * @param[in]  UINT32 uiEnable  是否使能
 * @param[out] None
 * @return     INT32   SAL_SOK
 *                     SAL_FAIL
 */
INT32 capt_hal_drvSetWDRPrm(UINT32 uiChn, UINT32 uiEnable)
{
    CAPT_LOGE("capt_hal_drvSetWDRPrm unsupported !!!\n");
    return SAL_SOK;
}

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
INT32 capt_hal_checkCaptBuffer(UINT32 uiChn)
{
    return SAL_SOK;
}

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
INT32 capt_hal_putCaptBuffer(UINT32 uiChn, UINT32 uiFrameInfo)
{
    return SAL_SOK;
}

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
INT32 capt_hal_getCaptBuffer(UINT32 uiChn, UINT32 *uiFrameInfo)
{
    return SAL_SOK;
}

/**
 * @function   capt_hal_getCaptUserPicStatue
 * @brief      采集模块使用用户图片，用于无信号时编码
 * @param[in]  UINT32 uiChn  
 * @param[out] None
 * @return     状态
 */
UINT32 capt_hal_getCaptUserPicStatue(UINT32 uiChn)
{
    UINT32 status = 0;
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->getCaptUserPicStatus))
    {
        CAPT_LOGE("capt_hal_getCaptUserPicStatue Failed !!!\n");
        return SAL_FAIL;
    }

    capt_hal_drvGetLock(uiChn);
    status = pcaptHalOps->getCaptUserPicStatus(uiChn);
    capt_hal_drvGetUnLock(uiChn);
    
    return  status;
}

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
INT32 capt_hal_setCaptUserPic(UINT32 uiDev, CAPT_CHN_USER_PIC *pstUserPic)
{

    if((NULL == pstUserPic) || (NULL == pcaptHalOps) || (NULL == pcaptHalOps->setCaptUserPic))
    {
        CAPT_LOGE("capt_hal_setCaptUserPic Failed !!!\n");
        return SAL_FAIL;
    }
    
    capt_hal_drvGetLock(uiDev);
    if (SAL_FAIL == pcaptHalOps->setCaptUserPic(uiDev, pstUserPic))
    {
        CAPT_LOGE("capt_hal_setCaptUserPic Failed !!!\n");
        capt_hal_drvGetUnLock(uiDev);
        return SAL_FAIL;
    }
    capt_hal_drvGetUnLock(uiDev);
    
    return SAL_SOK;
}


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
INT32 capt_hal_enableCaptUserPic(UINT32 uiChn, UINT32 uiEnble)
{

    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->enableCaptUserPic))
    {
        CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
        return SAL_FAIL;
    }

    capt_hal_drvGetLock(uiChn);
    if (SAL_FAIL == pcaptHalOps->enableCaptUserPic(uiChn, uiEnble))
    {
        CAPT_LOGE("Capt Hal start Dev Failed !!!\n");
        capt_hal_drvGetUnLock(uiChn);
        return SAL_FAIL;
    }
    capt_hal_drvGetUnLock(uiChn);

    return SAL_SOK;
}


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
INT32 capt_hal_setCaptRotate(UINT32 uiChn, UINT32 eRotate)
{
    CAPT_ROTATION en_Rotate = 0;
    UINT32 VpssWidth = 0;
    UINT32 VpssHeight = 0;
    INT32 ret = SAL_SOK;

    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->setCaptChnRotate))
    {
        CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
        return SAL_FAIL;
    }
    capt_hal_drvGetLock(uiChn);
    switch(eRotate)
    {
        case 0:
        {
            en_Rotate = CAPT_ROTATION_0;
            VpssWidth  = 1920;
            VpssHeight = 1080;
            g_stVideoMatrixInfo.width   = VpssWidth;
            g_stVideoMatrixInfo.height  = VpssHeight;
            g_stVideoMatrixInfo.rotate  = 0;
            g_stVideoMatrixInfo.mirror  = 0;
            break;
        }
        case 1:
        {
            en_Rotate = CAPT_ROTATION_270;
            VpssWidth  = 1080;
            VpssHeight = 1920;
            g_stVideoMatrixInfo.width   = VpssWidth;
            g_stVideoMatrixInfo.height  = VpssHeight;
            g_stVideoMatrixInfo.rotate  = 1;
            g_stVideoMatrixInfo.mirror  = 0;
            break;
        }
        case 2:
        {
            en_Rotate = CAPT_ROTATION_180;
            VpssWidth  = 1920;
            VpssHeight = 1080;
            g_stVideoMatrixInfo.width   = VpssWidth;
            g_stVideoMatrixInfo.height  = VpssHeight;
            g_stVideoMatrixInfo.rotate  = 2;
            g_stVideoMatrixInfo.mirror  = 0;
            break;
        }
        case 3:
        {
            en_Rotate = CAPT_ROTATION_90;
            VpssWidth  = 1080;
            VpssHeight = 1920;
            g_stVideoMatrixInfo.width   = VpssWidth;
            g_stVideoMatrixInfo.height  = VpssHeight;
            g_stVideoMatrixInfo.rotate  = 3;
            g_stVideoMatrixInfo.mirror  = 0;
            break;
        }
        default:
        {
            ret = SAL_FAIL;
            break;
        }
    }

    if (SAL_SOK != ret)
    {
        CAPT_LOGE("Set the wrong rotate!\n");
        capt_hal_drvGetUnLock(uiChn);
        return SAL_FAIL;
    }

    /*设置VI的旋转角度*/
    ret = pcaptHalOps->setCaptChnRotate(uiChn, 0, en_Rotate);
    if (SAL_SOK != ret)
    {
        CAPT_LOGE("HI_MPI_VI_SetRotate failed with %#x!\n", ret);
        capt_hal_drvGetUnLock(uiChn);
        return SAL_FAIL;
    }
    
    if ((gVpssChnWidth != VpssWidth) || (gVpssChnHeight != VpssHeight))
    {
#if  0
        /*设置VPSS通道的分辨率*/
        ret = DupHal_drvVpssSetSizeAttr(eRotate, VpssWidth, VpssHeight);
        if (SAL_SOK != ret)
        {
            CAPT_LOGE("Dup_halVpssSetAttr failed with %#x!\n", ret);
            return SAL_FAIL;
        }
#endif
    }

    gVpssChnWidth  = VpssWidth;
    gVpssChnHeight = VpssHeight;
    gHI_Rotate = en_Rotate;
    capt_hal_drvGetUnLock(uiChn);
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hal_getCaptPipeId
 功能描述  : 获取采集pipe ID
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2022年08月09日
    作     者   : fzp
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hal_getCaptPipeId(UINT32 uiChn, INT32 *pPipeId)
{
    INT32 s32PipeID = 0;
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->GetCaptPipeId))
    {
        CAPT_LOGW("GetCaptPipeId is NULL!!!\n");
        return SAL_SOK;
    }
  
    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        return SAL_FAIL;
    }

    if (SAL_FAIL == pcaptHalOps->GetCaptPipeId(uiChn, &s32PipeID))
    {
        CAPT_LOGE("Capt Hal Get Pipe id Failed !!!\n");
        return SAL_FAIL;
    }
    *pPipeId = s32PipeID;

    return SAL_SOK;
}


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
INT32 capt_hal_getCaptStatus(UINT32 uiChn, CAPT_HAL_CHN_STATUS *pstStatus)
{

    if((NULL == pstStatus) || (NULL == pcaptHalOps) || (NULL == pcaptHalOps->getCaptDevState))
    {
        CAPT_LOGE("capt_hal_getCaptStatus Dev Failed !!!\n");
        return SAL_FAIL;
    }

    capt_hal_drvGetLock(uiChn);
  
    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        capt_hal_drvGetUnLock(uiChn);
        return SAL_FAIL;
    }

     if (SAL_FAIL == pcaptHalOps->getCaptDevState(uiChn, pstStatus))
    {
        capt_hal_drvGetUnLock(uiChn);
        CAPT_LOGD("Capt Hal start Dev Failed !!!\n");
        return SAL_FAIL;
    }

    if (gVipipeCaptChangeFlg[uiChn])
    {
       gVipipeCaptChangeFlg[uiChn] = 0;
       capt_hal_drvGetUnLock(uiChn);
       return SAL_FAIL;
    }

    capt_hal_drvGetUnLock(uiChn);

    return SAL_SOK;
}


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
INT32 capt_hal_stopCaptDev(UINT32 uiChn)
{

    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->stopCaptDev))
    {
       CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
       return SAL_FAIL;
    }
    capt_hal_drvGetLock(uiChn);
    if (SAL_FAIL == pcaptHalOps->stopCaptDev(uiChn))
    {
        CAPT_LOGE("Capt Hal start Dev Failed !!!\n");
        capt_hal_drvGetUnLock(uiChn);
        return SAL_FAIL;
    }
    
    capt_hal_drvGetUnLock(uiChn);
    return SAL_SOK;
}

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
INT32 capt_hal_startCaptDev(UINT32 uiDev)
{
    INT32 s32Ret = SAL_SOK;

    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->startCaptDev))
    {
       CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
       return SAL_FAIL;
    }

    capt_hal_drvGetLock(uiDev);
    s32Ret = pcaptHalOps->startCaptDev(uiDev);
    if (SAL_FAIL == s32Ret)
    {
        CAPT_LOGE("Capt Hal start Dev Failed !!!\n");
        
    }
    capt_hal_drvGetUnLock(uiDev);
    
    return s32Ret;
}

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
INT32 capt_hal_releaseCaptDev(UINT32 uiChn)
{
    INT32 s32Ret = SAL_SOK;

    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->deInitCaptDev))
    {
        CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
        return SAL_FAIL;
    }

    capt_hal_drvGetLock(uiChn);
    
    s32Ret = pcaptHalOps->deInitCaptDev(uiChn);
    if (SAL_FAIL == s32Ret)
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");
    }
    capt_hal_drvGetUnLock(uiChn);

    SAL_mutexDelete(gstAdvInfo[uiChn].MutexHandle);

    return s32Ret;
}

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
INT32 capt_hal_reInitCaptDev(UINT32 uiChn)
{
    CAPT_CHN_ATTR pstChnAttr;
    INT32 s32Ret = SAL_SOK;

    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->initCaptDev))
    {
       CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
       return SAL_FAIL;
    }

    capt_hal_drvGetLock(uiChn);
    memset(&pstChnAttr, 0, sizeof(CAPT_CHN_ATTR));
    
    s32Ret = pcaptHalOps->initCaptDev(uiChn, &pstChnAttr);
    if (SAL_FAIL == s32Ret)
    {
        CAPT_LOGE("Capt Hal start Dev Failed !!!\n");
    }
    capt_hal_drvGetUnLock(uiChn);
    
    return s32Ret;
}

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
INT32 capt_hal_initCaptInterface(UINT32 uiChn)
{
    INT32 s32Ret = SAL_SOK;
    
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->initCaptInterface))
    {
        CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
        return SAL_FAIL;
    }
    
    s32Ret = pcaptHalOps->initCaptInterface(uiChn);
    if (SAL_FAIL == s32Ret)
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");
    }

     return s32Ret;
}

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
INT32 capt_hal_reSetCaptInterface(UINT32 uiChn, CAPT_RESOLUTION_S *stRes)
{
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->reSetCaptInterface))
    {
        CAPT_LOGE("capt_hal_getCaptUserPicStatue Failed !!!\n");
        return SAL_FAIL;
    }
    capt_hal_drvGetLock(uiChn);
    pcaptHalOps->reSetCaptInterface(uiChn, stRes->u32Width, stRes->u32Height, stRes->u32Fps);
    capt_hal_drvGetUnLock(uiChn);

    return SAL_SOK;
}


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
INT32 capt_hal_createCaptDev(UINT32 uiChn, CAPT_CHN_ATTR *pstChnAttr)
{
   
    if((NULL == pstChnAttr) || (NULL == pcaptHalOps) 
        || (NULL == pcaptHalOps->initCaptDev))
    {
        CAPT_LOGE("Capt Hal release Dev Failed !!!\n");
        return SAL_FAIL;
    }

    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    /* 配置 vi dev */
    if (SAL_FAIL == pcaptHalOps->initCaptDev(uiChn, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");
        return SAL_FAIL;
    }

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &gstAdvInfo[uiChn].MutexHandle);

    CAPT_LOGI("Capt Hal Create Dev success uiChn = %d !!!\n", uiChn);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名 : capt_hal_captGetFrame
 功能描述 : 输入获取帧数据
 输入参数   :   uiDev 输入通道
 输出参数   :   pstFrame 帧数据
 返 回 值 : 成功:SAL_SOK
            失败:SAL_FAIL
*****************************************************************************/
INT32 capt_hal_captGetFrame(UINT32 uiDev, SYSTEM_FRAME_INFO *pstFrame)
{
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->getCaptFrame) || (NULL == pstFrame))
    {
        CAPT_LOGW("getCaptFrame Failed !!!\n");
        return SAL_FAIL;
    }
    capt_hal_drvGetLock(uiDev);
    if (SAL_FAIL == pcaptHalOps->getCaptFrame(uiDev, pstFrame, 200))
    {
        CAPT_LOGE("Capt get Frame Failed !!!\n");
        capt_hal_drvGetUnLock(uiDev);
        return SAL_FAIL;
    }
    capt_hal_drvGetUnLock(uiDev);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名 : capt_hal_captRealseFrame
 功能描述 : 输入释放帧数据
 输入参数   :   uiDev 输入通道
            pstFrame 帧数据
 输出参数   : 无
 返 回 值 : 成功:SAL_SOK
            失败:SAL_FAIL
*****************************************************************************/
INT32 capt_hal_captRealseFrame(UINT32 uiDev, SYSTEM_FRAME_INFO *pstFrame)
{
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->realseCaptFrame) || (NULL == pstFrame))
    {
        CAPT_LOGE("realseCaptFrame Failed !!!\n");
        return SAL_FAIL;
    }
    capt_hal_drvGetLock(uiDev);
    if (SAL_FAIL == pcaptHalOps->realseCaptFrame(uiDev, pstFrame))
    {
        CAPT_LOGE("Capt realse Frame Failed !!!\n");
        capt_hal_drvGetUnLock(uiDev);
        return SAL_FAIL;
    }
    capt_hal_drvGetUnLock(uiDev);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名 : capt_hal_captIsOffset
 功能描述 : 计算图像是否有偏移
 输入参数   :   uiChn 输入通道
 输出参数   : 无
 返 回 值 : 无
*****************************************************************************/
BOOL capt_hal_captIsOffset(UINT32 uiDev)
{
    BOOL offset = SAL_TRUE;
    if((NULL == pcaptHalOps) || (NULL == pcaptHalOps->checkCaptIsOffset))
    {
        CAPT_LOGE("checkCaptIsOffset Failed !!!\n");
        return SAL_FAIL;
    }
    capt_hal_drvGetLock(uiDev);
    offset = pcaptHalOps->checkCaptIsOffset(uiDev);
    capt_hal_drvGetUnLock(uiDev);

    return offset;
}


#ifdef DSP_WEAK_FUNC
/**
 * @function   capt_halRegister
 * @brief      弱函数当有平台不支持capt采集时保证编译通过
               
 * @param[in]  void  
 * @param[out] None
 * @return     __attribute__((weak)) CAPT_PLAT_OPS *
 */
__attribute__((weak)) CAPT_PLAT_OPS *capt_halRegister(void)
{
    return NULL;
}

#endif

/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 初始化disp hal层函结构体
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 capt_hal_Init(VOID)
{
    if(NULL != pcaptHalOps)
    {
        return SAL_SOK;
    }

    pcaptHalOps = capt_halRegister();

    if(NULL == pcaptHalOps)
    {
        DISP_LOGE("capt_hal_Init failed!\n");
        return SAL_SOK;
    }

    return SAL_SOK;
}


