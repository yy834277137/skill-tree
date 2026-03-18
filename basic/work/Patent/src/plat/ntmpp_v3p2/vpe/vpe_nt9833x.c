/**
 * @file   vpe_nt98336.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台 vpe 模块
 * @author yeyanzhong
 * @date   2021/12/07
 * @note
 * @note \n History
   1.日    期: 2021/12/07
     作    者: yeyanzhong
     修改历史: 创建文件
 */


#include <sal.h>
//#include <dspcommon.h>
#include <platform_sdk.h>
#include "capbility.h"

//#include "../include/hal_inc/dup_hal.h"
#include "dup_hal_inter.h"
#include "vpe_nt9833x.h"

#line __LINE__ "vpe_nt9833x.c"

#define VDOPROC_DEV                 1024    ///< first 512 for VPE, last 512 for VPE Lite
#define VODPROC_INPUT_PORT          1       ///< Indicate max input port value
#define VODPROC_OUTPUT_PORT         12      ///< Indicate max output port value
#define BUF_NUM                     5       ///< 缓存个数

static INT32 vpe_clearScaleOutBuf(UINT32 u32GrpId, UINT32 u32Chn);

static DUP_PLAT_OPS g_stDupPlatOps;

typedef enum 
{
    VPE_UNINITED = 0,
    VPE_CREATED,
    VPE_STARTED,
    VPE_STOPPED,
    VPE_DESTROYED,
    VPE_STATE_MAX,
} VPE_STATE_E;

typedef enum
{
    VPE_OUT_BUF_UNALLOCTED = 0,
    VPE_OUT_BUF_ALLOCTED,
    VPE_OUT_BUF_WRITING,
    VPE_OUT_BUF_VALID,          //数据有效
    VPE_OUT_BUF_USER_USED,
    VPE_OUT_BUF_USER_RELEASED,
} VPE_OUT_BUF_STATE_E;

typedef struct _VPE_SCALE_OUT_BUF_S
{
    HD_VIDEO_FRAME stVideoFrm[BUF_NUM];
    UINT64 u64VirAddr[BUF_NUM];
    VPE_OUT_BUF_STATE_E enBufState[BUF_NUM];
    UINT32 w;
    UINT32 r;
    pthread_mutex_t chnBufMutex;
} VPE_SCALE_OUT_BUF_S;

static VPE_SCALE_OUT_BUF_S stScaleOutBuf[VDOPROC_DEV][VODPROC_OUTPUT_PORT];



static HD_PATH_ID pathID[VDOPROC_DEV][VODPROC_OUTPUT_PORT];  /* video process module */
static VPE_STATE_E enVpeState[VDOPROC_DEV][VODPROC_OUTPUT_PORT];


static HD_VIDEOPROC_SCA_BUF_INFO sca_buf_info[VDOPROC_DEV][VODPROC_OUTPUT_PORT];
static ALLOC_VB_INFO_S gaScaVbBuf[VDOPROC_DEV][VODPROC_OUTPUT_PORT];



static INT32 setVpeOutSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_OUT out;
    memset(&out, 0, sizeof(HD_VIDEOPROC_OUT));

    /*加锁，设置过程中不可以继续送帧会导致数据粉色等异常*/
    pthread_mutex_lock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }


    out.rect.x = 0;
    out.rect.y = 0;
    out.rect.w = ALIGN_CEIL(u32W, 16);
    out.rect.h = u32H;
    out.bg.w = ALIGN_CEIL(u32W, 16); /* 这里需要用整屏的分辨率大小，这里用vpe创建时配置的1080P */ 
    out.bg.h = u32H;
    
    out.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
    out.dir = HD_VIDEO_DIR_NONE;
    ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) 
    {
        printf("hd_videoproc_set out fail\n");
    }
    pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));

    return SAL_SOK;
}
static INT32 getVpeOutSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_OUT out;
    memset(&out, 0, sizeof(HD_VIDEOPROC_OUT));

    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    *pU32W = out.rect.w;
    *pU32H = out.rect.h;

    return SAL_SOK;
}

static INT32 setVpeOutRect(UINT32 u32GrpId, UINT32 u32Chn, IMAGE_VALID_RECT_S *pStImageRect)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_OUT out;
    memset(&out, 0, sizeof(HD_VIDEOPROC_OUT));

    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }


    out.rect.x = pStImageRect->stRect.u32X;
    out.rect.y = pStImageRect->stRect.u32Y;
    out.rect.w = ALIGN_CEIL(pStImageRect->stRect.u32W, 16);
    out.rect.h = pStImageRect->stRect.u32H;
    out.bg.w = ALIGN_CEIL(pStImageRect->stBg.u32Width, 16);  
    out.bg.h = pStImageRect->stBg.u32Height;
    
    out.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
    out.dir = HD_VIDEO_DIR_NONE;
    ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }



    return SAL_SOK;
}

static INT32 setVpeInSize(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_IN proc_in;
    
    memset(&proc_in, 0, sizeof(HD_VIDEOPROC_IN));

    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_IN, (void *)&proc_in);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    if(pParamInfo->stImgSize.u32Format == HD_VIDEO_PXLFMT_YUV420_NVX4)
    {
        proc_in.pxlfmt = pParamInfo->stImgSize.u32Format;
    }
    else
    {
        proc_in.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
    }

    proc_in.dim.w = pParamInfo->stImgSize.u32Width;
    proc_in.dim.h = pParamInfo->stImgSize.u32Height;
    ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_IN, &proc_in);
    if (ret != HD_OK) {
        return SAL_FAIL;
    }

    return SAL_SOK;
}



static INT32 setVpeExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{

    DUP_LOGE("nt9833x vpe not support extChan \n");
    return SAL_SOK;
}
static INT32 getVpeExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    DUP_LOGE("nt9833x vpe not support extChan \n");
    return SAL_SOK;
}

static INT32 setVpeChnMirror(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32Mirror,UINT32 u32Flip)
{

    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_OUT out;
    memset(&out, 0, sizeof(HD_VIDEOPROC_OUT));

    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    out.dir &= (~HD_VIDEO_DIR_MIRRORX);
    out.dir |= ((u32Mirror  << 28) & HD_VIDEO_DIR_MIRRORX);
    
    out.dir &= (~HD_VIDEO_DIR_MIRRORY);
    out.dir |= ((u32Flip  << 29) & HD_VIDEO_DIR_MIRRORY);

    ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    return SAL_SOK;

}

static INT32 setVpeChnFps(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32Fps)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_OUT out;
    memset(&out, 0, sizeof(out));

    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    out.frc = HD_VIDEO_FRC_RATIO(u32Fps, 60); //fixme，这里源帧率假定60fps

    ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }


    return SAL_SOK;
}

static INT32 setVpeChnRotate(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32Rotate)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_OUT out;
    memset(&out, 0, sizeof(HD_VIDEOPROC_OUT));

    ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    out.dir &= (~HD_VIDEO_DIR_ROTATE_MASK);
    out.dir |= HD_VIDEO_DIR_ROTATE(u32Rotate * 90);
    
    ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        printf("hd_videoproc_set out fail\n");
    }

    return SAL_SOK;
}



static HD_IRECT SEC_CROP_RECT[VDOPROC_DEV][VODPROC_OUTPUT_PORT];

