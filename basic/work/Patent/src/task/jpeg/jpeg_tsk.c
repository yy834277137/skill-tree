/**
 * @file   jpeg_tsk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  jpeg 模块 tsk 层
 * @author liuyun10
 * @date   liuyun10
 * @note
 * @note \n History
   1.Date        : c
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include <dspcommon.h>
#include "jpeg_tsk.h"
#include "jpeg_tsk_api.h"
#include "vdec_tsk_api.h"
#include "capbility.h"
#include "task_ism.h"
#include "link_drv_api.h"
#include "dup_tsk_api.h"
#include "jpeg_drv_api.h"
#include "sva_out.h"
#include "system_prm_api.h"

#line __LINE__ "jpeg_tsk.c"


/*****************************************************************************
                               宏定义
*****************************************************************************/

/* 图片描述信息缓存区的大小 */
#define JPEG_ADD_INFO_SIZE (256)

#define MAX_ENC_JPEG_SIZE (1920 * 1200 * 3 / 2)

/* 暂定最大两个JPEG 抓图，也就两个obj */
#define JPEG_MAX_TSK_NUM (MAX_VENC_CHAN + MAX_VDEC_CHAN)

#define JPEG_TASK_CHECK_RET(ret, ret_success, str) \
    { \
        if (ret != ret_success) \
        { \
            DISP_LOGE("%s, ret: 0x%x \n", str, ret); \
            return SAL_FAIL; \
        } \
    }


/*****************************************************************************
                            结构定义
*****************************************************************************/

/* 初始化的默认参数 */
typedef enum JpegDefaultPrmE
{
    JPEG_DEF_WIDTH = 1280,     /* 编码宽 */
    JPEG_DEF_HEIGHT = 720,     /* 编码宽 */
    JPEG_DEF_QUALITY = 80      /* 图像质量 */
} JPEG_DEFAULT_PRM_E;

/* Jpeg obj 管理结构体 */
typedef struct tagJpegTskDevInfost
{
    UINT32 isStart;
    UINT32 isCreate;
    UINT32 isDraw;
    UINT32 isSet;

    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Quality;

    JPEG_CTRL_PRM_S stCtrlPrm;     /* 当前抓图控制信息 */

    INT8 *pJpeg;                 /* 输出图片缓存区 */
    UINT32 u32Offset;             /* 需要增加附加信息 信息长度偏移 */

    SVA_PROCESS_OUT *pstSvaOut;     /*智能信息*/

    SYSTEM_FRAME_INFO stSysFrame;  /* 输入图片信息 普通抓图模块使用 */

    void *pDupHandle;

    SAL_ThrHndl hndl;            /* 抓图处理线程 */
    void *mutexHandle;

    void *pChanHandle;           /* drv的句柄*/
} JPEG_TSK_DEV_INFO_S;

typedef struct
{
    JPEG_TSK_DEV_INFO_S stJpegDevInfo[JPEG_MAX_TSK_NUM];
} JPEG_TSK_CTRL_S;

/*****************************************************************************
                               全局变量
*****************************************************************************/

static JPEG_TSK_CTRL_S gstJpegTskCtrl;

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   jpeg_saveHalPrm
 * @brief      保存hal层参数于本地
 * @param[in]  UINT32 u32Dev : 设备ID
 * @param[in]  JPEG_PARAM *pstInPrm: 设置参数
 * @param[out] None
 * @return     void
 */
static void jpeg_saveHalPrm(UINT32 u32Dev, JPEG_PARAM *pstInPrm)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return;
    }

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32Dev];
    pstJpegDevInfo->u32Width = pstInPrm->width;
    pstJpegDevInfo->u32Height = pstInPrm->height;
    pstJpegDevInfo->u32Quality = pstInPrm->quality;
}

/**
 * @function   jpeg_saveCtrlPrm
 * @brief      保存抓图控制参数于本地，主要是连续抓图的设置
 * @param[in]  UINT32 u32Dev : 设备ID
 * @param[in]  JPEG_PARAM *pstInParm: 设置参数
 * @param[out] None
 * @return     void
 */
static void jpeg_saveCtrlPrm(UINT32 u32Dev, JPEG_PARAM *pstInPrm)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    JPEG_CTRL_PRM_S *pstCtrlPrm = NULL;

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return;
    }

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32Dev];
    pstCtrlPrm = &pstJpegDevInfo->stCtrlPrm;

    if (CAP_CONTINUE == pstInPrm->eJpegMode)
    {
        pstCtrlPrm->eJpegMode = JPEG_MORE;
        pstCtrlPrm->uiCaptCnt = pstInPrm->uiCaptCnt;
        pstCtrlPrm->uiCaptSec = pstInPrm->uiCaptSec;
    }
    else
    {
        pstCtrlPrm->eJpegMode = JPEG_ONCE;
    }

    if (JPEG_MORE == pstCtrlPrm->eJpegMode)
    {
        if (0 == pstCtrlPrm->uiCaptCnt)
        {
            pstCtrlPrm->uiCaptCnt = 10;
        }

        if (0 == pstCtrlPrm->uiCaptSec)
        {
            pstCtrlPrm->uiCaptSec = 1;
        }

        pstCtrlPrm->uiCaptInterval = (pstCtrlPrm->uiCaptSec * 1000) / (pstCtrlPrm->uiCaptCnt);
        if (100 > pstCtrlPrm->uiCaptInterval)
        {
            /* 限制一秒最多10张 */
            pstCtrlPrm->uiCaptInterval = 100;
        }
    }

    pstCtrlPrm->uiFinCnt = 0;

}

