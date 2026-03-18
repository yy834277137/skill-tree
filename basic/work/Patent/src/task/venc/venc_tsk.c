/**
 * @file   venc_tsk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  vecn 模块 tsk 层
 * @author liuyun10
 * @date   2018年12月1日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月1日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <dspcommon.h>
#include "libmux.h"
#include "capbility.h"
#include "venc_tsk.h"
#include "system_prm_api.h"
#include "link_drv_api.h"
#include "venc_drv_api.h"
#include "osd_tsk_api.h"
#include "venc_tsk_api.h"
#include "dup_tsk_api.h"
#include "vca_pack_api.h"
#include "capt_chip_drv_api.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#line __LINE__ "venc_tsk.c"

#define VENC_NEED_REBULIDE (SAL_TRUE)

#define H264_KEY_FRAME_NALU_NUM (4)
#define H265_KEY_FRAME_NALU_NUM (6)

/* ========================================================================== */
/*                           全局结构体                                */
/* ========================================================================== */
typedef struct tagVencCreatePackPrmst
{
    UINT32 DataType;
    UINT32 UsePs;
    UINT32 UseRtp;
} VENC_CREATE_PACK_S;


static VENC_TSK_CTRL_S gstVencTskCtrl = {0};

/* ========================================================================== */
/*                          函数声明                                        */
/* ========================================================================== */

void *venc_tsk_encStreamProcThread(void *pPrm);
void *venc_tsk_sendFrmToVencThread(void *pPrm);

INT32 venc_getSrcRate(UINT32 viChn, UINT32 *viFps);

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   venc_muxCreate
 * @brief      通过打包组件创建一个打包通道
 * @param[in]  UINT32 u32PackTpye 封装类型
 * @param[out] UINT32 *pHandleNum 封装通道内部id
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_muxCreate(UINT32 u32PackTpye, UINT32 *pHandleNum)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32MuxType = 0;
    MUX_PARAM stMuxPrm = {0};

    /**********动态创建一个新的打包通道***************/
    u32MuxType = u32PackTpye;
    memset(&stMuxPrm, 0, sizeof(stMuxPrm));
    s32Ret = MuxControl(MUX_GET_CHAN, &u32MuxType, &stMuxPrm);
    if (s32Ret != SAL_SOK)
    {
        VENC_LOGE("MuxControl idx %d err %x,u32MuxType %d\n", MUX_GET_CHAN, s32Ret, u32MuxType);
        return SAL_FAIL;
    }

    /**********创建打包模块***************/
    stMuxPrm.audPack = 1; /*PS封装里面添加AUD打包 RTP无此项设置，不影响*/
    stMuxPrm.bufAddr = (PUINT8)SAL_memMalloc(stMuxPrm.bufLen, "venc", "drv");
    if (NULL == stMuxPrm.bufAddr)
    {
        VENC_LOGE("stMuxPrm buffer malloc err!\n");
        return SAL_FAIL;
    }

    s32Ret = MuxControl(MUX_CREATE, &stMuxPrm, NULL);
    if (SAL_isFail(s32Ret))
    {
        SAL_memfree(stMuxPrm.bufAddr, "venc", "drv");
        VENC_LOGE("MuxControl idx %d err %x\n", MUX_CREATE, s32Ret);
        return SAL_FAIL;
    }

    /* 记录当前打包handle的通道号 */
    *pHandleNum = stMuxPrm.muxHdl;
    return SAL_SOK;

}

/**
 * @function   venc_packCreate
 * @brief      创建一个打包模块
 * @param[in]  VENC_PACK_INFO_ST *pstVencPackInfo 封装信息
 * @param[in]  VENC_CREATE_PACK_S *pstInPrm 输入封装参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_packCreate(VENC_PACK_INFO_ST *pstVencPackInfo, VENC_CREATE_PACK_S *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    MUX_STREAM_INFO_S stStreamInfo = {0};

    if ((NULL == pstVencPackInfo) || (NULL == pstInPrm))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstInPrm->UsePs)
    {
        VENC_LOGD("Create Ps pChnHandle!!!\n");
        pstVencPackInfo->isUsePs = SAL_TRUE;
        if (SAL_SOK != venc_muxCreate(MUX_PS, &pstVencPackInfo->u32PsHandle))
        {
            VENC_LOGE("Ps Pack Create Failed !!!\n");
            return SAL_FAIL;
        }
    }

    if (SAL_TRUE == pstInPrm->UseRtp)
    {
        VENC_LOGD("Create RTP pChnHandle!!!\n");
        pstVencPackInfo->isUseRtp = SAL_TRUE;
        if (SAL_SOK != venc_muxCreate(MUX_RTP, &pstVencPackInfo->u32RtpHandle))
        {
            VENC_LOGE("RTP Pack Create Failed !!!\n");
            return SAL_FAIL;
        }
    }

    /* 申请存放码流的内存 */
    memset(&stStreamInfo, 0, sizeof(stStreamInfo));
    stStreamInfo.muxType = pstInPrm->DataType;
    s32Ret = MuxControl(MUX_GET_STREAMSIZE, &stStreamInfo, &pstVencPackInfo->u32PackBufSize);
    if (s32Ret != SAL_SOK)
    {
        VENC_LOGE("MuxControl idx %d err %#x\n", MUX_GET_STREAMSIZE, s32Ret);
        return SAL_FAIL;
    }

    pstVencPackInfo->pPackOutputBuf = (UINT8 *)SAL_memMalloc(pstVencPackInfo->u32PackBufSize, "venc", "drv");
    if (NULL == pstVencPackInfo->pPackOutputBuf)
    {
        VENC_LOGE("packOutputBuf malloc err \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_addChn
 * @brief      tsk层增加一个编码通道，传递默认参数，创建打包handle，仅占位，不创建hal层通道
 * @param[in]  UINT32 u32Dev VI通道
 * @param[in]  UINT32 u32StreamId 码流ID，主/子/第三码流
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_addChn(UINT32 u32Dev, UINT32 u32StreamId, VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32ViFps = 30;
    void *pVencDrvHandle = NULL;
    VENC_DRV_INIT_PRM_S stDrvInitPrm = {0};
    VENC_CREATE_PACK_S stVencPackPrm = {0};

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstChnInfo->enStreamType = STREAM_MULTI;  /* STREAM_MULTI; // STREAM_VIDEO; // */
    /* pstChnInfo->PackType    = (SAL_TRUE == u32StreamId)? gstVencTskCtrl.SubPackType : gstVencTskCtrl.MainPackType; */
    /* 根据产品型号设置默认分辨率安检机使用3519A */

    if (0 == pstChnInfo->resolution)
    {
        pstChnInfo->resolution = CIF_FORMAT;
    }

    if (VENC_STREAM_ID_MAIN == u32StreamId)
    {
        pstChnInfo->u32PackType = gstVencTskCtrl.u32MainPackType;
    }
    else if (VENC_STREAM_ID_SUB == u32StreamId)
    {
        pstChnInfo->u32PackType = gstVencTskCtrl.u32SubPackType;
    }
    else
    {
        pstChnInfo->u32PackType = gstVencTskCtrl.u32ThirdPackType;
    }

    pstChnInfo->u32Dev = u32Dev;
    pstChnInfo->u32StreamId = u32StreamId;
    pstChnInfo->enVsStandard = gstVencTskCtrl.u32Standard;

    /* 向drv层传递默认参数 */
    memset(&stDrvInitPrm, 0, sizeof(stDrvInitPrm));

    stDrvInitPrm.u32Dev = u32Dev;
    stDrvInitPrm.u32StreamId = pstChnInfo->u32StreamId;

    stDrvInitPrm.stVencPrm.standard = pstChnInfo->enVsStandard;
    stDrvInitPrm.stVencPrm.quality = VENC_DEF_QUALITY;
    stDrvInitPrm.stVencPrm.fps = (VS_STD_PAL == pstChnInfo->enVsStandard) ? MAX_FPS_P_STANDARD : MAX_FPS_N_STANDARD;
    stDrvInitPrm.stVencPrm.width = 352;
    stDrvInitPrm.stVencPrm.height = 288;
    stDrvInitPrm.stVencPrm.bpsType = 1; /* 默认定码率 */
    stDrvInitPrm.stVencPrm.bps = 2048;
    stDrvInitPrm.stVencPrm.encodeType = H264; /*set def prm*/

    if (stDrvInitPrm.stVencPrm.fps > u32ViFps)
    {
        VENC_LOGW("change fps[%u] to [%u]\n", stDrvInitPrm.stVencPrm.fps, u32ViFps);
        stDrvInitPrm.stVencPrm.fps = u32ViFps;
    }

    stDrvInitPrm.stVencPrm.fps = stDrvInitPrm.stVencPrm.fps | (u32ViFps << 16); /*高16位记录vifps，低16为记录编码fps*/

    VENC_LOGI("vi %d strem %d venc type %d res %d x %d!\n", u32Dev, u32StreamId,
              stDrvInitPrm.stVencPrm.encodeType, stDrvInitPrm.stVencPrm.width, stDrvInitPrm.stVencPrm.height);

    /* 通道默认参数初始化 */
    s32Ret = venc_drv_addChan(&pVencDrvHandle, &stDrvInitPrm);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_drv_addChan fail!\n");
        return SAL_FAIL;
    }

    /* 创建打包模块，打包组件目前不支持释放，在占位时创建，不释放 */
    memset(&stVencPackPrm, 0, sizeof(stVencPackPrm));
    stVencPackPrm.DataType = MUX_VID;
    stVencPackPrm.UsePs = (pstChnInfo->u32PackType & PS_STREAM) ? SAL_TRUE : SAL_FALSE;
    stVencPackPrm.UseRtp = (pstChnInfo->u32PackType & RTP_STREAM) ? SAL_TRUE : SAL_FALSE;
    s32Ret = venc_packCreate((void *)&pstChnInfo->stVencPakcInfo, &stVencPackPrm);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("pack create err !!!\n");
        return SAL_FAIL;
    }

    pstChnInfo->pVencDrvHandle = pVencDrvHandle;
    pstChnInfo->u32EncWidth = 352;
    pstChnInfo->u32EncHeight = 288;
    pstChnInfo->u32EncodeType = H264;

    pstChnInfo->isAdd = SAL_TRUE;
    pstChnInfo->isStart = SAL_FALSE;
    VENC_LOGI("VENC Add Dev %d, Chn %d,resolution %d\n", u32Dev, u32StreamId, pstChnInfo->resolution);
    return SAL_SOK;
}