static INT32 setVpeChnCrop(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    HD_RESULT ret = HD_OK;
    HD_VIDEOPROC_CROP crop;
    memset(&crop, 0, sizeof(crop));

    /* 二次裁剪，主要用于做全局放大和局部放大 */
    if (CROP_SECOND == pParamInfo->stCrop.enCropType)
    {
        if (ENABLED == pParamInfo->stCrop.u32CropEnable)
        {
            SEC_CROP_RECT[u32GrpId][u32Chn].x = pParamInfo->stCrop.u32X;
            SEC_CROP_RECT[u32GrpId][u32Chn].y = pParamInfo->stCrop.u32Y;
            SEC_CROP_RECT[u32GrpId][u32Chn].w = pParamInfo->stCrop.u32W;
            SEC_CROP_RECT[u32GrpId][u32Chn].h = pParamInfo->stCrop.u32H;
            DUP_LOGD("second crop grp %d, chn %d, [x %d, y %d, w %d, h %d] \n", u32GrpId, u32Chn,\
                pParamInfo->stCrop.u32X, pParamInfo->stCrop.u32Y, pParamInfo->stCrop.u32W, pParamInfo->stCrop.u32H);
        }
        else
        {
            memset(&(SEC_CROP_RECT[u32GrpId][u32Chn]), 0, sizeof(SEC_CROP_RECT[0][0]));
        }
    }
    else /* 原始裁剪，主要是为从[1920x4, 896]大的缓冲区里裁剪出当前帧有效的数据[1920, 896]; 解码时也要用该裁剪，从1088裁剪到1080 */
    {
        /* 解码时也要用该裁剪，从1088裁剪到1080，没有处于STARTED状态的vpe pathID，也需要进行裁剪 */
        /*if (VPE_STARTED != enVpeState[u32GrpId][u32Chn])
        {
           // return SAL_FAIL;
        }*/
        ret = hd_videoproc_get(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_IN_CROP, (void *)&crop);
        if (ret != HD_OK) 
        {
            printf("hd_videoproc_set in crop fail\n");
            return SAL_FAIL;
        } 

        if (ENABLED == pParamInfo->stCrop.u32CropEnable)
        {
            /* set parameters of crop window */
            crop.mode = HD_CROP_ON;
            if ( pParamInfo->stCrop.u32X + SEC_CROP_RECT[u32GrpId][u32Chn].x + SEC_CROP_RECT[u32GrpId][u32Chn].w > pParamInfo->stCrop.sInImageSize.u32Width)
            {
                DUP_LOGW("crop prm err grp %d, chn %d,x %d+ w%d > srcW %d or y %d+ h%d > srcH %d\n", u32GrpId, u32Chn,\
                pParamInfo->stCrop.u32X + SEC_CROP_RECT[u32GrpId][u32Chn].x ,SEC_CROP_RECT[u32GrpId][u32Chn].w ,pParamInfo->stCrop.sInImageSize.u32Width,\
                pParamInfo->stCrop.u32Y + SEC_CROP_RECT[u32GrpId][u32Chn].y ,SEC_CROP_RECT[u32GrpId][u32Chn].h ,pParamInfo->stCrop.sInImageSize.u32Height);

                if (pParamInfo->stCrop.sInImageSize.u32Width > pParamInfo->stCrop.u32X + SEC_CROP_RECT[u32GrpId][u32Chn].x)
                {
                    SEC_CROP_RECT[u32GrpId][u32Chn].w = pParamInfo->stCrop.sInImageSize.u32Width - pParamInfo->stCrop.u32X - SEC_CROP_RECT[u32GrpId][u32Chn].x;
                }
                else
                {
                    SEC_CROP_RECT[u32GrpId][u32Chn].w = 0;
                }
            }
            if ( pParamInfo->stCrop.u32Y + SEC_CROP_RECT[u32GrpId][u32Chn].y + SEC_CROP_RECT[u32GrpId][u32Chn].h > pParamInfo->stCrop.sInImageSize.u32Height)
            {
                DUP_LOGW("crop prm err grp %d, chn %d,x %d+ w%d > srcW %d or y %d+ h%d > srcH %d\n", u32GrpId, u32Chn,\
                pParamInfo->stCrop.u32X + SEC_CROP_RECT[u32GrpId][u32Chn].x ,SEC_CROP_RECT[u32GrpId][u32Chn].w ,pParamInfo->stCrop.sInImageSize.u32Width,\
                pParamInfo->stCrop.u32Y + SEC_CROP_RECT[u32GrpId][u32Chn].y ,SEC_CROP_RECT[u32GrpId][u32Chn].h ,pParamInfo->stCrop.sInImageSize.u32Height);

                if (pParamInfo->stCrop.sInImageSize.u32Height > pParamInfo->stCrop.u32Y + SEC_CROP_RECT[u32GrpId][u32Chn].y)
                {
                    SEC_CROP_RECT[u32GrpId][u32Chn].h = pParamInfo->stCrop.sInImageSize.u32Height - pParamInfo->stCrop.u32Y - SEC_CROP_RECT[u32GrpId][u32Chn].y;
                }
                else
                {
                    SEC_CROP_RECT[u32GrpId][u32Chn].h = 0;
                }
            }
            crop.win.rect.x = pParamInfo->stCrop.u32X + SEC_CROP_RECT[u32GrpId][u32Chn].x;  
            crop.win.rect.y = pParamInfo->stCrop.u32Y + SEC_CROP_RECT[u32GrpId][u32Chn].y;
            crop.win.rect.w = (SEC_CROP_RECT[u32GrpId][u32Chn].w > 0) ? SEC_CROP_RECT[u32GrpId][u32Chn].w : pParamInfo->stCrop.u32W;  
            crop.win.rect.h = (SEC_CROP_RECT[u32GrpId][u32Chn].h > 0) ? SEC_CROP_RECT[u32GrpId][u32Chn].h : pParamInfo->stCrop.u32H; //to scaling up over 8x
            crop.win.rect.w = crop.win.rect.w > 64 ? crop.win.rect.w : 64;
            crop.win.rect.h = crop.win.rect.h > 64 ? crop.win.rect.h : 64;
            crop.win.coord.w =  pParamInfo->stCrop.sInImageSize.u32Width;  //fixme: 坐标的宽高还需要外面传进来？？？
            crop.win.coord.h = pParamInfo->stCrop.sInImageSize.u32Height;
            crop.win.rect.w = ALIGN_FLOOR(crop.win.rect.w, 8);
            crop.win.rect.h = ALIGN_FLOOR(crop.win.rect.h, 2);
            ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_IN_CROP, (void *)&crop);
            if (ret != HD_OK) {
                printf("hd_videoproc_set in crop fail\n");
                return SAL_FAIL;
            }

            /* vpe flow绑定模式设置裁剪参数之后不调start会导致vpe_push_in失败，trigger模式可以不调 */
            ret = hd_videoproc_start(pathID[u32GrpId][u32Chn]);
            if (ret != HD_OK) {
                printf("hd_videoproc_start fail\n");
                return SAL_FAIL;
            } 
        }
        else
        {
            crop.mode = HD_CROP_OFF;
            ret = hd_videoproc_set(pathID[u32GrpId][u32Chn], HD_VIDEOPROC_PARAM_IN_CROP, (void *)&crop);
            if (ret != HD_OK) {
                printf("hd_videoproc_set in crop fail\n");
                return SAL_FAIL;
            } 
        }

    }
    return SAL_SOK;

}

static INT32 setVpeGrpCrop(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    INT32 i = 0;

    /* 组裁剪：主要用于原始的[1920x4, 896]裁剪到[1920, 896]以及全局放大里使用 */
    for (i = 0; i < VODPROC_OUTPUT_PORT; i++)
    {
        if ((enVpeState[u32GrpId][i] >= VPE_STARTED) && (enVpeState[u32GrpId][i] <= VPE_STOPPED))
        {
            setVpeChnCrop(u32GrpId, i, pParamInfo);
        }
    }
    
    return SAL_SOK;

}

static INT32 vpe_nt9833x_getPhyChnNum()
{
    return VODPROC_OUTPUT_PORT;
}

static INT32 vpe_nt9833x_getExtChnNum()
{
    return 0;
}




/* vpe一进一出，所以grp x chn 不能超过vpe dev总个数1024 */
#define GRP_NUM 128
#define CHN_NUM 8