/**
 * @function   jpeg_savePrm
 * @brief      保存设置的编码参数
 * @param[in]  UINT32 u32Dev : 设备ID
 * @param[in]  JPEG_PARAM *pstInPrm: 设置参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_savePrm(UINT32 u32Dev, JPEG_PARAM *pstInPrm)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pstInPrm))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    jpeg_saveHalPrm(u32Dev, pstInPrm);
    jpeg_saveCtrlPrm(u32Dev, pstInPrm);

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32Dev];
    pstJpegDevInfo->isSet = SAL_TRUE;

    return SAL_SOK;
}

/**
 * @function   jpeg_start
 * @brief      开启编码抓图，开启前判断是否有设置的参数
 * @param[in]  UINT32 u32Dev : 设备ID
 * @param[in]  JPEG_ADD_INFO_ST *pstInParm: 设置参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_start(UINT32 u32Dev, JPEG_PARAM *pstJpegPrm, INT8 * pstAddInfo, UINT32 uiSize)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    CAPB_JPEG *pstCapbJpeg = NULL;
    SAL_VideoFrameParam stJpegCreatePrm = {0};
    UINT32 u32VdecStatus = 0;

    if (JPEG_MAX_TSK_NUM <= u32Dev)
    {
        JPEG_LOGE("prm err!!!\n");
        return SAL_FAIL;
    }

    pstCapbJpeg = capb_get_jpeg();
    if (pstCapbJpeg == NULL)
    {
        JPEG_LOGE("NULL ERR\n");
        return SAL_FAIL;
    }

    if (u32Dev >= pstCapbJpeg->videv_cnt)
    {
        s32Ret = vdec_tsk_GetChnStatus(u32Dev - pstCapbJpeg->videv_cnt, &u32VdecStatus);
        if (SAL_SOK != s32Ret)
        {
            JPEG_LOGE("Chn %d Vdec_tskGetChnStatus Failed !!!\n", u32Dev - pstCapbJpeg->videv_cnt);
            return SAL_FAIL;
        }

        if ((u32VdecStatus != VDEC_STATE_RUNNING) && (u32VdecStatus != VDEC_STATE_PAUSE))
        {
            JPEG_LOGE("Chn %d u32VdecStatus is %d is not start or nonpause!!!\n", u32Dev - pstCapbJpeg->videv_cnt, u32VdecStatus);
            return SAL_FAIL;
        }
    }

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32Dev];

    /* 保存抓图参数 */
    if (NULL != pstJpegPrm)
    {
        jpeg_saveHalPrm(u32Dev, pstJpegPrm);
        jpeg_saveCtrlPrm(u32Dev, pstJpegPrm);
        pstJpegDevInfo->isSet = SAL_TRUE;
        JPEG_LOGI("Jpeg Dev %d Save Prm Success\n", u32Dev);
    }
    
    /* 如果需要，则添加附加信息 */
    if ((NULL != pstAddInfo) && (0 != uiSize))
    {
        if (uiSize > JPEG_ADD_INFO_SIZE)
        {
            uiSize = JPEG_ADD_INFO_SIZE;
        }

        if ((NULL != pstJpegDevInfo->pJpeg) && (0 != uiSize))
        {
            memcpy(pstJpegDevInfo->pJpeg, pstAddInfo, uiSize);
            pstJpegDevInfo->u32Offset = uiSize;
            JPEG_LOGI("Jpeg Dev %d Add Info Success\n", u32Dev);
        }
        else
        {
            JPEG_LOGW("please malloc first\n");
            pstJpegDevInfo->u32Offset = 0;
        }
    }
    else
    {
        pstJpegDevInfo->u32Offset = 0;
    }

    /* 未设置参数默认为单张抓图，并使用默认设置 */
    if (SAL_TRUE != pstJpegDevInfo->isSet)
    {
        pstJpegDevInfo->stCtrlPrm.eJpegMode = JPEG_ONCE;
        pstJpegDevInfo->u32Width = JPEG_DEF_WIDTH;
        pstJpegDevInfo->u32Height = JPEG_DEF_HEIGHT;
        pstJpegDevInfo->u32Quality = JPEG_DEF_QUALITY;
        JPEG_LOGI("Jpeg Dev %d Use Defaule Prm \n", u32Dev);
    }

    stJpegCreatePrm.width = pstJpegDevInfo->u32Width;
    stJpegCreatePrm.height = pstJpegDevInfo->u32Height;
    stJpegCreatePrm.quality = pstJpegDevInfo->u32Quality;
    stJpegCreatePrm.encodeType = MJPEG;
    stJpegCreatePrm.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_UV;

    if (SAL_FALSE == pstJpegDevInfo->isCreate)
    {
        s32Ret = jpeg_drv_createChn(&pstJpegDevInfo->pChanHandle, &stJpegCreatePrm);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("Chn %d Jpeg_drvStart Failed !!!\n", u32Dev);
            return SAL_FAIL;
        }
    }

    pstJpegDevInfo->isCreate = SAL_TRUE;

    return SAL_SOK;
}

