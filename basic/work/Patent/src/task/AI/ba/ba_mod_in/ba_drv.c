/*******************************************************************************
* ba_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年9月5日 Create
*
* Description :
* Modification:
*******************************************************************************/


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "ba_drv.h"
#include "dup_tsk.h"
#include "system_common_api.h"
#include "disp_hal_api.h"
#include "vdec_hal_api.h"
#include "system_prm_api.h"

#include "vdec_tsk_api.h"
#include "sva_out.h"
#include "vgs_drv_api.h"
#include "sys_tsk.h"
#include "dspcommon.h"


//#include "../../platform_sdk.h"

#include "vdec_tsk_api.h"

/* ========================================================================== */
/*                             宏定义区                                       */
/* ========================================================================== */

#define BA_DRV_CHECK_CHAN(chan, value) {if (chan > 3) {BA_LOGE("Input BA Chan[%d] is Invalid!\n", chan); return (value); }}
#define BA_DRV_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            BA_LOGE("%s \n", str); \
            goto loop; \
        } \
    }

#define BA_DRV_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            BA_LOGE("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define BA_DRV_CHECK_RET_NO_LOOP(ret, str) \
    { \
        if (ret) \
        { \
            BA_LOGE("%s, ret: 0x%x \n", str, ret); \
        } \
    }
#define GET_YUV_SIZE(w, h)				(w * h * 3 / 2)
#define GET_LUMA_SIZE(w, h)				(w * h)
#define FILTER_REPEAT_REAULT(a, b, c)	(a < b ? (b - a) > c : (a - b) > c)
#define BA_FILTER_TIMES         (25)
#define BA_INPUT_FRAME_GAP_MS	(133)
#define BA_INPUT_FRAME_RATE		(1000 / BA_INPUT_FRAME_GAP_MS)

#define ABNORMAL_BEHAVIOR_NUM	(6)
#define SBAE_RESULT_NODE_TYPE	(2901)  /* 该值与特定业务相关，为引擎内部数据，临时使用宏定义处理 */
#define BA_FILTER_FLAG_TRUE		(1)
#define BA_FILTER_FLAG_FALSE	(0)

extern INT32 Venc_drvDrawCorner(UINT32 uiX, UINT32 uiY, UINT32 uiW, UINT32 uiH,
                                VGS_DRAW_PRM_S *astVgsDrawLine, UINT32 uiPicW, UINT32 uiPicH, UINT32 uiColor);

/* ========================================================================== */
/*                             数据结构区                                     */
/* ========================================================================== */
static BA_COMMON g_ba_common =
{
    .uiDevCnt = 0,
};
static VGS_RECT_ARRAY_S stBaVgsDrawRect = {0};

static UINT32 auiLevel2Thr[BA_DETECT_LEVEL_NUM] = {10, 6, 3};     /* 当前需求要求低中高三个等级，告警次数分别是10、6、3 */

/* ========================================================================== */
/*                             函数定义区                                     */
/* ========================================================================== */
/* extern INT32 Venc_drvDrawLine(UINT32 uiX, UINT32 uiY, UINT32 uiW, UINT32 uiH, VGS_TASK_ATTR_S *pstTask, UINT32 uiPicW, UINT32 uiPicH,const UINT32 uiColor); */

/**
 * @function:   Ba_DrvGetDev
 * @brief:      获取通道全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     BA_DEV_PRM *
 */
static BA_DEV_PRM *Ba_DrvGetDev(UINT32 chan)
{
    BA_DRV_CHECK_CHAN(chan, NULL);

    return &g_ba_common.stBa_dev[chan];
}

/**
 * @function:   Ba_drvChnSignal
 * @brief:      通道信号量+1
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
static void *Ba_drvChnSignal(void *prm)
{
    BA_DEV_PRM *pstBa_dev = NULL;

    BA_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstBa_dev = (BA_DEV_PRM *)prm;

    SAL_mutexLock(pstBa_dev->mChnMutexHdl);

    if (SAL_FALSE == pstBa_dev->uiChnStatus)
    {
        SAL_mutexSignal(pstBa_dev->mChnMutexHdl);
    }

    SAL_mutexUnlock(pstBa_dev->mChnMutexHdl);

err:
    return NULL;
}

/**
 * @function:   Ba_drvChnWait
 * @brief:      通道信号量-1
 * @param[in]:  BA_DEV_PRM *pstBa_dev
 * @param[out]: None
 * @return:     void *
 */
static void *Ba_drvChnWait(BA_DEV_PRM *pstBa_dev)
{
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    SAL_mutexLock(pstBa_dev->mChnMutexHdl);

    if (SAL_FALSE == pstBa_dev->uiChnStatus)
    {
        SAL_mutexWait(pstBa_dev->mChnMutexHdl);
    }

    SAL_mutexUnlock(pstBa_dev->mChnMutexHdl);

err:
    return NULL;
}

