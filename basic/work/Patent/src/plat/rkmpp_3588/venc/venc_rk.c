/**
 * @file   venc_rk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---plat层接口封装
 * @author wangzhenya5
 * @date   2022/03/29 create
 * @note   rk3588 venc 平台层接口封装
 */
 
#include <sal.h>
#include <dspcommon.h>
#include "venc_rk.h"
#include "rk_comm_venc.h"
#include "sal_mem_new.h"
#include <platform_sdk.h>
#include "fmtConvert_rk.h"

#line __LINE__ "venc_rk.c"


/*****************************************************************************
                            宏定义
*****************************************************************************/
/*
// 以下宏定义未使用,暂时注释, 后续删除
//#define HISI_SDK_STREAM_MAX_CNT (64)
//#define CROP_VPSS_GROUP_WIDTH (1200)
//#define CROP_VPSS_GROUP_HEIGHT    (1920)
//#define MAX_VENC_FPS_P_STANDARD (50)
//#define MAX_VENC_FPS_N_STANDARD (60)

*/

#define VENC_HAL_CHECK_PRM(ptr, value) {if (!ptr) {VENC_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}
#define VENC_RK_JPEG_WIDHT_MIN  68
/*****************************************************************************
                            全局结构体
*****************************************************************************/

/* 编码器通道属性 */
typedef struct tagVencHalChnInfoSt
{
    UINT32 uiChn;                           /* 编码器通道号    */
    UINT32 uibStart;                        /* 是否是start状态 */
    UINT32 uiState;                        /* 编码状态，0 正常状态，1等待状态 */
    UINT32 isCreate;                        /* 编码器被创建好 */
    VENC_CHN_ATTR_S stVencChnAttr;          /* 编码器通道状态属性 */
    VENC_STREAM_S stStream;
    /* 固定申请4K足够的混存空间存放抓图时的 VENC_PACK_S信息 */
    UINT32 packBufLen;
    VENC_PACK_S *pVirtPack;

    UINT32 uiFd;                            /* 通道所对应的fd     */
    UINT32 vencFps;                         /* 编码帧率 */
} VENC_HAL_CHN_INFO_S;

/* 编码器模块编码全局属性与通道管理 */
typedef struct tagVencHalInfoSt
{
    void *MutexHandle;
    /* 编码器通道管理 通道号对应数组下标,支持 VENC_HISI_CHN_NUM 个 */
    VENC_HAL_CHN_INFO_S stVencHalChnPrm[VENC_RK_CHN_NUM];
} VENC_HAL_INFO_S;

 typedef struct tagVencFpsInfoSt
 {
    UINT32 u32Gop;
    UINT32 u32SrcFrameRateNum;
    UINT32 u32SrcFrameRateDen;
    UINT32 fr32DstFrameRateNum;
    UINT32 fr32DstFrameRateDen;
}VENC_FPS_INFO_S;

/*****************************************************************************
                            全局结构体
*****************************************************************************/

static VENC_HAL_INFO_S gstVencRkInfo = {0};

/* 记录 venc group 使用情况 */
static UINT32 isVencGroupUsed = 0;
static UINT64 streamPTS[VENC_RK_CHN_NUM] = {0};

/* #define VENC_HAL_SAVE_BITS_DATA */

#ifdef VENC_HAL_SAVE_BITS_DATA
/* 测试用 */
FILE *file[VENC_RK_CHN_NUM];
#endif


/*****************************************************************************
                            函数定义
*****************************************************************************/
/* jpeg编码调试使用 */
#if 0
static UINT32 dump_jpegnum = 0;
static INT32 dump_jpeg(INT32 chan, UINT32 len,const UINT8 * const addr,UINT32 w,UINT32 h)
{
    FILE   *fp;
    char    sztempTri[64] = {0};
    UINT32  WriteNum;
    
    sprintf(sztempTri, "/mnt/jpeg/dump_chan_%d_w%d_h%d_num%d.bgra", chan,w,h, dump_jpegnum);
    
    if((fp = fopen(sztempTri, "wb+")) == NULL)
    {
        OSD_LOGE("open file Fail,close!\n");
        return 0;
    }
    fseek(fp,0,SEEK_END);
    
    WriteNum = fwrite(addr, 1, len, fp);
    
    OSD_LOGE("write %d \n ", WriteNum);
    
    fflush(fp);

    fclose(fp);
    
    dump_jpegnum ++;

    return 0;
}
#endif

/**
 * @function    venc_getEncType
 * @brief       获取平台层编码类型
 * @param[in]   UINT32 u32InType 上层传入编码类型
 * @param[out]  None
 * @return      rkCODEC_ID_E RK层编码类型
 */
static RK_CODEC_ID_E venc_rk_getEncType(UINT32 u32InType)
{
    switch (u32InType)
    {
        case MJPEG:
            return RK_VIDEO_ID_JPEG;
        case H265:
            return RK_VIDEO_ID_HEVC;
        default:
            return RK_VIDEO_ID_AVC;
    }
}