/**
 * @function   jpeg_setVdecDupPrm
 * @brief      设置解码dup通道的属性
 * @param[in]  void *pChnHandle : 通道句柄
 * @param[in]  void *pstDupCopyInfo: dup通道拷贝信息
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_setVdecDupPrm(void *pChnHandle, void *pstDupCopyInfo)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    PARAM_INFO_S stDupPrm = {0};
    INT32 s32Ret = SAL_FAIL;

    if ((NULL == pstDupCopyInfo) || (NULL == pChnHandle))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegDevInfo = (JPEG_TSK_DEV_INFO_S *)pChnHandle;
    pstDupOutChnHandle = (DUP_ChanHandle *)pstJpegDevInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        JPEG_LOGE("dup null!!!\n");
        return SAL_FAIL;
    }

    stDupPrm.enType = IMAGE_SIZE_CFG;
    stDupPrm.stImgSize.u32Width = pstJpegDevInfo->u32Width;
    stDupPrm.stImgSize.u32Height = pstJpegDevInfo->u32Height;
    
    JPEG_LOGD("wwww set Vdec dup W%d H%d!!!\n",pstJpegDevInfo->u32Width,pstJpegDevInfo->u32Height);

    if (NULL == pstDupOutChnHandle->dupOps.OpDupSetBlitPrm)
    {
        JPEG_LOGE("dup fun: OpDupPutBlit null!!!\n");
        return SAL_FAIL;
    }

    s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("dup_task_setDupParam err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   jpeg_getDupData
 * @brief      从dup通道拷贝一帧数据，需要使用平台相关的结构体去拷贝
 * @param[in]  void *pChnHandle 抓图通道句柄
 * @param[in]  void *pstDupCopyInfo dup拷贝信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_getDupData(void *pChnHandle, void *pstDupCopyInfo)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    PARAM_INFO_S stDupPrm = {0};
    INT32 s32Ret = SAL_FAIL;

    if ((NULL == pstDupCopyInfo) || (NULL == pChnHandle))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegDevInfo = (JPEG_TSK_DEV_INFO_S *)pChnHandle;
    pstDupOutChnHandle = (DUP_ChanHandle *)pstJpegDevInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        JPEG_LOGE("dup null!!!\n");
        return SAL_FAIL;
    }

    stDupPrm.enType = IMAGE_SIZE_CFG;
    stDupPrm.stImgSize.u32Width = pstJpegDevInfo->u32Width;
    stDupPrm.stImgSize.u32Height = pstJpegDevInfo->u32Height;
    
    JPEG_LOGD("wwww set dup W%d H%d!!!\n",pstJpegDevInfo->u32Width,pstJpegDevInfo->u32Height);

    if (NULL == pstDupOutChnHandle->dupOps.OpDupSetBlitPrm)
    {
        JPEG_LOGE("dup fun: OpDupPutBlit null!!!\n");
        return SAL_FAIL;
    }

    s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm(pstDupOutChnHandle, &stDupPrm);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("dup_task_setDupParam err !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != pstDupOutChnHandle->dupOps.OpDupGetBlit((Ptr)pstDupOutChnHandle, pstDupCopyInfo))
    {
        JPEG_LOGE("DupGetBlit err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   jpeg_putDupData
 * @brief      释放dup资源
 * @param[in]  void *pChnHandle 抓图通道句柄
 * @param[in]  void *pstDupCopyInfo dup拷贝信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 jpeg_putDupData(void *pChnHandle, void *pstDupCopyInfo)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;

    if ((NULL == pstDupCopyInfo) || (NULL == pChnHandle))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstJpegDevInfo = (JPEG_TSK_DEV_INFO_S *)pChnHandle;
    pstDupOutChnHandle = (DUP_ChanHandle *)pstJpegDevInfo->pDupHandle;
    if (NULL == pstDupOutChnHandle)
    {
        JPEG_LOGE("dup null!!!\n");
        return SAL_FAIL;
    }

    if (NULL == pstDupOutChnHandle->dupOps.OpDupPutBlit)
    {
        JPEG_LOGE("dup fun: OpDupPutBlit null!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != pstDupOutChnHandle->dupOps.OpDupPutBlit((Ptr)pstDupOutChnHandle, pstDupCopyInfo))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}



	/* Open output file */
	//	exit(1);

	/* Write buffer to output file */


