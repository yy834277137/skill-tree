/**
 * @file   jpeg_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  jpeg 模块 drv 层
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
#include "jpeg_drv.h"
#include "jpeg_drv_api.h"
#include "plat_com_inter_hal.h"

/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                            结构定义
*****************************************************************************/

/*****************************************************************************
                            全局结构体
*****************************************************************************/
static JPEG_DRV_CTRL_S gstJpegDrvCtrl = {0};
JPEG_ADDR gstJpegAddr;
/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   jpeg_getFreeChn
 * @brief      获取一个空闲的抓图通道
 * @param[in]  None
 * @param[out] UINT32 *pChn 抓图通道指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_getFreeChn(UINT32 *pChn)
{
    INT32 i = 0;
    UINT32 u32MaxChanNum = JPEG_MAX_CHN_NUM;

    SAL_mutexLock(gstJpegDrvCtrl.pMutexHandle);
    for (i = 0; i < u32MaxChanNum; i++)
    {
        /* 标记为1了，表示已被使用 */
        if (gstJpegDrvCtrl.isJpegChnUsed & (1 << i))
        {
            continue;
        }
        else
        {
            /* 发现未使用的，返回通道号，并标记为已用 */
            *pChn = i;
            gstJpegDrvCtrl.isJpegChnUsed = gstJpegDrvCtrl.isJpegChnUsed | (1 << i);
            break;
        }
    }

    /* MAX_ENC_PACK_CHN 个都被使用了，表示无可用的，返回错误，此情况就该扩大 MAX_ENC_PACK_CHN 的值 */
    if (i == u32MaxChanNum)
    {
        *pChn = 0xff;
        SAL_mutexUnlock(gstJpegDrvCtrl.pMutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(gstJpegDrvCtrl.pMutexHandle);
    return SAL_SOK;
}

/**
 * @function   jpeg_putFreeChn
 * @brief      返还一个不用的抓图通道
 * @param[in]  UINT32 u32Chan 抓图通道号
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static void jpeg_putFreeChn(UINT32 u32Chan)
{
    SAL_mutexLock(gstJpegDrvCtrl.pMutexHandle);
    /* 编码器消耗后回收通道 */
    if (gstJpegDrvCtrl.isJpegChnUsed & (1 << u32Chan))
    {
        gstJpegDrvCtrl.isJpegChnUsed = gstJpegDrvCtrl.isJpegChnUsed & (~(1 << u32Chan));
    }

    SAL_mutexUnlock(gstJpegDrvCtrl.pMutexHandle);
}

/**
 * @function   jpeg_drv_createChn
 * @brief      创建一个抓图通道
 * @param[in]  SAL_VideoFrameParam *pstCreatePrm 抓图参数
 * @param[out] void **ppJpegHandle 抓图通道句柄指针
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_createChn(void **ppJpegHandle, SAL_VideoFrameParam *pstCreatePrm)
{
    UINT32 u32Chan = 0;
    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;

    if ((NULL == ppJpegHandle) || (NULL == pstCreatePrm))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    /*DUP 有效性判断*/

    s32Ret = jpeg_getFreeChn(&u32Chan);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("jpeg_getFreeChn err !!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = &gstJpegDrvCtrl.stJpegChnInfo[u32Chan];
    if (SAL_TRUE == pstJpegChnInfo->isCreate)
    {
        JPEG_LOGE("chan  isCreate %d, flg err!!!\n", pstJpegChnInfo->isCreate);
    }

    if (NULL != pstJpegChnInfo->pVencHandle)
    {
        JPEG_LOGE("enc handle  exist,  err!!!\n");
    }

    /* 创建硬件通道  */
    s32Ret = venc_hal_create(&pstJpegChnInfo->pVencHandle, pstCreatePrm);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("venc_hal_create err !!!\n");
        jpeg_putFreeChn(u32Chan);
        return SAL_FAIL;
    }

    /* 记录创建属性 */
    pstJpegChnInfo->u32Chan = u32Chan;
    pstJpegChnInfo->u32Width = pstCreatePrm->width;
    pstJpegChnInfo->u32Height = pstCreatePrm->height;
    pstJpegChnInfo->u32Quality = pstCreatePrm->quality;
    pstJpegChnInfo->isCreate = SAL_TRUE;
    *ppJpegHandle = (void *)pstJpegChnInfo;
    JPEG_LOGI("u32Chan %d Jpeg Create Success width:%u height:%u quality:%u\n", u32Chan, pstJpegChnInfo->u32Width, pstJpegChnInfo->u32Height, pstJpegChnInfo->u32Quality);
    return SAL_SOK;
}