/**
 * @function    venc_rk_getGroup
 * @brief       获取可用的 Venc 模块
 * @param[in]   None
 * @param[out] UINT32 *chan 通道指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_getGroup(UINT32 *pchn)
{
    int i = 0;

    /* rk最大编码通道最大个数为64 */
    if (VENC_RK_CHN_NUM > 32)
    {
        VENC_LOGE("HAL venc pchn overflow %d\n", VENC_RK_CHN_NUM);
        return SAL_FAIL;
    }

    SAL_mutexLock(gstVencRkInfo.MutexHandle);
    for (i = 0; i < VENC_RK_CHN_NUM; i++)
    {
        /* 标记为1了，表示已被使用 */
        if (isVencGroupUsed & (1 << i))
        {
            continue;
        }
        else
        {
            /* 发现未使用的，返回通道号，并标记为已用 */
            *pchn = i;
            isVencGroupUsed = isVencGroupUsed | (1 << i);
            break;
        }
    }

    /* DUP_CHN_NUM_MAX 个都被使用了，表示无可用的，返回错误，此情况就该扩大 DUP_CHN_NUM_MAX 的值 */
    if (i == VENC_RK_CHN_NUM)
    {
        *pchn = 0xff;
        SAL_mutexUnlock(gstVencRkInfo.MutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(gstVencRkInfo.MutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_rk_putGroup
 * @brief    释放的 Venc 模块
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static void venc_rk_putGroup(UINT32 u32Chan)
{
    SAL_mutexLock(gstVencRkInfo.MutexHandle);
    /* 编码器消耗后回收通道 */
    if (isVencGroupUsed & (1 << u32Chan))
    {
        isVencGroupUsed = isVencGroupUsed & (~(1 << u32Chan));
    }

    SAL_mutexUnlock(gstVencRkInfo.MutexHandle);
}

/**
 * @function   venc_checkResPrm
 * @brief    检查分辨率
 * @param[in]  UINT32 u32UserSetW, UINT32 u32UserSetH 宽高信息
 * @param[out] UINT32 *pUiW, UINT32 *pUiH 宽高信息
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_checkResPrm(UINT32 u32UserSetW, UINT32 u32UserSetH, UINT32 *pUiW, UINT32 *pUiH)
{
    /*607项目中宽为1080，高为1920，在全局初始化的时候已经下发(syschain)*/
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;

    /* 宽 */
    if (u32UserSetW <= VENC_MAX_WIDTH)
    {
        u32Width = u32UserSetW;
    }
    else
    {
        VENC_LOGW("Set Enc Prm Width(%d) is large than Max Width(%d), Please reset the resolution!!!\n", u32UserSetW, VENC_MAX_WIDTH);
        u32Width = VENC_MAX_WIDTH;
    }

    /* 高 */
    if (u32UserSetH <= VENC_MAX_HEIGHT)
    {
        u32Height = u32UserSetH;
    }
    else
    {
        VENC_LOGW("Set Enc Prm Height(%d) is large than Max Height(%d), Please reset the resolution !!!\n", u32UserSetH, VENC_MAX_HEIGHT);
        u32Height = VENC_MAX_HEIGHT;
    }

    /* todo: rk硬件编码最小只支持96*96的分辨率 */
    /* 3516av200 平台上的Jpeg编码最小分辨率是 32 * 32 */
    if (u32Width < VENC_MIN_WIDTH)
    {
        VENC_LOGW("Set Enc Prm Width(%d) is Smaller than Min Width(%d), Please reset the resolution !!!\n", u32UserSetW, VENC_MIN_WIDTH);
        u32Width = VENC_MIN_WIDTH;
    }

    if (u32Height < VENC_MIN_HEIGHT)
    {
        VENC_LOGW("Set Enc Prm Height(%d) is Smaller than Min Height(%d), Please reset the resolution !!!\n", u32UserSetH, VENC_MIN_HEIGHT);
        u32Height = VENC_MIN_HEIGHT;
    }

    *pUiW = u32Width;
    *pUiH = u32Height;

    return SAL_SOK;
}

/**
 * @function   venc_rk_destroy
 * @brief   销毁编码器
 * @param[in]  void * pHandle 编码句柄
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_destroy(void *pHandle)
{
    RK_S32 s32Ret = RK_SUCCESS;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_STREAM_S *pstStream = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    /* 关闭文件句柄 */
    s32Ret = RK_MPI_VENC_CloseFd(pstVencHalChnPrm->uiChn);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_CloseFd [%d] faild with %#x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VENC_DestroyChn(pstVencHalChnPrm->uiChn);
    if (s32Ret != RK_SUCCESS)
    {
        VENC_LOGE("DestroyChn chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    if (RK_VIDEO_ID_JPEG == pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
        if (NULL != pstVencHalChnPrm->pVirtPack)
        {
            SAL_memfree(pstVencHalChnPrm->pVirtPack, "venc", "rk_enc");
            pstVencHalChnPrm->pVirtPack = NULL;
        }
    }
    else
    {
        /* 释放底层编码器所需内存 */
        pstStream = &pstVencHalChnPrm->stStream;
        if (NULL != pstStream->pstPack)
        {
            SAL_memfree(pstStream->pstPack, "venc", "rk_enc");
            pstStream->pstPack = NULL;
        }
    }

    /* 释放编码器通道 */
    venc_rk_putGroup(pstVencHalChnPrm->uiChn);

    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->isCreate = SAL_FALSE;

    VENC_LOGI("VENC Chn HalChn %d Destroy success.\n", pstVencHalChnPrm->uiChn);
    return SAL_SOK;
}

/**
 * @function   venc_rk_stop
 * @brief    停止编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_stop(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    RK_S32 s32Ret = RK_SUCCESS;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (1 == pstVencHalChnPrm->uibStart)
    {
        s32Ret = RK_MPI_VENC_StopRecvFrame(pstVencHalChnPrm->uiChn);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc RK_MPI_VENC_StopRecvFrame Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }
        
//        s32Ret = RK_MPI_VENC_ResetChn(pstVencHalChnPrm->uiChn);
//        if (RK_SUCCESS != s32Ret)
//        {
//            VENC_LOGE("Venc RK_MPI_VENC_ResetChn Chn %d faild with %#x! \n",
//                      pstVencHalChnPrm->uiChn, s32Ret);
//            return SAL_FAIL;
//        }

        pstVencHalChnPrm->uibStart = 0;
    }

    return SAL_SOK;
}

/**
 * @function   venc_rk_delete
 * @brief    销毁编码器
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_delete(void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }
    
    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    s32Ret = venc_rk_stop(pHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("StopRecvPic chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    /* 销毁编码器通道 */
    s32Ret = venc_rk_destroy(pHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("DestroyChn chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_rk_start
 * @brief    开始编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_start(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_RECV_PIC_PARAM_S stRecvParam = {0};
    RK_S32 s32Ret = RK_SUCCESS;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    stRecvParam.s32RecvPicNum = -1;
    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;


    if (SAL_FALSE == pstVencHalChnPrm->uibStart)
    {
        s32Ret = RK_MPI_VENC_StopRecvFrame(pstVencHalChnPrm->uiChn);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc RK_MPI_VENC_StopRecvFrame Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        s32Ret = RK_MPI_VENC_ResetChn(pstVencHalChnPrm->uiChn);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc RK_MPI_VENC_ResetChn Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            /* return SAL_FAIL; */
        }

        s32Ret = RK_MPI_VENC_StartRecvFrame(pstVencHalChnPrm->uiChn, &stRecvParam);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc RK_MPI_VENC_StartRecvPic Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        pstVencHalChnPrm->uibStart = SAL_TRUE;
    }

    return SAL_SOK;
}


/**
 * @function   venc_setJpegPrm
 * @brief   配置编码参数
 * @param[in]  UINT32 u32Chn 编码通道
 * @param[in]  SAL_VideoFrameParam *pstVencHalChnPrm 编码参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_setJpegPrm(UINT32 u32Chn, SAL_VideoFrameParam *pstVencHalChnPrm)
{
    RK_S32 s32Ret = RK_SUCCESS;

    VENC_CHN_ATTR_S stAttr = {0};
    VENC_CHN_PARAM_S stChnParam = {0};
    VENC_JPEG_PARAM_S stJpegParam = {0};
    UINT32 u32JpgQuality = 0;
    UINT32 u32W = 0;
    UINT32 u32H = 0;

    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

    if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_GetChnAttr(u32Chn, &stAttr)))
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, %x !!!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    if (RK_VIDEO_ID_JPEG == stAttr.stVencAttr.enType)
    {
        memset(&stJpegParam, 0, sizeof(VENC_JPEG_PARAM_S));
        if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_GetJpegParam(u32Chn, &stJpegParam)))
        {
            VENC_LOGE("JPEG Chn %d get Chn Param Failed, %x !!!\n", u32Chn, s32Ret);
            return SAL_FAIL;
        }

        if (pstVencHalChnPrm->quality < 1)
        {
            u32JpgQuality = 1;
        }
        else if (pstVencHalChnPrm->quality > 99)
        {
            u32JpgQuality = 99;
        }
        else
        {
            u32JpgQuality = pstVencHalChnPrm->quality;
        }

        stJpegParam.u32Qfactor = u32JpgQuality;
        if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_SetJpegParam(u32Chn, &stJpegParam)))
        {
            VENC_LOGE("JPEG Chn %d Set Chn Param Failed, %x !!!\n", u32Chn, s32Ret);
            return SAL_FAIL;
        }

        venc_rk_checkResPrm(pstVencHalChnPrm->width,
                         pstVencHalChnPrm->height,
                         &u32W, &u32H);

        stAttr.stVencAttr.u32PicWidth  = u32W;
        stAttr.stVencAttr.u32PicHeight = u32H;
        /* rk 的jpeg编码虚宽虚高要16字节对齐 */
        stAttr.stVencAttr.u32VirWidth  = SAL_align(u32W, 16);
        stAttr.stVencAttr.u32VirHeight = SAL_align(u32H, 16);
        
        VENC_LOGD("u32Chn %d u32PicWidth %d u32PicHeight %d, u32VirWidth %d,u32VirHeight %d, dataFormat%#x,FrmRate %d !!!\n",
                  u32Chn, stAttr.stVencAttr.u32PicWidth, stAttr.stVencAttr.u32PicHeight, 
                  stAttr.stVencAttr.u32VirWidth,stAttr.stVencAttr.u32VirHeight, pstVencHalChnPrm->dataFormat,
                  stChnParam.stFrameRate.s32DstFrmRateNum);

    }
    else
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, Enc Type Failed , %d  !!!\n", u32Chn, stAttr.stVencAttr.enType);
        return SAL_FAIL;
    }

    if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_SetChnAttr(u32Chn, &stAttr)))
    {
        VENC_LOGE("u32PicWidth %d u32PicHeight %d, FrmRate %d !!!\n",
                  stAttr.stVencAttr.u32PicWidth, stAttr.stVencAttr.u32PicHeight, stChnParam.stFrameRate.s32DstFrmRateNum);
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, %x !!!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_rk_setJpegCropPrm
 * @brief    配置编码通道裁剪参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VENC_CROP_INFO_S *pstCropInfo crop信息
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_setH26xScale(void *pHandle, UINT32 u32Width, UINT32 u32Height)
{
    RK_S32 s32Ret = RK_SUCCESS;
    VENC_CHN_PARAM_S stChnParam = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    CAPB_VENC    *pstCapbVenc = {0};

    if ( (NULL == pHandle))
    {
        VENC_LOGE("pHandle or pstScaleInfo is null\n");
        return SAL_FAIL;
    }
    
    pstCapbVenc = capb_get_venc();

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    s32Ret = RK_MPI_VENC_GetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, ret: 0x%x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    /* venc_scale=0 表示编码模块内不做缩放 */
    if (0 == pstCapbVenc->venc_scale)
    {
        stChnParam.stFrameRate.bEnable = RK_FALSE;
        s32Ret = RK_MPI_VENC_SetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc Chn %d Set Chn Attr FrameRate ctrl Failed, ret: 0x%x !!!\n", pstVencHalChnPrm->uiChn,s32Ret);
            return SAL_FAIL;
        }
    }
    else
    {
        stChnParam.stCropCfg.enCropType = VENC_CROP_SCALE;//pstScaleInfo->enCropType;
        
        /* rk芯片裁剪缩放x/y/w/h,必须 2像素对齐 */
        stChnParam.stCropCfg.stScaleRect.stSrc.s32X = 0;//SAL_align(pstScaleInfo->stScaleRect.stSrc.s32X, 2);
        stChnParam.stCropCfg.stScaleRect.stSrc.s32Y = 0;//SAL_align(pstScaleInfo->stScaleRect.stSrc.s32Y, 2);
        stChnParam.stCropCfg.stScaleRect.stSrc.u32Width = 0;//SAL_align(pstScaleInfo->stScaleRect.stSrc.u32Width, 2);
        stChnParam.stCropCfg.stScaleRect.stSrc.u32Height = 0;//SAL_align(pstScaleInfo->stScaleRect.stSrc.u32Height, 2);
        
        stChnParam.stCropCfg.stScaleRect.stDst.s32X = 0;//SAL_align(pstScaleInfo->stScaleRect.stDst.s32X, 2);
        stChnParam.stCropCfg.stScaleRect.stDst.s32Y = 0;//SAL_align(pstScaleInfo->stScaleRect.stDst.s32Y, 2);
        stChnParam.stCropCfg.stScaleRect.stDst.u32Width = SAL_align(u32Width,2);//(pstScaleInfo->stScaleRect.stDst.u32Width, 2);
        stChnParam.stCropCfg.stScaleRect.stDst.u32Height = SAL_align(u32Height,2);//(pstScaleInfo->stScaleRect.stDst.u32Height, 2);
        
        VENC_LOGD("Venc Chn %d Set stScaleRect[%d-%d] w*h[%d-%d] !!!\n", pstVencHalChnPrm->uiChn, stChnParam.stCropCfg.stCropRect.s32X,
            stChnParam.stCropCfg.stCropRect.s32Y,stChnParam.stCropCfg.stCropRect.u32Width,stChnParam.stCropCfg.stCropRect.u32Height);

        s32Ret = RK_MPI_VENC_SetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc Chn %d Set Chn Attr Failed, ret: 0x%x Set stCropRectxy[%d-%d] w*h[%d-%d] !!!\n", pstVencHalChnPrm->uiChn,
                s32Ret,stChnParam.stCropCfg.stCropRect.s32X,stChnParam.stCropCfg.stCropRect.s32Y,
                stChnParam.stCropCfg.stCropRect.u32Width,stChnParam.stCropCfg.stCropRect.u32Height);
            return SAL_FAIL;
        }
    }
    return SAL_SOK;
}