/**
 * @function   jpeg_tsk_processThread
 * @brief      编码抓图线程
 * @param[in]  void *pPrm 设备id
 * @param[out] None
 * @return     void *
 */
void *jpeg_tsk_processThread(void *pPrm)
{
    UINT32 u32Dev = (UINT32)((PhysAddr)pPrm);
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT64 u64Time = 0;
    UINT32 u32GetData = 0;
    UINT32 u32SleepTime = 0;
    UINT32 u32SyncNum = 0;
    UINT32 u32NeedFree = 0;

    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    SYSTEM_FRAME_INFO stSysFrameInfo = {0};
    JPEG_COMMON_ENC_PRM_S stJpegEncPrm = {0};
    DUP_COPY_DATA_BUFF stDupCopyBuff = {0};
    STREAM_ELEMENT stStreamEle = {0};
    JPEG_SNAP_CB_RESULT_ST stJpegResult = {0};
    CAPB_JPEG *pstCapbJpeg = NULL;
    CAPB_PRODUCT *pstCapbPlat = NULL;

    if (JPEG_MAX_TSK_NUM <= u32Dev)
    {
        JPEG_LOGE("prm err!!!\n");
        return NULL;
    }

    pstCapbJpeg = capb_get_jpeg();
    if (pstCapbJpeg == NULL)
    {
        JPEG_LOGE("null err\n");
        return NULL;
    }

    pstCapbPlat = capb_get_product();
    if (pstCapbPlat == NULL)
    {
        JPEG_LOGE("null err\n");
        return NULL;
    }

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32Dev];

    /* SYSTEM_FRAME_INFO stSysFrameInfo; */

    if (pstJpegDevInfo->stSysFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&pstJpegDevInfo->stSysFrame);
        if (SAL_FAIL == s32Ret)
        {
            JPEG_LOGE("alloc video frm info err\n");
            return NULL;
        }
    }

    memset(&stSysFrameInfo, 0x00, sizeof(SYSTEM_FRAME_INFO));
    s32Ret = sys_hal_allocVideoFrameInfoSt(&stSysFrameInfo);
    if (SAL_FAIL == s32Ret)
    {
        JPEG_LOGE("alloc video frm info err\n");
        return NULL;
    }

    JPEG_LOGW("u32Dev %d\n", u32Dev);
    stDupCopyBuff.stDupDataCopyPrm.uiRotate = 0xFFFF;
    stDupCopyBuff.pstDstSysFrame = &stSysFrameInfo;
    SAL_SET_THR_NAME();

    while (1)
    {
        SAL_mutexLock(pstJpegDevInfo->mutexHandle);
        if (SAL_FALSE == pstJpegDevInfo->isStart)
        {
            break; /* 退出线程 */
        }

        /* 记录开始抓图时间 */
        u64Time = SAL_getTimeOfJiffies();
        if (u32Dev < pstCapbJpeg->videv_cnt)
        {
            s32Ret = jpeg_getDupData(pstJpegDevInfo, &stDupCopyBuff);
        }
        else
        {
            s32Ret = jpeg_setVdecDupPrm(pstJpegDevInfo, &stDupCopyBuff);
            /*解码时时抓图可能失败，尝试3次*/
            for(i = 0; i < 3; i++)
            {
                s32Ret = vdec_tsk_GetVdecFrame(u32Dev - pstCapbJpeg->videv_cnt, 2, VDEC_JPEG_GET_FRAME, &u32NeedFree, &pstJpegDevInfo->stSysFrame, NULL);
                if (SAL_isSuccess(s32Ret))
                {
                     break;
                }
                JPEG_LOGW("vdec_tsk_GetVdecFrame chn = %d err\n", u32Dev - pstCapbJpeg->videv_cnt);
            }
        
        }

        if (SAL_isSuccess(s32Ret))
        {
            stJpegEncPrm.OutWidth = pstJpegDevInfo->u32Width;
            stJpegEncPrm.OutHeight = pstJpegDevInfo->u32Height;
            stJpegEncPrm.quality = pstJpegDevInfo->u32Quality;
            stJpegEncPrm.pOutJpeg = pstJpegDevInfo->pJpeg + pstJpegDevInfo->u32Offset;

            if (u32Dev >= pstCapbJpeg->videv_cnt)
            {
                /* 抓图视频先暂停，返回播放界面图像，调整宽高底层处理逻辑不一会导致图像异常，直接返回原图 */
                stJpegEncPrm.stSysFrame.uiAppData = pstJpegDevInfo->stSysFrame.uiAppData;
                stJpegEncPrm.OutWidth = pstJpegDevInfo->stSysFrame.uiDataWidth;
                stJpegEncPrm.OutHeight = pstJpegDevInfo->stSysFrame.uiDataHeight;
            }

            SAL_DEBUG("0x%llx,[%d,%d],%d,0x%p\n", stJpegEncPrm.stSysFrame.uiAppData,
                      stJpegEncPrm.OutWidth, stJpegEncPrm.OutHeight,
                      stJpegEncPrm.quality, stJpegEncPrm.pOutJpeg);
            u32GetData = SAL_TRUE;
        }
        else
        {
            u32GetData = SAL_FALSE;
            JPEG_LOGE("Jpeg_drvCopyDupData err\n");
        }

        if (SAL_TRUE == u32GetData)
        {
            s32Ret = jpeg_drv_enc(pstJpegDevInfo->pChanHandle, &stJpegEncPrm, SAL_FALSE);
            if (SAL_isSuccess(s32Ret))
            {
                memset(&stStreamEle, 0, sizeof(STREAM_ELEMENT));
                stStreamEle.type = STREAM_ELEMENT_JPEG_IMG;
                stStreamEle.chan = u32Dev;
                stJpegResult.cJpegAddr = (UINT8 *)pstJpegDevInfo->pJpeg;
                stJpegResult.uiJpegSize = stJpegEncPrm.outSize + pstJpegDevInfo->u32Offset;
                stJpegResult.uiSyncNum = u32SyncNum;
                JPEG_LOGI("chan %d jpeg size %d save success\n", u32Dev, stJpegResult.uiJpegSize);
                SystemPrm_cbFunProc(&stStreamEle, (UINT8 *)&stJpegResult, sizeof(JPEG_SNAP_CB_RESULT_ST));
            }
            else
            {
                JPEG_LOGE("u32Dev %d,jpeg_drv_enc err\n", u32Dev);
            }

            if (u32Dev < pstCapbJpeg->videv_cnt)
            {
                s32Ret = jpeg_putDupData(pstJpegDevInfo, &stDupCopyBuff);
                if (s32Ret != SAL_SOK)
                {
                    JPEG_LOGE("u32Dev %d Jpeg put Dup Data fail %d\n", u32Dev, s32Ret);
                }
            }
            else
            {
                if (u32NeedFree == 1)
                {
                    s32Ret = vdec_tsk_PutVdecFrame(u32Dev - pstCapbJpeg->videv_cnt, 2, &pstJpegDevInfo->stSysFrame);
                    if (s32Ret != SAL_SOK)
                    {
                        JPEG_LOGE("u32Dev %d Vdec put Vdec Frame fail %d\n", u32Dev, s32Ret);
                    }
                }
            }
        }

        /* 判断抓图是否继续 */
        if (JPEG_ONCE == pstJpegDevInfo->stCtrlPrm.eJpegMode)
        {
            #if 0
            /* 抓图结束销毁资源，进程进入等待状态 */
            s32Ret = Jpeg_drvDelete(u32Dev);
            if (SAL_isFail(s32Ret))
            {
                JPEG_LOGE("Jpeg_drvDelete err !!!\n");
            }

            #endif
            JPEG_LOGW("u32Dev %d will wait\n", u32Dev);
            SAL_mutexWait(pstJpegDevInfo->mutexHandle);
            SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
        }
        else
        {
            pstJpegDevInfo->stCtrlPrm.uiFinCnt++;
            if (pstJpegDevInfo->stCtrlPrm.uiFinCnt < pstJpegDevInfo->stCtrlPrm.uiCaptCnt)
            {
                u64Time = SAL_getTimeOfJiffies() - u64Time;
                u32SleepTime = (pstJpegDevInfo->stCtrlPrm.uiCaptInterval > u64Time) ? \
                               (pstJpegDevInfo->stCtrlPrm.uiCaptInterval - u64Time) : 10;

                SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
                JPEG_LOGW("u32Dev %d u32SleepTime %d\n", u32Dev, u32SleepTime);
                SAL_msleep(u32SleepTime);
            }
            else
            {
                pstJpegDevInfo->stCtrlPrm.uiFinCnt = 0;
                #if 0
                /* 抓图结束销毁通道，进程进入等待状态 */
                s32Ret = Jpeg_drvDelete(u32Dev);
                if (SAL_isFail(s32Ret))
                {
                    JPEG_LOGE("Jpeg_drvDelete err !!!\n");
                }

                #endif
                JPEG_LOGW("u32Dev %d will wait\n", u32Dev);
                SAL_mutexWait(pstJpegDevInfo->mutexHandle);
                SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
            }
        }
    }

    s32Ret = sys_hal_rleaseVideoFrameInfoSt(&stSysFrameInfo);
    if (SAL_FAIL == s32Ret)
    {
        JPEG_LOGE("free video frm info err\n");
    }

    /* 线程主动退出接口 */
    SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
    JPEG_LOGI("Jpeg Dev %d exit!\n", u32Dev);
    SAL_thrExit(NULL);

    return NULL;
}