typedef struct _VPE_MNG_S
{
    UINT32 u32VpeDevId[GRP_NUM][CHN_NUM];  //这里一进一出时，一个chn映射到一个vpe dev_id
    VPE_MODE_E enVpeMode[GRP_NUM][CHN_NUM];
    HD_PATH_ID pathID[GRP_NUM][CHN_NUM];
    UINT64 u64VpeDevBeUsed[VDOPROC_DEV/64];
    pthread_mutex_t vpeMutex;
} VPE_MNG_S;

VPE_MNG_S g_stVpeMngInfo;

static INT32 vpe_getFreeDevId(UINT32 * pDevId)
{
    UINT32 i = 0, idx;
    UINT64 bit;

    pthread_mutex_lock(&(g_stVpeMngInfo.vpeMutex));

    for (i = 0; i < VDOPROC_DEV; i++)
    {
        idx = i / 64;
        bit = i % 64;
        /* 标记为1了，表示已被使用 */
        if (g_stVpeMngInfo.u64VpeDevBeUsed[idx] & (1<<bit))
        {
            continue;
        }
        else
        {
            /* 发现未使用的，返回通道号，并标记为已用 */
            g_stVpeMngInfo.u64VpeDevBeUsed[idx] = g_stVpeMngInfo.u64VpeDevBeUsed[idx] | (1 << bit);
            *pDevId = i;
            break;
        }
    }

    /* VDOPROC_DEV 个都被使用了，表示无可用的，返回错误
     */
    if (i == VDOPROC_DEV)
    {
        *pDevId = 0;
        pthread_mutex_unlock(&(g_stVpeMngInfo.vpeMutex));
        return SAL_FAIL;
    }

    pthread_mutex_unlock(&(g_stVpeMngInfo.vpeMutex));

    DUP_LOGI("get vpe devId  %u!\n", *pDevId);

    return SAL_SOK;
}

static INT32 vpe_releaseDevId(UINT32 u32VpeDevId)
{
    UINT32 idx;
    UINT64 bit;

    DUP_LOGI("free vpe devId   %u !\n", u32VpeDevId);
    if (u32VpeDevId >= VDOPROC_DEV)
    {
        DUP_LOGE("put free vpe    dev id   %u is wrong!\n", u32VpeDevId);
        return SAL_FAIL;
    }
    pthread_mutex_lock(&(g_stVpeMngInfo.vpeMutex));
    
    idx = u32VpeDevId / 64;
    bit = u32VpeDevId % 64;
    g_stVpeMngInfo.u64VpeDevBeUsed[idx] = g_stVpeMngInfo.u64VpeDevBeUsed[idx] & ~(1 << bit);

    pthread_mutex_unlock(&(g_stVpeMngInfo.vpeMutex));

    return SAL_SOK;
}

UINT32 vpe_nt9833x_getVpeDevId(UINT32 u32GrpId, UINT32 u32Chn)
{
    if ((u32GrpId >= GRP_NUM) || (u32Chn >= CHN_NUM))
    {
        DUP_LOGE("u32GrpId %d, u32Chn      %d is invalid!\n", u32GrpId, u32Chn);
        return SAL_FAIL;
    }

    return g_stVpeMngInfo.u32VpeDevId[u32GrpId][u32Chn];
}

INT32 vpe_nt9833x_setVpeMode(UINT32 u32GrpId, UINT32 u32Chn, VPE_MODE_E enVpeMode)
{
    if ((u32GrpId >= GRP_NUM) || (u32Chn >= CHN_NUM))
    {
        DUP_LOGE("u32GrpId %d, u32Chn      %d is invalid!\n", u32GrpId, u32Chn);
        return SAL_FAIL;
    }

    g_stVpeMngInfo.enVpeMode[u32GrpId][u32Chn] = enVpeMode;

    return SAL_SOK;
}

static INT32 vpe_nt9833x_getVpeMode(UINT32 u32GrpId, UINT32 u32Chn, VPE_MODE_E *pEnVpeMode)
{
    if ((u32GrpId >= GRP_NUM) || (u32Chn >= CHN_NUM))
    {
        DUP_LOGE("u32GrpId %d, u32Chn      %d is invalid!\n", u32GrpId, u32Chn);
        return SAL_FAIL;
    }

    *pEnVpeMode = g_stVpeMngInfo.enVpeMode[u32GrpId][u32Chn];

    return SAL_SOK;
}

static INT32 vpe_nt9833x_create(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm, DUP_HAL_CHN_INIT_ATTR *pstChnInitAttr)
{

    HD_RESULT ret = HD_OK;
    INT32 s32GrpId, i;
    HD_VIDEOPROC_DEV_CONFIG vpe_config;
    HD_VIDEOPROC_IN proc_in;
    HD_VIDEOPROC_OUT proc_out;
    
    UINT32 u32OutChnNum = pstHalVpssGrpPrm->uiPhyChnNum;
    s32GrpId = pstHalVpssGrpPrm->uiVpssChn;

    INT64 s32Ret = 0;
    //UINT64 u64PhyAddr = 0;
    //UINT64 *p64VirAddr = NULL;
    UINT64 u64BlkSize = (UINT64)((UINT64)SAL_align(pstHalVpssGrpPrm->uiGrpWidth, 32) * SAL_align(pstHalVpssGrpPrm->uiGrpHeight, 16) * 3 / 2);
    UINT32 u32DevId = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};

    for (i = 0; i < u32OutChnNum; i++) 
    {
#if 0        /* 一进多出方式 */
        if(0)
        {
            if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(s32GrpId, 0), HD_VIDEOPROC_OUT(s32GrpId, i), &pathID[s32GrpId][i])) != HD_OK) 
            {
                DUP_LOGE("hd_videoproc_open failed  !\n");
                return SAL_FAIL;            
            }
        }
        else
#endif
        {
            /* 一进一出方式*/
            if (SAL_FAIL == vpe_getFreeDevId(&u32DevId))
            {
                DUP_LOGE("vpe_getFreeDevId failed  !\n");
                return SAL_FAIL;            
            }
            if ((ret = hd_videoproc_open(HD_VIDEOPROC_IN(u32DevId, 0), HD_VIDEOPROC_OUT(u32DevId, 0), &pathID[s32GrpId][i])) != HD_OK)
            {
                return ret;
            } 
            g_stVpeMngInfo.u32VpeDevId[s32GrpId][i] = u32DevId;
        }
/*
        s32Ret = mem_hal_vbAlloc(u64BlkSize * 2, "VPE", "vpe_scale", NULL, SAL_FALSE, &u64PhyAddr, &p64VirAddr);
        if (SAL_FAIL == s32Ret)
        {
            DISP_LOGE("mem_hal_vbAlloc failed size %llu !\n", u64BlkSize);
            return SAL_FAIL;
        }
        stDispBuffer[s32GrpId][i].phy_addr[0] = u64PhyAddr;
        stDispBuffer[s32GrpId][i].blk = mem_hal_getVbBlk(p64VirAddr);
        stDispBuffer[s32GrpId][i].ddr_id = (u64PhyAddr < DDR1_ADDR_START)? DDR_ID0 : DDR_ID1;
        u64DispVirAddr[s32GrpId][i] = (UINT64)p64VirAddr;
*/

