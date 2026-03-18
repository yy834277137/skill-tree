/**
 * @file   dup_tsk_api.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  DUP是图像帧数据分发及处理模块，内部包含如下功能:
                 1. 从VI获取数据之后分发给后级多路消费者；
                 2. 从解码器获取数据之后分发给后级多路消费者；
                 3. 图像处理功能，送入DUP进行镜像、缩放、图像格式转换等处理。

                 流程:
                 1. DUP模块创建( 输入VI的视频参数 ，一个模块调用一次接口)
                 2. 获取一个dup通道(获取到dup通道及数据接口)
                 3. 拿到该dup通道的数据接口能力
                 4. 循环使用 OpDupGetBlit/OpDupPutBlit 或者 OpDupCopyBlit
                 5. 销毁整个DUP模块
 * @author yeyanzhong
 * @date   2021.6.3
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#include "dup_tsk.h"

#include "sys_tsk.h"
#include "system_prm_api.h"
#include "capbility.h"

#include "dup_tsk_api.h"
#include "capt_tsk_inter.h"

#include "link_cfg_api.h"


/*****************************************************************************
                            宏定义
*****************************************************************************/
#line __LINE__ "dup_tsk.c"

/*****************************************************************************
                            全局结构体
*****************************************************************************/
extern INST_CFG_S stFrontDupInstCfg;
extern INST_CFG_S stRearDupInstCfg;

/* vdec前级dup解码模块不需要知道，所以不需要解码模块传入；后级模块由于各个输出节点解码模块需要关心，所以由解码模块传给dup */
extern INST_CFG_S stVdecFrontDupInstCfg;


static DUP_OBJ_COMMON g_stDupObjCommon =
{
    .uiObjCnt = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
};


/*****************************************************************************
                            函数
*****************************************************************************/

/*****************************************************************************
   函 数 名  : dup_chnMutexLock
   功能描述  : Dup 模块上锁
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_objMutexLock()
{
    pthread_mutex_t *pViMutex = &g_stDupObjCommon.lock;

    pthread_mutex_lock(pViMutex);

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_objMutexUnLock
   功能描述  : Dup 模块去锁
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_objMutexUnLock()
{
    pthread_mutex_t *pViMutex = &g_stDupObjCommon.lock;

    pthread_mutex_unlock(pViMutex);

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : Dup_drviMutexInit
   功能描述  : Dup模块初始化锁
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2016年12月05日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_chnMutexInit(DUP_OBJ_PRM *pstDupChnPrm)
{
    pthread_mutex_t *pDupMutex = NULL;

    pDupMutex = &pstDupChnPrm->DupMutex;

    pthread_mutex_init(pDupMutex, NULL);

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_chnMutexLock
   功能描述  : Dup模块上锁
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2016年12月05日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_chnMutexLock(DUP_OBJ_PRM *pstDupChnPrm)
{
    pthread_mutex_t *pDupMutex = NULL;

    pDupMutex = &pstDupChnPrm->DupMutex;

    pthread_mutex_lock(pDupMutex);

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_chnMutexUnLock
   功能描述  : Dup模块去锁
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2016年12月05日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_chnMutexUnLock(DUP_OBJ_PRM *pstDupChnPrm)
{
    pthread_mutex_t *pDupMutex = NULL;

    pDupMutex = &pstDupChnPrm->DupMutex;

    pthread_mutex_unlock(pDupMutex);

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_opGetChnFrm
   功能描述  : Dup 模块数据的获取
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 dup_getChnPrm(DUP_ChanHandle *pUsrArgs, DUP_OBJ_PRM **ppstDupObjPrm)
{
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    /* SYSTEM_FRAME_INFO *pstSystemFrameInfo = NULL; */

    DUP_ChanHandle *pstDupChnPrm = NULL;
    UINT32 uiModChn = 0;

    pstDupChnPrm = (DUP_ChanHandle *)pUsrArgs;
    uiModChn = sys_task_getModChn(pstDupChnPrm->dupModule);
    pstDupObjPrm = g_stDupObjCommon.pstDupObj[uiModChn];

    *ppstDupObjPrm = pstDupObjPrm;

    return SAL_SOK;
}