/**
 * @function   jpeg_drv_deleteChn
 * @brief      销毁抓图通道
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_deleteChn(void *pChnHandle)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;

    if (NULL == pChnHandle)
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = (JPEG_DRV_CHN_INFO_S *)pChnHandle;

    if (SAL_FALSE == pstJpegChnInfo->isCreate)
    {
        JPEG_LOGW("Chn has been destroyed\n");
        return SAL_SOK;
    }

    /* 销毁通道 */
    SAL_mutexLock(pstJpegChnInfo->pMutexHandle);

    s32Ret = venc_hal_delete(pstJpegChnInfo->pVencHandle);
    if (SAL_SOK != s32Ret)
    {
        JPEG_LOGE("venc_hal_delete err !!!\n");
        SAL_mutexUnlock(pstJpegChnInfo->pMutexHandle);
        return SAL_FAIL;
    }

    pstJpegChnInfo->pVencHandle = NULL;

    jpeg_putFreeChn(pstJpegChnInfo->u32Chan);
    pstJpegChnInfo->isCreate = SAL_FALSE;
    SAL_mutexUnlock(pstJpegChnInfo->pMutexHandle);
    JPEG_LOGW("Jpeg Delete Success\n");
    return SAL_SOK;

}

/**
 * @function   jpeg_setPrm
 * @brief      销毁抓图通道
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_setPrm(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;
    SAL_VideoFrameParam stJpegHalPicInfo = {0};

    if ((NULL == pChnHandle) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = (JPEG_DRV_CHN_INFO_S *)pChnHandle;
    /* 如果参数发生变化，则设置hal层属性 */
    if ((pstJpegChnInfo->u32Width != pstInPrm->OutWidth)
        || (pstJpegChnInfo->u32Height != pstInPrm->OutHeight)
        || (pstJpegChnInfo->u32Quality != pstInPrm->quality))
    {
        stJpegHalPicInfo.width = pstInPrm->OutWidth;
        stJpegHalPicInfo.height = pstInPrm->OutHeight;
        stJpegHalPicInfo.quality = pstInPrm->quality;
        stJpegHalPicInfo.encodeType = MJPEG;

        s32Ret = venc_hal_setEncPrm(pstJpegChnInfo->pVencHandle, &stJpegHalPicInfo);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("VencHal_drvSetEncPrm err !!!\n");
            return SAL_FAIL;
        }

        /* 设置成功则保存当前参数到通道内部 */
        pstJpegChnInfo->u32Width = pstInPrm->OutWidth;
        pstJpegChnInfo->u32Height = pstInPrm->OutHeight;
        pstJpegChnInfo->u32Quality = pstInPrm->quality;

        JPEG_LOGI("set jpeg prm ok! width:%u height:%u quality:%u\n", pstJpegChnInfo->u32Width, pstJpegChnInfo->u32Height, pstJpegChnInfo->u32Quality);
    }

    return SAL_SOK;
}

/**
 * @function   jpeg_resetChn
 * @brief      参数变化时重新创建抓图通道
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_resetChn(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;
    SAL_VideoFrameParam stCreatePrm = {0};

    if ((SAL_isNull(pChnHandle)) || (SAL_isNull(pstInPrm)))
    {
        JPEG_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = (JPEG_DRV_CHN_INFO_S *)pChnHandle;

    if ((pstJpegChnInfo->u32Width == pstInPrm->OutWidth)
        && (pstJpegChnInfo->u32Height == pstInPrm->OutHeight)
        && (pstJpegChnInfo->u32Quality == pstInPrm->quality))

    {
        return SAL_SOK;
    }
    else
    {
        /* 销毁通道 */
        s32Ret = venc_hal_delete(pstJpegChnInfo->pVencHandle);
        if (SAL_SOK != s32Ret)
        {
            JPEG_LOGE("venc_hal_delete err !!!\n");
            return SAL_FAIL;
        }

        pstJpegChnInfo->pVencHandle = NULL;

        /* 创建硬件通道  */
        memset(&stCreatePrm, 0, sizeof(SAL_VideoFrameParam));
        stCreatePrm.width = pstInPrm->OutWidth;
        stCreatePrm.height = pstInPrm->OutHeight;
        stCreatePrm.quality = pstInPrm->quality;
        stCreatePrm.encodeType = MJPEG;

        /*如果没有销毁操作，是否需要判断handle == NULL?*/
        s32Ret = venc_hal_create(&pstJpegChnInfo->pVencHandle, &stCreatePrm);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("JpegHal_drvCreate err !!!\n");
            return SAL_FAIL;
        }

        pstJpegChnInfo->u32Width = pstInPrm->OutWidth;
        pstJpegChnInfo->u32Height = pstInPrm->OutHeight;
        pstJpegChnInfo->u32Quality = pstInPrm->quality;
    }

    return SAL_SOK;
}