#if 1
        /* trigger mode时 sample里推荐使用sca work buffer，缩放倍数超过8倍时需要该缩放缓冲区 */
        s32Ret = mem_hal_vbAlloc(u64BlkSize * 2, "VPE", "vpe_scale", NULL, SAL_FALSE, &stVbInfo);
        if (SAL_FAIL == s32Ret)
        {
            DISP_LOGE("mem_hal_vbAlloc failed size %llu !\n", u64BlkSize);
            return SAL_FAIL;
        }        
        memset(&sca_buf_info[s32GrpId][i], 0x0, sizeof(HD_VIDEOPROC_SCA_BUF_INFO));
        sca_buf_info[s32GrpId][i].ddr_id = (stVbInfo.u64PhysAddr < DDR1_ADDR_START)? DDR_ID0 : DDR_ID1;
        sca_buf_info[s32GrpId][i].pbuf_addr = stVbInfo.u64PhysAddr;
        sca_buf_info[s32GrpId][i].pbuf_size = u64BlkSize * 2;
        //u64ScaBufVirAddr[s32GrpId][i] = (UINT64)p64VirAddr;
        //gaScaVbBuf[s32GrpId][i].pVirAddr = stVbInfo.pVirAddr;
        //gaScaVbBuf[s32GrpId][i].u64VbBlk = stVbInfo.u64VbBlk;
        memcpy(&(gaScaVbBuf[s32GrpId][i]), &stVbInfo, sizeof(stVbInfo)) ;
        
        ret = hd_videoproc_set(pathID[s32GrpId][i], HD_VIDEOPROC_PARAM_SCA_WK_BUF, (void *)&sca_buf_info);
        if (ret != HD_OK) {
            printf("hd_videoproc_set work buffer fail\n");
            return SAL_FAIL;
        }
#endif
#if 1


                /* Set videoprocess out pool */
        memset(&vpe_config, 0x0, sizeof(HD_VIDEOPROC_DEV_CONFIG));
        vpe_config.data_pool[0].mode = HD_VIDEOPROC_POOL_ENABLE;
        vpe_config.data_pool[0].ddr_id = DDR_ID1;  //fixme ?
        vpe_config.data_pool[0].counts = HD_VIDEOPROC_SET_COUNT(3, 0);
        vpe_config.data_pool[0].max_counts = HD_VIDEOPROC_SET_COUNT(3, 0);

        vpe_config.data_pool[1].mode = HD_VIDEOPROC_POOL_DISABLE;
        vpe_config.data_pool[2].mode = HD_VIDEOPROC_POOL_DISABLE;
        vpe_config.data_pool[3].mode = HD_VIDEOPROC_POOL_DISABLE;

        ret = hd_videoproc_set(pathID[s32GrpId][i], HD_VIDEOPROC_PARAM_DEV_CONFIG, &vpe_config);
        if (ret != HD_OK) {
            return SAL_FAIL;
        }
#endif

        /* fixme 送入vpe的总的缓冲区大小 */
        proc_in.dim.w = pstHalVpssGrpPrm->uiGrpWidth; //1920 *4;  ipc解码时需要配置成1080P，安检裁剪不配成总的缓冲区的宽度1920 *4也能正常使用
        proc_in.dim.h = pstHalVpssGrpPrm->uiGrpHeight;  //896
        proc_in.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
        ret = hd_videoproc_set(pathID[s32GrpId][i], HD_VIDEOPROC_PARAM_IN, &proc_in);
        if (ret != HD_OK) {
            return SAL_FAIL;
        }

        /* Set videoproc output setting */
        proc_out.rect.x = 0;
        proc_out.rect.y = 0;
        proc_out.rect.w = pstChnInitAttr->stOutChnAttr[i].uiOutChnW;
        proc_out.rect.h = pstChnInitAttr->stOutChnAttr[i].uiOutChnH;
        proc_out.bg.w = pstChnInitAttr->stOutChnAttr[i].uiOutChnW;
        proc_out.bg.h = pstChnInitAttr->stOutChnAttr[i].uiOutChnH;
        
        proc_out.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
        proc_out.dir = HD_VIDEO_DIR_NONE;
        ret = hd_videoproc_set(pathID[s32GrpId][i], HD_VIDEOPROC_PARAM_OUT, &proc_out);
        if (ret != HD_OK) 
        {
            return SAL_FAIL;
        }

        enVpeState[s32GrpId][i] = VPE_CREATED;

    }


    return SAL_SOK;

}

static INT32 vpe_nt9833x_start(UINT32 u32GrpId, UINT32 u32Chn)
{
    HD_RESULT ret = HD_OK;

    if (enVpeState[u32GrpId][u32Chn] != VPE_STARTED)
    {
#ifndef USE_PUSH
        ret = hd_videoproc_start(pathID[u32GrpId][u32Chn]);
        if (ret != HD_OK) {
            DUP_LOGW("hd_videoproc_start fail, ret %d, u32GrpId %d, u32Chn %d\n", ret, u32GrpId, u32Chn);
           // return SAL_FAIL;  // trigger模式时SDK不需要调start，但vpe软件状态需要置为VPE_STARTED，这样send时才会往vpe里送；
        }
#endif
        /* 通道重新开启时清一下缓存，因为可能有残留数据。不放在stop里是因为有可能在清缓存时生产者在写缓存 */
        vpe_clearScaleOutBuf(u32GrpId, u32Chn); 

        DUP_LOGI("hd_videoproc_start ok, u32GrpId %d, u32Chn %d\n", u32GrpId, u32Chn);
        enVpeState[u32GrpId][u32Chn] = VPE_STARTED;
    }
    return ret;
}

static INT32 vpe_nt9833x_stop(UINT32 u32GrpId, UINT32 u32Chn)
{
    HD_RESULT ret = HD_OK;

    if(enVpeState[u32GrpId][u32Chn] != VPE_STOPPED)
    {
        ret = hd_videoproc_stop(pathID[u32GrpId][u32Chn]);
        if (ret != HD_OK) {
            printf("hd_videoproc_start fail, ret %d\n", ret);
            //return SAL_FAIL; // trigger模式时SDK不需要调stop，但vpe软件状态需要置为VPE_STOPPED，所以sdk出错时继续往下走。
        }

        enVpeState[u32GrpId][u32Chn] = VPE_STOPPED;
    }

    return SAL_SOK;
}
static INT32 vpe_nt9833x_destroy(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm)
{

    HD_RESULT ret = HD_OK;
    INT32 s32GrpId, i=0;
    INT32 s32Ret = 0;
    //HD_VIDEOPROC_DEV_CONFIG vpe_config;
    //HD_VIDEOPROC_IN proc_in;
    //HD_VIDEOPROC_OUT proc_out;
    UINT32 u32VpeDevId;
    UINT32 u32OutChnNum = pstHalVpssGrpPrm->uiPhyChnNum;
    s32GrpId = pstHalVpssGrpPrm->uiVpssChn;
    u32VpeDevId = g_stVpeMngInfo.u32VpeDevId[s32GrpId][i];

    for (i = 0; i < u32OutChnNum; i++) 
    {
        if (VPE_DESTROYED == enVpeState[s32GrpId][i])
        {
            DUP_LOGW("vpe grp_%d chn_%d has been closed \n", s32GrpId, i);
            continue;
        }

        pthread_mutex_lock(&(stScaleOutBuf[s32GrpId][i].chnBufMutex));
        if ((ret = hd_videoproc_close(pathID[s32GrpId][i])) != HD_OK)
        {
            DUP_LOGE("hd_videoproc_close fail, ret %d\n", ret);
            pthread_mutex_unlock(&(stScaleOutBuf[s32GrpId][i].chnBufMutex));
            return SAL_FAIL;
        }
        pthread_mutex_unlock(&(stScaleOutBuf[s32GrpId][i].chnBufMutex));

        u32VpeDevId = g_stVpeMngInfo.u32VpeDevId[s32GrpId][i];
        vpe_releaseDevId(u32VpeDevId);
        g_stVpeMngInfo.u32VpeDevId[s32GrpId][i] = (UINT32)(-1);
        pathID[s32GrpId][i] = (UINT32)(-1);
        enVpeState[s32GrpId][i] = VPE_DESTROYED;

        /* 释放缩放缓冲区，缩放倍数超过8倍时需要该缩放缓冲区 */
        s32Ret = mem_hal_vbFree((Ptr)gaScaVbBuf[s32GrpId][i].pVirAddr, "VPE", "vpe_scale", gaScaVbBuf[s32GrpId][i].u32Size, gaScaVbBuf[s32GrpId][i].u64VbBlk, gaScaVbBuf[s32GrpId][i].u32PoolId);
        if (SAL_FAIL == s32Ret)
        {
            DISP_LOGE("mem_hal_vbFree failed size grp %u  , chn %u!\n", s32GrpId, i);
            return SAL_FAIL;
        }
      
    }



    return SAL_SOK;

}