/**
 * @function:   Ba_DrvPutQueBuf
 * @brief:      回收队列Buf
 * @param[in]:  DSA_QueHndl *pstBufQue
 * @param[in]:  void *pstYuvFrame
 * @param[in]:  UINT32 uiTimeOut
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvPutQueBuf(DSA_QueHndl *pstBufQue, void *pstYuvFrame, UINT32 uiTimeOut)
{
    BA_DRV_CHECK_PTR(pstBufQue, err, "pstBufQue == null!");
    BA_DRV_CHECK_PTR(pstYuvFrame, err, "pstYuvFrame == null!");

    DSA_QuePut(pstBufQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);   /* 默认回收队列数据即刻返回 */

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetQueBuf
 * @brief:      获取空闲队列Buf
 * @param[in]:  DSA_QueHndl *pstBufQue
 * @param[in]:  void **ppstYuvFrame
 * @param[in]:  UINT32 uiTimeOut
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvGetQueBuf(DSA_QueHndl *pstBufQue, void **ppstYuvFrame, UINT32 uiTimeOut)
{
    BA_DRV_CHECK_PTR(pstBufQue, err, "pstBufQue == null!");

    DSA_QueGet(pstBufQue, (void **)ppstYuvFrame, SAL_TIMEOUT_NONE);    /* BA_TODO: 暂定接口不阻塞，后续考虑增加超时 */

    if (uiTimeOut)
    {
        /* BA_TODO:  */
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetCfgParam
 * @brief:      从队列中获取参数
 * @param[in]:  BA_QUEUE_PRM *pstQuePrm
 * @param[in]:  BA_PROCESS_IN *pstIn
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvGetCfgParam(UINT32 chan, BA_PROCESS_IN *pstIn)
{
    BA_DEV_PRM *pstBa_dev = NULL;

    BA_DRV_CHECK_PTR(pstIn, err, "pstIn == null!");
    
    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    sal_memcpy_s(pstIn, sizeof(BA_PROCESS_IN), &pstBa_dev->stIn, sizeof(BA_PROCESS_IN));

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvUpdateOutChnResolution
 * @brief:      更新解码（vpss）输出分辨率
 * @param[in]:  UINT32 vdecChn
 * @return:     INT32
 */
static INT32 Ba_DrvUpdateOutChnResolution(UINT32 vdecChn)
{
    INT32 s32Ret = SAL_FAIL;
    IA_UPDATE_OUTCHN_PRM stOutChnPrm = {0};

    stOutChnPrm.uiVdecChn = vdecChn;
    stOutChnPrm.enModule = IA_MOD_BA;
    stOutChnPrm.uiWidth = BA_CHN_WIDTH;
    stOutChnPrm.uiHeight = BA_CHN_HEIGHT;   /* 等比例缩放，暂时手动修改 */
    stOutChnPrm.uiVpssChn = 3;
    
    s32Ret = IA_UpdateOutChnResolution(&stOutChnPrm);
    BA_DRV_CHECK_RET(s32Ret, err, "IA_UpdateOutChnResolution failed!");

    return SAL_SOK;

err:
    return SAL_FAIL;

}
/**
 * @function:   Ba_DrvGetFrame
 * @brief:      获取解码通道数据
 * @param[in]:  UINT32 vdecChn
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrame
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ba_DrvGetFrame(BA_DEV_PRM *pstBa_dev, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uiVdecChn = 0;
    UINT32 u32NeedFree = 0;

    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    BA_DRV_CHECK_PTR(pstBa_dev, exit, "pstBa_dev == null!");
    BA_DRV_CHECK_PTR(pstFrame, exit, "pstFrame == null!");

    uiVdecChn = pstBa_dev->uiVdecChn;

    pstSysFrameInfo = &pstBa_dev->stDecFrameInfo;

    SAL_clear((PhysAddr *)pstSysFrameInfo->uiAppData);
#if 1
    s32Ret = vdec_tsk_GetVdecFrame(uiVdecChn, BA_VPSS_CHN_ID, VDEC_BA_GET_FRAME, &u32NeedFree,pstSysFrameInfo,NULL);
	if (SAL_SOK != s32Ret)
	{
		/* 此处不能加打印，目前sd应用存在没有码流也开启行为分析的sd操作，会刷屏 */
		goto exit;
	}
    s32Ret = sys_hal_getVideoFrameInfo(pstSysFrameInfo, &stVideoFrmBuf);
    BA_DRV_CHECK_RET(s32Ret,err,"sys_hal_getVideoFrameInfo failed");

	BA_LOGD(" the frame dataFormat is %d\n",stVideoFrmBuf.frameParam.dataFormat);

	/* 更新解码通道宽高 */
    if (stVideoFrmBuf.frameParam.width != BA_CHN_WIDTH || stVideoFrmBuf.frameParam.height != BA_CHN_HEIGHT)
    {
    	BA_LOGW("vdecChn %d, outChn %d, w %d, h %d, not %d_%d! need update! \n",
							  uiVdecChn, BA_VPSS_CHN_ID, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height,
							  BA_CHN_WIDTH, BA_CHN_HEIGHT);
        s32Ret = Ba_DrvUpdateOutChnResolution(uiVdecChn);
		BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvUpdateOutChnResolution failed");

		s32Ret = vdec_tsk_SetOutChnDataFormat(uiVdecChn, BA_VPSS_CHN_ID, SAL_VIDEO_DATFMT_YUV420SP_VU);
		if (SAL_SOK != s32Ret)
		{
			BA_LOGE("update ba vdecChn [%d] data format failed! \n", uiVdecChn);
			goto err;
		}
        /* 释放不满足条件的解码帧数据 */
        goto err;
    }
#if 0
		static CHAR path[64] = {0};
		static UINT32 count = 0;
		SAL_clear(path);

		sprintf(path, "./ba_jpg/_input_%dx%d_%d.yuv", BA_CHN_WIDTH,BA_CHN_HEIGHT,count);

		(VOID)Ba_HalDebugDumpData((CHAR *)(PhysAddr)stVideoFrmBuf.virAddr[0],
					  (CHAR *)path, GET_YUV_SIZE(BA_CHN_WIDTH, BA_CHN_HEIGHT), 1);
		count++;
		/* goto err;  / * debug * / */
#endif


    s32Ret = Ia_TdeQuickCopy(pstSysFrameInfo, pstFrame, 0, 0, stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.height, SAL_FALSE);
    BA_DRV_CHECK_RET(s32Ret, err, "Ia_TdeQuickCopy failed!");

	if (u32NeedFree)
	{
		s32Ret = vdec_tsk_PutVdecFrame(uiVdecChn, BA_VPSS_CHN_ID,pstSysFrameInfo);
        BA_DRV_CHECK_RET(s32Ret,err,"vdec_tsk_PutVdecFrame failed");
	}
#endif	
#if 0 //调试使用，后期无用则删除
		do
			{
				s32Ret = vdec_tsk_GetVdecFrame(uiVdecChn,
											   BA_VPSS_CHN_ID,
											   VDEC_BA_GET_FRAME,
											   &u32NeedFree,
											   pstSysFrameInfo,
											   NULL);
				if (SAL_SOK != s32Ret)
				{
				}
		
				(VOID)sys_hal_getVideoFrameInfo(pstSysFrameInfo, &stVideoFrmBuf);
				BA_LOGW(" the frame dataFormat is %d\n",stVideoFrmBuf.frameParam.dataFormat);
		
				if (BA_CHN_WIDTH != stVideoFrmBuf.frameParam.width || BA_CHN_HEIGHT != stVideoFrmBuf.frameParam.height)
				{
					BA_LOGW("vdecChn %d, outChn %d, w %d, h %d, not %d_%d! need update! \n",
							  uiVdecChn, BA_VPSS_CHN_ID, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height,
							  BA_CHN_WIDTH, BA_CHN_HEIGHT);
		
					s32Ret = Ba_DrvUpdateOutChnResolution(uiVdecChn);
					if (SAL_SOK != s32Ret)
					{
						BA_LOGE("Update Face Chn Resolution Failed! \n ");
					}
		
					s32Ret = vdec_tsk_SetOutChnDataFormat(uiVdecChn, BA_VPSS_CHN_ID, SAL_VIDEO_DATFMT_YUV420SP_VU);
					if (SAL_SOK != s32Ret)
					{
						BA_LOGE("update face login vdecChn [%d] data format failed! \n", uiVdecChn);
					}
		
					if (u32NeedFree)
					{
						/* 释放通道帧Buf */
						s32Ret = vdec_tsk_PutVdecFrame(uiVdecChn, BA_VPSS_CHN_ID, pstSysFrameInfo);
						if (SAL_SOK != s32Ret)
						{
							BA_LOGE("Vdec %d Put Frame fail!\n", uiVdecChn);
						}
					}
		
					BA_LOGI("Update uiVdecChn %d Resolution Success! format %d \n", uiVdecChn, SAL_VIDEO_DATFMT_YUV420SP_VU);
				}
			}
			while (BA_CHN_WIDTH != stVideoFrmBuf.frameParam.width || BA_CHN_HEIGHT != stVideoFrmBuf.frameParam.height);	
#endif

    return SAL_SOK;
err:
    if (u32NeedFree)
    {
        (VOID)vdec_tsk_PutVdecFrame(uiVdecChn, BA_VPSS_CHN_ID, pstSysFrameInfo);
    }
exit:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetVersion
 * @brief:      获取版本号信息
 * @param[in]:  UINT32 chan
 * @param[out]:  CHAR *pcVerInfo
 * @return:     INT32
 */
INT32 Ba_DrvGetVersion(IN UINT32 chan, OUT CHAR *pcVerInfo)
{
    BA_SUB_MOD_S *pstBaSubMod = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(pcVerInfo, err, "pcVerInfo == null!")

    pstBaSubMod = Ba_HalGetSubModPrm(0);
    BA_DRV_CHECK_PTR(pstBaSubMod, err, "pstBaSubMod == null!");

    sal_memcpy_s(pcVerInfo, BA_VERSION_INFO_LEN, pstBaSubMod->acVerInfo, BA_VERSION_INFO_LEN);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvSetDetectLevel
 * @brief:      设置异常行为检测级别
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvSetDetectLevel(UINT32 chan, void *prm)
{
    UINT32 uiDetLevel = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstBa_dev = Ba_DrvGetDev(0);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstPrmEmptQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstEmptQue;
    pstPrmFullQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstFullQue;

    BA_DRV_CHECK_PTR(pstPrmEmptQue, err, "pstPrmEmptQue == null!");
    BA_DRV_CHECK_PTR(pstPrmFullQue, err, "pstPrmFullQue == null!");

    uiDetLevel = *(UINT32 *)prm;
    if (uiDetLevel > BA_DETECT_LEVEL_NUM - 1)
    {
        BA_LOGE("Invalid level %d \n", uiDetLevel);
        goto err;
    }

    pstBa_dev->stIn.stAbnorBehavCntPrm.uiUpFlag = SAL_TRUE;
    pstBa_dev->stIn.stAbnorBehavCntPrm.uiAbnorBehavCnt = auiLevel2Thr[uiDetLevel];

    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);

    sal_memcpy_s(pstIn, sizeof(BA_PROCESS_IN), &pstBa_dev->stIn, sizeof(BA_PROCESS_IN));

    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    BA_LOGI("set det level %d end! chan %d \n", uiDetLevel, chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvSetDetectRegion
 * @brief:      设置行为分析检测区域
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvSetDetectRegion(UINT32 chan, void *prm)
{
    BA_DEV_PRM *pstBa_dev = NULL;
    SVA_RECT_F *pstRectInfo = NULL;
    BA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstRectInfo = (SVA_RECT_F *)prm;

    pstBa_dev = Ba_DrvGetDev(0);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstPrmEmptQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstEmptQue;
    pstPrmFullQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstFullQue;

    BA_DRV_CHECK_PTR(pstPrmEmptQue, err, "pstPrmEmptQue == null!");
    BA_DRV_CHECK_PTR(pstPrmFullQue, err, "pstPrmFullQue == null!");

	if (pstRectInfo->x < 0 || pstRectInfo->y < 0 || pstRectInfo->width < 0 || pstRectInfo->height < 0)
    {
        BA_LOGE("Invalid Prm, Pls Check! x %.4f y %.4f w %.4f h %.4f\n",
                pstRectInfo->x, pstRectInfo->y, pstRectInfo->width, pstRectInfo->height);
        return SAL_FAIL;
    }

    if (pstRectInfo->x > 1.0 || pstRectInfo->y > 1.0 || pstRectInfo->width > 1.0 || pstRectInfo->height > 1.0 || pstRectInfo->x + pstRectInfo->width> 1.0|| pstRectInfo->y + pstRectInfo->height > 1.0)
    {
        BA_LOGE("Invalid Prm, Pls Check! x %.4f y %.4f w %.4f h %.4f x+w %.4f y+h %.4f\n",
                pstRectInfo->x, pstRectInfo->y, pstRectInfo->width, pstRectInfo->height,pstRectInfo->x + pstRectInfo->width,pstRectInfo->y + pstRectInfo->height);
        return SAL_FAIL;
    }

    if ((pstBa_dev->stIn.rect.x == pstRectInfo->x)
        && (pstBa_dev->stIn.rect.y == pstRectInfo->y)
        && (pstBa_dev->stIn.rect.width == pstRectInfo->width)
        && (pstBa_dev->stIn.rect.height == pstRectInfo->height))
    {
        BA_LOGI("BA Module: Same Detect Region, return Success!\n");
        return SAL_SOK;
    }

    pstBa_dev->stIn.rect.x = pstRectInfo->x;
    pstBa_dev->stIn.rect.y = pstRectInfo->y;
    pstBa_dev->stIn.rect.width = pstRectInfo->width;
    pstBa_dev->stIn.rect.height = pstRectInfo->height;

    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);

    sal_memcpy_s(pstIn, sizeof(BA_PROCESS_IN), &pstBa_dev->stIn, sizeof(BA_PROCESS_IN));

    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    BA_LOGI("BA Module: Set Detect Region [%4f,%4f] [%4f,%4f] OK!\n",
            pstBa_dev->stIn.rect.x, pstBa_dev->stIn.rect.y,
            pstBa_dev->stIn.rect.width, pstBa_dev->stIn.rect.height);

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvSetSensity
 * @brief:      设置行为分析检测灵敏度(1~5)
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 ATTRIBUTE_UNUSED Ba_DrvSetSensity(UINT32 chan, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiSensity = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    BA_PROCESS_IN *pstIn = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(prm, err, "prm == null!");

    uiSensity = *(UINT32 *)prm;

    pstBa_dev = Ba_DrvGetDev(0);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    if (SAL_TRUE != pstBa_dev->uiChnStatus)
    {
        BA_LOGE("BA Module is closed! No need set sensity, pls check!\n");
        return SAL_FAIL;
    }

    if (0 == uiSensity || uiSensity > 5)
    {
        BA_LOGE("BA Module: Invalid uiSensity %d! Use default value %d. Please check.\n", uiSensity, 1);
        return SAL_SOK;
    }

    pstPrmEmptQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstEmptQue;
    pstPrmFullQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstFullQue;

    BA_DRV_CHECK_PTR(pstPrmEmptQue, err, "pstPrmEmptQue == null!");
    BA_DRV_CHECK_PTR(pstPrmFullQue, err, "pstPrmFullQue == null!");

    pstBa_dev->stIn.uiSensity = uiSensity;

    uiSensity = uiSensity >= BA_MAX_SENSITY + 1    \
                ? BA_MIN_SENSITY    \
                : uiSensity < BA_MIN_SENSITY ? BA_MIN_SENSITY : BA_MAX_SENSITY + 1 - uiSensity;

    if (pstBa_dev->stIn.uiSensity == uiSensity)
    {
        BA_LOGI("BA Module: Same sensity [%d], return Success!\n", uiSensity);
        return SAL_SOK;
    }

    s32Ret = Ba_HalSetIcfCfg(0, SENSITY_TYPE, &uiSensity);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalSetIcfCfg failed!");

    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);

    sal_memcpy_s(pstIn, sizeof(BA_PROCESS_IN), &pstBa_dev->stIn, sizeof(BA_PROCESS_IN));

    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    BA_LOGI("BA Module: Set uiSensity [%d] OK! \n", pstIn->uiSensity);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvCaptSwitch
 * @brief:      通道抓拍参数（暂未开启）
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiSwitch
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvCaptSwitch(UINT32 chan, UINT32 uiSwitch)
{
    BA_DEV_PRM *pstBa_dev = NULL;
    BA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;

    if (uiSwitch > 1)
    {
        BA_LOGE("Invalid BA switch flag %d > %d!\n", uiSwitch, 1);
        return SAL_FAIL;
    }

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstPrmEmptQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstEmptQue;
    pstPrmFullQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstFullQue;

    BA_DRV_CHECK_PTR(pstPrmEmptQue, err, "pstPrmEmptQue == null!");
    BA_DRV_CHECK_PTR(pstPrmFullQue, err, "pstPrmFullQue == null!");

    if (pstBa_dev->stIn.uiCapt == uiSwitch)
    {
        BA_LOGI("BA Module: Same Capt Status [%d], return Success!\n", uiSwitch);
        return SAL_SOK;
    }

    pstBa_dev->stIn.uiCapt = uiSwitch;

    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);

    sal_memcpy_s(pstIn, sizeof(BA_PROCESS_IN), &pstBa_dev->stIn, sizeof(BA_PROCESS_IN));

    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    BA_LOGI("BA Module: Set Capt [%d] OK!\n", pstBa_dev->stIn.uiCapt);
    return SAL_SOK;

err:
    return SAL_FAIL;
}


/**
 * @function:   Ba_DrvPutInputData
 * @brief:      释放空闲的input data节点
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 *puiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvPutInputData(UINT32 chan, UINT32 uiIdx)
{
    BA_DEV_PRM *pstBa_dev = NULL;
    BA_INPUT_PRM_S *pstBaInputPrm = NULL;
    BA_INPUT_DATA_S *pstBaInputData = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstBaInputPrm = &pstBa_dev->stBaInputPrm;

    if (uiIdx >= pstBaInputPrm->uiCnt)
    {
        BA_LOGE("Invalid index[%d] >= Max[%d], chan %d \n", uiIdx, pstBaInputPrm->uiCnt, chan);
        return SAL_FAIL;
    }

    pstBaInputData = pstBaInputPrm->pstBaInputData[uiIdx];
    BA_DRV_CHECK_PTR(pstBaInputData, err, "pstBaInputData == null!");

    if (SAL_TRUE != pstBaInputData->uiUseFlag)
    {
        BA_LOGW("input data is freed! \n");
    }

    pstBaInputData->uiUseFlag = SAL_FALSE;

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetInputData
 * @brief:      获取空闲的input data节点
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static BA_INPUT_DATA_S *
 */
static BA_INPUT_DATA_S *Ba_DrvGetInputData(UINT32 chan)
{
    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_INPUT_PRM_S *pstBaInputPrm = NULL;
    BA_INPUT_DATA_S *pstBaInputData = NULL;

    BA_DRV_CHECK_CHAN(chan, NULL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstBaInputPrm = &pstBa_dev->stBaInputPrm;

    for (i = 0; i < pstBaInputPrm->uiCnt; i++)
    {
        pstBaInputData = pstBaInputPrm->pstBaInputData[i];
        BA_DRV_CHECK_PTR(pstBaInputData, err, "pstBaInputData == null!");

        if (SAL_TRUE != pstBaInputData->uiUseFlag)
        {
            Ba_HalPrint("Get input data success!");
            pstBaInputData->uiUseFlag = SAL_TRUE;
            break;
        }
    }

    if (i >= pstBaInputPrm->uiCnt)
    {
        BA_LOGE("get input data fail! chan %d i %d uiCnt %d\n", chan, i, pstBaInputPrm->uiCnt);
        return NULL;
    }

    return pstBaInputData;

err:
    return NULL;
}

/**
 * @function:   Ba_DrvInputDataToEngine
 * @brief:      引擎推帧接口
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInputDataToEngine(UINT32 chan, SYSTEM_FRAME_INFO *pstSysFrmInfo)
{
    INT32 s32Ret = SAL_SOK;

    BA_DEV_PRM *pstBa_dev = NULL;

    BA_SUB_MOD_S *pstBaSubModPrm = NULL;
    VOID *pSubModHandle = NULL;
    BA_INPUT_DATA_S *pstBaInputData = NULL;
    SBAE_INPUT_INFO_T *pstSbaInputData = NULL;
    BA_MOD_S *pstBaModPrm = NULL;
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    pstBaModPrm = Ba_HalGetModPrm();
    BA_DRV_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(pstSysFrmInfo, err, "pstSysFrmInfo == null!");

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == NULL");

    pstBaSubModPrm = Ba_HalGetSubModPrm(chan);
    BA_DRV_CHECK_PTR(pstBaSubModPrm, err, "pstBaSubModPrm == NULL");

    pSubModHandle = pstBaSubModPrm->stHandle.pChannelHandle;
    BA_DRV_CHECK_PTR(pSubModHandle, err, "pSubModHandle == NULL");

    pstBaInputData = Ba_DrvGetInputData(chan);
    BA_DRV_CHECK_PTR(pstBaInputData, err, "pstBaInputData == NULL");

    pstSbaInputData = (SBAE_INPUT_INFO_T *)pstBaInputData->stIcfInputData.stBlobData[0].pData;
    BA_DRV_CHECK_PTR(pstSbaInputData, err, "pstSbaInputData == NULL");


    s32Ret = sys_hal_getVideoFrameInfo(pstSysFrmInfo, &stVideoFrmBuf);
    BA_DRV_CHECK_RET(s32Ret,err,"sys_hal_getVideoFrameInfo failed");


    /* 填充帧数据 */
    pstSbaInputData->pData = (void *)stVideoFrmBuf.virAddr[0];

    /* 填充时间戳和帧序号 */	
	pstBaInputData->stIcfInputData.stBlobData[0].nFrameNum = pstBa_dev->uiFrameNum++;
	pstBaInputData->stIcfInputData.stBlobData[0].nTimeStamp = 132 * pstBa_dev->uiFrameNum;

    /* 填充私有信息 */
    pstBaInputData->stIptPrvtData.uiChn = pstBaInputData->uiChn;
    pstBaInputData->stIptPrvtData.uiInputIdx = pstBaInputData->uiInputId;
    pstBaInputData->stIptPrvtData.uiInputTime = SAL_getCurMs();
    pstBaInputData->stIcfInputData.pUserPtr = (VOID *)&pstBaInputData->stIptPrvtData;

#ifdef DEBUG_LOG
    BA_LOGW("input: chan %d, idx %d, time %d, %p, frmNum %d \n",
            pstBaInputData->stIptPrvtData.uiChn, pstBaInputData->stIptPrvtData.uiInputIdx, pstBaInputData->stIptPrvtData.uiInputTime,
            pstBaInputData->stIcfInputData.pUserPtr, pstBa_dev->uiFrameNum);
#endif

	BA_LOGD("put data into engine!\n");
#if 0
	static CHAR path[64] = {0};

	SAL_clear(path);

	sprintf(path, "./ba_jpg/_input_%dx%d_%lld.yuv", BA_CHN_WIDTH,BA_CHN_HEIGHT,pstBaInputData->stIcfInputData.stBlobData[0].nFrameNum);

	(VOID)Ba_HalDebugDumpData((CHAR *)(PhysAddr)pstSbaInputData->pData,
				  (CHAR *)path, GET_YUV_SIZE(BA_CHN_WIDTH, BA_CHN_HEIGHT), 1);
	/* goto err;  / * debug * / */
#endif

    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfInputData(pSubModHandle, &pstBaInputData->stIcfInputData, sizeof(ICF_INPUT_DATA));
    BA_DRV_CHECK_RET(s32Ret,err,"Ba_IcfInputData failed");


    return SAL_SOK;

err:
    if (NULL != pstBaInputData)
    {
        (VOID)Ba_DrvPutInputData(chan, pstBaInputData->uiInputId);
    }

    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvDeInitInputDataPrm
 * @brief:      去初始化输入数据参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvDeInitInputDataPrm(UINT32 chan)
{
    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_INPUT_PRM_S *pstBaInputPrm = NULL;
    BA_INPUT_DATA_S *pstBaInputData = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstBaInputPrm = &pstBa_dev->stBaInputPrm;
    pstBaInputPrm->uiCnt = BA_INPUT_DATA_BUF_NUM;

    for (i = 0; i < pstBaInputPrm->uiCnt; i++)
    {
        pstBaInputData = pstBaInputPrm->pstBaInputData[i];
        if (NULL == pstBaInputData)
        {
            BA_LOGW("pstBaInputData is freed, continue! i %d, chan %d \n", i, chan);
            continue;
        }

        (VOID)Ba_HalFree((VOID *)pstBaInputData->stIcfInputData.stBlobData[0].pData);
        (VOID)Ba_HalFree((VOID *)pstBaInputData);
    }

    BA_LOGI("DeInit Input Data Prm end! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInitInputDataPrm
 * @brief:      初始化输入数据参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInitInputDataPrm(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_INPUT_PRM_S *pstBaInputPrm = NULL;
    BA_INPUT_DATA_S *pstBaInputData = NULL;
    ICF_INPUT_DATA *pstIcfInputData = NULL;
    SBAE_INPUT_INFO_T *pstSbaInputData = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstBaInputPrm = &pstBa_dev->stBaInputPrm;
    pstBaInputPrm->uiCnt = BA_INPUT_DATA_BUF_NUM;

    for (i = 0; i < pstBaInputPrm->uiCnt; i++)
    {
        if (NULL != pstBaInputPrm->pstBaInputData[i])
        {
            BA_LOGW("Ba Input Data is initialized, continue! i %d, chan %d \n", i, chan);
            continue;
        }

        s32Ret = Ba_HalMalloc((VOID **)&pstBaInputPrm->pstBaInputData[i], sizeof(BA_INPUT_DATA_S));
        BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalMalloc failed!");

        pstBaInputData = pstBaInputPrm->pstBaInputData[i];

        pstIcfInputData = &pstBaInputData->stIcfInputData;

        pstIcfInputData->nBlobNum = 1;
        pstIcfInputData->stBlobData[0].nShape[0] = BA_CHN_WIDTH;
        pstIcfInputData->stBlobData[0].nShape[1] = BA_CHN_HEIGHT;
        pstIcfInputData->stBlobData[0].eBlobFormat = ICF_INPUT_FORMAT_SENSOR;
        pstIcfInputData->stBlobData[0].nFrameNum = 0;
        pstIcfInputData->stBlobData[0].nTimeStamp = 0;
        pstIcfInputData->stBlobData[0].nSpace = ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE;

        s32Ret = Ba_HalMalloc((VOID **)&pstIcfInputData->stBlobData[0].pData, sizeof(SBAE_INPUT_INFO_T));
        BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalMalloc failed!");

        pstSbaInputData = (SBAE_INPUT_INFO_T *)pstIcfInputData->stBlobData[0].pData;

        pstSbaInputData->nSpeed = 40;           /* 给定车速,只有给了速度才会报行为分析报警 */
        pstSbaInputData->pData = NULL;          /* 数据初始化为NULL */
        pstBaInputData->uiChn = chan;           /* chan初始值 */
        pstBaInputData->uiInputId = i;
        pstBaInputData->uiUseFlag = SAL_FALSE;  /* 使用状态初始化 */
    }

    BA_LOGI("Init Input Data Prm end! chan %d \n", chan);
    return SAL_SOK;

err:
    (VOID)Ba_DrvDeInitInputDataPrm(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvDeInitQue
 * @brief:      去初始化队列参数（通用）
 * @param[in]:  BA_QUEUE_PRM *pstQuePrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvDeInitQue(BA_QUEUE_PRM *pstQuePrm)
{
    INT32 s32Ret = SAL_SOK;

    BA_DRV_CHECK_PTR(pstQuePrm, err, "pstQuePrm == null!");

    if (NULL != pstQuePrm->pstEmptQue)
    {
        s32Ret = DSA_QueDelete(pstQuePrm->pstEmptQue);
        if (SAL_SOK != s32Ret)
        {
            BA_LOGE("error !!!\n");
        }
    }

    (VOID)Ba_HalFree(pstQuePrm->pstEmptQue);

    if (NULL != pstQuePrm->pstFullQue)
    {
        s32Ret = DSA_QueDelete(pstQuePrm->pstFullQue);
        if (SAL_SOK != s32Ret)
        {
            BA_LOGE("error !!!\n");
        }
    }

    (VOID)Ba_HalFree(pstQuePrm->pstFullQue);

    BA_LOGI("deinit Que end! \n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInitQue
 * @brief:      初始化队列参数（通用）
 * @param[in]:  BA_QUEUE_PRM *pstQuePrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInitQue(BA_QUEUE_PRM *pstQuePrm)
{
    BA_DRV_CHECK_PTR(pstQuePrm, err, "pstQuePrm == null!");

    if (pstQuePrm->enType >= MAX_QUEUE_TYPE
        || 0 == pstQuePrm->uiBufCnt
        || 0 == pstQuePrm->uiBufSize)
    {
        BA_LOGE("Invalid input args: type %d, buf_cnt %d, buf_size %d \n",
                pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);
        return SAL_FAIL;
    }

    if (SAL_SOK != Ba_HalMalloc((VOID **)&pstQuePrm->pstEmptQue, sizeof(DSA_QueHndl)))
    {
        BA_LOGE("malloc failed !!!\n");
        goto err;
    }

    if (SAL_SOK != DSA_QueCreate(pstQuePrm->pstEmptQue, pstQuePrm->uiBufCnt))
    {
        BA_LOGE("error !!!\n");
        goto err;
    }

    if (DOUBLE_QUEUE_TYPE == pstQuePrm->enType)
    {
        if (SAL_SOK != Ba_HalMalloc((VOID **)&pstQuePrm->pstFullQue, sizeof(DSA_QueHndl)))
        {
            BA_LOGE("malloc failed !!!\n");
            goto err;
        }

        if (SAL_SOK != DSA_QueCreate(pstQuePrm->pstFullQue, pstQuePrm->uiBufCnt))
        {
            BA_LOGE("error !!!\n");
            goto err;
        }
    }

    BA_LOGI("init Que end! type %d, cnt %d, size %d \n", pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);
    return SAL_SOK;

err:
    (VOID)Ba_DrvDeInitQue(pstQuePrm);
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvDeInitYuvQue
 * @brief:      去初始化YUV队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvDeInitYuvQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_YUV_QUE_PRM *pstYuvQueInfo = NULL;
    BA_QUEUE_PRM *pstYuvQuePrm = NULL;
    BA_FRAME_INFO_ST *pstYuvFrame = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstYuvQueInfo = &pstBa_dev->stYuvQueInfo;

    if (NULL == pstYuvQueInfo->pstYuvQuePrm)
    {
        BA_LOGI("Yuv Que has been deinitialized! return success! chan %d \n", chan);
        return SAL_SOK;
    }

    pstYuvQuePrm = pstYuvQueInfo->pstYuvQuePrm;

    for (i = 0; i < pstYuvQuePrm->uiBufCnt; i++)
    {
        pstYuvFrame = pstYuvQueInfo->pstYuvFrame[i];
        if (NULL == pstYuvFrame)
        {
            BA_LOGI("Yuv Frame has been deinitialized! continue. i %d \n", i);
            continue;
        }

        s32Ret = disp_hal_putFrameMem(&pstYuvFrame->stFrame);
        if (s32Ret != SAL_SOK)
        {
            BA_LOGE("disp_hal_putFrameMem error !!!\n");
        }

        (VOID)Ba_HalFree(pstYuvQueInfo->pstYuvFrame[i]);
    }

    s32Ret = Ba_DrvDeInitQue(pstYuvQuePrm);
    if (SAL_SOK != s32Ret)
    {
        BA_LOGE("Init Que Failed! \n");
    }

    (VOID)Ba_HalFree(pstYuvQueInfo->pstYuvQuePrm);

    Ba_HalPrint((CHAR *)__func__);

    BA_LOGI("deinit Que Yuv Frm End! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInitYuvQue
 * @brief:      初始化YUV队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInitYuvQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_YUV_QUE_PRM *pstYuvQueInfo = NULL;
    BA_QUEUE_PRM *pstYuvQuePrm = NULL;
    BA_FRAME_INFO_ST *pstYuvFrame = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstYuvQueInfo = &pstBa_dev->stYuvQueInfo;

    s32Ret = Ba_HalMalloc((VOID **)&pstYuvQueInfo->pstYuvQuePrm, sizeof(BA_QUEUE_PRM));
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalMalloc failed!");

    Ba_HalPrint("szl_dbg: malloc mem for yuv que prm end!");

    pstYuvQuePrm = pstYuvQueInfo->pstYuvQuePrm;

    pstYuvQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstYuvQuePrm->uiBufCnt = BA_MAX_YUV_BUF_NUM;
    pstYuvQuePrm->uiBufSize = sizeof(BA_FRAME_INFO_ST);

    s32Ret = Ba_DrvInitQue(pstYuvQuePrm);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvInitQue failed!");

    pstEmptQue = pstYuvQuePrm->pstEmptQue;
    BA_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < pstYuvQuePrm->uiBufCnt; i++)
    {
        s32Ret = Ba_HalMalloc((VOID **)&pstYuvQueInfo->pstYuvFrame[i], sizeof(BA_FRAME_INFO_ST));
        BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalMalloc failed!");

        pstYuvFrame = pstYuvQueInfo->pstYuvFrame[i];
        BA_DRV_CHECK_PTR(pstYuvFrame, err, "pstYuvFrame == null!");

        s32Ret = disp_hal_getFrameMem(BA_CHN_WIDTH, BA_CHN_HEIGHT, &pstYuvFrame->stFrame);
        BA_DRV_CHECK_RET(s32Ret, err, "disp_hal_getFrameMem failed!");

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);
        BA_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    BA_LOGI("Init Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
            chan, pstYuvQuePrm->enType, pstYuvQuePrm->uiBufCnt, pstYuvQuePrm->uiBufSize);
    return SAL_SOK;

err:
    (VOID)Ba_DrvDeInitYuvQue(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvDeInitCfgQue
 * @brief:      去初始化CFG队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvDeInitCfgQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_CFG_QUE_PRM *pstCfgQueInfo = NULL;
    BA_QUEUE_PRM *pstCfgQuePrm = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstCfgQueInfo = &pstBa_dev->stCfgQueInfo;

    if (NULL == pstCfgQueInfo->pstCfgQuePrm)
    {
        BA_LOGI("Yuv Que has been deinitialized! return success! chan %d \n", chan);
        return SAL_SOK;
    }

    pstCfgQuePrm = pstCfgQueInfo->pstCfgQuePrm;

    for (i = 0; i < pstCfgQuePrm->uiBufCnt; i++)
    {
        if (NULL == pstCfgQueInfo->pstIn[i])
        {
            BA_LOGI("Yuv Frame has been deinitialized! continue. i %d \n", i);
            continue;
        }

        (VOID)Ba_HalFree(pstCfgQueInfo->pstIn[i]);
    }

    s32Ret = Ba_DrvDeInitQue(pstCfgQuePrm);
    if (SAL_SOK != s32Ret)
    {
        BA_LOGE("Init Que Failed! \n");
    }

    (VOID)Ba_HalFree(pstCfgQueInfo->pstCfgQuePrm);

    Ba_HalPrint((CHAR *)__func__);

    BA_LOGI("deinit Que Yuv Frm End! chan %d \n", chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInitCfgQue
 * @brief:      初始化CFG队列
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInitCfgQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_CFG_QUE_PRM *pstCfgQueInfo = NULL;
    BA_QUEUE_PRM *pstCfgQuePrm = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstCfgQueInfo = &pstBa_dev->stCfgQueInfo;

    s32Ret = Ba_HalMalloc((VOID **)&pstCfgQueInfo->pstCfgQuePrm, sizeof(BA_QUEUE_PRM));
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalMalloc failed!");

    Ba_HalPrint("szl_dbg: malloc mem for yuv que prm end!");

    pstCfgQuePrm = pstCfgQueInfo->pstCfgQuePrm;

    pstCfgQuePrm->enType = DOUBLE_QUEUE_TYPE;
    pstCfgQuePrm->uiBufCnt = BA_MAX_PRM_BUF_NUM;
    pstCfgQuePrm->uiBufSize = sizeof(BA_PROCESS_IN);

    s32Ret = Ba_DrvInitQue(pstCfgQuePrm);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvInitQue failed!");

    /* Default Param */
    pstBa_dev->stIn.uiCapt = SAL_TRUE;
    pstBa_dev->stIn.uiSensity = BA_DEFAULT_SENSITY;
    pstBa_dev->stIn.stAbnorBehavCntPrm.uiUpFlag = SAL_TRUE;
    pstBa_dev->stIn.stAbnorBehavCntPrm.uiAbnorBehavCnt = BA_DEFAULT_ALARM_CNT;
    pstBa_dev->stIn.rect.x = BA_INPUT_X;
    pstBa_dev->stIn.rect.y = BA_INPUT_Y;
    pstBa_dev->stIn.rect.width = BA_INPUT_WIDTH;
    pstBa_dev->stIn.rect.height = BA_INPUT_HEIGHT;

    pstEmptQue = pstCfgQuePrm->pstEmptQue;
    BA_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null");

    for (i = 0; i < pstCfgQuePrm->uiBufCnt; i++)
    {
        s32Ret = Ba_HalMalloc((VOID **)&pstCfgQueInfo->pstIn[i], sizeof(BA_PROCESS_IN));
        BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalMalloc failed!");

        sal_memcpy_s(pstCfgQueInfo->pstIn[i], sizeof(BA_PROCESS_IN), &pstBa_dev->stIn, sizeof(BA_PROCESS_IN));

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstCfgQueInfo->pstIn[i], SAL_TIMEOUT_NONE);
        BA_DRV_CHECK_RET(s32Ret, err, "DSA_QuePut failed!");
    }

    BA_LOGI("Init Cfg Que End! chan %d, type %d, cnt %d, size %d \n",
            chan, pstCfgQuePrm->enType, pstCfgQuePrm->uiBufCnt, pstCfgQuePrm->uiBufSize);
    return SAL_SOK;

err:
    (VOID)Ba_DrvDeInitCfgQue(chan);
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInitDefaultPrm
 * @brief:      初始化通道默认参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInitDefaultPrm(UINT32 chan)
{
    BA_DEV_PRM *pstBa_dev = NULL;
    BA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstBa_dev->uiChnStatus = SAL_FALSE;

    if (SAL_SOK != Ba_DrvInitYuvQue(chan))
    {
        BA_LOGE("Init Yuv Queue Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != Ba_DrvInitCfgQue(chan))
    {
        BA_LOGE("Init Cfg Queue Failed! \n");
        return SAL_FAIL;
    }

    pstPrmFullQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstFullQue;
    pstPrmEmptQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstEmptQue;

    BA_DRV_CHECK_PTR(pstPrmFullQue, err, "pstPrmFullQue == null!");
    BA_DRV_CHECK_PTR(pstPrmEmptQue, err, "pstPrmEmptQue == null!");

    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    Ba_HalPrint((CHAR *)__func__);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInitGlobalVar
 * @brief:      通道全局变量初始化
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvInitGlobalVar(UINT32 chan)
{
    BA_DEV_PRM *pstBa_dev = NULL;
    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    pstSysFrmInfo = &pstBa_dev->stDecFrameInfo;
    if (0x00 == pstSysFrmInfo->uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(pstSysFrmInfo))
        {
            BA_LOGE("malloc 1 failed!");
            goto err;
        }
    }

    pstSysFrmInfo = &pstBa_dev->stRltFrameInfo;
    if (0x00 == pstSysFrmInfo->uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(pstSysFrmInfo))
        {
            BA_LOGE("malloc 2 failed!");
            goto err;
        }
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetAlertSts
 * @brief:      获取告警状态
 * @param[in]:  UINT32 uiBitFlag
 * @param[in]:  UINT32 *puiAltSts
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvGetAlertSts(UINT32 uiBitFlag, UINT32 *puiAltSts)
{
    UINT32 offset = 0;

    BA_DRV_CHECK_PTR(uiBitFlag, err, "Ba_DrvGetAlertSts failed!");
    BA_DRV_CHECK_PTR(puiAltSts, err, "puiAltSts == null!");

    while (offset < ABNOR_BEHAV_TYPE_NUM && !(uiBitFlag >> offset & 0x1))
    {
        offset++;
    }

    if (ABNOR_BEHAV_TYPE_NUM == offset)
    {
        BA_LOGE("no abnormal behavior! \n");
        return SAL_FAIL;
    }

    *puiAltSts = 0 != (uiBitFlag >> offset & 0x1) ? PIC_SMD_LEAVE_DETECT + offset : -1;

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetBehavLastTime
 * @brief:      获取当前对于异常行为的配置(持续时间)
 * @param[in]:  IN ABNOR_BEHAV_NUM_E enBehavType
 * @param[in]:  IN SBAE_SENSCTRL_PARAMS_T *pstSensityPrm
 * @param[in]:  OUT float *pfTime
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvGetBehavLastTime(IN ABNOR_BEHAV_NUM_E enBehavType,
                                    IN SBAE_DBA_PROC_PARAMS_T *pstSensityPrm,
                                    OUT float *pfTime)
{
    float fLastTime = 0;

    BA_DRV_CHECK_PTR(pstSensityPrm, err, "pstSensityPrm == null!");
    BA_DRV_CHECK_PTR(pfTime, err, "puiTime == null!");

    switch (enBehavType)
    {
        case ABNOR_BEHAV_LEAVE_TYPE:
            fLastTime = 0.0;
            break;
        case ABNOR_BEHAV_EXIST_TYPE:
            fLastTime = 0.0;
            break;
        case ABNOR_BEHAV_COVER_TYPE:
            fLastTime = 5.0;
            break;
        case ABNOR_BEHAV_DHQ_TYPE:
            fLastTime = 2.0;
            break;
        case ABNOR_BEHAV_NLF_TYPE:
            fLastTime = 3.0;
            break;
        case ABNOR_BEHAV_CHAT_TYPE:
            fLastTime = 3.0;
            break;
        case ABNOR_BEHAV_TIRED_TYPE:
            fLastTime = 2.0;
            break;
        case ABNOR_BEHAV_SMOKE_TYPE:
            fLastTime = 2.0;
            break;
        case ABNOR_BEHAV_PHONE_TYPE:
            fLastTime = 2.0;
            break;

        default:
            fLastTime = 5.0;
            BA_LOGW("enter default loop! use default cfg 5s \n");
            break;
    }

    *pfTime = fLastTime;

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvGetFilterFlag
 * @brief:      获取过滤重复结果的状态
 * @param[in]:  UINT32 chan
 * @param[in]:  ABNOR_BEHAV_NUM_E enBehavType
 * @param[in]:  UINT32 uiCurFrmNum
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvGetFilterFlag(UINT32 chan, ABNOR_BEHAV_NUM_E enBehavType, UINT32 uiCurFrmNum)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiFilterFlag = 0;
    UINT32 uiLastFrmNum = 0;
    float fBehavLastTime = 0;
    float fFrmGap = 0;

    SBAE_DBA_PROC_PARAMS_T stSensityPrm = {0};
    SBAE_DBA_PROC_PARAMS_T stAppAlertPrm = {0};

    BA_DEV_PRM *pstBa_dev = NULL;

    if (enBehavType > ABNOR_BEHAV_TYPE_NUM - 1)
    {
        BA_LOGE("Invalid Behavior type %d, Max %d \n", enBehavType, ABNOR_BEHAV_TYPE_NUM - 1);
        return BA_FILTER_FLAG_FALSE;
    }

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    s32Ret = Ba_HalGetIcfCfg(chan, &stSensityPrm, &stAppAlertPrm);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalGetIcfCfg failed!");

    s32Ret = Ba_DrvGetBehavLastTime(enBehavType, &stSensityPrm, &fBehavLastTime);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvGetBehavLastTime failed!");

    uiLastFrmNum = pstBa_dev->stLastStsInfo.uiLastFrmNum[enBehavType];

    fFrmGap = (float)BA_INPUT_FRAME_RATE * fBehavLastTime;
    uiFilterFlag = FILTER_REPEAT_REAULT(uiLastFrmNum, uiCurFrmNum, fFrmGap);

    if (uiFilterFlag)
    {
        pstBa_dev->stLastStsInfo.uiLastFrmNum[enBehavType] = uiCurFrmNum;
    }

    return uiFilterFlag;

err:
    return BA_FILTER_FLAG_FALSE;
}

/**
 * @function:   Ba_DrvJudgeHeadValid
 * @brief:      判断头肩是否有效(检测区域内为有效)
 * @param[in]:  BA_OUTPUT_RESULT_S *pstBaOutRslt
 * @param[in]:  BA_PROCESS_IN *pstIn
 * @param[in]:  BA_OBJ_DET_RESULT *pstValidObj
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvJudgeHeadValid(BA_OUTPUT_RESULT_S *pstBaOutRslt, BA_PROCESS_IN *pstIn, BA_OBJ_DET_RESULT *pstValidObj)
{
    UINT32 uiRgnX = 0;
    UINT32 uiRgnY = 0;
    UINT32 uiRgnW = 0;
    UINT32 uiRgnH = 0;
    UINT32 uiHeadMidX = 0;
    UINT32 uiHeadMidY = 0;

    BA_DRV_CHECK_PTR(pstBaOutRslt, err, "pstFrameInfo == null!");
    BA_DRV_CHECK_PTR(pstIn, err, "pstIn == null!");
    BA_DRV_CHECK_PTR(pstValidObj, err, "pstValidObj == null!");

    uiRgnX = (UINT32)(pstIn->rect.x * BA_CHN_WIDTH);
    uiRgnY = (UINT32)(pstIn->rect.y * BA_CHN_HEIGHT);
    uiRgnW = (UINT32)(pstIn->rect.width * BA_CHN_WIDTH);
    uiRgnH = (UINT32)(pstIn->rect.height * BA_CHN_HEIGHT);

    uiHeadMidX = pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.x \
                  + pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.width / 2;

    uiHeadMidY = pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.y \
                  + pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.height / 2;

    if (uiHeadMidX <= uiRgnX || uiHeadMidX >= uiRgnX + uiRgnW || uiHeadMidY <= uiRgnY || uiHeadMidY >= uiRgnY + uiRgnH)
    {
#if 0
        BA_LOGW("zk:log:x_%d y_%d wid_%d hei_%d uiHeadMidX_%d uiHeadMidY_%d\n", pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.x, \
                pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.y, \
                pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.width, \
                pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.height, uiHeadMidX, uiHeadMidY);
#endif
        pstValidObj->uiObjectNum = 0;
        return SAL_SOK;
    }

    pstValidObj->Rect[0].x = (int)pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.x;
    pstValidObj->Rect[0].y = (int)pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.y;
    pstValidObj->Rect[0].width = (int)pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.width;
    pstValidObj->Rect[0].height = (int)pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.height;

    pstValidObj->uiObjectNum = 1;

    return SAL_SOK;

err:
    BA_LOGW("Ba Face Not in Region!\n");
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvPreProcRslt
 * @brief:      算法结果处理
 * @param[in]:  BA_OUTPUT_RESULT_S *pstBaOutRslt
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvPreProcRslt(UINT32 chan, BA_OUTPUT_RESULT_S *pstBaOutRslt, UINT32 *puiBitFlag)
{
    UINT32 s32Ret = SAL_FALSE;
    UINT32 uiBitMask = 0;
    UINT32 uiCurrSts = 0;
    UINT32 uiFaceValid = 0;
    UINT32 uiFilterFlag = 0;
    static UINT32 uiArriveCount = 0;
    static UINT32 uiLeaveCount = 0;
    static BA_DUTY_TYPE_E eCurrBaState = BA_DUTY_LEAVE_STATE;
    static BA_DUTY_TYPE_E eLastBaState = BA_DUTY_LEAVE_STATE;
    ABNOR_BEHAV_NUM_E enBehavType = 0;
    BA_DEV_PRM *pstBa_dev = NULL;
    SBAE_OUTPUT_INFO_T *pstSbaOutInfo = NULL;
    SBAE_DBA_POS_T *pstSbaPosInfo = NULL;
    BA_OBJ_DET_RESULT stObjDetInfo = {0};
    BA_PROCESS_IN stIn = {0};
    
    BA_DRV_CHECK_PTR(puiBitFlag, err, "puiBitFlag == null!");

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    s32Ret = Ba_DrvGetCfgParam(chan, &stIn);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvGetCfgParam Failed!");

    BA_DRV_CHECK_PTR(pstBaOutRslt, err, "pstBaOutRslt == null!");

    pstSbaOutInfo = &pstBaOutRslt->stSbaRsltInfo.stOutInfo;
    pstSbaPosInfo = &pstBaOutRslt->stSbaRsltInfo.stPosInfo;

    /* 检测结果有效性过滤，当前根据是否检测到头肩数据进行判断该数据是否有效 */
    sal_memset_s(&stObjDetInfo, sizeof(BA_OBJ_DET_RESULT), 0x00, sizeof(BA_OBJ_DET_RESULT));
    s32Ret = Ba_DrvJudgeHeadValid(pstBaOutRslt, &stIn, &stObjDetInfo);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvJudgeHeadValid failed!");
    if(stObjDetInfo.uiObjectNum == 0)
    {
        uiFaceValid = 0;                                                    //头肩在检测区域范围外
    }
    else
    {
        uiFaceValid = 1;                                                    //头肩在检测区域范围内
    }
	//BA_LOGW("+++++++++++++++++++++++++++++++++FaceValid %d! \n", uiFaceValid);

    /*在岗*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_ABNORMAL].eType;
    enBehavType = ABNOR_BEHAV_EXIST_TYPE;
    uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
    if ((SBAE_BEH_ABNORMAL != uiCurrSts) && (uiFaceValid == 1))
    {
        uiArriveCount++;
		//BA_LOGW("+++++++++++++++++++++++++++++++++uiArriveCount %d! \n", uiArriveCount);
        if (uiArriveCount >= BA_FILTER_TIMES)
        {
            eCurrBaState = BA_DUTY_ARRIVE_STATE;
            uiArriveCount = 0;
			uiLeaveCount  = 0;
            if ((BA_DUTY_ARRIVE_STATE == eCurrBaState) && (BA_DUTY_LEAVE_STATE == eLastBaState))
            {
                uiBitMask |= (uiFilterFlag << enBehavType);
                SAL_INFO("zk_log:uiCurrSts Arrive!!!\n");
				//BA_LOGE("++++++++++++++++++++++++++++zk_log:uiCurrSts Arrive!!!the uiFilterFlag is %d\n",uiFilterFlag);
            }
        }
    }

    pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;

    /*离岗*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_ABNORMAL].eType;
    enBehavType = ABNOR_BEHAV_LEAVE_TYPE;
    uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
    if (SBAE_BEH_ABNORMAL == uiCurrSts ||                                                         //目标不在图像内
        (uiFaceValid == 0 &&
         (0 != pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].nObjNum &&
          0 != pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.width &&
          0 != pstBaOutRslt->stSbaRsltInfo.stPosInfo.stPosMod[0].stObjArray.stObjPos.height)))    //判断目标在检测范围外且在图像内为离岗状态；（判断图像内有目标，避免遮挡也误报为离岗）
    {
        uiLeaveCount++;
		//BA_LOGW("+++++++++++++++++++++++++++++++++uiLeaveCount %d! \n", uiLeaveCount);
        if (uiLeaveCount >= BA_FILTER_TIMES)
        {
            eCurrBaState = BA_DUTY_LEAVE_STATE;
            uiLeaveCount = 0;
			uiArriveCount = 0;
            if ((BA_DUTY_LEAVE_STATE == eCurrBaState) && (BA_DUTY_ARRIVE_STATE == eLastBaState))
            {
                uiBitMask |= (uiFilterFlag << enBehavType);
                SAL_INFO("zk_log:uiCerrSts Leave!!!\n");
				//BA_LOGE("++++++++++++++++++++++++++++zk_log:uiCerrSts Leave!!!,the uiFilterFlag is %d\n",uiFilterFlag);
            }
        }
    }

    eLastBaState = eCurrBaState; /*更新在离岗状态*/
    pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;


    /*遮挡*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_HCD].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_COVER_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if (SBAE_BEH_ABNORMAL != uiCurrSts)
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    /*不目视前方--总共包括上\下\左\右\左下\右下六种异常不目视前方报警*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_NLF].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_NLF_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if ((SBAE_BEH_NLF_L == uiCurrSts || SBAE_BEH_NLF_R == uiCurrSts || SBAE_BEH_NLF_U == uiCurrSts || SBAE_BEH_NLF_D == uiCurrSts \
             || SBAE_BEH_NLF_LD == uiCurrSts || SBAE_BEH_NLF_RD == uiCurrSts) && (BA_DUTY_ARRIVE_STATE == eCurrBaState) && (1 == uiFaceValid))
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    /*打哈欠*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_FDD].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_DHQ_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if ((SBAE_BEH_FDW_YAWN == uiCurrSts) && (BA_DUTY_ARRIVE_STATE == eCurrBaState) && (1 == uiFaceValid))
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    /*眯眼报警*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_FDD].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_TIRED_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if ((SBAE_BEH_FDW_EYE == uiCurrSts) && (BA_DUTY_ARRIVE_STATE == eCurrBaState) && (1 == uiFaceValid))
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    /*聊天报警*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_TALK].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_CHAT_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if ((SBAE_BEH_TALK == uiCurrSts) && (BA_DUTY_ARRIVE_STATE == eCurrBaState) && (1 == uiFaceValid))
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    /*打电话报警*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_UPD].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_PHONE_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if ((SBAE_BEH_UPW == uiCurrSts) && (BA_DUTY_ARRIVE_STATE == eCurrBaState) && (1 == uiFaceValid))
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    /*抽烟报警*/
    uiCurrSts = pstSbaOutInfo->stResInfo[SBAE_DBA_FUNC_SMD].eType;
    if (SBAE_BEH_INVALID != uiCurrSts)
    {
        enBehavType = ABNOR_BEHAV_SMOKE_TYPE;
        uiFilterFlag = Ba_DrvGetFilterFlag(chan, enBehavType, pstSbaPosInfo->unFrameIdx);
        if ((SBAE_BEH_SMW == uiCurrSts) && (BA_DUTY_ARRIVE_STATE == eCurrBaState) && (1 == uiFaceValid))
        {
            uiBitMask |= (uiFilterFlag << enBehavType);
        }
        pstBa_dev->stLastStsInfo.auiLastAltsts[enBehavType] = uiCurrSts;
    }

    *puiBitFlag = uiBitMask;
    return SAL_SOK;
err:
    return SAL_FAIL;
}


/**
 * @function:   Ba_DrvEncJpeg
 * @brief:      编图
 * @param[in]:  UINT32 chan
 * @param[in]:  VIDEO_FRAME_INFO_S *pstFrameInfo
 * @param[out]: None
 * @return:     static INT32
 */
 
static INT32 Ba_DrvEncJpeg(UINT32 chan, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;

    CROP_S stCropInfo = {0};
    BA_DEV_PRM *pstBa_dev = NULL;
    JPEG_COMMON_ENC_PRM_S jpegEncPrm = {0};

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(pstFrame, err, "pstFrameInfo == null!");

    pstBa_dev = Ba_DrvGetDev(chan);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    stCropInfo.u32CropEnable = SAL_FALSE;
    stCropInfo.u32X = 0;
    stCropInfo.u32Y = 0;
    stCropInfo.u32H = BA_CHN_HEIGHT;
    stCropInfo.u32W = BA_CHN_WIDTH;

    
	memcpy(&jpegEncPrm.stSysFrame, pstFrame, sizeof(SYSTEM_FRAME_INFO));
    jpegEncPrm.pOutJpeg = pstBa_dev->stJpegVenc.pPicJpeg;

    s32Ret = jpeg_drv_cropEnc(pstBa_dev->stJpegVenc.pstJpegChnInfo,
                                &jpegEncPrm,
                                &stCropInfo);
    BA_DRV_CHECK_RET(s32Ret, err, "JpegHal_drvEncJpeg failed!");
    pstBa_dev->stJpegVenc.uiPicJpegSize= jpegEncPrm.outSize;

    SAL_LOGW("%s|%d--uiDecChn %d Enc Jpeg %p End! Jpeg Size %d\n", __func__, __LINE__,
                 pstBa_dev->stJpegVenc.uiJpegEncChn, pstBa_dev->stJpegVenc.pPicJpeg, pstBa_dev->stJpegVenc.uiPicJpegSize);

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvDrawRect
 * @brief:      画框
 * @param[in]:  BA_OUTPUT_RESULT_S *pstBaOutRslt
 * @param[in]:  VIDEO_FRAME_INFO_S *pstFrameInfo
 * @param[in]:  BA_PROCESS_IN *pstIn
 * @param[in]:  BA_OBJ_DET_RESULT *pstObjDetInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvDrawRect(BA_OUTPUT_RESULT_S *pstBaOutRslt,
                            SAL_VideoFrameBuf *pstSALVideoFrameBuf,
                            BA_PROCESS_IN *pstIn,
                            BA_OBJ_DET_RESULT *pstObjDetInfo)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 uiX = 0;
    UINT32 uiY = 0;
    UINT32 uiW = 0;
    UINT32 uiH = 0;

    VGS_RECT_ARRAY_S *pstVgsDrawRect = NULL;
    VGS_DRAW_RECT_S *pstRect = NULL;
    UINT32 u32Thick = capb_get_line()->u32LineWidth;
    VGS_ARTICLE_LINE_TYPE ailinetype;

    sal_memset_s(&ailinetype, sizeof(VGS_ARTICLE_LINE_TYPE), 0x00, sizeof(VGS_ARTICLE_LINE_TYPE));

    pstVgsDrawRect = &stBaVgsDrawRect;
    BA_DRV_CHECK_PTR(pstVgsDrawRect, err, "pstVgsDrawRect == null!");

    sal_memset_s(pstVgsDrawRect, sizeof(VGS_RECT_ARRAY_S), 0x00, sizeof(VGS_RECT_ARRAY_S));

    /* 结合检测区域进行头肩有效性框选 */
    s32Ret = Ba_DrvJudgeHeadValid(pstBaOutRslt, pstIn, pstObjDetInfo);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvJudgeHeadValid failed!");

    BA_LOGD("uiObjectNum %d ValidObjNum %d \n", pstObjDetInfo->uiObjectNum, pstObjDetInfo->uiObjectNum);
    
    for (i = 0; i < pstObjDetInfo->uiObjectNum; i++)
    {
        uiX = (UINT32)(pstObjDetInfo->Rect[i].x);
        uiY = (UINT32)(pstObjDetInfo->Rect[i].y);
        uiW = (UINT32)(pstObjDetInfo->Rect[i].width);
        uiH = (UINT32)(pstObjDetInfo->Rect[i].height);
#if 0
        BA_LOGW("i %d [x,y] [%d %d] [w,h] [%d %d] [uiX,uiY] [%d %d] [uiW,uiH] [%d %d]!\n",
                 i, pstObjDetInfo->Rect[i].x, pstObjDetInfo->Rect[i].y,
                 pstObjDetInfo->Rect[i].width, pstObjDetInfo->Rect[i].height,
                 uiX, uiY, uiW, uiH);
#endif
        if ((uiW > 0) && (uiH > 0))
        {
            BA_LOGD("i %d Jpeg entering!\n", i);

            uiX = (uiX + 1) / 2 * 2;
            uiY = (uiY + 1) / 2 * 2;
            uiW = (uiW + 1) / 2 * 2;
            uiH = (uiH + 1) / 2 * 2;

            pstRect = pstVgsDrawRect->astRect + pstVgsDrawRect->u32RectNum++;
            pstRect->s32X           = uiX;
            pstRect->s32Y           = uiY;
            pstRect->u32Width       = uiW;
            pstRect->u32Height      = uiH;
            pstRect->f32StartXFract = -1;
            pstRect->f32EndXFract   = -1;
            pstRect->u32Color       = 0xFF4500;
            pstRect->u32Thick       = u32Thick;
            memcpy(&pstRect->stLinePrm, &ailinetype.ailine, sizeof(ailinetype.ailine));
        }
    }

    (VOID)vgs_func_drawRectArraySoft(pstSALVideoFrameBuf, pstVgsDrawRect, SAL_FALSE, SAL_FALSE);
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvProcThread
 * @brief:      行为分析结果处理线程
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
int cnt_debug = 0;
static void *Ba_DrvProcThread(void *prm)
{
        UINT32 s32Ret = SAL_FAIL;
    
        UINT32 uiBitFlag = 0;
        UINT32 input_uiCapt = 0;
        UINT32 uiAbnorBehavCnt = 0;
		UINT32 uiQueueCnt = 0;
        UINT32 enAlertStatus = PIC_SMD_LEAVE_DETECT;
        BA_DEV_PRM *pstBa_dev = NULL;
		BA_PROCESS_IN *pstIn = NULL;
		DSA_QueHndl *pstPrmEmptQue = NULL;
		DSA_QueHndl *pstPrmFullQue = NULL;
        DSA_QueHndl *pstYuvFullQue = NULL;
        DSA_QueHndl *pstYuvEmptQue = NULL;
        BA_FRAME_INFO_ST *pstYuvFrame = NULL;
        SAL_VideoFrameBuf salVideoFrame = {0};
        //JPEG_COMMON_ENC_PRM_S jpegEncPrm = {0};
    
        BA_PROCESS_IN stIn = {0};
    
        STREAM_ELEMENT stStreamEle = {0};
        BA_DSP_OUT stBaDspOut = {0};
        BA_OBJ_DET_RESULT stObjDetInfo = {0};
    
        BA_DRV_CHECK_PTR(prm, err, "prm == null!");
    
        pstBa_dev = (BA_DEV_PRM *)prm;
		
		/*更新算法参数的队列*/
		pstPrmEmptQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstEmptQue;
		pstPrmFullQue = pstBa_dev->stCfgQueInfo.pstCfgQuePrm->pstFullQue;

		BA_DRV_CHECK_PTR(pstPrmEmptQue, err, "pstPrmEmptQue == null!");
		BA_DRV_CHECK_PTR(pstPrmFullQue, err, "pstPrmFullQue == null!");

	    /*获取结果的队列*/
        pstYuvEmptQue = pstBa_dev->stYuvQueInfo.pstYuvQuePrm->pstEmptQue;
        pstYuvFullQue = pstBa_dev->stYuvQueInfo.pstYuvQuePrm->pstFullQue;
    
        BA_DRV_CHECK_PTR(pstYuvEmptQue, err, "pstYuvEmptQue == null!");
        BA_DRV_CHECK_PTR(pstYuvFullQue, err, "pstYuvFullQue == null!");
    
        pstBa_dev->stJpegVenc.pPicJpeg = (INT8 *)SAL_memMalloc(BA_JPEG_SIZE,"ba","ba_jpeg");
        if (NULL == pstBa_dev->stJpegVenc.pPicJpeg)
        {
            BA_LOGE("error !!!\n");
            return NULL;
        }
    
        SAL_SET_THR_NAME();
    
        SAL_SetThreadCoreBind(1);
    
        while (SAL_TRUE)
        {	
            /* 调试使用，后续需要修改成阻塞等待 */
            if (SAL_FALSE == pstBa_dev->uiChnStatus)
            {
                pstBa_dev->uiRlsFlag = 1;
                stIn.stAbnorBehavCntPrm.uiUpFlag = SAL_TRUE;   /* 行为分析重启后需要重新计数 */
                usleep(80 * 1000);
                continue;
            }
    
            pstBa_dev->uiRlsFlag = 0;
            /*更新参数输入*/
			uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
			if (uiQueueCnt)
			{
			    /* 获取 buffer */
			    DSA_QueGet(pstPrmFullQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
			    memcpy(&stIn, pstIn, sizeof(BA_PROCESS_IN));
			    DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE);
			}
    
            uiAbnorBehavCnt = uiAbnorBehavCnt >= stIn.stAbnorBehavCntPrm.uiAbnorBehavCnt ? 0 : uiAbnorBehavCnt;
    
            s32Ret = Ba_DrvGetQueBuf(pstYuvFullQue, (void **)&pstYuvFrame, SAL_TIMEOUT_NONE);
            if (SAL_SOK != s32Ret || NULL == pstYuvFrame)
            {
                usleep(50 * 1000);
                continue;
            }
    
            /* 算法结果后处理 */
            uiBitFlag = 0;
            s32Ret = Ba_DrvPreProcRslt(0, &pstYuvFrame->stBaRslt, &uiBitFlag);
            if (SAL_SOK != s32Ret || 0 == uiBitFlag)
            {
                goto reuse;
            }
            /* 获取报警状态 */
            enAlertStatus = -1;
            s32Ret = Ba_DrvGetAlertSts(uiBitFlag, &enAlertStatus);
            BA_DRV_CHECK_RET(s32Ret, reuse, "Ba_DrvGetAlertSts Failed!");
    
            BA_LOGW("alert sts 0x%x, frameNum %d, uiAbnorBehavCnt %d! \n",
                    enAlertStatus, pstYuvFrame->stBaRslt.stSbaRsltInfo.stPosInfo.unFrameIdx, uiAbnorBehavCnt);
    
    
            input_uiCapt = SAL_TRUE;                    /* 目前默认行为分析开启则打开抓拍 */
    
            if (SAL_TRUE == input_uiCapt)
            {

                memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
                s32Ret = sys_hal_getVideoFrameInfo(&pstYuvFrame->stFrame, &salVideoFrame);
                if(SAL_SOK != s32Ret)
                {
                    BA_LOGW("sys_hal_getVideoFrameInfo failed !\n");
                }
                sal_memset_s(&stObjDetInfo, sizeof(BA_OBJ_DET_RESULT), 0x00, sizeof(BA_OBJ_DET_RESULT));
    
                /* 识别出的目标进行画框+编图 */
                s32Ret = Ba_DrvDrawRect(&pstYuvFrame->stBaRslt,  &salVideoFrame, &stIn, &stObjDetInfo);
                BA_DRV_CHECK_RET(s32Ret, reuse, "Ba_DrvDrawRect failed!");

#if 0
                static CHAR path[64] = {0};

                SAL_clear(path);

                sprintf(path, "./ba_jpg/_input_%dx%d_%d.yuv", BA_CHN_WIDTH,BA_CHN_HEIGHT,cnt_debug);

                (VOID)Ba_HalDebugDumpData((CHAR *)(PhysAddr)salVideoFrame.virAddr[0],
                              (CHAR *)path, GET_YUV_SIZE(BA_CHN_WIDTH, BA_CHN_HEIGHT), 1);
                cnt_debug++;
                /* goto err;  / * debug * / */
#endif
                s32Ret = Ba_DrvEncJpeg(0, &pstYuvFrame->stFrame);
                BA_DRV_CHECK_RET(s32Ret, reuse, "Ba_DrvEncJpeg failed!");
            }
    
            sal_memset_s(&stStreamEle, sizeof(STREAM_ELEMENT), 0, sizeof(STREAM_ELEMENT));
            sal_memset_s(&stBaDspOut, sizeof(BA_DSP_OUT), 0, sizeof(BA_DSP_OUT));
    
            stStreamEle.type = STREAM_ELEMENT_SVA_BA;
            stStreamEle.chan = pstBa_dev->chan;
            SAL_getDateTime(&stStreamEle.absTime);
    
            stBaDspOut.ba_jpeg_result.cJpegAddr = (UINT8 *)pstBa_dev->stJpegVenc.pPicJpeg;
            stBaDspOut.ba_jpeg_result.uiJpegSize = pstBa_dev->stJpegVenc.uiPicJpegSize;
            stBaDspOut.ba_jpeg_result.ullTimePts = SAL_getCurMs();
            stBaDspOut.ba_alert_mode = enAlertStatus;
    
            uiAbnorBehavCnt++;

	        stBaDspOut.ba_target_num = uiAbnorBehavCnt;
		
            stBaDspOut.ba_alert = enAlertStatus >= PIC_SMD_LEAVE_DETECT && enAlertStatus <= PIC_SMD_COVER_DETECT
                                  ? SAL_FALSE : uiAbnorBehavCnt >= stIn.stAbnorBehavCntPrm.uiAbnorBehavCnt ? SAL_TRUE : SAL_FALSE;
            /*兼容老版本报警数据,为保证以前应用保存的异常行为可以被正常搜索到*/
            if (stBaDspOut.ba_alert_mode == PIC_SMD_LEAVE_DETECT)
            {
                stBaDspOut.ba_alert_mode = PIC_SMD_LEAVE_DETECT - 1;
            }
    
            BA_LOGW("app CB! chan %d alert_mode 0x%x, trgt_num %d  alert %d\n", stStreamEle.chan, stBaDspOut.ba_alert_mode, stBaDspOut.ba_target_num,stBaDspOut.ba_alert);
            SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stBaDspOut, sizeof(BA_DSP_OUT));
    
    reuse:
            /* 释放这一帧 */
            if (NULL != pstYuvFrame)
            {
                (VOID)Ba_DrvPutQueBuf(pstYuvEmptQue, pstYuvFrame, SAL_TIMEOUT_NONE);
                pstYuvFrame = NULL;
            }
        }
    
    err:
        return NULL;

}

/**
 * @function:   Ba_DrvGetOutResult
 * @brief:      获取行为分析检测结果
 * @param[in]:  SBAE_OUTPUT_INFO *pstOutRslt
 * @param[in]:  ICF_MEDIA_INFO *pstMediaInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvGetOutResult(SBAE_OUTPUT_T *pstOutRslt, ICF_MEDIA_INFO *pstMediaInfo)
{
    INT32 s32Ret = SAL_SOK;

    BA_DEV_PRM *pstBa_dev = NULL;
    DSA_QueHndl *pstYuvFullQue = NULL;
    DSA_QueHndl *pstYuvEmptQue = NULL;
    BA_FRAME_INFO_ST *pstBaFrameInfo = NULL;
    SBAE_OUTPUT_T *pstBaOutRslt = NULL;
    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;

    //VIDEO_FRAME_INFO_S *pstFrameInfo = NULL;
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SBAE_INPUT_INFO_T *pstSbaInputData = NULL;

    //SYS_VIRMEM_INFO_S stPhyMemInfo = {0};

    BA_DRV_CHECK_PTR(pstOutRslt, err, "pstOutRslt == null!");
    BA_DRV_CHECK_PTR(pstMediaInfo, err, "pstMediaInfo == null!");

    pstBa_dev = Ba_DrvGetDev(0);       /* BA_TODO: 当前行为分析只支持一个通道，默认0 */

    pstYuvFullQue = pstBa_dev->stYuvQueInfo.pstYuvQuePrm->pstFullQue;
    pstYuvEmptQue = pstBa_dev->stYuvQueInfo.pstYuvQuePrm->pstEmptQue;

    BA_DRV_CHECK_PTR(pstYuvFullQue, err, "pstYuvFullQue == null!");
    BA_DRV_CHECK_PTR(pstYuvEmptQue, err, "pstYuvEmptQue == null!");

    s32Ret = Ba_DrvGetQueBuf(pstYuvEmptQue, (VOID **)&pstBaFrameInfo, SAL_TIMEOUT_NONE);
    BA_DRV_CHECK_RET(s32Ret, reuse, "Ba_DrvGetQueBuf failed!");
    BA_DRV_CHECK_PTR(pstBaFrameInfo, reuse, "Ba_DrvGetQueBuf == null!");

    /* 获取行为分析检测结果 */
    pstBaOutRslt = &pstBaFrameInfo->stBaRslt.stSbaRsltInfo;

    sal_memcpy_s(pstBaOutRslt, sizeof(SBAE_OUTPUT_T), pstOutRslt, sizeof(SBAE_OUTPUT_T));

    /* 获取原始帧数据 */
    pstSbaInputData = (SBAE_INPUT_INFO_T *)pstMediaInfo->stInputInfo.stBlobData[0].pData;
    BA_DRV_CHECK_PTR(pstSbaInputData, reuse, "pstSbaInputData == null!");

    pstSysFrmInfo = &pstBa_dev->stRltFrameInfo;
    if (0x00 == pstSysFrmInfo->uiAppData)
    {
        BA_LOGE("Rslt Frame is not initialized! chan %d \n", 0);
        goto reuse;
    }

    stVideoFrmBuf.virAddr[0] = (PhysAddr)pstSbaInputData->pData;
    stVideoFrmBuf.virAddr[1] = (PhysAddr)pstSbaInputData->pData + BA_CHN_WIDTH * BA_CHN_HEIGHT;
    stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
    stVideoFrmBuf.frameParam.width = BA_CHN_WIDTH;
    stVideoFrmBuf.frameParam.height = BA_CHN_HEIGHT;
    stVideoFrmBuf.stride[0] = BA_CHN_WIDTH;
    stVideoFrmBuf.stride[1] = BA_CHN_WIDTH;
    stVideoFrmBuf.stride[2] = BA_CHN_WIDTH;
	stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSysFrmInfo);
    BA_DRV_CHECK_RET(s32Ret,reuse,"sys_hal_buildVideoFrame failed !");

    s32Ret = Ia_TdeQuickCopy(pstSysFrmInfo, &pstBaFrameInfo->stFrame, 0, 0, BA_CHN_WIDTH, BA_CHN_HEIGHT, SAL_FALSE);
    BA_DRV_CHECK_RET(s32Ret,reuse,"Ia_TdeQuickCopy failed !");

    s32Ret = Ba_DrvPutQueBuf(pstYuvFullQue, (VOID *)pstBaFrameInfo, SAL_TIMEOUT_NONE);
    BA_DRV_CHECK_RET(s32Ret, reuse, "Ba_DrvPutQueBuf failed!");

    pstBaFrameInfo = NULL;

    return SAL_SOK;

reuse:
    if (NULL != pstBaFrameInfo)
    {
        (VOID)Ba_DrvPutQueBuf(pstYuvEmptQue, pstBaFrameInfo, SAL_TIMEOUT_NONE);
        pstBaFrameInfo = NULL;
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvPrintOutputResult
 * @brief:      打印行为分析检测结果
 * @param[in]:  SBAE_OUTPUT_INFO *pstDbdRslt
 * @param[in]:  UINT32 uiFrmNum
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ba_DrvPrintOutputResult(SBAE_OUTPUT_T *rslt_dhd, ICF_MEDIA_INFO *pstMediaInfo, UINT32 uiFrmNum)
{
    UINT32 uiCurTime = 0;
    static UINT32 uiCnt = 0;
    static UINT32 uiTotalTime = 0;
    BA_INPUT_PRVT_DATA_S *pstBaPrvtInfo = NULL;

    pstBaPrvtInfo = (BA_INPUT_PRVT_DATA_S *)pstMediaInfo->stInputInfo.pUserPtr;

//#if (!defined DEBUG_LOG && !defined TRACE_LOG)
    //return SAL_SOK;
//#endif
#if 1
	return SAL_SOK;
#endif

    BA_LOGW("------------------------- Result ------------------------\n");
    BA_LOGW("FrameNum=%d,nlf=0x%4x,fdd=0x%4x,smd=0x%4x,upd=0x%4x,belt=0x%4x,abnormal=0x%4x,hcd=0x%4x,sunglass=0x%4x \n",
            uiFrmNum, rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_NLF].eType, rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_FDD].eType,
            rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_SMD].eType, rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_UPD].eType,
            rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_BELT].eType, rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_ABNORMAL].eType,
            rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_HCD].eType, rslt_dhd->stOutInfo.stResInfo[SBAE_DBA_FUNC_SUNGLASS].eType);


    BA_LOGW("------------------------- Time Cost ------------------------\n");
    uiCnt++;
    uiCurTime = SAL_getCurMs();
    uiTotalTime += uiCurTime - pstBaPrvtInfo->uiInputTime;
    BA_LOGW("Alg Proc Cost Time: %d, ave %d \n", uiCurTime - pstBaPrvtInfo->uiInputTime, uiTotalTime / uiCnt);
    BA_LOGW("\n");


    BA_LOGW("------------------------- Head Info ------------------------\n");
    BA_LOGW("--Head:Num:%d ------------------------\n", rslt_dhd->stPosInfo.stPosMod[0].nObjNum);
    BA_LOGW("Head Location:x_%d y_%d wid_%d hei_%d\n", rslt_dhd->stPosInfo.stPosMod[0].stObjArray.stObjPos.x,
            rslt_dhd->stPosInfo.stPosMod[0].stObjArray.stObjPos.y,
            rslt_dhd->stPosInfo.stPosMod[0].stObjArray.stObjPos.width,
            rslt_dhd->stPosInfo.stPosMod[0].stObjArray.stObjPos.height);

    BA_LOGW("------------------------- End ------------------------\n");

    return SAL_SOK;
}

/**
 * @function:   Ba_DrvResultCallBack
 * @brief:      引擎结果回调接口
 * @param[in]:  INT32  nNodeID
 * @param[in]:  INT32  nCallBackType
 * @param[in]:  VOID   *pstOutput
 * @param[in]:  UINT32 nSize
 * @param[in]:  VOID   *pUsr
 * @param[in]:  INT32  nUserSize
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvResultCallBack(INT32 nNodeID,
                           INT32 nCallBackType,
                           VOID *pstOutput,
                           UINT32 nSize,
                           VOID *pUsr,
                           INT32 nUserSize)
{
    INT32 s32Ret = SAL_SOK;

    void *pstAlgDataPkt = NULL;
    SBAE_OUTPUT_T *pstRslt = NULL;
    ICF_MEDIA_INFO *pstInputYuvData = NULL;
    BA_INPUT_PRVT_DATA_S *pstIptPrvtData = NULL;

    BA_DRV_CHECK_PTR(pstOutput, exit, "pstOutput == null!");
    pstAlgDataPkt = (void *)pstOutput;

    /* 原始数据获取 */
    pstInputYuvData = (ICF_MEDIA_INFO *)ICF_GetDataPtrFromPkg(pstAlgDataPkt, ICF_ANA_MEDIA_DATA);
    BA_DRV_CHECK_PTR(pstInputYuvData, exit, "pstInputYuvData == null!");

    /* 输出结果获取 */
    pstRslt = (SBAE_OUTPUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPkt, SBAE_RESULT_NODE_TYPE);
    BA_DRV_CHECK_PTR(pstRslt, exit, "pstRslt == null!");

    /* 打印检测结果(调试使用) */
    s32Ret = Ba_DrvPrintOutputResult(pstRslt, pstInputYuvData, pstRslt->stPosInfo.unFrameIdx);
    BA_DRV_CHECK_RET(s32Ret, exit, "OutputResult failed!");
    
    /* 保存获取到的结果数据用于后续处理 */
    s32Ret = Ba_DrvGetOutResult(pstRslt, pstInputYuvData);
    BA_DRV_CHECK_RET_NO_LOOP(s32Ret, "Ba_DrvGetOutResult failed!");

exit:
    pstIptPrvtData = (BA_INPUT_PRVT_DATA_S *)pstInputYuvData->stInputInfo.pUserPtr;
    BA_DRV_CHECK_PTR(pstIptPrvtData, exit, "pstIptPrvtData == null!");

#ifdef DEBUG_LOG
    BA_LOGE("rslt: chan %d, idx %d, %p \n", pstIptPrvtData->uiChn, pstIptPrvtData->uiInputIdx, pstIptPrvtData);
#endif

    /* 释放input数据节点 */
    s32Ret = Ba_DrvPutInputData(pstIptPrvtData->uiChn, pstIptPrvtData->uiInputIdx);
    BA_DRV_CHECK_RET(s32Ret, exit, "Ba_DrvPutInputData failed!");

    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvSendFrameThread
 * @brief:      行为分析取帧线程
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
static void *Ba_DrvSendFrameThread(void *prm)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 chan = 0;
    UINT32 uiVdecChn = 0;
    UINT32 CurTime = 0;
    UINT32 LastTime = 0;
    UINT32 uiFailCnt = 0;
    UINT32 uiLastStatus = 0;   /* 0表示上一次获取解码帧失败，1表示成功 */

    BA_DEV_PRM *pstBa_dev = NULL;
    BA_MOD_S *pstBaModPrm = NULL;

    BA_FRAME_INFO_ST stYuvFrame = {0};

    pstBaModPrm = Ba_HalGetModPrm();
    BA_DRV_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    BA_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstBa_dev = (BA_DEV_PRM *)prm;

    if (0x00 == stYuvFrame.stFrame.uiAppData)
    {
        s32Ret = disp_hal_getFrameMem(BA_CHN_WIDTH, BA_CHN_HEIGHT, &stYuvFrame.stFrame);
        BA_DRV_CHECK_RET(s32Ret, err, "disp_hal_getFrameMem failed!");

    }

    SAL_SET_THR_NAME();

    while (SAL_TRUE)
    {
        Ba_drvChnWait(pstBa_dev);
		
		/* 若底层资源未完成初始化 or 通道未开启则休眠等待 */
        if (!pstBaModPrm->uiInitFlag || !pstBa_dev->uiChnStatus)
        {
            usleep(10*1000);
            continue;
        }

        uiVdecChn = pstBa_dev->uiVdecChn;

        CurTime = SAL_getCurMs(); /* %1000000; */
        if (CurTime < LastTime + BA_INPUT_FRAME_GAP_MS)
        {
            /* 保证每秒2帧 */
            usleep((BA_INPUT_FRAME_GAP_MS + LastTime - CurTime) * 1000);
            continue;
        }

        SAL_dbPrintf(Ba_HalGetDbLevel(), DEBUG_LEVEL2, "%s|%d Last %d, now %d, gap %d\n", __func__, __LINE__,
                     LastTime, SAL_getCurMs(), SAL_getCurMs() - LastTime);

        /* 获取解码后的帧数据, 此处增加解码流程异常时避免日志刷屏的处理 */
        if (SAL_SOK != Ba_DrvGetFrame(pstBa_dev, &stYuvFrame.stFrame))
        {
            usleep(80 * 1000);

            if (0 == uiLastStatus)
            {
                uiFailCnt++;
                uiLastStatus = 0;
                if (120 == uiFailCnt)
                {
                    uiFailCnt = 0;
                    BA_LOGE("Vdec %d Get Frame fail for 120 times! Pls Check if IPC is down or the VdecChan is stopped!\n", uiVdecChn);
                }
            }
            else
            {
                uiLastStatus = 0;
                BA_LOGE("Vdec %d Get Frame fail!\n", uiVdecChn);
            }

            /* 失败时reuse */
			goto reuse;
        }
        else
        {
            uiFailCnt = 0;
            uiLastStatus = 1;
        }

        s32Ret = Ba_DrvInputDataToEngine(chan, &stYuvFrame.stFrame);
        BA_DRV_CHECK_RET(s32Ret, reuse, "Ba_DrvInputDataToEngine failed!");

reuse:
		LastTime = CurTime;
        continue;
    }

err:
    return NULL;
}

/**
 * @function:   Ba_DrvModuleDeInit
 * @brief:      行为分析通道去初始化
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvModuleDeInit(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiTimeOutCnt = 0;

    BA_DEV_PRM *pstBa_dev = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstBa_dev = Ba_DrvGetDev(0);                /* 行为分析目前只有一个通道 */
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    if (SAL_TRUE != pstBa_dev->uiChnStatus)
    {
        BA_LOGI("BA Module is Closed, return Success! chan %d last chan %d \n", chan, pstBa_dev->chan);
        return SAL_SOK;
    }

    #if 0  /* 抓拍状态暂不使用，后续若确定不启用则删除 */
    if (SAL_SOK != Ba_DrvCaptSwitch(0))
    {
        BA_LOGE("BA Disable Capt Fail!\n");
        return SAL_FAIL;
    }

    BA_LOGI("BA Disable Capt Success!\n");
    #endif

    pstBa_dev->uiChnStatus = SAL_FALSE;

    usleep(300 * 1000);  /* 等待线程休眠后再进行资源去初始化 */

    while (1 != pstBa_dev->uiRlsFlag)
    {
        BA_LOGW("Pls Waiting for Dec Chn's Destroying!\n");
        usleep(20 * 1000);
        uiTimeOutCnt++;
        if (20 == uiTimeOutCnt)
        {
            BA_LOGW("Time out!\n");
            break;
        }
    }

    s32Ret = jpeg_drv_deleteChn(pstBa_dev->stJpegVenc.pstJpegChnInfo);
    BA_DRV_CHECK_RET(s32Ret, err, "Jpeg_drvDeleteChn failed!");

    pstBa_dev->stJpegVenc.bDecChnStatus = SAL_FALSE;

#if 0  /* fixme: 行为分析资源不进行去初始化，临时规避线程方式初始化引擎资源时存在异常内存泄露的问题 */
    s32Ret = Ba_HalDeinit();
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_HalEngineDeInit failed!");
#endif 

    pstBa_dev->uiFrameNum = 0;

    BA_LOGI("BA Module chan %d DeInit OK!\n", pstBa_dev->chan);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvSetProcModeThread
 * @brief:      配置模式切换
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
static void *Ba_DrvSetModeInitThread()
{
    INT32 s32Ret = SAL_SOK;

    SAL_SET_THR_NAME();

    s32Ret = Ba_HalInit();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Ba_HalInit Failed! \n");
        goto EXIT;
    }

    PPM_LOGI("Ba_HalInit End! \n");

EXIT:

    SAL_thrExit(NULL);
    return NULL;
}

/**
 * @function:   Ba_DrvModeInitThread
 * @brief:      模块资源初始化线程
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ba_DrvModeInitThread(VOID)
{
    SAL_ThrHndl Handle;

	BA_MOD_S *pstHalPrm = NULL;

	pstHalPrm = Ba_HalGetModPrm();
	BA_DRV_CHECK_PTR(pstHalPrm, exit, "get hal mod prm failed!");

	/* fixme: 此处规避线程方式创建底层资源时存在异常内存泄露的问题，临时仅创建一次 */
	if (pstHalPrm->uiInitFlag)
	{
		BA_LOGI("ba resource has created! return success! \n");
		return SAL_SOK;
	}
	
    /* hcnn库中存在1M多的栈内存使用，故此处创建2M的线程栈用于模式切换 */
    if (SAL_SOK != SAL_thrCreate(&Handle, Ba_DrvSetModeInitThread, SAL_THR_PRI_DEFAULT, 2 * 1024 * 1024, NULL))
    {
        SVA_LOGE("Create Engine Init Thread Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SAL_thrDetach(&Handle))
    {
        SVA_LOGE("pthread detach failed! \n");
        return SAL_FAIL;
    }
    return SAL_SOK;
exit:
	return SAL_FAIL;
}

/**
 * @function:   Ba_DrvModuleInit
 * @brief:      行为分析通道初始化
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvModuleInit(UINT32 chan, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 uiTimeOut = 300;              /* 300 * 100ms = 30s */
    UINT32 i = 0;
    UINT32 uiVdecChn = 0;

//    static UINT32 bFirstFlag = SAL_TRUE;
//    static UINT32 uiLastVdecChn = 0;

//    IA_UPDATE_OUTCHN_PRM stOutChnPrm = {0};
    SAL_VideoFrameParam stHalPrm = {0};

    DSA_QueHndl *pstFullQue;
    DSA_QueHndl *pstEmptQue;
    BA_FRAME_INFO_ST *pstYuvFrame = NULL;

    BA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    BA_DRV_CHECK_PTR(prm, err, "prm == null!");

    uiVdecChn = *(UINT32 *)prm;

    BA_DEV_PRM *pstBa_dev = Ba_DrvGetDev(0);
    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    if (SAL_TRUE != g_ba_common.uiPublic)
    {
        BA_LOGE("Ba Module Initialize failed! pls check! \n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstBa_dev->uiChnStatus)
    {
        BA_LOGI("BA Module is Running, return Success! chan %d last chan %d\n", chan, pstBa_dev->chan);
        return SAL_SOK;
    }

    pstBa_dev->chan = chan;
    pstBa_dev->uiVdecChn = uiVdecChn;

    /* 开启行为分析需要先创建编码通道 */
    if (SAL_FALSE == pstBa_dev->stJpegVenc.bDecChnStatus)
    {
        sal_memset_s(&stHalPrm, sizeof(SAL_VideoFrameParam), 0x00, sizeof(SAL_VideoFrameParam));

        stHalPrm.fps = 1;
        stHalPrm.width = BA_CHN_WIDTH;
        stHalPrm.height	= BA_CHN_HEIGHT;
        stHalPrm.encodeType = MJPEG;
        stHalPrm.quality = 80;
        stHalPrm.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

        s32Ret = jpeg_drv_createChn((void **)&pstBa_dev->stJpegVenc.pstJpegChnInfo, &stHalPrm);
        BA_DRV_CHECK_RET(s32Ret, err, "Jpeg_drvCreateChn failed!");

        pstBa_dev->stJpegVenc.uiJpegEncChn = pstBa_dev->stJpegVenc.pstJpegChnInfo->u32Chan;
        pstBa_dev->stJpegVenc.bDecChnStatus = SAL_TRUE;

        BA_LOGI("BA Create Dec Chan %d Success! bDecChnStatus %d\n",
                pstBa_dev->stJpegVenc.uiJpegEncChn,
                pstBa_dev->stJpegVenc.bDecChnStatus);
    }

    Ba_HalPrint("Jpeg_drvCreateChn");

    #if 0  /* 抓拍状态暂不使用，后续若确定不启用则删除 */
    if (SAL_SOK != Ba_DrvCaptSwitch(1))
    {
        BA_LOGE("BA Capture enable fail!\n");
        return SAL_FAIL;
    }

    BA_LOGI("BA Enable Capt Success!\n");
    #endif

    s32Ret = Ba_DrvModeInitThread();
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvModeInitThread failed!");

    while (!Ba_HalGetInitFlag() && uiTimeOut > 0)
    {
        usleep(100 * 1000);

        if (uiTimeOut > 0)
        {
            uiTimeOut--;
        }
		
    }
    Ba_HalPrint("Ba_HalEngineInit");

#if 0
    /* 更新解码输出的VPSS宽高，因为IPC输出无法设置为1280*720 */
    if (SAL_TRUE == bFirstFlag || uiLastVdecChn != uiVdecChn)
    {
        stOutChnPrm.uiVdecChn = uiVdecChn;
        stOutChnPrm.enModule = IA_MOD_BA;
        stOutChnPrm.uiWidth = BA_CHN_WIDTH;
        stOutChnPrm.uiHeight = BA_CHN_HEIGHT;   /* 等比例缩放，暂时手动修改 */
        stOutChnPrm.uiVpssChn = 3;

        s32Ret = IA_UpdateOutChnResolution(&stOutChnPrm);
        BA_DRV_CHECK_RET(s32Ret, err, "IA_UpdateOutChnResolution failed!");

        if (SAL_TRUE == bFirstFlag)
        {
            bFirstFlag = SAL_FALSE;
        }
    }

    /* 保留当前解码通道号配置 */
    uiLastVdecChn = uiVdecChn;
#endif

    pstFullQue = pstBa_dev->stYuvQueInfo.pstYuvQuePrm->pstFullQue;
    pstEmptQue = pstBa_dev->stYuvQueInfo.pstYuvQuePrm->pstEmptQue;

    BA_DRV_CHECK_PTR(pstFullQue, err, "pstFullQue == null!");
    BA_DRV_CHECK_PTR(pstEmptQue, err, "pstEmptQue == null!");

    for (i = 0; i < BA_MAX_YUV_BUF_NUM; i++)
    {
        (VOID)DSA_QueGet(pstFullQue, (void **)&pstYuvFrame, SAL_TIMEOUT_NONE);

        if (NULL != pstYuvFrame)
        {
            (VOID)DSA_QuePut(pstEmptQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);
        }
    }

    BA_LOGI("Clear Buf Que OK!\n");

    if (NULL != pstBa_dev->stJpegVenc.pPicJpeg)
    {
        sal_memset_s(pstBa_dev->stJpegVenc.pPicJpeg, BA_JPEG_SIZE, 0, BA_JPEG_SIZE);
    }
	
    (VOID)Ba_drvChnSignal(pstBa_dev);

    pstBa_dev->uiChnStatus = SAL_TRUE;

    BA_LOGI("BA Module: Init BA Chn [%d] OK! Vdec Chn %d\n", pstBa_dev->chan, uiVdecChn);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvThrInit
 * @brief:      行为分析线程初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_DrvThrInit(void)
{
    BA_DEV_PRM *pstBa_dev = NULL;

    pstBa_dev = Ba_DrvGetDev(0);

    BA_DRV_CHECK_PTR(pstBa_dev, err, "pstBa_dev == null!");

    /* 线程资源初始化 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstBa_dev->mChnMutexHdl);

    /* 取帧线程 */
    SAL_thrCreate(&pstBa_dev->stFrmThrHandl, Ba_DrvSendFrameThread, SAL_THR_PRI_DEFAULT, 0, pstBa_dev);

    /* 算法处理线程 */
    SAL_thrCreate(&pstBa_dev->stProcThrHandl, Ba_DrvProcThread, SAL_THR_PRI_DEFAULT, 0, pstBa_dev);

    /* 结果处理线程 */
    /* SAL_thrCreate(&pstBa_dev->stRltThrHandl, Ba_DrvRsltThread, SAL_THR_PRI_MAX, 0, pstBa_dev); */

    BA_LOGI("Ba_DrvThrInit OK!\n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_DrvInit
 * @brief:      行为分析驱动层初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvInit(void)
{
    INT32 s32Ret = SAL_FAIL;

    (VOID)Ba_HalInitRtld();

    s32Ret = Ba_DrvInitDefaultPrm(0);   /* 目前行为分析只有一个通道，默认通道号0，下同*/
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvInitDefaultPrm failed!");

    Ba_HalPrint("Ba_DrvInitDefaultPrm");

    s32Ret = Ba_DrvThrInit();
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvThrInit failed!");

    Ba_HalPrint("Ba_DrvThrInit");	

    s32Ret = Ba_DrvInitInputDataPrm(0);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvInitInputDataPrm failed!");

    Ba_HalPrint("Ba_DrvInitInputDataPrm");

    s32Ret = Ba_DrvInitGlobalVar(0);
    BA_DRV_CHECK_RET(s32Ret, err, "Ba_DrvInitInputDataPrm failed!");

    (VOID)Ba_HalSetCbFunc(0, (BA_CALL_BACK_FUNC)Ba_DrvResultCallBack);   /* BA_TODO: 将回调部分处理放到公共模块 */

    g_ba_common.uiPublic = SAL_TRUE; /* 暂未使用 */

    BA_LOGI("Ba_DrvInit Success !\n");
    return SAL_SOK;

err:
    (VOID)Ba_DrvDeInitInputDataPrm(0);
    return SAL_FAIL;
}