/**
 * @function   venc_resCheck
 * @brief      平台相关编码分辨率限制
 * @param[in]  UINT32 *pWidth
 * @param[in]  UINT32 *pHeight
 * @param[in]  None
 * @param[out] None
 * @return     void
 */
static void venc_resCheck(UINT32 *pWidth, UINT32 *pHeight)
{
    CAPB_VENC *pstVenc = NULL;

    if (pWidth == NULL || pHeight == NULL)
    {
        VENC_LOGE("pWidth pHeight NULL err\n");
        return;
    }

    pstVenc = capb_get_venc();
    if (pstVenc == NULL)
    {
        VENC_LOGE("null err\n");
        return;
    }

    if (*pWidth > pstVenc->venc_maxwidth || *pHeight > pstVenc->venc_maxheight)
    {
        *pWidth = pstVenc->venc_maxwidth;
        *pHeight = pstVenc->venc_maxheight;
    }
}

/**
 * @function   venc_getWHFormat
 * @brief      获取真实的宽高
 * @param[in]  UINT32 u32Res 分辨率
 * @param[out] UINT32 *pWidth 输出宽
 * @param[out] UINT32 *pHeight 输出高
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_getWHFormat(UINT32 u32Res, UINT32 *pWidth, UINT32 *pHeight)
{
    if ((NULL == pWidth) || (NULL == pHeight))
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    switch (u32Res)
    {
        case QQCIF_FORMAT:
        {
            *pWidth = 96;
            *pHeight = 80;
            break;
        }
        case QNCIF_FORMAT:
        {
            *pWidth = 160;
            *pHeight = 120;
            break;
        }
        case QCIF_FORMAT:
        {
            *pWidth = 176;
            *pHeight = 144;
            break;
        }
        case CIF_FORMAT:
        {
            *pWidth = 352;
            *pHeight = 288;
            break;
        }
        case VGA_FORMAT:
        {
            *pWidth = 640;
            *pHeight = 480;
            break;
        }
        case FCIF_FORMAT:
        {
            *pWidth = 704;
            *pHeight = 576;
            break;
        }
        case D1_FORMAT:
        {
            *pWidth = 720;
            *pHeight = 576;
            break;
        }
        case HD720p_FORMAT:
        {
            *pWidth = 1280;
            *pHeight = 720;
            break;
        }
        case XVGA_FORMAT:
        {
            *pWidth = 1280;
            *pHeight = 960;
            break;
        }
        case UXGA_FORMAT:
        {
            *pWidth = 1600;
            *pHeight = 1200;
            break;
        }

        case RES_1440_900_FORMAT:
        {
            *pWidth = 1440;
            *pHeight = 900;
            break;
        }

        case HD1080p_FORMAT:
        {
            *pWidth = 1920;
            *pHeight = 1080;
            break;
        }
        case RES_1080_1920_FORMAT:
        {
            *pWidth = 1080;
            *pHeight = 1920;
            break;
        }
        case RES_720_1280_FORMAT:
        {
            *pWidth = 720;
            *pHeight = 1280;
            break;
        }
        case RES_360_640_FORMAT:
        {
            *pWidth = 360;
            *pHeight = 640;
            break;
        }
        case RES_1080_720_FORMAT:
        {
            *pWidth = 1080;
            *pHeight = 720;
            break;
        }

        case SXGA_FORMAT:
        {
            *pWidth = 1280;
            *pHeight = 1024;
            break;
        }

        case XGA_FORMAT:
        {
            *pWidth = 1024;
            *pHeight = 768;
            break;
        }

        case SVGA_FORMAT:
        {
            *pWidth = 800;
            *pHeight = 600;
            break;
        }

        case RES_1366_768_FORMAT:
        {
            *pWidth = 1366;
            *pHeight = 768;
            break;
        }

        default:
        {
            VENC_LOGE("Not Support Res, %x !!!\n", u32Res);
            *pWidth = 1280;
            *pHeight = 720;
            break;
        }
    }

    venc_resCheck(pWidth, pHeight);
    return SAL_SOK;
}

/**
 * @function   venc_setDupMode
 * @brief      设置编码绑定前级VPSS的模式
 * @param[in]  DUP_ChanHandle *pstDupOutChnHandle   dup句柄
 * @param[out] 
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_setDupMode(DUP_ChanHandle *pstDupOutChnHandle)
{
    INT32 s32Ret = SAL_FAIL;
    CAPB_VENC   *pstCapbVenc = {0};
    PARAM_INFO_S stDupPrm   = {0};
    
    pstCapbVenc = capb_get_venc();
    

    if ((NULL == pstDupOutChnHandle->dupOps.OpDupSetBlitPrm) || (NULL == pstDupOutChnHandle->dupOps.OpDupGetBlitPrm))
    {
        VENC_LOGE("OpDupSetBlitPrm %p, && OpDupGetBlitPrm %p func is empty !!!\n",
                  pstDupOutChnHandle->dupOps.OpDupSetBlitPrm,
                  pstDupOutChnHandle->dupOps.OpDupGetBlitPrm);
        return SAL_FAIL;
    }
    
    if (NODE_BIND_TYPE_SDK_BIND != pstDupOutChnHandle->createFlags)
    {
        VENC_LOGW("not need set DupMode !!!\n");
        return SAL_SOK;
    }
    
    if(0 == pstCapbVenc->venc_scale)
    {
        stDupPrm.enType = VPSS_CHN_MODE;
        s32Ret = pstDupOutChnHandle->dupOps.OpDupGetBlitPrm(pstDupOutChnHandle, &stDupPrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("dup task getDupParam err !!!\n");
            return SAL_FAIL;
        }
    
        if (VPSS_MODE_USER !=stDupPrm.enVpssMode)
        {
            stDupPrm.enType = VPSS_CHN_MODE;
            stDupPrm.enVpssMode = VPSS_MODE_USER;
            s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);
        }
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("dup task setDupParam err !!!\n");
            return SAL_FAIL;
        }
        /* RK安检机暂时默认使用30帧编码,后续修改 */
        if((0 != pstCapbVenc->srcfps) && (0 != pstCapbVenc->dstfps))
        {
            stDupPrm.enType = FPS_CFG;
            stDupPrm.u32Fps = pstCapbVenc->dstfps | (pstCapbVenc->srcfps << 16); /*高16位记录vifps，低16为记录编码fps*/
            s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);

            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("dup task setDupParam err !!!\n");
                return SAL_FAIL;
            }
            VENC_LOGW("venc_setDupFPS srcfps%d  dstfps%d !!!\n",pstCapbVenc->srcfps,pstCapbVenc->dstfps);
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_checkPrm
 * @brief      用于检测用户下发的编码参数是否合法
 * @param[in]  ENCODER_PARAM *pInParm 输入参数
 * @param[out] SAL_VideoFrameParam *pstVencDrvPrm 编码参数
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_checkPrm(ENCODER_PARAM *pstInParm, SAL_VideoFrameParam *pstVencPrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 width = 0;
    UINT32 height = 0;

    if ((NULL == pstInParm) || (NULL == pstVencPrm))
    {
        VENC_LOGE("ENCODER_PARAM is NUll !!!\n");
        return SAL_FAIL;
    }

    /* 检查帧率和码率 */
    s32Ret = venc_getWHFormat(pstInParm->resolution, &width, &height);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_getWHFormat err !!!\n");
        return SAL_FAIL;
    }

    pstVencPrm->width = width;
    pstVencPrm->height = height;
    pstVencPrm->bps = pstInParm->bps;
    pstVencPrm->bpsType = pstInParm->bpsType;
    pstVencPrm->encodeType = pstInParm->videoType;
    pstVencPrm->fps = pstInParm->fps;
    #if 0
    s32Ret = Venc_tskCheckVideoFps(pstInParm->fps, &pstVencPrm->fps, &pstVencPrm->standard);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Venc_tskCheckVideoFps err !!!\n");
        return SAL_FAIL;
    }

    #endif

    if ((pstVencPrm->bps < 32) || (pstVencPrm->bps > 16384))
    {
        if (width * height <= 1280 * 720)
        {
            VENC_LOGW("bps set %d, use default CBR bps 3072\n", pstVencPrm->bps);
            pstVencPrm->bps = 3072;
            pstVencPrm->bpsType = 1;
        }
        else
        {

            VENC_LOGW("bps set %d, use default CBR bps 4096\n", pstVencPrm->bps);
            pstVencPrm->bps = 4096;
            pstVencPrm->bpsType = 1;
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_setDupPrm
 * @brief      建立和dup层的连接
 * @param[in]  void *pstChnInfo 通道句柄
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_setDupPrm(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_FAIL;
    PARAM_INFO_S stDupPrm = {0};
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("null prm !!!\n");
        return SAL_FAIL;
    }

    pstDupOutChnHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        VENC_LOGE("Please Get DupHandle first !!!\n");
        return SAL_FAIL;
    }

    /* 使用绑定则进行绑定 否则申请一块内存 */
    if (pstDupOutChnHandle->dupOps.OpDupSetBlitPrm != NULL)  /* (SAL_TRUE == VENC_USE_BIND) */
    {
        if (pstDspInitPrm->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN && pstDspInitPrm->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM
         && pstChnInfo->u32EncHeight >= pstDspInitPrm->dspCapbPar.xray_width_max && !ISM_5030D_INPUT_DUP_CHG) // 一屏双显输入非标准分辨率特殊处理
        {
            stDupPrm.enType = IMAGE_VALID_RECT_CFG;
            stDupPrm.stImgValidRect.stRect.u32X = 0;
            stDupPrm.stImgValidRect.stRect.u32Y = (int)(pstChnInfo->u32EncHeight - pstDspInitPrm->dspCapbPar.xray_width_max)/2 ;
            stDupPrm.stImgValidRect.stRect.u32W = pstChnInfo->u32EncWidth;
            stDupPrm.stImgValidRect.stRect.u32H = pstDspInitPrm->dspCapbPar.xray_width_max;
            stDupPrm.stImgValidRect.stBg.u32Width = pstChnInfo->u32EncWidth;
            stDupPrm.stImgValidRect.stBg.u32Height = pstChnInfo->u32EncHeight;
            VENC_LOGW("dup_task_setDupParam x %d y %d w %d h %d bg[%d %d] !!!\n",stDupPrm.stImgValidRect.stRect.u32X, stDupPrm.stImgValidRect.stRect.u32Y, stDupPrm.stImgValidRect.stRect.u32W,
                stDupPrm.stImgValidRect.stRect.u32H,stDupPrm.stImgValidRect.stBg.u32Width, stDupPrm.stImgValidRect.stBg.u32Height);
        }
        else
        {
            stDupPrm.enType = IMAGE_SIZE_CFG;
            stDupPrm.stImgSize.u32Width = pstChnInfo->u32EncWidth;
            stDupPrm.stImgSize.u32Height = pstChnInfo->u32EncHeight;
        }

        s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("dup_task_setDupParam err !!!\n");
            return SAL_FAIL;
        }
        
        s32Ret = venc_setDupMode(pstDupOutChnHandle);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc set Dup Mode err !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("OpDupSetBlitPrm %p, && OpDupGetBlitPrm %p func is empty !!!\n",
                  pstDupOutChnHandle->dupOps.OpDupSetBlitPrm,
                  pstDupOutChnHandle->dupOps.OpDupGetBlitPrm);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_bindDupChn
 * @brief      建立和dup层的连接
 * @param[in]  VENC_CHN_INFO_S *pstVencChnInfo 通道句柄
 * @param[in]  UINT32 isBind 绑定使能标志
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_bindDupChn(VENC_CHN_INFO_S *pstVencChnInfo, UINT32 isBind)
{
    INT32 s32Ret = SAL_SOK;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};

    pstDupOutChnHandle = (DUP_ChanHandle *)pstVencChnInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        VENC_LOGE("Please Get DupHandle first !!!\n");
        return SAL_FAIL;
    }

    stDupBindPrm.mod = SYSTEM_MOD_ID_VENC;
    stDupBindPrm.modChn = 0; /* 底层编码器的设备号 */
    stDupBindPrm.chn = pstVencChnInfo->u32HalChan; /* 底层编码器的通道 */

    if (NULL != pstDupOutChnHandle->dupOps.OpDupBindBlit)
    {
        s32Ret = pstDupOutChnHandle->dupOps.OpDupBindBlit((Ptr)pstDupOutChnHandle, (Ptr) & stDupBindPrm, isBind);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("Bind Hal Chn err !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   venc_setDupChn
 * @brief      设置dup
 * @param[in]  void *pstChnInfo 通道句柄
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_setDupChn(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_FAIL;
    PARAM_INFO_S stDupPrm = {0};
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("null prm !!!\n");
        return SAL_FAIL;
    }

    pstDupOutChnHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        VENC_LOGE("Please Get DupHandle first !!!\n");
        return SAL_FAIL;
    }

    if (pstDspInitPrm->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN && pstDspInitPrm->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM 
        && pstChnInfo->u32EncHeight >= pstDspInitPrm->dspCapbPar.xray_width_max && !ISM_5030D_INPUT_DUP_CHG) // 一屏双显输入非标准分辨率特殊处理
    {
        stDupPrm.enType = IMAGE_VALID_RECT_CFG;
        stDupPrm.stImgValidRect.stRect.u32X = 0;
        stDupPrm.stImgValidRect.stRect.u32Y = (int)(pstChnInfo->u32EncHeight - pstDspInitPrm->dspCapbPar.xray_width_max)/2 ;
        stDupPrm.stImgValidRect.stRect.u32W = pstChnInfo->u32EncWidth;
        stDupPrm.stImgValidRect.stRect.u32H = pstDspInitPrm->dspCapbPar.xray_width_max;
        stDupPrm.stImgValidRect.stBg.u32Width = pstChnInfo->u32EncWidth;
        stDupPrm.stImgValidRect.stBg.u32Height = pstChnInfo->u32EncHeight;
        VENC_LOGW("dup_task_setDupParam x %d y %d w %d h %d bg[%d %d] !!!\n",stDupPrm.stImgValidRect.stRect.u32X, stDupPrm.stImgValidRect.stRect.u32Y, stDupPrm.stImgValidRect.stRect.u32W,
            stDupPrm.stImgValidRect.stRect.u32H,stDupPrm.stImgValidRect.stBg.u32Width, stDupPrm.stImgValidRect.stBg.u32Height);
    }
    else
    {
        stDupPrm.enType = IMAGE_SIZE_CFG;
        stDupPrm.stImgSize.u32Width = pstChnInfo->u32EncWidth;
        stDupPrm.stImgSize.u32Height = pstChnInfo->u32EncHeight;
    }

    if (pstDupOutChnHandle->dupOps.OpDupSetBlitPrm != NULL)
    {
        s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("dup_task_setDupParam err !!!\n");
            return SAL_FAIL;
        }
    }
    s32Ret = venc_setDupMode(pstDupOutChnHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc set Dup Mode err !!!\n");
        return SAL_FAIL;
    }
    return SAL_SOK;
}