static INT32 vpe_nt9833x_setParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    CFG_TYPE_E enCfgType = 0;
    INT32 s32Ret = 0;

    if (NULL == pParamInfo)
    {
         DUP_LOGE("pParamInfo is null!!!\n");
         return SAL_FAIL;
    }

    enCfgType = pParamInfo->enType;
    if (enCfgType <= MIN_CFG || enCfgType >= MAX_CFG)
    {
        DUP_LOGE("s32CfgType is %d, illgal \n", enCfgType);
        return SAL_FAIL;
    }

    switch (enCfgType)
    {
        case IMAGE_SIZE_CFG:
            s32Ret = setVpeOutSize(u32GrpId, u32Chn, pParamInfo->stImgSize.u32Width, pParamInfo->stImgSize.u32Height);
            break;
        case IMAGE_VALID_RECT_CFG:
            s32Ret = setVpeOutRect(u32GrpId, u32Chn, &(pParamInfo->stImgValidRect));
            break;
        case GRP_SIZE_IN_CFG: 
            s32Ret = setVpeInSize(u32GrpId, u32Chn, pParamInfo);
            break;
        case IMAGE_SIZE_EXT_CFG:
            s32Ret = setVpeExtChnSize(u32GrpId, u32Chn, pParamInfo->stImgSize.u32Width, pParamInfo->stImgSize.u32Height);
            break;        
        
        case MIRROR_CFG:
            s32Ret = setVpeChnMirror(u32GrpId, u32Chn, pParamInfo->stMirror.u32Mirror, pParamInfo->stMirror.u32Flip);
            break;
            
        case FPS_CFG:
            s32Ret = setVpeChnFps(u32GrpId, u32Chn, pParamInfo->u32Fps);
            break;
            
        case ROTATE_CFG:
            s32Ret = setVpeChnRotate(u32GrpId, u32Chn, pParamInfo->u32Rotate);
            break;
    
        case CHN_CROP_CFG:
            s32Ret = setVpeChnCrop(u32GrpId, u32Chn, pParamInfo);
                break;
        case GRP_CROP_CFG:
            s32Ret = setVpeGrpCrop(u32GrpId, u32Chn, pParamInfo);            
                break;
        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}

static INT32 vpe_nt9833x_getParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    CFG_TYPE_E enCfgType = 0;
    INT32 s32Ret = 0;

    if (NULL == pParamInfo)
    {
         DUP_LOGE("pParamInfo is null!!!\n");
         return SAL_FAIL;
    }

    enCfgType = pParamInfo->enType;
    if (enCfgType <= MIN_CFG || enCfgType >= MAX_CFG)
    {
        DUP_LOGE("s32CfgType is %d, illgal \n", enCfgType);
        return SAL_FAIL;
    }

    switch (enCfgType)
    {
        case IMAGE_SIZE_CFG:
            s32Ret = getVpeOutSize(u32GrpId, u32Chn, &(pParamInfo->stImgSize.u32Width), &(pParamInfo->stImgSize.u32Height));
            break;
        case IMAGE_SIZE_EXT_CFG:
            s32Ret = getVpeExtChnSize(u32GrpId, u32Chn, &(pParamInfo->stImgSize.u32Width), &(pParamInfo->stImgSize.u32Height));
            break;

        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}

static void save_output(char *filename, void *data, int size)
{
	FILE *pfile;

	/* Open output file */
	if ((pfile = fopen(filename, "wb")) == NULL) {
		printf("[ERROR] Open File %s failed!!\n", filename);
		exit(1);
	}

	/* Write buffer to output file */
	fwrite(data, 1, size, pfile);
	fclose(pfile);
	printf("Write file: %s\n", filename);
}

/* 获取上层没有在使用的buf */
static INT32 vpe_getUnusedScaleOutBuf(UINT32 u32GrpId, UINT32 u32Chn)
{
    UINT32 w, i;
    INT32 idx = 0;

    pthread_mutex_lock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));

    w =  stScaleOutBuf[u32GrpId][u32Chn].w;

    for(i = w; i < BUF_NUM + w; i++)
    {
        idx = i % BUF_NUM;
        if (stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] != VPE_OUT_BUF_USER_USED)
        {
            break;
        }

    }
    if(i == (BUF_NUM + w))
    {
        DUP_LOGE("get unused scale out buf fail\n");
        pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
        return -1;
    }
    
    stScaleOutBuf[u32GrpId][u32Chn].w = idx; //写完之后更新写指针为w+1

    /* 这里在sendFrame接口里将要写该缓冲区，所以把状态改为ALLOCTED，一方面为了避免被上层获取该buff，另一方面告诉sendFrame里重新分配buf */
    if (stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] != VPE_OUT_BUF_UNALLOCTED)
    {
        stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] = VPE_OUT_BUF_ALLOCTED;
    }
    pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
    return idx;
}

static INT32 vpe_getValidScaleOutBuf(UINT32 u32GrpId, UINT32 u32Chn)
{
    UINT32 r, i;
    INT32 idx = 0;

    pthread_mutex_lock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));

    r =  stScaleOutBuf[u32GrpId][u32Chn].r;

    for(i = r; i < BUF_NUM + r; i++)
    {
        idx = i % BUF_NUM;
        /* 读写指针相等时，表示没有新的数据可读 */
        if (idx ==  stScaleOutBuf[u32GrpId][u32Chn].w)
        {
            stScaleOutBuf[u32GrpId][u32Chn].r = idx;
            DUP_LOGD(" Grp %u,  chn %u ,there is no new data, idx %d \n", u32GrpId, u32Chn, idx);
            pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
            return -1;
        }
        if (VPE_OUT_BUF_USER_USED == stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] )
        {
            DUP_LOGE(" Grp %u,  chn %u ,scale  OutBuf has been      get by others, idx %d \n", u32GrpId, u32Chn, idx);
            pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
            return -1;
        }
        if (VPE_OUT_BUF_VALID == stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] )
        {
            break;
        }

    }
    // 加锁 ?
    stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] = VPE_OUT_BUF_USER_USED;
    stScaleOutBuf[u32GrpId][u32Chn].r = idx; //在上层读取完之后调release时 对r加1？
    
    pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
    return idx;
}

/*清缓存。在通道关闭之后可能还有没被上层取走的数据，所以在下次开启时清一下 */
static INT32 vpe_clearScaleOutBuf(UINT32 u32GrpId, UINT32 u32Chn)
{
    INT32 idx = 0;

    pthread_mutex_lock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));

    /* 把缓冲区里原先残留的数据清掉，即置为无效；读写指针也清为0 */
    for(idx = 0; idx < BUF_NUM; idx++)
    {
        if ( VPE_OUT_BUF_VALID == stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] )
        {
            stScaleOutBuf[u32GrpId][u32Chn].enBufState[idx] = VPE_OUT_BUF_USER_RELEASED;
        }

    }
    stScaleOutBuf[u32GrpId][u32Chn].w = 0;
    stScaleOutBuf[u32GrpId][u32Chn].r = 0;
    
    pthread_mutex_unlock(&(stScaleOutBuf[u32GrpId][u32Chn].chnBufMutex));
    
    return SAL_SOK;
}


