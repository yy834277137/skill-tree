/**
 * @file   venc_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  venc 模块 fun 层
 * @author liuyun10
 * @date   2018年12月16日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月16日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include <platform_hal.h>
#include "venc_drv.h"
#include "venc_drv_api.h"

/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                            结构定义
*****************************************************************************/

/*****************************************************************************
                            全局结构体
*****************************************************************************/

static VENC_DRV_CTRL_S gstVencDrvCtrl;

/* ========================================================================== */
/*                          函数声明                                        */
/* ========================================================================== */


/*****************************************************************************
                            函数
*****************************************************************************/

/**
 * @function   venc_drv_getfps
 * @brief  获取原编码帧率
 * @param[in] pstChnInfo
 * @param[out] None
 * @return      int  帧率vencfps
 */
INT32 venc_drv_getfps(void *pstChnInfo)
{
    INT32 vencfps=0;
    VENC_DRV_CHN_INFO_S *pChnHandle;
    if (NULL == pstChnInfo)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }
    pChnHandle = (VENC_DRV_CHN_INFO_S *)pstChnInfo;
    vencfps = pChnHandle->stVencChnPrm.fps;
    return vencfps;
}

/**
 * @function   venc_getFreeChn
 * @brief      获取空闲的drv层通道
 * @param[in]  None
 * @param[out] UINT32 *pCh 通道指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_getFreeChn(UINT32 *pChn)
{
    INT32 i = 0;

    if (NULL == pChn)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    SAL_mutexLock(gstVencDrvCtrl.pMutexHandle);
    for (i = 0; i < VENC_MAX_CHAN_NUM; i++)
    {
        /* 标记为1了，表示已被使用 */
        if (gstVencDrvCtrl.u32VencChnUsedFlg & (1 << i))
        {
            continue;
        }
        else
        {
            /* 发现未使用的，返回通道号，并标记为已用 */
            *pChn = i;
            gstVencDrvCtrl.u32VencChnUsedFlg = gstVencDrvCtrl.u32VencChnUsedFlg | (1 << i);
            break;
        }
    }

    /* MAX_ENC_PACK_CHN 个都被使用了，表示无可用的，返回错误，此情况就该扩大 MAX_ENC_PACK_CHN 的值 */
    if (i == VENC_MAX_CHAN_NUM)
    {
        *pChn = 0xff;
        SAL_mutexUnlock(gstVencDrvCtrl.pMutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(gstVencDrvCtrl.pMutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_drv_getHalChan
 * @brief      获取hal通道ID
 * @param[in]  void *pChnHandle通道句柄
 * @param[out] UINT32 *pChn 通道指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_getHalChan(void *pChnHandle, UINT32 *pChn)
{
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;
    UINT32 u32Chn = 0;
    INT32 s32Ret = SAL_SOK;

    if ((NULL == pChnHandle) || (NULL == pChn))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    s32Ret = venc_hal_getChn(pstVencChnInfo->pVencHandle, &u32Chn);
    if (SAL_SOK == s32Ret)
    {
        *pChn = u32Chn;
    }
    else
    {
        *pChn = 0xff;
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_addChan
 * @brief      新增一个编码通道
 * @param[in]  VENC_DRV_INIT_PRM_S *pstInPrm 输入参数
 * @param[out] void **pphandle 通道句柄指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_addChan(void **pphandle, VENC_DRV_INIT_PRM_S *pstInPrm)
{
    UINT32 u32Chn = 0;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((NULL == pstInPrm) || (NULL == pphandle))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    /* 获取空闲通道 */
    if (SAL_SOK != venc_getFreeChn(&u32Chn))
    {
        VENC_LOGE("not enough chn\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = &gstVencDrvCtrl.stVencChnInfo[u32Chn];

    /* 传递参数 */
    memcpy(&pstVencChnInfo->stVencChnPrm, &pstInPrm->stVencPrm, sizeof(SAL_VideoFrameParam));
    pstVencChnInfo->u32Chn = u32Chn;
    pstVencChnInfo->u32Dev = pstInPrm->u32Dev;
    pstVencChnInfo->u32StreamId = pstInPrm->u32StreamId;

    *pphandle = (void *)pstVencChnInfo;
    return SAL_SOK;
}

/**
 * @function   venc_drv_staticParmCheck
 * @brief      编码器静态参数设置检查
 * @param[in]  void *pChnHandle 通道句柄
 * @param[in]  SAL_VideoFrameParam *pstVencChnParm 编码参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
UINT32 venc_drv_staticParmCheck(void *pChnHandle, SAL_VideoFrameParam *pstVencChnParm)
{
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pstVencChnParm))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_TRUE;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    /*hi3559A  编码器编码类型是静态参数，如果变化需要销毁重建编码器
     * 所有的编码器静态参数都要重新创建编码器
     * 编码参数变化是否需要重建编码器应该根据plat层来
     */
    if (pstVencChnInfo->stVencChnPrm.encodeType != pstVencChnParm->encodeType) /*res change?*/
    {
        VENC_LOGI("venc static parma is change");
        return SAL_TRUE;
    }

    return SAL_FALSE;
}

/**
 * @function   venc_drv_setPrm
 * @brief   配置编码参数: 切换分辨率、码率、I帧间隔、编码质量、帧率等
            编码器如果创建好立即生效，没有创建则下次开启编码器时生效
 * @param[in]  void *pChnHandle 通道句柄
 * @param[in]  SAL_VideoFrameParam *pstVencChnAttr 编码参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_setPrm(void *pChnHandle, SAL_VideoFrameParam *pstVencPrm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pstVencPrm))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    /* 记录用户配置的最新分辨率，下次开启编码时使用该分辨率 */
    pstVencChnInfo->stVencChnPrm.width = pstVencPrm->width;
    pstVencChnInfo->stVencChnPrm.height = pstVencPrm->height;
    pstVencChnInfo->stVencChnPrm.fps = pstVencPrm->fps;
    pstVencChnInfo->stVencChnPrm.standard = pstVencPrm->standard;
    pstVencChnInfo->stVencChnPrm.bpsType = pstVencPrm->bpsType;
    pstVencChnInfo->stVencChnPrm.bps = pstVencPrm->bps;
    pstVencChnInfo->stVencChnPrm.encodeType = pstVencPrm->encodeType;
    VENC_LOGW("Dev %d Chn %d set w %d h %d standard %d encodeType %d\n",
              pstVencChnInfo->u32Dev, pstVencChnInfo->u32StreamId,
              pstVencPrm->width, pstVencPrm->height, pstVencPrm->standard, pstVencPrm->encodeType);

    if (SAL_FALSE == pstVencChnInfo->isCreate)
    {
        VENC_LOGW("Chn is not used, save prm only \n");
        return SAL_SOK;
    }

    /* 设置hal层属性 */
    s32Ret = venc_hal_setEncPrm(pstVencChnInfo->pVencHandle, pstVencPrm);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("SetParam err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_requestIDR
 * @brief      编码模块强制I帧
 * @param[in]  void *pChnHandle 通道句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_requestIDR(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if (NULL == pChnHandle)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    s32Ret = venc_hal_requestIDR(pstVencChnInfo->pVencHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("ForceIFrame err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_getBuff
 * @brief      获取一帧码流
 * @param[in]  void *pChnHandle 通道句柄
 * @param[out] void *pBuffer 含有码流数据地址的结构体指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_getBuff(void *pChnHandle, void *pBuffer)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;
    SYSTEM_BITS_DATA_INFO_ST *pstDataInfo = NULL;
    STREAM_FRAME_INFO_ST *pstStreamFrameInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pBuffer))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;
    pstDataInfo = (SYSTEM_BITS_DATA_INFO_ST *)pBuffer;
    pstStreamFrameInfo = &pstDataInfo->stStreamFrameInfo[0];

    s32Ret = venc_hal_getFrame(pstVencChnInfo->pVencHandle, (void *)pstStreamFrameInfo);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("GetFrame err ret %d !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_putBuff
 * @brief      释放一帧数据
 * @param[in]  void *pChnHandle 通道句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_putBuff(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if (NULL == pChnHandle)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    s32Ret = venc_hal_putFrame(pstVencChnInfo->pVencHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_hisi_putFrame Failed !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_sendVideoFrm
 * @brief      向编码器发送一帧视频数据
 * @param[in]  void *pChnHandle 编码句柄
 * @param[in]  *pstInPrm 视频帧
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_sendVideoFrm(void *pChnHandle, void *pstInFrm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pstInFrm))
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    if (SAL_TRUE != pstVencChnInfo->isCreate)
    {
        SAL_ERROR("Chn %d has not been created!!!\n", pstVencChnInfo->u32Chn);
        return SAL_FAIL;
    }

    s32Ret = venc_hal_sendVideoFrm(pstVencChnInfo->pVencHandle, pstInFrm);
    if (SAL_isFail(s32Ret))
    {
        SAL_ERROR("enc err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_stop
 * @brief      停止编码器
 * @param[in]  void *pChnHandle 通道句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_stop(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32VencStatus = 0;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if (NULL == pChnHandle)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;
    VENC_LOGI("vichn %d,chn %d\n", pstVencChnInfo->u32Dev, pstVencChnInfo->u32Chn);

    u32VencStatus = 1;
    s32Ret = venc_hal_setStatus(pstVencChnInfo->pVencHandle, &u32VencStatus);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_hal_setStatus err");
    }

    /* 线程进入等待不取流后再关闭底层编码，防止线程取不到流额外等待200ms */
    s32Ret = venc_hal_stop(pstVencChnInfo->pVencHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_hisi_stop err");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_start
 * @brief      开启编码器
 * @param[in]  void *pChnHandle 通道句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_start(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32VencStatus = 0;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if (NULL == pChnHandle)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    u32VencStatus = 0;
    venc_hal_setStatus(pstVencChnInfo->pVencHandle, &u32VencStatus);

    s32Ret = venc_hal_start(pstVencChnInfo->pVencHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_hal_start!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_drv_create
 * @brief      创建编码通道，包括hal层的创建和线程的发布
 * @param[in]  void *pChnHandle 通道句柄
 * @param[in]  UINT32 isEnableDraw 使能画线标记
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_create(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;
    SAL_VideoFrameParam *pstVencCreatePrm = NULL;

    if (NULL == pChnHandle)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    if (SAL_TRUE == pstVencChnInfo->isCreate)
    {
        VENC_LOGW("Tsk is Running !!!\n");
        return SAL_SOK;
    }

    /* 使用 venc_drv_addChan 设置的默认参数配置编码通道 */
    if (NULL == pstVencChnInfo->pVencHandle)
    {
        pstVencCreatePrm = &pstVencChnInfo->stVencChnPrm;
        s32Ret = venc_hal_create(&pstVencChnInfo->pVencHandle, pstVencCreatePrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_hal_create fail!!!\n");
            return SAL_FAIL;
        }
    }

    pstVencChnInfo->isCreate = SAL_TRUE;

    VENC_LOGW("Venc Dev %d Chn %d Create\n", pstVencChnInfo->u32Dev, pstVencChnInfo->u32StreamId);
    return SAL_SOK;
}

/**
 * @function   venc_drv_delete
 * @brief      编码通道的销毁，停止线程，销毁hal层编码器
 * @param[in]  void *pChnHandle 通道句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_delete(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DRV_CHN_INFO_S *pstVencChnInfo = NULL;

    if (NULL == pChnHandle)
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = (VENC_DRV_CHN_INFO_S *)pChnHandle;

    /* 销毁hal层通道资源 */
    s32Ret = venc_hal_delete(pstVencChnInfo->pVencHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("VENC Delete fail!\n");
        return SAL_FAIL;
    }

    pstVencChnInfo->pVencHandle = NULL;
    pstVencChnInfo->isCreate = SAL_FALSE;

    VENC_LOGW("Venc Dev %d Chn %d delete\n", pstVencChnInfo->u32Dev, pstVencChnInfo->u32StreamId);
    return SAL_SOK;
}

/**
 * @function   venc_drv_init
 * @brief      drv层编码初始化
 * @param[in]  None
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_init(void)
{
    memset(&gstVencDrvCtrl, 0, sizeof(VENC_DRV_CTRL_S));

    /* 初始化通道管理锁 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &gstVencDrvCtrl.pMutexHandle);

    /* hal层初始化 */
    if (SAL_SOK != venc_hal_init())
    {
        VENC_LOGE("vencHal_drv_init err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