/**
 * @function   venc_rk_setH26xPrm
 * @brief    配置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_setH26xPrm(void *pHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SAL_VideoFrameParam *pstVencChnInfo = NULL;

    VENC_CHN_ATTR_S *pstAttr = NULL;
    VENC_RC_PARAM_S stRcParam = {0};
    UINT32 u32ActualWid = 0;
    UINT32 u32ActualHei = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;
    PIXEL_FORMAT_E enPixelFmt = 0;
    CAPB_DUP       *pCapbDup  = capb_get_dup();
    VENC_FPS_INFO_S stVencFpsInfo = {0};


    if ((NULL == pHandle) || (NULL == pstInPrm))
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = pstInPrm;
    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    if (SAL_FALSE == pstVencHalChnPrm->isCreate)
    {
        VENC_LOGE("VENC Hal chan %d no create.\n", pstVencHalChnPrm->uiChn);
        return SAL_SOK;
    }
    
    s32Ret    = fmtConvert_rk_sal2rk(pCapbDup->enOutputSalPixelFmt, &enPixelFmt);

    pstAttr = &pstVencHalChnPrm->stVencChnAttr;
    if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_GetChnAttr(pstVencHalChnPrm->uiChn, pstAttr)))
    {
        VENC_LOGE("Venc Hal Chn %d:%d Get Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    venc_rk_checkResPrm(pstVencChnInfo->width,
                     pstVencChnInfo->height,
                     &u32ActualWid,
                     &u32ActualHei);

    u32ViFps = (pstVencChnInfo->fps >> 16); /*高16位源fps，低16位编码fps*/
    u32EncFps = (pstVencChnInfo->fps & 0xffff);
    VENC_LOGI("src rate %d dst rate %d\n", u32ViFps, u32EncFps);

/* 安检机项目,编码模块不做帧率控制,放在vpss做帧率控制 */
#ifdef DSP_ISM
    stVencFpsInfo.u32SrcFrameRateNum  = 0;
    stVencFpsInfo.u32SrcFrameRateDen  = 0;
    stVencFpsInfo.fr32DstFrameRateNum = 0;
    stVencFpsInfo.fr32DstFrameRateDen = 0;
    stVencFpsInfo.u32Gop = 60;
#else
    stVencFpsInfo.u32SrcFrameRateNum  = (0 == u32ViFps) ? 60 : u32ViFps;
    stVencFpsInfo.u32SrcFrameRateDen  = 1;
    stVencFpsInfo.fr32DstFrameRateNum = u32EncFps > 0 ? u32EncFps : 30;
    stVencFpsInfo.fr32DstFrameRateDen = 1;
    stVencFpsInfo.u32Gop = u32EncFps * 2;
#endif

    pstVencHalChnPrm->vencFps = u32EncFps;
    pstAttr->stVencAttr.u32PicWidth = u32ActualWid;
    pstAttr->stVencAttr.u32PicHeight = u32ActualHei;
    pstAttr->stVencAttr.u32VirWidth  = SAL_align(u32ActualWid, 16);
    pstAttr->stVencAttr.u32VirHeight = SAL_align(u32ActualHei, 16);
    pstAttr->stVencAttr.enPixelFormat = enPixelFmt;
    
    if (SAL_SOK != venc_rk_setH26xScale(pstVencHalChnPrm,u32ActualWid,u32ActualHei))
    {
        VENC_LOGE("venc_setJpegCropPrm error !\n");
        return SAL_FAIL;
    }

    if (RK_VIDEO_ID_HEVC == venc_rk_getEncType(pstVencChnInfo->encodeType))
    {
        pstAttr->stRcAttr.enRcMode = (pstVencChnInfo->bpsType == 0) ? VENC_RC_MODE_H265VBR : VENC_RC_MODE_H265CBR;
        pstAttr->stVencAttr.enType = RK_VIDEO_ID_HEVC;
        if (VENC_RC_MODE_H265VBR == pstAttr->stRcAttr.enRcMode)
        {
            pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
            pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
            pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
            pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
            pstAttr->stRcAttr.stH265Vbr.u32Gop = stVencFpsInfo.u32Gop;
            pstAttr->stRcAttr.stH265Vbr.u32BitRate = pstVencChnInfo->bps;
            
            VENC_LOGW("set H265 VBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d \n",
                      pstVencHalChnPrm->uiChn, pstAttr->stVencAttr.u32PicWidth, pstAttr->stVencAttr.u32PicHeight, 
                      pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum, pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum,
                      pstAttr->stRcAttr.stH265Vbr.u32BitRate);
        }
        else if (VENC_RC_MODE_H265CBR == pstAttr->stRcAttr.enRcMode)
        {
            pstAttr->stRcAttr.stH265Cbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
            pstAttr->stRcAttr.stH265Cbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
            pstAttr->stRcAttr.stH265Cbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
            pstAttr->stRcAttr.stH265Cbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
            pstAttr->stRcAttr.stH265Cbr.u32Gop = stVencFpsInfo.u32Gop;
            pstAttr->stRcAttr.stH265Cbr.u32BitRate = pstVencChnInfo->bps;
            
            VENC_LOGW("set H265 CBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d \n",
                      pstVencHalChnPrm->uiChn, pstAttr->stVencAttr.u32PicWidth, pstAttr->stVencAttr.u32PicHeight, 
                      pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum, pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum,
                      pstAttr->stRcAttr.stH265Vbr.u32BitRate);

        }

        pstAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
        pstAttr->stGopAttr.s32VirIdrLen = 0;

        if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_SetChnAttr(pstVencHalChnPrm->uiChn, pstAttr)))
        {
            VENC_LOGE("Venc Chn %d:%d Set Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        return SAL_SOK;
    }
    else
    {
        /* 目前只有h265和h264 */
        pstAttr->stVencAttr.enType = RK_VIDEO_ID_AVC;
    }

    pstAttr->stRcAttr.enRcMode = (pstVencChnInfo->bpsType == 0) ? VENC_RC_MODE_H264VBR : VENC_RC_MODE_H264CBR;

    if (VENC_RC_MODE_H264VBR == pstAttr->stRcAttr.enRcMode)
    {
        VENC_LOGW("========= VBR RC control frame rate, Maxbitrate %d =========\n", pstVencChnInfo->bps);

        pstAttr->stRcAttr.stH264Vbr.u32BitRate = pstVencChnInfo->bps;
        pstAttr->stRcAttr.stH264Vbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
        pstAttr->stRcAttr.stH264Vbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
        pstAttr->stRcAttr.stH264Vbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
        pstAttr->stRcAttr.stH264Vbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
        pstAttr->stRcAttr.stH264Vbr.u32Gop = stVencFpsInfo.u32Gop;
        
        VENC_LOGW("set H264 VBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d \n",
                  pstVencHalChnPrm->uiChn, pstAttr->stVencAttr.u32PicWidth, pstAttr->stVencAttr.u32PicHeight, 
                  pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum, pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum,
                  pstAttr->stRcAttr.stH265Vbr.u32BitRate);
    }
    else if (VENC_RC_MODE_H264CBR == pstAttr->stRcAttr.enRcMode)
    {
        VENC_LOGW("========== CBR RC control frame rate, bitrate %d =========\n", pstVencChnInfo->bps);

        pstAttr->stRcAttr.stH264Cbr.u32BitRate = pstVencChnInfo->bps;
        pstAttr->stRcAttr.stH264Cbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
        pstAttr->stRcAttr.stH264Cbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
        pstAttr->stRcAttr.stH264Cbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
        pstAttr->stRcAttr.stH264Cbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
        pstAttr->stRcAttr.stH264Cbr.u32Gop = stVencFpsInfo.u32Gop;
        
        VENC_LOGW("set H264 VBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d \n",
                  pstVencHalChnPrm->uiChn, pstAttr->stVencAttr.u32PicWidth, pstAttr->stVencAttr.u32PicHeight, 
                  pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum, pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum,
                  pstAttr->stRcAttr.stH265Vbr.u32BitRate);
    }

    VENC_LOGI("HAL SrcFps %d DstFps %d !!!\n", u32ViFps, u32EncFps);

    pstAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pstAttr->stGopAttr.s32VirIdrLen  = 0;

    if (RK_SUCCESS != (s32Ret = RK_MPI_VENC_SetChnAttr(pstVencHalChnPrm->uiChn, pstAttr)))
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    SAL_clear(&stRcParam);
    s32Ret = RK_MPI_VENC_GetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    if (VENC_RC_MODE_H264VBR == pstAttr->stRcAttr.enRcMode)
    {
        stRcParam.s32FirstFrameStartQp = 25;
        stRcParam.stParamH264.u32StepQp = 4;
        stRcParam.stParamH264.u32MinQp = 10;
        stRcParam.stParamH264.u32MaxQp = 50;
        stRcParam.stParamH264.u32MinIQp = 10;
        stRcParam.stParamH264.u32MaxIQp = 50;
        stRcParam.stParamH264.s32DeltIpQp = -2;
        stRcParam.stParamH264.s32MaxReEncodeTimes = 2;

        VENC_LOGI("vbr StepQp [%d],pQp[%d,%d] IQp[%d,%d],DeltIpQp %d, ReEncodeTimes %d] !!!\n", stRcParam.stParamH264.u32StepQp, stRcParam.stParamH264.u32MinQp, stRcParam.stParamH264.u32MaxQp,
                  stRcParam.stParamH264.u32MinIQp, stRcParam.stParamH264.u32MaxIQp, stRcParam.stParamH264.s32DeltIpQp, stRcParam.stParamH264.s32MaxReEncodeTimes);
    }

    s32Ret = RK_MPI_VENC_SetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_SetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }
    /* rk不支持配置vui相关参数，暂时注释 */
#if 0
    s32Ret = RK_MPI_VENC_GetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_GetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    stVui.stVuiTimeInfo.timing_info_present_flag = 1;
    stVui.stVuiTimeInfo.time_scale = 60000;
    stVui.stVuiTimeInfo.num_units_in_tick = 30000 / stFrameRate.s32DstFrmRateNum;
    s32Ret = RK_MPI_VENC_SetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }
#endif
    return SAL_SOK;
}