/**
 * @function    dup_getDupObj
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 dup_getDupObj(UINT32 objChn, DUP_OBJ_PRM **ppstDupObjPrm)
{
    DUP_OBJ_PRM *pstDupObjPrm = NULL;

    pstDupObjPrm = g_stDupObjCommon.pstDupObj[objChn];
    if (NULL == pstDupObjPrm)
    {
        return SAL_FAIL;
    }

    *ppstDupObjPrm = pstDupObjPrm;

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_opGetChnFrm
   功能描述  : Dup 模块数据的获取
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_opGetChnFrm(Ptr pUsrArgs, Ptr pDstBuf)
{
    DUP_ChanHandle *pChanHandle = NULL;
    UINT32 ret = SAL_SOK;
    DUP_COPY_DATA_BUFF *pstDupDstBuf = NULL;

    if (NULL == pUsrArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        DUP_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    pChanHandle = (DUP_ChanHandle *)pUsrArgs;
    pstDupDstBuf = (DUP_COPY_DATA_BUFF *)pDstBuf;
    /* pstDataCopyPrm = &pstDupDstBuf->stDupDataCopyPrm; */

    if (NULL == pstDupDstBuf->pstDstSysFrame)
    {
        DUP_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    ret = dup_hal_getChnFrame(pChanHandle->u32Grp, pChanHandle->u32Chn, (SYSTEM_FRAME_INFO *)pstDupDstBuf->pstDstSysFrame);
    if (SAL_SOK != ret)
    {
        /* SAL_ERROR("error !!!\n"); */
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_opPutChnFrm
   功能描述  : Dup 模块数据释放
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_opPutChnFrm(Ptr pUsrArgs, Ptr pDstBuf)
{
    DUP_ChanHandle *pChanHandle = NULL;
    DUP_COPY_DATA_BUFF *pstDupDstBuf = NULL;
    INT32 ret = SAL_SOK;

    if (NULL == pUsrArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        DUP_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    pChanHandle = (DUP_ChanHandle *)pUsrArgs;
    pstDupDstBuf = (DUP_COPY_DATA_BUFF *)pDstBuf;
    /* pstDataCopyPrm = &pstDupDstBuf->stDupDataCopyPrm; */

    if (NULL == pstDupDstBuf->pstDstSysFrame)
    {
        DUP_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    ret = dup_hal_putChnFrame(pChanHandle->u32Grp, pChanHandle->u32Chn, (SYSTEM_FRAME_INFO *)pstDupDstBuf->pstDstSysFrame);
    if (ret == SAL_FAIL)
    {
        SAL_ERROR("Put Frame error !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_opCopyChnFrm
   功能描述  : Dup 模块数据拷贝
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_opCopyChnFrm(Ptr pUsrArgs, Ptr pDstBuf)
{
#if 0
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    DUP_ChanHandle *pChanHandle = NULL;
    INT32 ret = SAL_SOK;

    SYSTEM_FRAME_INFO stSrcSysFrame;
    SYSTEM_FRAME_INFO *pstSrcSysFrame = NULL;
    SYSTEM_FRAME_INFO *pstDstSysFrame = NULL;

    VIDEO_FRAME_INFO_S stVideoFrame;

    memset(&stSrcSysFrame, 0, sizeof(stSrcSysFrame));
    stSrcSysFrame.uiAppData = (PhysAddr) & stVideoFrame;

    DUP_COPY_DATA_BUFF *pstDupDstBuf = NULL;
    DUP_DATA_COPY_PRM *pstDataCopyPrm = NULL;

    VPSS_CROP_INFO_S stDispCrop;

    if (NULL == pUsrArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        DUP_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    pChanHandle = (DUP_ChanHandle *)pUsrArgs;

    /* 获取通道属性 */
    if (SAL_SOK != dup_getChnPrm(pChanHandle, &pstDupObjPrm))
    {
        DUP_LOGE("Dup Get Chn Prm Failed !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SystemPrm_getIsHisiPlatform())
    {
        if (!(pChanHandle->uiDupWIdx > 0))
        {
            DUP_LOGE("pChanHandle->uiDupWIdx %d !!!\n", pChanHandle->uiDupWIdx);
            return SAL_FAIL;
        }
    }

    pstDupDstBuf = (DUP_COPY_DATA_BUFF *)pDstBuf;
    pstDataCopyPrm = &pstDupDstBuf->stDupDataCopyPrm;

    if (NULL == pstDupDstBuf->pstDstSysFrame)
    {
        DUP_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    /* SAL_ERROR("pChanHandle->nMgicNum %d uiNeedCrop %d !!!\n", pChanHandle->nMgicNum, pstDataCopyPrm->uiNeedCrop); */
    /* 此处要分平台 */
    if (SAL_SOK == SystemPrm_getIsHisiPlatform())
    {
        if (1 == pstDataCopyPrm->uiNeedCrop)
        {
            /* 先配置属性 */
            memset(&stDispCrop, 0, sizeof(VPSS_CROP_INFO_S));
            stDispCrop.bEnable = 1;
            stDispCrop.stCropRect.s32X = pstDataCopyPrm->uiX;
            stDispCrop.stCropRect.s32Y = pstDataCopyPrm->uiY;;
            stDispCrop.stCropRect.u32Width = pstDataCopyPrm->uiDstWidth;
            stDispCrop.stCropRect.u32Height = pstDataCopyPrm->uiDstHeight;
            stDispCrop.enCropCoordinate = VPSS_CROP_ABS_COOR;

            DupHal_drvVpssSetChnCrop(pChanHandle->u32Grp, pChanHandle->u32Chn, stDispCrop.bEnable, stDispCrop.stCropRect.s32X, stDispCrop.stCropRect.s32Y, stDispCrop.stCropRect.u32Width, stDispCrop.stCropRect.u32Height);
        }

        if (1 == pstDataCopyPrm->uiNeedScale)
        {
            DupHal_drvVpssSetSizeAttr(pChanHandle->u32Grp, pChanHandle->u32Chn,
                                      pstDataCopyPrm->uiDstWidth, pstDataCopyPrm->uiDstHeight);
        }

        /* 先配置属性 */

        /* DUP_LOGI("uiVpssGrp %d VpssChn %d uiRotate %d !!!\n", pstHalVpssGrpPrm->stHalVpssGrpPrm0.uiVpssChn, pChanHandle->nMgicNum,pstDataCopyPrm->uiRotate); */
        if (pstDataCopyPrm->uiRotate < 4)
        {
            DupHal_drvVpssSetRotate(pChanHandle->u32Grp, pChanHandle->u32Chn, pstDataCopyPrm->uiRotate);
        }

        ret = dup_hal_getChnFrame(pChanHandle->u32Grp, pChanHandle->u32Chn, &stSrcSysFrame);
        if (SAL_SOK != ret)
        {
            DUP_LOGE("error !!!\n");
            return SAL_FAIL;
        }

        ret = HAL_bufQuickCopy(&stSrcSysFrame, pstDupDstBuf->pstDstSysFrame, 0, 0, stVideoFrame.stVFrame.u32Stride[0], stVideoFrame.stVFrame.u32Height);
        if (SAL_SOK != ret)
        {
            DUP_LOGE("error !!!\n");
            return SAL_FAIL;
        }

        ret = dup_hal_putChnFrame(pChanHandle->u32Grp, pChanHandle->u32Chn, &stSrcSysFrame);
        if (SAL_SOK != ret)
        {
            DUP_LOGE("error !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        /* 直接取最新的一帧 */
        pstSrcSysFrame = pChanHandle->pastInQue[(pChanHandle->uiDupWIdx - 1) % DUP_MAX_BUFFER];
        pstDstSysFrame = (SYSTEM_FRAME_INFO *)pstDupDstBuf->pstDstSysFrame;
        ret = HAL_bufQuickCopy(pstSrcSysFrame, pstDstSysFrame, 0, 0, 1920, 1080);
        if (SAL_SOK != ret)
        {
            DUP_LOGE("error !!!\n");
            return SAL_FAIL;
        }
    }
#endif
    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_opChnBind
   功能描述  : Dup 模块绑定
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_opChnBind(Ptr pUsrArgs, Ptr pDstBuf, UINT32 isBind)
{
    DUP_ChanHandle *pChanHandle = NULL;
    UINT32 uiMod = 0;
    UINT32 dupChn = 0;
    DUP_BIND_PRM *pstDupBindPrm = NULL;

    if (NULL == pUsrArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        DUP_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    pChanHandle = (DUP_ChanHandle *)pUsrArgs;
    /* 绑定 */
    pstDupBindPrm = (DUP_BIND_PRM *)pDstBuf;

    uiMod = pChanHandle->u32Grp;
    dupChn = pChanHandle->u32Chn;

    if (1 == isBind)
    {
        if (SAL_SOK != sys_hal_SDKbind(SYSTEM_MOD_ID_DUP, uiMod, dupChn, pstDupBindPrm->mod, pstDupBindPrm->modChn, pstDupBindPrm->chn))
        {
            DUP_LOGE("Dup Vpss Bind to %d Dest %d Error !!!\n", pstDupBindPrm->mod, pstDupBindPrm->modChn);
            return SAL_FAIL;
        }
    }
    else
    {
        if (SAL_SOK != sys_hal_SDKunbind(SYSTEM_MOD_ID_DUP, uiMod, dupChn, pstDupBindPrm->mod, pstDupBindPrm->modChn, pstDupBindPrm->chn))
        {
            DUP_LOGE("Dup Vpss unBind from %d Dest %d Error !!!\n", pstDupBindPrm->mod, pstDupBindPrm->modChn);
            return SAL_FAIL;
        }

    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_opSetChnPrm
   功能描述  : Dup 模块设置属性
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_opSetChnPrm(Ptr pInArgs, PARAM_INFO_S *pstParamInfo)
{
    DUP_ChanHandle *pstDupChnPrm = NULL;

    if (NULL == pInArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pstParamInfo)
    {
        DUP_LOGE("pstParamInfo is NULL !!!\n");
        return SAL_FAIL;
    }

    pstDupChnPrm = (DUP_ChanHandle *)pInArgs;
    if (SAL_SOK != dup_hal_setDupParam(pstDupChnPrm->u32Grp, pstDupChnPrm->u32Chn, pstParamInfo))
    {
        DUP_LOGE("Dup set Chn Prm Failed !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    dup_opGetChnPrm
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 dup_opGetChnPrm(Ptr pInArgs, PARAM_INFO_S *pstParamInfo)
{
    DUP_ChanHandle *pstDupChnPrm = NULL;

    if (NULL == pInArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pstParamInfo)
    {
        DUP_LOGE("pstParamInfo is NULL !!!\n");
        return SAL_FAIL;
    }

    pstDupChnPrm = (DUP_ChanHandle *)pInArgs;
    if (SAL_SOK != dup_hal_getDupParam(pstDupChnPrm->u32Grp, pstDupChnPrm->u32Chn, pstParamInfo))
    {
        DUP_LOGE("Dup set Chn Prm Failed !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    dup_opGetChnPrm
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 dup_opStartChn(Ptr pInArgs)
{
    DUP_ChanHandle *pstDupChnPrm = NULL;

    if (NULL == pInArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    pstDupChnPrm = (DUP_ChanHandle *)pInArgs;
    if (SAL_SOK != dup_hal_startChn(pstDupChnPrm->u32Grp, pstDupChnPrm->u32Chn))
    {
        DUP_LOGE("Dup dis/enable Chn failed,        !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    dup_opGetChnPrm
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 dup_opStopChn(Ptr pInArgs)
{
    DUP_ChanHandle *pstDupChnPrm = NULL;

    if (NULL == pInArgs)
    {
        DUP_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    pstDupChnPrm = (DUP_ChanHandle *)pInArgs;
    if (SAL_SOK != dup_hal_stopChn(pstDupChnPrm->u32Grp, pstDupChnPrm->u32Chn))
    {
        DUP_LOGE("Dup dis/enable Chn failed,        !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


#if 0
/*****************************************************************************
   函 数 名  : Dup_sendDataInfo
   功能描述  : Dup 模块向太空服输出通道送入数据信息
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Dup_sendDataInfo(DUP_ChanHandle *pUsrArgs, Ptr pDstBuf)
{
    DUP_ChanHandle *pstDupChnPrm = NULL;

    if (NULL == pUsrArgs)
    {
        DUP_LOGE("Chan Handle is NULL !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        DUP_LOGE("Dst Buf is NULL !!!\n");
        return SAL_FAIL;
    }

    pstDupChnPrm = (DUP_ChanHandle *)pUsrArgs;

    pstDupChnPrm->pastInQue[pstDupChnPrm->uiDupWIdx % DUP_MAX_BUFFER] = pDstBuf;

    if (0 == pstDupChnPrm->uiDupWIdx % (100 * DUP_MAX_BUFFER))
    {
        pstDupChnPrm->uiDupWIdx = 0;
    }

    pstDupChnPrm->uiDupWIdx = pstDupChnPrm->uiDupWIdx + 1;

    return SAL_SOK;
}


/*****************************************************************************
   函 数 名  : Dup_tskThrMain
   功能描述  : Dup 模块任务处理线程
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
void *Dup_tskThrMain(void *pPrm)
{
    /* SYSTEM_FRAME_INFO *pstTempFrameInfo   = NULL; */
    SYSTEM_FRAME_INFO *pstSystemFrameInfo = NULL;
    DUP_OBJ_PRM *pstDupObjPrm = (DUP_OBJ_PRM *)pPrm;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;
    INT32 i = 0;
    /* UINT32 uiCnt = 0; */
    INT32 iRet = 0;

    SAL_SET_THR_NAME();

    usleep(2000000);
    while (1)
    {
        /* 1. 归还数据缓冲区 队列里保留一帧 */
        while (DSA_QueGetQueuedCount(&pstDupObjPrm->pstInFullQue) > 2)
        {
            /* 获取缓冲区 */
            DSA_QueGet(&pstDupObjPrm->pstInFullQue, (void **)&pstSystemFrameInfo, SAL_TIMEOUT_FOREVER);

            /* 需要释放数据 */
            pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;
            if (HI_SUCCESS != (iRet = HI_MPI_VPSS_ReleaseChnFrame(pstDupObjPrm->uiModChn, 0, pstVideoFrame)))
            {
                DUP_LOGE("4444444 \n");
            }

            /* 归还缓冲区 */
            DSA_QuePut(&pstDupObjPrm->pstInEmptQue, (void *)pstSystemFrameInfo, SAL_TIMEOUT_NONE);
        }

        /* 2. 获取数据内存 */
        DSA_QueGet(&pstDupObjPrm->pstInEmptQue, (void **)&pstSystemFrameInfo, SAL_TIMEOUT_FOREVER);

        /* 3. 获取前端的数据 */
        pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;

        if (HI_SUCCESS != (iRet = HI_MPI_VPSS_GetChnFrame(pstDupObjPrm->uiModChn, 0, pstVideoFrame, 200)))
        {
            DUP_LOGE("33333333333333333 \n");
        }

        /* 4. 送入各输出通道 */
        for (i = 0; i < pstDupObjPrm->uiOutCnt; i++)
        {
            if (NODE_BIND_TYPE_SDK_BIND != pstDupObjPrm->pChanHandle[i]->createFlags)
            {
                Dup_sendDataInfo(pstDupObjPrm->pChanHandle[i], pstSystemFrameInfo);
            }
        }

        /* 5. 放入队列 */
        DSA_QuePut(&pstDupObjPrm->pstInFullQue, (void *)pstSystemFrameInfo, SAL_TIMEOUT_NONE);

        SAL_msleep(10);
    }
}
#endif
/*****************************************************************************
   函 数 名  : dup_objQueInit
   功能描述  : Dup tsk obj Chn 通道初始化
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年10月09日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_objQueInit(DUP_OBJ_PRM *pstDupObjPrm)
{
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    SYSTEM_FRAME_INFO *pstSystemFrameInfo = NULL;

    /* UINT32 i = 0; */
    UINT32 j = 0;

    if (NULL == pstDupObjPrm)
    {
        DUP_LOGE("Dup Tsk Obj is NULL !!!\n");
        return SAL_FAIL;
    }

    pstDupObjPrm->uiDupWIdx = 0;

    pstFullQue = &pstDupObjPrm->pstInFullQue;
    if (SAL_SOK != DSA_QueCreate(pstFullQue, DUP_MAX_BUFFER))
    {
        DUP_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstEmptQue = &pstDupObjPrm->pstInEmptQue;
    if (SAL_SOK != DSA_QueCreate(pstEmptQue, DUP_MAX_BUFFER))
    {
        DUP_LOGE("!!!\n");
        return SAL_FAIL;
    }

    for (j = 0; j < DUP_MAX_BUFFER; j++)
    {
        pstSystemFrameInfo = &pstDupObjPrm->astInQue[j];

        if (SAL_SOK != DSA_QuePut(pstEmptQue, (void *)pstSystemFrameInfo,
                                  SAL_TIMEOUT_NONE))
        {
            DUP_LOGE("!!!\n");
            return SAL_FAIL;
        }
    }

    /* 初始化锁 */
    dup_chnMutexInit(pstDupObjPrm);

    return SAL_SOK;
}

static INT32 dup_objQueDeinit(DUP_OBJ_PRM *pstDupObjPrm)
{
    
    DSA_QueDelete(&pstDupObjPrm->pstInEmptQue);

    DSA_QueDelete(&pstDupObjPrm->pstInFullQue);

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : dup_dupObjInit
   功能描述  : Dup tsk obj 初始化
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年10月09日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_dupObjInit(UINT32 uiDupIdx)
{
    DUP_OBJ_PRM *pstObjPrm = NULL;

    DUP_LOGD("1.Dup tsk create start !!!\n");

    pstObjPrm = SAL_memMalloc(sizeof(DUP_OBJ_PRM), "dup_tsk", "dup_obj");
    if (NULL == pstObjPrm)
    {
        DUP_LOGE("Dup Obj Alloc Failed !!!\n");

        return SAL_FAIL;
    }

    memset(pstObjPrm, 0, sizeof(DUP_OBJ_PRM));

    g_stDupObjCommon.pstDupObj[uiDupIdx] = pstObjPrm;

    DUP_LOGD("2.Dup %d tsk create done !!!\n", uiDupIdx);

    return SAL_SOK;
}

static INT32 dup_dupObjDeinit(UINT32 uiDupIdx)
{
    DUP_OBJ_PRM *pstObjPrm = NULL;

    pstObjPrm = g_stDupObjCommon.pstDupObj[uiDupIdx];
    
    SAL_memfree((Ptr)pstObjPrm, "dup_tsk", "dup_obj");

    g_stDupObjCommon.pstDupObj[uiDupIdx] = NULL;

    return SAL_SOK;
}


/*****************************************************************************
   函 数 名  : dup_createChan
   功能描述  : Dup 模块通道销毁
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 dup_createChan(DUP_ChanCreate *pChanArgs, DUP_ChanHandle **pDupChanHandle)
{
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    DUP_ChanHandle *pChanHandle = NULL;

    UINT32 uiCur = 0;
    UINT32 uiModChn = 0;

    if (NULL == pChanArgs)
    {
        DUP_LOGE("Obj Handle is NULL !!!\n");
        return SAL_FAIL;
    }

    /* 获取通道属性 */
    uiModChn = sys_task_getModChn(pChanArgs->dupModule);
    pstDupObjPrm = g_stDupObjCommon.pstDupObj[uiModChn];

    dup_chnMutexLock(pstDupObjPrm);

    uiCur = pstDupObjPrm->uiOutCnt;
    pstDupObjPrm->uiOutCnt++;

    dup_chnMutexUnLock(pstDupObjPrm);

    pChanHandle = (DUP_ChanHandle *)SAL_memZalloc(sizeof(DUP_ChanHandle), "dup_tsk", "dup");
    if (NULL == pChanHandle)
    {
        DUP_LOGE("Dup Obj Create Out Chn Failed !!!\n");
        return SAL_FAIL;
    }

    pChanHandle->nMgicNum = uiCur;
    pstDupObjPrm->uiOutChn++;

    pChanHandle->dupModule = pChanArgs->dupModule;
    pChanHandle->fps = pChanArgs->fps;

    /* 此处需要更多的判断条件 */
    pChanHandle->createFlags = pChanArgs->createFlags;

    pChanHandle->dupOps.OpDupCopyBlit = dup_opCopyChnFrm;
    pChanHandle->dupOps.OpDupGetBlit = dup_opGetChnFrm;
    pChanHandle->dupOps.OpDupPutBlit = dup_opPutChnFrm;
    pChanHandle->dupOps.OpDupSetBlitPrm = dup_opSetChnPrm;
    pChanHandle->dupOps.OpDupGetBlitPrm = dup_opGetChnPrm;
    pChanHandle->dupOps.OpDupStartBlit = dup_opStartChn;
    pChanHandle->dupOps.OpDupStopBlit = dup_opStopChn;

    if (NODE_BIND_TYPE_SDK_BIND == pChanHandle->createFlags)
    {
        pChanHandle->dupOps.OpDupBindBlit = dup_opChnBind;
        /* pChanHandle->dupOps.OpDupCopyBlit   = NULL; */
    }
    else
    {
        pChanHandle->dupOps.OpDupCopyBlit = dup_opCopyChnFrm;
        pChanHandle->dupOps.OpDupBindBlit = NULL;
    }

    pstDupObjPrm->pChanHandle[uiCur] = pChanHandle;
    *pDupChanHandle = pChanHandle;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Dup_HalSetDbLeave
* 描  述  :
* 输  入  : 无
* 输  出  : 无
* 返回值  :
*******************************************************************************/
void dup_task_setDbLeave(INT32 level)
{
    dup_hal_setDbLevel(level);
}

static INT32 dup_getFreeDupObjIdx(UINT32 *u32FreeDupIdx)
{
    INT32 i = 0;
    
    dup_objMutexLock();

    for (i = 0; i < DUP_MAX_OBJ; i++)
    {
        if (BIT_IS_SET(g_stDupObjCommon.u64DupObjBeUsed, i, UINT64))   /*记录已经使用的节点使用的被置位*/
        {
            continue;
        }
        else
        {
            BIT_SET(g_stDupObjCommon.u64DupObjBeUsed, i, UINT64);
            *u32FreeDupIdx = i;
            dup_objMutexUnLock();
            return SAL_SOK;
        }
    }
    DUP_LOGE(" DUP obj are all busy.\n");
    dup_objMutexUnLock();
    
    return SAL_FAIL;    
}

static INT32 dup_freeDupObjIdx(UINT32 u32FreeDupIdx)
{
    dup_objMutexLock();

    BIT_CLEAR(g_stDupObjCommon.u64DupObjBeUsed, u32FreeDupIdx, UINT64);

    dup_objMutexUnLock();
    return SAL_SOK;  
}

/**
 * @function    dup_create
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static DupHandle dup_create(CHAR *szInstName, NODE_CFG_S *pstNodeCfg, UINT32 u32NodeNum, HAL_VPSS_GRP_PRM *pstHalVpssGrpPrmInput)
{
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    UINT32 uiObjIdx = 0;
    DUP_ChanCreate stDupChnCreatePrm;
    DUP_ChanHandle *pDupChanHandle = NULL;
    INST_INFO_S *pstInst = NULL;
    UINT32 i = 0;
    HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm;
    UINT32 u32dupPhysChnNum = 0;
    DupHandle dupHandle;
    /* SYSTEM_FRAME_INFO *pstSystemFrameInfo = NULL; */
    //CAPB_LINE *capb_line = capb_get_line();
    //CAPB_OSD *capb_osd = capb_get_osd();

    g_stDupObjCommon.uiObjCnt++;

    if (SAL_SOK != dup_getFreeDupObjIdx(&uiObjIdx))
    {
        DUP_LOGE("dup_getFreeDupObjIdx Failed !!!\n");
        return SAL_FAIL;
    }

    
    /* 1.obj 的创建 */
    if (SAL_SOK != dup_dupObjInit(uiObjIdx))
    {
        DUP_LOGE("Dup_tskObjChnCreate Failed !!!\n");
        return SAL_FAIL;
    }

    pstDupObjPrm = g_stDupObjCommon.pstDupObj[uiObjIdx];

    /* 2.obj 的输入队列初始化 */
    if (SAL_SOK != dup_objQueInit(pstDupObjPrm))
    {

        DUP_LOGE("Dup_tskObjChn Init Failed !!!\n");
        return SAL_FAIL;
    }

    pstDupObjPrm->uiModChn = uiObjIdx;
    pstDupObjPrm->uiOutChn = 0;


    dupHandle = sys_task_makeTskID(SYSTEM_MOD_ID_DUP, uiObjIdx);

    pstHalVpssGrpPrm = &pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0;
    pstHalVpssGrpPrm->uiGrpWidth = pstHalVpssGrpPrmInput->uiGrpWidth;
    pstHalVpssGrpPrm->uiGrpHeight = pstHalVpssGrpPrmInput->uiGrpHeight;
    pstHalVpssGrpPrm->uiChnDepth = pstHalVpssGrpPrmInput->uiChnDepth;
    pstHalVpssGrpPrm->u32BlkCnt = pstHalVpssGrpPrmInput->u32BlkCnt;
    pstHalVpssGrpPrm->enVpssDevType = pstHalVpssGrpPrmInput->enVpssDevType;
    pstHalVpssGrpPrm->enInputSalPixelFmt = pstHalVpssGrpPrmInput->enInputSalPixelFmt;
    pstHalVpssGrpPrm->enOutputSalPixelFmt = pstHalVpssGrpPrmInput->enOutputSalPixelFmt;

    pstInst = link_drvReqstInst();
    if (NULL == pstInst)
    {
        DUP_LOGE("link_drvReqstInst Failed !!!\n");
        return SAL_FAIL;
    }
    pstDupObjPrm->pstInst = pstInst;
    /* snprintf(szInstName, NAME_LEN, szInstName); */
    link_drvInitInst(pstInst, szInstName);

    for (i = 0; i < u32NodeNum; i++)
    {
        link_drvInitNode(pstInst, &pstNodeCfg[i]);
        if ((pstNodeCfg[i].s32NodeIdx >= OUT_NODE_0) && (pstNodeCfg[i].s32NodeIdx <= NODE_NUM_MAX))
        {
            memset(&stDupChnCreatePrm, 0, sizeof(DUP_ChanCreate));
            stDupChnCreatePrm.createFlags = pstNodeCfg[i].enBindType;
            stDupChnCreatePrm.dupModule = dupHandle;
            dup_createChan(&stDupChnCreatePrm, &pDupChanHandle);
            link_drvAttachToNode(&(pstInst->stNode[pstNodeCfg[i].s32NodeIdx]), pDupChanHandle);
        }
    }

    u32dupPhysChnNum = dup_hal_getPhysChnNum();
    /* 物理通道通过 (总的输出node数 - dup初始化接口里配置的扩展通道数)计算而得 */
    pstHalVpssGrpPrm->uiPhyChnNum = pstDupObjPrm->uiOutCnt - pstHalVpssGrpPrmInput->uiExtChnNum;
    if (pstHalVpssGrpPrm->uiPhyChnNum > u32dupPhysChnNum)
    {
        DUP_LOGW(" out total chn %d, pstDupObjPrm->uiPhyChnNum %d exceed SOC support physChnNum %d \n", pstDupObjPrm->uiOutCnt, pstHalVpssGrpPrm->uiPhyChnNum, u32dupPhysChnNum);
        pstHalVpssGrpPrm->uiPhyChnNum = u32dupPhysChnNum;
    }
    for (i = 0; i < pstHalVpssGrpPrm->uiPhyChnNum; i++)
    {
        pstHalVpssGrpPrm->uiChnEnable[i] = ENABLED;
    }

    /* 扩展通道使能配置 */
    if (pstHalVpssGrpPrmInput->uiExtChnNum > dup_hal_getExtChnNum())
    {
        DUP_LOGW("  pstHalVpssGrpPrmInput->uiExtChnNum %d exceed SOC support physExtChnNum %d \n", pstHalVpssGrpPrmInput->uiExtChnNum, dup_hal_getExtChnNum());
        pstHalVpssGrpPrm->uiExtChnNum = dup_hal_getExtChnNum();
    }
    else
    {
        pstHalVpssGrpPrm->uiExtChnNum = pstHalVpssGrpPrmInput->uiExtChnNum;
    }
    for (i = 0; i < pstHalVpssGrpPrm->uiExtChnNum; i++)
    {
        pstHalVpssGrpPrm->uiChnEnable[i + u32dupPhysChnNum] = ENABLED;
    }

    dup_hal_initDupGroup(pstHalVpssGrpPrm);

    /* 物理通道号和扩展通道号保存到pchanHandle,主要是chan相关接口调用时需要用到 */
    for (i = 0; i < pstDupObjPrm->uiOutCnt; i++)
    {
        if (i < pstHalVpssGrpPrm->uiPhyChnNum)
        {
            pstDupObjPrm->pChanHandle[i]->u32Grp = pstHalVpssGrpPrm->uiVpssChn;
            pstDupObjPrm->pChanHandle[i]->u32Chn = i;
        }
        else
        {
            pstDupObjPrm->pChanHandle[i]->u32Grp = pstHalVpssGrpPrm->uiVpssChn;
            pstDupObjPrm->pChanHandle[i]->u32Chn = (i - pstHalVpssGrpPrm->uiPhyChnNum) + u32dupPhysChnNum;
        }

        /*
        #if defined(ss928) && defined(SVA_USE_EXT_CHN)
        if (((pstHalVpssGrpPrm->uiVpssChn == 0) || (pstHalVpssGrpPrm->uiVpssChn == 2)) && (i == 2))
        {            
            pstDupObjPrm->pChanHandle[i]->u32Chn = u32dupPhysChnNum + pstHalVpssGrpPrm->uiVpssChn;
        }
        #endif
        */
    }

    return dupHandle;

}

/**
 * @function    dup_task_dupDestroy
 * @brief       销毁dup
 * @param[in]   dupHandle dup创建时的handle
 * @param[out]  
 * @return
 */
static INT32 dup_destroy( DupHandle dupHandle)
{
    UINT32 u32ObjIdx = 0;
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm = NULL;
    
    u32ObjIdx = sys_task_getModChn(dupHandle);

    pstDupObjPrm = g_stDupObjCommon.pstDupObj[u32ObjIdx];

    pstHalVpssGrpPrm = &pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0;

    dup_hal_deinitDupGroup(pstHalVpssGrpPrm);

    link_drvFreeInst(pstDupObjPrm->pstInst);
 
    dup_objQueDeinit(pstDupObjPrm);

    dup_dupObjDeinit(u32ObjIdx);

    dup_freeDupObjIdx(u32ObjIdx);

    return SAL_SOK;
}

/**
 * @function    dup_task_bindToDup
 * @brief       绑定到dup group，比如可以把解码器vdec绑定到group
 * @param[in]   pSrcBuf 绑定源
                dupHandle 目标dupHandle
                isBind，1为绑定，0为解绑
 * @param[out]
 * @return
 */
INT32 dup_task_bindToDup(DUP_BIND_PRM *pSrcBuf, DupHandle dupHandle, UINT32 isBind)
{
    INT32 s32Ret = SAL_FAIL;
    DUP_BIND_PRM *pstDupBindPrm = NULL;
    UINT32 u32ModChn = 0;
    DUP_OBJ_PRM *pstDupObjPrm = NULL;

    pstDupBindPrm = (DUP_BIND_PRM *)pSrcBuf;

    u32ModChn = sys_task_getModChn(dupHandle);

    s32Ret = dup_getDupObj(u32ModChn, &pstDupObjPrm);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (isBind)
    {
        if (SYSTEM_MOD_ID_VDEC == pstDupBindPrm->mod)
        {
            s32Ret = sys_hal_SDKbind(SYSTEM_MOD_ID_VDEC, 0, pstDupBindPrm->chn, SYSTEM_MOD_ID_DUP, pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0);
            DUP_LOGI("mod %d chn %d Bind to     dup %d   !!!\n", pstDupBindPrm->mod, pstDupBindPrm->chn,  pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn);
        }
        else if (SYSTEM_MOD_ID_CAPT == pstDupBindPrm->mod)
        {
            s32Ret = sys_hal_SDKbind(SYSTEM_MOD_ID_CAPT, pstDupBindPrm->modChn, 0, SYSTEM_MOD_ID_DUP, pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0);
            DUP_LOGI("mod %d chn %d Bind to     dup %d   !!!\n", pstDupBindPrm->mod, pstDupBindPrm->modChn,  pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn);
        }
        else
        {
            DUP_LOGE("mod %d modChn %d Bind to     dup %d   Error !!!\n", pstDupBindPrm->mod, pstDupBindPrm->modChn,  pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn);
            return SAL_FAIL;
        }
    }
    else
    {
        if (SYSTEM_MOD_ID_VDEC == pstDupBindPrm->mod)
        {
            s32Ret = sys_hal_SDKunbind(SYSTEM_MOD_ID_VDEC, 0, pstDupBindPrm->chn, SYSTEM_MOD_ID_DUP, pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0);
            DUP_LOGI("mod %d chn %d unBind from      dup %d   !!!\n", pstDupBindPrm->mod, pstDupBindPrm->chn,  pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn);
        }
        else if (SYSTEM_MOD_ID_CAPT == pstDupBindPrm->mod)
        {
            s32Ret = sys_hal_SDKunbind(SYSTEM_MOD_ID_CAPT, pstDupBindPrm->modChn, 0, SYSTEM_MOD_ID_DUP, pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0);
            DUP_LOGI("mod %d chn %d unBind to     dup %d   !!!\n", pstDupBindPrm->mod, pstDupBindPrm->modChn,  pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn);
        }
        else
        {
            DUP_LOGE("mod %d modChn %d unBind from      dup %d   Error !!!\n", pstDupBindPrm->mod, pstDupBindPrm->modChn,  pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn);
            return SAL_FAIL;
        }

    }

    return s32Ret;
}

/**
 * @function    dup_task_sendToDup
 * @brief       发送图像帧到DUP group
 * @param[in]   dupHandle 发送图像帧的目的dup
                pstFrame 图像帧
 * @param[out]
 * @return
 */
INT32 dup_task_sendToDup(DupHandle dupHandle, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32ModChn = 0;
    DUP_OBJ_PRM *pstDupObjPrm = NULL;

    u32ModChn = sys_task_getModChn(dupHandle);

    s32Ret = dup_getDupObj(u32ModChn, &pstDupObjPrm);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;;
    }

    return dup_hal_sendDupFrame(pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, pstFrame);

}

/**
 * @function    dup_task_setDupParam
 * @brief       设置dup group的参数
 * @param[in]   dupHandle 需要配置参数的dup handle
                pstParamInfo 参数结构体，具体配置什么参数通过CFG_TYPE_E区别
 * @param[out]
 * @return
 */
INT32 dup_task_setDupParam(DupHandle dupHandle, PARAM_INFO_S *pstParamInfo)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32ModChn = 0;
    DUP_OBJ_PRM *pstDupObjPrm = NULL;

    u32ModChn = sys_task_getModChn(dupHandle);

    s32Ret = dup_getDupObj(u32ModChn, &pstDupObjPrm);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("dup_getDupObj failed, dupHandle: 0x%x, ModChn: 0x%x\n", dupHandle, u32ModChn);
        return SAL_FAIL;;
    }

    return dup_hal_setDupParam(pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0, pstParamInfo);
}

/**
 * @function    dup_task_getDupParam
 * @brief       获取dup group的参数
 * @param[in]   dupHandle 需要获取参数的dup handle
 * @param[out]  pstParamInfo 参数结构体，具体获取什么参数通过CFG_TYPE_E区别
 * @return
 */
INT32 dup_task_getDupParam(DupHandle dupHandle, PARAM_INFO_S *pstParamInfo)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32ModChn = 0;
    DUP_OBJ_PRM *pstDupObjPrm = NULL;

    u32ModChn = sys_task_getModChn(dupHandle);

    s32Ret = dup_getDupObj(u32ModChn, &pstDupObjPrm);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;;
    }

    return dup_hal_getDupParam(pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0, pstParamInfo);
}

/**
 * @function    dup_task_vpDupCreate
 * @brief       创建用于做图像处理的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32Chn 相同实例名字时对应的通道号，不同实例名通道号可以重复
 * @param[out]  pDupHandle 新创建的dup对应的handle

 * @return
 */
INT32 dup_task_vpDupCreate(INST_CFG_S *stInstCfg, UINT32 u32Chn, DupHandle *pDupHandle)
{
    CHAR szInstName[NAME_LEN];
    HAL_VPSS_GRP_PRM stHalVpssGrpPrm;
    /* UINT32 u32NodeNum; */
    CAPB_VPROC *capb_vproc = capb_get_vproc();
    INST_CFG_S *pstVpDupInstCfg = NULL;

    if (!stInstCfg || !pDupHandle)
    {
        DUP_LOGE("input param is null. \n");
        return SAL_FAIL;
    }
     memset(&stHalVpssGrpPrm, 0, sizeof(HAL_VPSS_GRP_PRM));

    stHalVpssGrpPrm.uiGrpWidth = capb_vproc->width;
    stHalVpssGrpPrm.uiGrpHeight = capb_vproc->height;
    stHalVpssGrpPrm.uiChnDepth = capb_vproc->vpssGrp.uiDepth;
	stHalVpssGrpPrm.u32BlkCnt = capb_vproc->vpssGrp.uiBlkCnt;
    stHalVpssGrpPrm.enVpssDevType = capb_vproc->vpssGrp.enVpssDevType;
    stHalVpssGrpPrm.enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;  /* fixme: vproc group默认输入输出nv21 */
    stHalVpssGrpPrm.enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;

    pstVpDupInstCfg = stInstCfg;
    snprintf(szInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pstVpDupInstCfg->szInstPreName, u32Chn);
    *pDupHandle = dup_create(szInstName, pstVpDupInstCfg->stNodeCfg, pstVpDupInstCfg->u32NodeNum, &stHalVpssGrpPrm);

    return SAL_SOK;
}

/**
 * @function    dup_task_PpmvpDupCreate
 * @brief       创建人包用于做2560x1440图像处理的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32Chn 相同实例名字时对应的通道号，不同实例名通道号可以重复
 * @param[out]  pDupHandle 新创建的dup对应的handle

 * @return
 */
INT32 dup_task_PpmvpDupCreate(INST_CFG_S *stInstCfg, UINT32 u32Chn, DupHandle *pDupHandle)
{
    CHAR szInstName[NAME_LEN];
    HAL_VPSS_GRP_PRM stHalVpssGrpPrm = {0};
    CAPB_VPROC *capb_vproc = capb_get_vproc();
    INST_CFG_S *pstVpDupInstCfg = NULL;

    if (!stInstCfg || !pDupHandle)
    {
        DUP_LOGE("input param is null. \n");
        return SAL_FAIL;
    }

    /* 人包关联需要使用4个vss通道用于裁剪和缩放 */
    stHalVpssGrpPrm.uiGrpWidth = 2560;
    stHalVpssGrpPrm.uiGrpHeight = 1440;
    stHalVpssGrpPrm.uiChnDepth = capb_vproc->vpssGrp.uiDepth;
    stHalVpssGrpPrm.u32BlkCnt = capb_vproc->vpssGrp.uiBlkCnt * 4;
    stHalVpssGrpPrm.enVpssDevType = capb_vproc->vpssGrp.enVpssDevType;
    stHalVpssGrpPrm.uiPhyChnNum = 4;  
    stHalVpssGrpPrm.uiChnEnable[0] = 1;
    stHalVpssGrpPrm.uiChnEnable[1] = 1;
    stHalVpssGrpPrm.uiChnEnable[2] = 1;
    stHalVpssGrpPrm.uiChnEnable[3] = 1;
    stHalVpssGrpPrm.enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    stHalVpssGrpPrm.enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;

#if 0
    stHalVpssGrpPrm.uiExtChnNum = 4;   /* 人包关联需要使用4个拓展通道用于裁剪和缩放 */
    stHalVpssGrpPrm.uiChnEnable[0] = 1;
    stHalVpssGrpPrm.uiChnEnable[1] = 1;
    stHalVpssGrpPrm.uiChnEnable[2] = 1;
    stHalVpssGrpPrm.uiChnEnable[3] = 1;
#endif

    pstVpDupInstCfg = stInstCfg;
    snprintf(szInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pstVpDupInstCfg->szInstPreName, u32Chn);
    *pDupHandle = dup_create(szInstName, pstVpDupInstCfg->stNodeCfg, pstVpDupInstCfg->u32NodeNum, &stHalVpssGrpPrm);

    return SAL_SOK;
}

/**
 * @function    dup_task_vdecDupDestroy
 * @brief       销毁dup
 * @param[in]   frontDupHandle dup创建时的前级handle
 *              frontDupHandle dup创建时的后级handle
 *              u32VdecChn     解码通道号
 * @param[out]  
 * @return
 */
INT32 dup_task_vdecDupDestroy( DupHandle frontDupHandle, DupHandle rearDupHandle, UINT32 u32VdecChn)
{
    CHAR szFrontInstName[NAME_LEN] = {0};
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    void *pHandle = NULL;
    DUP_ChanHandle *dupOutChnHandle = NULL;
    INST_CFG_S *pstVdecDupInstCfg = NULL;
    DUP_BIND_PRM stDupBindPrm;
    UINT32 uiModChn = 0;
    INT32 status;
 
    if (frontDupHandle != rearDupHandle)
    {
        pstVdecDupInstCfg = &stVdecFrontDupInstCfg;
        /*(NAME_LEN-2-2) 总的长度减chn最大需要的字节2，以及“_”，字符串结束符*/
        snprintf(szFrontInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pstVdecDupInstCfg->szInstPreName, u32VdecChn);
        pHandle = link_drvGetHandleFromNode(link_drvGetInst(szFrontInstName), "out_dupRear");        
        dupOutChnHandle = (DUP_ChanHandle *)pHandle;

        uiModChn = sys_task_getModChn(rearDupHandle);
        if (SAL_SOK != dup_getDupObj(uiModChn, &pstDupObjPrm))
        {
            DUP_LOGE("dup_getDupObj err !!!\n");
            return SAL_FAIL;
        }
        stDupBindPrm.mod = SYSTEM_MOD_ID_DUP;
        stDupBindPrm.modChn = pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn; /* 底层设备号 */
        stDupBindPrm.chn = 0; /* 底层通道 */

        if (NULL != dupOutChnHandle->dupOps.OpDupBindBlit)
        {
            status = dupOutChnHandle->dupOps.OpDupBindBlit((Ptr)dupOutChnHandle, (Ptr) &stDupBindPrm, SAL_FALSE);
            if (SAL_isFail(status))
            {
                DUP_LOGE("Bind Hal Chn err !!!\n");
                return SAL_FAIL;
            }
        }
        else
        {
            DUP_LOGE("empty input !!!\n");
            return SAL_FAIL;
        }
        
        dup_destroy(frontDupHandle);
    }
        
    dup_destroy(rearDupHandle);

    return SAL_SOK;
}

/**
 * @function    dup_task_vdecDupCreate
 * @brief       创建用于跟解码器绑定使用的dup
 * @param[in]   stInstCfg 绑定时需要用到的实例配置信息，在link_task.c里配置
                u32VdecChn 解码通道
 * @param[out]  pFrontDupHandle 新创建的前级dup对应的handle
                pRearDupHandle  新创建的后级dup对应的handle
 * @return
 */
INT32 dup_task_vdecDupCreate(INST_CFG_S *stInstCfg, UINT32 u32VdecChn, DupHandle *pFrontDupHandle, DupHandle *pRearDupHandle, VDEC_PRM *pstVdecPrm)
{
    //CHAR szInstName[NAME_LEN];
    CHAR szFrontInstName[NAME_LEN] = {0};
    CHAR szRearInstName[NAME_LEN] = {0};
    HAL_VPSS_GRP_PRM stHalVpssGrpPrm = {0};
    /* UINT32 u32NodeNum; */
    UINT32 u32BlkCnt;
    void *pHandle = NULL;
    DUP_ChanHandle *dupOutChnHandle = NULL;
    INST_CFG_S *pstVdecDupInstCfg = NULL;
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    DUP_BIND_PRM stDupBindPrm;
    UINT32 uiModChn = 0;
    INT32 status;
    
    if (!stInstCfg || !pFrontDupHandle || !pRearDupHandle)
    {
        DUP_LOGE("input param is null. \n");
        return SAL_FAIL;
    }
    CAPB_VDEC_DUP *capb_dup = capb_get_vdecDup();

    memset(&stHalVpssGrpPrm, 0, sizeof(HAL_VPSS_GRP_PRM));
    /* 后续分辨率信息需要解码模块传入，这样避免分配过大的内存导致浪费 */
    stHalVpssGrpPrm.uiGrpWidth = pstVdecPrm->decWidth;
    stHalVpssGrpPrm.uiGrpHeight = pstVdecPrm->decHeight;
	

    /* 1.创建后级vpss */
    stHalVpssGrpPrm.uiChnDepth = capb_dup->rearVpssGrp.uiDepth;
    stHalVpssGrpPrm.enVpssDevType = capb_dup->rearVpssGrp.enVpssDevType;
    stHalVpssGrpPrm.enInputSalPixelFmt = capb_dup->enInputSalPixelFmt;
    stHalVpssGrpPrm.enOutputSalPixelFmt = capb_dup->enOutputSalPixelFmt;
    
    pstVdecDupInstCfg = stInstCfg;

    /*(NAME_LEN-2-2) 总的长度减chn最大需要的字节2，以及“_”，字符串结束符*/
    snprintf(szRearInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pstVdecDupInstCfg->szInstPreName, u32VdecChn);
    u32BlkCnt = (0 == capb_dup->rearVpssGrp.uiBlkCnt) ? (pstVdecDupInstCfg->u32NodeNum - 1)* 3 : capb_dup->rearVpssGrp.uiBlkCnt; /* 每个通道分配3个vb block*/
    stHalVpssGrpPrm.u32BlkCnt = u32BlkCnt;
    *pRearDupHandle = dup_create(szRearInstName, pstVdecDupInstCfg->stNodeCfg, pstVdecDupInstCfg->u32NodeNum, &stHalVpssGrpPrm);

    /* 有些平台比如sd3403 vpss同一个时刻只有一个chan能做放大处理，所以需要有一个前级vpss用来做放大*/
    if (capb_dup->u32HasFrontDupScale)
    {
        /* 2.创建前级vpss */
        stHalVpssGrpPrm.uiChnDepth = capb_dup->frontVpssGrp.uiDepth;
        stHalVpssGrpPrm.enInputSalPixelFmt = capb_dup->enInputSalPixelFmt;
        stHalVpssGrpPrm.enOutputSalPixelFmt = capb_dup->enOutputSalPixelFmt;
    
        pstVdecDupInstCfg = &stVdecFrontDupInstCfg;
        /*(NAME_LEN-2-2) 总的长度减chn最大需要的字节2，以及“_”，字符串结束符*/
        snprintf(szFrontInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pstVdecDupInstCfg->szInstPreName, u32VdecChn);
        u32BlkCnt = (0 == capb_dup->frontVpssGrp.uiBlkCnt) ? (pstVdecDupInstCfg->u32NodeNum - 1)* 3 : capb_dup->frontVpssGrp.uiBlkCnt; /* 每个通道分配3个vb block*/
        stHalVpssGrpPrm.u32BlkCnt = u32BlkCnt;
        *pFrontDupHandle = dup_create(szFrontInstName, pstVdecDupInstCfg->stNodeCfg, pstVdecDupInstCfg->u32NodeNum, &stHalVpssGrpPrm);        

        pHandle = link_drvGetHandleFromNode(link_drvGetInst(szFrontInstName), "out_dupRear");        
        dupOutChnHandle = (DUP_ChanHandle *)pHandle;

        uiModChn = sys_task_getModChn(*pRearDupHandle);
        if (SAL_SOK != dup_getDupObj(uiModChn, &pstDupObjPrm))
        {
            DUP_LOGE("dup_getDupObj err !!!\n");
            return SAL_FAIL;
        }
        stDupBindPrm.mod = SYSTEM_MOD_ID_DUP;
        stDupBindPrm.modChn = pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn; /* 底层设备号 */
        stDupBindPrm.chn = 0; /* 底层通道 */

        /* dup通道与vpss通道绑定 */
        if (NULL != dupOutChnHandle->dupOps.OpDupBindBlit)
        {
            status = dupOutChnHandle->dupOps.OpDupBindBlit((Ptr)dupOutChnHandle, (Ptr) &stDupBindPrm, SAL_TRUE);
            if (SAL_isFail(status))
            {
                DUP_LOGE("Bind Hal Chn err !!!\n");
                return SAL_FAIL;
            }
        }
        else
        {
            DUP_LOGE("empty input !!!\n");
            return SAL_FAIL;
        }

    }
    else
    {
        /* 对于HI3559A，每个vpss chan都可以做放大，不需要前级dup做放大 */
        *pFrontDupHandle = *pRearDupHandle;
    }

    return SAL_SOK;
}

/**
 * @function    dup_task_multDupCreate
 * @brief       创建安检数据进来时需要用到的两个dup，在分析仪里两个dup进行绑定；安检机里两个dup不需要绑定，往各自的dup送数据即可
 * @param[in]   pCreate创建dup时需要用到的参数
                u32Chn 第几路安检数据
 * @param[out]  pDupHandle1 创建的第一个dup handle
                pDupHandle2 创建的第二个dup handle
 * @return
 */
INT32 dup_task_multDupCreate(DUP_MOD_CREATE_PRM *pCreate, UINT32 u32Chn, DupHandle *pDupHandle1, DupHandle *pDupHandle2)
{
    DUP_OBJ_PRM *pstDupObjPrm = NULL;
    CAPT_ChanHandle *pstCaptChanHandle = NULL;
    CHAR szFrontInstName[NAME_LEN] = {0};
    CHAR szRearInstName[NAME_LEN] = {0};
    UINT32 viPipe = 0;
    UINT32 uiModChn = 0;
    UINT32 uiModId = 0;

    INT32 status;
    UINT32 u32NodeNum;
    CAPB_DUP *capb_dup = capb_get_dup();
    CAPB_PRODUCT *capb_product = capb_get_product();
    /* SYSTEM_FRAME_INFO *pstSystemFrameInfo = NULL; */
    HAL_VPSS_GRP_PRM stHalVpssGrpPrm;
    DUP_ChanHandle *dupOutChnHandle = NULL;
    DUP_BIND_PRM stDupBindPrm;

    void *pHandle = NULL;
    INT32 s32Ret = SAL_FAIL;

    INST_CFG_S *pFrontDupInstCfg = NULL;
    INST_CFG_S *pRearDupInstCfg = NULL;

    link_task_getDupInstCfg(&pFrontDupInstCfg, &pRearDupInstCfg);
    if(pFrontDupInstCfg == NULL || pRearDupInstCfg== NULL)
    {
        DUP_LOGE("chan %u, dup_create failed\n", u32Chn);
        return SAL_FAIL;
    }

    memset(&stHalVpssGrpPrm, 0, sizeof(stHalVpssGrpPrm));

    /* 1.创建前级vpss */
    stHalVpssGrpPrm.uiGrpWidth = capb_dup->width;
    stHalVpssGrpPrm.uiGrpHeight = capb_dup->height;
    stHalVpssGrpPrm.uiChnDepth = capb_dup->frontVpssGrp.uiDepth;
    stHalVpssGrpPrm.u32BlkCnt = capb_dup->frontVpssGrp.uiBlkCnt;
    stHalVpssGrpPrm.enVpssDevType = capb_dup->frontVpssGrp.enVpssDevType;
    stHalVpssGrpPrm.enInputSalPixelFmt = capb_dup->enInputSalPixelFmt;
    stHalVpssGrpPrm.enOutputSalPixelFmt = capb_dup->enOutputSalPixelFmt;

    snprintf(szFrontInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pFrontDupInstCfg->szInstPreName, u32Chn);
    u32NodeNum = pFrontDupInstCfg->u32NodeNum;
    *pDupHandle1 = dup_create(szFrontInstName, pFrontDupInstCfg->stNodeCfg, u32NodeNum, &stHalVpssGrpPrm);
    if ((DupHandle)SAL_FAIL == *pDupHandle1)
    {
        DUP_LOGE("chan %u, dup_create %s failed\n", u32Chn, szFrontInstName);
        return SAL_FAIL;
    }
    else
    {
        DUP_LOGI("chan %u, dup_create %s successfully, DupHandle: 0x%x\n", u32Chn, szFrontInstName, *pDupHandle1);
    }

    /* 2.创建后级vpss */
    stHalVpssGrpPrm.uiChnDepth = capb_dup->rearVpssGrp.uiDepth;
    stHalVpssGrpPrm.u32BlkCnt = capb_dup->rearVpssGrp.uiBlkCnt;
    stHalVpssGrpPrm.enVpssDevType = capb_dup->rearVpssGrp.enVpssDevType;
    snprintf(szRearInstName, NAME_LEN, "%.*s_%u",   (NAME_LEN-2-2),  pRearDupInstCfg->szInstPreName, u32Chn);
    u32NodeNum = pRearDupInstCfg->u32NodeNum;
    *pDupHandle2 = dup_create(szRearInstName, pRearDupInstCfg->stNodeCfg, u32NodeNum, &stHalVpssGrpPrm);
    if ((DupHandle)SAL_FAIL == *pDupHandle2)
    {
        DUP_LOGE("chan %u, dup_create %s failed\n", u32Chn, szRearInstName);
        return SAL_FAIL;
    }
    else
    {
        DUP_LOGI("chan %u, dup_create %s successfully, DupHandle: 0x%x\n", u32Chn, szRearInstName, *pDupHandle2);
    }

    /* 3.分析仪需要前后级DUP进行绑定，安检机不需要 */
    if (capb_dup->u32FrontBindRear)
    {
        pHandle = link_drvGetHandleFromNode(link_drvGetInst(szFrontInstName), "out_dupRear");        
        dupOutChnHandle = (DUP_ChanHandle *)pHandle;

        uiModChn = sys_task_getModChn(*pDupHandle2);
        s32Ret = dup_getDupObj(uiModChn, &pstDupObjPrm);
        stDupBindPrm.mod = SYSTEM_MOD_ID_DUP;
        stDupBindPrm.modChn = pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn; /* 底层设备号 */
        stDupBindPrm.chn = 0; /* 底层通道 */

        if (NULL != dupOutChnHandle->dupOps.OpDupBindBlit)
        {
            status = dupOutChnHandle->dupOps.OpDupBindBlit((Ptr)dupOutChnHandle, (Ptr) & stDupBindPrm, SAL_TRUE);
            if (SAL_isFail(status))
            {
                DUP_LOGE("Bind Hal Chn err !!!\n");
                return SAL_FAIL;
            }

            status = dupOutChnHandle->dupOps.OpDupStartBlit((Ptr)dupOutChnHandle);
            if (SAL_isFail(status))
            {
                DUP_LOGE("Dup Start Chn err !!!\n");
                return SAL_FAIL;
            }
        }
        else
        {
            DUP_LOGE("empty input !!!\n");
            return SAL_FAIL;
        }
    }

    /* 4.分析仪需要把VI绑定到DUP，安检机不需要 */
    if (VIDEO_INPUT_OUTSIDE == capb_product->enInputType)
    {
        pstCaptChanHandle = (CAPT_ChanHandle *)pCreate->captHandle;
        if (NULL == pstCaptChanHandle)
        {
            DUP_LOGE("null handle\n");
            return SAL_FAIL;
        }

        DUP_LOGI("pstCaptChanHandle->module:%u\n",pstCaptChanHandle->module);

        uiModChn = sys_task_getModChn(pstCaptChanHandle->module);
        /*uiModId  = System_commonGetModID(pstCaptChanHandle->module); */
        capt_hal_getCaptPipeId(uiModChn,(INT32 *)&viPipe);
        
        uiModChn = sys_task_getModChn(*pDupHandle1);
        s32Ret = dup_getDupObj(uiModChn, &pstDupObjPrm);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;;
        }

        if (SAL_FAIL == sys_hal_SDKbind(SYSTEM_MOD_ID_CAPT, viPipe, 0, SYSTEM_MOD_ID_DUP, pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn, 0))
        {
            DUP_LOGE("Dup Capt Bind to Vpss Failed uiModId %d!!!\n", uiModId);
            return SAL_FAIL;
        }
    }
    return SAL_SOK;
}