/**
 * @function   venc_startDupChn
 * @brief      设置dup
 * @param[in]  void *pstChnInfo 通道句柄
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_startDupChn(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_FAIL;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("null prm !!!\n");
        return SAL_FAIL;
    }

    pstDupOutChnHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        VENC_LOGE("Please Get DupHandle first !!!\n");
        return SAL_FAIL;
    }

    if (pstDupOutChnHandle->dupOps.OpDupStartBlit != NULL)
    {
        s32Ret = pstDupOutChnHandle->dupOps.OpDupStartBlit(pstDupOutChnHandle);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("dup start err !!!\n");
            //return SAL_FAIL;  //非绑定模式，vpe sdk 不需要调start，但vpe 软件状态需要置成STARTED状态，才会送去vpe做裁剪放大
        }
    }

    return SAL_SOK;
}


/**
 * @function   venc_initDupChan
 * @brief      建立和dup层的连接
 * @param[in]  void *pstChnInfo 通道句柄
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_initDupChan(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_FAIL;
    PARAM_INFO_S stDupPrm = {0};
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("null prm !!!\n");
        return SAL_FAIL;
    }

    pstDupOutChnHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        VENC_LOGE("Please Get DupHandle first !!!\n");
        return SAL_FAIL;
    }

    /* 使用绑定则进行绑定 否则申请一块内存 */
    if (pstDupOutChnHandle->dupOps.OpDupSetBlitPrm != NULL)  /* (SAL_TRUE == VENC_USE_BIND) */
    {
        s32Ret = venc_bindDupChn(pstChnInfo, SAL_TRUE);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_bindDupChn err !!!\n");
            return SAL_FAIL;
        }

        if (pstDspInitPrm->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN && pstDspInitPrm->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM
         && pstChnInfo->u32EncHeight >= pstDspInitPrm->dspCapbPar.xray_width_max && !ISM_5030D_INPUT_DUP_CHG) // 一屏双显输入非标准分辨率特殊处理
        {
            stDupPrm.enType = IMAGE_VALID_RECT_CFG;
            stDupPrm.stImgValidRect.stRect.u32X = 0;
            stDupPrm.stImgValidRect.stRect.u32Y = (int)(pstChnInfo->u32EncHeight - pstDspInitPrm->dspCapbPar.xray_width_max)/2 ;
            stDupPrm.stImgValidRect.stRect.u32W = pstChnInfo->u32EncWidth;
            stDupPrm.stImgValidRect.stRect.u32H = pstDspInitPrm->dspCapbPar.xray_width_max;
            stDupPrm.stImgValidRect.stBg.u32Width = pstChnInfo->u32EncWidth;
            stDupPrm.stImgValidRect.stBg.u32Height = pstChnInfo->u32EncHeight;
            VENC_LOGW("dup_task_setDupParam x %d y %d w %d h %d bg[%d %d] !!!\n",stDupPrm.stImgValidRect.stRect.u32X, stDupPrm.stImgValidRect.stRect.u32Y, stDupPrm.stImgValidRect.stRect.u32W,
                stDupPrm.stImgValidRect.stRect.u32H,stDupPrm.stImgValidRect.stBg.u32Width, stDupPrm.stImgValidRect.stBg.u32Height);
        }
        else
        {
            stDupPrm.enType = IMAGE_SIZE_CFG;
            stDupPrm.stImgSize.u32Width = pstChnInfo->u32EncWidth;
            stDupPrm.stImgSize.u32Height = pstChnInfo->u32EncHeight;
        }

        s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("dup_task_setDupParam err !!!\n");
            return SAL_FAIL;
        }

        VENC_LOGW("set dup prm res %d %d !!!\n", pstChnInfo->u32EncWidth, pstChnInfo->u32EncHeight);
        
        s32Ret = venc_setDupMode(pstDupOutChnHandle);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc set Dup Mode err !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("OpDupBindBlit %p, && OpDupCopyBlit %p func is empty !!!\n",
                  pstDupOutChnHandle->dupOps.OpDupBindBlit,
                  pstDupOutChnHandle->dupOps.OpDupCopyBlit);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   venc_startEncode
 * @brief      创建编码通道，开启编码
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_startEncode(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_SOK;
    DUP_ChanHandle *pstDupHandle = NULL;

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstChnInfo->isStart)
    {
        VENC_LOGW("Venc has started !!!\n");
        return SAL_SOK;
    }

    /* 该通道是否添加*/
    if (SAL_TRUE != pstChnInfo->isAdd)
    {
        VENC_LOGE("chan not ready!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_setDupChn(pstChnInfo);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("set dup fail!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_drv_create(pstChnInfo->pVencDrvHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Venc startVenc fail!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_drv_getHalChan(pstChnInfo->pVencDrvHandle, &pstChnInfo->u32HalChan);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Venc startVenc fail!\n");
        return SAL_FAIL;
    }

    VENC_LOGI("chan %d stream %d hal chan %d !!!\n", pstChnInfo->u32Dev, pstChnInfo->u32StreamId, pstChnInfo->u32HalChan);

    /* 创建 压缩后的编码码流采集处理线程 */
    pstChnInfo->taskRun = SAL_TRUE;
    SAL_thrCreate(&pstChnInfo->hndl, venc_tsk_encStreamProcThread, SAL_THR_PRI_DEFAULT, 0, pstChnInfo);

    pstDupHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
    if (NULL == pstDupHandle)
    {
        VENC_LOGE("Please Get DupHandle first !!!\n");
        return SAL_FAIL;
    }

    /* 从vpe获取帧，手动送venc */
    if (NODE_BIND_TYPE_GET == pstDupHandle->createFlags)
    {
        pstChnInfo->sendFrmTaskRun = SAL_TRUE;
        //SAL_thrCreate(&pstChnInfo->hndle2, venc_tsk_sendFrmToVencThread, SAL_THR_PRI_DEFAULT, 0, pstChnInfo);
    }
    /* vpe 与venc 绑定模式 */
    else if (NODE_BIND_TYPE_SDK_BIND == pstDupHandle->createFlags)
    {
        s32Ret = venc_bindDupChn(pstChnInfo, SAL_TRUE);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_bindDupChn fail!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("pstDupHandle->createFlags illegal %d!\n", pstDupHandle->createFlags);
        return SAL_FAIL;
    }

    s32Ret = venc_startDupChn(pstChnInfo);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_drv_start fail!\n");
        return SAL_FAIL;
    }

    /* 编码器开启收图进行视频编码 */
    s32Ret = venc_drv_start(pstChnInfo->pVencDrvHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_drv_start fail!\n");
        return SAL_FAIL;
    }

    pstChnInfo->uiStart = SAL_TRUE;
    SAL_mutexSignal(pstChnInfo->mutexHandle);
    if (NODE_BIND_TYPE_GET == pstDupHandle->createFlags)
    {
        SAL_mutexSignal(pstChnInfo->sendFrmMutexHandle);
    }
    osd_tsk_hangEncProc(pstChnInfo->u32Dev, pstChnInfo->u32StreamId, pstChnInfo->u32HalChan);
    osd_tsk_restart(pstChnInfo->u32Dev);

    return SAL_SOK;

}

