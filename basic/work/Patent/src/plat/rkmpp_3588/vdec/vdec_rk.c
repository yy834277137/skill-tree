/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_rk.c
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#include <sal.h>
#include "vdec_hal_platfrom.h"
#include "sal_mem_new.h"
#include "vdec_hal_inter.h"
#include <platform_sdk.h>




#define MAX_STREAM_CNT               8

/*解码器属性，平台差异化部分*/
typedef struct _VDEC_rk_SPEC_
{
    MB_SOURCE_E vbModule;
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
    MB_POOL stPool;
} VDEC_RK_SPEC;

typedef struct _VDEC_HAL_PLT_HANDLE_
{
    INT32 width;                                              /*解码器宽*/
    INT32 height;                                             /*解码器高*/
    INT32 encType;                                            /*解码数据编码格式*/
    INT32 decChan;                                            /*对应实际的解码器通道号*/
    INT32 vdecOutStatue;                                      /* 解码器输出创建1，解码器输出未创建0, 2 正在使用 */
    VDEC_RK_SPEC vdecHisiHdl;
} VDEC_HAL_PLT_HANDLE;

static VDEC_HAL_FUNC vdecFunc;


//#define SAVE_VDEC_RKFRAME
#ifdef SAVE_VDEC_RKFRAME
static FILE *pVdecfile[MAX_STREAM_CNT] = {NULL};

/**
 * @function   vdec_rk_saveBuffFile
 * @brief      保存送帧解码器前数据调试
 * @param[in]  INT32 chn 通道号
 * @param[in]  UINT32 encodeType 码流类型
 * @param[in]  void *data 数据源
 * @param[in]  int size 数据大小
 * @param[out] None
 * @return    INT32
 */
static void vdec_rk_saveFrame(INT32 chn,UINT32 encodeType, void *data, int size)
{
    char filename[16]={0};
    /* Open output file */
    if(pVdecfile[chn] == NULL)
    {
        sprintf(filename, "./vdec%d_%d.bin", chn, encodeType);
        if ((pVdecfile[chn] = fopen(filename, "wb")) == NULL) 
        {
            VDEC_LOGE("[ERROR] Open File %s failed!!\n", filename);
            return;
        }
    }
    /* Write buffer to output file */
    fwrite(data, 1, size, pVdecfile[chn]);
}
#endif


/******************************************************************
   Function:   vdec_rk_MemAlloc
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
VOID *vdec_rk_MemAlloc(UINT32 u32Format, void *prm)
{
    return NULL;
}

/******************************************************************
   Function:   vdec_rk_MemFree
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 vdec_rk_MemFree(void *ptr, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    return s32Ret;
}

/**
 * @function   vdec_rk_ChangePixFormat
 * @brief     映射平台相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return    INT32
 */
static INT32 vdec_rk_ChangePixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            format = RK_FMT_YUV420SP_VU;
            break;
        }
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            format = RK_FMT_BGR888;
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
 * @function   vdec_rk_TransPixFormat
 * @brief     映射业务相关格式
 * @param[in]  INT32 pixFormat 数据像素格式
 * @param[out] None
 * @return    INT32
 */