/**
 * @function   jpeg_drv_enc
 * @brief   编码一张图片，调用前需要先使用Jpeg_drvCreate创建一个handle
            输出的jpeg内存由使用者分配
            非平台相关输入NV21 的YUV图片地址和宽高
            和平台相关则直接在输入参数内填入相关信息，默认平台相关的输入图像参数是合法的
            如果接口频繁调用，且编码参数会经常变化，推荐创建编码通道时直接创建最大分辨率
            每次编码不要重置编码器
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[in]  UINT32 isReset 是否需要重置编码器
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_enc(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm, UINT32 isReset)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = (JPEG_DRV_CHN_INFO_S *)pChnHandle;

    if (SAL_TRUE != pstJpegChnInfo->isCreate)
    {
        JPEG_LOGE("Chn %d has not been created {%d,%d}!!!\n", pstJpegChnInfo->u32Chan, pstJpegChnInfo->u32Width, pstJpegChnInfo->u32Width);
        return SAL_FAIL;
    }

    SAL_mutexLock(pstJpegChnInfo->pMutexHandle);

    if (SAL_TRUE == isReset)
    {
        s32Ret = jpeg_resetChn(pChnHandle, pstInPrm);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("Jpeg_drvSetFrameInfo err !!!\n");
            SAL_mutexUnlock(pstJpegChnInfo->pMutexHandle);
            return SAL_FAIL;
        }
    }
    else
    {
        s32Ret = jpeg_setPrm(pChnHandle, pstInPrm);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("Jpeg_drvSetFrameInfo err !!!\n");
            SAL_mutexUnlock(pstJpegChnInfo->pMutexHandle);
            return SAL_FAIL;
        }
    }

    s32Ret = venc_hal_encJpeg(pstJpegChnInfo->pVencHandle, &pstInPrm->stSysFrame, pstInPrm->pOutJpeg, &pstInPrm->outSize, NULL, SAL_FALSE);

    SAL_mutexUnlock(pstJpegChnInfo->pMutexHandle);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("JpegHal_drvEncOnePic err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Jpeg_drvVbAlloc
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Jpeg_drvVbAlloc(JPEG_VB_ALLOC_ST *pVbMemPrm, CHAR *szMemName)
{
    INT32 s32Ret = SAL_SOK;
    ALLOC_VB_INFO_S stVbInfo = {0};

    s32Ret = mem_hal_vbAlloc(pVbMemPrm->u32MemSize, "jpeg", szMemName, NULL, SAL_FALSE, &stVbInfo);
    if (s32Ret != SAL_SOK)
    {
        XPACK_LOGE("malloc mem failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    pVbMemPrm->u32PoolId = stVbInfo.u32PoolId;
    pVbMemPrm->phyAddr = stVbInfo.u64PhysAddr;
    pVbMemPrm->virAddr = stVbInfo.pVirAddr;
    pVbMemPrm->u64vbBlk = stVbInfo.u64VbBlk;
    /* XPACK_LOGD("vb %d\n", mem_hal_getVbBlk(pVbMemPrm->virAddr)); */

    return s32Ret;
}

INT32 Jpeg_drvRgbToYuv(SYSTEM_FRAME_INFO *stSysFrame)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_VB_ALLOC_ST *pXpackJpgAddr = NULL;
    SYSTEM_FRAME_INFO *pstYuvFrame = &gstJpegAddr.stYuvFrame;
    pXpackJpgAddr = &gstJpegAddr.stYuvBackAddr;
    SAL_VideoFrameBuf pstVideoFrame ;
    // XSP_LOGE("get in rgb2Yuv\n");
    
    /* 构建缩小到1280x1024的缩放目的帧 */

        if(pXpackJpgAddr->virAddr==NULL)
        {
            XPACK_LOGE("RGB TO YUV ERROR pXpackJpgAddr->virAddr is NULL\n");
            return SAL_FAIL;
        }
        memset(&pstVideoFrame, 0x00, sizeof(SAL_VideoFrameBuf));
        memset(pXpackJpgAddr->virAddr , 0 , pXpackJpgAddr->u32MemSize);
        SAL_halMakeFrame(pXpackJpgAddr->virAddr, pXpackJpgAddr->phyAddr, pXpackJpgAddr->u32PoolId, stSysFrame->uiDataWidth, stSysFrame->uiDataHeight, SAL_align(stSysFrame->uiDataWidth,64), \
                            SAL_VIDEO_DATFMT_YUV420SP_UV, &pstVideoFrame);
        sys_hal_buildVideoFrame(&pstVideoFrame, pstYuvFrame);
        //将RGB格式转换为YUV格式
        s32Ret  = vgs_hal_scaleFrame(pstYuvFrame, stSysFrame, stSysFrame->uiDataWidth, stSysFrame->uiDataHeight);
        if(s32Ret != SAL_SOK)
        {
            XPACK_LOGE("RGB TO YUV ERROR\n");
            return SAL_FAIL;
        }
        sal_memcpy_s(stSysFrame , sizeof(SYSTEM_FRAME_INFO), pstYuvFrame, sizeof(SYSTEM_FRAME_INFO));

    return SAL_SOK;
}