/**
 * @function   venc_delete
 * @brief      删除编码器
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_delete(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_SOK;
    DUP_ChanHandle *pstDupHandle = NULL;

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstChnInfo->taskRun)
    {
        /* 等待线程结束，回收线程资源 */
        pstChnInfo->taskRun = SAL_FALSE;
        s32Ret = SAL_thrJoin(&pstChnInfo->hndl);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("pthread exit fail!\n");
            return SAL_FAIL;
        }
    }
    
    VENC_LOGW("sendFrmTaskRun pthread START!\n");
    if (SAL_TRUE == pstChnInfo->sendFrmTaskRun)
    {
        /* 等待线程结束，回收线程资源 */
        pstChnInfo->sendFrmTaskRun = SAL_FALSE;
        /*s32Ret = SAL_thrJoin(&pstChnInfo->hndle2);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("pthread exit fail!\n");
            return SAL_FAIL;
        }*/
    } 

    osd_tsk_hangEncProc(pstChnInfo->u32Dev, pstChnInfo->u32StreamId, pstChnInfo->u32HalChan);
    
    /* 停止通道 */
    s32Ret = venc_drv_stop(pstChnInfo->pVencDrvHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Venc startVenc fail!\n");
        return SAL_FAIL;
    }

    pstDupHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
    if (NULL != pstDupHandle)
    {
        if (NODE_BIND_TYPE_SDK_BIND == pstDupHandle->createFlags)
        {
            /* 使用绑定则进行解绑 */
            s32Ret = venc_bindDupChn(pstChnInfo, SAL_FALSE);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("venc_bindDupChn err !!!\n");
                return SAL_FAIL;
            }
        }
    }
      
    /* 销毁通道 */
    s32Ret = venc_drv_delete(pstChnInfo->pVencDrvHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Venc startVenc fail!\n");
        return SAL_FAIL;
    }

    pstChnInfo->uiStart = SAL_FALSE;
    return SAL_SOK;
}