/**
 * @function   jpeg_tsk_setAiPrm
 * @brief      设置智能参数
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_setAiPrm(UINT32 u32Dev, void *pInParam)
{
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    INT32 bEnableDraw = SAL_FALSE;

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pInParam))
    {
        JPEG_LOGE("u32Dev %d error\n", u32Dev);
        return SAL_FAIL;
    }

    bEnableDraw = *(INT32 *)pInParam;
    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32Dev];
    if (pstJpegDevInfo->isDraw == bEnableDraw)
    {
        return SAL_SOK;
    }

    SAL_mutexLock(pstJpegDevInfo->mutexHandle);

    pstJpegDevInfo->isDraw = bEnableDraw;
    SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);

    return SAL_SOK;
}

/**
 * @function   jpeg_tsk_setPrm
 * @brief      设置编码抓图命令
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_setPrm(UINT32 u32Dev, void *pInParam)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    UINT32 u32ChanId = 0;
    JPEG_ADD_INFO_ST *pstJpegAddInfo = NULL;
    JPEG_PARAM *pstJpegParam = NULL;
    CAPB_JPEG *pstCapbJpeg = NULL;

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pInParam))
    {
        JPEG_LOGE("u32Dev %d error\n", u32Dev);
        return SAL_FAIL;
    }

    pstCapbJpeg = capb_get_jpeg();
    if (pstCapbJpeg == NULL)
    {
        JPEG_LOGE("NULL ERR\n");
        return SAL_FAIL;
    }

    pstJpegAddInfo = (JPEG_ADD_INFO_ST *)pInParam;

    if (JEEG_SRCTYPE_VDEC == pstJpegAddInfo->srcType)
    {
        u32ChanId = u32Dev + pstCapbJpeg->videv_cnt;
    }
    else
    {
        u32ChanId = u32Dev;
    }

    pstJpegParam = &pstJpegAddInfo->stJpegPrm;

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32ChanId];

    SAL_mutexLock(pstJpegDevInfo->mutexHandle);

    s32Ret = jpeg_savePrm(u32ChanId, (JPEG_PARAM *)pstJpegParam);
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("Dev %d Jpeg_drvSetEncPrm Failed !!!\n", u32Dev);
        SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
    JPEG_LOGI("Jpeg Dev %d Set Success! width:%u height:%u quality:%u\n", u32Dev, pstJpegParam->width, pstJpegParam->height, pstJpegParam->quality);
    return SAL_SOK;
}

/**
 * @function   jpeg_tsk_stop
 * @brief      销毁编码抓图命令
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_stop(UINT32 u32Dev, void *pInParam)
{
    INT32 s32Ret = SAL_SOK;
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    JPEG_ADD_INFO_ST *pstJpegAddInfo = NULL;
    CAPB_JPEG *pstCapbJpeg = NULL;
    UINT32 u32ChanId = 0;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pInParam))
    {
        JPEG_LOGE("u32Dev %d error\n", u32Dev);
        return SAL_FAIL;
    }

    pstCapbJpeg = capb_get_jpeg();
    if (pstCapbJpeg == NULL)
    {
        JPEG_LOGE("NULL ERR\n");
        return SAL_FAIL;
    }

    pstJpegAddInfo = (JPEG_ADD_INFO_ST *)pInParam;

    if (JEEG_SRCTYPE_VDEC == pstJpegAddInfo->srcType)
    {
        u32ChanId = u32Dev + pstCapbJpeg->videv_cnt;
    }
    else
    {
        u32ChanId = u32Dev;
    }

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32ChanId];

    if (SAL_FALSE == pstJpegDevInfo->isStart)
    {
        JPEG_LOGW("Jpeg Dev %d hasn't started yet !!! !!!\n", u32Dev);
        return SAL_SOK;
    }

    SAL_mutexLock(pstJpegDevInfo->mutexHandle);
    if (SAL_TRUE == pstJpegDevInfo->isCreate)
    {
        s32Ret = jpeg_drv_deleteChn(pstJpegDevInfo->pChanHandle);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("Dev %d Jpeg_drvSetEncPrm Failed !!!\n", u32ChanId);
            SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
            return SAL_FAIL;
        }

        pstJpegDevInfo->pChanHandle = NULL;
    }

    pstJpegDevInfo->isCreate = SAL_FALSE;
    pstJpegDevInfo->isStart = SAL_FALSE;
    SAL_mutexSignal(pstJpegDevInfo->mutexHandle);
    SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);

    /* 等待线程退出后释放资源 */
    SAL_thrJoin(&pstJpegDevInfo->hndl);

    pstDupOutChnHandle = pstJpegDevInfo->pDupHandle;
    if (pstDupOutChnHandle)
    {
        s32Ret = pstDupOutChnHandle->dupOps.OpDupStopBlit(pstDupOutChnHandle);
        JPEG_TASK_CHECK_RET(s32Ret, SAL_SOK, "OpDupStopBlit err !!!")
    };

    /* 释放开启时申请的内存 */
    if (NULL != pstJpegDevInfo->pJpeg)
    {
        SAL_memfree(pstJpegDevInfo->pJpeg, "jpeg", "tsk");
        pstJpegDevInfo->pJpeg = NULL;
    }
    if (NULL != pstJpegDevInfo->pstSvaOut)
    {
        SAL_memfree(pstJpegDevInfo->pstSvaOut, "jpeg", "tsk");
        pstJpegDevInfo->pstSvaOut = NULL;
    }

    JPEG_LOGI("Jpeg Dev %d Stop Success!\n", u32Dev);
    return SAL_SOK;
}