static INT32 vpe_nt9833x_getFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    static UINT32 buf_idx[VDOPROC_DEV][VODPROC_OUTPUT_PORT];
    INT32 s32Idx = 0;
    char filename[64];
    UINT32 u32TryTimes = 20, j = 0;
    UINT32 u32SleepUintUs = 1000; //每次睡眠的时间，单位us
    HD_VIDEO_FRAME *pstVideoFrame = NULL;
 
    if (!pstFrame)
    {
        DUP_LOGE("SYSTEM_FRAME_INFO is NULL \n");
        return SAL_FAIL;
    }
    pstVideoFrame = (HD_VIDEO_FRAME *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

#if 0
    /* NT常用做法是send的时候就需要把scale_out_buffer带进去，所以这里pull out时可用send时的scale_out_buffer；也可把外部传进来的pstVideoFrame送进去 */
    ret = hd_videoproc_pull_out_buf(pathID[u32GrpId][u32Chn], &(scale_out_buffer[u32GrpId][u32Chn]), 2000);
    if (HD_OK != ret)
    {
        DUP_LOGE("Vpss %d Get vpss Chn %d Frame Failed, %x !!!\n", u32GrpId, u32Chn, ret);
        return SAL_FAIL;
    }
#endif

    u32TryTimes =  u32TimeoutMs * 1000 / u32SleepUintUs + 1; //超时时间为0时，只尝试获取一次
    for(j = 0; j < u32TryTimes; j++)
    {
        s32Idx = vpe_getValidScaleOutBuf(u32GrpId, u32Chn);
        if (s32Idx < 0 || s32Idx >= BUF_NUM)
        {
            usleep(u32SleepUintUs);
            continue;
        }
        else
        {
            break;
        }
    }
    if (j == u32TryTimes)
    {
        DUP_LOGE("get valid scale OutBuf timeout, idx %d \n", s32Idx);
        return SAL_FAIL;
    }

    memcpy((void *)pstFrame->uiAppData, (void *)&(stScaleOutBuf[u32GrpId][u32Chn].stVideoFrm[s32Idx]), sizeof(HD_VIDEO_FRAME));
    buf_idx[u32GrpId][u32Chn]++;

    if ((buf_idx[u32GrpId][u32Chn] % 100) == 0 && 0)
    {
        DUP_LOGI("Vpss Grp %d Chn %d get Frame, w %u, h %u, idx %d , count %d !!!\n", u32GrpId, u32Chn, pstVideoFrame->dim.w, pstVideoFrame->dim.h, s32Idx, buf_idx[u32GrpId][u32Chn]);
    }

    
    pstFrame->uiDataAddr = stScaleOutBuf[u32GrpId][u32Chn].u64VirAddr[s32Idx];

    pstFrame->uiDataWidth = pstVideoFrame->dim.w;
    pstFrame->uiDataHeight = pstVideoFrame->dim.h;


    if (0 && ((buf_idx[u32GrpId][u32Chn] % 100)== 0) && (buf_idx[u32GrpId][u32Chn] < 600))
    {
        sprintf(filename, "vpe_getFrm_chn%d_idx%d%dx%d_YUV420.yuv", u32Chn, buf_idx[u32GrpId][u32Chn], pstFrame->uiDataWidth, pstFrame->uiDataHeight);
        save_output(filename, (void *)pstFrame->uiDataAddr,  pstFrame->uiDataWidth*pstFrame->uiDataHeight * 3/2);
    }

    return SAL_SOK;

}