/**
 * @function   venc_getSrcRate
 * @brief      获取编码源帧率
 * @param[in]  UINT32 viChn 编码通道号
 * @param[out] UINT32 *viFps vi帧率
 * @return     int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_getSrcRate(UINT32 viChn, UINT32 *viFps)
{
    INT32 s32Ret = SAL_FAIL;
    CAPT_STATUS_S stStatus = {0};
    CAPB_PRODUCT *pstProduct = NULL;
    CAPB_VENC *pstCapbVenc = NULL;

    pstProduct = capb_get_product();
    pstCapbVenc = capb_get_venc();
    if ((pstCapbVenc == NULL) || (pstProduct == NULL))
    {
        VENC_LOGE("null err\n");
        return SAL_FAIL;
    }

    if (NULL == viFps)
    {
        VENC_LOGE("Input viFps is null!\n");
        return SAL_FAIL;
    }

    if (VIDEO_INPUT_INSIDE == pstProduct->enInputType)
    {
        if (pstCapbVenc->dstfps != 0)
        {
            *viFps = pstCapbVenc->dstfps;
        }
        else
        {
            *viFps = 60;
        }

        return SAL_SOK;
    }

    s32Ret = capt_func_chipGetStatus(viChn, &stStatus);
    if (s32Ret < 0)
    {
        VENC_LOGE("viChn = %d\n", viChn);
        return s32Ret;
    }

    *viFps = stStatus.stRes.u32Fps ? stStatus.stRes.u32Fps : 60;

    return SAL_SOK;
}

/**
 * @function   venc_setParam
 * @brief      设置编码参数
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道信息
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_setParam(VENC_CHN_INFO_S *pstChnInfo, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 isStaticPrmChg = 0;
    UINT32 u32ViFps = 30;

    if ((NULL == pstChnInfo) || (NULL == pstInPrm))
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    /* 该通道是否创建*/
    if (SAL_TRUE != pstChnInfo->isAdd)
    {
        VENC_LOGE("chan not ready!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_getSrcRate(pstChnInfo->u32Dev, &u32ViFps);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("get vi fps Failed!\n");
        return SAL_FAIL;
    }

    if (pstInPrm->fps > u32ViFps)
    {
        VENC_LOGW("change fps[%u] to [%u]\n", pstInPrm->fps, u32ViFps);
        pstInPrm->fps = u32ViFps;
    }

    pstInPrm->fps = pstInPrm->fps | (u32ViFps << 16); /*高16位记录vifps，低16为记录编码fps*/

    /* 该通道是已经开启，未开启仅保存设置的编码参数 */
    if (SAL_TRUE == pstChnInfo->isStart)
    {
        isStaticPrmChg = venc_drv_staticParmCheck(pstChnInfo->pVencDrvHandle, pstInPrm);
        if (SAL_TRUE == isStaticPrmChg)
        {
            s32Ret = venc_delete(pstChnInfo);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("Venc delete fail!\n");
                return SAL_FAIL;
            }
        }
        else
        {
            s32Ret = venc_drv_stop(pstChnInfo->pVencDrvHandle);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("venc_drv_stop Failed!\n");
                return SAL_FAIL;
            }
        }

        /* 2. 配置编码器  */
        s32Ret = venc_drv_setPrm(pstChnInfo->pVencDrvHandle, pstInPrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_drv_setPrm Failed !!!\n");
            return SAL_FAIL;
        }

        if (SAL_TRUE == isStaticPrmChg)
        {
            s32Ret = venc_drv_create(pstChnInfo->pVencDrvHandle);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("Venc startVenc fail!\n");
                return SAL_FAIL;
            }

            s32Ret = venc_drv_getHalChan(pstChnInfo->pVencDrvHandle, &pstChnInfo->u32HalChan);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("Venc startVenc fail!\n");
                return SAL_FAIL;
            }

            s32Ret = venc_initDupChan(pstChnInfo);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("Venc venc_initDupChan fail!\n");
                return SAL_FAIL;
            }
        }
        else
        {
            s32Ret = venc_setDupPrm(pstChnInfo);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("Venc venc_setDupPrm fail!\n");
                return SAL_FAIL;
            }
        }

        s32Ret = venc_drv_start(pstChnInfo->pVencDrvHandle);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_drv_start Failed!\n");
            return SAL_FAIL;
        }

        pstChnInfo->uiStart = SAL_TRUE;
        SAL_mutexSignal(pstChnInfo->mutexHandle);
        osd_tsk_hangEncProc(pstChnInfo->u32Dev, pstChnInfo->u32StreamId, pstChnInfo->u32HalChan);
        osd_tsk_restart(pstChnInfo->u32Dev);
    }
    else
    {
        /* 1. 配置编码器  */
        s32Ret = venc_drv_setPrm(pstChnInfo->pVencDrvHandle, pstInPrm);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_drv_setPrm Failed !!!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_forceIFrame
 * @brief      编码器强制I帧
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_forceIFrame(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_SOK;

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_FALSE == pstChnInfo->isStart)
    {
        VENC_LOGE("Your must start venc first !!!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_drv_requestIDR(pstChnInfo->pVencDrvHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("venc_drv_requestIDR Failed!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_stopEncode
 * @brief      停止编码，销毁编码器
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_stopEncode(VENC_CHN_INFO_S *pstChnInfo)
{
    INT32 s32Ret = SAL_SOK;

    if (NULL == pstChnInfo)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_FALSE == pstChnInfo->isStart)
    {
        VENC_LOGW("Venc hasn't started yet !!!\n");
        return SAL_SOK;
    }

    if (SAL_TRUE == VENC_NEED_REBULIDE)
    {
        s32Ret = venc_delete(pstChnInfo);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("Venc delete fail!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        s32Ret = venc_drv_stop(pstChnInfo->pVencDrvHandle);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("venc_drv_stop fail!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_videoFramePack
 * @brief      编码数据打包
 * @param[in]  VENC_CHN_INFO_S *pstChnInfo 通道句柄
 * @param[in]  SYSTEM_BITS_DATA_INFO_ST *pstBitsDataInfo 输入码流信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_videoFramePack(VENC_CHN_INFO_S *pstChnInfo, SYSTEM_BITS_DATA_INFO_ST *pstBitsDataInfo)
{
    INT32 i = 0;
    UINT32 u32Dev = 0;
    UINT32 u32StreamId = 0;
    INT32 s32Ret = SAL_FAIL;
    UINT8 *pData = NULL;
    MUX_INFO stMuxInfo = {0};

    STREAM_FRAME_INFO_ST *pstStreamFrameInfo = NULL;
    VENC_PACK_INFO_ST *pstVencPackInfo = NULL;

    MUX_IN_BITS_INTO_S *pstMuxInInfo = NULL;
    MUX_OUT_BITS_INTO_S *pstMuxOutInfo = NULL;
    MUX_PROC_INFO_S stMuxProcInfo = {0};

    REC_STREAM_INFO_ST stRecStreamInfo = {0};
    NET_STREAM_INFO_ST stNetStreamInfo = {0};
    VCA_PACK_INFO_ST stVcaPackInfo = {0};


    if ((NULL == pstChnInfo) || (NULL == pstBitsDataInfo))
    {
        VENC_LOGE("empty input !!!\n");
        return SAL_FAIL;
    }

    pstVencPackInfo = &pstChnInfo->stVencPakcInfo;
    u32Dev = pstChnInfo->u32Dev;
    u32StreamId = pstChnInfo->u32StreamId;

    stVcaPackInfo.u32Dev = pstChnInfo->u32Dev;
    stVcaPackInfo.u32StreamId = pstChnInfo->u32StreamId;
    stVcaPackInfo.u32Chn = pstChnInfo->u32Dev * VENC_DEV_CHN_NUM + pstChnInfo->u32StreamId;
    stVcaPackInfo.u32Width = pstChnInfo->u32EncWidth;
    stVcaPackInfo.u32Height = pstChnInfo->u32EncHeight;
    stVcaPackInfo.u64Pts = pstBitsDataInfo->stStreamFrameInfo[0].stNaluInfo[0].u64PTS;

    stVcaPackInfo.isUsePs = pstChnInfo->stVencPakcInfo.isUsePs;
    stVcaPackInfo.isUseRtp = pstChnInfo->stVencPakcInfo.isUseRtp;
    stVcaPackInfo.u32PsHandle = pstChnInfo->stVencPakcInfo.u32PsHandle;
    stVcaPackInfo.u32RtpHandle = pstChnInfo->stVencPakcInfo.u32RtpHandle;
    stVcaPackInfo.u32PackBufSize = pstChnInfo->stVencPakcInfo.u32PackBufSize;
    stVcaPackInfo.pPackOutputBuf = pstChnInfo->stVencPakcInfo.pPackOutputBuf;

    s32Ret = vca_pack_process(&stVcaPackInfo);
    if (SAL_FAIL == s32Ret)
    {
        VENC_LOGE("vca pack err!!!\n");
    }

    for (i = 0; i < pstBitsDataInfo->uiFrameCnt; i++)
    {
        pstStreamFrameInfo = &pstBitsDataInfo->stStreamFrameInfo[i];

        memset(&stMuxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
        pstMuxInInfo = &stMuxProcInfo.muxData.stInBuffer;
        pstMuxOutInfo = &stMuxProcInfo.muxData.stOutBuffer;

        pstMuxInInfo->bVideo = SAL_TRUE;
        pstMuxInInfo->naluNum = 1;
        pstMuxInInfo->frame_type = pstStreamFrameInfo->eFrameType;

        pData = pstStreamFrameInfo->stNaluInfo[0].pucNaluPtr;
        if (NULL == pData)
        {
            VENC_LOGE("empty memory\n");
            return SAL_FAIL;
        }

        if (STREAM_TYPE_H264_IFRAME == pstStreamFrameInfo->eFrameType)
        {
            pstMuxInInfo->is_key_frame = SAL_TRUE;
            pstMuxInInfo->naluNum = 3;

            /* SPS 00 00 00 01 67 */
            pstMuxInInfo->spsLen = pstStreamFrameInfo->stNaluInfo[0].uiNaluLen;
            pstMuxInInfo->spsBufferAddr = pstStreamFrameInfo->stNaluInfo[0].pucNaluPtr;

            /* PPS 00 00 00 01 68 */
            pstMuxInInfo->ppsLen = pstStreamFrameInfo->stNaluInfo[1].uiNaluLen;
            pstMuxInInfo->ppsBufferAddr = pstStreamFrameInfo->stNaluInfo[1].pucNaluPtr;

            /* 剩余的为 Islice  00 00 00 01 65 */
            pstMuxInInfo->bufferAddr[0] = pstStreamFrameInfo->stNaluInfo[pstStreamFrameInfo->uiNaluNum - 1].pucNaluPtr;
            pstMuxInInfo->bufferLen[0] = pstStreamFrameInfo->stNaluInfo[pstStreamFrameInfo->uiNaluNum - 1].uiNaluLen;

            pstMuxInInfo->vpsLen = 0;              /* H264无vps */
            pstMuxInInfo->vpsBufferAddr = NULL;    /* */
            pstMuxInInfo->u64PTS = pstStreamFrameInfo->stNaluInfo[0].u64PTS;
        }
        else if (STREAM_TYPE_H265_IFRAME == pstStreamFrameInfo->eFrameType)
        {
            pstMuxInInfo->is_key_frame = SAL_TRUE;
            pstMuxInInfo->naluNum = 4;

            /* VPS 00 00 00 01 40 */
            pstMuxInInfo->vpsLen = pstStreamFrameInfo->stNaluInfo[0].uiNaluLen;
            pstMuxInInfo->vpsBufferAddr = pstStreamFrameInfo->stNaluInfo[0].pucNaluPtr;

            /* SPS 00 00 00 01 67 */
            pstMuxInInfo->spsLen = pstStreamFrameInfo->stNaluInfo[1].uiNaluLen;
            pstMuxInInfo->spsBufferAddr = pstStreamFrameInfo->stNaluInfo[1].pucNaluPtr;

            /* PPS 00 00 00 01 68 */
            pstMuxInInfo->ppsLen = pstStreamFrameInfo->stNaluInfo[2].uiNaluLen;
            pstMuxInInfo->ppsBufferAddr = pstStreamFrameInfo->stNaluInfo[2].pucNaluPtr;

            /* 剩余的为 Islice  00 00 00 01 65 */
            pstMuxInInfo->bufferAddr[0] = pstStreamFrameInfo->stNaluInfo[pstStreamFrameInfo->uiNaluNum - 1].pucNaluPtr;
            pstMuxInInfo->bufferLen[0] = pstStreamFrameInfo->stNaluInfo[pstStreamFrameInfo->uiNaluNum - 1].uiNaluLen;

            pstMuxInInfo->u64PTS = pstStreamFrameInfo->stNaluInfo[0].u64PTS;
        }
        else
        {
            pstMuxInInfo->is_key_frame = 0;
            pstMuxInInfo->bufferAddr[0] = pstStreamFrameInfo->stNaluInfo[0].pucNaluPtr;
            pstMuxInInfo->bufferLen[0] = pstStreamFrameInfo->stNaluInfo[0].uiNaluLen;
            pstMuxInInfo->u64PTS = pstStreamFrameInfo->stNaluInfo[0].u64PTS;
        }

        pstMuxInInfo->width = pstStreamFrameInfo->uiFrameWidth;
        pstMuxInInfo->height = pstStreamFrameInfo->uiFrameHeight;
        pstMuxInInfo->fps = pstStreamFrameInfo->uiFps;

        /* pstMuxInInfo->u64PTS /= 1000; */

        /* Ps 打包 */
        if (SAL_TRUE == pstVencPackInfo->isUsePs)
        {
            /* VENC_LOGI("uiChn %d..u64PTS %llu\n",pstVencChnInfo->uiChn,pstMuxInInfo->u64PTS); */
            pstMuxOutInfo->streamLen = 0;
            pstMuxOutInfo->bufferAddr = pstVencPackInfo->pPackOutputBuf;
            pstMuxOutInfo->bufferLen = pstVencPackInfo->u32PackBufSize;
            stMuxProcInfo.muxHdl = pstVencPackInfo->u32PsHandle; /* Ps 打包创建的通道 */;
            s32Ret = MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL);
            if (s32Ret != SAL_SOK)
            {
                VENC_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
                return SAL_FAIL;
            }

            s32Ret = MuxControl(MUX_GET_INFO, &pstVencPackInfo->u32PsHandle, &stMuxInfo);
            if (s32Ret != SAL_SOK)
            {
                VENC_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
                return SAL_FAIL;
            }

            /* 写入录像缓存 */
            memset(&stRecStreamInfo, 0, sizeof(REC_STREAM_INFO_ST));
            stRecStreamInfo.pucAddr = pstMuxOutInfo->bufferAddr;
            stRecStreamInfo.uiSize = pstMuxOutInfo->streamLen;
            stRecStreamInfo.uiType = 1;
            stRecStreamInfo.streamType = pstMuxInInfo->is_key_frame;
            stRecStreamInfo.IFrameInfo = (1 | (pstMuxOutInfo->streamLen << 1));
            SAL_getDateTime(&stRecStreamInfo.absTime);
            stRecStreamInfo.stdTime = stMuxInfo.time_stamp;
            /* VENC_LOGI("stdTime %u\n",stRecStreamInfo.stdTime); */
            stRecStreamInfo.pts = pstMuxInInfo->u64PTS;
            SystemPrm_writeRecPool(u32Dev, u32StreamId, &stRecStreamInfo);
        }

        /* Rtp 打包 */
        if (SAL_TRUE == pstVencPackInfo->isUseRtp)
        {
            pstMuxOutInfo->streamLen = 0;
            pstMuxOutInfo->bufferAddr = pstVencPackInfo->pPackOutputBuf;
            pstMuxOutInfo->bufferLen = pstVencPackInfo->u32PackBufSize;
            stMuxProcInfo.muxHdl = pstVencPackInfo->u32RtpHandle;       /* Rtp 打包创建的通道 */
            s32Ret = MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL);
            if (s32Ret != SAL_SOK)
            {
                VENC_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);

                return SAL_FAIL;
            }

            /* 写入共享内存 */
            memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));
            stNetStreamInfo.pucAddr = pstMuxOutInfo->bufferAddr;;
            stNetStreamInfo.uiSize = pstMuxOutInfo->streamLen;;
            stNetStreamInfo.uiType = 1;
            /*static int cnt_debug_enc = 0;
            if (u32Dev == 0&& u32StreamId == 0 && cnt_debug_enc >= 240)
            {
                cnt_debug_enc = 0;
                VENC_LOGI("uiSize %d pucAddr %p\n",stNetStreamInfo.uiSize, stNetStreamInfo.pucAddr);
            }
            cnt_debug_enc++;*/
            SystemPrm_writeToNetPool(u32Dev, u32StreamId, &stNetStreamInfo);
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_tsk_encStreamProcThread
 * @brief      编码模块码流处理线程
 * @param[in]  void *pPrm 编码通道结构体
 * @param[out] None
 * @return     void *
 */
void *venc_tsk_encStreamProcThread(void *pPrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32StreamId = 0;
    UINT32 u32Dev = 0;
    BOOL bFreeBuf = 0;

    VENC_CHN_INFO_S *pstChnInfo = NULL;
    DUP_ChanHandle *pstDupHandle = NULL;
    DUP_COPY_DATA_BUFF stDupGetBuff;
    DUP_COPY_DATA_BUFF *pstDupGetBuff;
    SYSTEM_FRAME_INFO stDupGetSystemFrame;
    SYSTEM_BITS_DATA_INFO_ST *pstStreamFrameInfo = NULL;
    UINT64 time0 = 0, time1 = 1, time2 =0, time3 = 0;

    if (NULL == pPrm)
    {
        VENC_LOGE("!!!\n");
        return NULL;
    }

    pstChnInfo = (VENC_CHN_INFO_S *)pPrm;
    u32StreamId = pstChnInfo->u32StreamId;
    u32Dev = pstChnInfo->u32Dev;
    pstStreamFrameInfo = &pstChnInfo->stStreamFrameInfo[0];

    stDupGetBuff.stDupDataCopyPrm.uiRotate = 0xFFFF;
    stDupGetBuff.pstDstSysFrame = &stDupGetSystemFrame;
    memset(&stDupGetSystemFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));
    if (stDupGetSystemFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stDupGetSystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
                            //goto out4;
                //fixme 出错时还需要释放内存
                return NULL;
        }

    }
    VENC_LOGW("Venc Dev %d u32StreamId %d Start\n", pstChnInfo->u32Dev, pstChnInfo->u32StreamId);
    SAL_SET_THR_NAME();
    while (1)
    {
        if (SAL_FALSE == pstChnInfo->taskRun)
        {
            break;
        }

        /* 等待编码模块开启 */
        SAL_mutexLock(pstChnInfo->mutexHandle);
        if (SAL_FALSE == pstChnInfo->uiStart)
        {
            VENC_LOGW("Dev %d Chn %d task sleep !\n", u32Dev, u32StreamId);

            /* 防止进入等待前delete函数先获得锁，条件信号提前发送导致的无限等待 */
            SAL_mutexWait(pstChnInfo->mutexHandle);
            VENC_LOGW("Dev %d Chn %d task wake up !\n", u32Dev, u32StreamId);
        }

        SAL_mutexUnlock(pstChnInfo->mutexHandle);

        pstDupHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
        if (pstDupHandle == NULL)
        {
            DISP_LOGW("pstDupHandle NULL, u32Dev %d, u32StreamId %d\n", u32Dev, u32StreamId);
            SAL_msleep(10);
            continue;
        }

        bFreeBuf = SAL_FALSE;
        time0 = sal_get_tickcnt();
        /* 通过DUP的号码，获取DUP信息，包括通道的handle。 */
        /* 通过通道handle信息，获取vpss对应通道数据 */
        if (NODE_BIND_TYPE_GET == pstDupHandle->createFlags)
        {
            pstDupGetBuff = &stDupGetBuff;
            s32Ret = pstDupHandle->dupOps.OpDupGetBlit(pstDupHandle, pstDupGetBuff);
            if (SAL_SOK == s32Ret)
            {
                venc_drv_sendVideoFrm(pstChnInfo->pVencDrvHandle, pstDupGetBuff->pstDstSysFrame);
                bFreeBuf = SAL_TRUE;
                //usleep(15000);
            }
            else
            {
                VENC_LOGE("OpDupGetBlit:%d\n", s32Ret);
            }
        }
        else
        {

        }
        time1 = sal_get_tickcnt();

        /* 获取码流 */
        s32Ret = venc_drv_getBuff(pstChnInfo->pVencDrvHandle, pstStreamFrameInfo);
        if (SAL_isSuccess(s32Ret))
        {
            pstStreamFrameInfo->uiFrameCnt = 1;
        }
        else
        {
            VENC_LOGW("Venc Dev %d Chn %d Get Frame fail.\n", u32Dev, u32StreamId);
            pstStreamFrameInfo->uiFrameCnt = 0;
        }
        time2 = sal_get_tickcnt();

        if (bFreeBuf)
        {
            pstDupHandle->dupOps.OpDupPutBlit(pstDupHandle, pstDupGetBuff);
        }

        /* 打包 */
        if (pstStreamFrameInfo->uiFrameCnt > 0)
        {
            s32Ret = venc_videoFramePack(pstChnInfo, pstStreamFrameInfo);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("venc_videoFramePack err !!!\n");
            }

            /* 释放码流 */
            s32Ret = venc_drv_putBuff(pstChnInfo->pVencDrvHandle);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("venc_drv_putBuff fail\n");
            }
        }
        time3 = sal_get_tickcnt();
        
        VENC_LOGD("u32Dev %d send time %llu get time %llu pack time %llu all time %llu\n",u32Dev,time1-time0,time2-time1,time3-time2,time3-time0);
    }

    s32Ret = sys_hal_rleaseVideoFrameInfoSt(&stDupGetSystemFrame);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("sys_hal_rleaseVideoFrameInfoSt fail\n");
    }

    VENC_LOGW("Venc Dev %d u32StreamId %d exit.\n", pstChnInfo->u32Dev, pstChnInfo->u32StreamId);
    SAL_thrExit(NULL);
    return NULL;
}