/**
 * @function   jpeg_tsk_start
 * @brief      开启编码抓图命令
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_start(UINT32 u32Dev, void *pInParam)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32ChanId = 0;
    JPEG_ADD_INFO_ST *pstJpegAddInfo = NULL;
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    JPEG_PARAM          *pstJpegPrm     = NULL;
    CAPB_JPEG *pstCapbJpeg = NULL;
    CHAR szDstInstName[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;
    CHAR *pszSrcNodeName = NULL;
    DUP_ChanHandle *pstDupOutChnHandle = NULL;
    PARAM_INFO_S stParamInfo;
    
    CAPB_VDEC_DUP *capb_VdecDup = NULL;
    CAPB_DUP *capb_dup = NULL;
    INST_INFO_S *pstInst = NULL;
    CHAR szInstPreName[NAME_LEN] = {0};
    CHAR szOutNodeNm[NAME_LEN] = {0};

    if ((JPEG_MAX_TSK_NUM <= u32Dev) || (NULL == pInParam))
    {
        JPEG_LOGE("u32Dev %d error\n", u32Dev);
        return SAL_FAIL;
    }
    
    capb_VdecDup = capb_get_vdecDup();
    capb_dup     = capb_get_dup();

    pstCapbJpeg = capb_get_jpeg();
    if (pstCapbJpeg == NULL)
    {
        JPEG_LOGE("NULL ERR\n");
        return SAL_FAIL;
    }

    pstJpegAddInfo = (JPEG_ADD_INFO_ST *)pInParam;

    if (JEEG_SRCTYPE_VDEC == pstJpegAddInfo->srcType)
    {
        u32ChanId = u32Dev + pstCapbJpeg->videv_cnt;
    }
    else
    {
        u32ChanId = u32Dev;
    }

    pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[u32ChanId];
    /* VI通道的jpeg编码 */
    if (JEEG_SRCTYPE_VI == pstJpegAddInfo->srcType)
    {
        if (!pstJpegDevInfo->pDupHandle)
        {
            snprintf(szDstInstName, NAME_LEN, "VENC_JPG%d", u32ChanId);
            s32Ret = link_drvGetSrcNode(szDstInstName, "in_0", &pstSrcInst, &pszSrcNodeName);
            if (s32Ret)
            {
                JPEG_LOGE("link_drvGetSrcNode fail, s32Ret %d, chan %d !!!\n", s32Ret, u32ChanId);
                return s32Ret;
            }

            pstJpegDevInfo->pDupHandle = link_drvGetHandleFromNode(pstSrcInst, pszSrcNodeName);
        }
        
        pstDupOutChnHandle = pstJpegDevInfo->pDupHandle;
        
        if(SAL_VIDEO_DATFMT_RGB24_888 == capb_dup->enOutputSalPixelFmt)
        {
            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = PIX_FORMAT_CFG;
            
            stParamInfo.enPixFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
            s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm((Ptr)pstDupOutChnHandle, &stParamInfo);
            if (SAL_isFail(s32Ret))
            {
                DISP_LOGE("OpDupSetBlitPrm Chn err !!!\n");
                return SAL_FAIL;
            }
        }

        s32Ret = pstDupOutChnHandle->dupOps.OpDupStartBlit(pstDupOutChnHandle);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("dup_task_setDupParam err !!!\n");
            return SAL_FAIL;
        }
        
        if (pstJpegDevInfo->pstSvaOut == NULL)
        {
            pstJpegDevInfo->pstSvaOut = (SVA_PROCESS_OUT *)SAL_memMalloc(sizeof(SVA_PROCESS_OUT), "jpeg", "tsk");
            if (pstJpegDevInfo->pstSvaOut == NULL)
            {
                JPEG_LOGE("u32Dev %d svaOut NULL err\n", u32ChanId);
                SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
                return SAL_FAIL;
            }
        }
    }
    
    if ((JEEG_SRCTYPE_VDEC == pstJpegAddInfo->srcType) && (capb_VdecDup->enOutputSalPixelFmt == SAL_VIDEO_DATFMT_RGB24_888))
    {
        snprintf(szInstPreName, NAME_LEN, "DUP_VDEC_%d", u32Dev);
        pstInst = link_drvGetInst(szInstPreName);
        if (NULL == pstInst)
        {
            JPEG_LOGE("jpeg link drv GetInst fail, s32Ret %d, chan %d !!!\n", s32Ret, u32ChanId);
            return SAL_FAIL;
        }
        snprintf(szOutNodeNm, NAME_LEN, "out_jpeg");
        pstDupOutChnHandle = link_drvGetHandleFromNode(pstInst, szOutNodeNm);
        if (NULL == pstDupOutChnHandle)
        {
            JPEG_LOGE("jpeg link_drvGetHandleFromNode fail, s32Ret %d, chan %d !!!\n", s32Ret, u32ChanId);
            return SAL_FAIL;
        }
        pstJpegDevInfo->pDupHandle = pstDupOutChnHandle;

        memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
        stParamInfo.enType = PIX_FORMAT_CFG;

        stParamInfo.enPixFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
        s32Ret = pstDupOutChnHandle->dupOps.OpDupSetBlitPrm((Ptr)pstDupOutChnHandle, &stParamInfo);
        if (SAL_isFail(s32Ret))
        {
            DISP_LOGE("OpDupSetBlitPrm Chn err !!!\n");
            return SAL_FAIL;
        }

        s32Ret = pstDupOutChnHandle->dupOps.OpDupStartBlit(pstDupOutChnHandle);
        if (SAL_isFail(s32Ret))
        {
            JPEG_LOGE("OpDupStartBlit err !!!\n");
            return SAL_FAIL;
        }
        
    }

    SAL_mutexLock(pstJpegDevInfo->mutexHandle);

    /* 申请抓图内存，用于保存jpeg图片 */
    if (NULL == pstJpegDevInfo->pJpeg)
    {
        pstJpegDevInfo->pJpeg = (INT8 *)SAL_memMalloc(MAX_ENC_JPEG_SIZE + JPEG_ADD_INFO_SIZE, "jpeg", "tsk");
        if (NULL == pstJpegDevInfo->pJpeg)
        {
            JPEG_LOGE("malloc err\n");
            return SAL_FAIL;
        }
    }
    
    /* 不设置抓图参数就使用DSP默认的 */

    /* 设置了参数 */
    pstJpegPrm = (JPEG_PARAM *)&pstJpegAddInfo->stJpegPrm;
    if ((0 == pstJpegPrm->width)||(0 == pstJpegPrm->height)||(0 == pstJpegPrm->quality))
    {
        JPEG_LOGE("Chn %d Jpeg_drvStart %d*%d,%d !!!\n", u32ChanId,pstJpegPrm->width,pstJpegPrm->height, pstJpegPrm->quality);
        s32Ret = jpeg_start(u32ChanId, NULL, (INT8 *)pstJpegAddInfo->cInfo, pstJpegAddInfo->uiSize);
        
    }
    else
    {
        s32Ret = jpeg_start(u32ChanId, pstJpegPrm, (INT8 *)pstJpegAddInfo->cInfo, pstJpegAddInfo->uiSize);
    }
    
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("Chn %d Jpeg_drvStart Failed !!!\n", u32ChanId);
        SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
        return SAL_FAIL;
    }

    if (SAL_FALSE == pstJpegDevInfo->isStart)
    {
        pstJpegDevInfo->isStart = SAL_TRUE;
        SAL_thrCreate(&pstJpegDevInfo->hndl, jpeg_tsk_processThread, SAL_THR_PRI_DEFAULT, 0, (void *)((PhysAddr)u32ChanId));
    }
    else
    {
        SAL_mutexSignal(pstJpegDevInfo->mutexHandle);
    }

    SAL_mutexUnlock(pstJpegDevInfo->mutexHandle);
    JPEG_LOGI("Jpeg Dev %d ,srcType %d,Start Success!\n", u32Dev, pstJpegAddInfo->srcType);
    return SAL_SOK;
}

