/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_hisi.c
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#include "vdec_hisi.h"
#include "sal_mem_new.h"
#include "vdec_hal_inter.h"
#include <platform_sdk.h>


/*解码器属性，平台差异化部分*/
typedef struct _VDEC_HISI_SPEC_
{
    VB_SOURCE_E vbModule;
    VIDEO_MODE_E enMode;                    /*解码模式，帧模式，流模式，兼容模式*/
    VIDEO_DEC_MODE_E enDecMode;             /*0:IPB  1:IP 2:I*/
    DATA_BITWIDTH_E enBitWidth;             /*数据位*/
    PIXEL_FORMAT_E enPixelFormat;           /*数据格式*/
    DYNAMIC_RANGE_E enDynamicRange;             /* RW; DynamicRange of target image. */
    VIDEO_DISPLAY_MODE_E enDisplayMode;     /* 回放显示模式*/
    UINT32 displayFrameNum;                 /*显示帧数*/
    UINT32 refFrameNum;                     /**/
    UINT32 frameBufCnt;                     /**/
    UINT32 u32PicBufSzie;
    UINT32 u32TmvBufSzie;
    VDEC_CHN_POOL_S stPool;
} VDEC_HISI_SPEC;

typedef struct _VDEC_HAL_PLT_HANDLE_
{
    INT32 width;                                              /*解码器宽*/
    INT32 height;                                             /*解码器高*/
    INT32 encType;                                            /*解码数据编码格式*/
    INT32 decChan;                                            /*对应实际的解码器通道号*/
    INT32 vdecOutStatue;                                      /* 解码器输出创建1，解码器输出未创建0, 2 正在使用 */
    VDEC_HISI_SPEC vdecHisiHdl;
} VDEC_HAL_PLT_HANDLE;

static VDEC_HAL_FUNC vdecFunc;

/******************************************************************
   Function:   vdec_hisi_MemAlloc
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
VOID *vdec_hisi_MemAlloc(UINT32 u32Format, void *prm)
{
    return NULL;
}

/******************************************************************
   Function:   vdec_hisi_MemFree
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 vdec_hisi_MemFree(void *ptr, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    return s32Ret;
}

/**
 * @function   vdec_hisi_ChangePixFormat
 * @brief	  映射平台相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_hisi_ChangePixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            format = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
            break;
        }

        default:
        {
            VDEC_LOGE("err format %d pixFormat %d \n", format, pixFormat);
            return -1;
        }
    }

    return format;
}

/**
 * @function   vdec_hisi_TransPixFormat
 * @brief	  映射业务相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_hisi_TransPixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case PIXEL_FORMAT_YVU_SEMIPLANAR_420:
        {
            format = SAL_VIDEO_DATFMT_YUV420SP_VU;
            break;
        }

        default:
        {
            VDEC_LOGE("err format %d pixFormat %d\n", format, pixFormat);
            return -1;
        }
    }

    return format;
}

/**
 * @function   vdec_hisi_TransEncType
 * @brief	  映射业务相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_TransEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case PT_H264:
        {
            type = H264;
            break;
        }
        case PT_H265:
        {
            type = H265;
            break;
        }
        case PT_JPEG:
        {
            type = MJPEG;
            break;
        }
        default:
        {
            VDEC_LOGE("err type %d encType %d\n", type, encType);
            return -1;
        }
    }

    return type;
}

/**
 * @function   vdec_hisi_TransEncType
 * @brief	  映射平台相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_hisi_ChangeEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case H264:
        {
            type = PT_H264;
            break;
        }
        case H265:
        {
            type = PT_H265;
            break;
        }
        case MJPEG:
        {
            type = PT_JPEG;
            break;
        }
        default:
        {
            VDEC_LOGE("err type %d encType %d\n", type, encType);
            return -1;
        }
    }

    return type;
}

/**
 * @function   vdec_hisi_SetModParam
 * @brief	  设置模式
 * @param[in]  VB_SOURCE_E vbModule 模式
 * @param[out] None
 * @return	  INT32
 */