/**
 * @function   venc_tsk_encStreamProcThread
 * @brief      编码模块码流处理线程
 * @param[in]  void *pPrm 编码通道结构体
 * @param[out] None
 * @return     void *
 */
void *venc_tsk_sendFrmToVencThread(void *pPrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32StreamId = 0;
    UINT32 u32Dev = 0;
    DUP_ChanHandle *pstDupHandle = NULL;

    VENC_CHN_INFO_S *pstChnInfo = NULL;
    DUP_COPY_DATA_BUFF stDupGetBuff;
    DUP_COPY_DATA_BUFF *pstDupGetBuff;
    SYSTEM_FRAME_INFO stDupGetSystemFrame;

    if (NULL == pPrm)
    {
        VENC_LOGE("!!!\n");
        return NULL;
    }

    pstChnInfo = (VENC_CHN_INFO_S *)pPrm;
    u32StreamId = pstChnInfo->u32StreamId;
    u32Dev = pstChnInfo->u32Dev;

    VENC_LOGW("Venc send frame thread, Dev %d u32StreamId %d Start\n", pstChnInfo->u32Dev, pstChnInfo->u32StreamId);
    SAL_SET_THR_NAME();


    stDupGetBuff.stDupDataCopyPrm.uiRotate = 0xFFFF;
    stDupGetBuff.pstDstSysFrame = &stDupGetSystemFrame;
    memset(&stDupGetSystemFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));
    if (stDupGetSystemFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stDupGetSystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
                            //goto out4;
                //fixme 出错时还需要释放内存
                return NULL;
        }

    }
    while (1)
    {
        if (SAL_FALSE == pstChnInfo->sendFrmTaskRun) //是否用同一个taskRun?
        {
            break;
        }

        /* 等待编码模块开启 */
        SAL_mutexLock(pstChnInfo->sendFrmMutexHandle);
        if (SAL_FALSE == pstChnInfo->uiStart)
        {
            VENC_LOGW("Dev %d Chn %d task sleep !\n", u32Dev, u32StreamId);

            /* 防止进入等待前delete函数先获得锁，条件信号提前发送导致的无限等待 */
            SAL_mutexWait(pstChnInfo->sendFrmMutexHandle);
            VENC_LOGW("Dev %d Chn %d task wake up !\n", u32Dev, u32StreamId);
        }

        SAL_mutexUnlock(pstChnInfo->sendFrmMutexHandle);

        pstDupHandle = (DUP_ChanHandle *)pstChnInfo->pDupHandle;
        if (pstDupHandle == NULL)
        {
            DISP_LOGW("pstDupHandle NULL, u32Dev %d, u32StreamId %d\n", u32Dev, u32StreamId);
            SAL_msleep(10);
            continue;
        }

        /* 通过DUP的号码，获取DUP信息，包括通道的handle。 */
        /* 通过通道handle信息，获取vpss对应通道数据 */
        if (NODE_BIND_TYPE_GET == pstDupHandle->createFlags)
        {
            pstDupGetBuff = &stDupGetBuff;
            s32Ret = pstDupHandle->dupOps.OpDupGetBlit(pstDupHandle, pstDupGetBuff);
            if (SAL_SOK == s32Ret)
            {
                venc_drv_sendVideoFrm(pstChnInfo->pVencDrvHandle, pstDupGetBuff->pstDstSysFrame);
                usleep(5000);
                pstDupHandle->dupOps.OpDupPutBlit(pstDupHandle, pstDupGetBuff);
            }
            else
            {
                VENC_LOGE("OpDupGetBlit:%d\n", s32Ret);
            }
        }
        else
        {

        }

    }

    s32Ret = sys_hal_rleaseVideoFrameInfoSt(&stDupGetSystemFrame);

    VENC_LOGW("Venc Dev %d u32StreamId %d exit.\n", pstChnInfo->u32Dev, pstChnInfo->u32StreamId);
    SAL_thrExit(NULL);
    return NULL;
}