static INT32 vdec_rk_TransPixFormat(INT32 pixFormat)
{
    INT32 format = 0;

    switch (pixFormat)
    {
        case RK_FMT_YUV420SP_VU:
        {
            format = SAL_VIDEO_DATFMT_YUV420SP_VU;
            break;
        }
        case RK_FMT_BGR888:
        {
            format = SAL_VIDEO_DATFMT_RGB24_888;
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
 * @function   vdec_rk_TransEncType
 * @brief     映射业务相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_TransEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case RK_VIDEO_ID_AVC:
        {
            type = H264;
            break;
        }
        case RK_VIDEO_ID_HEVC:
        {
            type = H265;
            break;
        }
        case RK_VIDEO_ID_MJPEG:
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
 * @function   vdec_rk_TransEncType
 * @brief     映射平台相关编码格式
 * @param[in]  INT32 encType 编码格式
 * @param[out] None
 * @return    INT32
 */
static INT32 vdec_rk_ChangeEncType(INT32 encType)
{
    INT32 type = 0;

    switch (encType)
    {
        case H264:
        {
            type = RK_VIDEO_ID_AVC;
            break;
        }
        case H265:
        {
            type = RK_VIDEO_ID_HEVC;
            break;
        }
        case MJPEG:
        {
            type = RK_VIDEO_ID_MJPEG;
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
 * @function   vdec_rk_SetModParam
 * @brief     设置模式
 * @param[in]  MB_SOURCE_E vbModule 模式
 * @param[out] None
 * @return    INT32
 */
static INT32 vdec_rk_SetModParam(MB_SOURCE_E vbModule)
{
    INT32 s32Ret = RK_SUCCESS;
    VDEC_MOD_PARAM_S stModParam;

    memset(&stModParam, 0x00, sizeof(VDEC_MOD_PARAM_S));

    s32Ret = RK_MPI_VDEC_GetModParam(&stModParam);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error.\n");
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (stModParam.enVdecMBSource != vbModule)
    {
        stModParam.enVdecMBSource = vbModule;

        s32Ret = RK_MPI_VDEC_SetModParam(&stModParam);
        if (RK_SUCCESS != s32Ret)
        {
            VDEC_LOGE("error.\n");
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_DeleteVBPool
 * @brief     删除vb内存
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_DeleteVBPool(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;

    if (pVdecHalHdl->vdecHisiHdl.vbModule == MB_SOURCE_USER)
    {
        if (MB_INVALID_POOLID != pVdecHalHdl->vdecHisiHdl.stPool)
        {
            s32Ret = RK_MPI_MB_DestroyPool(pVdecHalHdl->vdecHisiHdl.stPool);
            if (RK_SUCCESS != s32Ret)
            {
                VDEC_LOGE("vb destory pool failed s32Ret 0x%x\n", s32Ret);
                return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
            }

            pVdecHalHdl->vdecHisiHdl.stPool = MB_INVALID_POOLID;
        }
    }
    else
    {
         RK_LOGE(" del VB pool MB_SOURCE_USER. err\n");
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_InitVbPool
 * @brief     初始化vb内存
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_InitVbPool(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = SAL_SOK;
    MB_POOL_CONFIG_S stVbPoolCfg;
    VDEC_PIC_BUF_ATTR_S stVdecPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));


    stVdecPicBufAttr.enCodecType = vdec_rk_ChangeEncType(pVdecHalHdl->encType);
    stVdecPicBufAttr.stPicBufAttr.u32Width = pVdecHalHdl->width;
    stVdecPicBufAttr.stPicBufAttr.u32Height = pVdecHalHdl->height;
    stVdecPicBufAttr.stPicBufAttr.enPixelFormat = (RK_S32)RK_FMT_YUV420SP;
    stVdecPicBufAttr.stPicBufAttr.enCompMode = COMPRESS_MODE_NONE;
    s32Ret = RK_MPI_CAL_VDEC_GetPicBufferSize(&stVdecPicBufAttr, &stMbPicCalResult);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("get picture buffer size failed. err 0x%x", s32Ret);
        return s32Ret;
    }
    pVdecHalHdl->vdecHisiHdl.u32PicBufSzie = stMbPicCalResult.u32MBSize;




    if (pVdecHalHdl->vdecHisiHdl.vbModule == MB_SOURCE_USER)
    {

        pVdecHalHdl->vdecHisiHdl.stPool = MB_INVALID_POOLID;
      
        if ((0 != pVdecHalHdl->vdecHisiHdl.u32PicBufSzie) && (0 != pVdecHalHdl->vdecHisiHdl.frameBufCnt))
        {
            memset(&stVbPoolCfg, 0x00, sizeof(MB_POOL_CONFIG_S));
            stVbPoolCfg.u64MBSize = pVdecHalHdl->vdecHisiHdl.u32PicBufSzie;
            stVbPoolCfg.u32MBCnt = pVdecHalHdl->vdecHisiHdl.frameBufCnt;
            stVbPoolCfg.enRemapMode = MB_REMAP_MODE_NONE;
            stVbPoolCfg.bPreAlloc = RK_TRUE;
            pVdecHalHdl->vdecHisiHdl.stPool = RK_MPI_MB_CreatePool(&stVbPoolCfg);
            if (MB_INVALID_POOLID == pVdecHalHdl->vdecHisiHdl.stPool)
            {
                VDEC_LOGE("uiChn %d \n", pVdecHalHdl->decChan);
                return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
            }
        }

    }
    else
    {
        RK_LOGE(" init VB pool MB_SOURCE_USER. err\n");
    }
   
    return s32Ret;
}

/**
 * @function   vdec_rk_ChnCreate
 * @brief     解码通道创建
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_ChnCreate(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
{
    INT32 s32Ret = 0;
    VDEC_CHN_ATTR_S stChnAttr;
    VDEC_CHN_PARAM_S stChnParam;
    VDEC_PIC_BUF_ATTR_S stVdecPicBufAttr;
    MB_PIC_CAL_S stMbPicCalResult;

    memset(&stChnAttr, 0x00, sizeof(VDEC_CHN_ATTR_S));
    memset(&stChnParam, 0x00, sizeof(VDEC_CHN_PARAM_S));

    CHECK_PTR_NULL(pVdecHalHdl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    stChnAttr.enType = vdec_rk_ChangeEncType(pVdecHalHdl->encType);
    stChnAttr.enMode = pVdecHalHdl->vdecHisiHdl.enMode;
    stChnAttr.u32PicWidth = pVdecHalHdl->width;
    stChnAttr.u32PicHeight = pVdecHalHdl->height;

    stChnAttr.u32StreamBufSize = pVdecHalHdl->width * pVdecHalHdl->height * 3 / 2;
    stChnAttr.u32FrameBufCnt = pVdecHalHdl->vdecHisiHdl.frameBufCnt;
    stChnAttr.u32StreamBufCnt = MAX_STREAM_CNT;



    stVdecPicBufAttr.enCodecType = vdec_rk_ChangeEncType(pVdecHalHdl->encType);
    stVdecPicBufAttr.stPicBufAttr.u32Width = pVdecHalHdl->width;
    stVdecPicBufAttr.stPicBufAttr.u32Height = pVdecHalHdl->height;
    stVdecPicBufAttr.stPicBufAttr.enPixelFormat = RK_FMT_YUV420SP_VU;
    stVdecPicBufAttr.stPicBufAttr.enCompMode = COMPRESS_MODE_NONE;
    s32Ret = RK_MPI_CAL_VDEC_GetPicBufferSize(&stVdecPicBufAttr, &stMbPicCalResult);
    if (s32Ret != RK_SUCCESS) {
        RK_LOGE("get picture buffer size failed. err 0x%x", s32Ret);
        return s32Ret;
    }
    stChnAttr.u32FrameBufSize = stMbPicCalResult.u32MBSize;


    if (RK_VIDEO_ID_AVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType) || 
          RK_VIDEO_ID_HEVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnAttr.enMode = VIDEO_MODE_FRAME;
        stChnAttr.stVdecVideoAttr.u32RefFrameNum = pVdecHalHdl->vdecHisiHdl.refFrameNum;
        stChnAttr.stVdecVideoAttr.bTemporalMvpEnable = 1;
        
        if ((RK_VIDEO_ID_AVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType)) && 
                 (VIDEO_DEC_MODE_IPB != pVdecHalHdl->vdecHisiHdl.enDecMode))
        {
            stChnAttr.stVdecVideoAttr.bTemporalMvpEnable = 0;
        }

    }
    else if (RK_VIDEO_ID_MJPEG == vdec_rk_ChangeEncType(pVdecHalHdl->encType) || 
          RK_VIDEO_ID_JPEG == vdec_rk_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnAttr.enMode = VIDEO_MODE_FRAME;
    }

    VDEC_LOGI("Create Vdec chn %d enType %d enMode %d   %d x %d \n", pVdecHalHdl->decChan, stChnAttr.enType, stChnAttr.enMode, stChnAttr.u32PicWidth, stChnAttr.u32PicHeight);

    s32Ret = RK_MPI_VDEC_CreateChn(pVdecHalHdl->decChan, &stChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VDEC_GetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    if (RK_VIDEO_ID_AVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType) || RK_VIDEO_ID_HEVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnParam.stVdecVideoParam.enDecMode = pVdecHalHdl->vdecHisiHdl.enDecMode;
        stChnParam.stVdecVideoParam.enCompressMode = COMPRESS_MODE_NONE;
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

    s32Ret = RK_MPI_VDEC_SetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }


    if (MB_SOURCE_USER == pVdecHalHdl->vdecHisiHdl.vbModule)
     {
    
         s32Ret = RK_MPI_VDEC_AttachMbPool(pVdecHalHdl->decChan, pVdecHalHdl->vdecHisiHdl.stPool);
         if (RK_SUCCESS != s32Ret)
         {
             VDEC_LOGE("error 0x%x \n", s32Ret);
             return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
         }
     }


    s32Ret = RK_MPI_VDEC_SetDisplayMode(pVdecHalHdl->decChan, pVdecHalHdl->vdecHisiHdl.enDisplayMode);

    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecCreate
 * @brief     解码器创建
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 参数
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecCreate(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
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
    pVdecHalHdl->vdecHisiHdl.vbModule = MB_SOURCE_USER;
    pVdecHalHdl->vdecHisiHdl.enDynamicRange = DYNAMIC_RANGE_SDR8;
    pVdecHalHdl->vdecHisiHdl.enMode = VIDEO_MODE_FRAME;
    pVdecHalHdl->vdecHisiHdl.enDecMode = VIDEO_DEC_MODE_IP;
    pVdecHalHdl->vdecHisiHdl.enBitWidth = DATA_BITWIDTH_8;
    pVdecHalHdl->vdecHisiHdl.enPixelFormat = RK_FMT_YUV420SP_VU;
    pVdecHalHdl->vdecHisiHdl.enDisplayMode = VIDEO_DISPLAY_MODE_PREVIEW;
    pVdecHalHdl->vdecHisiHdl.displayFrameNum = 5;
    pVdecHalHdl->vdecHisiHdl.refFrameNum = 3;
    pVdecHalHdl->vdecHisiHdl.frameBufCnt = pVdecHalHdl->vdecHisiHdl.displayFrameNum
                                           + pVdecHalHdl->vdecHisiHdl.refFrameNum;
    vdec_rk_SetModParam(pVdecHalHdl->vdecHisiHdl.vbModule);

    s32Ret = vdec_rk_InitVbPool(pVdecHalHdl);
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

    s32Ret = vdec_rk_ChnCreate(pVdecHalHdl);
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
 * @function   vdec_rk_VdecStart
 * @brief      启动解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecStart(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = RK_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_ENABLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecDeinit
 * @brief      释放解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecDeinit(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    if (pVdecHalHdl->vdecHisiHdl.vbModule == MB_SOURCE_USER)
    {
        s32Ret = RK_MPI_VDEC_DetachMbPool(pVdecHalHdl->decChan);
        if (RK_SUCCESS != s32Ret)
        {
            VDEC_LOGE("VDEC_DetachMbPool error 0x%x \n", s32Ret);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
        }

    }
    
    s32Ret = RK_MPI_VDEC_DestroyChn(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = vdec_rk_DeleteVBPool(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    if(0 != pVdecHalHdl->vdecOutStatue)
    {
         VDEC_LOGI("VDEC SAL_memfree %d \n", pVdecHalHdl->vdecOutStatue);
         SAL_memfree((Ptr)pVdecHalHdl, "VDEC", "mallocOs");
    }
   
    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecSetPrm
 * @brief      设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 解码参数
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecSetPrm(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm)
{
    INT32 s32Ret = SAL_SOK;

    VDEC_CHN_ATTR_S stChnAttr;
    VDEC_CHN_PARAM_S stChnParam;

    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    memset(&stChnAttr, 0x00, sizeof(VDEC_CHN_ATTR_S));
    memset(&stChnParam, 0x00, sizeof(VDEC_CHN_PARAM_S));

    CHECK_PTR_NULL(pVdecHalPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)pVdecHalPrm->decPlatHandle;

    s32Ret = RK_MPI_VDEC_GetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    if (RK_VIDEO_ID_AVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType) || RK_VIDEO_ID_HEVC == vdec_rk_ChangeEncType(pVdecHalHdl->encType))
    {
        stChnParam.enType = vdec_rk_ChangeEncType(pVdecHalHdl->encType);
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

    s32Ret = RK_MPI_VDEC_SetChnParam(pVdecHalHdl->decChan, &stChnParam);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VDEC_StopRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VDEC_GetChnAttr(pVdecHalHdl->decChan, &stChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stChnAttr.enType = vdec_rk_ChangeEncType(pVdecHalHdl->encType);
    stChnAttr.enMode = pVdecHalHdl->vdecHisiHdl.enMode;

    VDEC_LOGI("set attr Vdec chn %d enType %d enMode %d   %d x %d \n", pVdecHalHdl->decChan, stChnAttr.enType, stChnAttr.enMode, stChnAttr.u32PicWidth, stChnAttr.u32PicHeight);

    s32Ret = RK_MPI_VDEC_SetChnAttr(pVdecHalHdl->decChan, &stChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    VDEC_LOGI("decChan %d ok\n", pVdecHalHdl->decChan);

    return s32Ret;
}

/**
 * @function   vdec_rk_VdecClear
 * @brief      清除解码器数据
 * @param[in]  void *handle handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecClear(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;

    s32Ret = RK_MPI_VDEC_StopRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    /* reset vdec chn*/
    s32Ret = RK_MPI_VDEC_ResetChn(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = RK_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_ENABLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecStop
 * @brief      停止解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecStop(void *handle)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;

    CHECK_PTR_NULL(handle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalHdl = (VDEC_HAL_PLT_HANDLE *)handle;
    
   


    s32Ret = RK_MPI_VDEC_StopRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    /* reset vdec chn*/
    s32Ret = RK_MPI_VDEC_ResetChn(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

#ifdef SAVE_VDEC_RKFRAME
    fclose(pVdecfile[pVdecHalHdl->decChan]);
    pVdecfile[pVdecHalHdl->decChan] = NULL;
#endif

    return SAL_SOK;
}

/**
 * @function   vdec_rk_UpdatVdecChn
 * @brief      更新解码器
 * @param[in]  VDEC_HAL_PLT_HANDLE *pVdecHalHdl handle
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_UpdatVdecChn(VDEC_HAL_PLT_HANDLE *pVdecHalHdl)
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

    s32Ret = vdec_rk_VdecStop(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_rk_VdecDeinit(pVdecHalHdl);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_DISABLE);
    }

    s32Ret = vdec_rk_InitVbPool(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    s32Ret = vdec_rk_ChnCreate(pVdecHalHdl);
    if (SAL_SOK != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_INIT);
    }

    s32Ret = RK_MPI_VDEC_StartRecvStream(pVdecHalHdl->decChan);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("error 0x%x \n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_FAILED_ENABLE);
    }

    pVdecHalHdl->vdecOutStatue = 1;

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecDecframe
 * @brief      清除解码器数据
 * @param[in]  void *handle handle
 * @param[in]  SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in]  SAL_VideoFrameBuf *pFrameOut 输出数据
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecDecframe(void *handle, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut)
{
    INT32 s32Ret = SAL_SOK;
    RK_S32 s32MilliSec = 1000;
    VDEC_STREAM_S stStream = {0};
    VDEC_CHN_STATUS_S stStatus = {0};
    VDEC_HAL_PLT_HANDLE *pVdecHalHdl = NULL;
    MB_EXT_CONFIG_S stMbExtConfig = {0};

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
            || vdec_rk_ChangeEncType(pFrameIn->frameParam.encodeType) != vdec_rk_ChangeEncType(pVdecHalHdl->encType))
        {
            VDEC_LOGE("vdec chn %d width %d height %d type %d is error dec[w %d h %d type %d]\n", pVdecHalHdl->decChan, pFrameIn->frameParam.width, pFrameIn->frameParam.height,
                      pFrameIn->frameParam.encodeType, pVdecHalHdl->width, pVdecHalHdl->height, pVdecHalHdl->encType);
            return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_PARAM);
        }

        s32Ret = RK_MPI_VDEC_QueryStatus(pVdecHalHdl->decChan, &stStatus);
        if (RK_SUCCESS != s32Ret)
        {
            VDEC_LOGW("will reset vdec\n");
            s32Ret = vdec_rk_VdecClear(pVdecHalHdl);
            if (SAL_SOK != s32Ret)
            {
                VDEC_LOGE("error %#x\n", s32Ret);
                return s32Ret;
            }
        }
    }
  
    stMbExtConfig.pu8VirAddr = (RK_U8 *)pFrameIn->virAddr[0];
    stMbExtConfig.u64Size   = pFrameIn->bufLen;
    s32Ret = RK_MPI_SYS_CreateMB(&(stStream.pMbBlk), &stMbExtConfig);  /*外部malloc通过CreateMB获取pMbBlk，ReleaseMB需要释放*/
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("RK_MPI_SYS_CreateMB %#x\n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_TIMEOUT);
    }

    stStream.u64PTS = pFrameIn->pts;//0
    stStream.u32Len = pFrameIn->bufLen;
    stStream.bEndOfFrame = RK_FALSE;
    stStream.bEndOfStream = RK_FALSE;
    stStream.bBypassMbBlk = RK_FALSE;  /*RK_FALSE拷贝模式，RK_TRUE直通模式*/
    
#ifdef SAVE_VDEC_RKFRAME
    vdec_rk_saveFrame(pVdecHalHdl->decChan,pFrameIn->frameParam.encodeType,(void *)pFrameIn->virAddr[0], pFrameIn->bufLen);
#endif

    s32Ret = RK_MPI_VDEC_SendStream(pVdecHalHdl->decChan, &stStream, s32MilliSec);
    if (RK_SUCCESS != s32Ret)
    {
         VDEC_LOGW("RK_MPI_VDEC_SendStream %#x\n", s32Ret);
         s32Ret = vdec_rk_UpdatVdecChn(pVdecHalHdl);
         if (SAL_SOK != s32Ret)
         {
             VDEC_LOGE("error %#x\n", s32Ret);
             RK_MPI_MB_ReleaseMB(stStream.pMbBlk);
             return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
         }
         s32Ret = RK_MPI_VDEC_SendStream(pVdecHalHdl->decChan, &stStream, s32MilliSec);
         if (RK_SUCCESS != s32Ret)
         {
             VDEC_LOGE("RK_MPI_VDEC_SendStream %#x\n", s32Ret);
             RK_MPI_MB_ReleaseMB(stStream.pMbBlk);
             return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
         }
         
    }

    s32Ret = RK_MPI_MB_ReleaseMB(stStream.pMbBlk);
    if (RK_SUCCESS != s32Ret)
    {
        VDEC_LOGE("RK_MPI_VDEC_SendStream %#x\n", s32Ret);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return s32Ret;
}

/**
 * @function   TdeQuickCopyYuv
 * @brief      数据帧tde拷贝
 * @param[in]  VIDEO_FRAME_INFO_S *pstDstframe 源数据
 * @param[in]  VIDEO_FRAME_INFO_S *pstSrcframe 目的
 * @param[out] None
 * @return    INT32
 */
static INT32 TdeQuickCopy(VIDEO_FRAME_INFO_S *pstDstframe, VIDEO_FRAME_INFO_S *pstSrcframe)
{
    INT32 s32Ret = 0;
    TDE_HANDLE s32Handle;
    TDE_SURFACE_S srcSurface, dstSurface;
    TDE_RECT_S srcRect, dstRect;

    s32Handle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == s32Handle || RK_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        VDEC_LOGE("RK_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    memset(&srcSurface, 0, sizeof(TDE_SURFACE_S));
    memset(&dstSurface, 0, sizeof(TDE_SURFACE_S));
    memset(&srcRect, 0, sizeof(TDE_RECT_S));
    memset(&dstRect, 0, sizeof(TDE_RECT_S));

    srcSurface.enColorFmt = pstSrcframe->stVFrame.enPixelFormat;
    dstSurface.enColorFmt = pstSrcframe->stVFrame.enPixelFormat;

    /*拷贝需要使用虚宽虚高 */
    srcSurface.u32Width = pstSrcframe->stVFrame.u32VirWidth;
    srcSurface.u32Height = pstSrcframe->stVFrame.u32VirHeight;
    srcSurface.pMbBlk = pstSrcframe->stVFrame.pMbBlk;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = srcSurface.u32Width;
    srcRect.u32Height = srcSurface.u32Height;

    dstSurface.u32Width = pstDstframe->stVFrame.u32VirWidth;
    dstSurface.u32Height = pstDstframe->stVFrame.u32VirHeight;
    dstSurface.pMbBlk = pstDstframe->stVFrame.pMbBlk;
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = dstSurface.u32Width;
    dstRect.u32Height = dstSurface.u32Height;


    s32Ret = RK_TDE_QuickCopy(s32Handle, &srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != RK_SUCCESS)
    {
        VDEC_LOGE("RK_TDE_CancelJob failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        RK_TDE_CancelJob(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = RK_TDE_EndJob(s32Handle, RK_FALSE, RK_TRUE, 100);
    if (s32Ret != RK_SUCCESS)
    {
        VDEC_LOGE("RK_TDE_CancelJob failed %#x!\n", s32Ret);
        RK_TDE_CancelJob(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    s32Ret = RK_TDE_WaitForDone(s32Handle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        RK_TDE_CancelJob(s32Handle);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_HANDLE);
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecMakeframe
 * @brief      生成帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据内存
 * @param[in]  void *pstVideoFrame 数据帧
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstVideoFrame)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    CHECK_PTR_NULL(pPicFramePrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pstFrame = (VIDEO_FRAME_INFO_S *)pstVideoFrame;

   // pstFrame->u32PoolId = pPicFramePrm->poolId;
    pstFrame->stVFrame.u32Width = pPicFramePrm->width;
    pstFrame->stVFrame.u32Height = pPicFramePrm->height;
    pstFrame->stVFrame.enField = VIDEO_FIELD_FRAME;
    pstFrame->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    pstFrame->stVFrame.enPixelFormat = vdec_rk_ChangePixFormat(pPicFramePrm->videoFormat);
    pstFrame->stVFrame.pMbBlk = (MB_BLK)pPicFramePrm->phy[0];
    pstFrame->stVFrame.pVirAddr[0] = pPicFramePrm->addr[0];
    pstFrame->stVFrame.pVirAddr[1] = pPicFramePrm->addr[1];
    pstFrame->stVFrame.u32VirWidth = pPicFramePrm->width;
    pstFrame->stVFrame.u32VirHeight = pPicFramePrm->height;

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecCpyframe
 * @brief      数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcVideoFrame, void *pstDstVideoFrame)
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
        pstDstFrame->stVFrame.u32VirWidth = pstSrcFrame->stVFrame.u32VirWidth;
        pstDstFrame->stVFrame.u32VirHeight = pstSrcFrame->stVFrame.u32VirHeight;
        pstDstFrame->stVFrame.enField = pstSrcFrame->stVFrame.enField;
        pstDstFrame->stVFrame.enVideoFormat = pstSrcFrame->stVFrame.enVideoFormat;
        pstDstFrame->stVFrame.enPixelFormat = pstSrcFrame->stVFrame.enPixelFormat;
        if (copyEn->copyth == 1)
        {
            /*CPU拷贝需要刷新Cache*/
            if(pstSrcFrame->stVFrame.pMbBlk)
            {
                s32Ret = RK_MPI_SYS_MmzFlushCache((void*)pstSrcFrame->stVFrame.pMbBlk, 0);
                if (SAL_SOK != s32Ret)
                {
                    SAL_LOGD("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
                }
            }

            /*RK使用RGB显示，只有智能和jpeg使用RK_FMT_YUV420SP_VU*/
            if(pstSrcFrame->stVFrame.enPixelFormat != RK_FMT_YUV420SP_VU)
            {
                /*RGB拷贝需要使用虚宽虚高*/
                memcpy((pstDstFrame->stVFrame.pVirAddr[0]), (pstSrcFrame->stVFrame.pVirAddr[0]) , pstSrcFrame->stVFrame.u32VirHeight*pstSrcFrame->stVFrame.u32VirWidth*3);
            }
            else
            {
                for (i = 0; i < pstSrcFrame->stVFrame.u32Height; i++)
                {
                    memcpy(pstDstFrame->stVFrame.pVirAddr[0] + i * pstDstFrame->stVFrame.u32Width, pstSrcFrame->stVFrame.pVirAddr[0] + i * pstSrcFrame->stVFrame.u32Width, pstSrcFrame->stVFrame.u32Width);
                    if (i % 2 == 0)
                    {
                        memcpy(pstDstFrame->stVFrame.pVirAddr[1] + i * pstDstFrame->stVFrame.u32Width / 2, pstSrcFrame->stVFrame.pVirAddr[1] + i * pstSrcFrame->stVFrame.u32Width / 2, pstSrcFrame->stVFrame.u32Width);
                    }
                }
            }

            if(pstDstFrame->stVFrame.pMbBlk)
            {
                s32Ret = RK_MPI_SYS_MmzFlushCache((void*)pstDstFrame->stVFrame.pMbBlk, 0);
                if (SAL_SOK != s32Ret)
                {
                    SAL_LOGD("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
                }
            }
        }
        else
        {
            s32Ret = TdeQuickCopy(pstDstFrame, pstSrcFrame);
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
 * @function   vdec_rk_VdecGetframe
 * @brief      获取解码器数据帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecGetframe(void *handle, UINT32 dstChn, void *pstframe)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_rk_VdecReleaseframe
 * @brief      释放解码帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecReleaseframe(void *handle, UINT32 dstChn, void *pstframe)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_rk_FreeYuvPoolBlockMem
 * @brief      释放yuv内存
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_FreeYuvPoolBlockMem(HAL_MEM_PRM *addrPrm)
{
    RK_S32 s32Ret = RK_SUCCESS;
    RK_U32 pool = MB_INVALID_POOLID;
    MB_BLK VbBlk = MB_INVALID_HANDLE;

    CHECK_PTR_NULL(addrPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pool = addrPrm->poolId;
    VbBlk = (MB_BLK)addrPrm->vbBlk;


    if (MB_INVALID_HANDLE != VbBlk)
    {
        s32Ret = RK_MPI_MB_ReleaseMB(VbBlk);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    if (MB_INVALID_POOLID != pool)
    {
        s32Ret =  RK_MPI_MB_DestroyPool(pool);
        if (RK_SUCCESS != s32Ret)
        {
            VENC_LOGE("err %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   vdec_rk_GetYuvPoolBlockMem
 * @brief      申请yuv平台相关内存
 * @param[in]  UINT32 imgW 宽
 * @param[in]  UINT32 imgH 高
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_GetYuvPoolBlockMem(UINT32 imgW, UINT32 imgH, HAL_MEM_PRM *pOutFrm)
{
/*  char mmzName[10] = "VDEC"; */
    MB_POOL pool = MB_INVALID_POOLID;
    UINT64 u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3);
    MB_BLK VbBlk = 0;
    UINT64 *p64VirAddr = NULL;
    MB_POOL_CONFIG_S stVbPoolCfg;

    if (MB_INVALID_POOLID == pool)
    {
        SAL_clear(&stVbPoolCfg);
        stVbPoolCfg.u64MBSize = u64BlkSize;
        stVbPoolCfg.u32MBCnt = 1;
        stVbPoolCfg.enRemapMode = MB_REMAP_MODE_NONE;
        stVbPoolCfg.bPreAlloc = RK_TRUE;

        pool = RK_MPI_MB_CreatePool(&stVbPoolCfg);

        if (MB_INVALID_POOLID == pool)
        {
            DUP_LOGE("RK_MPI_MB_CreatePool failed size %llu!\n", u64BlkSize);
            return SAL_FAIL;
        }
    }

    VbBlk = RK_MPI_MB_GetMB(pool, u64BlkSize, RK_FALSE);
    if (MB_INVALID_POOLID == pool)
    {
        DUP_LOGE("RK_MPI_MB_CreatePool failed size %llu!\n", u64BlkSize);
        return SAL_FAIL;
    }

    p64VirAddr = (UINT64 *) RK_MPI_MB_Handle2VirAddr(VbBlk);
    if (NULL == p64VirAddr)
    {
        DUP_LOGE("RK_MPI_SYS_Mmap failed size %llu!\n", u64BlkSize);
        return SAL_FAIL;
    }

    pOutFrm->phyAddr = 0;
    pOutFrm->virAddr = p64VirAddr;
    pOutFrm->memSize = u64BlkSize;
    pOutFrm->poolId = pool;
    pOutFrm->vbBlk = (PhysAddr)VbBlk;

    return SAL_SOK;
}

/**
 * @function   vdec_rk_VdecMmap
 * @brief      帧映射
 * @param[in]  void *pVideoFrame 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm  映射地址
 * @param[out] None
 * @return    INT32
 */
INT32 vdec_rk_VdecMmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    CHECK_PTR_NULL(pVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    pstFrame = (VIDEO_FRAME_INFO_S *)pVideoFrame;

    if (pstFrame->stVFrame.pVirAddr[0] == 0)
    {
        pstFrame->stVFrame.pVirAddr[0] = RK_MPI_MB_Handle2VirAddr(pstFrame->stVFrame.pMbBlk);
        pstFrame->stVFrame.pVirAddr[1] = pstFrame->stVFrame.pVirAddr[0] + pstFrame->stVFrame.u32Width*pstFrame->stVFrame.u32Height;
    }

    pPicprm->width = pstFrame->stVFrame.u32Width;
    pPicprm->height = pstFrame->stVFrame.u32Height;
    pPicprm->stride = pstFrame->stVFrame.u32VirWidth;
    pPicprm->videoFormat = vdec_rk_TransPixFormat(pstFrame->stVFrame.enPixelFormat);
   // pPicprm->poolId = pstFrame->u32PoolId;
    pPicprm->addr[0] = (void *)pstFrame->stVFrame.pVirAddr[0];
    pPicprm->addr[1] = (void *)pstFrame->stVFrame.pVirAddr[1];
    pPicprm->phy[0] = (PhysAddr)pstFrame->stVFrame.pMbBlk;

    return SAL_SOK;
}

/**
 * @function   vdec_halRegister
 * @brief      函数注册
 * @param[in]
 * @param[out] None
 * @return    INT32
 */
VDEC_HAL_FUNC *vdec_halRegister()
{
    memset(&vdecFunc, 0, sizeof(VDEC_HAL_FUNC));

    vdecFunc.VdecCreate = vdec_rk_VdecCreate;
    vdecFunc.VdecDeinit = vdec_rk_VdecDeinit;
    vdecFunc.VdecClear = vdec_rk_VdecClear;
    vdecFunc.VdecSetPrm = vdec_rk_VdecSetPrm;
    vdecFunc.VdecStart = vdec_rk_VdecStart;
    vdecFunc.VdecStop = vdec_rk_VdecStop;
    vdecFunc.VdecDecframe = vdec_rk_VdecDecframe;
    vdecFunc.VdecGetframe = vdec_rk_VdecGetframe;
    vdecFunc.VdecReleaseframe = vdec_rk_VdecReleaseframe;
    vdecFunc.VdecCpyframe = vdec_rk_VdecCpyframe;
    vdecFunc.VdecFreeYuvPoolBlockMem = vdec_rk_FreeYuvPoolBlockMem;
    vdecFunc.VdecMallocYUVMem = vdec_rk_GetYuvPoolBlockMem;
    vdecFunc.VdecMakeframe = vdec_rk_VdecMakeframe;
    vdecFunc.VdecMmap = vdec_rk_VdecMmap;

    return &vdecFunc;
}