static INT32 vdec_hisi_SetModParam(VB_SOURCE_E vbModule)
{
    INT32 s32Ret = HI_SUCCESS;
    VDEC_MOD_PARAM_S stModParam;

    memset(&stModParam, 0x00, sizeof(VDEC_MOD_PARAM_S));

    s32Ret = HI_MPI_VDEC_GetModParam(&stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error.\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (stModParam.enVdecVBSource != vbModule)
    {
        stModParam.enVdecVBSource = vbModule;

        s32Ret = HI_MPI_VDEC_SetModParam(&stModParam);
        if (HI_SUCCESS != s32Ret)
        {
            VDEC_LOGE("error.\n");
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_DeleteVBPool
 * @brief	  删除vb内存
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_DeleteVBPool(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;

    if (pVdecHalHdl->vdecHisiHdl.vbModule == VB_SOURCE_USER)
    {
        if (VB_INVALID_POOLID != pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool)
        {
            s32Ret = HI_MPI_VB_DestroyPool(pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool);
            if (HI_SUCCESS != s32Ret)
            {
                VDEC_LOGE("vb destory pool failed s32Ret 0x%x\n", s32Ret);
                return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
            }

            pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool = VB_INVALID_POOLID;
        }

        if (VB_INVALID_POOLID != pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool)
        {
            s32Ret = HI_MPI_VB_DestroyPool(pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool);
            if (HI_SUCCESS != s32Ret)
            {
                VDEC_LOGE("DestroyPool s32Ret 0x%x\n", s32Ret);
                return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
            }

            pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool = VB_INVALID_POOLID;
        }
    }
    else
    {
        s32Ret = HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
        if (HI_SUCCESS != s32Ret)
        {
            VDEC_LOGE("ExitModCommPool failed s32Ret 0x%x\n", s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_InitVbPool
 * @brief	  初始化vb内存
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_InitVbPool(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;
    VB_POOL_CONFIG_S stVbPoolCfg;
    VB_CONFIG_S stVbConf;

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    if (PT_H265 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
    {
        pVdecHalHdl->vdecHisiHdl.u32PicBufSzie = VDEC_GetPicBufferSize(vdec_hisi_ChangeEncType(pVdecHalHdl->encType), pVdecHalHdl->width, pVdecHalHdl->height,
                                                                       PIXEL_FORMAT_YVU_SEMIPLANAR_420, pVdecHalHdl->vdecHisiHdl.enBitWidth, 0);
        pVdecHalHdl->vdecHisiHdl.u32TmvBufSzie = VDEC_GetTmvBufferSize(vdec_hisi_ChangeEncType(pVdecHalHdl->encType), pVdecHalHdl->width, pVdecHalHdl->height);
    }
    else if (PT_H264 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
    {
        pVdecHalHdl->vdecHisiHdl.u32PicBufSzie = VDEC_GetPicBufferSize(vdec_hisi_ChangeEncType(pVdecHalHdl->encType), pVdecHalHdl->width, pVdecHalHdl->height,
                                                                       PIXEL_FORMAT_YVU_SEMIPLANAR_420, pVdecHalHdl->vdecHisiHdl.enBitWidth, 0);
        if (VIDEO_DEC_MODE_IPB == pVdecHalHdl->vdecHisiHdl.enDecMode)
        {
            pVdecHalHdl->vdecHisiHdl.u32TmvBufSzie = VDEC_GetTmvBufferSize(vdec_hisi_ChangeEncType(pVdecHalHdl->encType), pVdecHalHdl->width, pVdecHalHdl->height);
        }
    }
    else
    {
        pVdecHalHdl->vdecHisiHdl.u32PicBufSzie = VDEC_GetPicBufferSize(vdec_hisi_ChangeEncType(pVdecHalHdl->encType), pVdecHalHdl->width, pVdecHalHdl->height,
                                                                       pVdecHalHdl->vdecHisiHdl.enPixelFormat, DATA_BITWIDTH_8, 0);
    }

    if (pVdecHalHdl->vdecHisiHdl.vbModule == VB_SOURCE_USER)
    {

        pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool = VB_INVALID_POOLID;
        pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool = VB_INVALID_POOLID;

        if ((0 != pVdecHalHdl->vdecHisiHdl.u32PicBufSzie) && (0 != pVdecHalHdl->vdecHisiHdl.frameBufCnt))
        {
            memset(&stVbPoolCfg, 0x00, sizeof(VB_POOL_CONFIG_S));
            stVbPoolCfg.u64BlkSize = pVdecHalHdl->vdecHisiHdl.u32PicBufSzie;
            stVbPoolCfg.u32BlkCnt = pVdecHalHdl->vdecHisiHdl.frameBufCnt;
            stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
            pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool = HI_MPI_VB_CreatePool(&stVbPoolCfg);
            if (VB_INVALID_POOLID == pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool)
            {
                VDEC_LOGE("uiChn %d \n", pVdecHalHdl->decChan);
                return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
            }
        }

        if (0 != pVdecHalHdl->vdecHisiHdl.u32TmvBufSzie)
        {
            memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
            stVbPoolCfg.u64BlkSize = pVdecHalHdl->vdecHisiHdl.u32TmvBufSzie;
            stVbPoolCfg.u32BlkCnt = pVdecHalHdl->vdecHisiHdl.refFrameNum + 1;
            stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
            pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool = HI_MPI_VB_CreatePool(&stVbPoolCfg);
            if (VB_INVALID_POOLID == pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool)
            {
                VDEC_LOGE("uiChn %d \n", pVdecHalHdl->decChan);
                return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
            }
        }
    }
    else
    {
        memset(&stVbConf, 0x00, sizeof(VB_CONFIG_S));
        if (pVdecHalHdl->vdecHisiHdl.u32PicBufSzie != 0)
        {
            stVbConf.astCommPool[0].u64BlkSize = pVdecHalHdl->vdecHisiHdl.u32PicBufSzie;
            stVbConf.astCommPool[0].u32BlkCnt = (pVdecHalHdl->decChan + 1) * pVdecHalHdl->vdecHisiHdl.frameBufCnt;
        }

        if (pVdecHalHdl->vdecHisiHdl.u32TmvBufSzie != 0)
        {
            stVbConf.astCommPool[1].u64BlkSize = pVdecHalHdl->vdecHisiHdl.u32TmvBufSzie;
            stVbConf.astCommPool[1].u32BlkCnt = (pVdecHalHdl->decChan + 1) * (pVdecHalHdl->vdecHisiHdl.refFrameNum + 1);
        }

        stVbConf.u32MaxPoolCnt = 2;

        s32Ret = HI_MPI_VB_SetModPoolConfig(VB_UID_VDEC, &stVbConf);
        if (HI_SUCCESS != s32Ret)
        {
            VDEC_LOGE("uiChn %d s32Ret 0x%x \n", pVdecHalHdl->decChan, s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        s32Ret = HI_MPI_VB_InitModCommPool(VB_UID_VDEC);
        if (HI_SUCCESS != s32Ret)
        {
            VDEC_LOGE("uiChn %d s32Ret 0x%x \n", pVdecHalHdl->decChan, s32Ret);
            HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }
    }

    return s32Ret;
}

/**
 * @function   vdec_hisi_ChnCreate
 * @brief	  解码通道创建
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_ChnCreate(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = 0;
    VDEC_CHN_ATTR_S stChnAttr;
    VDEC_CHN_POOL_S stPool;
    VDEC_CHN_PARAM_S stChnParam;

    memset(&stChnAttr, 0x00, sizeof(VDEC_CHN_ATTR_S));
    memset(&stPool, 0x00, sizeof(VDEC_CHN_POOL_S));
    memset(&stChnParam, 0x00, sizeof(VDEC_CHN_PARAM_S));

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    stChnAttr.enType = vdec_hisi_ChangeEncType(pVdecHalHdl->encType);
    stChnAttr.enMode = pVdecHalHdl->vdecHisiHdl.enMode;
    stChnAttr.u32PicWidth = pVdecHalHdl->width;
    stChnAttr.u32PicHeight = pVdecHalHdl->height;

    stChnAttr.u32StreamBufSize = pVdecHalHdl->width * pVdecHalHdl->height * 3 / 2;
    stChnAttr.u32FrameBufCnt = pVdecHalHdl->vdecHisiHdl.frameBufCnt;

    if (PT_H264 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType) || PT_H265 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnAttr.stVdecVideoAttr.u32RefFrameNum = pVdecHalHdl->vdecHisiHdl.refFrameNum;
        stChnAttr.stVdecVideoAttr.bTemporalMvpEnable = 1;
        if ((PT_H264 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType)) && (VIDEO_DEC_MODE_IPB != pVdecHalHdl->vdecHisiHdl.enDecMode))
        {
            stChnAttr.stVdecVideoAttr.bTemporalMvpEnable = 0;
        }

        stChnAttr.u32FrameBufSize = VDEC_GetPicBufferSize(stChnAttr.enType, pVdecHalHdl->width, pVdecHalHdl->height,
                                                          PIXEL_FORMAT_YVU_SEMIPLANAR_420, pVdecHalHdl->vdecHisiHdl.enBitWidth, 0);
    }
    else if (PT_JPEG == vdec_hisi_ChangeEncType(pVdecHalHdl->encType) || PT_MJPEG == vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnAttr.enMode = VIDEO_MODE_FRAME;
        stChnAttr.u32FrameBufSize = VDEC_GetPicBufferSize(stChnAttr.enType, pVdecHalHdl->width, pVdecHalHdl->height, pVdecHalHdl->vdecHisiHdl.enPixelFormat, DATA_BITWIDTH_8, 0);
    }

    VDEC_LOGI("Create Vdec chn %d enType %d enMode %d	%d x %d \n", pVdecHalHdl->decChan, stChnAttr.enType, stChnAttr.enMode, stChnAttr.u32PicWidth, stChnAttr.u32PicHeight);

    s32Ret = HI_MPI_VDEC_CreateChn(pVdecHalHdl->decChan, &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    if (VB_SOURCE_USER == pVdecHalHdl->vdecHisiHdl.vbModule)
    {
        stPool.hPicVbPool = pVdecHalHdl->vdecHisiHdl.stPool.hPicVbPool;
        stPool.hTmvVbPool = pVdecHalHdl->vdecHisiHdl.stPool.hTmvVbPool;
        s32Ret = HI_MPI_VDEC_AttachVbPool(pVdecHalHdl->decChan, &stPool);
        if (HI_SUCCESS != s32Ret)
        {
            VDEC_LOGE("error 0x%x \n", s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }

    s32Ret = HI_MPI_VDEC_GetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (PT_H264 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType) || PT_H265 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnParam.stVdecVideoParam.enDecMode = pVdecHalHdl->vdecHisiHdl.enDecMode;
        stChnParam.stVdecVideoParam.enCompressMode = COMPRESS_MODE_NONE;
        stChnParam.stVdecVideoParam.enVideoFormat = VIDEO_FORMAT_TILE_64x16;
        if (VIDEO_DEC_MODE_IPB == stChnParam.stVdecVideoParam.enDecMode)
        {
            stChnParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DISP;
        }
        else
        {
            stChnParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DEC;
        }
    }
    else
    {
        stChnParam.stVdecPictureParam.enPixelFormat = pVdecHalHdl->vdecHisiHdl.enPixelFormat;
        stChnParam.stVdecPictureParam.u32Alpha = 255;
    }

    stChnParam.u32DisplayFrameNum = pVdecHalHdl->vdecHisiHdl.displayFrameNum;

    s32Ret = HI_MPI_VDEC_SetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = HI_MPI_VDEC_SetDisplayMode(pVdecHalHdl->decChan, pVdecHalHdl->vdecHisiHdl.enDisplayMode);

    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecCreate
 * @brief	  解码器创建
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecCreate(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;
    INT32 cnt = 10;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)SAL_memMalloc(sizeof(VDEC_HAL_PLT_HANDLE), "VDEC", "mallocOs");
    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    sal_memset_s(pVdecHalHdl, sizeof(VDEC_HAL_PLT_HANDLE), 0x00, sizeof(VDEC_HAL_PLT_HANDLE));
    pVdecHalHdl->width = pVdecHalPrm->vdecPrm.decWidth;
    pVdecHalHdl->height = pVdecHalPrm->vdecPrm.decHeight;
    pVdecHalHdl->encType = pVdecHalPrm->vdecPrm.encType;

    pVdecHalHdl->decChan = u32VdecChn;
    pVdecHalHdl->vdecHisiHdl.vbModule = VB_SOURCE_USER;
    pVdecHalHdl->vdecHisiHdl.enDynamicRange = DYNAMIC_RANGE_SDR8;
    pVdecHalHdl->vdecHisiHdl.enMode = VIDEO_MODE_FRAME;
    pVdecHalHdl->vdecHisiHdl.enDecMode = VIDEO_DEC_MODE_IP;
    pVdecHalHdl->vdecHisiHdl.enBitWidth = DATA_BITWIDTH_8;
    pVdecHalHdl->vdecHisiHdl.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pVdecHalHdl->vdecHisiHdl.enDisplayMode = VIDEO_DISPLAY_MODE_PREVIEW;
    pVdecHalHdl->vdecHisiHdl.displayFrameNum = 5;
    pVdecHalHdl->vdecHisiHdl.refFrameNum = 3;
    pVdecHalHdl->vdecHisiHdl.frameBufCnt = pVdecHalHdl->vdecHisiHdl.displayFrameNum
                                           + pVdecHalHdl->vdecHisiHdl.refFrameNum + 1;
    vdec_hisi_SetModParam(pVdecHalHdl->vdecHisiHdl.vbModule);

    s32Ret = vdec_hisi_InitVbPool(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    while (2 == pVdecHalHdl->vdecOutStatue)
    {
        usleep(100 * 1000);
        if (cnt <= 0)
        {
            VDEC_LOGW("vdecChn %d out(vpss) is use\n", u32VdecChn);
            break;
        }

        cnt--;
    }

    pVdecHalHdl->vdecOutStatue = 0;

    s32Ret = vdec_hisi_ChnCreate(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_CONFIG);
    }

    pVdecHalHdl->vdecOutStatue = 1;
    pVdecHalHdl->decChan = u32VdecChn;
    pVdecHalPrm->decPlatHandle = (void *)pVdecHalHdl;

    VDEC_LOGI("chn %d w %d h %d type %d\n", u32VdecChn, pVdecHalHdl->width, pVdecHalHdl->height, pVdecHalHdl->encType);
    return s32Ret;
}

/**
 * @function   vdec_hisi_VdecStart
 * @brief	   启动解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecStart(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = HI_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_ENABLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecDeinit(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = HI_MPI_VDEC_DestroyChn(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = vdec_hisi_DeleteVBPool(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    SAL_memfree((Ptr)pVdecHalHdl, "VDEC", "mallocOs");
    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 解码参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecSetPrm(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;

    VDEC_CHN_ATTR_S stChnAttr;
    VDEC_CHN_POOL_S stPool;
    VDEC_CHN_PARAM_S stChnParam;

    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    memset(&stChnAttr, 0x00, sizeof(VDEC_CHN_ATTR_S));
    memset(&stPool, 0x00, sizeof(VDEC_CHN_POOL_S));
    memset(&stChnParam, 0x00, sizeof(VDEC_CHN_PARAM_S));

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)pVdecHalPrm->decPlatHandle;

    s32Ret = HI_MPI_VDEC_GetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    if (PT_H264 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType) || PT_H265 == vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnParam.enType = vdec_hisi_ChangeEncType(pVdecHalHdl->encType);
        stChnParam.stVdecVideoParam.enDecMode = pVdecHalHdl->vdecHisiHdl.enDecMode;
        stChnParam.stVdecVideoParam.enCompressMode = COMPRESS_MODE_NONE;
        stChnParam.stVdecVideoParam.enVideoFormat = VIDEO_FORMAT_TILE_64x16;
        if (VIDEO_DEC_MODE_IPB == stChnParam.stVdecVideoParam.enDecMode)
        {
            stChnParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DISP;
        }
        else
        {
            stChnParam.stVdecVideoParam.enOutputOrder = VIDEO_OUTPUT_ORDER_DEC;
        }
    }
    else
    {
        stChnParam.stVdecPictureParam.enPixelFormat = pVdecHalHdl->vdecHisiHdl.enPixelFormat;
        stChnParam.stVdecPictureParam.u32Alpha = 255;
    }

    stChnParam.u32DisplayFrameNum = pVdecHalHdl->vdecHisiHdl.displayFrameNum;

    s32Ret = HI_MPI_VDEC_SetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VDEC_StopRecvStream(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VDEC_GetChnAttr(pVdecHalHdl->decChan, &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stChnAttr.enType = vdec_hisi_ChangeEncType(pVdecHalHdl->encType);
    stChnAttr.enMode = pVdecHalHdl->vdecHisiHdl.enMode;

    VDEC_LOGI("set attr Vdec chn %d enType %d enMode %d   %d x %d \n", pVdecHalHdl->decChan, stChnAttr.enType, stChnAttr.enMode, stChnAttr.u32PicWidth, stChnAttr.u32PicHeight);

    s32Ret = HI_MPI_VDEC_SetChnAttr(pVdecHalHdl->decChan, &stChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    VDEC_LOGI("decChan %d ok\n", pVdecHalHdl->decChan);

    return s32Ret;
}

/**
 * @function   vdec_hisi_VdecClear
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecClear(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = HI_MPI_VDEC_StopRecvStream(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    /* reset vdec chn*/
    s32Ret = HI_MPI_VDEC_ResetChn(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = HI_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_ENABLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecStop
 * @brief	   停止解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecStop(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = HI_MPI_VDEC_StopRecvStream(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    /* reset vdec chn*/
    s32Ret = HI_MPI_VDEC_ResetChn(pVdecHalHdl->decChan);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_UpdatVdecChn
 * @brief	   更新解码器
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_UpdatVdecChn(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;
    INT32 cnt = 10;

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    while (2 == pVdecHalHdl->vdecOutStatue)
    {
        usleep(100 * 1000);
        if (cnt <= 0)
        {
            VDEC_LOGW("vdecChn  is use\n");
            break;
        }

        cnt--;
    }

    pVdecHalHdl->vdecOutStatue = 0;

    s32Ret = vdec_hisi_VdecStop(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_hisi_VdecDeinit(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_hisi_InitVbPool(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    s32Ret = vdec_hisi_ChnCreate(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    pVdecHalHdl->vdecOutStatue = 1;

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecDecframe
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[in]  SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in]  SAL_VideoFrameBuf *pFrameOut 输出数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecDecframe(void *handle, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut)
{
    INT32 s32Ret = SAL_SOK;
    HI_S32 s32MilliSec = 1000;
    VDEC_STREAM_S stStream = {0};
    VDEC_CHN_STATUS_S stStatus = {0};
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameIn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameOut, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    if (pFrameIn->encodeType != MJPEG)
    {
        if (pVdecHalHdl->width <= MIN_VDEC_WIDTH || pVdecHalHdl->height <= MIN_VDEC_HEIGHT
            || pVdecHalHdl->width > MAX_PPM_VDEC_WIDTH || pVdecHalHdl->height > MAX_VDEC_HEIGHT)
        {
            VDEC_LOGE("vdec chn %d width %d height %d is error\n", pVdecHalHdl->decChan, pVdecHalHdl->width, pVdecHalHdl->height);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        if (pFrameIn->frameParam.width != pVdecHalHdl->width || pFrameIn->frameParam.height != pVdecHalHdl->height
            || vdec_hisi_ChangeEncType(pFrameIn->frameParam.encodeType) != vdec_hisi_ChangeEncType(pVdecHalHdl->encType))
        {
            VDEC_LOGE("vdec chn %d width %d height %d type %d is error dec[w %d h %d type %d]\n", pVdecHalHdl->decChan, pFrameIn->frameParam.width, pFrameIn->frameParam.height,
                      pFrameIn->frameParam.encodeType, pVdecHalHdl->width, pVdecHalHdl->height, pVdecHalHdl->encType);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        s32Ret = HI_MPI_VDEC_QueryStatus(pVdecHalHdl->decChan, &stStatus);
        if (HI_SUCCESS != s32Ret)
        {
            VDEC_LOGW("will reset vdec\n");
            s32Ret = vdec_hisi_VdecClear(pVdecHalHdl);
            if (SAL_SOK != s32Ret)
            {
                VDEC_LOGE("error %#x\n", s32Ret);
                return s32Ret;
            }
        }
    }

    stStream.u64PTS = 0;
    stStream.pu8Addr = (UINT8 *)pFrameIn->virAddr[0];
    stStream.u32Len = pFrameIn->bufLen;
    stStream.bEndOfFrame = HI_TRUE;
    stStream.bEndOfStream = 0;
    stStream.bDisplay = 1;

    s32Ret = HI_MPI_VDEC_SendStream(pVdecHalHdl->decChan, &stStream, s32MilliSec);
    if (HI_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error %#x\n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return s32Ret;
}

/**
 * @function   TdeQuickCopyYuv
 * @brief	   数据帧tde拷贝
 * @param[in]  VIDEO_FRAME_INFO_S *pstDstframe 源数据
 * @param[in]  VIDEO_FRAME_INFO_S *pstSrcframe 目的
 * @param[out] None
 * @return	  INT32
 */
static INT32 TdeQuickCopyYuv(VIDEO_FRAME_INFO_S *pstDstframe, VIDEO_FRAME_INFO_S *pstSrcframe)
{
    INT32 s32Ret = 0;
    TDE_HANDLE s32Handle;
    TDE2_SURFACE_S srcSurface, dstSurface;
    TDE2_RECT_S srcRect, dstRect;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        VDEC_LOGE("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    memset(&srcSurface, 0, sizeof(TDE2_SURFACE_S));
    memset(&dstSurface, 0, sizeof(TDE2_SURFACE_S));
    memset(&srcRect, 0, sizeof(TDE2_RECT_S));
    memset(&dstRect, 0, sizeof(TDE2_RECT_S));

    srcSurface.bAlphaMax255 = HI_TRUE;
    srcSurface.enColorFmt = TDE2_COLOR_FMT_byte;
    dstSurface.bAlphaMax255 = HI_TRUE;
    dstSurface.enColorFmt = TDE2_COLOR_FMT_byte;

    /* Y分量 */
    srcSurface.u32Width = pstSrcframe->stVFrame.u32Width;
    srcSurface.u32Height = pstSrcframe->stVFrame.u32Height;
    srcSurface.u32Stride = pstSrcframe->stVFrame.u32Stride[0];
    srcSurface.PhyAddr = pstSrcframe->stVFrame.u64PhyAddr[0];
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = srcSurface.u32Width;
    srcRect.u32Height = srcSurface.u32Height;

    dstSurface.u32Width = pstDstframe->stVFrame.u32Width;
    dstSurface.u32Height = pstDstframe->stVFrame.u32Height;
    dstSurface.u32Stride = pstDstframe->stVFrame.u32Stride[0];
    dstSurface.PhyAddr = pstDstframe->stVFrame.u64PhyAddr[0];
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = dstSurface.u32Width;
    dstRect.u32Height = dstSurface.u32Height;

    s32Ret = HI_TDE2_QuickCopy(s32Handle, &srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != HI_SUCCESS)
    {
        VDEC_LOGE("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    /* UV分量 */
    srcSurface.u32Width = pstSrcframe->stVFrame.u32Width;
    srcSurface.u32Height = pstSrcframe->stVFrame.u32Height / 2;
    srcSurface.u32Stride = pstSrcframe->stVFrame.u32Stride[1];
    srcSurface.PhyAddr = pstSrcframe->stVFrame.u64PhyAddr[1];
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = srcSurface.u32Width;
    srcRect.u32Height = pstSrcframe->stVFrame.u32Height / 2;

    dstSurface.u32Width = pstDstframe->stVFrame.u32Width;
    dstSurface.u32Height = pstDstframe->stVFrame.u32Height / 2;
    dstSurface.u32Stride = pstDstframe->stVFrame.u32Stride[1];
    dstSurface.PhyAddr = pstDstframe->stVFrame.u64PhyAddr[1];
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = dstSurface.u32Width;
    dstRect.u32Height = pstDstframe->stVFrame.u32Height / 2;

    s32Ret = HI_TDE2_QuickCopy(s32Handle, &srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != HI_SUCCESS)
    {
        VDEC_LOGE("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        VDEC_LOGE("HI_TDE2_EndJob failed %#x!\n", s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecMakeframe
 * @brief	   生成帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据内存
 * @param[in]  void *pstVideoFrame 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstVideoFrame)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    CHECK_PTR_NULL(pPicFramePrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstFrame = (VIDEO_FRAME_INFO_S *)pstVideoFrame;

    pstFrame->u32PoolId = pPicFramePrm->poolId;
    pstFrame->stVFrame.u32Width = pPicFramePrm->width;
    pstFrame->stVFrame.u32Height = pPicFramePrm->height;
    pstFrame->stVFrame.enField = VIDEO_FIELD_FRAME;
    pstFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    pstFrame->stVFrame.enPixelFormat = vdec_hisi_ChangePixFormat(pPicFramePrm->videoFormat);
    pstFrame->stVFrame.u64PhyAddr[0] = pPicFramePrm->phy[0];
    pstFrame->stVFrame.u64PhyAddr[1] = pPicFramePrm->phy[1];
    pstFrame->stVFrame.u64PhyAddr[2] = pPicFramePrm->phy[2];
    pstFrame->stVFrame.u64VirAddr[0] = HI_ADDR_P2U64(pPicFramePrm->addr[0]);
    pstFrame->stVFrame.u64VirAddr[1] = HI_ADDR_P2U64(pPicFramePrm->addr[1]);
    pstFrame->stVFrame.u64VirAddr[2] = HI_ADDR_P2U64(pPicFramePrm->addr[2]);
    pstFrame->stVFrame.u32Stride[0] = pPicFramePrm->stride;
    pstFrame->stVFrame.u32Stride[1] = pPicFramePrm->stride;
    pstFrame->stVFrame.u32Stride[2] = pPicFramePrm->stride;

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcVideoFrame, void *pstDstVideoFrame)
{
    INT32 s32Ret = SAL_SOK;
    VIDEO_FRAME_INFO_S *pstSrcFrame = NULL;
    VIDEO_FRAME_INFO_S *pstDstFrame = NULL;
    INT32 i = 0;

    CHECK_PTR_NULL(copyEn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstSrcVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstDstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstSrcFrame = (VIDEO_FRAME_INFO_S *)pstSrcVideoFrame;
    pstDstFrame = (VIDEO_FRAME_INFO_S *)pstDstVideoFrame;

    if (copyEn->copyMeth == VDEC_DATA_ADDR_COPY)
    {
        memcpy(pstDstFrame, pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));
    }
    else if (copyEn->copyMeth == VDEC_DATA_COPY)
    {
        pstDstFrame->stVFrame.u32TimeRef = pstSrcFrame->stVFrame.u32TimeRef;
        pstDstFrame->stVFrame.u64PTS = pstSrcFrame->stVFrame.u64PTS;
        pstDstFrame->stVFrame.u32Width = pstSrcFrame->stVFrame.u32Width;
        pstDstFrame->stVFrame.u32Height = pstSrcFrame->stVFrame.u32Height;
        pstDstFrame->stVFrame.enField = pstSrcFrame->stVFrame.enField;
        pstDstFrame->stVFrame.enVideoFormat = pstSrcFrame->stVFrame.enVideoFormat;
        pstDstFrame->stVFrame.enPixelFormat = pstSrcFrame->stVFrame.enPixelFormat;
        if (copyEn->copyth == 1)
        {
            for (i = 0; i < pstSrcFrame->stVFrame.u32Height; i++)
            {
                memcpy(HI_ADDR_U642P(pstDstFrame->stVFrame.u64VirAddr[0]) + i * pstDstFrame->stVFrame.u32Stride[0], HI_ADDR_U642P(pstSrcFrame->stVFrame.u64VirAddr[0]) + i * pstSrcFrame->stVFrame.u32Stride[0], pstSrcFrame->stVFrame.u32Width);
                if (i % 2 == 0)
                {
                    memcpy(HI_ADDR_U642P(pstDstFrame->stVFrame.u64VirAddr[1] + i * pstDstFrame->stVFrame.u32Stride[1] / 2), HI_ADDR_U642P(pstSrcFrame->stVFrame.u64VirAddr[1] + i * pstSrcFrame->stVFrame.u32Stride[1] / 2), pstSrcFrame->stVFrame.u32Width);
                }
            }
        }
        else
        {
            s32Ret = TdeQuickCopyYuv(pstDstFrame, pstSrcFrame);
            if (s32Ret != SAL_SOK)
            {
                VDEC_LOGE("tde copy failed\n");
                return s32Ret;
            }
        }
    }
    else
    {
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
    }

    return s32Ret;
}

/**
 * @function   vdec_hisi_VdecGetframe
 * @brief	   获取解码器数据帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecGetframe(void *handle, UINT32 dstChn, void *pstframe)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_hisi_VdecReleaseframe
 * @brief	   释放解码帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecReleaseframe(void *handle, UINT32 dstChn, void *pstframe)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_hisi_FreeYuvPoolBlockMem
 * @brief	   释放yuv内存
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_FreeYuvPoolBlockMem(HAL_MEM_PRM *addrPrm)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 pool = VB_INVALID_POOLID;
    HI_U64 VbBlk = VB_INVALID_HANDLE;
    HI_U8 *p64VirAddr = NULL;
    HI_U32 u32Size = 0;

    CHECK_PTR_NULL(addrPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pool = addrPrm->poolId;
    p64VirAddr = addrPrm->virAddr;
    VbBlk = addrPrm->vbBlk;
    u32Size = addrPrm->memSize;

    if (NULL != p64VirAddr)
    {
        s32Ret = HI_MPI_SYS_Munmap(p64VirAddr, u32Size);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    if (VB_INVALID_HANDLE != VbBlk)
    {
        s32Ret = HI_MPI_VB_ReleaseBlock(VbBlk);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    if (VB_INVALID_POOLID != pool)
    {
        s32Ret = HI_MPI_VB_DestroyPool(pool);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_GetYuvPoolBlockMem
 * @brief	   申请yuv平台相关内存
 * @param[in]  UINT32 imgW 宽
 * @param[in]  UINT32 imgH 高
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_GetYuvPoolBlockMem(UINT32 imgW, UINT32 imgH, HAL_MEM_PRM *pOutFrm)
{
/*	char mmzName[10] = "VDEC"; */
    VB_POOL pool = VB_INVALID_POOLID;
    UINT64 u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3 / 2);
    VB_BLK VbBlk = 0;
    UINT64 u64PhyAddr = 0;
    UINT64 *p64VirAddr = NULL;
    UINT32 u32LumaSize = 0;
    UINT32 u32ChrmSize = 0;
    UINT64 u64Size = 0;
    VB_POOL_CONFIG_S stVbPoolCfg;

    if (VB_INVALID_POOLID == pool)
    {
        SAL_clear(&stVbPoolCfg);
        stVbPoolCfg.u64BlkSize = u64BlkSize;
        stVbPoolCfg.u32BlkCnt = 1;
        stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;

/*	    sprintf(stVbPoolCfg.acMmzName, "%s", mmzName); */
        pool = HI_MPI_VB_CreatePool(&stVbPoolCfg);

        if (VB_INVALID_POOLID == pool)
        {
            DUP_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64BlkSize);
            return SAL_FAIL;
        }
    }

    u32LumaSize = (imgW * imgH);
    u32ChrmSize = (imgW * imgH / 4);
    u64Size	= (UINT64)(u32LumaSize + (u32ChrmSize << 1));

    VbBlk = HI_MPI_VB_GetBlock(pool, u64Size, NULL);
    if (VB_INVALID_POOLID == pool)
    {
        DUP_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    u64PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == u64PhyAddr)
    {
        DUP_LOGE("HI_MPI_VB_Handle2PhysAddr failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    p64VirAddr = (UINT64 *) HI_MPI_SYS_Mmap(u64PhyAddr, u64Size);
    if (NULL == p64VirAddr)
    {
        DUP_LOGE("HI_MPI_SYS_Mmap failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    pOutFrm->phyAddr = u64PhyAddr;
    pOutFrm->virAddr = p64VirAddr;
    pOutFrm->memSize = u64BlkSize;
    pOutFrm->poolId = pool;

    return SAL_SOK;
}

/**
 * @function   vdec_hisi_VdecMmap
 * @brief	   帧映射
 * @param[in]  void *pVideoFrame 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm  映射地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecMmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    CHECK_PTR_NULL(pVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pstFrame = (VIDEO_FRAME_INFO_S *)pVideoFrame;
    void *virAddr = NULL;

    if (pstFrame->stVFrame.u64VirAddr[0] == 0)
    {
        HI_MPI_VB_GetBlockVirAddr(pstFrame->u32PoolId, pstFrame->stVFrame.u64PhyAddr[0], (void **)&virAddr);
        pstFrame->stVFrame.u64VirAddr[0] = (HI_U64)virAddr;
        HI_MPI_VB_GetBlockVirAddr(pstFrame->u32PoolId, pstFrame->stVFrame.u64PhyAddr[1], (void **)&virAddr);
        pstFrame->stVFrame.u64VirAddr[1] = (HI_U64)virAddr;
        HI_MPI_VB_GetBlockVirAddr(pstFrame->u32PoolId, pstFrame->stVFrame.u64PhyAddr[2], (void **)&virAddr);
        pstFrame->stVFrame.u64VirAddr[2] = (HI_U64)virAddr;
    }

    pPicprm->width = pstFrame->stVFrame.u32Width;
    pPicprm->height = pstFrame->stVFrame.u32Height;
    pPicprm->stride = pstFrame->stVFrame.u32Stride[0];
    pPicprm->videoFormat = vdec_hisi_TransPixFormat(pstFrame->stVFrame.enPixelFormat);
    pPicprm->poolId = pstFrame->u32PoolId;
    pPicprm->addr[0] = (void *)pstFrame->stVFrame.u64VirAddr[0];
    pPicprm->addr[1] = (void *)pstFrame->stVFrame.u64VirAddr[1];
    pPicprm->addr[2] = (void *)pstFrame->stVFrame.u64VirAddr[2];
    pPicprm->phy[0] = pstFrame->stVFrame.u64PhyAddr[0];
    pPicprm->phy[1] = pstFrame->stVFrame.u64PhyAddr[1];
    pPicprm->phy[2] = pstFrame->stVFrame.u64PhyAddr[2];

    return SAL_SOK;
}

/**
 * @function   vdec_halRegister
 * @brief	   函数注册
 * @param[in]
 * @param[out] None
 * @return	  INT32
 */
VDEC_HAL_FUNC *vdec_halRegister()
{
    memset(&vdecFunc, 0, sizeof(VDEC_HAL_FUNC));

    vdecFunc.VdecCreate = vdec_hisi_VdecCreate;
    vdecFunc.VdecDeinit = vdec_hisi_VdecDeinit;
    vdecFunc.VdecClear = vdec_hisi_VdecClear;
    vdecFunc.VdecSetPrm = vdec_hisi_VdecSetPrm;
    vdecFunc.VdecStart = vdec_hisi_VdecStart;
    vdecFunc.VdecStop = vdec_hisi_VdecStop;
    vdecFunc.VdecDecframe = vdec_hisi_VdecDecframe;
    vdecFunc.VdecGetframe = vdec_hisi_VdecGetframe;
    vdecFunc.VdecReleaseframe = vdec_hisi_VdecReleaseframe;
    vdecFunc.VdecCpyframe = vdec_hisi_VdecCpyframe;
    vdecFunc.VdecFreeYuvPoolBlockMem = vdec_hisi_FreeYuvPoolBlockMem;
    vdecFunc.VdecMallocYUVMem = vdec_hisi_GetYuvPoolBlockMem;
    vdecFunc.VdecMakeframe = vdec_hisi_VdecMakeframe;
    vdecFunc.VdecMmap = vdec_hisi_VdecMmap;

    return &vdecFunc;
}