/**
 * @function   venc_tsk_init
 * @brief      初始模块初始化
 * @param[in]  None
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_init(void)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;
    CHAR szInstName[NAME_LEN] = {0};
    INST_INFO_S *pstInst = NULL;
    NODE_CFG_S stNodeCfg[] = {
        {IN_NODE_0, "in_0", NODE_BIND_TYPE_SDK_BIND},
    };
    DSPINITPARA *pDspInitParm = SystemPrm_getDspInitPara();

    memset(&gstVencTskCtrl, 0, sizeof(VENC_TSK_CTRL_S));

    /* 视频源通道的配置由初始化时候指定，每个视频源下在分别有主码流和子码流 */
    gstVencTskCtrl.u32DevNum = pDspInitParm->encChanCnt;
    gstVencTskCtrl.u32MainPackType = pDspInitParm->stStreamInfo.stVideoParam.packetType;
    gstVencTskCtrl.u32SubPackType = pDspInitParm->stStreamInfo.stVideoParam.subPacketType;
    gstVencTskCtrl.u32ThirdPackType = pDspInitParm->stStreamInfo.stVideoParam.ThirdPacketType;
    s32Ret = SystemPrm_getSysVideoFormat();
    gstVencTskCtrl.u32Standard = (SAL_SOK == s32Ret) ? VS_STD_PAL : VS_STD_NTSC;

    if (VENC_DEV_NUM < gstVencTskCtrl.u32DevNum)
    {
        VENC_LOGE("VENC u32DevNum [%u], dev num overflow!!!!\n", gstVencTskCtrl.u32DevNum);
        return SAL_FAIL;
    }

    /* 初始通道内部化锁 */
    for (i = 0; i < gstVencTskCtrl.u32DevNum; i++)
    {
        pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[i];
        for (j = 0; j < VENC_DEV_CHN_NUM; j++)
        {
            pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[j];
            SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstVencChnInfo->mutexHandle);
            SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstVencChnInfo->sendFrmMutexHandle);

            snprintf(szInstName, NAME_LEN, "VENC_INDEV%d_STRM%d", i, j);
            pstInst = link_drvReqstInst();
            if (pstInst)
            {
                s32Ret = link_drvInitInst(pstInst, szInstName);
                if (SAL_FAIL == s32Ret)
                {
                    VENC_LOGE("link inst init err\n");
                    return SAL_FAIL;
                }

                for (k = 0; k < sizeof(stNodeCfg) / sizeof(NODE_CFG_S); k++)
                {
                    s32Ret = link_drvInitNode(pstInst, &stNodeCfg[k]);
                    if (SAL_FAIL == s32Ret)
                    {
                        VENC_LOGE("link node init err\n");
                        return SAL_FAIL;
                    }
                }
            }
        }
    }

    /* drv层初始化 */
    if (SAL_SOK != venc_drv_init())
    {
        VENC_LOGE("venc_drv_init err\n");
        return SAL_FAIL;
    }

    /* 初始化打包模块结构体 */
    MuxInit();

    /* 初始化通道内部锁 */
    for (i = 0; i < VENC_DEV_NUM * VENC_DEV_CHN_NUM; i++)
    {
        s32Ret = vca_pack_init(i);
        if (SAL_FAIL == s32Ret)
        {
            SAL_ERROR("vca %d pack init err\n", i);
            return SAL_FAIL;
        }
    }

    /* tsk与drv通道绑定 */
    for (i = 0; i < gstVencTskCtrl.u32DevNum; i++)
    {
        pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[i];
        for (j = 0; j < VENC_DEV_CHN_NUM; j++)
        {
            pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[j];
            s32Ret = venc_addChn(i, j, pstVencChnInfo);
            if (SAL_isFail(s32Ret))
            {
                VENC_LOGE("venc_addChn Failed!\n");
                return SAL_FAIL;
            }

            pstVencChnInfo->isAdd = SAL_TRUE;
        }
    }

    VENC_LOGW("---------venc init finish!---------\n");

    return SAL_SOK;
}