/**
 * @function   venc_rk_setEncPrm
 * @brief    设置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_setEncPrm(void *ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SAL_VideoFrameParam stVencFrmPrm = {0};

    if ((NULL == ppHandle) || (NULL == pstInPrm))
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)ppHandle;

    if (RK_VIDEO_ID_JPEG == pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
        memset(&stVencFrmPrm, 0, sizeof(SAL_VideoFrameParam));

        stVencFrmPrm.width = pstInPrm->width;
        stVencFrmPrm.height = pstInPrm->height;
        stVencFrmPrm.quality = pstInPrm->quality;

        if (SAL_SOK != venc_rk_stop(ppHandle))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_rk_setJpegPrm(pstVencHalChnPrm->uiChn, &stVencFrmPrm))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_rk_start(ppHandle))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        s32Ret = venc_rk_setH26xPrm(ppHandle, pstInPrm);
        if (SAL_SOK != s32Ret)
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @function   venc_rk_requestIDR
 * @brief    强制I帧编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_requestIDR(void *pHandle)
{
    RK_S32 s32Ret = RK_SUCCESS;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    s32Ret = RK_MPI_VENC_RequestIDR(pstVencHalChnPrm->uiChn, SAL_TRUE);
    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_RequestIDR Chn %d faild with %#x! \n", pstVencHalChnPrm->uiChn, s32Ret);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   venc_rk_getFrame
 * @brief    从指定的编码通道获取最新的一帧编码码流，保证和VencHal_putFrame成对调用
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] void *pInPrm 获取编码码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_getFrame(void *pHandle, void *pInPrm)
{
    INT32 s32Ret = 0;
    INT32 i = 0;
    UINT32 u32Fd = 0;
    UINT32 u32RcType = 0;
    fd_set fdRread;
    UINT32 u32TimeOut = 0;
    UINT32 u32Cnt = 0;
    VOID  *pData = NULL;
//    RK_CODEC_ID_E enVencType;

    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    STREAM_FRAME_INFO_ST *pstFrameInfo = NULL;
    VENC_STREAM_S *pstStream = NULL;
    VENC_CHN_STATUS_S stStat = {0};

    if ((NULL == pHandle) || (NULL == pInPrm))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrameInfo = (STREAM_FRAME_INFO_ST *)pInPrm;
    pstStream = &pstVencHalChnPrm->stStream;
    if (NULL == pstStream->pstPack)
    {
        VENC_LOGE("Memory err!!!\n");
        return SAL_FAIL;
    }

    u32Fd = pstVencHalChnPrm->uiFd;
//    enVencType = pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType;
    
    /* 获取码流前先清0标志位 */
    pstFrameInfo->uiValid = 0;
    pstFrameInfo->uiNaluNum = 0;
    memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));
    u32TimeOut = 1000 / (pstVencHalChnPrm->vencFps > 0 ? pstVencHalChnPrm->vencFps : 1);

    do
    {
        FD_ZERO(&fdRread);
        FD_SET(u32Fd, &fdRread);
        s32Ret = SAL_mselect(u32Fd, 100);
        if (s32Ret < 0)
        {
            VENC_LOGE("Select Error \n");
            return SAL_FAIL;
        }
        else if (0 == s32Ret)
        {
            /* return SAL_FAIL; */
            if ((0 != pstVencHalChnPrm->uiState) || (u32Cnt * 100 > u32TimeOut))
            {
                return SAL_FAIL;
            }

            u32Cnt++;
            continue;
        }
        else
        {
            if (FD_ISSET(u32Fd, &fdRread))
            {
                s32Ret = RK_MPI_VENC_QueryStatus(pstVencHalChnPrm->uiChn, &stStat);
                if (RK_SUCCESS != s32Ret)
                {
                    VENC_LOGE("RK_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
                    return SAL_FAIL;
                }

                if (0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                {
                    if (0 != pstVencHalChnPrm->uiState)
                    {
                        VENC_LOGE("RK_MPI_VENC_QueryStatus chn[%d][%d,%d]\n", pstVencHalChnPrm->uiChn, stStat.u32CurPacks, stStat.u32LeftStreamFrames);
                        return SAL_FAIL;
                    }

                    continue;
                }

                pstStream->u32PackCount = stStat.u32CurPacks;
                
                s32Ret = RK_MPI_VENC_GetStream(pstVencHalChnPrm->uiChn, pstStream, 1000);
                if (RK_SUCCESS != s32Ret)
                {

                    VENC_LOGE("VENC chan %d RK_MPI_VENC_GetStream failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
                    return SAL_FAIL;
                }
                
            #ifdef VENC_HAL_SAVE_BITS_DATA
                /* 保存码流到文件 */
                venc_saveH264Stream(file[pstVencHalChnPrm->uiChn], pstStream);
            #endif
            #if 0
                if (RK_VIDEO_ID_AVC == enVencType)
                {
                    pstStream->u32PackCount = (0 == pstStream->stH264Info.enRefType) ? 4 : 1;
                }
                else if(RK_VIDEO_ID_HEVC == enVencType)
                {
                    pstStream->u32PackCount = (0 == pstStream->stH264Info.enRefType) ? 5 : 1;
                }
            #endif
                /* 提取信息 */
                /* pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 3) ? (H264_TYPE_IFRAME): (H264_TYPE_PFRAME); */
                for (i = 0; i < pstStream->u32PackCount; i++)
                {
                    pData = RK_MPI_MB_Handle2VirAddr(pstStream->pstPack[i].pMbBlk);
                    if (RK_NULL == pData)
                    {
                        VENC_LOGE("VENC chan %d i %d RK_MPI_MB_Handle2VirAddr failed \n", pstVencHalChnPrm->uiChn, i);
                        return SAL_FAIL;
                    }
/* 调试时使用,暂时保留 */
#if 0
                    if (RK_VIDEO_ID_AVC == enVencType)
                    {
                        VENC_LOGE("wzy VENC chan %d enRefType %d \n", pstVencHalChnPrm->uiChn, pstStream->stH264Info.enRefType);
                        
                        //pstStream->pstPack[i].DataType.enH264EType;
                        VENC_LOGE("wzy VENC chan %d enH264EType %d \n", pstVencHalChnPrm->uiChn, pstStream->pstPack[i].DataType.enH264EType);
                        
                    }
                    else if (RK_VIDEO_ID_HEVC == enVencType)
                    {
                        VENC_LOGE("wzy VENC chan %d enRefType %d \n", pstVencHalChnPrm->uiChn, pstStream->stH265Info.enRefType);
                        
                        //pstStream->pstPack[i].DataType.enH264EType;
                        VENC_LOGE("wzy VENC chan %d enH264EType %d \n", pstVencHalChnPrm->uiChn, pstStream->pstPack[i].DataType.enH265EType);
                    }
                    else
                    {
                        VENC_LOGE("VENC chan %d enVencType unknown %d \n", pstVencHalChnPrm->uiChn,enVencType);
                    }
                    
                    
                    VENC_LOGE("wzy VENC chan %d i %d u32Offset %d u32Len %d !\n", pstVencHalChnPrm->uiChn,i , pstStream->pstPack[i].u32Offset,
                         pstStream->pstPack[i].u32Len);
#endif
                    pstFrameInfo->stNaluInfo[i].pucNaluPtr = pData + pstStream->pstPack[i].u32Offset;
                    pstFrameInfo->stNaluInfo[i].uiNaluLen = pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
                    pstFrameInfo->stNaluInfo[i].u64PTS = pstStream->pstPack[i].u64PTS / 1000;
                    
                    pstFrameInfo->uiNaluNum++;
                    if (i == 0)
                    {
                        /* VENC_LOGI("uiChn %d,Nalu Pts %llu,stream Pts %llu\n",uiChn,pstFrameInfo->stNaluInfo[i].u64PTS ,pstStream->pstPack[i].u64PTS); */
                        if (streamPTS[pstVencHalChnPrm->uiChn] >= pstFrameInfo->stNaluInfo[i].u64PTS)
                        {
                            RK_U64 p64CurPts = 0;
                            RK_MPI_SYS_GetCurPTS(&p64CurPts);
                            /* todo: 因时间戳有点问题, 该处会引起刷屏打印，暂时注释 */
                            VENC_LOGD("p64CurPts is %lu old Pts %llu,cur Pts %llu\n", p64CurPts / 1000, streamPTS[pstVencHalChnPrm->uiChn], pstFrameInfo->stNaluInfo[i].u64PTS);
                            //pstFrameInfo->stNaluInfo[i].u64PTS += 2;
                        }

                        streamPTS[pstVencHalChnPrm->uiChn] = pstFrameInfo->stNaluInfo[i].u64PTS;
                    }

                 #if 0 /* 开启此处条件判断和宏 TEST_H264_STREAM 便可以直接用 vlc 拉 H264 裸流 */
                    UnitTest_rtspDemoPutStreamData(pstFrameInfo->stNaluInfo[i].pucNaluPtr, \
                                                   pstFrameInfo->stNaluInfo[i].uiNaluLen, \
                                                   pstFrameInfo->stNaluInfo[i].u64PTS, \
                                                   pstStream->pstPack[i].DataType.enH264EType, \
                                                   pstStream->u32Seq);
                #endif
                }

                pstFrameInfo->uiFrameWidth = pstVencHalChnPrm->stVencChnAttr.stVencAttr.u32PicWidth;
                pstFrameInfo->uiFrameHeight = pstVencHalChnPrm->stVencChnAttr.stVencAttr.u32PicHeight;

                u32RcType = pstVencHalChnPrm->stVencChnAttr.stRcAttr.enRcMode;
                if (VENC_RC_MODE_H264CBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRateNum;
                    pstFrameInfo->eFrameType = (0 == pstStream->stH264Info.enRefType) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                }
                else if (VENC_RC_MODE_H264VBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH264Vbr.fr32DstFrameRateNum;
                    pstFrameInfo->eFrameType = (0 == pstStream->stH264Info.enRefType) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                }
                else if (VENC_RC_MODE_H265CBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRateNum;
                    pstFrameInfo->eFrameType = (0 == pstStream->stH265Info.enRefType) ? (STREAM_TYPE_H265_IFRAME) : (STREAM_TYPE_H265_PFRAME);
                }
                else if (VENC_RC_MODE_H265VBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH265Vbr.fr32DstFrameRateNum;
                    pstFrameInfo->eFrameType = (0 == pstStream->stH265Info.enRefType) ? (STREAM_TYPE_H265_IFRAME) : (STREAM_TYPE_H265_PFRAME);
                }
                else
                {
                    pstFrameInfo->uiFps = 25;
                    pstFrameInfo->eFrameType = (0 == pstStream->stH265Info.enRefType) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                    VENC_LOGW("u32RcType error is %d\n", u32RcType);
                }

                pstFrameInfo->uiValid = 1;
                return SAL_SOK;
            }
            else
            {
                return SAL_FAIL;
            }
        }
    }
    while (1);

    return SAL_SOK;
}


/**
 * @function   venc_rk_putFrame
 * @brief    释放指定通道的码流缓存，和VencHal_getFrame 成对出现
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_putFrame(void *pHandle)
{
    RK_S32 s32Ret = RK_SUCCESS;
    VENC_STREAM_S *pstStream = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstStream = &pstVencHalChnPrm->stStream;
    if ((NULL == pstStream) || (NULL == pstStream->pstPack))
    {
        VENC_LOGE("Stream is NULL !!!\n");
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VENC_ReleaseStream(pstVencHalChnPrm->uiChn, pstStream);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("s32Ret %x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   venc_rk_getChan
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] UINT32 *poutChan plat层通道号指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 venc_rk_getChan(void *pHandle, UINT32 *pOutChan)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if ((NULL == pHandle) || (NULL == pOutChan))
    {
        VENC_LOGE("inv prm\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    *pOutChan = pstVencHalChnPrm->uiChn;

    return SAL_SOK;
}


/**
 * @function   venc_rk_setStatus
 * @brief    设置编码状态，方便drv层管理编码、取流流程
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VOID *pStr 状态信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_setStatus(void *pHandle, VOID *pStr)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    UINT32 u32Status = 0;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    u32Status = *((UINT32 *)pStr);

    pstVencHalChnPrm->uiState = u32Status;

    return SAL_SOK;
}

/**
 * @function   venc_rk_setJpegCropPrm
 * @brief    配置编码通道裁剪参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VENC_CROP_INFO_S *pstCropInfo crop信息
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_setJpegCropPrm(void *pHandle, VENC_CROP_INFO_S *pstCropInfo)
{
    RK_S32 s32Ret = RK_SUCCESS;
    VENC_CHN_PARAM_S stChnParam = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (NULL == pstCropInfo || SAL_TRUE != pstCropInfo->enCropType)
    {
        return SAL_SOK;
    }

    s32Ret = RK_MPI_VENC_GetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, ret: 0x%x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    stChnParam.stCropCfg.enCropType = pstCropInfo->enCropType;
    
    /* rk芯片裁剪x/w/h,必须 2像素对齐 */
    stChnParam.stCropCfg.stCropRect.s32X = SAL_align(pstCropInfo->stCropRect.s32X, 2);
    stChnParam.stCropCfg.stCropRect.s32Y = SAL_align(pstCropInfo->stCropRect.s32Y, 2);
    stChnParam.stCropCfg.stCropRect.u32Width = SAL_align(pstCropInfo->stCropRect.u32Width, 2);
    stChnParam.stCropCfg.stCropRect.u32Height = SAL_align(pstCropInfo->stCropRect.u32Height, 2);
    
    VENC_LOGD("Venc Chn %d Set stCropRectxy[%d-%d] w*h[%d-%d] !!!\n", pstVencHalChnPrm->uiChn, stChnParam.stCropCfg.stCropRect.s32X,
        stChnParam.stCropCfg.stCropRect.s32Y,stChnParam.stCropCfg.stCropRect.u32Width,stChnParam.stCropCfg.stCropRect.u32Height);

    s32Ret = RK_MPI_VENC_SetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, ret: 0x%x Set stCropRectxy[%d-%d] w*h[%d-%d] !!!\n", pstVencHalChnPrm->uiChn,
            s32Ret,stChnParam.stCropCfg.stCropRect.s32X,stChnParam.stCropCfg.stCropRect.s32Y,
            stChnParam.stCropCfg.stCropRect.u32Width,stChnParam.stCropCfg.stCropRect.u32Height);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_rk_sendFrame
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VOID *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_sendFrame(void *pHandle, VOID *pStr)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    RK_S32 s32Ret = RK_SUCCESS;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrame = (VIDEO_FRAME_INFO_S *)pStr;

    /* 调试信息 */
#if 0
    VENC_LOGD("frm info: w %d h %d virw %d virh %d Field %d pixelformat %#x Videoformat %d CompressMode %d dynamic %d color_gamut %d time_ref %d pts %lu\n",
              pstFrame->stVFrame.u32Width, pstFrame->stVFrame.u32Height,
              pstFrame->stVFrame.u32VirWidth, pstFrame->stVFrame.u32VirHeight,pstFrame->stVFrame.enField,
              pstFrame->stVFrame.enPixelFormat, pstFrame->stVFrame.enVideoFormat, pstFrame->stVFrame.enCompressMode,
              pstFrame->stVFrame.enDynamicRange, pstFrame->stVFrame.enColorGamut,
              pstFrame->stVFrame.u32TimeRef, pstFrame->stVFrame.u64PTS);

    VENC_LOGD("frm info:pMbBlk %p pVirAddr[0] %p pVirAddr[1] %p u64PrivateData %lu u32FrameFlag %d \n",
              pstFrame->stVFrame.pMbBlk, pstFrame->stVFrame.pVirAddr[0],
              pstFrame->stVFrame.pVirAddr[1], pstFrame->stVFrame.u64PrivateData, pstFrame->stVFrame.u32FrameFlag);
#endif

    if (RK_VIDEO_ID_JPEG == pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
        s32Ret = RK_MPI_VENC_SendFrame(pstVencHalChnPrm->uiChn, pstFrame, -1);
    }
    else
    {
        s32Ret = RK_MPI_VENC_SendFrame(pstVencHalChnPrm->uiChn, pstFrame, 500);
    }

    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC Chn %d Send Frame fail!, s32Ret = %x\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_getJpegFrame
 * @brief    Jpeg编码通道获取一帧数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out]  INT8 *pcJpeg 编码码流
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_getJpegFrame(void *pHandle, INT8 *pcJpeg, UINT32 *pJpegSize)
{
    RK_S32 s32Ret = RK_SUCCESS;
    INT32 i = 0;
    UINT32 u32Size = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    VENC_STREAM_S stStream = {0};
    VENC_CHN_STATUS_S stStat = {0};
    VENC_PACK_S *pstData = NULL;;

    UINT32 QueryCount = 0;
    VOID  *pData = NULL;

    if ((NULL == pHandle) || (NULL == pcJpeg) || (NULL == pJpegSize))
    {
        VENC_LOGE("inv ptr\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));

    while (0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
    {
        s32Ret = RK_MPI_VENC_QueryStatus(pstVencHalChnPrm->uiChn, &stStat);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("RK_MPI_VENC_Query chn[%d] failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        QueryCount++;
        if (QueryCount >= 100)
        {
            VENC_LOGE("chn[%d] Can not get jpeg frame, cost time:%d ms!\n", pstVencHalChnPrm->uiChn, QueryCount * 10);
            return SAL_FAIL;
        }

        SAL_msleep(10);
    }

    stStream.pstPack = pstVencHalChnPrm->pVirtPack;
    if (NULL == stStream.pstPack)
    {
        VENC_LOGE("chn[%d] malloc stream pack failed!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    stStream.u32PackCount = stStat.u32CurPacks;

    s32Ret = RK_MPI_VENC_GetStream(pstVencHalChnPrm->uiChn, &stStream, -1);
    if (RK_SUCCESS != s32Ret)
    {
        stStream.pstPack = NULL;
        VENC_LOGE("chn %d RK_MPI_VENC_GetStream failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    u32Size = 0;
    for (i = 0; i < stStream.u32PackCount; i++)
    {
        pstData = &stStream.pstPack[i];
        
        pData = RK_MPI_MB_Handle2VirAddr(pstData->pMbBlk);
        if (RK_NULL == pData)
        {
        
            VENC_LOGE("VENC chan %d RK_MPI_MB_Handle2VirAddr failed \n", pstVencHalChnPrm->uiChn);
            return SAL_FAIL;
        }

        if (pstData->u32Len > 0 && pstData->u32Len > pstData->u32Offset)
        {
            memcpy(pcJpeg + u32Size,
                   pData + pstData->u32Offset,
                   pstData->u32Len - pstData->u32Offset);

            u32Size += pstData->u32Len - pstData->u32Offset;
        }
    }

    *pJpegSize = u32Size;
    s32Ret = RK_MPI_VENC_ReleaseStream(pstVencHalChnPrm->uiChn, &stStream);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC release fail!\n");
        stStream.pstPack = NULL;
        return SAL_FAIL;
    }

    memset(pstVencHalChnPrm->pVirtPack, 0x00, pstVencHalChnPrm->packBufLen);
    stStream.pstPack = NULL;

    return SAL_SOK;
}

/**
 * @function   venc_rk_copyYuvFrame
 * @brief    数据拷贝做格式转换
 * @param[in]  VIDEO_FRAME_INFO_S *pstSrcFrameInfo 输入数据帧
 * @param[out]  VIDEO_FRAME_INFO_S *pstDstFrameInfo 输出数据帧
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_rk_copyYuvFrame(VIDEO_FRAME_INFO_S *pstSrcFrameInfo, VIDEO_FRAME_INFO_S *pstDstFrameInfo)
{
    TDE_HAL_SURFACE  srcSurface, dstSurface;
    TDE_HAL_RECT     srcRect, dstRect;
    INT32 s32Ret   = SAL_SOK;

    if(pstSrcFrameInfo == NULL || pstDstFrameInfo == NULL)
    {
         SAL_LOGE("pstSrcFrameInfo NULL or pstDstFrameInfo is NULL\n");
         return SAL_FAIL;
    }

    memset(&srcSurface, 0, sizeof(TDE_HAL_SURFACE));
    memset(&dstSurface, 0, sizeof(TDE_HAL_SURFACE));
    memset(&srcRect, 0, sizeof(TDE_HAL_RECT));
    memset(&dstRect, 0, sizeof(TDE_HAL_RECT));

    srcSurface.u32Width   = pstSrcFrameInfo->stVFrame.u32VirWidth; /* RK tde 拷贝时入参里没有virWidth，srcSurface.u32Width需要赋值成virWidth */
    srcSurface.u32Height  = pstSrcFrameInfo->stVFrame.u32Height;
    srcSurface.u32Stride  = pstSrcFrameInfo->stVFrame.u32VirWidth;
    srcSurface.PhyAddr = 0;
    fmtConvert_rk_rk2sal(pstSrcFrameInfo->stVFrame.enPixelFormat, &srcSurface.enColorFmt);
    srcSurface.vbBlk =  (UINT64)pstSrcFrameInfo->stVFrame.pMbBlk;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width =  pstSrcFrameInfo->stVFrame.u32Width;
    srcRect.u32Height = pstSrcFrameInfo->stVFrame.u32Height;

    dstSurface.u32Width   = pstSrcFrameInfo->stVFrame.u32Width;
    dstSurface.u32Height  = pstSrcFrameInfo->stVFrame.u32Height;
    dstSurface.u32Stride  = pstSrcFrameInfo->stVFrame.u32Width;
    dstSurface.PhyAddr = 0;
    dstSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420SP_UV;   /*编码jpeg使用yuv数据*/
    dstSurface.vbBlk = (UINT64)pstDstFrameInfo->stVFrame.pMbBlk;
    dstRect.s32Xpos   = 0;
    dstRect.s32Ypos   = 0;
    dstRect.u32Width  =  pstSrcFrameInfo->stVFrame.u32Width;
    dstRect.u32Height = pstSrcFrameInfo->stVFrame.u32Height;

    s32Ret = tde_hal_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, 0);
    if(s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        s32Ret = SAL_FAIL;
    }


    return s32Ret;
}

/**
 * @function   venc_rk_encJpeg
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @param[in]  CROP_S *pstCropInfo 编码crop信息
 * @param[in]  BOOL bSetPrm 编码参数更新，用于重配编码器
 * @param[out] INT8 *pJpeg 编码输出JPEG码流
 * @param[out] INT8 *pJpegSize 编码输出JPEG码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_encJpeg(void *pHandle, SYSTEM_FRAME_INFO *pstFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm)
{
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpeg, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpegSize, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pHandle, SAL_FAIL);
    SYSTEM_FRAME_INFO stPicSystemFrame = {0};
    VIDEO_FRAME_INFO_S *pstPicFrameInfo = NULL;
    INT32 s32Ret = SAL_SOK; 

    VIDEO_FRAME_INFO_S *pstFrameInfo = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrameInfo, SAL_FAIL);

    SAL_VideoFrameParam stJpegPicInfo = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    VENC_CROP_INFO_S stCropInfo = {0};

    if ((SAL_TRUE == bSetPrm) || (NULL != pstCropInfo)) /*抠图模式必然要设置参数*/
    {
        stJpegPicInfo.quality = 99; /* 80; */

        if (NULL != pstCropInfo)
        {
            if (SAL_TRUE == pstCropInfo->u32CropEnable)
            {
                stCropInfo.enCropType = VENC_CROP_ONLY;
            }
            stCropInfo.stCropRect.u32Height = (RK_U32)pstCropInfo->u32H;
            stCropInfo.stCropRect.u32Width = (RK_U32)pstCropInfo->u32W;
            stCropInfo.stCropRect.s32X = (RK_S32)pstCropInfo->u32X;
            stCropInfo.stCropRect.s32Y = (RK_S32)pstCropInfo->u32Y;

            stJpegPicInfo.height = pstFrameInfo->stVFrame.u32Height;
            stJpegPicInfo.width  = pstFrameInfo->stVFrame.u32Width;
        }
        else
        {
            stCropInfo.enCropType = VENC_CROP_NONE;
            stCropInfo.stCropRect.u32Height = (RK_U32)pstFrameInfo->stVFrame.u32Height;
            stCropInfo.stCropRect.u32Width = (RK_U32)pstFrameInfo->stVFrame.u32Width;
            stCropInfo.stCropRect.s32X = (RK_S32)0;
            stCropInfo.stCropRect.s32Y = (RK_U32)0;
            stJpegPicInfo.height = pstFrameInfo->stVFrame.u32Height;
            stJpegPicInfo.width = pstFrameInfo->stVFrame.u32Width;
        }

        if (SAL_SOK != venc_rk_setJpegPrm(pstVencHalChnPrm->uiChn, &stJpegPicInfo))
        {
            VENC_LOGE("venc_setJpegPrm fail!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_rk_setJpegCropPrm(pHandle, &stCropInfo))
        {
            VENC_LOGE("venc_setJpegCropPrm error !\n");
            return SAL_FAIL;
        }
    }

    if (SAL_SOK != venc_rk_start(pHandle))
    {
        VENC_LOGE("VencHal_drvStart error !\n");
        return SAL_FAIL;
    }
    
#if 0
    {
        UINT8 *data = NULL;
        data = RK_MPI_MB_Handle2VirAddr(pstFrameInfo->stVFrame.pMbBlk);
        VENC_LOGE("enPixelFormat %#x!\n",pstFrameInfo->stVFrame.enPixelFormat);
        
        dump_jpeg(pstVencHalChnPrm->uiChn, pstFrameInfo->stVFrame.u32Height * pstFrameInfo->stVFrame.u32Width*3,(UINT8 *)data,
            stCropInfo.stCropRect.u32Width,stCropInfo.stCropRect.u32Height);
    }
#endif
    SAL_LOGD("w:%u, h:%u, vir w:%u, vir h:%u\n", pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height, 
             pstFrameInfo->stVFrame.u32VirWidth, pstFrameInfo->stVFrame.u32VirHeight);
    /*RK3588 jpeg编码宽小于68的ARGB8888需使用TDE先把ARGB转为yuv，TDE优先使用rga硬核，如果源图像宽高小于rga的限制68x2时，TDE会调用调用GPU进行格式转换 */
    /*RK3588 TDE拷贝为YUV，使用RGA会将白色背景置为EB，改使用VGS*/
    if ((pstFrameInfo->stVFrame.enPixelFormat != RK_FMT_YUV420SP) && (pstFrameInfo->stVFrame.u32Width < VENC_RK_JPEG_WIDHT_MIN))
    {
        s32Ret = venc_hal_getFrameMem(pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height, &stPicSystemFrame);
        if (SAL_SOK != s32Ret)
        {
            venc_rk_stop(pHandle);
            VENC_LOGE("venc_hal_getFrameMem error !\n");
            return SAL_FAIL;
        }


        pstPicFrameInfo = (VIDEO_FRAME_INFO_S *)stPicSystemFrame.uiAppData;
        s32Ret = venc_rk_copyYuvFrame(pstFrameInfo, pstPicFrameInfo);
        if (SAL_SOK != s32Ret)
        {
            venc_rk_stop(pHandle);
            s32Ret = SAL_FAIL;
            VENC_LOGE("venc_rk_copyYuvFrame error !\n");
            goto exit;
        }

        if (SAL_SOK != venc_rk_sendFrame(pHandle, pstPicFrameInfo))
        {
            VENC_LOGE("venc_hisi_sendFrame error !\n");
            venc_rk_stop(pHandle);
            s32Ret = SAL_FAIL;
            goto exit;
        }
    }
    else
    {
        if (SAL_SOK != venc_rk_sendFrame(pHandle, pstFrameInfo))
        {
            VENC_LOGE("venc_hisi_sendFrame error !\n");
            venc_rk_stop(pHandle);
            s32Ret = SAL_FAIL;
            goto exit;
        }
    }

    if (SAL_SOK != venc_rk_getJpegFrame(pHandle, pJpeg, pJpegSize))
    {
        VENC_LOGE("venc_getJpegFrame error !\n");
        venc_rk_stop(pHandle);
        s32Ret = SAL_FAIL;
        goto exit;
    }

    if (SAL_SOK != venc_rk_stop(pHandle))
    {
        VENC_LOGE("venc_hisi_stop error !\n");
        s32Ret = SAL_FAIL;
        goto exit;
    }

exit:
        if ((pstFrameInfo->stVFrame.enPixelFormat != RK_FMT_YUV420SP) && (pstFrameInfo->stVFrame.u32Width < VENC_RK_JPEG_WIDHT_MIN))
        {
            s32Ret = venc_hal_putFrameMem(&stPicSystemFrame);
            if (SAL_FAIL == s32Ret)
            {
                VENC_LOGE("failed %d\n", s32Ret);
                return SAL_FAIL;
            }
        }

    return s32Ret;

}

/**
 * @function   venc_rk_sendFrm
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_sendFrm(void *pHandle, void *pStr)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SYSTEM_FRAME_INFO *pstSrc = NULL;
    RK_S32 s32Ret = RK_SUCCESS;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstSrc = (SYSTEM_FRAME_INFO *)pStr;

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSrc->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);

#if 0
    VENC_LOGI("frm info:mod %#x pool %#x w %d h %d pixel_format %d v format %d compress %d dynamic %d color_gamut %d time_ref %d pts %d\n",
              pstFrame->mod_id, pstFrame->pool_id,
              pstFrame->video_frame.width, pstFrame->video_frame.height,
              pstFrame->video_frame.pixel_format, pstFrame->video_frame.video_format, pstFrame->video_frame.compress_mode,
              pstFrame->video_frame.dynamic_range, pstFrame->video_frame.color_gamut,
              pstFrame->video_frame.time_ref, pstFrame->video_frame.pts);

    VENC_LOGI("frm info:header_stride %d stride %d header_phys_addr %#x phys_addr %#x header_virt_addr %#x virt_addr %#x\n",
              pstFrame->video_frame.header_stride[0], pstFrame->video_frame.stride[0],
              pstFrame->video_frame.header_phys_addr[0], pstFrame->video_frame.phys_addr[0],
              pstFrame->video_frame.header_virt_addr[0], pstFrame->video_frame.virt_addr[0]);

    VENC_LOGE("frm info: w %d h %d virw %d virh %d Field %d pixelformat %d Videoformat %d CompressMode %d dynamic %d color_gamut %d time_ref %d pts %lu\n",
              pstFrame->stVFrame.u32Width, pstFrame->stVFrame.u32Height,
              pstFrame->stVFrame.u32VirWidth, pstFrame->stVFrame.u32VirHeight,pstFrame->stVFrame.enField,
              pstFrame->stVFrame.enPixelFormat, pstFrame->stVFrame.enVideoFormat, pstFrame->stVFrame.enCompressMode,
              pstFrame->stVFrame.enDynamicRange, pstFrame->stVFrame.enColorGamut,
              pstFrame->stVFrame.u32TimeRef, pstFrame->stVFrame.u64PTS);

    VENC_LOGE("frm info:pMbBlk %p pVirAddr[0] %p pVirAddr[1] %p \n",
              pstFrame->stVFrame.pMbBlk, pstFrame->stVFrame.pVirAddr[0],
              pstFrame->stVFrame.pVirAddr[1]);
#endif


    if (RK_VIDEO_ID_JPEG != pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
         s32Ret = RK_MPI_VENC_SendFrame((VENC_CHN)pstVencHalChnPrm->uiChn, pstFrame, 500);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("RK_mpi_venc_send_frame fail!, s32Ret = %#x time ref %d pts %lu\n", s32Ret, pstFrame->stVFrame.u32TimeRef, pstFrame->stVFrame.u64PTS);
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("enc type err, %#x\n", pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   venc_createJpeg
 * @brief    创建JPEG编码通道
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out]  void *pHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_createJpeg(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32HalChan = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_CHN_ATTR_S *pstVencChnAttr = NULL;
    PIXEL_FORMAT_E enPixelFmt = RK_FMT_YUV420SP;

    if ((NULL == pstInPrm) || (NULL == ppHandle))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_rk_getGroup(&u32HalChan);
    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("get chan fail, Chn %d Create Failed !!!\n", u32HalChan);
        return SAL_FAIL;
    }

    pstVencHalChnPrm = &gstVencRkInfo.stVencHalChnPrm[u32HalChan];
    pstVencHalChnPrm->uiChn = u32HalChan;

    pstVencChnAttr = &pstVencHalChnPrm->stVencChnAttr;
    memset(pstVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
   
    s32Ret    = fmtConvert_rk_sal2rk(pstInPrm->dataFormat, &enPixelFmt);
    
    /* 配置通道属性 */
    pstVencChnAttr->stVencAttr.enType = RK_VIDEO_ID_JPEG;
    pstVencChnAttr->stVencAttr.enPixelFormat = enPixelFmt;
    pstVencChnAttr->stVencAttr.u32PicWidth = pstInPrm->width;
    pstVencChnAttr->stVencAttr.u32PicHeight = pstInPrm->height;
    
    pstVencChnAttr->stVencAttr.u32VirWidth = SAL_align(pstInPrm->width, 16); 
    pstVencChnAttr->stVencAttr.u32VirHeight = SAL_align(pstInPrm->height, 16); 

    pstVencChnAttr->stVencAttr.u32StreamBufCnt = 1;
    /* rk没看到u32BufSize要64字节对齐，但是编码器对数据的访问是 16 对齐的,此处暂时保持不变 */
    /* 需要 64 对齐 */
    pstVencChnAttr->stVencAttr.u32BufSize = pstVencChnAttr->stVencAttr.u32VirWidth * pstVencChnAttr->stVencAttr.u32VirHeight * 4;
    //pstVencChnAttr->stVencAttr.u32BufSize = SAL_align((pstVencChnAttr->stVencAttr.u32VirWidth * pstVencChnAttr->stVencAttr.u32VirHeight * 3 ), 64);
    pstVencChnAttr->stVencAttr.u32Profile = 0;    /* Jpeg/Mjpeg 取值0 */
    pstVencChnAttr->stVencAttr.bByFrame = RK_TRUE;
    pstVencChnAttr->stVencAttr.stAttrJpege.bSupportDCF = RK_FALSE;
    pstVencChnAttr->stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
    pstVencChnAttr->stVencAttr.stAttrJpege.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;

    VENC_LOGI("JPEG Chn enc_%d  W : %d H : %d PixelFmt %d!!!\n", pstVencHalChnPrm->uiChn, pstInPrm->width, pstInPrm->height, enPixelFmt);
    s32Ret = RK_MPI_VENC_CreateChn(pstVencHalChnPrm->uiChn, pstVencChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("Hal Create Jpeg [%d] faild with %#x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        venc_rk_putGroup(u32HalChan);
        return SAL_FAIL;
    }

    /* RK_MPI_VENC_SetMaxStreamCnt(pstVencHalChnPrm->uiChn, 1); */

    pstVencHalChnPrm->uiFd = RK_MPI_VENC_GetFd(pstVencHalChnPrm->uiChn);
    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->packBufLen = SAL_KB * 4;
    pstVencHalChnPrm->pVirtPack = (VENC_PACK_S *)SAL_memMalloc(pstVencHalChnPrm->packBufLen, "jpeg", "rk_enc");
    if (NULL == pstVencHalChnPrm->pVirtPack)
    {
        VENC_LOGE("!!!\n");
        venc_rk_putGroup(u32HalChan);
        return SAL_FAIL;
    }

    *ppHandle = (void *)pstVencHalChnPrm;

    VENC_LOGI("Hal Create JPEG Chn %d Suc !!!\n", pstVencHalChnPrm->uiChn);

    return SAL_SOK;
}

/**
 * @function   venc_creatH26x
 * @brief    创建H26x硬件编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_creatH26x(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    RK_S32 s32Ret = RK_SUCCESS;
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_CHN_ATTR_S *pstVencChnAttr = NULL;
    VENC_STREAM_S *pstStream = NULL;
    SAL_VideoFrameParam *pstHalPrm = NULL;
    VENC_RC_PARAM_S stRcParam = {0};
//    VENC_H264_VUI_S stVui = {0};
    VENC_H264_ENTROPY_S stH264Entropy = {0};
    UINT32 u32Chan = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;
    RK_CODEC_ID_E enEncType = RK_VIDEO_ID_AVC; /*def prm*/
    PIXEL_FORMAT_E enPixelFmt = RK_FMT_YUV420SP;
    CAPB_DUP       *pCapbDup  = capb_get_dup();
    VENC_FPS_INFO_S stVencFpsInfo = {0};


    if ((NULL == pstInPrm) || (ppHandle == NULL))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    pstHalPrm = pstInPrm;
    if (SAL_FAIL == venc_rk_getGroup(&u32Chan))
    {
        VENC_LOGE("HAL Not Venc Chn to Used, Chn %d Create Failed !!!\n", u32Chan);
        return SAL_FAIL;
    }

    pstVencHalChnPrm = &gstVencRkInfo.stVencHalChnPrm[u32Chan];
    pstVencHalChnPrm->uiChn = u32Chan;

    u32Width = pstHalPrm->width;
    u32Height = pstHalPrm->height;
    
    VENC_LOGI("u32Chan %d u32Width %d u32Height %d  !!!\n", u32Chan, u32Width, u32Height);
    
    enEncType = venc_rk_getEncType(pstHalPrm->encodeType);
    s32Ret    = fmtConvert_rk_sal2rk(pCapbDup->enOutputSalPixelFmt, &enPixelFmt);

    pstVencChnAttr = &pstVencHalChnPrm->stVencChnAttr;
    memset(pstVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    
    pstVencChnAttr->stVencAttr.enPixelFormat = enPixelFmt;
    pstVencChnAttr->stVencAttr.enType = enEncType;
    pstVencChnAttr->stVencAttr.u32PicWidth = u32Width;
    pstVencChnAttr->stVencAttr.u32PicHeight = u32Height;
    pstVencChnAttr->stVencAttr.u32VirWidth = SAL_align(u32Width, 16);
    pstVencChnAttr->stVencAttr.u32VirHeight = SAL_align(u32Height, 16);
    
    pstVencChnAttr->stVencAttr.u32StreamBufCnt = 4;
    

    /* rk没看到u32BufSize要64字节对齐，但是编码器对数据的访问是 16 对齐的,此处暂时保持不变 */
    /* rgb格式 */
    pstVencChnAttr->stVencAttr.u32BufSize = (pstVencChnAttr->stVencAttr.u32VirWidth * pstVencChnAttr->stVencAttr.u32VirHeight * 3);
    //pstVencChnAttr->stVencAttr.u32BufSize = SAL_align((pstVencChnAttr->stVencAttr.u32VirWidth * pstVencChnAttr->stVencAttr.u32VirHeight * 3 / 2), 64);
    if (RK_VIDEO_ID_AVC == pstVencChnAttr->stVencAttr.enType)
    {
        pstVencChnAttr->stVencAttr.u32Profile = H264E_PROFILE_BASELINE;
    }
    else if(RK_VIDEO_ID_HEVC == pstVencChnAttr->stVencAttr.enType)
    {
        pstVencChnAttr->stVencAttr.u32Profile = H265E_PROFILE_MAIN;
    }
        
    pstVencChnAttr->stVencAttr.bByFrame = RK_TRUE;

    u32ViFps = (pstHalPrm->fps >> 16); /* 高16位源fps，低16位编码fps */
    u32EncFps = (pstHalPrm->fps & 0xffff);
    VENC_LOGI("src rate %d dst rate %d\n", u32ViFps, u32EncFps);

/* 安检机项目,编码模块不做帧率控制,放在vpss做帧率控制 */
#ifdef DSP_ISM
    stVencFpsInfo.u32SrcFrameRateNum  = 0;
    stVencFpsInfo.u32SrcFrameRateDen  = 0;
    stVencFpsInfo.fr32DstFrameRateNum = 0;
    stVencFpsInfo.fr32DstFrameRateDen = 0;
    stVencFpsInfo.u32Gop = 60;
#else
    stVencFpsInfo.u32SrcFrameRateNum  = (0 == u32ViFps) ? 60 : u32ViFps;
    stVencFpsInfo.u32SrcFrameRateDen  = 1;
    stVencFpsInfo.fr32DstFrameRateNum = u32EncFps > 0 ? u32EncFps : 30;
    stVencFpsInfo.fr32DstFrameRateDen = 1;
    stVencFpsInfo.u32Gop = u32EncFps * 2;
#endif

    if (0 == pstHalPrm->bpsType)
    {
        /* 默认使用变码率 */
        if (RK_VIDEO_ID_AVC == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
            /* u32BitRate海思为最大码率,rk为平均码率 */
            pstVencChnAttr->stRcAttr.stH264Vbr.u32BitRate = pstHalPrm->bps;

            pstVencChnAttr->stRcAttr.stH264Vbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
            pstVencChnAttr->stRcAttr.stH264Vbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
            pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
            pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
            pstVencChnAttr->stRcAttr.stH264Vbr.u32Gop = stVencFpsInfo.u32Gop;
            
            VENC_LOGW("create H264 VBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d, RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH264Vbr.u32SrcFrameRateNum,
                      pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRateNum, pstHalPrm->bps,pstVencChnAttr->stRcAttr.enRcMode);
        }
        else if (RK_VIDEO_ID_HEVC == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
            pstVencChnAttr->stRcAttr.stH265Vbr.u32BitRate = pstHalPrm->bps;

            pstVencChnAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
            pstVencChnAttr->stRcAttr.stH265Vbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
            pstVencChnAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
            pstVencChnAttr->stRcAttr.stH265Vbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
            pstVencChnAttr->stRcAttr.stH265Vbr.u32Gop = stVencFpsInfo.u32Gop;
            
            VENC_LOGW("create H265 VBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d, RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH265Vbr.u32SrcFrameRateNum,
                      pstVencChnAttr->stRcAttr.stH265Vbr.fr32DstFrameRateNum, pstHalPrm->bps,pstVencChnAttr->stRcAttr.enRcMode);
        }
        else
        {
            VENC_LOGE(" encType (%d) error only support h264/h265....\n", pstHalPrm->encodeType);
            goto err;
        }
    }
    else
    {
        /* 默认使用定码率 */
        if (RK_VIDEO_ID_AVC == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32BitRate = pstHalPrm->bps;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
            pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
            pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32Gop = stVencFpsInfo.u32Gop;

            VENC_LOGW("create H264 CBR chn %d w is %d, h is %d,Srcfps is %d Dstfps is %d, bps is %d,RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH264Cbr.u32SrcFrameRateNum,
                      pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRateNum, pstHalPrm->bps, pstVencChnAttr->stRcAttr.enRcMode);
        }
        else if (RK_VIDEO_ID_HEVC == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32BitRate = pstHalPrm->bps;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32SrcFrameRateNum = stVencFpsInfo.u32SrcFrameRateNum;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32SrcFrameRateDen = stVencFpsInfo.u32SrcFrameRateDen;
            pstVencChnAttr->stRcAttr.stH265Cbr.fr32DstFrameRateNum = stVencFpsInfo.fr32DstFrameRateNum;
            pstVencChnAttr->stRcAttr.stH265Cbr.fr32DstFrameRateDen = stVencFpsInfo.fr32DstFrameRateDen;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32Gop = stVencFpsInfo.u32Gop;

            VENC_LOGW("create H265 CBR chn %d w is %d, h is %d, Srcfps is %d Dstfps is %d, bps is %d, RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH265Cbr.u32SrcFrameRateNum,
                      pstVencChnAttr->stRcAttr.stH265Cbr.fr32DstFrameRateNum, pstHalPrm->bps,pstVencChnAttr->stRcAttr.enRcMode);
        }
        else
        {
            VENC_LOGE(" encType %d(%d) error only support h264/h265....\n", enEncType, pstHalPrm->encodeType);
            goto err;
        }
    }

    pstVencChnAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pstVencChnAttr->stGopAttr.s32VirIdrLen = 0;

    s32Ret = RK_MPI_VENC_CreateChn(pstVencHalChnPrm->uiChn, pstVencChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC Module %d chan %d Create faild with %#x! ===\n", u32Chan, pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }
    
    if (SAL_SOK != venc_rk_setH26xScale(pstVencHalChnPrm,u32Width,u32Height))
    {
        VENC_LOGE("venc_setJpegCropPrm error !\n");
        return SAL_FAIL;
    }
    
    SAL_clear(&stH264Entropy);
    
    if (RK_VIDEO_ID_AVC == enEncType)
    {
        s32Ret = RK_MPI_VENC_GetH264Entropy(pstVencHalChnPrm->uiChn,  &stH264Entropy);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("RK_MPI_VENC_GetH264Entropy [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }
        
        stH264Entropy.u32EntropyEncMode = 0;

        s32Ret = RK_MPI_VENC_SetH264Entropy(pstVencHalChnPrm->uiChn,  &stH264Entropy);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("RK_MPI_VENC_SetH264Entropy [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }
    }
    
    SAL_clear(&stRcParam);
    s32Ret = RK_MPI_VENC_GetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_GetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    if (0 == pstHalPrm->bpsType)
    {
        if (RK_VIDEO_ID_AVC == enEncType)
        {
            stRcParam.s32FirstFrameStartQp = 25;
            stRcParam.stParamH264.u32StepQp = 4;
            stRcParam.stParamH264.u32MinQp = 10;
            stRcParam.stParamH264.u32MaxQp = 50;
            stRcParam.stParamH264.u32MinIQp = 10;
            stRcParam.stParamH264.u32MaxIQp = 50;
            stRcParam.stParamH264.s32DeltIpQp = -2;
            stRcParam.stParamH264.s32MaxReEncodeTimes = 2;

            VENC_LOGI("vbr StepQp [%d],pQp[%d,%d] IQp[%d,%d],DeltIpQp %d, ReEncodeTimes %d] !!!\n", stRcParam.stParamH264.u32StepQp, stRcParam.stParamH264.u32MinQp, stRcParam.stParamH264.u32MaxQp,
                      stRcParam.stParamH264.u32MinIQp, stRcParam.stParamH264.u32MaxIQp, stRcParam.stParamH264.s32DeltIpQp, stRcParam.stParamH264.s32MaxReEncodeTimes);
        }
        else if (RK_VIDEO_ID_HEVC == enEncType)
        {
            stRcParam.stParamH265.u32MinQp = 10;
            stRcParam.stParamH265.u32MinIQp = 10;
            stRcParam.stParamH265.u32MaxQp = 40;
        }
        else
        {
            VENC_LOGE(" encType %d(%d) error only support h264/h265....\n", enEncType, pstHalPrm->encodeType);
            goto err;
        }
    }

    s32Ret = RK_MPI_VENC_SetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (RK_SUCCESS != s32Ret)
    {
        VENC_LOGE("RK_MPI_VENC_SetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    if (RK_VIDEO_ID_AVC == enEncType)
    {
        
/* rk暂不支持stVuiTimeInfo参数设置, 暂时注释 */
#if 0
        s32Ret = RK_MPI_VENC_GetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("RK_MPI_VENC_GetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }

        /* 这里设置时间戳的单位 60K时钟  80K时钟  90K时钟，具体值需要和海思确认过 */
        stVui.stVuiTimeInfo.timing_info_present_flag = 1;
        stVui.stVuiTimeInfo.time_scale = 60000;
        stVui.stVuiTimeInfo.num_units_in_tick = 30000 / (pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRateNum);
        
        //默认为off 16-255,on 表示0-255.色域范围更大
        stVui.stVuiVideoSignal.video_full_range_flag = 0;
        s32Ret = RK_MPI_VENC_SetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("RK_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }
#endif
    }

    pstVencHalChnPrm->uiFd = RK_MPI_VENC_GetFd(pstVencHalChnPrm->uiChn);

#ifdef VENC_HAL_SAVE_BITS_DATA

    char aszFileName[64];
    snprintf(aszFileName, 64, "./DSP_TEST_RES/Stream/ES/stream_chn%d.es.h264", u32Chan);

    file[uiChn] = fopen(aszFileName, "wb");

    if (RK_NULL == file[u32Chan])
    {
        VENC_LOGE("open file failed:%s!\n", strerror(errno));
    }

#endif

    /* 申请底层编码器所需内存 */
    pstStream = &pstVencHalChnPrm->stStream;
//    pstStream->pstPack = (VENC_PACK_S *)SAL_memMalloc(sizeof(VENC_PACK_S) * FRAME_MAX_NALU_NUM, "venc", "rk_enc");
    pstStream->pstPack = (VENC_PACK_S *)SAL_memMalloc(sizeof(VENC_PACK_S), "venc", "rk_enc");

    if (NULL == pstStream->pstPack)
    {
        VENC_LOGE("malloc stream pack failed!\n");
        goto err;
    }

    *ppHandle = pstVencHalChnPrm;

    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->isCreate = SAL_TRUE;
    VENC_LOGI("VENC Chn %d HalChn %d Create success.\n", u32Chan, pstVencHalChnPrm->uiChn);
    return SAL_SOK;

err:
    venc_rk_putGroup(u32Chan);
    return SAL_FAIL;
}

/**
 * @function   venc_hisi_create
 * @brief    创建视频硬件编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if ((NULL == pstInPrm) || (ppHandle == NULL))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    if (RK_VIDEO_ID_JPEG == venc_rk_getEncType(pstInPrm->encodeType))
    {
        s32Ret = venc_rk_createJpeg(ppHandle, pstInPrm);
    }
    else
    {
        s32Ret = venc_rk_creatH26x(ppHandle, pstInPrm);
    }

    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_rk_init
 * @brief      初始化通道管理锁
 * @param[in]  None
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_rk_init(void)
{
    /* 初始化通道管理锁 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &gstVencRkInfo.MutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] VENC_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_register(VENC_PLAT_OPS_S *pstVencPlatOps)
{
    if (NULL == pstVencPlatOps)
    {
        return SAL_FAIL;
    }

    /* 注册hal层处理函数 */
    pstVencPlatOps->Init = venc_rk_init;
    pstVencPlatOps->Create = venc_rk_create;
    pstVencPlatOps->Delete = venc_rk_delete;
    pstVencPlatOps->Start = venc_rk_start;
    pstVencPlatOps->Stop = venc_rk_stop;
    pstVencPlatOps->SetParam = venc_rk_setEncPrm;
    pstVencPlatOps->ForceIFrame = venc_rk_requestIDR;
    pstVencPlatOps->GetFrame = venc_rk_getFrame;
    pstVencPlatOps->PutFrame = venc_rk_putFrame;
    pstVencPlatOps->GetHalChn = venc_rk_getChan;
    pstVencPlatOps->SetVencStatues = venc_rk_setStatus;
    pstVencPlatOps->EncJpegProcess = venc_rk_encJpeg;
    pstVencPlatOps->EncSendFrm = venc_rk_sendFrm;

    return SAL_SOK;
}