/**
 * @function   jpeg_drv_cropEnc
 * @brief      编码一张图片，支持抠图
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[in]  CROP_S *pstCropInfo 抠图信息，可为NULL
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_cropEnc(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm, CROP_S *pstCropInfo)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = (JPEG_DRV_CHN_INFO_S *)pChnHandle;

    if (SAL_TRUE != pstJpegChnInfo->isCreate)
    {
        JPEG_LOGE("Chn %d has not been created {%d,%d}!!!\n", pstJpegChnInfo->u32Chan, pstJpegChnInfo->u32Width, pstJpegChnInfo->u32Width);
        return SAL_FAIL;
    }

    SAL_mutexLock(pstJpegChnInfo->pMutexHandle);
    Jpeg_drvRgbToYuv(&pstInPrm->stSysFrame);
    s32Ret = venc_hal_encJpeg(pstJpegChnInfo->pVencHandle, &pstInPrm->stSysFrame, pstInPrm->pOutJpeg, &pstInPrm->outSize, pstCropInfo, SAL_TRUE);
    SAL_mutexUnlock(pstJpegChnInfo->pMutexHandle);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("JpegHal_drvEncOnePic err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   jpeg_drv_init
 * @brief      drv层编码器初始化
 * @param[in]  None
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_init(void)
{
    UINT32 i = 0;
    SAL_STATUS sRet = 0;
    SYSTEM_FRAME_INFO *pstYuvFrame = &gstJpegAddr.stYuvFrame;
    JPEG_VB_ALLOC_ST *stYuvBackAddr = &gstJpegAddr.stYuvBackAddr;
    JPEG_DRV_CHN_INFO_S *pstJpegChanInfo = NULL;

    memset(&gstJpegDrvCtrl, 0, sizeof(JPEG_DRV_CTRL_S));

    if (pstYuvFrame->uiAppData == 0x00)
    {
        sRet = sys_hal_allocVideoFrameInfoSt(pstYuvFrame);
        if (SAL_SOK != sRet)
        {
            XPACK_LOGE("sys_hal_allocVideoFrameInfost error!\n");
            return SAL_FAIL;
        }
    }

    SAL_clear(stYuvBackAddr);
    stYuvBackAddr->u32MemSize = 2496 * 1280 * 4;
    sRet = Jpeg_drvVbAlloc(stYuvBackAddr, "jpb-imgBack");
    if (SAL_SOK != sRet)
    {
        XPACK_LOGE("Jpeg_drvVbAlloc failed with 0x%x\n", sRet);
        return SAL_FAIL;
    }

    /* 初始化通道管理lock*/
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &gstJpegDrvCtrl.pMutexHandle);

    /* 初始化设备管理模块lock*/
    for (i = 0; i < JPEG_MAX_CHN_NUM; i++)
    {
        pstJpegChanInfo = &gstJpegDrvCtrl.stJpegChnInfo[i];
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstJpegChanInfo->pMutexHandle);
    }

    /* hal层初始化 */
    if (SAL_SOK != venc_hal_init())
    {
        JPEG_LOGE("vencHal_drv_init err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#if 0

/**
 * @function   Jpeg_drvSetFrameInfo
 * @brief   码抓图的内存帧属性，将非平台相关的图像转换为平台相关
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_drvSetFrameInfo(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm)
{

    INT32 s32Ret = SAL_SOK;
    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo = NULL;

    if ((NULL == pChnHandle) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegChnInfo = (JPEG_DRV_CHN_INFO_S *)pChnHandle;
    if (SAL_FALSE == pInPrm->isPlatForm)
    {
        /* 非平台相关的数据要把yuv数据转换为平台相关的数据 */
        s32Ret = JpegHal_drvSetFrameInfo(pstJpegChnInfo->u32Chan, pstInPrm);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("JpegHal_drvSetFrameInfo err !!!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

#endif