/**
 * @function   venc_tsk_init
 * @brief      初始模块初始化
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_start(UINT32 u32Dev, UINT32 u32StreamId)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;
    CHAR szDstInstName[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;
    CHAR *pszSrcNodeName = NULL;
    void *pHandle = NULL;

    if ((u32Dev >= gstVencTskCtrl.u32DevNum) || (u32StreamId >= VENC_DEV_CHN_NUM))
    {
        VENC_LOGE("Not enough Chn !!!\n");
        return SAL_FAIL;
    }

    pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[u32Dev];
    pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[u32StreamId];

    /* 应用一般先调配置参数接口VENC_CmdSetPrm，所以DupHandle在配置参数接口里已赋值
       这里再做一下检查*/
    if (!pstVencChnInfo->pDupHandle)
    {
        snprintf(szDstInstName, NAME_LEN, "VENC_INDEV%d_STRM%d", u32Dev, u32StreamId);
        s32Ret = link_drvGetSrcNode(szDstInstName, "in_0", &pstSrcInst, &pszSrcNodeName);
        if (SAL_isFail(s32Ret))
        {
            VENC_LOGE("link_drvGetSrcNode fail, s32Ret %d !!!\n", s32Ret);
            return s32Ret;
        }

        pHandle = link_drvGetHandleFromNode(pstSrcInst, pszSrcNodeName);
        pstVencChnInfo->pDupHandle = pHandle;
        VENC_LOGI("vi %d stream %d create dup %s!\n", u32Dev, u32StreamId, szDstInstName);
    }

    SAL_mutexLock(pstVencChnInfo->mutexHandle);

    /* 创建视频编码器 */
    s32Ret = venc_startEncode(pstVencChnInfo);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGW("Enc Dev %d Chn %d start fail !!!\n", u32Dev, u32StreamId);
        SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
        return SAL_FAIL;
    }

    pstVencChnInfo->isStart = SAL_TRUE;
    VENC_LOGI("Venc viDev %d Chn %d Start Success!\n", u32Dev, u32StreamId);
    SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_tsk_setPrm
 * @brief      设置编码参数命令，通道未创建时，仅设置编码参数，不执行其他操作
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[in]  void *prm 编码参数信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */

INT32 venc_tsk_setPrm(UINT32 u32Dev, UINT32 u32StreamId, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;
    ENCODER_PARAM *pstVencPrm = NULL;
    SAL_VideoFrameParam stVencPrm = {0};  /* Drv 层设置 */

    CHAR szDstInstName[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;
    CHAR *pszSrcNodeName = NULL;
    void *pHandle = NULL;

    if ((u32Dev >= gstVencTskCtrl.u32DevNum) || (u32StreamId >= VENC_DEV_CHN_NUM))
    {
        VENC_LOGE("Not enough Chn !!!\n");
        return SAL_FAIL;
    }

    if (NULL == prm)
    {
        VENC_LOGE("ENCODER_PARAM NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencPrm = (ENCODER_PARAM *)prm;
    
    VENC_LOGI("u32Dev %d,u32StreamId %d\n", u32Dev, u32StreamId);
    pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[u32Dev];
    pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[u32StreamId];

    sprintf(szDstInstName, "VENC_INDEV%d_STRM%d", u32Dev, u32StreamId);
    s32Ret = link_drvGetSrcNode(szDstInstName, "in_0", &pstSrcInst, &pszSrcNodeName);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("link_drvGetSrcNode fail, s32Ret %d !!!\n", s32Ret);
        return SAL_FAIL;
    }

    pHandle = link_drvGetHandleFromNode(pstSrcInst, pszSrcNodeName);
    pstVencChnInfo->pDupHandle = pHandle;
    /* 检测分辨率是否支持，不支持直接返回，若支持则检查帧率和码率设置 */
    s32Ret = venc_checkPrm(pstVencPrm, &stVencPrm);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("ENCODER_PARAM illegal !!!\n");
        return SAL_FAIL;
    }
    //todo
    if( pstVencChnInfo->resolution == pstVencPrm->resolution && pstVencChnInfo->u32EncodeType == stVencPrm.encodeType
        && pstVencChnInfo->u32EncWidth == stVencPrm.width && pstVencChnInfo->u32EncHeight == stVencPrm.height
        &&venc_drv_getfps(pstVencChnInfo->pVencDrvHandle) == stVencPrm.fps)
    {
        VENC_LOGW("Venc viDev %d Chn %d resolution 0x%x Set Prm Success!\n", u32Dev, u32StreamId, pstVencPrm->resolution);
        return SAL_SOK;
    }
    pstVencChnInfo->resolution = pstVencPrm->resolution;
    pstVencChnInfo->u32EncWidth = stVencPrm.width;
    pstVencChnInfo->u32EncHeight = stVencPrm.height;
    pstVencChnInfo->u32EncodeType = stVencPrm.encodeType;

    SAL_mutexLock(pstVencChnInfo->mutexHandle);
    s32Ret = venc_setParam(pstVencChnInfo, &stVencPrm);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Venc Dev %d Chn %d fail!\n", u32Dev, u32StreamId);
        SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
        return SAL_FAIL;
    }

    VENC_LOGI("Venc viDev %d Chn %d resolution 0x%x Set Prm Success!\n", u32Dev, u32StreamId, pstVencPrm->resolution);
    SAL_mutexUnlock(pstVencChnInfo->mutexHandle);

    return SAL_SOK;
}

/**
 * @function   venc_tsk_forceIFrame
 * @brief      强制I帧命令
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_forceIFrame(UINT32 u32Dev, UINT32 u32StreamId)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((u32Dev >= gstVencTskCtrl.u32DevNum) || (u32StreamId >= VENC_DEV_CHN_NUM))
    {
        VENC_LOGE("Not enough Chn !!!\n");
        return SAL_FAIL;
    }

    pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[u32Dev];
    pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[u32StreamId];

    SAL_mutexLock(pstVencChnInfo->mutexHandle);
    s32Ret = venc_forceIFrame(pstVencChnInfo);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Enc Dev %d Chn %d force I frame Failed !!!\n", u32Dev, u32StreamId);
        SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
        return SAL_FAIL;
    }

    VENC_LOGI("Venc Dev %d Chn %d Force I Frame Success!\n", u32Dev, u32StreamId);
    SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_tsk_stop
 * @brief      停止编码命令
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_stop(UINT32 u32Dev, UINT32 u32StreamId)
{
    INT32 s32Ret = SAL_SOK;
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((u32Dev >= gstVencTskCtrl.u32DevNum) || (u32StreamId >= VENC_DEV_CHN_NUM))
    {
        VENC_LOGE("Not enough Chn !!!\n");
        return SAL_FAIL;
    }

    pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[u32Dev];
    pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[u32StreamId];

    SAL_mutexLock(pstVencChnInfo->mutexHandle);
    s32Ret = venc_stopEncode(pstVencChnInfo);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("Enc Dev %d Chn %d stop Failed !!!\n", u32Dev, u32StreamId);
        SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
        return SAL_FAIL;
    }

    pstVencChnInfo->isStart = SAL_FALSE;
    VENC_LOGI("Venc Dev %d Chn %d Stop Success!\n", u32Dev, u32StreamId);
    SAL_mutexUnlock(pstVencChnInfo->mutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_tsk_getHalChan
 * @brief      获取hal通道ID
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] UINT32 *pChn 返回通道号
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_getHalChan(UINT32 u32Dev, UINT32 u32StreamId, UINT32 *pChn)
{
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((u32Dev >= gstVencTskCtrl.u32DevNum) || (u32StreamId >= VENC_DEV_CHN_NUM))
    {
        VENC_LOGE("Not enough Chn !!!\n");
        return SAL_FAIL;
    }

    pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[u32Dev];
    pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[u32StreamId];

    if (SAL_TRUE == pstVencChnInfo->uiStart)
    {
        *pChn = pstVencChnInfo->u32HalChan;
    }
    else
    {
        /*VENC_LOGE("enc not start dev %d stream %d !!!\n", u32Dev, u32StreamId);*/
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_tsk_getResolution
 * @brief      获取编码分辨率
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] UINT32 *pResolution 返回分辨率信息
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_getResolution(UINT32 u32Dev, UINT32 u32StreamId, UINT32 *pResolution)
{
    VENC_DEV_INFO_S *pstVencDevInfo = NULL;
    VENC_CHN_INFO_S *pstVencChnInfo = NULL;

    if ((u32Dev >= gstVencTskCtrl.u32DevNum) || (u32StreamId >= VENC_DEV_CHN_NUM))
    {
        VENC_LOGE("Not enough Chn !!!\n");
        return SAL_FAIL;
    }

    pstVencDevInfo = &gstVencTskCtrl.stVencDevInfo[u32Dev];
    pstVencChnInfo = &pstVencDevInfo->stVencChnInfo[u32StreamId];

    if (SAL_TRUE == pstVencChnInfo->uiStart)
    {
        *pResolution = pstVencChnInfo->resolution;
    }
    else
    {
        VENC_LOGE("enc not start dev %d stream %d !!!\n", u32Dev, u32StreamId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