/**
 * @function   jpeg_tsk_init
 * @brief      Jpeg tsk层初始化资源
 * @param[in]  None
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_init(void)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 k = 0;
    JPEG_TSK_DEV_INFO_S *pstJpegDevInfo = NULL;
    CAPB_JPEG *pstCapbJpeg = NULL;
    CHAR szInstName[NAME_LEN] = {0};
    INST_INFO_S *pstInst = NULL;
    NODE_CFG_S stNodeCfg[] = {
        {IN_NODE_0, "in_0", NODE_BIND_TYPE_GET},
    };

    pstCapbJpeg = capb_get_jpeg();
    if (pstCapbJpeg == NULL)
    {
        JPEG_LOGE("null err\n");
        return SAL_FAIL;
    }

    memset(&gstJpegTskCtrl, 0, sizeof(JPEG_TSK_CTRL_S));
    for (i = 0; i < pstCapbJpeg->maxdev_num; i++)
    {
        pstJpegDevInfo = &gstJpegTskCtrl.stJpegDevInfo[i];
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstJpegDevInfo->mutexHandle);

        /*enc和dec要不要分开处理？这里只处理enc相关的抓图*/
        if (i < pstCapbJpeg->videv_cnt)
        {
            snprintf(szInstName, NAME_LEN, "VENC_JPG%d", i);
            pstInst = link_drvReqstInst();
            if (pstInst)
            {
                s32Ret = link_drvInitInst(pstInst, szInstName);
                if (SAL_FAIL == s32Ret)
                {
                    VENC_LOGE("link node init err\n");
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

    /* drv层资源初始化 */
    s32Ret = jpeg_drv_init();
    if (SAL_isFail(s32Ret))
    {
        JPEG_LOGE("!!!\n");
        return SAL_FAIL;
    }
    VENC_LOGW("---------jpeg init finish!---------\n");

    return SAL_SOK;
}