static INT32 vpe_nt9833x_releaseFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame)
{
    //HD_RESULT iRet = 0;
    HD_VIDEO_FRAME *pstVideoFrame = NULL;
    //UINT32 u32Size = 0;
    //INT32 s32Ret;
    //return SAL_SOK;
    UINT32 r;

    if (!pstFrame)
    {
        return SAL_FAIL;
    }
    pstVideoFrame = (HD_VIDEO_FRAME *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

#if 0
    u32Size = pstVideoFrame->dim.w * pstVideoFrame->dim.h * 3/2;

    /*获取vpss数据的时候mmap相应的释放的时候虚拟地址需要释放*/
    iRet = hd_common_mem_munmap((void *)pstFrame->uiDataAddr, u32Size);
    if (HD_OK != iRet)
    {
        DUP_LOGE("Vpss %d  vpss Chn %d ss_mpi_sys_munmap Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }
#endif

    r  = stScaleOutBuf[u32GrpId][u32Chn].r;

/*    
    s32Ret = mem_hal_vbFree((Ptr)(stScaleOutBuf[u32GrpId][u32Chn].u64VirAddr[r]), "VPE", "vpe_scale");
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("mem_hal_vbFree failed size grp %u  , chn %u!\n", u32GrpId, u32Chn);
        return SAL_FAIL;
    }
*/
    stScaleOutBuf[u32GrpId][u32Chn].enBufState[r] = VPE_OUT_BUF_USER_RELEASED;
    stScaleOutBuf[u32GrpId][u32Chn].r = (r + 1) % BUF_NUM;
    
#if 0
    /* trigger模式不需要调release_out_buf；flow mode时，也只需要在最后停止时调pull_out_buf/release_out_buf */
    iRet = hd_videoproc_release_out_buf(pathID[u32GrpId][u32Chn], pstVideoFrame);
    if (HD_OK != iRet)
    {
        DUP_LOGE("Vpss %d release vpss Chn %d Frame Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }
#endif
    return SAL_SOK;

}

#define USE_PUSH 
static INT32 vpe_nt9833x_sendFrame(UINT32 s32GrpId, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    HD_RESULT ret = HD_OK;
    UINT32 i = 0;    
#ifndef USE_PUSH
    UINT32 u32EnabledNum = 0;
    HD_VIDEOPROC_SEND_LIST video_bs[VODPROC_OUTPUT_PORT];
#endif

#ifdef USE_PUSH

    UINT64 time1, time2, time3;
    UINT32 ysize = 0, uvsize = 0;
    UINT8 *pu8Y = NULL, *pu8UV = NULL;
    INT32 s32Ret;

    static UINT32 count = 0;
    static UINT32 buf_idx[VDOPROC_DEV][VODPROC_OUTPUT_PORT];
    HD_VIDEO_FRAME *pstVideoFrm = NULL;
    INT32 idx = 0;
    VPE_MODE_E enVpeMode = VPE_TRIGGER_MODE;
    count++;
    ALLOC_VB_INFO_S stVbInfo = {0};

    for(i = 0; i < VODPROC_OUTPUT_PORT; i++)
    {

        if((pathID[s32GrpId][i] != (UINT32)(-1)) && (VPE_STARTED == enVpeState[s32GrpId][i])) 
        {
            idx = vpe_getUnusedScaleOutBuf(s32GrpId, i);
            if (idx < 0 || idx >= BUF_NUM)
            {
                DUP_LOGE(" get unused scaleOutBuf failed, %d, will not push in vpe\n", idx);
                continue;
            }
          
            /* 每次push in vpe操作时联咏研发反馈不需要重新申请scaleOutBuf */
            if (stScaleOutBuf[s32GrpId][i].enBufState[idx] == VPE_OUT_BUF_UNALLOCTED)
            {
                s32Ret = mem_hal_vbAlloc(1920*1080 * 3/2, "VPE", "vpe_scale", NULL, SAL_FALSE, &stVbInfo);
                if (SAL_FAIL == s32Ret)
                {
                    DISP_LOGE("mem_hal_vbAlloc failed size !\n");
                    return SAL_FAIL;
                }
                stScaleOutBuf[s32GrpId][i].stVideoFrm[idx].phy_addr[0] = stVbInfo.u64PhysAddr;
                stScaleOutBuf[s32GrpId][i].stVideoFrm[idx].blk = stVbInfo.u64VbBlk;
                stScaleOutBuf[s32GrpId][i].stVideoFrm[idx].ddr_id = (stVbInfo.u64PhysAddr < DDR1_ADDR_START)? DDR_ID0 : DDR_ID1;
                stScaleOutBuf[s32GrpId][i].u64VirAddr[idx] = (UINT64)stVbInfo.pVirAddr;
                stScaleOutBuf[s32GrpId][i].enBufState[idx] = VPE_OUT_BUF_ALLOCTED;

                /*背景置白*/
                ysize = 1920 *1080;
                uvsize = 1920 * 1080 / 2;
                pu8Y = (UINT8 *)stVbInfo.pVirAddr;
                pu8UV = (UINT8 *)stVbInfo.pVirAddr + ysize;
                memset(pu8Y, 0xEB, ysize);
                memset(pu8UV, 0x80, uvsize);
                DUP_LOGW(" vpe_nt9833x_sendFrame idx %d, grp %d, ch %d \n", idx, s32GrpId, i);
            }

            pstVideoFrm =  &(stScaleOutBuf[s32GrpId][i].stVideoFrm[idx]);
            time1= SAL_getCurUs();
            
            pthread_mutex_lock(&(stScaleOutBuf[s32GrpId][i].chnBufMutex));
            if ((ret = hd_videoproc_push_in_buf(pathID[s32GrpId][i], (HD_VIDEO_FRAME *)pstFrame->uiAppData, pstVideoFrm, 500)) < 0)
            {
                DUP_LOGE(" hd_videoproc_push_in_buf fail(%d), grp %d, ch %d             ! \n", ret, s32GrpId, i);
            }

            time2= SAL_getCurUs();

            vpe_nt9833x_getVpeMode(s32GrpId, i, &enVpeMode);
            /* 非绑定模式时，需要在同一个线程里立马pull out出来，pull out不同线程vpe容易发生错误并且难恢复 */
            if (enVpeMode != VPE_FLOW_MODE)
            {
                ret = hd_videoproc_pull_out_buf(pathID[s32GrpId][i], pstVideoFrm, 500);
                if (HD_OK != ret)
                {
                    DUP_LOGE("pull_out_buf Failed grp %d      Chn %d  , %x !!!\n", s32GrpId, i, ret);   
                    pthread_mutex_unlock(&(stScaleOutBuf[s32GrpId][i].chnBufMutex));
                    return SAL_FAIL;
                }
                stScaleOutBuf[s32GrpId][i].enBufState[idx] = VPE_OUT_BUF_VALID;
            }
            pthread_mutex_unlock(&(stScaleOutBuf[s32GrpId][i].chnBufMutex));
            
            stScaleOutBuf[s32GrpId][i].w = (stScaleOutBuf[s32GrpId][i].w + 1) % BUF_NUM;
            
            time3 = SAL_getCurUs();


            /* 调试接口耗时，后面可以之后删除 */
            buf_idx[s32GrpId][i]++;
            if (((buf_idx[s32GrpId][i] % 500) == 0) && 1)
            {
                DUP_LOGD("push in vpe %llu us,   pull out %llu us, grp: %d, ch %d, idx %d, count %d \n", time2-time1, time3-time2, s32GrpId, i, idx, buf_idx[s32GrpId][i]);
            }
        }

     }
#else
    static UINT32 count = 0;
    UINT32 time0, time1;
    count++;
    for(i = 0; i < VODPROC_OUTPUT_PORT; i++)
    {
        if((pathID[s32GrpId][i] != (UINT32)(-1)) && (VPE_STARTED == enVpeState[s32GrpId][i])) 
        {
            video_bs[u32EnabledNum].path_id = pathID[s32GrpId][i];
            video_bs[u32EnabledNum].user_bs.p_bs_buf = (CHAR *)pstFrame->uiDataAddr;
            /* dim: 7680 x 896 */
            video_bs[u32EnabledNum].user_bs.bs_buf_size = ((HD_VIDEO_FRAME *)pstFrame->uiAppData)->dim.w * ((HD_VIDEO_FRAME *)pstFrame->uiAppData)->dim.h * 3/2;
            video_bs[u32EnabledNum].user_bs.time_align = HD_VIDEODEC_TIME_ALIGN_USER;
            video_bs[u32EnabledNum].user_bs.time_diff = 10 * 1000; //fixme
            u32EnabledNum++;            
        }
        
    }
    time0 = SAL_getCurMs();
    if ((ret = hd_videoproc_send_list(video_bs, u32EnabledNum, 2000)) < 0) {
        DUP_LOGE("<send bitstream fail(%d), grp %d,         u32EnabledNum %u!>\n", ret, s32GrpId, u32EnabledNum);
    }
    time1 = SAL_getCurMs();
    if (((count % 300)>= 0) && ((count % 300)<= 5))
    {
        DUP_LOGE("vpe push in buf spent %d ms, u32EnabledNum %d \n", time1-time0, u32EnabledNum);
    }
#endif

    return SAL_SOK;
}

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
static INT32 vpe_nt9833x_scaleYuvRange(SAL_VideoSize *pstSrcSize, SAL_VideoSize *pstDstSize, 
                                       SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm, 
                                       CHAR *pu8Src, CHAR *pu8Dst, BOOL bIfMirror)
{
    HD_RESULT ret = HD_OK;
    UINT32 raw_frame_size = 0;
    const UINT32 u32YuvWidthMax = 2048, u32YuvHeightMax = 1280;
    HD_PATH_ID path_id;
    HD_COMMON_MEM_POOL_TYPE pool = HD_COMMON_MEM_COMMON_POOL;

    HD_VIDEOPROC_CROP crop = {0};
    HD_VIDEOPROC_OUT out = {0};
    VENDOR_VIDEO_PARAM_YUV_RANGE stYuvRangeParam = {0};

    static UINT32 u32DevId = 0;
    static HD_COMMON_MEM_VB_BLK blk;
    static HD_COMMON_MEM_DDR_ID ddr_id;
    static void *scale_in_buffer_va = NULL;         // 输入数据
    static void *scale_out_buffer_va = NULL;        // 输出数据
    static HD_VIDEO_FRAME scale_in_buffer = {0};
    static HD_VIDEO_FRAME scale_out_buffer = {0};
    static BOOL bIfNeedInit = SAL_TRUE;             // 是否需要执行时初始化

    if (NULL == pu8Src || NULL == pu8Dst || NULL == pstSrcSize || NULL == pstDstSize || NULL == pstSrcCropPrm || NULL == pstDstCropPrm)
    {
        SAL_ERROR("Null pointer error.\n");
        return SAL_FALSE;
    }

    // 获取ddrid, 分配内存，仅执行一次
    if (SAL_TRUE == bIfNeedInit)
    {
        bIfNeedInit = SAL_FALSE;                // 执行一次后便不再执行初始化
        ret = vendor_common_get_ddrid(pool, COMMON_PCIE_CHIP_RC, (HD_COMMON_MEM_DDR_ID *)&ddr_id);
        if (HD_OK != ret)
        {
            SAL_LOGE("vendor_common_get_ddrid fail, err:%d\n", ret);
            return SAL_FAIL;
        }
        /* Allocate in buffer */
        scale_in_buffer.ddr_id = 0;
        scale_in_buffer.dim.w = u32YuvWidthMax;
        scale_in_buffer.dim.h = u32YuvHeightMax;
        scale_in_buffer.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
        raw_frame_size = u32YuvWidthMax * u32YuvHeightMax * 2;
        blk = hd_common_mem_get_block(pool, raw_frame_size, ddr_id);
        if (HD_COMMON_MEM_VB_INVALID_BLK == blk) 
        {
            SAL_ERROR("hd_common_mem_get_block fail\r\n");
            return SAL_FAIL;
        }
        scale_in_buffer.phy_addr[0] = hd_common_mem_blk2pa(blk);
        if (scale_in_buffer.phy_addr[0] == 0) 
        {
            SAL_ERROR("hd_common_mem_blk2pa fail, blk = %#lx\r\n", blk);
            hd_common_mem_release_block(blk);
            return SAL_FAIL;
        }
        scale_in_buffer_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE,
                                                scale_in_buffer.phy_addr[0],
                                                raw_frame_size);

        /* Allocate out buffer */
        blk = hd_common_mem_get_block(pool, raw_frame_size, ddr_id);
        if (HD_COMMON_MEM_VB_INVALID_BLK == blk) {
            SAL_ERROR("hd_common_mem_get_block fail\r\n");
            ret = HD_ERR_NG;
            goto exit;
        }
        scale_out_buffer.phy_addr[0] = hd_common_mem_blk2pa(blk);
        if (scale_out_buffer.phy_addr[0] == 0) 
        {
            SAL_ERROR("hd_common_mem_blk2pa fail, blk = %#lx\r\n", blk);
            ret = HD_ERR_NG;
            goto exit;
        }
        scale_out_buffer_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE,
                                                 scale_out_buffer.phy_addr[0],
                                                 raw_frame_size);

        if (SAL_SOK != vpe_getFreeDevId(&u32DevId))
        {
            SAL_ERROR("vpe_getFreeDevId fail\n");
            ret = SAL_FAIL;
            goto exit;
        }
    }

     /* Set parameters */
    ret =  hd_videoproc_open(HD_VIDEOPROC_IN(u32DevId, 0), HD_VIDEOPROC_OUT(u32DevId, 0), &path_id);
    if (ret != HD_OK) {
        SAL_ERROR("hd_videoproc_open fail\n");
        goto exit;
    }

    crop.win.rect.x = pstSrcCropPrm->left;
    crop.win.rect.y = pstSrcCropPrm->top;
    crop.win.rect.w = pstSrcCropPrm->width;          // 目标区域宽
    crop.win.rect.h = pstSrcCropPrm->height;         // 目标区域高
    crop.win.coord.w = pstSrcSize->width;            // 图像总宽
    crop.win.coord.h = pstSrcSize->height;           // 图像总高
    ret = hd_videoproc_set(path_id, HD_VIDEOPROC_PARAM_IN_CROP, (void *)&crop);
    if (ret != HD_OK)
    {
        SAL_ERROR("hd_videoproc_set in crop fail\n");
        goto exit;
    }

    out.rect.x = pstDstCropPrm->left;
    out.rect.y = pstDstCropPrm->top;
    out.rect.w = pstDstCropPrm->width;          // 输出图像宽
    out.rect.h = pstDstCropPrm->height;         // 输出图像高
    out.bg.w = pstDstSize->width;               // 背景图像宽，图像合并时使用，与输出图像宽高一致时表示没有图像合并
    out.bg.h = pstDstSize->height;              // 背景图像高，
    out.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
    out.dir = (TRUE == bIfMirror ? HD_VIDEO_DIR_MIRRORY : HD_VIDEO_DIR_NONE);
    ret = hd_videoproc_set(path_id, HD_VIDEOPROC_PARAM_OUT, (void *)&out);
    if (ret != HD_OK) {
        SAL_ERROR("hd_videoproc_set out fail\n");
        goto exit;
    }

    /* 调整SCA范围 */
    stYuvRangeParam.yuv_range = SCA_RANGE_TV2PC;
    ret = vendor_video_set(path_id, VENDOR_VIDEO_YUV_RANGE, &stYuvRangeParam);
    if (ret != HD_OK) {
        SAL_ERROR("vendor_video_set fail\n");
        goto exit;
    }

    /* Push in buffer */
    memcpy(scale_in_buffer_va, (void *)pu8Src, pstSrcSize->width * pstSrcSize->height * 3 >> 1);

    ret = hd_videoproc_push_in_buf(path_id, &scale_in_buffer, &scale_out_buffer, 1000);
    if (ret != HD_OK) 
    {
        SAL_ERROR("hd_videoproc_push_in_buf fail, err:%d\n", ret);
        goto exit;
    }
    /* Pull out buffer */
    ret = hd_videoproc_pull_out_buf(path_id, &scale_out_buffer, 1000);
    if (ret != HD_OK) 
    {
        SAL_ERROR("hd_videoproc_pull_out_buf fail, err:%d\n", ret);
        goto exit;
    }
    else
    {
        /* 保存输出数据 */
        memcpy((void *)pu8Dst, scale_out_buffer_va, pstDstSize->width * pstDstSize->height * 3 >> 1);

        // char filename[128] = {0};
        // static int count = 0;
        // sprintf(filename, "user_vpe_%d_%dx%d_YUV422.yuv", count, scale_out_buffer.dim.w, scale_out_buffer.dim.h);
        // save_output(filename, scale_out_buffer_va, pstSrcSize->width * pstSrcSize->height * 3 / 2);
        // count++;
    }

    /* close video process module */
    ret = hd_videoproc_close(path_id);
    if (ret != HD_OK) {
        SAL_ERROR("hd_videoproc_close fail\n");
        goto exit;
    }

    return ret;

exit:
    bIfNeedInit = SAL_TRUE;     // 出错后下次需要初始化
    if (SAL_SOK != vpe_releaseDevId(u32DevId))  // 释放处理通道
    {
        SAL_ERROR("vpe_releaseDevId[%u] fail\n", u32DevId);
    }
    /* Release in buffer */
    hd_common_mem_munmap(scale_in_buffer_va, raw_frame_size);
    hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)scale_in_buffer.phy_addr[0]);
    /* Release out buffer */
    hd_common_mem_munmap(scale_out_buffer_va, raw_frame_size);
    hd_common_mem_release_block((HD_COMMON_MEM_VB_BLK)scale_out_buffer.phy_addr[0]);
    
    /* close video process module */
    if (HD_OK != hd_videoproc_close(path_id)) 
    {
        SAL_ERROR("hd_videoproc_close fail\n");
    }

    return ret;
}

static INT32 vpe_nt9833x_init()
{
    HD_RESULT ret = HD_OK;
    UINT32 i, j;

    memset(pathID, -1, sizeof(pathID));
    
    if ((ret = hd_videoproc_init()) != HD_OK) {
        return ret;
    }

    memset(&g_stVpeMngInfo, 0, sizeof(g_stVpeMngInfo));
    memset(&g_stVpeMngInfo.pathID, -1, sizeof(g_stVpeMngInfo.pathID));
    memset(&g_stVpeMngInfo.u32VpeDevId, -1, sizeof(g_stVpeMngInfo.u32VpeDevId));
    pthread_mutex_init(&(g_stVpeMngInfo.vpeMutex), NULL);

    
    memset(&stScaleOutBuf, 0, sizeof(stScaleOutBuf));
    for (i = 0; i < VDOPROC_DEV; i++)
    {
        for (j = 0; j < VODPROC_OUTPUT_PORT; j++)
        {
            pthread_mutex_init(&(stScaleOutBuf[i][j].chnBufMutex), NULL);
        }

    }

    return ret;
}


static INT32 vpe_nt9833x_deinit()
{
    HD_RESULT ret = HD_OK;
        
    if ((ret = hd_videoproc_uninit()) != HD_OK) {
        return ret;
    }

    return ret;

}

const DUP_PLAT_OPS *dup_hal_register()
{
    g_stDupPlatOps.init = vpe_nt9833x_init;
    g_stDupPlatOps.deinit = vpe_nt9833x_deinit;
    g_stDupPlatOps.create = vpe_nt9833x_create;
    g_stDupPlatOps.start = vpe_nt9833x_start;
    g_stDupPlatOps.stop = vpe_nt9833x_stop;
    g_stDupPlatOps.destroy = vpe_nt9833x_destroy;

    g_stDupPlatOps.getFrame = vpe_nt9833x_getFrame;
    g_stDupPlatOps.releaseFrame = vpe_nt9833x_releaseFrame;
    g_stDupPlatOps.sendFrame = vpe_nt9833x_sendFrame;
    g_stDupPlatOps.setParam = vpe_nt9833x_setParam;
    g_stDupPlatOps.getParam = vpe_nt9833x_getParam;

    g_stDupPlatOps.getPhyChnNum = vpe_nt9833x_getPhyChnNum;
    g_stDupPlatOps.getExtChnNum = vpe_nt9833x_getExtChnNum;

    g_stDupPlatOps.scaleYuvRange = vpe_nt9833x_scaleYuvRange;

    if (g_stDupPlatOps.init)
    {
        g_stDupPlatOps.init();
    }
    return &g_stDupPlatOps;
}




