/**
 * @file   disp_tsk.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  显示模块
 * @author liuxianying
 * @date   2021/8/3
 * @note
 * @note \n History
   1.日    期: 2021/8/3
     作    者: liuxianying
     修改历史: 创建文件

   2.日    期: 2022/1/22
     作    者: yeyanzhong
     修改历史: 增加手动拼帧功能，在主线程里disp_tskVoChnThreadStitch做
 **/

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "disp_tsk_api.h"
#include "disp_tsk_inter.h"
#include "sys_tsk.h"
#include "dup_tsk_api.h"
#include "system_prm_api.h"
#include "task_ism.h"
#include "vdec_tsk_api.h"
#include "vca_unpack_api.h"
#include "disp_chip_drv_api.h"
#include "system_plat_common.h"

#line __LINE__ "disp_tsk.c"

#define DISP_TASK_CHECK_RET(ret, ret_success, str) \
    { \
        if (ret != ret_success) \
        { \
            DISP_LOGE("%s, ret: 0x%x \n", str, ret); \
            return SAL_FAIL; \
        } \
    }
#define DISP_TASK_CHECK_NULL_PTR(ptr, str) \
    { \
        if (!ptr) \
        { \
            DISP_LOGE("%s  \n", str); \
            return SAL_FAIL; \
        } \
    }

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define DISP_TASK_NAME "disp_tsk"

/*解码显示DUP通道固定为0：原因当前linktak out_disp0命名固定无法根据后级调整out_disp不可无限制增加，解码当前需求只会显示一路*/
#define DISP_VDEC_DUP_CHN       (0)
/*************debug调试*************/
#define DUMP_NOSIGNAL			(0)
#define DUMP_VO					(1)
#define DUMP_NOSIGNAL_BEFORE	(2)
/*************debug end*************/

/*****************************************************************************
                            全局结构体
*****************************************************************************/
static DISP_MODULE_COMMON g_disp_common = {0};
static DISP_MAGNIFIER_COMMON gMagnifer[DISP_HAL_MAX_DEV_NUM] = {0};
static SVA_PROCESS_IN g_stIn[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM];
static SVA_PROCESS_OUT g_stOut[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM];
static DISP_TAR_CNT_INFO g_stPkgTarCnt[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM];

/*************debug调试*************/
static INT32 dispDbLevel = DEBUG_LEVEL0;
DISP_DEBUG_CTL gDispDebug[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM];
/*************debug end*************/


#ifdef DISP_VIDEO_FUSION
static UINT64 g_aU64PushInNum[DISP_HAL_MAX_DEV_NUM] = {0};
/* 送VO显示的循环缓冲区 */
static DUP_DISP_DATA_BUFF_POOL stDupDispPool[DISP_HAL_MAX_DEV_NUM];
/* 除了0通道，其他通道从vpe获取帧数据之后先拷贝到临时缓冲区stDupCopyDispPool，vo chn 0送显示时把临时缓冲区里的图像帧通过dma或gfx叠加上去 */
static DUP_COPY_DISP_DATA_BUFF_POOL stDupCopyDispPool[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM];

static INT32 disp_cleardupCopyDispPool(DISP_CHN_COMMON *pDispChn);
static INT32 disp_clearPushIntoVoNum(UINT32 u32voDev);

#endif



/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   disp_tskGetTimeMilli
 * @brief      获取系统时间
 * @param[in]  void
 * @param[out] None
 * @return     UINT64
 */
UINT64 disp_tskGetTimeMilli(void)
{
    struct timeval tv;
    UINT64 time = 0;

    gettimeofday(&tv, NULL);
    time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time;
}

/**
 * @function   disp_tskGetVoDev
 * @brief      获取显示设备
 * @param[in]  UINT32 uiDev 设备号
 * @param[out] None
 * @return     DISP_DEV_COMMON *
 */
DISP_DEV_COMMON *disp_tskGetVoDev(UINT32 uiDev)
{
    if (uiDev < g_disp_common.uiDevMaxCnt)
    {
        return g_disp_common.pstDevObj[uiDev];
    }

    return NULL;
}

/**
 * @function   disp_tskGetVoCnt
 * @brief      获取显示设备最大数(应用配置下来的数值)
 * @param[in]  VOID
 * @param[out] None
 * @return     UINT32
 */
UINT32 disp_tskGetVoCnt(VOID)
{
    return g_disp_common.uiDevMaxCnt;
}

/**
 * @function   disp_tskGetVoChn
 * @brief      获取显示通道
 * @param[in]  UINT32 uiDev  设备号
 * @param[in]  UINT32 uiChn  通道号
 * @param[out] None
 * @return     DISP_CHN_COMMON *
 */
DISP_CHN_COMMON *disp_tskGetVoChn(UINT32 uiDev, UINT32 uiChn)
{
    if (uiDev < disp_tskGetVoCnt())
    {
        if (uiChn < g_disp_common.pstDevObj[uiDev]->szLayer.uiMaxChnCnt)
        {
            return g_disp_common.pstDevObj[uiDev]->szLayer.pstVoChn[uiChn];
        }
    }

    return NULL;
}

/**
 * @function   disp_tskGetVoChnCnt
 * @brief      获取显示设备上最大通道数(应用配置下来)
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     UINT32 cnt 通道数
 */
UINT32 disp_tskGetVoChnCnt(UINT32 uiDev)
{
    UINT32 cnt = 0;

    DISP_DEV_COMMON *pDispDev = NULL;

    if (uiDev < disp_tskGetVoCnt())
    {
        pDispDev = disp_tskGetVoDev(uiDev);
        if (pDispDev != NULL)
        {
            cnt = pDispDev->szLayer.uiMaxChnCnt;
        }
    }

    return cnt;
}

/**
 * @function   disp_tskCheckVoIllegal
 * @brief      检测显示设备号合法性
 * @param[in]  UINT32 chan
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskCheckVoIllegal(UINT32 chan)
{
    UINT32 uiMaxDevCnt = disp_tskGetVoCnt();

    if (chan > (uiMaxDevCnt - 1))
    {
        DISP_LOGW("Chan (Illegal parameters),Vo Dev %d > Max %d (0~%d)\n", chan, uiMaxDevCnt, (uiMaxDevCnt - 1));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_drvVoChnWait
* 描  述  : 等待VO通道使能
* 输  入  : - pDispChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_tskVoChnWait(DISP_CHN_COMMON *pDispChn)
{
    if (NULL == pDispChn)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    SAL_mutexLock(pDispChn->mChnMutexHdl);

    if (SAL_FALSE == pDispChn->uiEnable)
    {
        SAL_mutexWait(pDispChn->mChnMutexHdl);
    }

    SAL_mutexUnlock(pDispChn->mChnMutexHdl);

    return SAL_SOK;
}

/**
 * @function   disp_tskVoChnCtrl
 * @brief      设置通道使能状态
 * @param[in]  DISP_CHN_COMMON *pDispChn
 * @param[in]  UINT32 Ctrl      设置使能
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskVoChnCtrl(DISP_CHN_COMMON *pDispChn, UINT32 Ctrl)
{
    if (NULL == pDispChn)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    SAL_mutexLock(pDispChn->mChnMutexHdl);
    pDispChn->uiEnable = Ctrl;
    pDispChn->srcStatus = 0;
    if (pDispChn->uiEnable == SAL_TRUE)
    {
        SAL_mutexSignal(pDispChn->mChnMutexHdl);
    }

    SAL_mutexUnlock(pDispChn->mChnMutexHdl);

    return SAL_SOK;
}

/**
 * @function   disp_tskSetVoChnStatus
 * @brief      设置显示通道状态
 * @param[in]  DISP_CHN_COMMON *pDispChn
 * @param[in]  UINT32 uiSrcStatus  原始状态
 * @param[in]  UINT32 uiDstStatus  设置状态
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskSetVoChnStatus(DISP_CHN_COMMON *pDispChn, UINT32 uiSrcStatus, UINT32 uiDstStatus)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == pDispChn)
    {
        DISP_LOGE("pDispChn is NULL\n");
        return SAL_FAIL;
    }

    SAL_mutexLock(pDispChn->mChnMutexHdl);
    if (pDispChn->uiEnable == uiSrcStatus)
    {
        pDispChn->uiEnable = uiDstStatus;
        pDispChn->srcStatus = 0;
        s32Ret = SAL_SOK;
    }

    SAL_mutexUnlock(pDispChn->mChnMutexHdl);


    /* 后续根据帧率不同来修改延时，当前使用60帧处理60帧以下兼容*/
    /*usleep((1000/pDispChn->uiFps)*1000);*/
    usleep(16 * 1000); /* 保证当前数据显示完成，按照60帧处理 */

    return s32Ret;
}

/**
 * @function   disp_tskCheckVoChnPrm
 * @brief      校验VoChn通道是否超出范围
 * @param[in]  DISP_LAYER_COMMON *pstLayer  视频层
 * @param[in]  DISP_REGION *pstRegin        窗口显示区域
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskCheckVoChnPrm(DISP_LAYER_COMMON *pstLayer, DISP_REGION *pstRegin)
{
    /* INT32 s32Ret = 0; */

    if (pstRegin == NULL || pstLayer == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    if (pstRegin->x > pstLayer->uiLayerWith || pstRegin->y > pstLayer->uiLayerHeight
        || pstRegin->w > pstLayer->uiLayerWith || pstRegin->h > pstLayer->uiLayerHeight)
    {
        DISP_LOGE("voDev %d alloc( %d, %d ) w %d h %d,layer w %d h %d \n", pstLayer->uiLayerNo, pstRegin->x, pstRegin->y, pstRegin->w, pstRegin->h, pstLayer->uiLayerWith, pstLayer->uiLayerHeight);
        return SAL_FAIL;
    }
    else if (((pstRegin->x + pstRegin->w) > pstLayer->uiLayerWith) || ((pstRegin->y + pstRegin->h) > pstLayer->uiLayerHeight))
    {
        DISP_LOGE("voDev %d alloc( %d, %d ) w %d h %d,layer w %d h %d \n", pstLayer->uiLayerNo, pstRegin->x, pstRegin->y, pstRegin->w, pstRegin->h, pstLayer->uiLayerWith, pstLayer->uiLayerHeight);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskVoChnClearBuf
 * @brief      清空对应VoChn对应数据
 * @param[in]  UINT32 uiVoDev
 * @param[in]  VOID *prm
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskVoChnClearBuf(UINT32 uiVoDev, VOID *prm)
{
    UINT32 i = 0;
    INT32 s32Ret = SAL_FAIL;
    UINT32 uiVoChn = 0;
    UINT32 uiMaxVoChn = 0;
    UINT32 uiLayerNo = 0;
    static UINT32 cnt = 0;   /* 用来线程同步使用 */

    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CLEAR_CTRL *pstDispCtrl = NULL;

    if ((NULL == prm) || (DISP_MAX_DEV_NUM <= uiVoDev))
    {
        DISP_LOGE("ptr == NULL or vodev[%d] invalid!\n", uiVoDev);
        return SAL_FAIL;
    }

    pstDispCtrl = (DISP_CLEAR_CTRL *)prm;

    uiMaxVoChn = disp_tskGetVoChnCnt(uiVoDev);

    for (i = 0; i < pstDispCtrl->uiCnt; i++)
    {
        uiVoChn = pstDispCtrl->uiChan[i];

        if (uiVoChn > uiMaxVoChn)
        {
            DISP_LOGE("invalid vo chan %d\n", uiVoChn);
            continue;
        }

        pDispChn = disp_tskGetVoChn(uiVoDev, uiVoChn);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;
        }

        if (SAL_TRUE != pDispChn->uiEnable)
        {
            DISP_LOGW("i %d vo chn %d is not open!\n", i, uiVoChn);
            continue;
        }

        pDispChn->flag = 0;
        while (1 != pDispChn->flag)
        {
            usleep(50 * 1000);
            cnt++;
            if (100 < cnt)   /*超时100*50ms=5s直接退出，防止设备卡死*/
            {

                DISP_LOGE("wait cnt %d failed\n", cnt);
                pDispChn->flag = 1;
                cnt = 0;
                return SAL_FAIL;
            }

            if (pDispChn->uiEnable == SAL_FALSE)
            {
                pDispChn->flag = 1;
                break;
            }

            DISP_LOGW("wait cnt %d\n", cnt);
        }

        cnt = 0;

        uiLayerNo = pDispChn->uiLayerNo;

        /* 清空Vo chan缓存 */
        s32Ret = disp_hal_clearVoBuf(uiLayerNo, uiVoChn, SAL_TRUE);  /* HI_FALSE保留一帧数据，HI_TRUE全部清空 */
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("Errrrrr! uiLayerNo %d uiVoChn %d\n", uiLayerNo, uiVoChn);
            /* return SAL_FAIL; */
        }

        /* 重新给Vo窗口填充HIKVISION图片LOGO */
        /* pDispChn->uiNoSignal = SAL_TRUE; */

        DISP_LOGI("i %d Vo %d Chn %d LayerNo %d Clear Buf OK!\n", i, uiVoDev, uiVoChn, uiLayerNo);
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskGetVoDupHandle
 * @brief      获取显示VO通道前一级duphandle
 * @param[in]  UINT32 voDev               显示设备号
 * @param[in]  DISP_CHN_COMMON *pDispChn  显示通道参数
 * @param[out] None
 * @return     static DUP_ChanHandle *
 */
static DUP_ChanHandle *disp_tskGetVoDupHandle(UINT32 voDev, DISP_CHN_COMMON *pDispChn)
{
    INT32 s32Ret = SAL_FAIL;
    CHAR szName[NAME_LEN] = {0};
    CHAR szDistName[NAME_LEN] = {0};
    CHAR *pszSrcNodeName = NULL;
    DUP_ChanHandle *dupOutChnHandle = NULL;
    INST_INFO_S *pstSrcInst = NULL;
    UINT32 uiInDev = 0;

    if (NULL == pDispChn)
    {
        SAL_WARN("pDispChn = NULL !!!\n");
        return NULL;
    }

    uiInDev = pDispChn->stInModule.uiChn;

    snprintf(szName, NAME_LEN, "DISP%d_CHN%d", voDev, pDispChn->uiChn);
    s32Ret = link_drvGetSrcNode(szName, "in_0", &pstSrcInst, &pszSrcNodeName);
    if (SAL_isFail(s32Ret))
    {
        if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
        {
            snprintf(szName, NAME_LEN, "DUP_VDEC_%d", uiInDev);
            pstSrcInst = link_drvGetInst(szName);
            snprintf(szDistName, NAME_LEN, "out_disp%d", DISP_VDEC_DUP_CHN);
            dupOutChnHandle = (DUP_ChanHandle *)(link_drvGetHandleFromNode(pstSrcInst, szDistName));
        }
        else
        {
            /* SAL_WARN("no node   link to  disp chn = %d  szName = %s !!!\n", pDispChn->uiChn, szName); */
            dupOutChnHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
        }
    }
    else
    {
        dupOutChnHandle = (DUP_ChanHandle *)link_drvGetHandleFromNode(pstSrcInst, pszSrcNodeName);
    }

    return dupOutChnHandle;
}

/**
 * @function   disp_tskVoChnStop
 * @brief      停止VOdev对应已经使能的通道
 * @param[in]  UINT32 uiDev
 * @param[in]  VOID *prm
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskVoChnStop(UINT32 uiDev, VOID *prm)
{
    UINT32 i = 0;
    INT32 s32Ret = 0;
    UINT32 uiVoChn = 0;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CLEAR_CTRL *pstDispCtrl = NULL;
    UINT64 u64StartTime = 0;
    UINT64 u64EndTime = 0;

#ifdef DISP_VIDEO_FUSION
    UINT32 au32BgColor[4] = {0xEB, 0x80, 0x80};
#endif

    pstDispCtrl = (DISP_CLEAR_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    if (pstDispCtrl->uiCnt == 0x00 || pstDispCtrl->uiCnt > disp_tskGetVoChnCnt(uiDev))
    {
        DISP_LOGE("error uiCnt %d \n", pstDispCtrl->uiCnt);
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    /* 清除显示设备上已经使能的显示通道 */
    for (i = 0; i < pstDispCtrl->uiCnt; i++)
    {
        uiVoChn = pstDispCtrl->uiChan[i];

        pDispChn = disp_tskGetVoChn(uiDev, uiVoChn);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error !\n");
            return SAL_FAIL;
        }

        u64StartTime = disp_tskGetTimeMilli();
        pDispChn->enCleanStatus = DISP_CLEAN_STATUS_START;
        pDispChn->stInModule.uiHandle = 0;    /* 关窗口清handle */

        s32Ret = disp_tskSetVoChnStatus(pDispChn, SAL_TRUE, SAL_FALSE);
        if (s32Ret == SAL_SOK)
        {
            pDispChn->enCleanStatus = DISP_CLEAN_STATUS_CLEANING;
            u64EndTime = disp_tskGetTimeMilli();
            if ((u64EndTime - u64StartTime) > 200)
            {
                DISP_LOGW("chn[%u] change clean status cost too much, start time:%llu ms, end time:%llu ms, interval:%llu ms\n",
                          pDispChn->uiChn, u64StartTime, u64EndTime, (u64EndTime - u64StartTime));
            }

            s32Ret = disp_hal_disableChn(pDispChn);
            if (SAL_SOK != s32Ret)
            {
                pDispChn->enCleanStatus = DISP_CLEAN_STATUS_IDLE;
                DISP_LOGE("disp_hal_disableChn failed, dev: %u, idx: %u, chn: %u\n", uiDev, i, uiVoChn);
                return SAL_FAIL;
            }
        }
        else
        {
            if (SAL_TRUE == pDispChn->uiEnable)
            {
                DISP_LOGE("disp_tskSetVoChnStatus failed, dev: %u, idx: %u, chn: %u\n", uiDev, i, uiVoChn);
            }
        }

        pDispChn->enCleanStatus = DISP_CLEAN_STATUS_IDLE;
    }

#ifdef DISP_VIDEO_FUSION
    /* vo关闭时，需要清楚pushInNum */
    disp_clearPushIntoVoNum(uiDev);

    for (i = 0; i < DISP_BUF_FRM_CNT; i++)
    {
        disp_hal_clearFrameMem(&stDupDispPool[uiDev].stSysFrm[i], au32BgColor);
    }

#endif

    return SAL_SOK;
}

/**
 * @function   disp_tskSetVoChnDupPrm
 * @brief      设置显示VO通道前一级dup参数
 * @param[in]  UINT32 voDev
 * @param[in]  DISP_CHN_COMMON *pDispChn  显示通道参数
 * @param[in]  PARAM_INFO_S *stParamInfo  设置参数
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskSetVoChnDupPrm(UINT32 voDev, DISP_CHN_COMMON *pDispChn, PARAM_INFO_S *stParamInfo)
{
    INT32 s32Ret = SAL_FAIL;
    DUP_ChanHandle *dupOutChnHandle = NULL;

    if (NULL == pDispChn || NULL == stParamInfo)
    {
        SAL_WARN("pDispChn = NULL !!!\n");
        return SAL_FAIL;
    }

    dupOutChnHandle = disp_tskGetVoDupHandle(voDev, pDispChn);
    if (NULL != dupOutChnHandle)
    {
        s32Ret = dupOutChnHandle->dupOps.OpDupSetBlitPrm((Ptr)dupOutChnHandle, stParamInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_WARN("OpDupSetBlitPrm failed !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        DISP_LOGD("dupOutChnHandle is NULL !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskGetVoChnDupFrame
 * @brief      获取显示VO通道前一级dup一帧帧数据
 * @param[in]  UINT32 voDev
 * @param[in]  DISP_CHN_COMMON *pDispChn
 * @param[in]  SYSTEM_FRAME_INFO *stGetFrame  帧数据
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskGetVoChnDupFrame(UINT32 voDev, DISP_CHN_COMMON *pDispChn, SYSTEM_FRAME_INFO *stGetFrame)
{
    INT32 s32Ret = SAL_FAIL;
    DUP_ChanHandle *dupOutChnHandle = NULL;
    DUP_COPY_DATA_BUFF pDstBuf;

    if (NULL == pDispChn || NULL == stGetFrame)
    {
        SAL_WARN("pDispChn = NULL !!!\n");
        return SAL_FAIL;
    }

    memset(&pDstBuf, 0x0, sizeof(DUP_COPY_DATA_BUFF));
    pDstBuf.pstDstSysFrame = stGetFrame;

    dupOutChnHandle = disp_tskGetVoDupHandle(voDev, pDispChn);
    if (NULL != dupOutChnHandle)
    {
        s32Ret = dupOutChnHandle->dupOps.OpDupGetBlit((Ptr)dupOutChnHandle, (Ptr) & pDstBuf);
        if (SAL_SOK != s32Ret)
        {
            SAL_WARN("OpDupGetBlitPrm failed !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_WARN("dupOutChnHandle is NULL !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskPutVoChnDupFrame
 * @brief      释放显示VO通道前一级dup一帧帧数据
 * @param[in]  UINT32 voDev
 * @param[in]  DISP_CHN_COMMON *pDispChn
 * @param[in]  SYSTEM_FRAME_INFO *pstPutFrame  帧数据
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskPutVoChnDupFrame(UINT32 voDev, DISP_CHN_COMMON *pDispChn, SYSTEM_FRAME_INFO *pstPutFrame)
{
    INT32 s32Ret = SAL_FAIL;
    DUP_ChanHandle *dupOutChnHandle = NULL;
    DUP_COPY_DATA_BUFF stDstBuf;

    if (NULL == pDispChn || NULL == pstPutFrame)
    {
        SAL_WARN("pDispChn = NULL !!!\n");
        return SAL_FAIL;
    }

    memset(&stDstBuf, 0x0, sizeof(DUP_COPY_DATA_BUFF));
    stDstBuf.pstDstSysFrame = pstPutFrame;

    dupOutChnHandle = disp_tskGetVoDupHandle(voDev, pDispChn);
    if (NULL != dupOutChnHandle)
    {
        s32Ret = dupOutChnHandle->dupOps.OpDupPutBlit((Ptr)dupOutChnHandle, (Ptr) & stDstBuf);
        if (SAL_SOK != s32Ret)
        {
            SAL_WARN("OpDupGetBlitPrm failed !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_WARN("dupOutChnHandle is NULL !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskSetVoChnAttr
 * @brief      显示参数设置（vpss vo绑定等）
 * @param[in]  UINT32 uiVoDevChn
 * @param[in]  UINT32 uiModId   绑定模块类型
 * @param[in]  VOID *prm
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskSetVoChnAttr(UINT32 uiVoDevChn, UINT32 uiModId, VOID *prm)
{
    INT32 i = 0;
    INT32 j = 0;
    INT32 s32Ret = 0;
    UINT32 uiVoChn = 0;
    UINT32 uiInDev = 0;
    UINT32 uiDraw = 0;
    UINT32 uiVoCnt = 0;
    PhysAddr uiHandle = 0;
    DISP_WINDOW_PRM stDecPrm;
    DUP_ChanHandle *pHandle = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DSP_DISP_RCV_PARAM *pDispRegion = NULL;
    CAPB_DISP *capb_disp = NULL;
    PARAM_INFO_S stParamInfo;

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pDispRegion = (DSP_DISP_RCV_PARAM *)prm;

    if (NULL == pDispRegion)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiVoCnt = disp_tskGetVoChnCnt(uiVoDevChn);

    /* 设置的参数表 */
    for (i = 0; i < pDispRegion->uiCnt; i++)
    {
        uiInDev = pDispRegion->addr[i];
        uiVoChn = pDispRegion->out[i];
        uiDraw = pDispRegion->draw[i];
        /* pip     = pDispRegion->pip[i]; */

        DISP_LOGI("InModel %d addr %d voDev %d out %d draw %d uiVoCnt %d \n", uiModId, uiInDev, uiVoDevChn, uiVoChn, uiDraw, uiVoCnt);

        if (uiModId == SYSTEM_MOD_ID_VDEC)
        {
            uiHandle = uiInDev;

            for (j = 0; j < uiVoCnt; j++)
            {
                pDispChn = disp_tskGetVoChn(uiVoDevChn, j);
                if (pDispChn == NULL)
                {
                    DISP_LOGE("error\n");
                    return SAL_FAIL;
                }

                /* 相同显示器上有同源 (指通道号) */
                if (pDispChn->stInModule.uiHandle == uiHandle && pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
                {
                    if (j == uiVoChn)
                    {
                        DISP_LOGW("voDev %d voChn %d is the same input module !\n", uiVoDevChn, j);
                    }
                    else
                    {
                        pDispChn->stInModule.uiUseFlag = SAL_FALSE;
                        pDispChn->stInModule.uiModId = 0x00;
                        pDispChn->stInModule.uiHandle = 0x00;
                        pDispChn->stInModule.uiAiFlag = SAL_FALSE;
                        pDispChn->uiNoSignal = SAL_TRUE;
                    }

                    break;
                }
            }

            pDispChn = disp_tskGetVoChn(uiVoDevChn, uiVoChn);
            if (pDispChn == NULL)
            {
                DISP_LOGE("error\n");
                return SAL_FAIL;
            }

            if (pDispChn->uiW > 0 && pDispChn->uiH > 0)
            {
                DISP_LOGI("uiVoDevChn %d wxh %d x %d!\n", uiVoDevChn, pDispChn->uiW, pDispChn->uiH);
                pDispChn->stInModule.uiChn = uiInDev;
                pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_VDEC;
                memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                stParamInfo.enType = IMAGE_SIZE_CFG;
                stParamInfo.stImgSize.u32Width = pDispChn->uiW;
                stParamInfo.stImgSize.u32Height = pDispChn->uiH;
                s32Ret = disp_tskSetVoChnDupPrm(uiVoDevChn, pDispChn, &stParamInfo);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGW("disp_tskSetVoChnDupPrm error !!\n"); /*解码通通道未创建或者未建立绑定关系仍然可以获取无视频信号*/
                }
            }
            else
            {
                DISP_LOGW("uiVoDevChn %d wxh %d x %d!\n", uiVoDevChn, pDispChn->uiW, pDispChn->uiH);
            }

            pHandle = disp_tskGetVoDupHandle(uiVoDevChn, pDispChn);
            if (NULL == pHandle)
            {
                DISP_LOGE("dev:%u chn:%u get dup handle fail\n", uiVoDevChn, uiVoChn);
            }
            else
            {
                if (pHandle->dupOps.OpDupStartBlit != NULL)
                {
                    s32Ret = pHandle->dupOps.OpDupStartBlit((Ptr)pHandle);
                    if (SAL_SOK != s32Ret)
                    {
                        DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                    }
                }
            }

            pDispChn->stInModule.uiHandle = uiHandle;
            pDispChn->stInModule.uiAiFlag = uiDraw;
            pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_VDEC;
            pDispChn->stInModule.uiUseFlag = SAL_TRUE;
            pDispChn->stInModule.uiChn = uiInDev;

            memset(&stDecPrm, 0, sizeof(DISP_WINDOW_PRM));
            stDecPrm.bind = SAL_TRUE;
            stDecPrm.enable[uiVoDevChn] = SAL_TRUE;
            stDecPrm.rect[uiVoDevChn].uiWidth = pDispChn->uiW;
            stDecPrm.rect[uiVoDevChn].uiHeight = pDispChn->uiH;
            vdec_tsk_SetDispPrm(uiInDev, &stDecPrm);

        }
        else
        {
            DISP_LOGE("uiModId = %d error\n", uiModId);
        }
    }

    /* 把已经建立绑定关系的打印出来 */
    for (j = 0; j < uiVoCnt; j++)
    {
        pDispChn = disp_tskGetVoChn(uiVoDevChn, j);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;
        }

        if (pDispChn->stInModule.uiUseFlag == SAL_TRUE)
        {
            if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
            {
                DISP_LOGI("voDev %d voChn %d Input Module < Vdec chn %lu Alg %d >\n", uiVoDevChn, j, (UINTL)pDispChn->stInModule.uiHandle, pDispChn->stInModule.uiAiFlag);
            }
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
            {
                pHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
                if (pHandle == NULL)
                {
                    DISP_LOGW("warn\n");
                    continue;
                }

                DISP_LOGI("voDev %d voChn %d Input Module < Vi chn %d Alg %d >\n", uiVoDevChn, j, sys_task_getModChn(pHandle->dupModule), pDispChn->stInModule.uiAiFlag);
            }
        }
    }

    return SAL_SOK;
}

/* @function   disp_tskSetVoChnBind
 * @brief      该接口通过link模块对VI输入通道和输出通道进-
               行绑定
 * @param[in]  UINT32 uiVoDevChn
 * @param[in]  UINT32 uiModId   绑定模块类型
 * @param[in]  VOID *prm        通道参数
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetVoChnBind(UINT32 uiVoDevChn, UINT32 uiModId, VOID *prm)
{
    INT32 i = 0;
    INT32 j = 0;

    INT32 s32Ret = 0;
    UINT32 uiVoChn = 0;
    UINT32 uiInDev = 0;
    /* UINT32 pip     = 0; */
    UINT32 uiDraw = 0;
    /* UINT32 uiVdecChn = 0; */
    UINT32 uiVoCnt = 0;
    UINT32 uiGrp = 0;
    UINT32 uiChn = 0;
    UINT32 vpssGrp = 0;
    PhysAddr uiHandle = 0;
    CHAR szName[NAME_LEN] = {0};
    CHAR szOutNodeNm[NAME_LEN] = {0};
    CHAR szInNodeNm[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;
    INST_INFO_S *pstDstInst = NULL;
    DUP_ChanHandle *pHandle = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DSP_DISP_RCV_PARAM *pDispRegion = NULL;
    DSPINITPARA *pDspInitPara = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};
    PARAM_INFO_S stParamInfo;
    DISP_WINDOW_PRM stDecPrm;

    pDspInitPara = SystemPrm_getDspInitPara();
    pDispRegion = (DSP_DISP_RCV_PARAM *)prm;

    if (NULL == pDispRegion)
    {
        SAL_ERROR("error\n");
        return SAL_FAIL;
    }

    uiVoCnt = disp_tskGetVoChnCnt(uiVoDevChn);

    /* 设置的参数表 */
    for (i = 0; i < pDispRegion->uiCnt; i++)
    {
        uiInDev = pDispRegion->addr[i];
        uiVoChn = pDispRegion->out[i];
        uiDraw = pDispRegion->draw[i];
        /* pip     = pDispRegion->pip[i]; */

        DISP_LOGI("InModel %d addr %d voDev %d out %d draw %d uiVoCnt %d \n", uiModId, uiInDev, uiVoDevChn, uiVoChn, uiDraw, uiVoCnt);

        /* 得到TSKID */
        if (uiModId == SYSTEM_MOD_ID_DUP)
        {
            snprintf(szName, NAME_LEN, "DUP_FRONT_%d", uiInDev);
            pstSrcInst = link_drvGetInst(szName);
            if (!pstSrcInst)
            {
                DISP_LOGE("get inst %s failed. \n", szName);
                return SAL_FAIL;
            }

            /* 这里约定out_disp0节点输出到显示器0上，out_disp1节点输出到显示器1上；
                        如果不做这个约定，需要上层下发哪个dup out_node输出到哪个显示器；*/
            snprintf(szOutNodeNm, NAME_LEN, "out_disp%d", uiVoDevChn);

            snprintf(szName, NAME_LEN, "DISP%d_CHN%d", uiVoDevChn, uiVoChn);
            pstDstInst = link_drvGetInst(szName);
            if (!pstDstInst)
            {
                DISP_LOGE("get inst %s failed. \n", szName);
                return SAL_FAIL;
            }

            snprintf(szInNodeNm, NAME_LEN, "in_0");

            /* 即使已经绑定也要继续往下走，因为智能画框是否开启的uiDraw标志位配置也是走该流程 */
            if (SAL_SOK == link_drvCheckCurBind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm))
            {
                ;
            }
            else
            {
                /* 先解绑 */
                link_drvUnbind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm);
                /* 进行绑定 */
                link_drvBind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm);
            }

            pHandle = link_drvGetHandleFromNode(pstSrcInst, szOutNodeNm);
            if (!pHandle)
            {
                DISP_LOGE("get handle from node failed, srcInst %s, outNodeName %s \n", pstSrcInst->szInstName, szOutNodeNm);
                return SAL_FAIL;
            }

            uiHandle = (PhysAddr)pHandle;

            for (j = 0; j < uiVoCnt; j++)
            {
                pDispChn = disp_tskGetVoChn(uiVoDevChn, j);
                if (pDispChn == NULL)
                {
                    SAL_ERROR("error\n");
                    return SAL_FAIL;
                }

                /* 相同显示器上有同源 (指通道号) */
                if (pDispChn->stInModule.uiHandle == uiHandle && pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
                {
                    if (j == uiVoChn)
                    {
                        SAL_WARN("voDev %d voChn %d is the same input module !\n", uiVoDevChn, j);
                    }
                    else
                    {
                        pDispChn->stInModule.uiUseFlag = SAL_FALSE;
                        pDispChn->stInModule.uiModId = 0x00;
                        pDispChn->stInModule.uiHandle = 0x00;
                        pDispChn->stInModule.uiAiFlag = SAL_FALSE;
                        pDispChn->uiNoSignal = SAL_TRUE;
                    }

                    SAL_WARN("pHandle %p\n", (void *)pDispChn->stInModule.uiHandle);
                    break;
                }
            }

            pDispChn = disp_tskGetVoChn(uiVoDevChn, uiVoChn);
            if (pDispChn == NULL)
            {
                SAL_ERROR("error\n");
                return SAL_FAIL;
            }

            if (pDispChn->uiW > 0 && pDispChn->uiH > 0)
            {
                DISP_LOGI("uiVoDevChn %d wxh %d x %d  bCenter %d!\n", uiVoDevChn, pDispChn->uiW, pDispChn->uiH, pDispRegion->bCenter[i]);
                pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_DUP;
                pDispChn->stInModule.uiChn = uiInDev;
                memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                if (pDispRegion->bCenter[i] && pDspInitPara->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM && pDspInitPara->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN
                    && !ISM_5030D_INPUT_DUP_CHG)
                {
                    stParamInfo.enType = IMAGE_VALID_RECT_CFG;
                    stParamInfo.stImgValidRect.stRect.u32X = 0;
                    stParamInfo.stImgValidRect.stRect.u32Y = SAL_align((pDispChn->uiH - pDspInitPara->dspCapbPar.xray_width_max) / 2, 16);
                    stParamInfo.stImgValidRect.stRect.u32W = pDspInitPara->dspCapbPar.xray_height_max;
                    stParamInfo.stImgValidRect.stRect.u32H = pDspInitPara->dspCapbPar.xray_width_max;
                    stParamInfo.stImgValidRect.stBg.u32Width = pDispChn->uiW;
                    stParamInfo.stImgValidRect.stBg.u32Height = pDispChn->uiH;
                    DISP_LOGW("disp_tskSetVoChnDupPrm x %d y %d w %d h %d bg[%d %d] !!!\n", stParamInfo.stImgValidRect.stRect.u32X, stParamInfo.stImgValidRect.stRect.u32Y, stParamInfo.stImgValidRect.stRect.u32W,
                              stParamInfo.stImgValidRect.stRect.u32H, stParamInfo.stImgValidRect.stBg.u32Width, stParamInfo.stImgValidRect.stBg.u32Height);
                }
                else
                {
                    stParamInfo.enType = IMAGE_SIZE_CFG;
                    stParamInfo.stImgSize.u32Width = pDispChn->uiW;
                    stParamInfo.stImgSize.u32Height = pDispChn->uiH;
                }

                s32Ret = disp_tskSetVoChnDupPrm(uiVoDevChn, pDispChn, &stParamInfo);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("error !!\n");
                    return SAL_FAIL;
                }

                /* RK显示启动新算法，保证图像效果 */
#ifdef RK3588
                stParamInfo.enType = SCALING_ALGORITHM_CFG;
                stParamInfo.bNewScalingAlgorithm = SAL_TRUE;

                s32Ret = disp_tskSetVoChnDupPrm(uiVoDevChn, pDispChn, &stParamInfo);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGW("Dup set scaling algorithm cfg error !!\n");
                }
#endif
            }
            else
            {
                SAL_WARN("uiInDev %d uiVoDevChn %d wxh %d x %d!\n", uiInDev, uiVoDevChn, pDispChn->uiW, pDispChn->uiH);
            }

            if (pDspInitPara->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM && 0)
            {
                uiGrp = (uiInDev == 0) ? VPSS_ENLARGE_GRP_CHN_0 : VPSS_ENLARGE_GRP_CHN_1;
                uiChn = (0 == uiVoDevChn) ? 0 : 1;
                pDispChn->stInModule.frameNum = 0;
                SAL_WARN("VPSS BIND TO VO (%d,%d),(%d,%d)\n", uiGrp, uiChn, pDispChn->uiLayerNo, pDispChn->uiChn);

                stDupBindPrm.mod = SYSTEM_MOD_ID_DUP;
                stDupBindPrm.modChn = uiGrp;
                stDupBindPrm.chn = uiChn;
                s32Ret = pHandle->dupOps.OpDupBindBlit((Ptr)pHandle, (Ptr) & stDupBindPrm, SAL_TRUE);
                if (SAL_isFail(s32Ret))
                {
                    DUP_LOGE("Bind Hal Chn err !!!\n");
                    return SAL_FAIL;
                }

                /*fixme 20210617 需用用dup_task.c里新的dup接口 */
                /* Hal_drvVpssBindToDisp(uiGrp,uiChn, pDispChn->uiLayerNo, pDispChn->uiChn); */
            }

            /* 绑定模式 */
            if (NODE_BIND_TYPE_SDK_BIND == pHandle->createFlags)
            {

            
                if( 0 == uiVoChn)  /*安检机0通道X光画面才使用新算法，节省GPU*/
                {
                    pDispChn->uiScaleAlgo = SAL_TRUE;   /*实际绑定的时候设置VO新算法参数*/
                    s32Ret = disp_hal_setVoCommonPrm(DISP_SCALING_ALGORITHM_CFG, pDispChn);
                    if (SAL_isFail(s32Ret))
                    {
                        DISP_LOGE("disp_hal_setVoCommonPrm err !!!\n");
                        return SAL_FAIL;
                    }
                }
                uiGrp = pDispChn->uiDevNo;
                uiChn = pDispChn->uiChn;
                pDispChn->stInModule.frameNum = 0;
                DISP_LOGW("VPSS (%d,%d) BIND TO VO (%d,%d)\n", pHandle->u32Grp, pHandle->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);

                stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
                stDupBindPrm.modChn = uiGrp;
                stDupBindPrm.chn = uiChn;

                s32Ret = pHandle->dupOps.OpDupBindBlit((Ptr)pHandle, (Ptr) & stDupBindPrm, SAL_TRUE);
                if (SAL_isFail(s32Ret))
                {
                    DISP_LOGE("Bind Hal Chn err !!!\n");
                    return SAL_FAIL;
                }
            }

            s32Ret = pHandle->dupOps.OpDupStartBlit((Ptr)pHandle);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
            }

            s32Ret = disp_hal_startChn(pDispChn); /* DispChn已在分屏接口里开启，但NT里要求先绑定再开启，后续可试一下把删掉这里的开启操作 */
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                /*    return SAL_FAIL; */
            }

            pDispChn->stInModule.uiHandle = uiHandle;
            pDispChn->stInModule.uiAiFlag = uiDraw;              /*智能和视频叠加信息相关*/
            pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_DUP;
            pDispChn->stInModule.uiUseFlag = SAL_TRUE;
            pDispChn->stInModule.uiChn = uiInDev;

        }
        else if (uiModId == SYSTEM_MOD_ID_VDEC) /* 目前解码没有走这里 */
        {
            uiHandle = uiInDev;

            for (j = 0; j < uiVoCnt; j++)
            {
                pDispChn = disp_tskGetVoChn(uiVoDevChn, j);
                if (pDispChn == NULL)
                {
                    SAL_ERROR("error\n");
                    return SAL_FAIL;
                }

                /* 相同显示器上有同源 (指通道号) */
                if (pDispChn->stInModule.uiHandle == uiHandle && pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
                {
                    if (j == uiVoChn)
                    {
                        SAL_WARN("voDev %d voChn %d is the same input module !\n", uiVoDevChn, j);
                    }
                    else
                    {
                        pDispChn->stInModule.uiUseFlag = SAL_FALSE;
                        pDispChn->stInModule.uiModId = 0x00;
                        pDispChn->stInModule.uiHandle = 0x00;
                        pDispChn->stInModule.uiAiFlag = SAL_FALSE;
                        pDispChn->uiNoSignal = SAL_TRUE;
                    }

                    break;
                }
            }

            pDispChn = disp_tskGetVoChn(uiVoDevChn, uiVoChn);
            if (pDispChn == NULL)
            {
                SAL_ERROR("error\n");
                return SAL_FAIL;
            }

            snprintf(szName, NAME_LEN, "DUP_VDEC_%d", uiInDev);
            pstSrcInst = link_drvGetInst(szName);
            if (pstSrcInst)
            {
                snprintf(szOutNodeNm, NAME_LEN, "out_disp%d", uiVoDevChn);
                pHandle = link_drvGetHandleFromNode(pstSrcInst, szOutNodeNm);
                if (!pHandle)
                {
                    DISP_LOGE("get handle from node failed, srcInst %s, outNodeName %s \n", pstSrcInst->szInstName, szOutNodeNm);
                    return SAL_FAIL;
                }
            }
            else
            {
                DISP_LOGE("get inst %s failed. \n", szName);
                return SAL_FAIL;
            }

            snprintf(szName, NAME_LEN, "DISP%d_CHN%d", uiVoDevChn, uiVoChn);
            pstDstInst = link_drvGetInst(szName);
            if (!pstDstInst)
            {
                DISP_LOGE("get inst %s failed. \n", szName);
                return SAL_FAIL;
            }

            snprintf(szInNodeNm, NAME_LEN, "in_0");
            /* 即使已经绑定也要继续往下走，因为智能画框是否开启的uiDraw标志位配置也是走该流程 */
            if (SAL_SOK != link_drvCheckCurBind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm))
            {
                /* 先解绑 */
                link_drvUnbind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm);
                /* 进行绑定 */
                link_drvBind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm);
            }

            if (pDispChn->uiW > 0 && pDispChn->uiH > 0)
            {
                DISP_LOGI("uiVoDevChn %d wxh %d x %d!\n", uiVoDevChn, pDispChn->uiW, pDispChn->uiH);
                pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_VDEC;
                pDispChn->stInModule.uiChn = uiInDev;
                memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                stParamInfo.enType = IMAGE_SIZE_CFG;
                stParamInfo.stImgSize.u32Width = pDispChn->uiW;
                stParamInfo.stImgSize.u32Height = pDispChn->uiH;
                s32Ret = disp_tskSetVoChnDupPrm(uiVoDevChn, pDispChn, &stParamInfo);
                DISP_TASK_CHECK_RET(s32Ret, SAL_SOK, "set dup chan prm failed");
            }
            else
            {
                SAL_WARN("vpssGrp %d uiVoDevChn %d wxh %d x %d!\n", vpssGrp, uiVoDevChn, pDispChn->uiW, pDispChn->uiH);
            }

            if (NODE_BIND_TYPE_SDK_BIND == pHandle->createFlags)
            {
                uiGrp = pDispChn->uiDevNo;
                uiChn = pDispChn->uiChn;
                pDispChn->stInModule.frameNum = 0;
                SAL_WARN("VPSS BIND TO VO (%d,%d),(%d,%d)\n", pHandle->u32Grp, pHandle->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);

                stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
                stDupBindPrm.modChn = uiGrp;
                stDupBindPrm.chn = uiChn;
                if (pHandle->dupOps.OpDupBindBlit != NULL && pHandle->dupOps.OpDupStartBlit != NULL)
                {
                    s32Ret = pHandle->dupOps.OpDupBindBlit((Ptr)pHandle, (Ptr) & stDupBindPrm, SAL_TRUE);
                    if (SAL_isFail(s32Ret))
                    {
                        DISP_LOGE("Bind Hal Chn err !!!\n");
                        return SAL_FAIL;
                    }
                }
            }

            if (pHandle->dupOps.OpDupStartBlit != NULL)
            {
                s32Ret = pHandle->dupOps.OpDupStartBlit((Ptr)pHandle);
            }

            s32Ret = disp_hal_startChn(pDispChn);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                /* return SAL_FAIL; */
            }

            pDispChn->stInModule.uiHandle = uiInDev;
            pDispChn->stInModule.uiAiFlag = uiDraw;
            pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_VDEC;
            pDispChn->stInModule.uiUseFlag = SAL_TRUE;
            pDispChn->stInModule.uiChn = uiInDev;

            /*
               备注：
                   此处根据显示窗口大小动态修改解码输出vpss宽高的修改,在R8项目上验证时候存在问题: 预览界面双击ipc,进入全屏的时候
               预览会黑屏;
             */
            memset(&stDecPrm, 0, sizeof(DISP_WINDOW_PRM));
            stDecPrm.bind = SAL_TRUE;
            stDecPrm.enable[uiVoDevChn] = SAL_TRUE;
            stDecPrm.rect[uiVoDevChn].uiWidth = pDispChn->uiW;
            stDecPrm.rect[uiVoDevChn].uiHeight = pDispChn->uiH;
            vdec_tsk_SetDispPrm(uiInDev, &stDecPrm);

        }
    }

    /* 把已经建立绑定关系的打印出来 */
    for (j = 0; j < uiVoCnt; j++)
    {
        pDispChn = disp_tskGetVoChn(uiVoDevChn, j);
        if (pDispChn == NULL)
        {
            SAL_ERROR("error\n");
            return SAL_FAIL;
        }

        if (pDispChn->stInModule.uiUseFlag == SAL_TRUE)
        {
            if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
            {
                DISP_LOGI("voDev %d voChn %d Input Module < Vdec chn %lld Alg %d >\n", uiVoDevChn, j, pDispChn->stInModule.uiHandle, pDispChn->stInModule.uiAiFlag);
            }
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
            {
                pHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
                if (pHandle == NULL)
                {
                    SAL_WARN("warn\n");
                    continue;
                }

                DISP_LOGI("voDev %d voChn %d Input Module < Vi chn %d Alg %d >\n", uiVoDevChn, j, sys_task_getModChn(pHandle->dupModule), pDispChn->stInModule.uiAiFlag);
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskSetVideoRotate
 * @brief      设置旋转参数
 * @param[in]  UINT32 uiDevChn
 * @param[in]  VOID *prm    旋转属性参数
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetVideoRotate(UINT32 uiDevChn, VOID *prm)
{
    UINT32 ret = SAL_FAIL;
    PARAM_INFO_S stParamInfo;
    DISP_CHN_COMMON *pDispChn = NULL;
    VIDEO_ROTATE_ATTR_ST *pRotate = NULL;

    pRotate = (VIDEO_ROTATE_ATTR_ST *)prm;
    if (pRotate == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(uiDevChn, pRotate->uiChan);
    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pDispChn->stChnPrm.uiRotate = pRotate->eRotate;

    DISP_LOGI("uiVoDevChn %d eRotate = %d!\n", uiDevChn, pRotate->eRotate);
    memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
    stParamInfo.enType = ROTATE_CFG;
    stParamInfo.u32Rotate = pRotate->eRotate;
    ret = disp_tskSetVoChnDupPrm(uiDevChn, pDispChn, &stParamInfo);
    if (SAL_SOK != ret)
    {
        DISP_LOGE("error !!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskSetVideoMirror
 * @brief      设置显示镜像功能，vo前一级在vpss做镜像
 * @param[in]  UINT32 uiDevChn
 * @param[in]  VOID *prm
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetVideoMirror(UINT32 uiDevChn, VOID *prm)
{

    UINT32 ret = SAL_FAIL;
    DISP_CHN_COMMON *pDispChn = NULL;
    VIDEO_MIRROR_ATTR_ST *pVideoMirror = NULL;
    PARAM_INFO_S stParamInfo;

    pVideoMirror = (VIDEO_MIRROR_ATTR_ST *)prm;
    if (pVideoMirror == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;;
    }

    pDispChn = disp_tskGetVoChn(uiDevChn, pVideoMirror->uiChan);
    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;;
    }

    if (pVideoMirror->eMirror == MIRROR_MODE_HORIZONTAL)
    {
        pDispChn->stChnPrm.uiMirror = SAL_TRUE;
        pDispChn->stChnPrm.uiFlip = SAL_FALSE;
    }
    else if (pVideoMirror->eMirror == MIRROR_MODE_VERTICAL)
    {
        pDispChn->stChnPrm.uiMirror = SAL_FALSE;
        pDispChn->stChnPrm.uiFlip = SAL_TRUE;
    }
    else if (pVideoMirror->eMirror == MIRROR_MODE_CENTER)
    {
        pDispChn->stChnPrm.uiMirror = SAL_TRUE;
        pDispChn->stChnPrm.uiFlip = SAL_TRUE;
    }
    else
    {
        pDispChn->stChnPrm.uiMirror = SAL_FALSE;
        pDispChn->stChnPrm.uiFlip = SAL_FALSE;
    }

    DISP_LOGI("uiVoDevChn %d uiMirror = %d uiFlip = %d!\n", uiDevChn, pDispChn->stChnPrm.uiMirror, pDispChn->stChnPrm.uiFlip);
    memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
    stParamInfo.enType = MIRROR_CFG;
    stParamInfo.stMirror.u32Flip = pDispChn->stChnPrm.uiFlip;
    stParamInfo.stMirror.u32Mirror = pDispChn->stChnPrm.uiMirror;
    ret = disp_tskSetVoChnDupPrm(uiDevChn, pDispChn, &stParamInfo);
    if (SAL_SOK != ret)
    {
        DISP_LOGE("error !!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_drvSetPosCheck
* 描  述  : 设置小窗口
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_tskSetPosCheck(UINT32 uiDev, DISP_POS_CTRL *pstDispCtrl)
{
    INT32 x = 0, y = 0, w = 0, h = 0;
    INT32 VoCnt = 0;
    CAPB_DISP *capb_disp = NULL;
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

    VoCnt = disp_tskGetVoChnCnt(pstDispCtrl->voDev);

    pstDispCtrl->stDispRegion.x = SAL_align(pstDispCtrl->stDispRegion.x, 2);
    pstDispCtrl->stDispRegion.y = SAL_align(pstDispCtrl->stDispRegion.y, 2);
    pstDispCtrl->stDispRegion.w = SAL_align(pstDispCtrl->stDispRegion.w, 2);
    pstDispCtrl->stDispRegion.h = SAL_align(pstDispCtrl->stDispRegion.h, 2);

    x = pstDispCtrl->stDispRegion.x;
    y = pstDispCtrl->stDispRegion.y;
    w = pstDispCtrl->stDispRegion.w;
    h = pstDispCtrl->stDispRegion.h;

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    if (pstDispCtrl->voDev > capb_disp->disp_vodev_cnt || pstDispCtrl->voChn > VoCnt)
    {
        DISP_LOGE("voDev %d VoCnt %d  error\n", pstDispCtrl->voDev, pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    if (x < 0 || y < 0 || w < 0 || h < 0)
    {
        DISP_LOGE("x %d y %d w %d h %d error\n", x, y, w, h);
        return SAL_FAIL;
    }

    if (x + w > pstDspInfo->stVoInitInfoSt.stVoInfoPrm[uiDev].stDispDevAttr.videoOutputWidth || y + h > pstDspInfo->stVoInitInfoSt.stVoInfoPrm[uiDev].stDispDevAttr.videoOutputHeight)
    {
        DISP_LOGE(" chan %d x %d y %d w %d h %d wmax %d hmax %derror\n", uiDev, x, y, w, h, pstDspInfo->stVoInitInfoSt.stVoInfoPrm[uiDev].stDispDevAttr.videoOutputWidth, pstDspInfo->stVoInitInfoSt.stVoInfoPrm[uiDev].stDispDevAttr.videoOutputHeight);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_drvSetPos
* 描  述  : 设置IPC解码小窗口
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_tskSetPos(UINT32 uiDev, VOID *prm)
{
    INT32 s32Ret = 0, exist = 0;
    DISP_POS_CTRL *pstDispCtrl = NULL;
    INT32 i = 0;

#ifdef DISP_VIDEO_FUSION
    PARAM_INFO_S stParamInfo;
    DISP_WINDOW_PRM stDecPrm = {0};

#endif

    DISP_CHN_COMMON *pDispChn = NULL;

    pstDispCtrl = (DISP_POS_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    s32Ret = disp_tskSetPosCheck(uiDev, pstDispCtrl);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE(" Check error \n");
        return SAL_FAIL;
    }

    INT32 VoCnt = disp_tskGetVoChnCnt(pstDispCtrl->voDev);

    for (i = 0; i < VoCnt; i++)
    {
        pDispChn = disp_tskGetVoChn(pstDispCtrl->voDev, i);
        if (pDispChn == NULL)
        {
            DISP_LOGE("Disp_drvGetDevChn error i %d is null\n", i);
            return SAL_FAIL;
        }

        if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
        {
            DISP_LOGD("chn %d vodev %d chn %d %d \n", pDispChn->uiChn, pDispChn->uiDevNo, i, pstDispCtrl->voChn);
            if (i != pstDispCtrl->voChn)
            {
                pDispChn->u32Priority = 1;
                s32Ret = Disp_halSetVoChnPriority((void *)pDispChn);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("error !!\n");
                    return SAL_FAIL;
                }
            }
            else
            {
                exist = 1;
            }
        }
        else
        {
            continue;
        }
    }

    if (exist != 1)
    {
        DISP_LOGE("error no the vodev :chn %d vodev %d vocnt %d\n", pstDispCtrl->voChn, pDispChn->uiDevNo, VoCnt);
        return SAL_FAIL;
    }

    pstDispCtrl = (DISP_POS_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(pstDispCtrl->voDev, pstDispCtrl->voChn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("Disp_drvGetDevChn error i %d is null\n", pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    pstDispCtrl->stDispRegion.x = SAL_align(pstDispCtrl->stDispRegion.x, 2);
    pstDispCtrl->stDispRegion.y = SAL_align(pstDispCtrl->stDispRegion.y, 2);
    pstDispCtrl->stDispRegion.w = SAL_align(pstDispCtrl->stDispRegion.w, 16);
    pstDispCtrl->stDispRegion.h = SAL_align(pstDispCtrl->stDispRegion.h, 2);

#ifdef DISP_VIDEO_FUSION

    if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
    {
            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = IMAGE_SIZE_CFG;
            stParamInfo.stImgSize.u32Width = pstDispCtrl->stDispRegion.w;
            stParamInfo.stImgSize.u32Height = pstDispCtrl->stDispRegion.h;
            s32Ret = disp_tskSetVoChnDupPrm(uiDev, pDispChn, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                  // 当设置失败时，重新配置Dec窗口的大小，等待解码就绪时生效
                stDecPrm.bind = SAL_TRUE;
                stDecPrm.enable[pstDispCtrl->voDev] = SAL_TRUE;
                stDecPrm.rect[pstDispCtrl->voDev].uiWidth = pstDispCtrl->stDispRegion.w;
                stDecPrm.rect[pstDispCtrl->voDev].uiHeight = pstDispCtrl->stDispRegion.h;
                vdec_tsk_SetDispPrm(pstDispCtrl->voDev, &stDecPrm);
                DISP_LOGD("error !!\n");
                /* return SAL_FAIL;     / *设置失败不影响其他窗口拖拽* / */
            }

    }

#endif

    pDispChn->uiX = pstDispCtrl->stDispRegion.x;
    pDispChn->uiY = pstDispCtrl->stDispRegion.y;
    pDispChn->uiW = pstDispCtrl->stDispRegion.w;
    pDispChn->uiH = pstDispCtrl->stDispRegion.h;
    pDispChn->uiColor = pstDispCtrl->stDispRegion.color;
    pDispChn->uiChn = pstDispCtrl->voChn;
    pDispChn->uiLayerNo = pstDispCtrl->voDev;
    pDispChn->u32Priority = 2;
    pDispChn->bDispBorder = pstDispCtrl->stDispRegion.bDispBorder;
    pDispChn->BorDerColor = pstDispCtrl->stDispRegion.BorDerColor;
    pDispChn->BorDerLineW = pstDispCtrl->stDispRegion.BorDerLineW;

#ifndef DISP_VIDEO_FUSION
    s32Ret = disp_hal_setChnPos(pDispChn);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("error !!\n");
        return SAL_FAIL;
    }

#endif

    return SAL_SOK;
}

/**
 * @function   disp_tskSetVideoRegion
 * @brief      设置通道显示区域
 * @param[in]  UINT32 uiDevChn
 * @param[in]  VOID *prm    显示区域参数
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetVideoRegion(UINT32 uiDevChn, VOID *prm)
{
    int i = 0;
    UINT32 ret = SAL_FAIL;
    UINT32 dispCropChan = 0;
    DISP_CHN_COMMON *pDispChn = NULL;
    CROP_PARAM *pCropParam = NULL;
    PARAM_INFO_S stParamInfo;

    pCropParam = (CROP_PARAM *)prm;

    if (NULL == pCropParam)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    for (i = 0; i < pCropParam->uiCnt; i++)
    {
        dispCropChan = pCropParam->stCropRegion[i].channel; /*0:wl  1:ir*/
        pDispChn = disp_tskGetVoChn(uiDevChn, dispCropChan);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;;
        }

        pDispChn->stChnPrm.uiCrop = pCropParam->stCropRegion[i].uiEnable;
        pDispChn->stChnPrm.uiCropX = pCropParam->stCropRegion[i].uiX;
        pDispChn->stChnPrm.uiCropY = pCropParam->stCropRegion[i].uiY;
        pDispChn->stChnPrm.uiCropW = pCropParam->stCropRegion[i].uiW;
        pDispChn->stChnPrm.uiCropH = pCropParam->stCropRegion[i].uiH;

        memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
        stParamInfo.enType = CHN_CROP_CFG;
        stParamInfo.stCrop.u32CropEnable = pDispChn->stChnPrm.uiCrop;
        stParamInfo.stCrop.u32H = pCropParam->stCropRegion[i].uiH;
        stParamInfo.stCrop.u32W = pCropParam->stCropRegion[i].uiW;
        stParamInfo.stCrop.u32X = pCropParam->stCropRegion[i].uiX;
        stParamInfo.stCrop.u32Y = pCropParam->stCropRegion[i].uiY;
        ret = disp_tskSetVoChnDupPrm(uiDevChn, pDispChn, &stParamInfo);
        if (SAL_SOK != ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskGetCurEffectRegCoordinate
 * @brief      获取当前显示包裹的有效区域
 * @param[in]  UINT32 srcChn
 * @param[in]  void *prm
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskGetCurEffectRegCoordinate(UINT32 srcChn, void *prm)
{
    RECT rect = {0};
    DISP_EFF_REG *pstDispEffectRegion = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    DSPINITPARA *pstDspInitPrm = NULL;

    INT32 voDev = 0;
    INT32 vochn = 0;
    INT32 s32DispChnWidth = 0;
    INT32 s32DispChnHeight = 0;
    INT32 s32SrcDataW = 0;
    INT32 s32SrcDataH = 0;

    if (prm == NULL)
    {
        DISP_LOGE("DISP_EFF_REG * input prm null err\n");
        return SAL_FAIL;
    }

    pstDspInitPrm = SystemPrm_getDspInitPara();
    pstDispEffectRegion = (DISP_EFF_REG *)prm;
    voDev = pstDispEffectRegion->vodev;
    vochn = pstDispEffectRegion->vochn;
    pDispChn = disp_tskGetVoChn(voDev, vochn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("voDev %d chn %d pDispChn null err\n", voDev, vochn);
        return SAL_FAIL;
    }

    if (pDispChn->stInModule.uiModId != SYSTEM_MOD_ID_DUP)
    {
        DISP_LOGE("vodev %d chn %d mode is %derr\n", voDev, vochn, pDispChn->stInModule.uiModId);
        return SAL_FAIL;
    }

    s32DispChnWidth = pDispChn->uiW;
    s32DispChnHeight = pDispChn->uiH;

    rect.uiX = (pstCapbDisp->disp_yuv_w_max - pstDispEffectRegion->height) / 2;
    rect.uiY = pstCapbDisp->disp_h_top_padding;
    rect.uiHeight = pstDispEffectRegion->width;
    rect.uiWidth = pstDispEffectRegion->height;

    /* TODO：上下分屏显示时TIP抠图需要验证，左右分屏显示时TIP抠图需要考虑 */
    if (pstDspInitPrm->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN)
    {
        s32SrcDataW = pstCapbDisp->disp_yuv_w_max;
        s32SrcDataH = pstCapbDisp->disp_h_middle_indeed;
    }
    else
    {
        s32SrcDataW = pstCapbDisp->disp_yuv_w_max;
        s32SrcDataH = pstCapbDisp->disp_yuv_h;
    }

    /* 转换成基于显示窗口坐标 */
    pstDispEffectRegion->rect.uiX = rect.uiX * s32DispChnWidth / s32SrcDataW;
    pstDispEffectRegion->rect.uiY = rect.uiY * s32DispChnHeight / s32SrcDataH;
    pstDispEffectRegion->rect.uiWidth = rect.uiWidth * s32DispChnWidth / s32SrcDataW;
    pstDispEffectRegion->rect.uiHeight = rect.uiHeight * s32DispChnHeight / s32SrcDataH;
    pstDispEffectRegion->fZoomRatioH = (float)s32SrcDataW / s32DispChnWidth;
    pstDispEffectRegion->fZoomRatioV = (float)s32SrcDataH / s32DispChnHeight;

    DISP_LOGI("\n[IN]  voDev:%d, vochn:%d, raw_w:%d raw_h:%d, dispChnW:%d, dispChnH:%d, dispDataW:%d, dispDataH:%d\n"
              "[TMP] rect   x:%d y:%d, w:%d, h:%d\n"
              "[OUT] region x:%d y:%d, w:%d, h:%d, ZoomRatioH:%f, ZoomRatioV:%f\n",
              voDev, vochn, pstDispEffectRegion->width, pstDispEffectRegion->height,
              s32DispChnWidth, s32DispChnHeight, pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h,
              rect.uiX, rect.uiY, rect.uiWidth, rect.uiHeight,
              pstDispEffectRegion->rect.uiX, pstDispEffectRegion->rect.uiY, pstDispEffectRegion->rect.uiWidth,
              pstDispEffectRegion->rect.uiHeight, pstDispEffectRegion->fZoomRatioH, pstDispEffectRegion->fZoomRatioV);

    return SAL_SOK;
}

/**
 * @function   disp_tskSetPipPrm
 * @brief      设置放大镜窗口
 * @param[in]  UINT32 uiDev
 * @param[in]  VOID *prm    显示窗口属性
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetPipPrm(UINT32 uiDev, VOID *prm)
{
    /* UINT32  i = 0; */
    INT32 s32Ret = 0;
    DISP_SVA_SWITCH stDispOsd = {0};
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CHN_COMMON *pDispChnmain = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_POS_CTRL *pstDispCtrl = NULL;
    DISP_MAGNIFIER_COMMON *pMagnifer = NULL;
    CAPB_DISP *capb_disp = NULL;
    DUP_ChanHandle *pHandle = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};
    PARAM_INFO_S stParamInfo;

    /* DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara(); */

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pstDispCtrl = (DISP_POS_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(pstDispCtrl->voDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error, voDev %d\n", pstDispCtrl->voDev);
        return SAL_FAIL;
    }

    /* 获取要显示的通道个数 */
    if (pDispDev->szLayer.uiChnCnt == 0x00 || pDispDev->szLayer.uiChnCnt > disp_tskGetVoChnCnt(pstDispCtrl->voDev))
    {
        DISP_LOGE("error uiChnCnt %d \n", pDispDev->szLayer.uiChnCnt);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(pstDispCtrl->voDev, pstDispCtrl->voChn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("error, voDev %d, voChn %d !\n", pstDispCtrl->voDev, pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
    {
        DISP_LOGE("voDev:%d voChn:%d uiModId is SYSTEM_MOD_ID_VDEC! not support set pip!\n", pstDispCtrl->voDev, pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    pDispChn->useMode = 0;
    if (0 == pDispChn->uiEnable && pstDispCtrl->enable)
    {
        pMagnifer = &gMagnifer[uiDev];
        pMagnifer->viChn = uiDev;
        pMagnifer->voDev = pstDispCtrl->voDev;
        pMagnifer->voChn = 0;
#if 0
        SAL_mutexLock(pMagnifer->mChnMutexHdl);
        if (pstDispCtrl->enable != pMagnifer->enable)
        {
            pMagnifer->enable = pstDispCtrl->enable;
            if (pMagnifer->enable == SAL_TRUE)
            {
                SAL_mutexSignal(pMagnifer->mChnMutexHdl);
            }
        }

        SAL_mutexUnlock(pMagnifer->mChnMutexHdl);
#endif
        DISP_LOGW("pDispChn %p !enable\n", pDispChn);
        pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_DUP;
        pDispChn->stInModule.uiChn = uiDev;
        pDispChn->stInModule.uiUseFlag = SAL_TRUE;
        pstDispCtrl->stDispRegion.x = SAL_align(pstDispCtrl->stDispRegion.x, 2);
        pstDispCtrl->stDispRegion.y = SAL_align(pstDispCtrl->stDispRegion.y, 2);
        pstDispCtrl->stDispRegion.w = SAL_align(pstDispCtrl->stDispRegion.w, 16); /* */
        pstDispCtrl->stDispRegion.h = SAL_align(pstDispCtrl->stDispRegion.h, 2);

        pDispChn->uiX = pstDispCtrl->stDispRegion.x;
        pDispChn->uiY = pstDispCtrl->stDispRegion.y;
        pDispChn->uiW = pstDispCtrl->stDispRegion.w;
        pDispChn->uiH = pstDispCtrl->stDispRegion.h;
        pDispChn->uiColor = pstDispCtrl->stDispRegion.color;
        pDispChn->uiChn = pstDispCtrl->voChn;
        pDispChn->uiLayerNo = pstDispCtrl->voDev;
        pDispChn->bDispBorder = pstDispCtrl->stDispRegion.bDispBorder;
        pDispChn->BorDerColor = pstDispCtrl->stDispRegion.BorDerColor;
        pDispChn->BorDerLineW = pstDispCtrl->stDispRegion.BorDerLineW;
        pDispChn->u32Priority = pstDispCtrl->voChn;

#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
        s32Ret = disp_hal_enableChn(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

        s32Ret = Disp_halSetVoChnPriority((void *)pDispChn);
#endif
        pHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
        if (NULL == pHandle)
        {
            DISP_LOGE("DUP_ChanHandle is NULL  !!!\n");
            return SAL_FAIL;
        }

        memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
        stParamInfo.enType = IMAGE_SIZE_CFG;

        stParamInfo.stImgSize.u32Width = pDispChn->uiW;
        stParamInfo.stImgSize.u32Height = pDispChn->uiH;
        s32Ret = pHandle->dupOps.OpDupSetBlitPrm((Ptr)pHandle, &stParamInfo);
        if (SAL_isFail(s32Ret))
        {
            DISP_LOGE("OpDupSetBlitPrm Chn err !!!\n");
            return SAL_FAIL;
        }

        /* TODO: 不同设备间的兼容，绑定或者取帧模式 */
        if (NODE_BIND_TYPE_SDK_BIND == pHandle->createFlags)
        {
            stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
            stDupBindPrm.modChn = pDispChn->uiDevNo;
            stDupBindPrm.chn = pDispChn->uiChn;
            pDispChn->stInModule.frameNum = 0;
            SAL_WARN("VPSS BIND TO VO (%d,%d),(%d,%d)\n", pHandle->u32Grp, pHandle->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);
            s32Ret = pHandle->dupOps.OpDupBindBlit((Ptr)pHandle, (Ptr) & stDupBindPrm, SAL_TRUE);
            if (SAL_isFail(s32Ret))
            {
                DISP_LOGE("Bind Hal Chn err !!!\n");
                return SAL_FAIL;
            }

            s32Ret = pHandle->dupOps.OpDupStartBlit((Ptr)pHandle);

            s32Ret = disp_hal_startChn(pDispChn); /* 开始vo显示 */
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                return SAL_FAIL;
            }

            /* 绑定模式只需要更新状态，不需要唤醒显示线程 */
            disp_tskSetVoChnStatus(pDispChn, SAL_FALSE, SAL_TRUE);

        }
        else
        {
            s32Ret = pHandle->dupOps.OpDupStartBlit((Ptr)pHandle);
#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
            s32Ret = disp_hal_startChn(pDispChn);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                /* return SAL_FAIL; */
            }

#endif

#ifdef  DISP_VIDEO_FUSION
            disp_cleardupCopyDispPool(pDispChn); /* 显示缓存池要清掉，拼帧线程从该显示缓存池那帧，如果重新开启该通道先不清一下，会导致把上一次开启状态时的帧给拼上 */
#endif
            /* 送帧模式，除了更新状态，还需要唤醒显示线程 */
            disp_tskVoChnCtrl(pDispChn, SAL_TRUE);

        }

        return SAL_SOK;
    }
    else if (0 == pstDispCtrl->enable)
    {
        pHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
        if (pHandle == NULL)
        {
            DISP_LOGE("pHandle is %p err !!!\n", pHandle);
            return SAL_FAIL;
        }

        if (pHandle->dupOps.OpDupStopBlit != NULL)
        {
            s32Ret = pHandle->dupOps.OpDupStopBlit((Ptr)pHandle);
            if (SAL_isFail(s32Ret))
            {
                DISP_LOGE("OpDupStopBlit err !!!\n");
                return SAL_FAIL;
            }
        }

        stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
        stDupBindPrm.modChn = pDispChn->uiDevNo;
        stDupBindPrm.chn = pDispChn->uiChn;
        pDispChn->stInModule.frameNum = 0;
        pDispChn->stInModule.uiHandle = 0;    /* 关窗口清handle */

        /* TODO: 不同设备间的兼容，绑定或者取帧模式 */
        if (NODE_BIND_TYPE_SDK_BIND == pHandle->createFlags)
        {
            SAL_WARN("VPSS unBIND TO VO (%d,%d),(%d,%d)\n", pHandle->u32Grp, pHandle->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);
            s32Ret = pHandle->dupOps.OpDupBindBlit((Ptr)pHandle, (Ptr) & stDupBindPrm, SAL_FALSE);
            if (SAL_isFail(s32Ret))
            {
                DISP_LOGE("unBind Hal Chn err !!!\n");
                return SAL_FAIL;
            }
        }

        if (capb_disp->disp_videv_cnt == 1 && capb_disp->disp_vodev_cnt == 2 && pstDispCtrl->voDev == 1)
        {
            /* Disp_drvVpssUnBindToVo(VPSS_ENLARGE_GRP_CHN_0, 0, pDispChn->uiLayerNo, pDispChn->uiChn); */
            s32Ret = SAL_SOK;
        }
        else
        {
            s32Ret = disp_tskSetVoChnStatus(pDispChn, SAL_TRUE, SAL_FALSE);
        }

#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
        s32Ret = disp_hal_disableChn(pDispChn); /* 禁止访问vo */
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

#endif
        pDispChn->stInModule.uiModId = 0x00;
        pDispChn->stInModule.uiUseFlag = SAL_FALSE;

        /* 局部放大退出后智能信息框显示 */
        pDispChn->enlargeprm.uiDisappear = 0;
        pDispChnmain = disp_tskGetVoChn(pstDispCtrl->voDev, pDispChn->enlargeprm.globalchn);
        if (pDispChnmain == NULL)
        {
            DISP_LOGE("error !\n");
            return SAL_FAIL;
        }

        pDispChnmain->enlargeprm.partchn = -1;

        if (pDispChnmain->enlargeprm.globalratio <= 1.0 || pDispChnmain->enlargeprm.defaultenlargesign)
        {
            pDispChnmain->enlargeprm.uiDisappear = 0;
        }
        stDispOsd.dispSvaSwitch = SAL_TRUE;
        Xpack_DrvSetAiXrOsdShow(pstDispCtrl->voDev, &stDispOsd, SAL_TRUE);

        DISP_LOGW("pDispChn %p !disable\n", pDispChn);
        return SAL_SOK;
    }

    /* 局部放大窗口可以拖动，坐标需要实时改变 */
    if (pDispDev->szLayer.uiChnCnt > 0)
    {
        pstDispCtrl->stDispRegion.x = SAL_align(pstDispCtrl->stDispRegion.x, 2);
        pstDispCtrl->stDispRegion.y = SAL_align(pstDispCtrl->stDispRegion.y, 2);
        pstDispCtrl->stDispRegion.w = SAL_align(pstDispCtrl->stDispRegion.w, 2);
        pstDispCtrl->stDispRegion.h = SAL_align(pstDispCtrl->stDispRegion.h, 2);

        pDispChn->uiX = pstDispCtrl->stDispRegion.x;
        pDispChn->uiY = pstDispCtrl->stDispRegion.y;
        pDispChn->uiW = pstDispCtrl->stDispRegion.w;
        pDispChn->uiH = pstDispCtrl->stDispRegion.h;
        pDispChn->uiColor = pstDispCtrl->stDispRegion.color;
        pDispChn->uiChn = pstDispCtrl->voChn;
        pDispChn->uiLayerNo = pstDispCtrl->voDev;
        pDispChn->bDispBorder = pstDispCtrl->stDispRegion.bDispBorder;
        pDispChn->BorDerColor = pstDispCtrl->stDispRegion.BorDerColor;
        pDispChn->BorDerLineW = pstDispCtrl->stDispRegion.BorDerLineW;
#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
        s32Ret = disp_hal_setChnPos(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

#endif
    }
    else
    {
        DISP_LOGE("uiChnCnt %d,error !!\n", pDispDev->szLayer.uiChnCnt);
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskSetThumbnailPrm
 * @brief      设置小窗口
 * @param[in]  UINT32 uiDev
 * @param[in]  VOID *prm   显示窗口属性
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetThumbnailPrm(UINT32 uiDev, VOID *prm)
{
    /* UINT32  i = 0; */
    INT32 s32Ret = 0;
    CHAR szName[NAME_LEN] = {0};
    CHAR szOutNodeNm[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_POS_CTRL *pstDispCtrl = NULL;
    DISP_MAGNIFIER_COMMON *pMagnifer = NULL;
    PARAM_INFO_S stParamInfo;
    /* 缩略图使用grp0chn3通道 */
    /* DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara(); */
    DUP_ChanHandle *dupOutChnHandle = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};

    pstDispCtrl = (DISP_POS_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(pstDispCtrl->voDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error, voDev %d\n", pstDispCtrl->voDev);
        return SAL_FAIL;
    }

    /* 获取要显示的通道个数 */
    if (pDispDev->szLayer.uiChnCnt == 0x00 || pDispDev->szLayer.uiChnCnt > disp_tskGetVoChnCnt(pstDispCtrl->voDev))
    {
        DISP_LOGE("error uiChnCnt %d \n", pDispDev->szLayer.uiChnCnt);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(pstDispCtrl->voDev, pstDispCtrl->voChn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("error, voDev %d, voChn %d !\n", pstDispCtrl->voDev, pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    DISP_LOGI("dev: %d, chn: %d, thumbnail mode: %d, enable: %d-%d\n",
              uiDev, pstDispCtrl->voChn, pDispChn->useMode, pDispChn->uiEnable, pstDispCtrl->enable);

    pDispChn->useMode = 1;
    if (0 == pDispChn->uiEnable && pstDispCtrl->enable)
    {
        pMagnifer = &gMagnifer[uiDev];
        pMagnifer->viChn = uiDev;
        pMagnifer->voDev = pstDispCtrl->voDev;
        pMagnifer->voChn = 0;
#if 0
        SAL_mutexLock(pMagnifer->mChnMutexHdl);
        if (pstDispCtrl->enable != pMagnifer->enable)
        {
            pMagnifer->enable = pstDispCtrl->enable;
            if (pMagnifer->enable == SAL_TRUE)
            {
                SAL_mutexSignal(pMagnifer->mChnMutexHdl);
            }
        }

        SAL_mutexUnlock(pMagnifer->mChnMutexHdl);
#endif

        pDispChn->stInModule.uiModId = SYSTEM_MOD_ID_DUP;
        pDispChn->stInModule.uiChn = uiDev;

        pDispChn->uiX = SAL_align(pstDispCtrl->stDispRegion.x, 2);
        pDispChn->uiY = SAL_align(pstDispCtrl->stDispRegion.y, 2);
        pDispChn->uiW = SAL_alignDown(pstDispCtrl->stDispRegion.w, 16);
        pDispChn->uiH = SAL_alignDown(pstDispCtrl->stDispRegion.h, 16);
        pDispChn->uiColor = pstDispCtrl->stDispRegion.color;
        pDispChn->uiChn = pstDispCtrl->voChn;
        pDispChn->uiLayerNo = pstDispCtrl->voDev;
        pDispChn->bDispBorder = pstDispCtrl->stDispRegion.bDispBorder;
        pDispChn->BorDerColor = pstDispCtrl->stDispRegion.BorDerColor;
        pDispChn->BorDerLineW = pstDispCtrl->stDispRegion.BorDerLineW;
        pDispChn->u32Priority = pstDispCtrl->voChn;

        DISP_LOGI("uiVoDevChn %d wxh %d x %d!\n", pstDispCtrl->voDev, pDispChn->uiW, pDispChn->uiH);
        snprintf(szName, NAME_LEN, "DUP_REAR_%d", uiDev);
        pstSrcInst = link_drvGetInst(szName);
        if (!pstSrcInst)
        {
            DISP_LOGE("get inst %s failed. \n", szName);
            return SAL_FAIL;
        }

        snprintf(szOutNodeNm, NAME_LEN, "out_thumbnail");
        dupOutChnHandle = (DUP_ChanHandle *)link_drvGetHandleFromNode(pstSrcInst, szOutNodeNm);
        if (dupOutChnHandle != NULL)
        {
            pDispChn->stInModule.uiHandle = (PhysAddr)dupOutChnHandle;

            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = IMAGE_SIZE_CFG;
            stParamInfo.stImgSize.u32Width = pDispChn->uiW;
            stParamInfo.stImgSize.u32Height = pDispChn->uiH;
            s32Ret = dupOutChnHandle->dupOps.OpDupSetBlitPrm((Ptr)dupOutChnHandle, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }
        }

#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
        s32Ret = disp_hal_enableChn(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

        s32Ret = Disp_halSetVoChnPriority((void *)pDispChn);
#endif

        /* TODO: 不同设备间的兼容，绑定或者取帧模式 */
        if (NODE_BIND_TYPE_SDK_BIND == dupOutChnHandle->createFlags)
        {
            if (dupOutChnHandle != NULL)
            {
                stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
                stDupBindPrm.modChn = pDispChn->uiDevNo;
                stDupBindPrm.chn = pDispChn->uiChn;
                pDispChn->stInModule.frameNum = 0;
                SAL_WARN("VPSS BIND TO VO (%d,%d),(%d,%d)\n", dupOutChnHandle->u32Grp, dupOutChnHandle->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);
                s32Ret = dupOutChnHandle->dupOps.OpDupBindBlit((Ptr)dupOutChnHandle, (Ptr) & stDupBindPrm, SAL_TRUE);
                if (SAL_isFail(s32Ret))
                {
                    DISP_LOGE("Bind Hal Chn err !!!\n");
                    return SAL_FAIL;
                }

                s32Ret = dupOutChnHandle->dupOps.OpDupStartBlit((Ptr)dupOutChnHandle);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("dev:%u chn:%u dupOutChnHandle->dupOps.OpDupStartBlit fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                    return SAL_FAIL;
                }

                s32Ret = disp_hal_startChn(pDispChn);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                    return SAL_FAIL;
                }
            }
            else
            {
                DISP_LOGE("DUP_ChanHandle     is NUll !!!\n");
            }

            /* 绑定模式时只需要更新状态，不需要唤醒显示线程;    */
            /* disp_tskVoChnCtrl(pDispChn, SAL_TRUE); */
            disp_tskSetVoChnStatus(pDispChn, SAL_FALSE, SAL_TRUE);
        }
        else
        {
            if (dupOutChnHandle != NULL)
            {
                s32Ret = dupOutChnHandle->dupOps.OpDupStartBlit((Ptr)dupOutChnHandle);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("dev:%u chn:%u dupOutChnHandle->dupOps.OpDupStartBlit fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                    /* return SAL_FAIL; */
                }

#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
                s32Ret = disp_hal_startChn(pDispChn);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("dev:%u chn:%u start fail\n", pDispChn->uiLayerNo, pDispChn->uiChn);
                    /* return SAL_FAIL; */
                }

#endif
            }

#ifdef  DISP_VIDEO_FUSION
            disp_cleardupCopyDispPool(pDispChn); /* 显示缓存池要清掉，拼帧线程从该显示缓存池那帧，如果重新开启该通道先不清一下，会导致把上一次开启状态时的帧给拼上 */
#endif
            /* 送帧模式，除了更新状态，还需要唤醒显示线程 */
            disp_tskVoChnCtrl(pDispChn, SAL_TRUE);

        }

        /* 确认窗口设置完成以后再开启显示，否者把整个设置变化过程显示出来 */
        DISP_LOGW("pDispChn %p !enable\n", pDispChn);
        pDispChn->stInModule.uiUseFlag = SAL_TRUE;

        return SAL_SOK;
    }
    else if (0 == pstDispCtrl->enable)
    {
        s32Ret = disp_tskSetVoChnStatus(pDispChn, SAL_TRUE, SAL_FALSE);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("disp_tskSetVoChnStatus err !!!\n");
            return SAL_FAIL;
        }

        dupOutChnHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
        if (dupOutChnHandle->dupOps.OpDupStopBlit != NULL)
        {
            s32Ret = dupOutChnHandle->dupOps.OpDupStopBlit((Ptr)dupOutChnHandle);
            if (SAL_isFail(s32Ret))
            {
                DISP_LOGE("OpDupStopBlit err !!!\n");
                return SAL_FAIL;
            }
        }

        stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
        stDupBindPrm.modChn = pDispChn->uiDevNo;
        stDupBindPrm.chn = pDispChn->uiChn;
        pDispChn->stInModule.frameNum = 0;
        pDispChn->stInModule.uiHandle = 0;    /* 关窗口清handle */
        SAL_WARN("VPSS unBIND TO VO (%d,%d),(%d,%d)\n", dupOutChnHandle->u32Grp, dupOutChnHandle->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);
        if ((NODE_BIND_TYPE_SDK_BIND == dupOutChnHandle->createFlags) && (dupOutChnHandle->dupOps.OpDupBindBlit != NULL))
        {
            s32Ret = dupOutChnHandle->dupOps.OpDupBindBlit((Ptr)dupOutChnHandle, (Ptr) & stDupBindPrm, SAL_FALSE);
            if (SAL_isFail(s32Ret))
            {
                DISP_LOGE("unBind Hal Chn err !!!\n");
                return SAL_FAIL;
            }
        }

#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
        s32Ret = disp_hal_disableChn(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

#endif
        pDispChn->stInModule.uiUseFlag = SAL_FALSE;
        DISP_LOGW("pDispChn %p !disable\n", pDispChn);
        return SAL_SOK;
    }

    if (pDispDev->szLayer.uiChnCnt > 0)
    {
        pDispChn->uiX = SAL_align(pstDispCtrl->stDispRegion.x, 2);
        pDispChn->uiY = SAL_align(pstDispCtrl->stDispRegion.y, 2);
        pDispChn->uiW = SAL_alignDown(pstDispCtrl->stDispRegion.w, 16);
        pDispChn->uiH = SAL_alignDown(pstDispCtrl->stDispRegion.h, 16);
        pDispChn->uiColor = pstDispCtrl->stDispRegion.color;
        pDispChn->uiChn = pstDispCtrl->voChn;
        pDispChn->uiLayerNo = pstDispCtrl->voDev;
        pDispChn->bDispBorder = pstDispCtrl->stDispRegion.bDispBorder;
        pDispChn->BorDerColor = pstDispCtrl->stDispRegion.BorDerColor;
        pDispChn->BorDerLineW = pstDispCtrl->stDispRegion.BorDerLineW;
#ifndef DISP_VIDEO_FUSION   /* DISP_VIDEO_FUSION采用拼帧，所以只需要开启vo chn 0 */
        s32Ret = disp_hal_setChnPos(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

#endif
    }
    else
    {
        DISP_LOGE("uiChnCnt %d,error !!\n", pDispDev->szLayer.uiChnCnt);
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskCheckDptzpartPrm
 * @brief      查看局部放大参数的有效性
 * @param[in]  INT32 uiDev
 * @param[in]  DISP_CHN_COMMON *pDispChnsuff  通道信息
 * @param[in]  DISP_CHN_COMMON *pDispChnMain  视频全局参数
 * @param[in]  DISP_DPTZ *pstDispCtrl         设置参数
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskCheckDptzpartPrm(INT32 uiDev, DISP_CHN_COMMON *pDispChnsuff, DISP_CHN_COMMON *pDispChnMain, DISP_DPTZ *pstDispCtrl)
{
    INT32 x = 0;
    INT32 y = 0;
    INT32 w = 0;
    INT32 h = 0;
    float ratio = 1;

    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("pstDispCtrl NULL err\n");
        return SAL_FAIL;
    }

    if (pDispChnMain == NULL)
    {
        DISP_LOGE("chn %d pDispChnMain NULL err\n", pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    if (pDispChnsuff == NULL)
    {
        DISP_LOGE("chn %d pDispChnsuff NULL err\n", pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    if (pstDispCtrl->w < MIN_CROP_WIDTH || pstDispCtrl->h < MIN_CROP_WIDTH || pstDispCtrl->w > pDispChnMain->uiW || pstDispCtrl->h > pDispChnMain->uiH)
    {
        DISP_LOGE("dev %d chn %d crop width,height [%d,%d] main [%d %d]err\n", uiDev, pstDispCtrl->voChn, pstDispCtrl->w, pstDispCtrl->h, pDispChnMain->uiW, pDispChnMain->uiH);
        return SAL_FAIL;
    }

    if (pstDispCtrl->ratio < 1.0)
    {
        pstDispCtrl->ratio = 1.0;
    }

    x = pstDispCtrl->x;
    y = pstDispCtrl->y;
    w = pstDispCtrl->w;
    h = pstDispCtrl->h;
    ratio = pstDispCtrl->ratio;

    if (pstDispCtrl->x > pDispChnMain->uiW || pstDispCtrl->y > pDispChnMain->uiH)
    {
        DISP_LOGE("chn %d x,y not in Region:[%d,%d]\n", pstDispCtrl->voChn, pstDispCtrl->x, pstDispCtrl->y);
        return SAL_FAIL;
    }

    /* 左侧 */
    w = w / ratio > MIN_CROP_WIDTH ? w / ratio : MIN_CROP_WIDTH;
    h = h / ratio > MIN_CROP_WIDTH ? h / ratio : MIN_CROP_WIDTH;
    if (w < MIN_CROP_WIDTH || h < MIN_CROP_WIDTH)
    {
        DISP_LOGE("chn %d crop width,height[%d,%d] err\n", pstDispCtrl->voChn, w, h);
        return SAL_FAIL;
    }

    if (x - w / 2 < 0)
    {
        if (y < h / 2)
        {
            if (x <= w / 2)
            {
                x = w / 2;
            }

            if (y <= h / 2)
            {
                y = h / 2;
            }
        }
        else if (y >= h / 2 && y <= pDispChnMain->uiH - h / 2)
        {
            if (x <= w / 2)
            {
                x = w / 2;
            }
        }
        else if (y > pDispChnMain->uiH - h / 2)
        {
            if (x <= w / 2)
            {
                x = w / 2;
            }

            if (pDispChnMain->uiH - y <= h / 2)
            {
                y = pDispChnMain->uiH - h / 2;
            }
        }
    }

    /* 右侧 */
    if (x + w / 2 > pDispChnMain->uiW)
    {
        if (y < h / 2)
        {
            if (pDispChnMain->uiW - x <= w / 2)
            {
                x = pDispChnMain->uiW - w / 2;
            }

            if (y <= h / 2)
            {
                y = h / 2;
            }
        }
        else if (y >= h / 2 && y <= pDispChnMain->uiH - h / 2)
        {
            if (pDispChnMain->uiW - x <= w / 2)
            {
                x = pDispChnMain->uiW - w / 2;
            }
        }
        else if (y > pDispChnMain->uiH - h / 2)
        {
            if (pDispChnMain->uiW - x <= w / 2)
            {
                x = pDispChnMain->uiW - w / 2;
            }

            if (pDispChnMain->uiH - y <= h / 2)
            {
                y = pDispChnMain->uiH - h / 2;
            }
        }
    }

    /* 上半部 */
    if (y < h / 2 && (x < pDispChnMain->uiW - w / 2) && (x > w / 2))
    {
        if (y <= h / 2)
        {
            y = h / 2;
        }
    }

    /* 下半部 */
    if (y > pDispChnMain->uiH - h / 2 && (x < pDispChnMain->uiW - w / 2) && (x > w / 2))
    {
        if (pDispChnMain->uiH - y <= h / 2)
        {
            y = pDispChnMain->uiH - h / 2;
        }
    }

    pstDispCtrl->x = x;
    pstDispCtrl->y = y;
    /* pstDispCtrl->w = w; */
    /* pstDispCtrl->h = h; */
    return SAL_SOK;
}

/**
 * @function   disp_tskCheckDptzglobalPrm
 * @brief      检查全局放大参数有效性
 * @param[in]  INT32 uiDev
 * @param[in]  DISP_DPTZ *pstDispCtrl  设置参数
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskCheckDptzglobalPrm(INT32 uiDev, DISP_DPTZ *pstDispCtrl)
{
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    float minratio = 1.0;
    int x, y, w, h;
    int pDispChn_x = 0;
    int pDispChn_y = 0;
    int pDispChn_w = 0;
    int pDispChn_h = 0;
    int needreset = 0;

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("uiDev:%d NULL error \n", uiDev);
        return SAL_FAIL;
    }

    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("uiDev:%d pstDispCtrl NULL err\n", uiDev);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(uiDev, pstDispCtrl->voChn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("chn:%d pDispChn NULL error\n", pstDispCtrl->voChn);
        return SAL_FAIL;
    }

    if (pstDispCtrl->ratio < 1.0)
    {
        pstDispCtrl->ratio = 1.0;
    }

    minratio = pstDispCtrl->ratio;
    x = pstDispCtrl->x;
    y = pstDispCtrl->y;
    w = pDispChn->uiW / minratio;
    h = pDispChn->uiH / minratio;
    pDispChn_x = (int)pDispChn->uiX;
    pDispChn_y = (int)pDispChn->uiY;
    pDispChn_w = (int)pDispChn->uiW;
    pDispChn_h = (int)pDispChn->uiH;

    if (x < pDispChn_x || y < pDispChn_y || x > (pDispChn_x + pDispChn_w) || y > (pDispChn_y + pDispChn_h))
    {
        DISP_LOGE("chn %d x , y [%d,%d]not in region[%d %d %d %d]\n", pstDispCtrl->voChn, x, y, pDispChn_x, pDispChn_y, pDispChn_w, pDispChn_h);
        return SAL_FAIL;
    }

    if (w < MIN_CROP_WIDTH || h < MIN_CROP_WIDTH || w > pDispChn_w || h > pDispChn_h)
    {
        DISP_LOGE("chn %d w,h[%d,%d] err\n", pstDispCtrl->voChn, w, h);
        return SAL_FAIL;
    }

    /* 参数异常需要修改 */
    if ((x - w / 2 < pDispChn_x) || (x + w / 2 > pDispChn_x + pDispChn->uiW)
        || (y - h / 2 < pDispChn_y) || (y + h / 2 > pDispChn_y + pDispChn_h))
    {
        needreset = 1;
    }

    if (needreset)
    {
        w = pDispChn_w / pstDispCtrl->ratio;
        h = pDispChn_h / pstDispCtrl->ratio;

        if (w / 2 + pDispChn_x > x)
        {
            x = w / 2 + pDispChn_x;
        }

        if (pDispChn_x + pDispChn_w - w / 2 < x)
        {
            x = pDispChn_x + pDispChn_w - w / 2;
        }

        if (h / 2 + pDispChn_y > y)
        {
            y = h / 2 + pDispChn_y;
        }

        if (pDispChn_y + pDispChn_h - h / 2 < y)
        {
            y = pDispChn_y + pDispChn_h - h / 2;
        }

        minratio = pstDispCtrl->ratio;
    }

    pstDispCtrl->x = (UINT32)x;
    pstDispCtrl->y = (UINT32)y;

    pstDispCtrl->ratio = minratio;

    return SAL_SOK;

}

/**
 * @function   disp_tskGetVoChnDupPrm
 * @brief      获取显示VO通道前一级dup参数
 * @param[in]  UINT32 voDev
 * @param[in]  DISP_CHN_COMMON *pDispChn
 * @param[in]  PARAM_INFO_S *stParamInfo  dup参数
 * @param[out] None
 * @return     static INT32
 */
 #if 0  /*暂时不用*/
static INT32 disp_tskGetVoChnDupPrm(UINT32 voDev, DISP_CHN_COMMON *pDispChn, PARAM_INFO_S *stParamInfo)
{
    INT32 s32Ret = SAL_FAIL;
    DUP_ChanHandle *dupOutChnHandle = NULL;

    if (NULL == pDispChn || NULL == stParamInfo)
    {
        SAL_WARN("pDispChn = NULL !!!\n");
        return SAL_FAIL;
    }

    dupOutChnHandle = disp_tskGetVoDupHandle(voDev, pDispChn);
    if (NULL != dupOutChnHandle)
    {
        s32Ret = dupOutChnHandle->dupOps.OpDupGetBlitPrm((Ptr)dupOutChnHandle, stParamInfo);
        if (SAL_SOK != s32Ret)
        {
            SAL_WARN("OpDupGetBlitPrm failed !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_WARN("dupOutChnHandle is NULL !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#endif

/**
 * @function   disp_tskSetVoChnDptzPrm
 * @brief      显示全局放大和局部放大：通过VPSS做裁剪放大

 * @param[in]  UINT32 uiDev
 * @param[in]  VOID *prm    设置放大参数
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskSetVoChnDptzPrm(UINT32 uiDev, VOID *prm)
{
    INT32 s32Ret = 0;
    INT32 startX = 0;
    INT32 startY = 0;
    INT32 rectW = 0;
    INT32 rectH = 0;
    UINT32 inputwidth = 0;
    UINT32 inputheight = 0;
    /* UINT32 u32ImageHeight = 0; */
    /* UINT32 u32ImageWidth = 0; */
    DISP_SVA_SWITCH stDispOsd = {0};
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CHN_COMMON *pDispChnmain = NULL;
    /* VPSS_CHN_ATTR_S stVpssChnAttr = {0}; */
    DISP_DPTZ *pstDispCtrl = NULL;
    CROP_PARAM cropParam = {0};
    DSPINITPARA *pDspInitPara = NULL;
    DISP_GLOBALENLARGE_PRM *GlobalEnlarge;
    static int cnt[2] = {0}; /* 操作计数，防止一直打印 */
    CAPB_DISP *capb_disp = NULL;
    PARAM_INFO_S stParamInfo;
    CHAR szName[NAME_LEN] = {0};
    CHAR szOutNodeNm[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pDspInitPara = SystemPrm_getDspInitPara();
    pstDispCtrl = (DISP_DPTZ *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("pstDispCtrl NULL error \n");
        return SAL_FAIL;
    }

    inputwidth = capb_disp->disp_yuv_w_max;
    /* inputheight = capb_disp->disp_yuv_h; */
    if (pDspInitPara->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN)
    {
        inputheight = capb_disp->disp_h_middle_indeed;
    }
    else
    {
        /* 这里需要考虑到超分功能 */
        inputheight = capb_disp->disp_h_middle_indeed + capb_disp->disp_h_top_padding + capb_disp->disp_h_bottom_padding;
    }

    pDispDev = disp_tskGetVoDev(pstDispCtrl->voDev); /* 获取显示设备信息 */
    if (pDispDev == NULL)
    {
        DISP_LOGE("uiDev %d pDispDev NULL error \n", pstDispCtrl->voDev);
        return SAL_FAIL;
    }

    /* 获取要显示的通道个数 */
    if (pDispDev->szLayer.uiChnCnt == 0x00 || pDispDev->szLayer.uiChnCnt > disp_tskGetVoChnCnt(pstDispCtrl->voDev))
    {
        DISP_LOGE("error uiChnCnt %d \n", pDispDev->szLayer.uiChnCnt);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(pstDispCtrl->voDev, pstDispCtrl->voChn); /* 获取显示通道信息 */
    if (pDispChn == NULL)
    {
        DISP_LOGE("uiDev %d pDispChn NULL error !\n", pstDispCtrl->voDev);
        return SAL_FAIL;
    }

    if (pDispChn->stInModule.uiHandle == 0 && !pstDispCtrl->mode)
    {
        snprintf(szName, NAME_LEN, "DUP_FRONT_%d", uiDev);
        pstSrcInst = link_drvGetInst(szName);
        if (!pstSrcInst)
        {
            DISP_LOGE("get inst %s failed. \n", szName);
            return SAL_FAIL;
        }

        snprintf(szOutNodeNm, NAME_LEN, "out_dupRear");
        DISP_LOGW("disp_tskSetVoChnDptzPrm set bind input %d!\n", uiDev);
        pDispChn->stInModule.uiHandle = (PhysAddr)link_drvGetHandleFromNode(pstSrcInst, szOutNodeNm);
    }

    DISP_LOGD("uiDev %d x %d y %d w %d h %d %f %d\n", uiDev, pstDispCtrl->x, pstDispCtrl->y, pstDispCtrl->w, pstDispCtrl->h, pstDispCtrl->ratio, pstDispCtrl->voChn);

    /* 全局放大 */
    if (pstDispCtrl->mode)
    {
        if (SAL_SOK != disp_tskCheckDptzglobalPrm(pstDispCtrl->voDev, pstDispCtrl))
        {
            DISP_LOGE("disp_tskCheckDptzglobalPrm err!\n");
            return SAL_FAIL;
        }

        if (pstDispCtrl->voChn != 0 && pstDispCtrl->voChn != 1)   /* 临时添加 以防应用全局放大的vo通道发错 */
        {
            DISP_LOGE("globla vodev %d chn %d err!\n", pstDispCtrl->voDev, pstDispCtrl->voChn);
            return SAL_FAIL;
        }

        /* 将配置好的中心点坐标以及缩放比率返回给app */
        if (pstDispCtrl->voDev < MAX_XRAY_CHAN && pstDispCtrl->voChn < MAX_DISP_CHAN)
        {
            GlobalEnlarge = &(pDspInitPara->GlobalEnlarge[pstDispCtrl->voDev][pstDispCtrl->voChn]);
            GlobalEnlarge->x = pstDispCtrl->x;
            GlobalEnlarge->y = pstDispCtrl->y;
            GlobalEnlarge->ratio = pstDispCtrl->ratio;
            /* DISP_LOGI("GlobalEnlarge x:%d,y:%d,ratio:%f\n",GlobalEnlarge->x,GlobalEnlarge->y,GlobalEnlarge->ratio); */
        }

        if (pstDispCtrl->ratio > 1.0 && pDispChn->uiW > MIN_CROP_WIDTH && pDispChn->uiH > MIN_CROP_WIDTH)
        {
            /* cropParam中参数保存截取区域左上角x,y坐标 w,h裁剪宽高 */
            /* 宽高根据缩放比例进行缩放 在vpssgrp中裁剪 宽高是原始数据大小1216*640 */
            /* 应用传下来的是中心点坐标，坐标是相对屏幕左上角坐标，局部放大也是，所以裁剪左上角坐标需要做偏移 */
            cropParam.uiCnt = 1;
            cropParam.stCropRegion[0].uiEnable = 1;
            cropParam.stCropRegion[0].channel = pstDispCtrl->voChn;
            cropParam.stCropRegion[0].uiW = (UINT32)((float)inputwidth / pstDispCtrl->ratio);
            cropParam.stCropRegion[0].uiH = (UINT32)((float)inputheight / pstDispCtrl->ratio);
            cropParam.stCropRegion[0].uiX = SAL_SUB_SAFE(pstDispCtrl->x, pDispChn->uiX) * inputwidth / pDispChn->uiW;
            /* DISP_LOGW("pstDispCtrl->x %d, pDispChn->uiX %d, inputwidth %d, pDispChn->uiW %d", pstDispCtrl->x, pDispChn->uiX, inputwidth, pDispChn->uiW); */
            cropParam.stCropRegion[0].uiX = SAL_SUB_SAFE(cropParam.stCropRegion[0].uiX, cropParam.stCropRegion[0].uiW / 2);
            cropParam.stCropRegion[0].uiY = SAL_SUB_SAFE(pstDispCtrl->y, pDispChn->uiY) * inputheight / pDispChn->uiH;
            /* DISP_LOGW("pstDispCtrl->y %d, pDispChn->uiY %d, inputheight %d, pDispChn->uiH %d", pstDispCtrl->y, pDispChn->uiY, inputheight, pDispChn->uiH); */
            cropParam.stCropRegion[0].uiY = SAL_SUB_SAFE(cropParam.stCropRegion[0].uiY, cropParam.stCropRegion[0].uiH / 2);
            /* 将裁减宽高保存在局部放大使用 */
            pDispChn->enlargeprm.uiCropX = SAL_alignDown(cropParam.stCropRegion[0].uiX, 2);
            pDispChn->enlargeprm.uiCropY = SAL_alignDown(cropParam.stCropRegion[0].uiY, 2);
            pDispChn->enlargeprm.uiCropW = SAL_alignDown(cropParam.stCropRegion[0].uiW, 2);
            pDispChn->enlargeprm.uiCropH = SAL_alignDown(cropParam.stCropRegion[0].uiH, 2);
            pDispChn->enlargeprm.globalratio = pstDispCtrl->ratio;
            /* 临时修改hisi底层不支持缩放超过16倍 */
            if (cropParam.stCropRegion[0].uiW <= 120 || cropParam.stCropRegion[0].uiH <= 80)
            {
                DISP_LOGE("chn %d cropw,croph[%d,%d] err\n", pstDispCtrl->voChn, cropParam.stCropRegion[0].uiW, cropParam.stCropRegion[0].uiH);
                return SAL_FAIL;
            }

            DISP_LOGD("x %d y %d w %d h %d\n", cropParam.stCropRegion[0].uiX, cropParam.stCropRegion[0].uiY, cropParam.stCropRegion[0].uiW, cropParam.stCropRegion[0].uiH);

            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            #if (defined (DISP_VIDEO_FUSION))  /* 全局放大在chn中放大，因为用户融合只进入一次 vpe只能做一次裁剪，如果在grp做二次裁剪做为全局放大，那做局部放大时要在vpe_xxx.c同一个chan做第三次偏移做裁剪 */
            stParamInfo.enType = CHN_CROP_CFG;
            stParamInfo.stCrop.enCropType = CROP_SECOND;
            #else

            stParamInfo.enType = GRP_CROP_CFG;   /* 全局放大在grp中放大；在全局放大基础上再局部放大，NT里放大倍数不是很精确，所以先保留GRP裁剪 */
            #endif
            stParamInfo.stCrop.u32CropEnable = 1;
            stParamInfo.stCrop.u32H = SAL_alignDown(cropParam.stCropRegion[0].uiH, 2);
            stParamInfo.stCrop.u32W = SAL_alignDown(cropParam.stCropRegion[0].uiW, 2);
            stParamInfo.stCrop.u32X = SAL_alignDown(cropParam.stCropRegion[0].uiX, 2);
            stParamInfo.stCrop.u32Y = SAL_alignDown(cropParam.stCropRegion[0].uiY, 2);
            s32Ret = disp_tskSetVoChnDupPrm(pstDispCtrl->voDev, pDispChn, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }
        }
        else if (pstDispCtrl->ratio <= 1.0)
        {
            cropParam.uiCnt = 1;
            cropParam.stCropRegion[0].uiEnable = 0;
            cropParam.stCropRegion[0].channel = pstDispCtrl->voChn;
            cropParam.stCropRegion[0].uiW = inputwidth;
            cropParam.stCropRegion[0].uiH = inputheight;
            cropParam.stCropRegion[0].uiX = 0;
            cropParam.stCropRegion[0].uiY = 0;
            pDispChn->enlargeprm.uiCropX = SAL_alignDown(cropParam.stCropRegion[0].uiX, 2);
            pDispChn->enlargeprm.uiCropY = SAL_alignDown(cropParam.stCropRegion[0].uiY, 2);
            pDispChn->enlargeprm.uiCropW = cropParam.stCropRegion[0].uiW;
            pDispChn->enlargeprm.uiCropH = cropParam.stCropRegion[0].uiH;
            pDispChn->enlargeprm.globalratio = pstDispCtrl->ratio;
            if (pDispChn->enlargeprm.globalratio < 1.0)
            {
                pDispChn->enlargeprm.globalratio = 1.0;
            }

            if (cropParam.stCropRegion[0].uiW <= 120 || cropParam.stCropRegion[0].uiH <= 120)
            {
                DISP_LOGE("chn %d cropw,croph[%d,%d] err\n", pstDispCtrl->voChn, cropParam.stCropRegion[0].uiW, cropParam.stCropRegion[0].uiH);
                return SAL_FAIL;
            }

            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            #if (defined (DISP_VIDEO_FUSION))
            stParamInfo.enType = CHN_CROP_CFG;
            stParamInfo.stCrop.enCropType = CROP_SECOND;
            #else
            stParamInfo.enType = GRP_CROP_CFG;
            #endif
            stParamInfo.stCrop.u32CropEnable = 1;
            stParamInfo.stCrop.u32H = SAL_alignDown(cropParam.stCropRegion[0].uiH, 2);
            stParamInfo.stCrop.u32W = SAL_alignDown(cropParam.stCropRegion[0].uiW, 2);
            stParamInfo.stCrop.u32X = SAL_alignDown(cropParam.stCropRegion[0].uiX, 2);
            stParamInfo.stCrop.u32Y = SAL_alignDown(cropParam.stCropRegion[0].uiY, 2);
            s32Ret = disp_tskSetVoChnDupPrm(pstDispCtrl->voDev, pDispChn, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }
        }
        else
        {
            DISP_LOGE("error !\n");
            return SAL_FAIL;
        }

        pDispChn->enlargeprm.defaultenlargesign = pstDispCtrl->defaultenlargesign;
        if (((s32Ret == SAL_SOK) && (pstDispCtrl->ratio > 1.0) && (0 == pDispChn->enlargeprm.defaultenlargesign)) || (pDispChn->enlargeprm.partchn >= 0))
        {
            pDispChn->enlargeprm.uiDisappear = 1;
        }
        else
        {
            pDispChn->enlargeprm.uiDisappear = 0;
        }

        return s32Ret;
    }

    /* 局部放大 */
    if (pDispDev->szLayer.uiChnCnt > 0)
    {
        pDispChn->enlargeprm.globalchn = pstDispCtrl->viGlobalChn;
        pDispChnmain = disp_tskGetVoChn(pstDispCtrl->voDev, pDispChn->enlargeprm.globalchn);
        if (pDispChnmain == NULL)
        {
            DISP_LOGE("error !\n");
            return SAL_FAIL;
        }

        pDispChnmain->enlargeprm.partchn = pstDispCtrl->voChn;
        /* 设置局部放大时全局放大没有设置则默认全局放大为1.0, 下面计算局部放大参数时需要用到全局主画面参数 */
        if (pDispChnmain->enlargeprm.globalratio < 1.0)
        {
            pDispChnmain->enlargeprm.globalratio = 1.0;
            cropParam.uiCnt = 1;
            cropParam.stCropRegion[0].uiEnable = 0;
            cropParam.stCropRegion[0].channel = pDispChnmain->uiChn;
            cropParam.stCropRegion[0].uiW = inputwidth / pDispChnmain->enlargeprm.globalratio;
            cropParam.stCropRegion[0].uiH = inputheight / pDispChnmain->enlargeprm.globalratio;
            cropParam.stCropRegion[0].uiX = SAL_SUB_SAFE(inputwidth, cropParam.stCropRegion[0].uiW) / 2;  /* uiW裁剪宽度 */
            cropParam.stCropRegion[0].uiY = SAL_SUB_SAFE(inputheight, cropParam.stCropRegion[0].uiH) / 2; /* uiH裁剪高度 */
            pDispChnmain->enlargeprm.uiCropX = SAL_align(cropParam.stCropRegion[0].uiX, 2);
            pDispChnmain->enlargeprm.uiCropY = SAL_align(cropParam.stCropRegion[0].uiY, 2);
            pDispChnmain->enlargeprm.uiCropW = cropParam.stCropRegion[0].uiW;
            pDispChnmain->enlargeprm.uiCropH = cropParam.stCropRegion[0].uiH;
            if (cropParam.stCropRegion[0].uiW <= 120 || cropParam.stCropRegion[0].uiH <= 120)
            {
                DISP_LOGE("chn %d cropw,croph[%d,%d] err\n", pDispChnmain->uiChn, cropParam.stCropRegion[0].uiW, cropParam.stCropRegion[0].uiH);
                return SAL_FAIL;
            }
        }

        pDispChn->uiW = pstDispCtrl->w;
        pDispChn->uiH = pstDispCtrl->h;
        pDispChn->uiChn = pstDispCtrl->voChn;
        pDispChn->uiLayerNo = pstDispCtrl->voDev;
        pDispChn->stInModule.uiChn = uiDev;

        if (pDispChn->uiW > MIN_CROP_WIDTH && pDispChn->uiH > MIN_CROP_WIDTH)
        {
            /* 将x,y移动到显示画面窗口(0,0)处 */
            pstDispCtrl->x = SAL_SUB_SAFE(pstDispCtrl->x, pDispChnmain->uiX);
            pstDispCtrl->y = SAL_SUB_SAFE(pstDispCtrl->y, pDispChnmain->uiY);
            pstDispCtrl->w = pDispChn->uiW;
            pstDispCtrl->h = pDispChn->uiH;
            /* 检查传进来的x,y坐标有没有在显示区域以外 */
            if (SAL_SOK != disp_tskCheckDptzpartPrm(pstDispCtrl->voDev, pDispChn, pDispChnmain, pstDispCtrl))
            {
                DISP_LOGE("uiChnCnt %d,DptzPrm error !!\n", pDispDev->szLayer.uiChnCnt);
                return SAL_FAIL;
            }

            if (pstDispCtrl->ratio == 0.0)
            {
                DISP_LOGE("error \n");
                return SAL_FAIL;
            }

            /* 局部放大缩放宽高 */
            pstDispCtrl->w = (INT32)((float)pDispChn->uiW / pstDispCtrl->ratio) > MIN_CROP_WIDTH ? (INT32)((float)pDispChn->uiW / pstDispCtrl->ratio) : MIN_CROP_WIDTH;
            pstDispCtrl->h = (INT32)((float)pDispChn->uiH / pstDispCtrl->ratio) > MIN_CROP_WIDTH ? (INT32)((float)pDispChn->uiH / pstDispCtrl->ratio) : MIN_CROP_WIDTH;
            if (pDispChnmain->uiW > 0 && pDispChnmain->uiH > 0 && pDispChnmain->enlargeprm.uiCropW > 0 && pDispChnmain->enlargeprm.uiCropH > 0)
            {
                /* 局部放大 vpss20/vpss21 从grp到chn 宽会从1216放大到1920 高会从640放大到1080 所以映射相应点的时候要对x,y,w,h进行相应的放大，比如x先映射在grp中的位置 */
                /* 然后在映射到chn中的位置 */
                #if (defined (DISP_VIDEO_FUSION))
                {
                    pstDispCtrl->x = (pstDispCtrl->x * pDispChnmain->enlargeprm.uiCropW / pDispChnmain->uiW) + pDispChnmain->enlargeprm.uiCropX; /* + pDispChnmain->uiX; */
                    pstDispCtrl->y = (pstDispCtrl->y * pDispChnmain->enlargeprm.uiCropH / pDispChnmain->uiH) + pDispChnmain->enlargeprm.uiCropY; /* + pDispChnmain->uiY; */
                    pstDispCtrl->w = (pstDispCtrl->w * pDispChnmain->enlargeprm.uiCropW / pDispChnmain->uiW) * pDispChnmain->enlargeprm.uiCropW / capb_disp->disp_yuv_w_max;
                    pstDispCtrl->h = (pstDispCtrl->h * pDispChnmain->enlargeprm.uiCropH / pDispChnmain->uiH) * pDispChnmain->enlargeprm.uiCropH / capb_disp->disp_yuv_h;
                    DISP_LOGD("x %d y %d w %d h %d SRC %d %d %d %d dst %d %d %d %d\n", pDispChnmain->enlargeprm.uiCropX, pDispChnmain->enlargeprm.uiCropY, pDispChnmain->enlargeprm.uiCropW, pDispChnmain->enlargeprm.uiCropH,
                              pDispChnmain->uiX, pDispChnmain->uiY, pDispChnmain->uiW, pDispChnmain->uiH, pstDispCtrl->x, pstDispCtrl->y, pstDispCtrl->w, pstDispCtrl->h);
                }
                #else
                {
                    pstDispCtrl->x = (pstDispCtrl->x * pDispChnmain->enlargeprm.uiCropW / pDispChnmain->uiW) * capb_disp->disp_yuv_w_max / pDispChnmain->enlargeprm.uiCropW;
                    pstDispCtrl->y = (pstDispCtrl->y * pDispChnmain->enlargeprm.uiCropH / pDispChnmain->uiH) * capb_disp->disp_yuv_h / pDispChnmain->enlargeprm.uiCropH;
                    pstDispCtrl->w = (pstDispCtrl->w * pDispChnmain->enlargeprm.uiCropW / pDispChnmain->uiW) * capb_disp->disp_yuv_w_max / pDispChnmain->enlargeprm.uiCropW;
                    pstDispCtrl->h = (pstDispCtrl->h * pDispChnmain->enlargeprm.uiCropH / pDispChnmain->uiH) * capb_disp->disp_yuv_h / pDispChnmain->enlargeprm.uiCropH;
                }
                #endif

            }

            startX = SAL_alignDown(pstDispCtrl->x, 2);
            startY = SAL_alignDown(pstDispCtrl->y, 2);
            rectW = SAL_alignDown(pstDispCtrl->w, 2);
            rectH = SAL_alignDown(pstDispCtrl->h, 2);
            startX = SAL_alignDown(SAL_SUB_SAFE(startX, rectW / 2), 2);
            startY = SAL_alignDown(SAL_SUB_SAFE(startY, rectH / 2), 2);
            /* 防止裁剪时数据无效 */
            if (startX < 0)
            {
                startX = 0;
            }

            if (startY < 0)
            {
                startY = 0;
            }

            if (rectW < MIN_CROP_WIDTH)
            {
                rectW = MIN_CROP_WIDTH;
            }

            if (rectH < MIN_CROP_WIDTH)
            {
                rectH = MIN_CROP_WIDTH;
            }

            /* 局部放大vpssgrp chn使用2通道在chn中放大 0通道用于主屏显示 1通道用于辅屏显示 */
            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = CHN_CROP_CFG;
            stParamInfo.stCrop.enCropType = CROP_SECOND;
            stParamInfo.stCrop.u32CropEnable = 1;
            stParamInfo.stCrop.u32H = SAL_alignDown(rectH, 2);
            stParamInfo.stCrop.u32W = SAL_alignDown(rectW, 2);
            stParamInfo.stCrop.u32X = SAL_alignDown(startX, 2);
            stParamInfo.stCrop.u32Y = SAL_alignDown(startY, 2);
            s32Ret = disp_tskSetVoChnDupPrm(pstDispCtrl->voDev, pDispChn, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }

            /* 当放大不显示的时候加个打印 */
            if (0 == pDispChnmain->enlargeprm.uiDisappear
                || 0 == pDispChn->enlargeprm.uiDisappear)
            {
                if (cnt[pstDispCtrl->voDev] % 100 == 0)
                {
                    DISP_LOGW("chnDev %d,will uiDisappear 1 \n", pstDispCtrl->voDev);
                }

                cnt[pstDispCtrl->voDev]++;
            }

            /* 局部放大智能信息不显示 */
            pDispChnmain->enlargeprm.uiDisappear = 1;
            pDispChn->enlargeprm.uiDisappear = 1;

            stDispOsd.dispSvaSwitch = SAL_FALSE;
            Xpack_DrvSetAiXrOsdShow(pstDispCtrl->voDev, &stDispOsd, SAL_FALSE);
        }
        else
        {
            DISP_LOGE("uiChnCnt %d,error !!\n", pDispDev->szLayer.uiChnCnt);
        }
    }
    else
    {
        DISP_LOGE("uiChnCnt %d,error !!\n", pDispDev->szLayer.uiChnCnt);
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskStopDisp
 * @brief      停止显示（只是解绑定前一级）
 * @param[in]  UINT32 uiVoDev
 * @param[in]  VOID *prm    显示通道信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskStopDisp(UINT32 uiVoDev, VOID *prm)
{
    INT32 i = 0;
    INT32 s32Ret = SAL_SOK;
    UINT32 uiVoChn = 0;
    /* UINT32 uiGrp = 0; */
    /* UINT32 uiChn = 0; */
    DISP_CHN_COMMON *pDispChn = NULL;
    DSP_DISP_RCV_PARAM *pDispRegion = NULL;
    CAPB_DISP *capb_disp = NULL;
    DSPINITPARA *pDspInitPara = NULL;
    CHAR szName[NAME_LEN] = {0};
    CHAR szOutNodeNm[NAME_LEN] = {0};
    CHAR szInNodeNm[NAME_LEN] = {0};
    INST_INFO_S *pstSrcInst = NULL;
    INST_INFO_S *pstDstInst = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    DUP_ChanHandle *pstDup = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};

    pDspInitPara = SystemPrm_getDspInitPara();

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pDispRegion = (DSP_DISP_RCV_PARAM *)prm;

    if (NULL == pDispRegion)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(uiVoDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    /* 设置的参数表 */
    for (i = 0; i < pDispRegion->uiCnt; i++)
    {
        uiVoChn = pDispRegion->out[i];

        DISP_LOGI("cnt %d STOP DISP voDev %d voChn %d \n", i, uiVoDev, uiVoChn);

        pDispChn = disp_tskGetVoChn(uiVoDev, uiVoChn);
        if (pDispChn == NULL)
        {
            DISP_LOGW("warn\n");
            continue;
        }

#ifndef DISP_VIDEO_FUSION
        s32Ret = disp_hal_stopChn(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("dev:%u chn:%u stop fail\n", uiVoDev, uiVoChn);
            return SAL_FAIL;
        }

#endif
        if (pDispChn->stInModule.uiModId != SYSTEM_MOD_ID_VDEC)   /*解码绑定模式在解码停止时解绑和停止DUP*/
        {

            pstDup = disp_tskGetVoDupHandle(uiVoDev, pDispChn);
            if (NULL == pstDup)
            {
                DISP_LOGE("dev:%u chn:%u get dup handle fail\n", uiVoDev, uiVoChn);
                return SAL_FAIL;
            }

            if (pstDup->dupOps.OpDupStopBlit != NULL)
            {
                s32Ret = pstDup->dupOps.OpDupStopBlit((Ptr)pstDup);
                if (SAL_SOK != s32Ret)
                {
                    DUP_LOGE("OpDupStopBlit Chn err !!!\n");
                }
            }

            if (NODE_BIND_TYPE_SDK_BIND == pstDup->createFlags)
            {
                pDispChn->stInModule.frameNum = 0;
                SAL_WARN("VPSS (%d,%d) BIND TO VO (%d,%d)\n", pstDup->u32Grp, pstDup->u32Chn, pDispChn->uiLayerNo, pDispChn->uiChn);

                stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
                stDupBindPrm.modChn = pDispChn->uiLayerNo;
                stDupBindPrm.chn = pDispChn->uiChn;

                s32Ret = pstDup->dupOps.OpDupBindBlit((Ptr)pstDup, (Ptr) & stDupBindPrm, SAL_FALSE);
                if (SAL_isFail(s32Ret))
                {
                    DUP_LOGE("Bind Hal Chn err !!!\n");
                    return SAL_FAIL;
                }
            }

            if ((pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP) && (MOD_LINK_TYPE_USER == capb_disp->disp_link_type)
                && (PRODUCT_TYPE_ISM == pDspInitPara->dspCapbPar.dev_tpye))
            {
                pDispChn->stInModule.frameNum = 0;
                pDispDev->szLayer.runFlg[pDispChn->stInModule.uiChn] = 0;
                DISP_LOGW("chn %d VPSS will UnBIND TO VO %d voChn %d \n", i, uiVoDev, uiVoChn);
            }

            /* 这里先处理模拟通道的解绑逻辑，后面IPC解码绑定采用link之后，这里可以把这个判断语句去掉 */
            if ((pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP) || (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC))
            {
                if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
                {
                    snprintf(szName, NAME_LEN, "DUP_FRONT_%d", pDispChn->stInModule.uiChn);
                }
                else
                {
                    snprintf(szName, NAME_LEN, "DUP_VDEC_%d", pDispChn->stInModule.uiChn);
                }

                pstSrcInst = link_drvGetInst(szName);
                if (!pstSrcInst)
                {
                    DISP_LOGE("get inst %s failed. \n", szName);
                    return SAL_FAIL;
                }

                /* 这里约定out_disp0节点输出到显示器0上，out_disp1节点输出到显示器1上；
                            如果不做这个约定，需要上层下发哪个dup out_node输出到哪个显示器；*/
                snprintf(szOutNodeNm, NAME_LEN, "out_disp%d", uiVoDev);

                snprintf(szName, NAME_LEN, "DISP%d_CHN%d", uiVoDev, uiVoChn);
                pstDstInst = link_drvGetInst(szName);
                if (!pstDstInst)
                {
                    DISP_LOGE("get inst %s failed. \n", szName);
                    return SAL_FAIL;
                }

                snprintf(szInNodeNm, NAME_LEN, "in_0");

                /* 先解绑 */
                link_drvUnbind(pstSrcInst, szOutNodeNm, pstDstInst, szInNodeNm);
            }
        }
        else
        {
#ifdef DISP_VIDEO_FUSION   /*安检回放同步关闭显示后还需要从VPSS获取暂停帧*/
            pstDup = disp_tskGetVoDupHandle(uiVoDev, pDispChn);
            if (NULL == pstDup)
            {
                DISP_LOGE("dev:%u chn:%u get dup handle fail\n", uiVoDev, uiVoChn);
            }
            else
            {
                /* TODO: 不同设备间的兼容 */
                if (pstDup->dupOps.OpDupStopBlit != NULL)
                {
                    s32Ret = pstDup->dupOps.OpDupStopBlit((Ptr)pstDup);
                    if (SAL_SOK != s32Ret)
                    {
                        DUP_LOGE("OpDupStopBlit Chn err !!!\n");
                    }
                }
            }

#endif
        }

        pDispChn->stInModule.uiUseFlag = SAL_FALSE;
        pDispChn->stInModule.uiAiFlag = SAL_FALSE;
        pDispChn->stInModule.uiModId = 0x00;
        pDispChn->stInModule.uiHandle = 0x00;
        pDispChn->uiNoSignal = SAL_TRUE;
        pDispChn->stInModule.bClearOsdBuf = SAL_TRUE;

    }

    return SAL_SOK;
}

/**
 * @function   disp_tskVoChnStart
 * @brief      使能显示通道，设置初始化通道参数
 * @param[in]  UINT32 voDev
 * @param[in]  VOID *prm   显示通道属性
 * @param[out] None
 * @return     static INT32
 */

static INT32 disp_tskCheckChnPrm(DISP_LAYER_COMMON *pstLayer, DISP_REGION *pstRegin)
{
    /* INT32 s32Ret = 0; */

    if (pstRegin == NULL || pstLayer == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    if (pstRegin->x > pstLayer->uiLayerWith || pstRegin->y > pstLayer->uiLayerHeight
        || pstRegin->w > pstLayer->uiLayerWith || pstRegin->h > pstLayer->uiLayerHeight)
    {
        DISP_LOGE("voDev %d alloc( %d, %d ) w %d h %d,layer w %d h %d \n", pstLayer->uiLayerNo, pstRegin->x, pstRegin->y, pstRegin->w, pstRegin->h, pstLayer->uiLayerWith, pstLayer->uiLayerHeight);
        return SAL_FAIL;
    }
    else if (((pstRegin->x + pstRegin->w) > pstLayer->uiLayerWith) || ((pstRegin->y + pstRegin->h) > pstLayer->uiLayerHeight))
    {
        DISP_LOGE("voDev %d alloc( %d, %d ) w %d h %d,layer w %d h %d \n", pstLayer->uiLayerNo, pstRegin->x, pstRegin->y, pstRegin->w, pstRegin->h, pstLayer->uiLayerWith, pstLayer->uiLayerHeight);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskVoChnStart
 * @brief      使能显示通道，设置初始化通道参数
 * @param[in]  UINT32 voDev
 * @param[in]  VOID *prm   显示通道属性
 * @param[out] None
 * @return     static INT32
 */

static INT32 disp_tskAddVo(UINT32 voDev, VOID *prm)
{
    UINT32 i = 0, AddChnNum = 0, j = 0;
    INT32 s32Ret = 0;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CTRL *pstDispCtrl = NULL;
    UINT32 skip = 0;
    PARAM_INFO_S stParamInfo;

    DISP_LOGI("disp_tskAddVo start %d!!\n", voDev);

    pstDispCtrl = (DISP_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(voDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    /* 获取要显示的通道个数  pstDispCtrl->uiCnt =1 */
    if (pstDispCtrl->uiCnt > disp_tskGetVoChnCnt(voDev))
    {
        DISP_LOGE("error uiChnCnt %d prm uiCnt = %d\n", pDispDev->szLayer.uiChnCnt, pstDispCtrl->uiCnt);
        return SAL_FAIL;
    }

    /* 显示单个通道 */
    AddChnNum = pstDispCtrl->uiCnt;
    for (j = 0; j < AddChnNum; j++)
    {
        i = pstDispCtrl->stDispRegion[j].uiChan;

        pDispChn = disp_tskGetVoChn(voDev, i);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error !\n");
            return SAL_FAIL;
        }

        pstDispCtrl->stDispRegion[j].x = SAL_align(pstDispCtrl->stDispRegion[j].x, 2);
        pstDispCtrl->stDispRegion[j].y = SAL_align(pstDispCtrl->stDispRegion[j].y, 2);
        pstDispCtrl->stDispRegion[j].w = SAL_align(pstDispCtrl->stDispRegion[j].w, 16);    /* NT平台VPE需要16bit对齐 */
        pstDispCtrl->stDispRegion[j].h = SAL_align(pstDispCtrl->stDispRegion[j].h, 2);


        s32Ret = disp_tskCheckChnPrm(&pDispDev->szLayer, &pstDispCtrl->stDispRegion[j]);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

        pDispChn->uiX = pstDispCtrl->stDispRegion[j].x;
        pDispChn->uiY = pstDispCtrl->stDispRegion[j].y;
        pDispChn->uiW = pstDispCtrl->stDispRegion[j].w;
        pDispChn->uiH = pstDispCtrl->stDispRegion[j].h;
        pDispChn->uiColor = pstDispCtrl->stDispRegion[j].color;
        pDispChn->uiChn = pstDispCtrl->stDispRegion[j].uiChan;
        pDispChn->uiLayer = pstDispCtrl->stDispRegion[j].layer;

        if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP || pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
        {

            pDispChn->stInModule.frameNum = 0;
            skip = 0;
        }
        else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
        {
            skip = 0;
        }
        else
        {
            skip = 1;
        }

        /*vo输入源是VDEC需要设置vpss属性*/
        if (skip == 0)
        {
            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = IMAGE_SIZE_CFG;
            stParamInfo.stImgSize.u32Width = pDispChn->uiW;
            stParamInfo.stImgSize.u32Height = pDispChn->uiH;
            s32Ret = disp_tskSetVoChnDupPrm(voDev, pDispChn, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                /* return SAL_FAIL; */
            }
        }

        pDispChn->bDispBorder = pstDispCtrl->stDispRegion[j].bDispBorder;
        pDispChn->BorDerColor = pstDispCtrl->stDispRegion[j].BorDerColor;
        pDispChn->BorDerLineW = pstDispCtrl->stDispRegion[j].BorDerLineW;
        pDispChn->uiScaleAlgo = SAL_FALSE;
        s32Ret = disp_hal_enableChn(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

        pDispChn->uiNoSignal = SAL_TRUE;
        /*创建时窗口优先级设置最高，拖动时调整优先级*/
        pDispChn->u32Priority = pDispChn->uiChn;

        s32Ret = Disp_halSetVoChnPriority((void *)pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("Disp_halSetVoChnPriority err !!\n");
            return SAL_FAIL;
        }

#ifdef  DISP_VIDEO_FUSION
        disp_cleardupCopyDispPool(pDispChn); /* 显示缓存池要清掉，拼帧线程从该显示缓存池那帧，如果重新开启该通道先不清一下，会导致把上一次开启状态时的帧给拼上 */
#endif
        disp_tskVoChnCtrl(pDispChn, SAL_TRUE);
    }

    if (pDispChn != NULL)
    {
        DISP_LOGI("disp_tskAddVo %d end !!\n", pDispChn->uiChn);
        return SAL_SOK;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskVoChnStart
 * @brief      使能显示通道，设置初始化通道参数
 * @param[in]  UINT32 voDev
 * @param[in]  VOID *prm   显示通道属性
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskVoChnStart(UINT32 voDev, VOID *prm)
{
    UINT32 i = 0;
    INT32 s32Ret = 0;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CTRL *pstDispCtrl = NULL;
    UINT32 skip = 0;
    PARAM_INFO_S stParamInfo;

    pstDispCtrl = (DISP_CTRL *)prm;
    if (pstDispCtrl == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(voDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    /* 获取要显示的通道个数 */
    pDispDev->szLayer.uiChnCnt = pstDispCtrl->uiCnt;

    if (pDispDev->szLayer.uiChnCnt == 0x00 || pDispDev->szLayer.uiChnCnt > disp_tskGetVoChnCnt(voDev))
    {
        DISP_LOGE("error uiChnCnt %d \n", pDispDev->szLayer.uiChnCnt);
        return SAL_FAIL;
    }

    /* 显示单个通道 */
    if (pDispDev->szLayer.uiChnCnt == 1)
    {
        pDispChn = disp_tskGetVoChn(voDev, pstDispCtrl->stDispRegion[0].uiChan);
        if (pDispChn == NULL)
        {
            DISP_LOGE("dev:%u chn:%u get chn fail!\n", voDev, pstDispCtrl->stDispRegion[0].uiChan);
            return SAL_FAIL;
        }

        s32Ret = disp_tskCheckVoChnPrm(&pDispDev->szLayer, &pstDispCtrl->stDispRegion[0]);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

        pDispChn->uiX = pstDispCtrl->stDispRegion[0].x;
        pDispChn->uiY = pstDispCtrl->stDispRegion[0].y;
        pDispChn->uiW = pstDispCtrl->stDispRegion[0].w;
        pDispChn->uiH = pstDispCtrl->stDispRegion[0].h;
        pDispChn->uiColor = pstDispCtrl->stDispRegion[0].color;
        pDispChn->uiChn = pstDispCtrl->stDispRegion[0].uiChan;
        pDispChn->uiLayer = pstDispCtrl->stDispRegion[0].layer;

        if ((pDispDev->uiDevWith > 0) && (pDispDev->uiDevHeight > 0))
        {
            pDispChn->stChnPrm.uiRatio = ((float)pDispChn->uiW * (float)pDispChn->uiH) / (pDispDev->uiDevWith * pDispDev->uiDevHeight);
            DISP_LOGI("dev %u, chn %u, x %u, y %u, w %u, h %u, devW: %u, devH: %u, ratio %f\n", voDev, pDispChn->uiChn,
                      pDispChn->uiX, pDispChn->uiY, pDispChn->uiW, pDispChn->uiH,
                      pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispChn->stChnPrm.uiRatio);
        }

        if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP || pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
        {
            pDispChn->stInModule.frameNum = 0;
            skip = 0;
        }
        else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
        {
            skip = 0;
        }
        else
        {
            skip = 1;
        }

        /*vo输入源是VDEC需要设置vpss属性*/
        if (skip == 0)
        {
            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = IMAGE_SIZE_CFG;
            stParamInfo.stImgSize.u32Width = pDispChn->uiW;
            stParamInfo.stImgSize.u32Height = pDispChn->uiH;
            s32Ret = disp_tskSetVoChnDupPrm(voDev, pDispChn, &stParamInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }
        }

        pDispChn->bDispBorder = pstDispCtrl->stDispRegion[0].bDispBorder;
        pDispChn->BorDerColor = pstDispCtrl->stDispRegion[0].BorDerColor;
        pDispChn->BorDerLineW = pstDispCtrl->stDispRegion[0].BorDerLineW;
        pDispChn->uiScaleAlgo = SAL_FALSE;   /*默认开始时不使用新算法，实际绑定的时候设置*/
        s32Ret = disp_hal_enableChn(pDispChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }

        pDispChn->uiNoSignal = SAL_TRUE;

#ifdef  DISP_VIDEO_FUSION
        disp_cleardupCopyDispPool(pDispChn); /* 显示缓存池要清掉，拼帧线程从该显示缓存池那帧，如果重新开启该通道先不清一下，会导致把上一次开启状态时的帧给拼上 */
#endif
        disp_tskVoChnCtrl(pDispChn, SAL_TRUE); /* 线程是在这里面创造的 */
    }
    else
    {
        /* 使能显示设备上的多个显示通道 */
        for (i = 0; i < pDispDev->szLayer.uiChnCnt; i++)
        {
            pDispChn = disp_tskGetVoChn(voDev, i);
            if (pDispChn == NULL)
            {
                DISP_LOGE("error !\n");
                return SAL_FAIL;
            }

            s32Ret = disp_tskCheckVoChnPrm(&pDispDev->szLayer, &pstDispCtrl->stDispRegion[i]);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }

            pDispChn->uiX = pstDispCtrl->stDispRegion[i].x;
            pDispChn->uiY = pstDispCtrl->stDispRegion[i].y;
            pDispChn->uiW = pstDispCtrl->stDispRegion[i].w;
            pDispChn->uiH = pstDispCtrl->stDispRegion[i].h;
            pDispChn->uiColor = pstDispCtrl->stDispRegion[i].color;
            pDispChn->uiChn = pstDispCtrl->stDispRegion[i].uiChan;
            pDispChn->uiLayer = pstDispCtrl->stDispRegion[i].layer;
            
            if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
            {
                skip = 0;
                pDispChn->stInModule.frameNum = 0;
            }
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
            {
                skip = 0;
            }
            else
            {
                skip = 1;
            }

            /*vo输入源是VDEC需要设置vpss属性*/
            if (skip == 0)
            {
                memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
                stParamInfo.enType = IMAGE_SIZE_CFG;
                stParamInfo.stImgSize.u32Width = pDispChn->uiW;
                stParamInfo.stImgSize.u32Height = pDispChn->uiH;
                s32Ret = disp_tskSetVoChnDupPrm(voDev, pDispChn, &stParamInfo);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGE("error !!\n");
                    return SAL_FAIL;
                }
            }

            pDispChn->bDispBorder = pstDispCtrl->stDispRegion[i].bDispBorder;
            pDispChn->BorDerColor = pstDispCtrl->stDispRegion[i].BorDerColor;
            pDispChn->BorDerLineW = pstDispCtrl->stDispRegion[i].BorDerLineW;
            pDispChn->uiScaleAlgo = SAL_FALSE;    /*默认开始时不使用新算法，实际绑定的时候设置*/
            s32Ret = disp_hal_enableChn(pDispChn);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }

            pDispChn->uiNoSignal = SAL_TRUE;
            if ((pDispDev->uiDevWith > 0) && (pDispDev->uiDevHeight > 0))
            {
                pDispChn->stChnPrm.uiRatio = ((float)pDispChn->uiW * (float)pDispChn->uiH) / (pDispDev->uiDevWith * pDispDev->uiDevHeight);
                DISP_LOGI("dev %u, chn %u, idx %d, x %u, y %u, w %u, h %u, devW: %u, devH: %u, ratio %f\n", voDev, pDispChn->uiChn, i,
                          pDispChn->uiX, pDispChn->uiY, pDispChn->uiW, pDispChn->uiH,
                          pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispChn->stChnPrm.uiRatio);
            }

#ifdef  DISP_VIDEO_FUSION
            disp_cleardupCopyDispPool(pDispChn); /* 显示缓存池要清掉，拼帧线程从该显示缓存池那帧，如果重新开启该通道先不清一下，会导致把上一次开启状态时的帧给拼上 */
#endif
            disp_tskVoChnCtrl(pDispChn, SAL_TRUE);
        }
    }

    return SAL_SOK;
}

#if 0

/**
 * @function    save_output
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void save_output(char *filename, void *data, int size)
{
    FILE *pfile;

    /* Open output file */
    if ((pfile = fopen(filename, "wb")) == NULL)
    {
        printf("[ERROR] Open File %s failed!!\n", filename);
        exit(1);
    }

    /* Write buffer to output file */
    fwrite(data, 1, size, pfile);
    fclose(pfile);
    printf("Write file: %s\n", filename);
}

/**
 * @function    read_input
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void read_input(char *filename, void *data, int size)
{
    FILE *pfile;

    /* Open output file */
    if ((pfile = fopen(filename, "rb")) == NULL)
    {
        printf("[ERROR] Open File %s failed!!\n", filename);
        exit(1);
    }

    /* Write buffer to output file */
    fread(data, 1, size, pfile);
    fclose(pfile);
    printf("read file: %s\n", filename);
}

#endif


/**
 * @function   disp_tskSendFrame
 * @brief      向VO通道发送一帧数据
 * @param[in]  DISP_CHN_COMMON *pDispChn
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrame  帧信息
 * @param[in]  UINT32 uiRotate    解码显示AI状态信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskSendFrame(DISP_CHN_COMMON *pDispChn, SYSTEM_FRAME_INFO *pstSystemFrame, UINT32 uiRotate)
{
    INT32 s32Ret = SAL_SOK;
    CAPB_DISP *capb_disp = NULL;
    CAPB_PRODUCT *pstProduct = NULL;
    static UINT32 u32Count[4] = {0};
    UINT32 time1, time2;
    UINT32 u32LayNo = 0;

    /* 获取能力级*/
    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pstProduct = capb_get_product();
    if (NULL == pstProduct)
    {
        DISP_LOGE("get platform capblity fail\n");
        return SAL_FAIL;
    }

    /* 参数校验*/
    if (NULL == pstSystemFrame || NULL == pDispChn)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    u32LayNo = pDispChn->uiLayerNo;

    /* 叠加AI信息，目前只有DUP通道会叠框 */
    if (pDispChn->stInModule.uiAiFlag == SAL_TRUE)
    {
        DISP_TAR_CNT_INFO *pstPkgCntInfo = NULL;
        SAL_VideoFrameBuf salVideoFrame = {0};
        SVA_PROCESS_IN *pstIn = NULL;
        SVA_PROCESS_OUT *pstOut = NULL;
        UINT32 uiChn = 0;
        UINT32 u32TimeRef = 0;

        pstIn = &g_stIn[pDispChn->uiLayerNo][pDispChn->uiChn];
        pstOut = &g_stOut[pDispChn->uiLayerNo][pDispChn->uiChn];
        pstPkgCntInfo = &g_stPkgTarCnt[pDispChn->uiLayerNo][pDispChn->uiChn];

        memset(pstIn, 0x00, sizeof(SVA_PROCESS_IN));
        memset(pstOut, 0x00, sizeof(SVA_PROCESS_OUT));
        memset(pstPkgCntInfo, 0x00, sizeof(DISP_TAR_CNT_INFO));

        if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
        {
            uiChn = pDispChn->stInModule.uiChn;
            s32Ret = Sva_DrvGetCfgParam(uiChn, pstIn);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("Get Sva Cfg Param Failed! chan %d \n", uiChn);
                return SAL_FAIL;
            }

            /* 目前只有一个通道支持算法 */
            if (SAL_TRUE == pstProduct->bXPackEnable)
            {
                time1 = SAL_getCurMs();
                s32Ret = disp_hal_sendFrame(pDispChn, (void *)pstSystemFrame->uiAppData, -1);
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGE("dev %d chn %d uichn %d send frame failed\n", pDispChn->uiDevNo, pDispChn->uiChn, pDispChn->stInModule.uiChn);
                    return SAL_FAIL;
                }

                time2 = SAL_getCurMs();
                u32Count[u32LayNo]++;
                if (time2 - time1 > 9)
                {
                    /* if (((u32Count[u32LayNo] % 1000) == 0)) */
                    DISP_LOGW("disp_hal_sendFrame spent time %u,  layerNo %d, u32Count %u\n", time2 - time1, u32LayNo, u32Count[u32LayNo]);
                }

                return SAL_SOK;
            }
            else
            {
                /* s32Ret = Sva_DrvGetSvaOut(uiChn, pstOut); */
                s32Ret = sys_hal_getVideoFrameInfo(pstSystemFrame, &salVideoFrame);
                if (SAL_SOK != s32Ret)
                {
                    DISP_LOGW("disp_hal_getFrameInfo failed !\n");
                }

                u32TimeRef = salVideoFrame.frameNum;

                s32Ret = Sva_DrvGetSvaFromPool(uiChn, u32TimeRef, pstOut);
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGW("Sva_DrvGetSvaFromPool failed !\n");
                }

                if ((salVideoFrame.frameNum != pstOut->uiTimeRef) && (pstOut->frame_num != 0))
                {
                    DISP_LOGD("disp_timRef %u, sva TimeRef %u, frm_num %u, uiChn %u \n",
                              salVideoFrame.frameNum, pstOut->uiTimeRef, pstOut->frame_num, uiChn);
                }
            }
        }
        else /*解码模块结果放在队列，只要取了一帧解码数据，就得取一个结果*/
        {
            if (uiRotate == 0x00)
            {
                uiChn = (UINT32)pDispChn->stInModule.uiHandle;
                vca_unpack_getResult(uiChn, pstOut);
            }
        }

        s32Ret = disp_osdAiDrawFrame(pDispChn, pstSystemFrame, pstOut, pstIn, pstPkgCntInfo);
    }

    time1 = SAL_getCurMs();
    s32Ret |= disp_hal_sendFrame(pDispChn, (void *)pstSystemFrame->uiAppData, -1);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("dev %d chn %d uichn %d send frame failed\n", pDispChn->uiDevNo, pDispChn->uiChn, pDispChn->stInModule.uiChn);
        return SAL_FAIL;
    }

    time2 = SAL_getCurMs();
    u32Count[u32LayNo]++;
    if (((u32Count[u32LayNo] % 1000) == 0) && 0)
    {
        DISP_LOGW("disp_hal_sendFrame spent time %u,  layerNo         %d, u32Count %u\n", time2 - time1, u32LayNo, u32Count[u32LayNo]);
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskGetNoSignalPic
 * @brief      获取无视频信号数据
 * @param[in]  UINT32 status
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskGetNoSignalPic(UINT32 status, SYSTEM_FRAME_INFO *pstSystemFrame)
{
    INT32 s32Ret = SAL_FAIL;
    VOID *pFrame = NULL;
    DSPINITPARA *pDspInitPara = NULL;
    SAL_VideoFrameBuf salVideoFrame = {0};

    if (NULL == pstSystemFrame)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    salVideoFrame.frameParam.width = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgW[status];
    salVideoFrame.frameParam.height = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgH[status];
    salVideoFrame.virAddr[0] = (PhysAddr)(pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgAddr[status]);
    salVideoFrame.virAddr[1] = salVideoFrame.virAddr[0] + salVideoFrame.frameParam.width * salVideoFrame.frameParam.height;
    salVideoFrame.bufLen = pDspInitPara->stCaptNoSignalInfo.uiNoSignalImgSize[status];

    if (salVideoFrame.bufLen == 0x00 || salVideoFrame.frameParam.width == 0 || salVideoFrame.frameParam.height == 0)
    {
        DISP_LOGE("error picSize %d w %d h %d \n", salVideoFrame.bufLen, salVideoFrame.frameParam.width, salVideoFrame.frameParam.height);
        return SAL_FAIL;
    }

    pFrame = (void *)pstSystemFrame->uiAppData;

    s32Ret = disp_hal_putNoSignalPicFrame(&salVideoFrame, pFrame);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("disp_hal_setFrameInfo err !\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#ifndef DISP_VIDEO_FUSION

/**
 * @function   Disp_tskVoChnThread
 * @brief      显示通道任务线程
 * @param[in]  void *pPrm  通道属性
 * @param[out] None
 * @return     static void *
 */
static void *disp_tskVoChnThread(void *pPrm)
{
    INT32 s32Ret = 0;
    UINT32 voDev = 0;
    UINT32 voChn = 0;
    UINT32 vdecChn = 0;
    UINT32 LogoPicFresh = 0; /* hiklogo vgs缩放失败再刷新一帧 */
    INT32 iWaitCnt = 0;
    INT32 idx = 0, w_idx = 0, r_idx = 0;
    DUP_COPY_DATA_BUFF stDupCopyBuff;
    DUP_COPY_DATA_BUFF_POOL stDupCopyPool;
    DUP_COPY_DATA_BUFF *pstDupCopyBuff = NULL;
    static SYSTEM_FRAME_INFO stSystemFrame;
    SYSTEM_FRAME_INFO stGetPutFrame;
    SYSTEM_FRAME_INFO stGetPutFrameInfo[DUP_MNG_FRM_CNT];
    DUP_ChanHandle *pstDupHandle = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    SYSTEM_FRAME_INFO stSystemFrameResize = {0};
    UINT64 time0 = 0, time1 = 0;
    UINT32 fps = 0;
    UINT32 uiModId = SYSTEM_MOD_ID_MAX;
    CAPB_DISP *capb_disp = NULL;
    UINT32 needFree = 0;
    DISP_DEBUG_CHN_TIME *pstDbgTime = NULL;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    UINT32 lastuiW = 0,lastuiH = 0;

    /* 入口参数校验 */
    pDispChn = (DISP_CHN_COMMON *)pPrm;
    if (pDispChn == NULL)
    {
        DISP_LOGE("error !\n");
        return NULL;
    }

    /* 通道和设备信息获取*/
    voDev = pDispChn->uiDevNo;
    voChn = pDispChn->uiChn;

    /* 统计信息索引初始化*/
    pDispChn->u32DbgTimeIdx = 0;

    /* 能力级获取 */
    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return NULL;
    }

    /* DISP_LOGI("VoDev %d VoChn %d %s Create %p!\n",voDev,voChn,__func__,pDispChn); */

    /* 参数初始化 */
    memset(&stDupCopyBuff, 0x00, sizeof(DUP_COPY_DATA_BUFF));
    memset(&stDupCopyPool, 0x00, sizeof(DUP_COPY_DATA_BUFF_POOL));
    memset(&stSystemFrameResize, 0x00, sizeof(SYSTEM_FRAME_INFO));
    /* 旋转、无视频信号等临时处理帧缓冲申请,只在通道0的时候申请一次*/
    if (stSystemFrame.uiAppData == 0x00 && voChn == 0)
    {
        s32Ret = disp_hal_getFrameMem(1920, 1080, &stSystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            goto out3;
        }

        s32Ret = disp_tskGetNoSignalPic(0, &stSystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            goto out2;
        }

        gDispDebug[voDev][voChn].no_signal_frame_before = (void *)stSystemFrame.uiAppData;
    }

    memset(&stGetPutFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));
    if (stGetPutFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stGetPutFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            goto out2;
        }

        gDispDebug[voDev][voChn].vo_frame = (void *)stGetPutFrame.uiAppData;
    }

    /* 显示送帧管理，帧信息*/
    stDupCopyBuff.stDupDataCopyPrm.uiRotate = 0xFFFF;
    memset(&stGetPutFrameInfo, 0x00, sizeof(stGetPutFrameInfo));
    for (idx = 0; idx < DUP_MNG_FRM_CNT; idx++)
    {
        stDupCopyPool.stDupFrm[idx].stDupDataCopyPrm.uiRotate = 0xFFFF;
        stDupCopyPool.stDupFrm[idx].pstDstSysFrame = &(stGetPutFrameInfo[idx]);
        if (stGetPutFrameInfo[idx].uiAppData == 0x00)
        {
            s32Ret = sys_hal_allocVideoFrameInfoSt(&(stGetPutFrameInfo[idx]));
            if (s32Ret != SAL_SOK)
            {
                DISP_LOGE("disp_hal_getFrameMem error !\n");
                goto out1;
            }
        }
    }

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "disp_tskVo_%d_%d", voDev, voChn);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    /* 这个延时具体机制，随后分析 */
    SAL_msleep(5000);

    while (1)
    {
        /* 时间调试统计 */
        pstDbgTime = &pDispChn->stDbgTime[pDispChn->u32DbgTimeIdx];
        pstDbgTime->u64TimeEntry = sal_get_tickcnt();

        /* 等待清屏 */
        iWaitCnt = 200;
        while (DISP_CLEAN_STATUS_START == pDispChn->enCleanStatus)
        {
            if (pDispChn->stInModule.uiUseFlag == SAL_FALSE)
            {
                pDispChn->flag = SAL_TRUE;
            }

            SAL_msleep(10);
            iWaitCnt--;
            if (iWaitCnt < 0)
            {
                DISP_LOGE("chn[%u] waiting for screen clean timeout\n", voChn);
                break;
            }
        }

        /* 调整线程优先级 */
        if (pDispChn->stInModule.uiModId != uiModId)
        {
            uiModId = pDispChn->stInModule.uiModId;
            pstDupHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;

            /* DUP手动获取模式，提高优先级*/
            if ((uiModId == SYSTEM_MOD_ID_DUP)
                && (pstDupHandle != NULL)
                && (pstDupHandle->createFlags == NODE_BIND_TYPE_GET))
            {
                SAL_thrChangePri(&pDispChn->stChnThrHandl, SAL_THR_PRI_MAX - 15);
            }
            else
            {
                SAL_thrChangePri(&pDispChn->stChnThrHandl, SAL_THR_PRI_MIN);
            }
        }

        /* 打印显示帧率信息 */
        if ((time1 - time0 >= 990) && (fps > 0) && (voChn == 0))
        {
            DISP_LOGD("voDev %d,fps %d,%llu\n", voDev, fps, time1 - time0);
            if ((fps < 50) && (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP))
            {
                DISP_LOGI("voDev %d,fps %d,%llu\n", voDev, fps, time1 - time0);
            }

            fps = 0;
        }

        if (fps == 0)
        {
            /*和sva模块一样统一使用系统时间，调试有问题再切换*/
            time0 = disp_tskGetTimeMilli();
        }

        time1 = disp_tskGetTimeMilli();

        /* 1、等待通道被启用 */
        disp_tskVoChnWait(pDispChn);

        /* 2、智能信息闪烁 */
        #ifdef DSP_ISM
        disp_osdFlickCheck(pDispChn);
        #endif

        /* 时间调试统计 */
        pstDbgTime->u64TimePrcStart = sal_get_tickcnt();
        pstDbgTime->uiUseFlag = pDispChn->stInModule.uiUseFlag;
        pstDbgTime->uiModId = pDispChn->stInModule.uiModId;

        /* 3、显示帧控制 */
        if (pDispChn->stInModule.uiUseFlag == SAL_TRUE) /* 输入模块被启用 */
        {
              if (stSystemFrameResize.uiAppData != 0x00 && pDispChn->stInModule.uiModId != SYSTEM_MOD_ID_VDEC)
              {
                  (VOID)disp_hal_putFrameMem(&stSystemFrameResize);
                  memset(&stSystemFrameResize, 0x00, sizeof(SYSTEM_FRAME_INFO));
                  lastuiH = 0;
                  lastuiW = 0;
              }
            /* 解码源：中间过段状态，是为了重置显示通道，或者发送无视频信号视频 */
            if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
            {
                 if(pDispChn->uiH == 0 || pDispChn->uiW == 0)
                 {
                     DISP_LOGE("error  pDispChn->uiH:%d  pDispChn->uiW:%d\n",pDispChn->uiH,pDispChn->uiW);
                     continue ;
                 }
                 if(lastuiH != pDispChn->uiH || lastuiW != pDispChn->uiW)
                 {
                    if (stSystemFrameResize.uiAppData != 0x00)
                    {
                        (VOID)disp_hal_putFrameMem(&stSystemFrameResize);
                        memset(&stSystemFrameResize, 0x00, sizeof(SYSTEM_FRAME_INFO));
                    }
                    if (stSystemFrameResize.uiAppData == 0x00)
                    {
                        lastuiW = SAL_align(pDispChn->uiW,64);
                        s32Ret = disp_hal_getFrameMem(lastuiW, pDispChn->uiH, &stSystemFrameResize);
                        if (s32Ret != SAL_SOK)
                        {
                            DISP_LOGE("disp_hal_getFrameMem error !\n");
                            lastuiH = 0;
                            lastuiW = 0;
                            continue;
                        }
                    }
                    lastuiH = pDispChn->uiH;
                    lastuiW = pDispChn->uiW;
                 }
                SAL_mutexLock(pDispChn->mChnMutexHdl);
                vdecChn = (UINT32)pDispChn->stInModule.uiHandle;

                /* 复用 */
                pDispChn->stChnPrm.uiRotate = 0;
                s32Ret = vdec_tsk_GetVdecFrame(vdecChn, DISP_VDEC_DUP_CHN, VDEC_DISP_GET_FRAME, &needFree, &stGetPutFrame, &pDispChn->stChnPrm.uiRotate);
                if (s32Ret != SAL_SOK)
                {
                    SAL_mutexUnlock(pDispChn->mChnMutexHdl);
                    SAL_msleep(10);
                    DISP_LOGD("vdecCh %d,voDev %d\n", vdecChn, voDev);
                    continue;
                }
                else
                {
                    if (pDispChn->srcStatus == pDispChn->stChnPrm.uiRotate && pDispChn->stChnPrm.uiRotate == 1)
                    {
                        SAL_mutexUnlock(pDispChn->mChnMutexHdl);
                        SAL_msleep(10);
                        DISP_LOGD("vdecCh %u,voDev %u,srcStatus %u\n", vdecChn, voDev, pDispChn->srcStatus);
                        continue;
                    }

                    DISP_LOGD("vdecCh %u,voDev %u,srcStatus %u\n", vdecChn, voDev, pDispChn->srcStatus);
                    pDispChn->srcStatus = pDispChn->stChnPrm.uiRotate;
                    /* 送显示 */
                    if (pDispChn->uiEnable == SAL_TRUE)
                    {
                        if (pDispChn->stChnPrm.uiRotate == 1)
                        {
                            vgs_hal_scaleFrame(&stSystemFrameResize, &stGetPutFrame,
                                               pDispChn->uiW, pDispChn->uiH);
                            s32Ret = disp_tskSendFrame(pDispChn, &stSystemFrameResize, pDispChn->stChnPrm.uiRotate);
                            if (s32Ret != SAL_SOK)
                            {
                                DISP_LOGW("disp_tskSendFrame warning !\n");
                            }

                            DISP_LOGI("disp_tskSendFrame no video pic (w:%d,h:%d)!\n", pDispChn->uiW, pDispChn->uiH);
                        }
                        else
                        {
                            s32Ret = disp_tskSendFrame(pDispChn, &stGetPutFrame, pDispChn->stChnPrm.uiRotate);
                            if (s32Ret != SAL_SOK)
                            {
                                DISP_LOGW("disp_tskSendFrame warning !\n");
                            }
                        }
                    }

                    if (needFree)
                    {
                        vdec_tsk_PutVdecFrame(vdecChn, DISP_VDEC_DUP_CHN, &stGetPutFrame);
                    }
                }
                SAL_mutexUnlock(pDispChn->mChnMutexHdl);
                SAL_msleep(16);   /*解码帧率30帧，需要休眠获取后续增加帧率控制*/
            }
            /* 输入源：DUP或VI */
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP) /* dup */
            {
                pDispChn->srcStatus = 0;
                pstDupHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
                if (pstDupHandle == NULL)
                {
                    DISP_LOGW("pstDupHandle %p,voChn %d\n", pstDupHandle, voChn);
                    SAL_msleep(10);
                    continue;
                }

                /* 通过DUP的号码，获取DUP信息，包括通道的handle。 */
                /* 通过通道handle信息，获取vpss对应通道数据 */
                if (NODE_BIND_TYPE_GET == pstDupHandle->createFlags) /* 手动获取 */
                {
                    w_idx = stDupCopyPool.w_idx;
                    pstDupCopyBuff = &(stDupCopyPool.stDupFrm[w_idx]);
                    /* dup中获取视频 */

                    pstDbgTime->u64TimeGetStart = sal_get_tickcnt();

                    if (SAL_SOK == pstDupHandle->dupOps.OpDupGetBlit(pstDupHandle, pstDupCopyBuff))
                    {
                        pstDbgTime->u64TimeGetTimeOut = SAL_FALSE;
                        pstDbgTime->u64TimeGetEnd = sal_get_tickcnt();

                        stDupCopyPool.w_idx = (w_idx + 1) % DUP_MNG_FRM_CNT;
                        stDupCopyPool.new_cnt++;

                        pstDbgTime->u64TimeSendStart = sal_get_tickcnt();

                        /* 延迟delay_nums 帧送去显示，等算法处理完毕*/
                        if (stDupCopyPool.new_cnt > capb_disp->disp_delay_nums)
                        {
                            r_idx = stDupCopyPool.r_idx;
                            pstDupCopyBuff = &(stDupCopyPool.stDupFrm[r_idx]);
                            SAL_mutexLock(pDispChn->mChnMutexHdl);
                            /* 送显示 */
                            if (pDispChn->uiEnable == SAL_TRUE)
                            {
                                s32Ret = disp_tskSendFrame(pDispChn, pstDupCopyBuff->pstDstSysFrame, pDispChn->stChnPrm.uiRotate);

                                if (s32Ret != SAL_SOK)
                                {
                                    DISP_LOGW("disp_tskSendFrame warning !\n");
                                }
                            }

                            SAL_mutexUnlock(pDispChn->mChnMutexHdl);

                            pstDupHandle->dupOps.OpDupPutBlit(pstDupHandle, pstDupCopyBuff);
                            stDupCopyPool.r_idx = (r_idx + 1) % DUP_MNG_FRM_CNT;
                            if (stDupCopyPool.new_cnt > 0)
                            {
                                stDupCopyPool.new_cnt--;
                            }
                            else
                            {
                                DISP_LOGW("warning stDupCopyPool.new_cnt %u ! \n", stDupCopyPool.new_cnt);
                            }

                            fps++;
                        }

                        pstDbgTime->u64TimeSendEnd = sal_get_tickcnt();
                    }
                    else
                    {
                        pstDbgTime->u64TimeGetEnd = sal_get_tickcnt();
                        pstDbgTime->u64TimeGetTimeOut = SAL_TRUE;

                        pstDbgTime->u64TimeSendStart = sal_get_tickcnt();
                        if (stDupCopyPool.new_cnt > 0)
                        {
                            r_idx = stDupCopyPool.r_idx;
                            pstDupCopyBuff = &(stDupCopyPool.stDupFrm[r_idx]);
                            SAL_mutexLock(pDispChn->mChnMutexHdl);
                            /* 送显示 */
                            if (pDispChn->uiEnable == SAL_TRUE)
                            {
                                s32Ret = disp_tskSendFrame(pDispChn, pstDupCopyBuff->pstDstSysFrame, pDispChn->stChnPrm.uiRotate);
                                if (s32Ret != SAL_SOK)
                                {
                                    DISP_LOGW("disp_tskSendFrame warning !\n");
                                }
                            }

                            SAL_mutexUnlock(pDispChn->mChnMutexHdl);

                            pstDupHandle->dupOps.OpDupPutBlit(pstDupHandle, pstDupCopyBuff);
                            stDupCopyPool.r_idx = (r_idx + 1) % DUP_MNG_FRM_CNT;
                            if (stDupCopyPool.new_cnt > 0)
                            {
                                stDupCopyPool.new_cnt--;
                            }
                            else
                            {
                                DISP_LOGW("warning stDupCopyPool.new_cnt %u ! \n", stDupCopyPool.new_cnt);
                            }

                            fps++;
                        }

                        pstDbgTime->u64TimeSendEnd = sal_get_tickcnt();
                    }
                }
                else if (NODE_BIND_TYPE_SDK_BIND == pstDupHandle->createFlags) /* 绑定模式 */
                {

                    usleep(500000); /* 防止造成死循环； */
                    DISP_LOGD("SDK_BIND mode, should not run here \n");
                }
            }
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_CAPT) /* vi capt */
            {
                /*调试时需要注意vpss通道是否正确*/
                pstDbgTime->u64TimeGetStart = sal_get_tickcnt();
                s32Ret = disp_tskGetVoChnDupFrame(voDev, pDispChn, &stGetPutFrame);
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGW("disp_tskSendFrame warning !\n");
                    continue;
                }

                pstDbgTime->u64TimeGetTimeOut = SAL_FALSE;
                pstDbgTime->u64TimeGetEnd = sal_get_tickcnt();

                SAL_mutexLock(pDispChn->mChnMutexHdl);
                /* 送显示 */
                pstDbgTime->u64TimeSendStart = sal_get_tickcnt();
                if (pDispChn->uiEnable == SAL_TRUE)
                {
                    s32Ret = disp_tskSendFrame(pDispChn, &stGetPutFrame, pDispChn->stChnPrm.uiRotate);
                    if (s32Ret != SAL_SOK)
                    {
                        DISP_LOGW("disp_tskSendFrame warning !\n");
                    }
                }

                pstDbgTime->u64TimeSendEnd = sal_get_tickcnt();
                SAL_mutexUnlock(pDispChn->mChnMutexHdl);

                disp_tskPutVoChnDupFrame(voDev, pDispChn, &stGetPutFrame);
            }
        }
        else
        {
            if (pDispChn->uiNoSignal == SAL_TRUE)
            {
                /* 参数根据status选择不同的无视频信号 */
                if(pDispChn->uiH == 0 || pDispChn->uiW == 0)
                {
                    DISP_LOGE("error  pDispChn->uiH:%d  pDispChn->uiW:%d\n",pDispChn->uiH,pDispChn->uiW);
                    continue ;
                }
                disp_tskGetNoSignalPic(0, &stSystemFrame);
                /*无视频信号画面临时缓存改为动态申请*/
                if(lastuiH != pDispChn->uiH || lastuiW != pDispChn->uiW)
                {
                    /*当通道宽高发生改变需重新申请缓存，申请前先将之前缓存释放*/
                    if (stSystemFrameResize.uiAppData != 0x00)
                    {
                        (VOID)disp_hal_putFrameMem(&stSystemFrameResize);
                        memset(&stSystemFrameResize, 0x00, sizeof(SYSTEM_FRAME_INFO));
                    }
                    if (stSystemFrameResize.uiAppData == 0x00)
                    {
                        lastuiW = SAL_align(pDispChn->uiW,64);
                        s32Ret = disp_hal_getFrameMem(lastuiW, pDispChn->uiH, &stSystemFrameResize);
                        if (s32Ret != SAL_SOK)
                        {
                            DISP_LOGE("disp_hal_getFrameMem error !\n");
                            lastuiH = 0;
                            lastuiW = 0;
                            continue;
                        }
                    }
                    lastuiH = pDispChn->uiH;
                    lastuiW = pDispChn->uiW;
                }
                s32Ret = vgs_hal_scaleFrame(&stSystemFrameResize, &stSystemFrame,
                                            pDispChn->uiW, pDispChn->uiH);

                LogoPicFresh = ((s32Ret == SAL_FAIL) ? 1 : 0);

                SAL_mutexLock(pDispChn->mChnMutexHdl);

                if (pDispChn->uiEnable == SAL_TRUE)
                {
                    s32Ret = disp_hal_sendFrame(pDispChn, (void *)stSystemFrameResize.uiAppData, 100);
                    if (s32Ret != SAL_SOK)
                    {
                        DISP_LOGW("disp_tskSendFrame warning !\n");
                    }
                }

                SAL_mutexUnlock(pDispChn->mChnMutexHdl);
                if (LogoPicFresh == 0)
                {
                    pDispChn->uiNoSignal = SAL_FALSE;
                }
                else
                {
                    pDispChn->uiNoSignal = SAL_TRUE; /* 再刷新一次logo */
                }

                DISP_LOGI("voDev %d voChn %d uiNoSignal\n", pDispChn->uiDevNo, pDispChn->uiChn);
            }

            pDispChn->srcStatus = 0;

            if (SAL_FALSE == pDispChn->uiEnable)
            {
                LogoPicFresh = 0;
                pDispChn->uiNoSignal = SAL_FALSE;
            }

            /* 用于线程同步使用，当应用调用disp_tskVoChnClearBuf时，需要对显示通道进行缓存清理 */
            if (0 == pDispChn->flag && LogoPicFresh == 0)
            {
                pDispChn->flag = 1;
            }

            SAL_msleep(18);
        }

        pstDbgTime->u64TimeExit = sal_get_tickcnt();
        GO_FORWARD(pDispChn->u32DbgTimeIdx, DISP_DEBUG_CHN_TIME_NUM);
    }

out1:
    for (idx = 0; idx < DUP_MNG_FRM_CNT; idx++)
    {
        if (stGetPutFrameInfo[idx].uiAppData != 0x00)
        {
            (VOID)sys_hal_rleaseVideoFrameInfoSt(&(stGetPutFrameInfo[idx]));
        }
    }

    if (stGetPutFrame.uiAppData != 0x00)
    {
        (VOID)sys_hal_rleaseVideoFrameInfoSt(&stGetPutFrame);
    }

out2:
    if (stSystemFrame.uiAppData != 0x00 && voChn == 0)
    {
        (VOID)disp_hal_putFrameMem(&stSystemFrame);
    }

out3:
    if (stSystemFrameResize.uiAppData != 0x00)
    {
        (VOID)disp_hal_putFrameMem(&stSystemFrameResize);
    }

    return NULL;
}

#else

/**
 * @function:   disp_dmaCopyToDispBuf
 * @brief:      DMA拷贝video frame到显示帧缓存，显示线程里会把显示帧缓存直接送给VO
 * @param[in]:  SYSTEM_FRAME_INFO *pSrcFrame，被拷贝的源帧信息
 * @param[in]:  SYSTEM_FRAME_INFO  *pDstFrame，显示帧
 * @param[in]:  TDE_HAL_RECT *pdstRect，拷贝到显示显示帧缓存的位置信息
 * @return:     INT32
 */
static INT32 disp_dmaCopyToDispBuf(SYSTEM_FRAME_INFO *pSrcFrame, SYSTEM_FRAME_INFO *pDstFrame, TDE_HAL_RECT *pdstRect)
{
    INT32 s32Ret = 0;

    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};
    SAL_VideoFrameBuf stDstVideoFrmBuf = {0};
    TDE_HAL_SURFACE srcSurface = {0};
    TDE_HAL_SURFACE dstSurface = {0};
    TDE_HAL_RECT srcRect = {0};
    TDE_HAL_RECT dstRect = {0};

    (VOID)sys_hal_getVideoFrameInfo(pSrcFrame, &stSrcVideoFrmBuf);
    (VOID)sys_hal_getVideoFrameInfo(pDstFrame, &stDstVideoFrmBuf);

    srcSurface.u32Width = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.u32Height = stSrcVideoFrmBuf.frameParam.height;
    srcSurface.u32Stride = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.PhyAddr = stSrcVideoFrmBuf.phyAddr[0];
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = stSrcVideoFrmBuf.frameParam.width;
    srcRect.u32Height = stSrcVideoFrmBuf.frameParam.height;

    dstSurface.u32Width = stDstVideoFrmBuf.frameParam.width;
    dstSurface.u32Height = stDstVideoFrmBuf.frameParam.height;
    dstSurface.u32Stride = stDstVideoFrmBuf.frameParam.width;
    dstSurface.PhyAddr = stDstVideoFrmBuf.phyAddr[0];
    dstRect.s32Xpos = pdstRect->s32Xpos;
    dstRect.s32Ypos = pdstRect->s32Ypos;
    dstRect.u32Width = pdstRect->u32Width;
    dstRect.u32Height = pdstRect->u32Height;

    s32Ret = tde_hal_DmaCopy(&srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    stDstVideoFrmBuf.frameNum = stSrcVideoFrmBuf.frameNum;
    stDstVideoFrmBuf.pts = stSrcVideoFrmBuf.pts;


    s32Ret = sys_hal_buildVideoFrame(&stDstVideoFrmBuf, pDstFrame);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   disp_dmaCopyToVideoPool
 * @brief:      DMA拷贝video frame到显示视频缓存池，除了0通道的其他通道从底层vpe获取帧信息之后把他们拷贝到视频缓存池，0通道主线程在送VO之前从缓存池拷贝过来进行拼帧
 * @param[in]:  SYSTEM_FRAME_INFO *pSrcFrame，被拷贝的源帧信息
 * @param[in]:  SYSTEM_FRAME_INFO  *pDstFrame，显示视频缓存池里的帧信息
 * @return:     INT32
 */
static INT32 disp_dmaCopyToVideoPool(SYSTEM_FRAME_INFO *pSrcFrame, SYSTEM_FRAME_INFO *pDstFrame)
{
    INT32 s32Ret = 0;

    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};
    SAL_VideoFrameBuf stDstVideoFrmBuf = {0};
    TDE_HAL_SURFACE srcSurface = {0};
    TDE_HAL_SURFACE dstSurface = {0};
    TDE_HAL_RECT srcRect = {0};
    TDE_HAL_RECT dstRect = {0};

    (VOID)sys_hal_getVideoFrameInfo(pSrcFrame, &stSrcVideoFrmBuf);
    (VOID)sys_hal_getVideoFrameInfo(pDstFrame, &stDstVideoFrmBuf);

    srcSurface.u32Width = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.u32Height = stSrcVideoFrmBuf.frameParam.height;
    srcSurface.u32Stride = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.PhyAddr = stSrcVideoFrmBuf.phyAddr[0];
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = stSrcVideoFrmBuf.frameParam.width;
    srcRect.u32Height = stSrcVideoFrmBuf.frameParam.height;

    /* dstSurface.u32Width = stDstVideoFrmBuf.frameParam.width; */
    /* dstSurface.u32Height = stDstVideoFrmBuf.frameParam.height; */
    /* dstSurface.u32Stride = stDstVideoFrmBuf.frameParam.width; */
    /* 拷贝到临时视频缓冲区时stDstVideoFrmBuf的宽高信息比源里的宽高信息大，因为视频缓冲区stDupCopyDispPool按照1080P分配一次，如果按照每个窗口大小分配就需要动态释放申请*/
    dstSurface.u32Width = stSrcVideoFrmBuf.frameParam.width;
    dstSurface.u32Height = stSrcVideoFrmBuf.frameParam.height;
    dstSurface.u32Stride = stSrcVideoFrmBuf.frameParam.width;
    dstSurface.PhyAddr = stDstVideoFrmBuf.phyAddr[0];
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = stSrcVideoFrmBuf.frameParam.width;
    dstRect.u32Height = stSrcVideoFrmBuf.frameParam.height;
    static INT32 count = 0;
    count++;
    if ((count % 300) > 0 && (count % 300) < 5)
    {
        SAL_DEBUG("src: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", srcSurface.PhyAddr, srcSurface.u32Width, srcSurface.u32Height, srcRect.s32Xpos, srcRect.s32Ypos, srcRect.u32Width, srcRect.u32Height);
        SAL_DEBUG(" dst: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", dstSurface.PhyAddr, dstSurface.u32Width, dstSurface.u32Height, dstRect.s32Xpos, dstRect.s32Ypos, dstRect.u32Width, dstRect.u32Height);
    }

    s32Ret = tde_hal_DmaCopy(&srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    stDstVideoFrmBuf.frameNum = stSrcVideoFrmBuf.frameNum;
    stDstVideoFrmBuf.pts = stSrcVideoFrmBuf.pts;

    /* 拷贝到拼接视频缓存池时，由于初始化时内存按照最大分配，所以需要把宽高信息也拷过来 */
    stDstVideoFrmBuf.frameParam.width = stSrcVideoFrmBuf.frameParam.width;
    stDstVideoFrmBuf.frameParam.height = stSrcVideoFrmBuf.frameParam.height;
    stDstVideoFrmBuf.stride[0] = stSrcVideoFrmBuf.stride[0];
    stDstVideoFrmBuf.stride[1] = stSrcVideoFrmBuf.stride[1];
    stDstVideoFrmBuf.virAddr[0] = pDstFrame->uiDataAddr;

    s32Ret = sys_hal_buildVideoFrame(&stDstVideoFrmBuf, pDstFrame);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   disp_dmaCopyToDispBuf
 * @brief:      DMA拷贝video frame到显示帧缓存，显示线程里会把显示帧缓存直接送给VO
 * @param[in]:  SYSTEM_FRAME_INFO *pSrcFrame，被拷贝的源帧信息
 * @param[in]:  SYSTEM_FRAME_INFO  *pDstFrame，显示帧
 * @param[in]:  TDE_HAL_RECT *pdstRect，拷贝到显示显示帧缓存的位置信息
 * @return:     INT32
 */
static INT32 disp_tdeCopyToDispBuf(SYSTEM_FRAME_INFO *pSrcFrame, SYSTEM_FRAME_INFO *pDstFrame, TDE_HAL_RECT *pdstRect)
{
    INT32 s32Ret = 0;

    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};
    SAL_VideoFrameBuf stDstVideoFrmBuf = {0};
    TDE_HAL_SURFACE srcSurface = {0};
    TDE_HAL_SURFACE dstSurface = {0};
    TDE_HAL_RECT srcRect = {0};
    TDE_HAL_RECT dstRect = {0};

    (VOID)sys_hal_getVideoFrameInfo(pSrcFrame, &stSrcVideoFrmBuf);
    (VOID)sys_hal_getVideoFrameInfo(pDstFrame, &stDstVideoFrmBuf);

    srcSurface.u32Width = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.u32Height = stSrcVideoFrmBuf.frameParam.height;
    srcSurface.u32Stride = stSrcVideoFrmBuf.frameParam.width;
    srcSurface.enColorFmt = stSrcVideoFrmBuf.frameParam.dataFormat;
    srcSurface.PhyAddr = stSrcVideoFrmBuf.phyAddr[0];
    srcSurface.vbBlk = stSrcVideoFrmBuf.vbBlk;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = pdstRect->u32Width;
    srcRect.u32Height = pdstRect->u32Height;

    dstSurface.u32Width = stDstVideoFrmBuf.frameParam.width;
    dstSurface.u32Height = stDstVideoFrmBuf.frameParam.height;
    dstSurface.u32Stride = stDstVideoFrmBuf.frameParam.width;
    dstSurface.enColorFmt = stDstVideoFrmBuf.frameParam.dataFormat;
    dstSurface.PhyAddr = stDstVideoFrmBuf.phyAddr[0];
    dstSurface.vbBlk = stDstVideoFrmBuf.vbBlk;
    dstRect.s32Xpos = pdstRect->s32Xpos;
    dstRect.s32Ypos = pdstRect->s32Ypos;
    dstRect.u32Width = pdstRect->u32Width;
    dstRect.u32Height = pdstRect->u32Height;

    if ((srcRect.u32Width > srcSurface.u32Width) || (srcRect.u32Height > srcSurface.u32Height))
    {
        srcRect.u32Width = srcSurface.u32Width;
        srcRect.u32Height = srcSurface.u32Height;
        dstRect.u32Width = srcRect.u32Width;
        dstRect.u32Height = srcRect.u32Height;
    }

    static INT32 count = 0;
    count++;
    if ((count % 300) > 0 && (count % 300) < 5)
    {
        SAL_DEBUG("src: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", srcSurface.PhyAddr, srcSurface.u32Width, srcSurface.u32Height, srcRect.s32Xpos, srcRect.s32Ypos, srcRect.u32Width, srcRect.u32Height);
        SAL_DEBUG(" dst: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", dstSurface.PhyAddr, dstSurface.u32Width, dstSurface.u32Height, dstRect.s32Xpos, dstRect.s32Ypos, dstRect.u32Width, dstRect.u32Height);
    }

    s32Ret = tde_hal_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, SAL_FALSE);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    get_dupCopyDispPoolRIdx
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 get_dupCopyDispPoolRIdx(DISP_CHN_COMMON *pDispChn)
{
    DUP_COPY_DISP_DATA_BUFF_POOL *pstDupCopyDispPool;
    UINT32 u32CopyPoolRIdx, i;
    INT32 idx = 0;

    pstDupCopyDispPool = &(stDupCopyDispPool[pDispChn->uiDevNo][pDispChn->uiChn]);
    u32CopyPoolRIdx = pstDupCopyDispPool->r;

    for (i = u32CopyPoolRIdx; i < u32CopyPoolRIdx + DISP_COPY_FRM_CNT; i++)
    {
        idx = i % DISP_COPY_FRM_CNT;
        if (idx == pstDupCopyDispPool->w)
        {
            DISP_LOGD("no new frm,r=w voDev %d, chn %d \n", pDispChn->uiDevNo, pDispChn->uiChn);
            return -1;
        }

        if (pstDupCopyDispPool->u32IsNew[idx] == 1)
        {

            break;
        }
    }

    if (i == (u32CopyPoolRIdx + DISP_COPY_FRM_CNT))
    {
        DISP_LOGW("no new frm, voDev %d, chn %d \n", pDispChn->uiDevNo, pDispChn->uiChn);
        return -1;
    }

    pstDupCopyDispPool->r = idx;
    return idx;
}

/* 显示缓存池要清掉，拼帧线程从该显示缓存池那帧，如果重新开启该通道先不清一下，会导致把上一次开启状态时的帧给拼上 */
static INT32 disp_cleardupCopyDispPool(DISP_CHN_COMMON *pDispChn)
{
    DUP_COPY_DISP_DATA_BUFF_POOL *pstDupCopyDispPool;
    UINT32 i;

    pstDupCopyDispPool = &(stDupCopyDispPool[pDispChn->uiDevNo][pDispChn->uiChn]);

    for (i = 0; i < DISP_COPY_FRM_CNT; i++)
    {
        if (pstDupCopyDispPool->u32IsNew[i] != 0)
        {

            pstDupCopyDispPool->u32IsNew[i] = 0;
        }
    }

    pstDupCopyDispPool->hasBeenUpdated = 0;

    return SAL_SOK;
}

/**
 * @function    disp_clearPushIntoVoNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 disp_clearPushIntoVoNum(UINT32 u32voDev)
{
    if (u32voDev >= DISP_MAX_DEV_NUM)
    {
        DISP_LOGE("voDev %u excceed %u \n", u32voDev, DISP_MAX_DEV_NUM);
        return SAL_FAIL;
    }

    g_aU64PushInNum[u32voDev] = 0;

    return SAL_SOK;
}

/**
 * @function   Disp_tskVoChnThread
 * @brief      显示通道任务线程
 * @param[in]  void *pPrm  通道属性
 * @param[out] None
 * @return     static void *
 */
static void *disp_tskVoChnThreadStitch(void *pPrm)
{
    INT32 s32Ret = 0;
    UINT32 voDev = 0;
    UINT32 voChn = 0;
    UINT32 vdecChn = 0;
    UINT32 LogoPicFresh = 0; /* hiklogo vgs缩放失败再刷新一帧 */
    INT32 iWaitCnt = 0;
    INT32 idx = 0, j = 0;
    UINT32 u32DispBufWIdx = 0;
    INT32 u32CopyPoolRIdx = 0;
    INT32 disp_vpss_mem_w = 0, disp_vpss_mem_h = 0;
    DUP_COPY_DATA_BUFF stDupCopyBuff;
    DUP_COPY_DATA_BUFF_POOL stDupCopyPool;
    DUP_DISP_DATA_BUFF_POOL *pstDupDispPool = NULL;
    DUP_COPY_DISP_DATA_BUFF_POOL *pstDupCopyDispPool = NULL;
    DUP_COPY_DATA_BUFF *pstDupCopyBuff = NULL;
    TDE_HAL_RECT stDstRect;

    SYSTEM_FRAME_INFO stDupCopySystemFrame;
    SYSTEM_FRAME_INFO stSystemFrame;
    SYSTEM_FRAME_INFO stGetPutFrame;
    SYSTEM_FRAME_INFO stGetPutFrameInfo[DUP_MNG_FRM_CNT];
    SYSTEM_FRAME_INFO stPullOutFrame; /* vendor_vo_pull_out时使用 */
    DUP_ChanHandle *pstDupHandle = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_CHN_COMMON *pDispStitchedChn = NULL; /* 被拼接的通道 */
    SYSTEM_FRAME_INFO stSystemFrameResize = {0};
    SYSTEM_FRAME_INFO *pdecSystemFrame = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    UINT32 u32Width = 0, u32Height = 0;
    UINT64 timeCtr0 = 0, timeCtr1 = 0;
    UINT64 time0 = 0, time1 = 0;
    UINT32 time2 = 0, time3 = 0;
    UINT32 fps = 0;
    UINT32 uiModId = SYSTEM_MOD_ID_VDEC;
    CAPB_DISP *capb_disp = NULL;
    DSPINITPARA *pDspInitPara = NULL;
    UINT32 needFree = 0;
    static UINT32 u32Count1 = 0, u32Count2 = 0;
    char thrdName[64] = {0};
    UINT32 u32Core = 1;
    BOOL vdecFrameStatus = SAL_TRUE;     /*获取失败状态标志。0通道失败需要继续送帧否者第二路无法显示*/
    INT32 dispFpMs = 0;
    pDspInitPara = SystemPrm_getDspInitPara();

    pDispChn = (DISP_CHN_COMMON *)pPrm;
    if (pDispChn == NULL)
    {
        DISP_LOGE("error !\n");
        return NULL;
    }

    voDev = pDispChn->uiDevNo;
    voChn = pDispChn->uiChn;

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return NULL;
    }

    /* 第一个屏主画面显示线程绑定到CPU1，第二个屏主画面绑定到CPU0，其他不绑定 */
    if ((0 == voDev) && (0 == voChn))
    {
        u32Core = 1;
        SAL_SetThreadCoreBind(u32Core);
    }
    else if ((1 == voDev) && (0 == voChn))
    {
        u32Core = 0;
        SAL_SetThreadCoreBind(u32Core);
    }
    else
    {
        ;
    }

    /* DISP_LOGI("VoDev %d VoChn %d %s Create %p!\n",voDev,voChn,__func__,pDispChn); */

    memset(&stDupCopyBuff, 0x00, sizeof(DUP_COPY_DATA_BUFF));
    memset(&stDupCopyPool, 0x00, sizeof(DUP_COPY_DATA_BUFF_POOL));
    memset(&stSystemFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));
    memset(&stSystemFrameResize, 0x00, sizeof(SYSTEM_FRAME_INFO));
    memset(&stDupCopySystemFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));

    disp_vpss_mem_w = (capb_disp->disp_yuv_w_max > 1920) ? capb_disp->disp_yuv_w_max : 1920;
    disp_vpss_mem_h = (capb_disp->disp_yuv_h > 1080) ? capb_disp->disp_yuv_h : 1080;

    if (stSystemFrameResize.uiAppData == 0x00)
    {
        s32Ret = disp_hal_getFrameMem(disp_vpss_mem_w, disp_vpss_mem_h, &stSystemFrameResize);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            return NULL;
        }

        gDispDebug[voDev][voChn].no_signal_frame = (void *)stSystemFrameResize.uiAppData;
    }

    pDispDev = disp_tskGetVoDev(voDev);
    if (!pDispDev)
    {
        DISP_LOGE("pDispDev NULL!\n");
        return NULL;
    }

    dispFpMs = 1000 / pDispDev->uiDevFps;
    u32Width = pDispDev->uiDevWith;
    u32Height = pDispDev->uiDevHeight;
    DISP_LOGI("vo dev(%u) will alloc disp mem, w * h: %u * %u !\n", voDev, u32Width, u32Height);

    if (pDispChn->uiChn == 0)
    {
        pstDupDispPool = &(stDupDispPool[pDispChn->uiDevNo]);
        for (idx = 0; idx < DISP_BUF_FRM_CNT; idx++)
        {
            if (pstDupDispPool->stSysFrm[idx].uiAppData == 0x00)
            {
                /* 显示缓存的分辨率需要跟VO输出分辨率一致 */
                s32Ret = disp_hal_getFrameMem(u32Width, u32Height, &(pstDupDispPool->stSysFrm[idx]));
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGE("disp_hal_getFrameMem error !\n");
                    return NULL;
                }

                /* 调试代码: 图片送去显示 */
                /* sprintf(filename, "./yuv/%u.yuv", idx % 3); */
                /* read_input(filename, (void *)pstDupDispPool->stSysFrm[idx].uiDataAddr,  pstDupDispPool->stSysFrm[idx].uiDataWidth * pstDupDispPool->stSysFrm[idx].uiDataHeight * 3/2); */
            }
        }
    }
    else
    {
        pstDupCopyDispPool = &(stDupCopyDispPool[pDispChn->uiDevNo][pDispChn->uiChn]);
        for (idx = 0; idx < DISP_COPY_FRM_CNT; idx++)
        {
            if (pstDupCopyDispPool->stSysFrm[idx].uiAppData == 0x00)
            {
                s32Ret = disp_hal_getFrameMem(u32Width, u32Height, &(pstDupCopyDispPool->stSysFrm[idx]));
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGE("disp_hal_getFrameMem error !\n");
                    return NULL;
                }
            }
        }
    }

    stDupCopyBuff.stDupDataCopyPrm.uiRotate = 0xFFFF;
    stDupCopyBuff.pstDstSysFrame = &stDupCopySystemFrame;
    if (stDupCopySystemFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stDupCopySystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("sys_hal_allocVideoFrameInfoSt error !\n");
            return NULL;
        }
    }

    if (stSystemFrame.uiAppData == 0x00)
    {
        s32Ret = disp_hal_getFrameMem(1920, 1080, &stSystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            goto out3;
        }

        s32Ret = disp_tskGetNoSignalPic(1, &stSystemFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            goto out2;
        }

        gDispDebug[voDev][voChn].no_signal_frame_before = (void *)stSystemFrame.uiAppData;
    }

    memset(&stGetPutFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));
    if (stGetPutFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stGetPutFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("sys_hal_allocVideoFrameInfoSt error !\n");
            goto out2;
        }

        gDispDebug[voDev][voChn].vo_frame = (void *)stGetPutFrame.uiAppData;
    }

    memset(&stGetPutFrameInfo, 0x00, sizeof(stGetPutFrameInfo));
    for (idx = 0; idx < DUP_MNG_FRM_CNT; idx++)
    {
        stDupCopyPool.stDupFrm[idx].stDupDataCopyPrm.uiRotate = 0xFFFF;
        stDupCopyPool.stDupFrm[idx].pstDstSysFrame = &(stGetPutFrameInfo[idx]);
        if (stGetPutFrameInfo[idx].uiAppData == 0x00)
        {
            s32Ret = sys_hal_allocVideoFrameInfoSt(&(stGetPutFrameInfo[idx]));
            if (s32Ret != SAL_SOK)
            {
                DISP_LOGE("sys_hal_allocVideoFrameInfoSt error !\n");
                goto out1;
            }
        }
    }

    memset(&stPullOutFrame, 0x00, sizeof(SYSTEM_FRAME_INFO));
    if (stPullOutFrame.uiAppData == 0x00)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(&stPullOutFrame);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("disp_hal_getFrameMem error !\n");
            goto out1;
        }
    }

    snprintf(thrdName, sizeof(thrdName), "dispstitch_%d_%d", pDispChn->uiDevNo, pDispChn->uiChn);
    prctl(PR_SET_NAME, (unsigned long)thrdName);
    SAL_msleep(5000);

    while (1)
    {
        iWaitCnt = 200;
        while (DISP_CLEAN_STATUS_START == pDispChn->enCleanStatus)
        {
            if (pDispChn->stInModule.uiUseFlag == SAL_FALSE)
            {
                pDispChn->flag = 1;
            }

            SAL_msleep(10);
            iWaitCnt--;
            if (iWaitCnt < 0)
            {
                DISP_LOGE("chn[%u] waiting for screen clean timeout\n", voChn);
                break;
            }
        }

        if (pDispChn->stInModule.uiModId != uiModId)
        {
            uiModId = pDispChn->stInModule.uiModId;
            if (uiModId == SYSTEM_MOD_ID_DUP)
            {
                SAL_thrChangePri(&pDispChn->stChnThrHandl, SAL_THR_PRI_MAX - 5);
            }
            else
            {
                SAL_thrChangePri(&pDispChn->stChnThrHandl, SAL_THR_PRI_DEFAULT);
            }
        }

        if ((time1 - time0 >= 990) && (fps > 0) && (voChn == 0))
        {
            DISP_LOGD("voDev %d,fps %d,%llu\n", voDev, fps, time1 - time0);
            if ((fps < 50) && (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP))
            {
                DISP_LOGD("voDev %d,fps %d,%llu\n", voDev, fps, time1 - time0);
            }

            fps = 0;
        }

        if (fps == 0)
        {
            /*和sva模块一样统一使用系统时间，调试有问题再切换*/
            time0 = disp_tskGetTimeMilli();

            /* s32Ret = HI_MPI_SYS_GetCurPTS(&time0);
               if (s32Ret != 0)
               {
                 time0 = disp_tskGetTimeMilli();
                 DISP_LOGW("error\n");
               }
               else
               {
                 time0 = (time0 / 1000);
               }*/
        }

        time1 = disp_tskGetTimeMilli();

        /*s32Ret = HI_MPI_SYS_GetCurPTS(&time1);
           if (s32Ret != 0)
           {
            time1 = disp_tskGetTimeMilli();
            DISP_LOGW("error\n");
           }
           else
           {
            time1 = (time1 / 1000);
           }*/

        /* 1、等待通道被启用 */
        disp_tskVoChnWait(pDispChn);
        /* 智能信息闪烁 */

        if ((pDspInitPara->boardType >= SECURITY_MACHINE_START) && (pDspInitPara->boardType <= SECURITY_MACHINE_END))
        {
            disp_osdFlickCheck(pDispChn);
        }

        /* 按照1s 60帧计算 当当前帧是消失帧且下一帧最多是消失帧的最后一帧且闪烁次数超过1000次，更新基准时间 */



        /* 输入模块被启用 */
        if (pDispChn->stInModule.uiUseFlag == SAL_TRUE)
        {
            /* 中间过段状态，是为了重置显示通道，或者发送无视频信号视频 */
            if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_VDEC)
            {
                vdecChn = (UINT32)pDispChn->stInModule.uiHandle;


                /* 复用 */
                pDispChn->stChnPrm.uiRotate = 0;
                vdecFrameStatus = SAL_TRUE;
                s32Ret = vdec_tsk_GetVdecFrame(vdecChn, voDev, VDEC_DISP_GET_FRAME, &needFree, &stGetPutFrame, &pDispChn->stChnPrm.uiRotate);
                if (s32Ret != SAL_SOK)
                {
                    SAL_msleep(10);
                    DISP_LOGD("vdecCh %d,voDev %d\n", vdecChn, voDev);

                    /*0通道失败需要继续送帧否者第二路无法显示*/
                    if (pDispChn->uiChn != 0)
                    {
                        continue;
                    }

                    vdecFrameStatus = SAL_FALSE;
                }

                if (pDispChn->stInModule.uiUseFlag)
                {
                    if (pDispChn->srcStatus == pDispChn->stChnPrm.uiRotate && pDispChn->stChnPrm.uiRotate == 1)
                    {
                        SAL_msleep(10);
                        DISP_LOGD("vdecCh %u,voDev %u,srcStatus %u\n", vdecChn, voDev, pDispChn->srcStatus);
                        /* continue;   / *拼帧需要继续显示否者vochn为0时不再继续显示其他通道数据* / */
                    }

                    DISP_LOGD("vdecCh %u,voDev %u,srcStatus %u\n", vdecChn, voDev, pDispChn->srcStatus);
                    pDispChn->srcStatus = pDispChn->stChnPrm.uiRotate;
                    SAL_mutexLock(pDispChn->mChnMutexHdl);
                    /* 送显示 */
                    if (pDispChn->uiEnable == SAL_TRUE)
                    {
                        if (pDispChn->stChnPrm.uiRotate == 2)
                        {

                            vgs_hal_scaleFrame(&stSystemFrameResize, &stGetPutFrame,
                                               pDispChn->uiW, pDispChn->uiH);
                            pdecSystemFrame = &stSystemFrameResize;
                            DISP_LOGD("disp_tskSendFrame no video pic (w:%d,h:%d) needFree= %d!\n", pDispChn->uiW, pDispChn->uiH, needFree);

                        }
                        else
                        {
                            pdecSystemFrame = &stGetPutFrame;
                        }

                        if (pDispChn->uiChn != 0)
                        {
                            pstDupCopyDispPool = &(stDupCopyDispPool[pDispChn->uiDevNo][pDispChn->uiChn]);
                            u32DispBufWIdx = pstDupCopyDispPool->w;
                            s32Ret = disp_dmaCopyToVideoPool(pdecSystemFrame, &(pstDupCopyDispPool->stSysFrm[u32DispBufWIdx]));
                            if (s32Ret != SAL_SOK)
                            {
                                DISP_LOGW("disp_quickCopyVideo error !\n");
                                u32DispBufWIdx = (u32DispBufWIdx + DISP_COPY_FRM_CNT - 1) % DISP_COPY_FRM_CNT;
                            }
                            else
                            {
                                pstDupCopyDispPool->u32IsNew[u32DispBufWIdx] = 1;
                                pstDupCopyDispPool->w = (pstDupCopyDispPool->w + 1) % DISP_COPY_FRM_CNT;
                                pstDupCopyDispPool->hasBeenUpdated = 1;  /* 开启显示通道之后从底层获取到一帧之后就置1 */
                            }
                        }
                        else
                        {

                            pstDupDispPool = &(stDupDispPool[pDispChn->uiDevNo]);
                            u32DispBufWIdx = pstDupDispPool->w;
                            pDispStitchedChn = disp_tskGetVoChn(pDispChn->uiDevNo, pDispChn->uiChn);
                            /* u32DispBufWIdx = (u32Count++ / 500) % DISP_BUF_FRM_CNT; */
                            DISP_LOGD("pDispChn chn %d [x,y,w,h][%u,%u,%u,%u], w           %d \n", pDispChn->uiChn, pDispChn->uiX, pDispChn->uiY, pDispChn->uiW, pDispChn->uiH, u32DispBufWIdx);
                            time2 = SAL_getCurMs();
                            if (vdecFrameStatus)
                            {
                                stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                stDstRect.u32Width = pDispStitchedChn->uiW;
                                stDstRect.u32Height = pDispStitchedChn->uiH;
                                s32Ret = disp_tdeCopyToDispBuf(pdecSystemFrame, &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                            }

                            time3 = SAL_getCurMs();
                            u32Count1++;
                            if ((0 < (u32Count1 % 1000)) && ((u32Count1 % 1000) <= 3) && 0)
                            {
                                DISP_LOGW("disp_dmaCopyToDispBuf spent time %u, u32Count %u\n", time3 - time2, u32Count1);
                            }

                            if (s32Ret != SAL_SOK)
                            {
                                DISP_LOGD("disp_quickCopyVideo error !\n");
                                u32DispBufWIdx = (u32DispBufWIdx + DISP_BUF_FRM_CNT - 1) % DISP_BUF_FRM_CNT;
                            }
                            else
                            {
                                pstDupDispPool->w = (u32DispBufWIdx + 1) % DISP_BUF_FRM_CNT;
                            }

                            /* DISP_LOGW("u32DispBufWIdx      %u !\n", u32DispBufWIdx); */

                            for (j = 1; j < disp_tskGetVoChnCnt(pDispChn->uiDevNo); j++)
                            {
                                pDispStitchedChn = disp_tskGetVoChn(pDispChn->uiDevNo, j);
                                if (pDispStitchedChn == NULL || SAL_TRUE != pDispStitchedChn->uiEnable)
                                {
                                    continue;
                                }

                                pstDupCopyDispPool = &(stDupCopyDispPool[pDispStitchedChn->uiDevNo][pDispStitchedChn->uiChn]);
                                u32CopyPoolRIdx = get_dupCopyDispPoolRIdx(pDispStitchedChn);
                                if ((u32CopyPoolRIdx >= 0) && (u32CopyPoolRIdx < DISP_COPY_FRM_CNT))
                                {
                                    stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                    stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                    stDstRect.u32Width = pDispStitchedChn->uiW;
                                    stDstRect.u32Height = pDispStitchedChn->uiH;

                                    DISP_LOGD("pDispStitchedChn voDev %d chn %d [x,y,w,h][%u,%u,%u,%u] \n", pDispStitchedChn->uiDevNo, pDispStitchedChn->uiChn, pDispStitchedChn->uiX, pDispStitchedChn->uiY, pDispStitchedChn->uiW, pDispStitchedChn->uiH);

                                    s32Ret = disp_tdeCopyToDispBuf(&(pstDupCopyDispPool->stSysFrm[u32CopyPoolRIdx]), &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                                    pstDupCopyDispPool->u32IsNew[u32CopyPoolRIdx] = 0;
                                    pstDupCopyDispPool->r = (u32CopyPoolRIdx + 1) % DISP_COPY_FRM_CNT;

                                }
                                else
                                {
                                    /*当未检测到数据获取上一帧数据送显示*/
                                    u32CopyPoolRIdx = (pstDupCopyDispPool->w + DISP_COPY_FRM_CNT - 1) % DISP_COPY_FRM_CNT;
                                    stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                    stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                    stDstRect.u32Width = pDispStitchedChn->uiW;
                                    stDstRect.u32Height = pDispStitchedChn->uiH;
                                    s32Ret = disp_tdeCopyToDispBuf(&(pstDupCopyDispPool->stSysFrm[u32CopyPoolRIdx]), &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);

                                }
                            }

                            if (pDispChn->uiChn != 0)
                            {
                                DISP_LOGW("disp_tskSendFrame warning,  chn       %d !\n", pDispChn->uiChn);
                            }

                            if ((0 < (u32Count1 % 1000)) && ((u32Count1 % 1000) <= 3) && 0)
                            {
                                DISP_LOGW("disp_tskSendFrame VoDev %d, u32DispBufWIdx %u,        dataVaddr %llx!\n", pDispChn->uiDevNo, u32DispBufWIdx, pstDupDispPool->stSysFrm[u32DispBufWIdx].uiDataAddr);
                            }

                            s32Ret = disp_tskSendFrame(pDispChn, &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), pDispChn->stChnPrm.uiRotate);
                            if (s32Ret != SAL_SOK)
                            {
                                DISP_LOGW("disp_tskSendFrame warning !\n");
                            }

                            g_aU64PushInNum[pDispChn->uiDevNo]++;
                            if (g_aU64PushInNum[pDispChn->uiDevNo] > (DISP_BUF_FRM_CNT - 2))
                            {
                                time2 = SAL_getCurMs();
                                disp_hal_pullOutFrame(pDispChn, (void *)(stPullOutFrame.uiAppData), 30);
                                time3 = SAL_getCurMs();
                                u32Count2++;
                                /* if ((0<(u32Count2 % 1000)) && ((u32Count2 % 1000)<=4) ) */
                                if ((time3 - time2) > 20)
                                {
                                    DISP_LOGW("disp_hal_pullOutFrame spent time %u, u32Count %u\n", time3 - time2, u32Count2);
                                }

                                g_aU64PushInNum[pDispChn->uiDevNo]--;
                            }
                        }
                    }

                    SAL_mutexUnlock(pDispChn->mChnMutexHdl);
                }

                if (needFree)
                {
                    vdec_tsk_PutVdecFrame(vdecChn, voDev, pdecSystemFrame);
                }

                SAL_msleep(10); /* 这里睡眠主要是减少从vpe获取不到时查询的概率 */

            }
            /* 8、输入模块是DUP(VI) */
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
            {
                timeCtr0 = SAL_getCurMs();
                pDispChn->srcStatus = 0;
                pstDupHandle = (DUP_ChanHandle *)pDispChn->stInModule.uiHandle;
                if (pstDupHandle == NULL)
                {
                    DISP_LOGW("pstDupHandle %p,voChn %d\n", pstDupHandle, voChn);
                    SAL_msleep(10);
                    continue;
                }

                /* 通过DUP的号码，获取DUP信息，包括通道的handle。 */
                /* 通过通道handle信息，获取vpss对应通道数据 */
                if (NODE_BIND_TYPE_GET == pstDupHandle->createFlags)
                {
                    pstDupCopyBuff = &stDupCopyBuff;
                    if (SAL_SOK == pstDupHandle->dupOps.OpDupGetBlit(pstDupHandle, pstDupCopyBuff))
                    {
                        SAL_mutexLock(pDispChn->mChnMutexHdl);
                        if (pDispChn->uiEnable == SAL_TRUE)
                        {
                            /*vo chn 0作为主画面，chn 0从vpe获取图像帧，然后从其他窗口的临时缓冲区stDupCopyDispPool获取图像帧叠上去, 最后送显 */
                            /* 从vpe拷贝到送显主画面只拷贝一次，主要是DMA/GFX性能都比较紧张，只拷贝一次的方案在双路双屏里可节省120fps的数据性能 */
                            if (pDispChn->uiChn == 0)
                            {
                                stDstRect.s32Xpos = pDispChn->uiX;
                                stDstRect.s32Ypos = pDispChn->uiY;
                                stDstRect.u32Width = pDispChn->uiW;
                                stDstRect.u32Height = pDispChn->uiH;
                                pstDupDispPool = &(stDupDispPool[pDispChn->uiDevNo]);
                                u32DispBufWIdx = pstDupDispPool->w;
                                /* u32DispBufWIdx = (u32Count++ / 500) % DISP_BUF_FRM_CNT; */
                                DISP_LOGD("pDispChn chn %d [x,y,w,h][%u,%u,%u,%u], w           %d \n", pDispChn->uiChn, pDispChn->uiX, pDispChn->uiY, pDispChn->uiW, pDispChn->uiH, u32DispBufWIdx);

                                time2 = SAL_getCurMs();
                                s32Ret = disp_tdeCopyToDispBuf(pstDupCopyBuff->pstDstSysFrame, &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                                time3 = SAL_getCurMs();
                                u32Count1++;
                                if ((0 < (u32Count1 % 1000)) && ((u32Count1 % 1000) <= 3) && 0)
                                {
                                    DISP_LOGW("disp_dmaCopyToDispBuf spent time %u, u32Count %u\n", time3 - time2, u32Count1);
                                }

                                if (s32Ret != SAL_SOK)
                                {
                                    DISP_LOGW("disp_quickCopyVideo error !\n");
                                    /* dma拷贝失败时，把上一帧送去显示，pstDupDispPool->w不需要更新 */
                                    u32DispBufWIdx = (u32DispBufWIdx + DISP_BUF_FRM_CNT - 1) % DISP_BUF_FRM_CNT;
                                }
                                else
                                {
                                    pstDupDispPool->w = (u32DispBufWIdx + 1) % DISP_BUF_FRM_CNT;
                                }

                                /* DISP_LOGW("u32DispBufWIdx      %u !\n", u32DispBufWIdx); */

                                for (j = 1; j < disp_tskGetVoChnCnt(pDispChn->uiDevNo); j++)
                                {
                                    pDispStitchedChn = disp_tskGetVoChn(pDispChn->uiDevNo, j);
                                    if (pDispStitchedChn == NULL || SAL_TRUE != pDispStitchedChn->uiEnable)
                                    {
                                        continue;
                                    }

                                    /* 宽度相等与主画面ch 0相同，说明是第二路输入的安检画面，用DMA拷贝(只支持一维)，二维用gfx拷贝 */
                                    if (pDispStitchedChn->uiW == pDispChn->uiW)
                                    {
                                        pstDupCopyDispPool = &(stDupCopyDispPool[pDispStitchedChn->uiDevNo][pDispStitchedChn->uiChn]);
                                        u32CopyPoolRIdx = get_dupCopyDispPoolRIdx(pDispStitchedChn);

                                        if ((u32CopyPoolRIdx >= 0) && (u32CopyPoolRIdx < DISP_COPY_FRM_CNT))
                                        {
                                            stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                            stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                            stDstRect.u32Width = pDispStitchedChn->uiW;
                                            stDstRect.u32Height = pDispStitchedChn->uiH;
                                            DISP_LOGD("pDispStitchedChn chn %d [x,y,w,h][%u,%u,%u,%u] \n", pDispStitchedChn->uiChn, pDispStitchedChn->uiX, pDispStitchedChn->uiY, pDispStitchedChn->uiW, pDispStitchedChn->uiH);
                                            s32Ret = disp_dmaCopyToDispBuf(&(pstDupCopyDispPool->stSysFrm[u32CopyPoolRIdx]), &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                                            pstDupCopyDispPool->u32IsNew[u32CopyPoolRIdx] = 0;
                                            pstDupCopyDispPool->r = (u32CopyPoolRIdx + 1) % DISP_COPY_FRM_CNT;
                                        }
                                        else
                                        {
                                            if (pstDupCopyDispPool->hasBeenUpdated != 0) /* 该显示通道重新开启之后有从底层拿到帧之后才能拼老的帧，否则会把该通道上一次使能的残留帧给拼上 */
                                            {
                                                /*当未检测到数据获取上一帧数据送显示*/
                                                u32CopyPoolRIdx = (pstDupCopyDispPool->w + DISP_COPY_FRM_CNT - 1) % DISP_COPY_FRM_CNT;
                                                stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                                stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                                stDstRect.u32Width = pDispStitchedChn->uiW;
                                                stDstRect.u32Height = pDispStitchedChn->uiH;
                                                s32Ret = disp_dmaCopyToDispBuf(&(pstDupCopyDispPool->stSysFrm[u32CopyPoolRIdx]), &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                                            }
                                        }
                                    }
                                    else /* 二维用gfx拷贝 */
                                    {
                                        pstDupCopyDispPool = &(stDupCopyDispPool[pDispStitchedChn->uiDevNo][pDispStitchedChn->uiChn]);
                                        u32CopyPoolRIdx = get_dupCopyDispPoolRIdx(pDispStitchedChn);
                                        if ((u32CopyPoolRIdx >= 0) && (u32CopyPoolRIdx < DISP_COPY_FRM_CNT))
                                        {
                                            stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                            stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                            stDstRect.u32Width = pDispStitchedChn->uiW;
                                            stDstRect.u32Height = pDispStitchedChn->uiH;

                                            DISP_LOGD("pDispStitchedChn voDev %d chn %d [x,y,w,h][%u,%u,%u,%u] \n", pDispStitchedChn->uiDevNo, pDispStitchedChn->uiChn, pDispStitchedChn->uiX, pDispStitchedChn->uiY, pDispStitchedChn->uiW, pDispStitchedChn->uiH);

                                            s32Ret = disp_tdeCopyToDispBuf(&(pstDupCopyDispPool->stSysFrm[u32CopyPoolRIdx]), &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                                            pstDupCopyDispPool->u32IsNew[u32CopyPoolRIdx] = 0;
                                            pstDupCopyDispPool->r = (u32CopyPoolRIdx + 1) % DISP_COPY_FRM_CNT;
                                        }
                                        else
                                        {
                                            if (pstDupCopyDispPool->hasBeenUpdated != 0) /* 该显示通道重新开启之后有从底层拿到帧之后才能拼老的帧，否则会把该通道上一次使能的残留帧给拼上 */
                                            {
                                                /*当未检测到数据获取上一帧数据送显示*/
                                                u32CopyPoolRIdx = (pstDupCopyDispPool->w + DISP_COPY_FRM_CNT - 1) % DISP_COPY_FRM_CNT;
                                                stDstRect.s32Xpos = pDispStitchedChn->uiX;
                                                stDstRect.s32Ypos = pDispStitchedChn->uiY;
                                                stDstRect.u32Width = pDispStitchedChn->uiW;
                                                stDstRect.u32Height = pDispStitchedChn->uiH;
                                                s32Ret = disp_tdeCopyToDispBuf(&(pstDupCopyDispPool->stSysFrm[u32CopyPoolRIdx]), &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), &stDstRect);
                                            }
                                        }
                                    }
                                }

                                if (pDispChn->uiChn != 0)
                                {
                                    DISP_LOGW("disp_tskSendFrame warning,  chn       %d !\n", pDispChn->uiChn);
                                }

                                if ((0 < (u32Count1 % 1000)) && ((u32Count1 % 1000) <= 3) && 0)
                                {
                                    DISP_LOGW("disp_tskSendFrame VoDev %d, u32DispBufWIdx %u,        dataVaddr %llx!\n", pDispChn->uiDevNo, u32DispBufWIdx, pstDupDispPool->stSysFrm[u32DispBufWIdx].uiDataAddr);
                                }

                                s32Ret = disp_tskSendFrame(pDispChn, &(pstDupDispPool->stSysFrm[u32DispBufWIdx]), pDispChn->stChnPrm.uiRotate);
                                if (s32Ret != SAL_SOK)
                                {
                                    DISP_LOGW("disp_tskSendFrame warning !\n");
                                }

                                g_aU64PushInNum[pDispChn->uiDevNo]++;
                                if (g_aU64PushInNum[pDispChn->uiDevNo] > (DISP_BUF_FRM_CNT - 2))
                                {
                                    time2 = SAL_getCurMs();
                                    disp_hal_pullOutFrame(pDispChn, (void *)(stPullOutFrame.uiAppData), 30);
                                    time3 = SAL_getCurMs();
                                    u32Count2++;
                                    /* if ((0<(u32Count2 % 1000)) && ((u32Count2 % 1000)<=4) ) */
                                    if ((time3 - time2) > 20)
                                    {
                                        DISP_LOGW("disp_hal_pullOutFrame spent time %u, u32Count %u\n", time3 - time2, u32Count2);
                                    }

                                    g_aU64PushInNum[pDispChn->uiDevNo]--;
                                }
                            }
                            else
                            {
                                pstDupCopyDispPool = &(stDupCopyDispPool[pDispChn->uiDevNo][pDispChn->uiChn]);
                                u32DispBufWIdx = pstDupCopyDispPool->w;
                                s32Ret = disp_dmaCopyToVideoPool(pstDupCopyBuff->pstDstSysFrame, &(pstDupCopyDispPool->stSysFrm[u32DispBufWIdx]));
                                if (s32Ret != SAL_SOK)
                                {
                                    DISP_LOGW("disp_quickCopyVideo error !\n");
                                    u32DispBufWIdx = (u32DispBufWIdx + DISP_COPY_FRM_CNT - 1) % DISP_COPY_FRM_CNT;
                                }
                                else
                                {
                                    pstDupCopyDispPool->u32IsNew[u32DispBufWIdx] = 1;
                                    pstDupCopyDispPool->w = (pstDupCopyDispPool->w + 1) % DISP_COPY_FRM_CNT;

                                    pstDupCopyDispPool->hasBeenUpdated = 1; /* 开启显示通道之后从底层获取到一帧之后就置1 */
                                }
                            }
                        }

                        SAL_mutexUnlock(pDispChn->mChnMutexHdl);

                        pstDupHandle->dupOps.OpDupPutBlit(pstDupHandle, pstDupCopyBuff);

                        fps++;

                    }
                    else
                    {
                        ;
                    }
                    
                    timeCtr1 = SAL_getCurMs() - timeCtr0;
                    if(timeCtr1 < dispFpMs)
                    {
                        if(dispFpMs-timeCtr1 > 4)//延时精确度不高，可能导致超过预期延时时间导致卡顿，预留一定时间
                        SAL_msleep(dispFpMs - timeCtr1 -4);   /*用于帧率控制*/
                    }
                    
                }
                else if (NODE_BIND_TYPE_SDK_BIND == pstDupHandle->createFlags)
                {

                    s32Ret = disp_tskGetVoChnDupFrame(voDev, pDispChn, &stGetPutFrame);
                    if (s32Ret != SAL_SOK)
                    {
                        DISP_LOGW("disp_tskSendFrame warning !\n");
                        continue;
                    }

                    SAL_mutexLock(pDispChn->mChnMutexHdl);
                    /* 送显示 */
                    if (pDispChn->uiEnable == SAL_TRUE)
                    {
                        if (pDispChn->uiChn != 0)
                        {
                            DISP_LOGW("disp_tskSendFrame warning,  chn       %d !\n", pDispChn->uiChn);
                        }

                        s32Ret = disp_tskSendFrame(pDispChn, &stGetPutFrame, pDispChn->stChnPrm.uiRotate);
                        if (s32Ret != SAL_SOK)
                        {
                            DISP_LOGW("disp_tskSendFrame warning !\n");
                        }
                    }

                    SAL_mutexUnlock(pDispChn->mChnMutexHdl);

                    disp_tskPutVoChnDupFrame(voDev, pDispChn, &stGetPutFrame);
                }
            }
            else if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_CAPT)
            {

                /*调试时需要注意vpss通道是否正确*/
                s32Ret = disp_tskGetVoChnDupFrame(voDev, pDispChn, &stGetPutFrame);
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGW("disp_tskSendFrame warning !\n");
                    continue;
                }

                SAL_mutexLock(pDispChn->mChnMutexHdl);
                /* 送显示 */
                if (pDispChn->uiEnable == SAL_TRUE)
                {
                    s32Ret = disp_tskSendFrame(pDispChn, &stGetPutFrame, pDispChn->stChnPrm.uiRotate);
                    if (s32Ret != SAL_SOK)
                    {
                        DISP_LOGW("disp_tskSendFrame warning !\n");
                    }
                }

                SAL_mutexUnlock(pDispChn->mChnMutexHdl);

                disp_tskPutVoChnDupFrame(voDev, pDispChn, &stGetPutFrame);

            }
        }
        else
        {
            if (pDispChn->uiNoSignal == SAL_TRUE)
            {
                /* 参数根据status选择不同的无视频信号 */
                disp_tskGetNoSignalPic(0, &stSystemFrame);

                s32Ret = vgs_hal_scaleFrame(&stSystemFrameResize, &stSystemFrame,
                                            pDispChn->uiW, pDispChn->uiH);

                LogoPicFresh = ((s32Ret == SAL_FAIL) ? 1 : 0);

                SAL_mutexLock(pDispChn->mChnMutexHdl);

                if (pDispChn->uiEnable == SAL_TRUE)
                {
#ifdef DISP_VIDEO_FUSION
                    if (pDispChn->uiChn == 0)
                    {
                        /*NT通道0底层固定1080P，当前无法适配需要固定*/
                        s32Ret = vgs_hal_scaleFrame(&stSystemFrameResize, &stSystemFrame,
                                                    1920, 1080);

                        s32Ret = disp_hal_sendFrame(pDispChn, (void *)stSystemFrameResize.uiAppData, 100);
                        if (s32Ret != SAL_SOK)
                        {
                            DISP_LOGW("disp_tskSendFrame warning !\n");
                        }

                        g_aU64PushInNum[pDispChn->uiDevNo]++;
                        if (g_aU64PushInNum[pDispChn->uiDevNo] > (DISP_BUF_FRM_CNT - 2))
                        {
                            time2 = SAL_getCurMs();
                            disp_hal_pullOutFrame(pDispChn, (void *)(stPullOutFrame.uiAppData), 30);
                            time3 = SAL_getCurMs();
                            u32Count2++;
                            /* if ((0<(u32Count2 % 1000)) && ((u32Count2 % 1000)<=4) ) */
                            if ((time3 - time2) > 20)
                            {
                                DISP_LOGW("disp_hal_pullOutFrame spent time %u, u32Count %u\n", time3 - time2, u32Count2);
                            }

                            g_aU64PushInNum[pDispChn->uiDevNo]--;
                        }
                    }
                    else
                    {
                        pstDupCopyDispPool = &(stDupCopyDispPool[pDispChn->uiDevNo][pDispChn->uiChn]);
                        u32DispBufWIdx = pstDupCopyDispPool->w;
                        s32Ret = disp_dmaCopyToVideoPool(&stSystemFrameResize, &(pstDupCopyDispPool->stSysFrm[u32DispBufWIdx]));
                        if (s32Ret != SAL_SOK)
                        {

                            u32DispBufWIdx = (u32DispBufWIdx + DISP_COPY_FRM_CNT - 1) % DISP_COPY_FRM_CNT;
                            DISP_LOGW("disp_quickCopyVideo error u32DispBufWIdx = %u !\n", u32DispBufWIdx);
                        }
                        else
                        {
                            pstDupCopyDispPool->u32IsNew[u32DispBufWIdx] = 1;
                            pstDupCopyDispPool->w = (pstDupCopyDispPool->w + 1) % DISP_COPY_FRM_CNT;
                            pstDupCopyDispPool->hasBeenUpdated = 1; /* 开启显示通道之后从底层获取到一帧之后就置1 */
                        }
                    }

#else
                    s32Ret = disp_hal_sendFrame(pDispChn, (void *)stSystemFrameResize.uiAppData, -1);
                    if (s32Ret != SAL_SOK)
                    {
                        DISP_LOGW("disp_tskSendFrame warning !\n");
                    }

#endif


                }

                SAL_mutexUnlock(pDispChn->mChnMutexHdl);
                if (LogoPicFresh == 0)
                {
                    pDispChn->uiNoSignal = SAL_FALSE;
                }
                else
                {
                    pDispChn->uiNoSignal = SAL_TRUE; /* 再刷新一次logo */
                }

                DISP_LOGI("voDev %d voChn %d uiNoSignal\n", pDispChn->uiDevNo, pDispChn->uiChn);
            }

            pDispChn->srcStatus = 0;

            if (SAL_FALSE == pDispChn->uiEnable)
            {
                LogoPicFresh = 0;
                pDispChn->uiNoSignal = SAL_FALSE;
            }

            /* 用于线程同步使用，当应用调用disp_tskVoChnClearBuf时，需要对显示通道进行缓存清理 */
            if (0 == pDispChn->flag && LogoPicFresh == 0)
            {
                pDispChn->flag = 1;
            }

            SAL_msleep(18); /* 防止无视频信号一直在送 */
        }

        /* SAL_msleep(300); */
        /* SAL_msleep(12); */
    }

out1:
    for (idx = 0; idx < DUP_MNG_FRM_CNT; idx++)
    {
        if (stGetPutFrameInfo[idx].uiAppData != 0x00)
        {
            (VOID)sys_hal_rleaseVideoFrameInfoSt(&(stGetPutFrameInfo[idx]));
        }
    }

    if (stGetPutFrame.uiAppData != 0x00)
    {
        (VOID)sys_hal_rleaseVideoFrameInfoSt(&stGetPutFrame);
    }

out2:
    if (stSystemFrame.uiAppData != 0x00)
    {
        (VOID)disp_hal_putFrameMem(&stSystemFrame);
    }

out3:
    if (stSystemFrameResize.uiAppData != 0x00)
    {
        (VOID)disp_hal_putFrameMem(&stSystemFrameResize);
    }

    return NULL;
}

#endif

/**
 * @function   disp_tskCreateVoChnThr
 * @brief      创建显示设备单个通道任务线程
 * @param[in]  UINT32 uiDev
 * @param[in]  UINT32 uiChnCnt
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskCreateVoChnThr(UINT32 uiDev, UINT32 uiChnCnt)
{
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    INST_INFO_S *pstInst = NULL;
    CHAR szInstName[NAME_LEN] = {0};
    UINT32 i = 0;
    NODE_CFG_S stNodeCfg[] =
    {
        {IN_NODE_0, "in_0", NODE_BIND_TYPE_GET},
    };
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();

    if (pstDspInitPrm == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error");
        return SAL_FAIL;
    }

    pDispChn = (DISP_CHN_COMMON *)SAL_memZalloc(sizeof(DISP_CHN_COMMON), DISP_TASK_NAME, DISP_MEM_NAME);
    if (pDispChn == NULL)
    {
        for (i = 0; i < pDispDev->szLayer.uiMaxChnCnt; i++)
        {
            pDispChn = disp_tskGetVoChn(uiDev, i);
            if (pDispChn != NULL)
            {
                SAL_memfree(pDispChn, DISP_TASK_NAME, DISP_MEM_NAME);
                pDispChn = NULL;
            }
        }

        DISP_LOGE("error !\n");
        return SAL_FAIL;
    }

    SAL_clear(pDispChn);
    pDispChn->uiLayerNo = pDispDev->szLayer.uiLayerNo;
    pDispChn->uiDevNo = pDispDev->uiDevNo;
    pDispChn->uiChn = uiChnCnt;
    pDispChn->flag = 1;
    pDispChn->enCleanStatus = DISP_CLEAN_STATUS_IDLE;
    pDispChn->enlargeprm.partchn = -1;
    pDispChn->enlargeprm.globalratio = 1.0;
    pDispChn->enlargeprm.uiCropW = pstCapbDisp->disp_yuv_w_max;
    pDispChn->enlargeprm.uiCropH = pstCapbDisp->disp_yuv_h;
    pDispChn->stInModule.g_AiDispSwitch = 1;
    pDispDev->szLayer.pstVoChn[uiChnCnt] = pDispChn;

    pstInst = link_drvReqstInst();
    if (pstInst)
    {
        snprintf(szInstName, NAME_LEN, "DISP%d_CHN%d", pDispChn->uiDevNo, uiChnCnt);
        link_drvInitInst(pstInst, szInstName);
        for (i = 0; i < sizeof(stNodeCfg) / sizeof(NODE_CFG_S); i++)
        {
            link_drvInitNode(pstInst, &stNodeCfg[i]);
        }
    }

    /* 2.创建线程 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pDispChn->mChnMutexHdl);
#ifdef DISP_VIDEO_FUSION
    SAL_thrCreate(&pDispChn->stChnThrHandl, disp_tskVoChnThreadStitch, 60, 0, pDispChn);
#else
    /* vo chan创建时并不知道该chan是绑定还是送帧，所以送帧线程还是需要创建 */
    SAL_thrCreate(&pDispChn->stChnThrHandl, disp_tskVoChnThread, 60, 0, pDispChn);
#endif

    if (pstDspInitPrm->dspCapbPar.dev_tpye != PRODUCT_TYPE_ISM)
    {
        disp_osdDrawInit(pDispDev->szLayer.uiLayerNo, uiChnCnt, capb_get_line()->enDrawMod, capb_get_osd()->enDrawMod);
    }
    else
    {
        DISP_LOGD("Vo %d Disp_HalDrawInit ism not support !\n", pDispDev->uiDevNo);
    }

    DISP_LOGI("Vo %d Chn %d Tsk Init Success !\n", pDispDev->uiDevNo, uiChnCnt);

    return SAL_SOK;
}

/**
 * @function   disp_tskCreateVoTsk
 * @brief      创建显示设备所有通道任务线程
 * @param[in]  UINT32 uiDev
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskCreateVoTsk(UINT32 uiDev)
{

    INT32 s32Ret = 0;
    UINT32 voChn = 0;
    DSPINITPARA *pDspInitPara = NULL;
    VO_INIT_PRM *pstVoInfoPrm = NULL;

    if (disp_tskGetVoCnt() <= uiDev)
    {
        DISP_LOGE("error uiDev %d (Max Dev %d)\n", uiDev, disp_tskGetVoCnt());
        return SAL_FAIL;
    }

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pstVoInfoPrm = &pDspInitPara->stVoInitInfoSt.stVoInfoPrm[uiDev];
    if (pstVoInfoPrm == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pstVoInfoPrm->voChnCnt > DISP_HAL_MAX_CHN_NUM)
    {
        DISP_LOGE("error");
        return SAL_FAIL;
    }

#if 0  /*新的Trunk这里已经注释掉*/
    if ((pDspInitPara->boardType >= SECURITY_MACHINE_START) && (pDspInitPara->boardType <= SECURITY_MACHINE_END))
    {
        if (gMagnifer[0].stChnThrHandl.hndl == 0 && uiDev == 0)
        {
            disp_hal_InitVpssGroup(VPSS_ENLARGE_GRP_CHN_0, 0, 3);
            gMagnifer[0].stChnThrHandl.hndl = 1;
        }

        if (gMagnifer[1].stChnThrHandl.hndl == 0 && uiDev == 1)
        {
            disp_hal_InitVpssGroup(VPSS_ENLARGE_GRP_CHN_1, 0, 3);
            gMagnifer[1].stChnThrHandl.hndl = 1;
        }
    }

#endif

    for (voChn = 0; voChn < pstVoInfoPrm->voChnCnt; voChn++)
    {
        s32Ret = disp_tskCreateVoChnThr(uiDev, voChn);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("disp_tskCreateVoChnThr uiVoDev %u  voChn = %u failed!!!\n", uiDev, voChn);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskVoDeInit
 * @brief      去初始化显示设备
 * @param[in]  UINT32 uiDev
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskVoDeInit(UINT32 uiDev)
{
    UINT32 i = 0;
    INT32 s32Ret = SAL_FAIL;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DUP_ChanHandle *pstDup = NULL;
    DUP_BIND_PRM stDupBindPrm = {0};

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error \n");
        return SAL_FAIL;
    }

    if (pDispDev->uiStatus == SAL_FALSE)
    {
        DISP_LOGW("Vo Dev %d DeInit Success !\n", pDispDev->uiDevNo);
        return SAL_SOK;
    }

    /* 清除显示设备上已经使能的显示通道 */
    for (i = 0; i < disp_tskGetVoChnCnt(uiDev); i++)
    {
        pDispChn = disp_tskGetVoChn(uiDev, i);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;
        }

        s32Ret = disp_tskSetVoChnStatus(pDispChn, SAL_TRUE, SAL_FALSE);
        if (s32Ret == SAL_SOK)
        {
            s32Ret = disp_hal_stopChn(pDispChn);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("dev:%u chn:%u stop fail\n", uiDev, pDispChn->uiChn);
                return SAL_FAIL;
            }

            pstDup = disp_tskGetVoDupHandle(uiDev, pDispChn);
            if (NULL == pstDup)
            {
                DISP_LOGE("dev:%u chn:%u get dup handle fail\n", uiDev, pDispChn->uiChn);
                return SAL_FAIL;
            }

            /* TODO: 不同设备间的兼容 */
            if (pstDup->dupOps.OpDupStopBlit != NULL)
            {
                s32Ret = pstDup->dupOps.OpDupStopBlit((Ptr)pstDup);
                if (SAL_SOK != s32Ret)
                {
                    DUP_LOGE("OpDupStopBlit Chn err !!!\n");
                }
            }

            stDupBindPrm.mod = SYSTEM_MOD_ID_DISP;
            stDupBindPrm.modChn = uiDev;
            stDupBindPrm.chn = pDispChn->uiChn;
            if (pstDup->dupOps.OpDupBindBlit != NULL)
            {
                s32Ret = pstDup->dupOps.OpDupBindBlit((Ptr)pstDup, (Ptr) & stDupBindPrm, SAL_FALSE);
                if (SAL_SOK != s32Ret)
                {
                    DUP_LOGE("Bind Hal Chn err !!!\n");
                    return SAL_FAIL;
                }
            }

            s32Ret = disp_hal_disableChn(pDispChn);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("error !!\n");
                return SAL_FAIL;
            }
        }
    }

    /* 去使能显示设备 */
    if (SAL_SOK != disp_hal_deleteDev(pDispDev))
    {
        DISP_LOGE("uiChn %d !!!\n", uiDev);
        return SAL_FAIL;
    }

    pDispDev->uiStatus = SAL_FALSE;

    DISP_LOGI("Vo Dev %d DeInit Success !\n", pDispDev->uiDevNo);

    return SAL_SOK;
}

/**
 * @function   disp_tskGetFrameBuffer
 * @brief      获取fb显存
 * @param[in]  UINT32 uiChn
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  fb显存属性
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskGetFrameBuffer(UINT32 uiChn, SCREEN_CTRL *pstHalFbAttr)
{
    INT32 s32Ret = SAL_FAIL;
    DSPINITPARA *pDspInitPara = NULL;
    FB_INIT_ATTR_ST stFbPrm = {0};

    /* 获取显示设备设置的分辨率 */
    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        SAL_ERROR("error !!\n");
        return SAL_FAIL;
    }

    stFbPrm.uiH = pDspInitPara->stVoInitInfoSt.stVoInfoPrm[uiChn].stDispDevAttr.videoOutputHeight;
    stFbPrm.uiW = pDspInitPara->stVoInitInfoSt.stVoInfoPrm[uiChn].stDispDevAttr.videoOutputWidth;

    s32Ret = fb_hal_getFrameBuffer(uiChn, pstHalFbAttr, &stFbPrm);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("fb_hal_getFrameBuffer is err!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_tskVoInit
 * @brief      初始化显示设备
 * @param[in]  UINT32 uiDev
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_tskVoInit(UINT32 uiDev)
{
    INT32 s32Ret = 0;
    UINT32 u32BoardType;
    DISP_DEV_COMMON *pDispDev = NULL;
    DISP_LAYER_COMMON *pstLayer = NULL;
    CAPB_DISP *capb_disp = NULL;
    DSPINITPARA *pDspInitPara = NULL;
    VO_INIT_PRM *pstVoInfoPrm = NULL;

    if (disp_tskGetVoCnt() <= uiDev)
    {
        DISP_LOGE("error uiDev %d (Max Dev %d)\n", uiDev, disp_tskGetVoCnt());
        return SAL_FAIL;
    }

    capb_disp = capb_get_disp();
    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error");
        return SAL_FAIL;
    }

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pstVoInfoPrm = &pDspInitPara->stVoInitInfoSt.stVoInfoPrm[uiDev];
    if (pstVoInfoPrm == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pDispDev->uiStatus == SAL_TRUE)
    {
        if ((pDispDev->uiDevWith == pstVoInfoPrm->stDispDevAttr.videoOutputWidth)
            && (pDispDev->uiDevHeight == pstVoInfoPrm->stDispDevAttr.videoOutputHeight)
            && (pDispDev->uiDevFps == pstVoInfoPrm->stDispDevAttr.videoOutputFps))
        {
            DISP_LOGI("Vo Dev %d Init Success !\n", pDispDev->uiDevNo);
            return SAL_SOK;
        }
        else
        {
            s32Ret = disp_tskVoDeInit(uiDev);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("disp_tskVoDeInit error dev %d !!!\n", uiDev);
                return SAL_FAIL;
            }
        }
    }

    pDispDev->uiBgcolor = capb_disp->disp_screen_bg_color;
    pDispDev->uiDevWith = pstVoInfoPrm->stDispDevAttr.videoOutputWidth;
    pDispDev->uiDevHeight = pstVoInfoPrm->stDispDevAttr.videoOutputHeight;
    pDispDev->uiDevFps = pstVoInfoPrm->stDispDevAttr.videoOutputFps;
    pDispDev->eVoDev = pstVoInfoPrm->stDispDevAttr.eVoDev;
    pDispDev->uiDevNo = uiDev;

    DISP_LOGI("Disp Dev %d  W %d H %d Fps %d eVoDev %d !!!\n", uiDev, pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps, pDispDev->eVoDev);

    u32BoardType = HARDWARE_GetBoardType();

    if (VO_DEV_MIPI == pDispDev->eVoDev)
    {
        /* 环通设备要先初始化海思mipi再，初始化转换芯片lt9211; 102和204设备要先复位6211才能再初始化海思mipi*/
        if ((DB_DS50018_V1_0 == u32BoardType)
            || (DB_RS20001_V1_0 == u32BoardType)
            || (DB_RS20001_V1_1 == u32BoardType)
            || (DB_RS20007_A_V1_0 == u32BoardType)
            || (DB_RS20007_V1_0 == u32BoardType)
            || (DB_RS20006_V1_0 == u32BoardType))
        {
            disp_chipSetRes(pDispDev, DISP_VO_MIPI);
        }
    }

    pstLayer = &pDispDev->szLayer;
    pstLayer->uiLayerNo = pDispDev->uiDevNo;

    s32Ret = disp_hal_createDev(pDispDev);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("uiDev %d !!!\n", uiDev);
        return SAL_FAIL;
    }

    pstLayer->uiLayerWith = pDispDev->uiDevWith;
    pstLayer->uiLayerHeight = pDispDev->uiDevHeight;
    pstLayer->uiLayerFps = pDispDev->uiDevFps;
    pstLayer->uiMaxChnCnt = pstVoInfoPrm->voChnCnt;
    pstLayer->enInputSalPixelFmt = capb_disp->enInputSalPixelFmt;

    s32Ret = disp_hal_createLayer(pstLayer);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("uiLayerNo %d !!!\n", pDispDev->szLayer.uiLayerNo);
        return SAL_FAIL;
    }

    DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "HDMI", pDispDev->uiDevFps);
    s32Ret = disp_hal_voInterface(pDispDev);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("disp_hal_voInterface %d failed!!!\n", pDispDev->uiDevNo);
        return SAL_FAIL;
    }

    if (VO_DEV_HDMI == pDispDev->eVoDev)
    {
        disp_chipSetRes(pDispDev, DISP_VO_HDMI);
    }
    else if ((VO_DEV_MIPI == pDispDev->eVoDev))
    {

        /* 环通设备要先初始化海思mipi再，初始化转换芯片lt9211; 102和204设备要先复位6211才能再初始化海思mipi*/
        if ((DB_RS20025_V1_0 == u32BoardType)
            || (DB_RS20016_V1_0 == u32BoardType)
            || (DB_RS20016_V1_1 == u32BoardType)
            || (DB_RS20016_V1_1_F303 == u32BoardType)
            || (DB_TS3637_V1_0 == u32BoardType)
            || (DB_TS3719_V1_0 == u32BoardType)
            || (DB_RS20046_V1_0 == u32BoardType))
        {
            disp_chipSetRes(pDispDev, DISP_VO_MIPI);
            DISP_LOGI("VoDev %d eVoDev %s eVoDev_Num %d \n", pDispDev->uiDevNo, "MIPI", pDispDev->eVoDev);
        }
    }
    else if ((VO_DEV_MIPI_1 == pDispDev->eVoDev))
    {

        if ((DB_RS20025_V1_0 == u32BoardType)
            || (DB_RS20016_V1_0 == u32BoardType)
            || (DB_RS20016_V1_1 == u32BoardType)
            || (DB_RS20016_V1_1_F303 == u32BoardType)
            || (DB_TS3637_V1_0 == u32BoardType)
            || (DB_TS3719_V1_0 == u32BoardType)
            || (DB_RS20046_V1_0 == u32BoardType))
        {
            disp_chipSetRes(pDispDev, DISP_VO_MIPI_1);
            DISP_LOGI("VoDev %d eVoDev %s eVoDev_Num %d \n", pDispDev->uiDevNo, "MIPI_1", pDispDev->eVoDev);
        }
    }

    pDispDev->uiStatus = SAL_TRUE;

    DISP_LOGI("Vo Dev %d Init Success !\n", pDispDev->uiDevNo);

    return SAL_SOK;
}

/**
 * @fn      disp_tskGetDptzoffset
 * @brief   获取XRay过包图像全局放大的抠图区域
 * 
 * @param   chan[IN] XRay通道号 
 * @param   fZoomRatio[OUT] 全局放大比例，不小于1.0
 * @param   pstDispArea[OUT] 基于显示图像（CAPB_DISP->disp_yuv_w_max × disp_yuv_h）的抠图区域
 * 
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
SAL_STATUS disp_tskGetDptzoffset(UINT32 chan, FLOAT32 *fZoomRatio, RECT *pstDispArea)
{
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();
    INT32 i = 0;

    if (chan > 2)
    {
        DISP_LOGE("chn %d err\n", chan);
        return SAL_FAIL;
    }

    if (pstDispArea == NULL)
    {
        DISP_LOGE("chn %d null err\n", chan);
        return SAL_FAIL;
    }

    if (pstDspInitPrm->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN)
    {
        pDispChn = disp_tskGetVoChn(0, chan);
        if (pDispChn == NULL)
        {
            DISP_LOGE("voDev %d chn %d pDispChn null err\n", chan, i);
            return SAL_FAIL;
        }

        if (NULL != fZoomRatio)
        {
            *fZoomRatio = SAL_MAX(pDispChn->enlargeprm.globalratio, 1.0);
        }
        pstDispArea->uiX = pDispChn->enlargeprm.uiCropX;
        pstDispArea->uiY = pDispChn->enlargeprm.uiCropY;
        pstDispArea->uiWidth = pDispChn->enlargeprm.uiCropW;
        pstDispArea->uiHeight = pDispChn->enlargeprm.uiCropH;

        return SAL_SOK;
    }

    pDispDev = disp_tskGetVoDev(chan);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error chan[%d] pDispDev [%p]\n", chan, pDispDev);
        return SAL_FAIL;
    }

    for (i = 0; i < pDispDev->szLayer.uiMaxChnCnt; i++)
    {
        pDispChn = disp_tskGetVoChn(chan, i);
        if (pDispChn == NULL)
        {
            DISP_LOGE("voDev %d chn %d pDispChn null err\n", chan, i);
            return SAL_FAIL;
        }

        if (pDispChn->stInModule.uiModId == SYSTEM_MOD_ID_DUP)
        {
            break;
        }
    }
    if (pDispChn == NULL)
    {
        DISP_LOGE("voDev %d chn %d pDispChn null err\n", chan, i);
        return SAL_FAIL;
    }

    if (NULL != fZoomRatio)
    {
        *fZoomRatio = SAL_MAX(pDispChn->enlargeprm.globalratio, 1.0);
    }
    pstDispArea->uiX = pDispChn->enlargeprm.uiCropX;
    pstDispArea->uiY = pDispChn->enlargeprm.uiCropY;
    pstDispArea->uiWidth = pDispChn->enlargeprm.uiCropW;
    pstDispArea->uiHeight = pDispChn->enlargeprm.uiCropH;

    return SAL_SOK;
}

/**
 * @function   CmdProc_dispCmdProc
 * @brief      显示控制指令（应用层交互）
 * @param[in]  HOST_CMD cmd
 * @param[in]  UINT32 chan
 * @param[in]  void *prm   参数有可能为NULL
 * @param[out] None
 * @return     INT32
 */
INT32 CmdProc_dispCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    int iRet = SAL_FAIL;

    if (HOST_CMD_SET_MOUSE_POS != cmd && HOST_CMD_PIP_VDECDISP_REGION != cmd && HOST_CMD_DISP_FB_SHOW != cmd && HOST_CMD_DISP_GET_DISP_WBC != cmd &&
        HOST_CMD_PIP_DISP_REGION != cmd && HOST_CMD_DPTZ_DISP_PRM != cmd)
    {
        DISP_LOGI("chan %d, cmd: 0x%x\n", chan, cmd);
    }

    switch (cmd)
    {
        case HOST_CMD_MODULE_DISP: /* 显示模块的能力级 */
        {
            iRet = SAL_SOK;

            break;
        }
        /*自动显示进入到这里面  chan=0/1  prm窗口显示的属性*/
        case HOST_CMD_ALLOC_DISP_REGION: /* 配置显示通道布局 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))   /*检测显示设备号合法性*/
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            /* 检查显示设备是否使能 */
            if (SAL_SOK != disp_tskVoInit(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskVoChnStart(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_DISP_ADD_VO: /* 配置显示通道布局 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            /* 检查显示设备是否使能 */
            if (SAL_SOK != disp_tskVoInit(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskAddVo(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_PIP_DISP_REGION: /* 配置显示通道布局 */
        {
            if (SAL_SOK != disp_tskSetPipPrm(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_SET_THUMBNAIL_PRM: /* 配置缩略图通道布局 */
        {

            if (SAL_SOK != disp_tskSetThumbnailPrm(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_DISPFLICKER_PRM:  /* 设置智能框闪烁以及画线类型*/
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_osdSetDispFlickPrm(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_DISP_GET_ARTICLE_RESULT:
        {
            DISP_LOGW("Chan %u not supported cmd:0x%x, return success\n", chan, cmd);

            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_SET_DISPLINETYPE_PRM:  /* 设置智能框闪烁以及画线类型*/
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_osdSetLineTypePrm(chan, prm)) /* 分析仪disp叠框参数配置 */
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != Xpack_DrvSetAidgrLinePrm(chan, (DISPLINE_PRM *)prm)) /* 安检机Xpack叠框参数配置，分析仪空函数实现 */
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }

        case HOST_CMD_DPTZ_DISP_PRM: /* 配置显示通道布局 */
        {

            if (SAL_SOK != disp_tskSetVoChnDptzPrm(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_PIP_VDECDISP_REGION:
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskSetPos(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_DISP_CLEAR: /* 清除布局 */
        {
            DISP_LOGI("chan %u, HOST_CMD_DISP_CLEAR, WinCnt: %u, WinIdx: %u %u %u %u\n", chan, ((DISP_CLEAR_CTRL *)prm)->uiCnt,
                      ((DISP_CLEAR_CTRL *)prm)->uiChan[0], ((DISP_CLEAR_CTRL *)prm)->uiChan[1], ((DISP_CLEAR_CTRL *)prm)->uiChan[2],
                      ((DISP_CLEAR_CTRL *)prm)->uiChan[3]);
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskVoChnStop(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_DISP_ROTATE: /* 配置显示 Rotate 属性 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskSetVideoRotate(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_DISP_MIRROR: /* 配置显示 Mirror 属性 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskSetVideoMirror(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_OUTPUT_REGION: /* 配置显示通道的输出区域   */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskSetVideoRegion(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_VI_DISP:   /* 配置采集预览功能 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskSetVoChnBind(chan, SYSTEM_MOD_ID_DUP, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_SET_DEC_DISP:     /* 配置解码预览功能 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskSetVoChnAttr(chan, SYSTEM_MOD_ID_VDEC, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_STOP_DISP:     /* 解除VO CHN输入模块关系 */
        {
            DISP_LOGI("chan %u, HOST_CMD_STOP_DISP, WinCnt: %u, Idx-Vi-Win: 0 - %u %u, 1 - %u %u\n", chan, ((DSP_DISP_RCV_PARAM *)prm)->uiCnt,
                      ((DSP_DISP_RCV_PARAM *)prm)->addr[0], ((DSP_DISP_RCV_PARAM *)prm)->out[0], ((DSP_DISP_RCV_PARAM *)prm)->addr[1],
                      ((DSP_DISP_RCV_PARAM *)prm)->out[1]);

            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskStopDisp(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }

        case HOST_CMD_SET_VO_STYLE: /* 设置显示输出风格 */
        {
            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_VO_CSC:  /* 设置显示输出效果 */
        {
            if (SAL_SOK != disp_tskCheckVoIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_hal_setVoLayerCsc(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_DISP_FB_INIT:
        {
            DISP_LOGI("HOST_CMD_DISP_FB_INIT \n\n");
            if (SAL_SOK != fb_hal_Init())
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            DISP_LOGI("HOST_CMD_DISP_FB_INIT\n\n");

            break;
        }
        case HOST_CMD_DISP_FB_MMAP:   /* UI层内存映射   */
        {
            DISP_LOGI("chan %d HOST_CMD_DISP_FB_MMAP start \n\n", chan);

            if (SAL_SOK == disp_tskCheckVoIllegal(chan))
            {
                if (disp_tskVoInit(chan) != SAL_SOK)
                {
                    DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                    break;
                }
            }

            if (SAL_SOK != fb_hal_checkFbIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != disp_tskGetFrameBuffer(chan, (SCREEN_CTRL *)prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            DISP_LOGI("chan %d HOST_CMD_DISP_FB_MMAP End \n\n", chan);

            break;
        }
        case HOST_CMD_DISP_FB_UMAP:  /* UI层内存去映射 */
        {
            DISP_LOGI("chan %d HOST_CMD_DISP_FB_UMAP start \n\n", chan);

            if (SAL_SOK != fb_hal_checkFbIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != fb_hal_releaseFb(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK == disp_tskCheckVoIllegal(chan))
            {
                if (disp_tskVoDeInit(chan) != SAL_SOK)
                {
                    DISP_LOGE("error \n");
                    break;
                }
            }

            iRet = SAL_SOK;

            DISP_LOGI("chan %d HOST_CMD_DISP_FB_UMAP End \n\n", chan);

            break;
        }
        case HOST_CMD_DISP_FB_SHOW:    /* 刷新UI显示     */ /* HISI和RK更新鼠标坐标时也使用该接口 */
        {
            if (SAL_SOK != fb_hal_checkFbIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != fb_hal_refreshFb(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_MOUSE_POS: /*20220421 NT98336更新鼠标坐标时使用该接口，HISI和RK不用该接口 */
        {
            if (SAL_SOK != fb_hal_checkFbIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != fb_hal_setPos(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_DISP_FB_SNAP:
        {
            if (SAL_SOK != fb_hal_checkFbIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != fb_hal_snapFb(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_MOUSE_BIND_CHN:
        {
            if (SAL_SOK != fb_hal_checkFbIllegal(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != fb_hal_setMouseFbChn(chan))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_VO_SWITCH: /* 配置显示叠加OSD开关 */
        {
            if (SAL_SOK != disp_osdEnableVoAiFlag(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_MENU:  /* 设置菜单属性   */
        {
            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_REFRESH_MENU_DISP:  /* 刷新菜单显示   */
        {
            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_VO_STANDARD: /* 修改CVBS输出制式 */
        {
            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_CLEAR_VO_BUFFER: /* 清除Vo窗口数据 */
        {
            if (SAL_SOK != disp_tskVoChnClearBuf(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;

            break;
        }
        case HOST_CMD_SET_AI_DISP_SWITCH: /*设置智能显示开关*/
        {
            if (SAL_SOK != disp_osdSetVoAiSwitch(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            if (SAL_SOK != Xpack_DrvSetAiXrOsdShow(chan, (DISP_SVA_SWITCH *)prm, SAL_TRUE))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_DISP_SET_ORG_SVAPRM:
        {
            DISP_LOGW("Chan %u not supported cmd:0x%x, return success\n", chan, cmd);

            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_DISP_GET_CUREFFECTREG:
        {
            if (SAL_SOK != disp_tskGetCurEffectRegCoordinate(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_SET_DISPLINEWIDTH_PRM:
        {
            if (SAL_SOK != disp_osdSetDispLineWidthPrm(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }
#if 1
        case HOST_CMD_DISP_WBC_ENABLE:
        {
            if (SAL_SOK != disp_hal_enableDispWbc(chan, (unsigned int *)prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }

        case HOST_CMD_DISP_GET_DISP_WBC:
        {
            if (SAL_SOK != disp_hal_getDispWbc(chan, prm))
            {
                DISP_LOGE("Chn %d Host Cmd %x Failed !!!\n", chan, cmd);
                break;
            }

            iRet = SAL_SOK;
            break;
        }
#endif
        default:
        {
            DISP_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            iRet = SAL_FAIL;
            break;
        }
    }

    return iRet;
}

/**
 * @function   disp_tskVoCreate
 * @brief      创建显示设备结构体，申请后不释放。
 * @param[in]  UINT32 uiDev
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tskVoCreate(UINT32 uiDev)
{

    DISP_DEV_COMMON *pDispDev = NULL;

    pDispDev = (DISP_DEV_COMMON *)SAL_memZalloc(sizeof(DISP_DEV_COMMON), DISP_TASK_NAME, DISP_MEM_NAME);
    if (pDispDev == NULL)
    {
        DISP_LOGE("error !\n");
        return SAL_FAIL;
    }

    SAL_clear(pDispDev);

    g_disp_common.pstDevObj[uiDev] = pDispDev;

    g_disp_common.uiDevMaxCnt++;

    if (NULL == g_disp_common.mMutexHdl)
    {
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_disp_common.mMutexHdl);
    }

    DISP_LOGI("Vo Dev %d Create Success !\n", uiDev);

    return SAL_SOK;
}

/**
 * @function   disp_tsk_moduleInit
 * @brief      显示模块注册
 * @param[in]  void
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tsk_moduleInit(void)
{
    UINT32 i = 0;

    DSPINITPARA *pDspInitPara = NULL;

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        return SAL_FAIL;
    }

    /* bsp启动时会启动显示logo，所以要反初始化vo */
    for (i = 0; i < pDspInitPara->stVoInitInfoSt.uiVoCnt; i++)
    {
        if (SAL_SOK != disp_tsk_deInitStartingup(i))
        {
            SYS_LOGW("Delete vo %d fail\n", i);
        }
    }

    if (SAL_SOK != disp_hal_modInit(NULL))
    {
        SYS_LOGW("disp module init fail\n");
    }

    for (i = 0; i < pDspInitPara->stVoInitInfoSt.uiVoCnt; i++)
    {
        if (disp_tskVoCreate(i) != SAL_SOK)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;
        }

        if (disp_tskVoInit(i) != SAL_SOK)
        {
            DISP_LOGE("error \n");
            return SAL_FAIL;
        }

        if (disp_tskCreateVoTsk(i) != SAL_SOK)
        {
            DISP_LOGE("error \n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   disp_tsk_deInitStartingup
 * @brief      开机时反初始化显示
 * @param[in]  UINT32 uiDev
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tsk_deInitStartingup(UINT32 uiDev)
{
    if (uiDev > MAX_VODEV_CNT)
    {
        DISP_LOGE("uiDev %d Delete StartingLogo !!!\n", uiDev);
        return SAL_FAIL;
    }

    return disp_hal_deInitStartingup(uiDev);
}

/**
 * @function   disp_tsk_setDbLevel
 * @brief      设置调试打印等级
 * @param[in]  INT32 level
 * @param[out] None
 * @return     void
 */
void disp_tsk_setDbLevel(INT32 level)
{
    dispDbLevel = (level > 0) ? level : 0;
    DISP_LOGI("debugLevel %d\n", dispDbLevel);
}

/**
 * @fn      disp_tskShowVoWin
 * @brief   查看VO设备上的窗口属性
 *
 * @param   [IN] u32VoDev VO设备号
 */
void disp_tskShowVoWin(UINT32 u32VoDev)
{
    UINT32 i = 0, u32VoChnMax = 0;
    DISP_DEV_COMMON *pstDispDev = NULL;
    DISP_CHN_COMMON *pstDispChn = NULL;

    pstDispDev = disp_tskGetVoDev(u32VoDev);
    if (NULL == pstDispDev)
    {
        DISP_LOGE("disp_tskGetVoDev failed, VoDev: %u\n", u32VoDev);
        return;
    }

    SAL_print("\nDISP > VoDev Idx VoChn En    X    Y    W    H In -Mod -Chn \n");
    u32VoChnMax = disp_tskGetVoChnCnt(u32VoDev);
    for (i = 0; i < u32VoChnMax; i++)
    {
        pstDispChn = disp_tskGetVoChn(u32VoDev, i);
        if (NULL != pstDispChn)
        {
            SAL_print("%12u %u/%u %5u %2u %4u %4u %4u %4u %2u %4u %4u\n", u32VoDev, i, u32VoChnMax,
                      pstDispChn->uiChn, pstDispChn->uiEnable, pstDispChn->uiX, pstDispChn->uiY, pstDispChn->uiW, pstDispChn->uiH,
                      pstDispChn->stInModule.uiUseFlag, pstDispChn->stInModule.uiModId, pstDispChn->stInModule.uiChn);
        }
        else
        {
            DISP_LOGE("disp_tskGetVoChn failed, VoDev: %u, VoChn: %u\n", u32VoDev, i);
        }
    }

    return;
}

/**
 * @function   disp_tskShowVoTime
 * @brief      查看VO设备上环节时间信息
 * @param[in]  UINT32 u32VoDev  vo设备号
 * @param[out] None
 * @return     void
 */
void disp_tskShowVoTime(UINT32 u32VoDev)
{
    UINT32 i = 0, j = 0;
    UINT32 u32VoChnMax = 0;
    UINT32 u32StartIdx = 0;
    DISP_DEV_COMMON *pstDispDev = NULL;
    DISP_CHN_COMMON *pstDispChn = NULL;
    DISP_DEBUG_CHN_TIME *pstDbgTime = NULL;

    pstDispDev = disp_tskGetVoDev(u32VoDev);
    if (NULL == pstDispDev)
    {
        DISP_LOGE("disp_tskGetVoDev failed, VoDev: %u\n", u32VoDev);
        return;
    }

    u32VoChnMax = disp_tskGetVoChnCnt(u32VoDev);

    for (i = 0; i < u32VoChnMax; i++)
    {
        pstDispChn = disp_tskGetVoChn(u32VoDev, i);
        if (NULL == pstDispChn)
        {
            DISP_LOGE("disp_tskGetVoChn failed, VoDev: %u, VoChn: %u\n", u32VoDev, i);
        }

        if (pstDispChn->uiEnable)
        {
            u32StartIdx = pstDispChn->u32DbgTimeIdx;

            SAL_print("\nTime > %d  UseFlag ModId   Entry      PrcStart   GetStart    GetEnd   GetTimeOut  SendStart   SendEnd       Exit  Total\n", i);
            for (j = u32StartIdx + 1; j < u32StartIdx + DISP_DEBUG_CHN_TIME_NUM; j++)
            {
                pstDbgTime = &pstDispChn->stDbgTime[j % DISP_DEBUG_CHN_TIME_NUM];
                SAL_print("%17u %5u %10llu %10llu %10llu  %10llu %10llu %10llu %10llu %10llu %5llu\n",
                          pstDbgTime->uiUseFlag, pstDbgTime->uiModId, pstDbgTime->u64TimeEntry,
                          pstDbgTime->u64TimePrcStart, pstDbgTime->u64TimeGetStart, pstDbgTime->u64TimeGetEnd,
                          pstDbgTime->u64TimeGetTimeOut, pstDbgTime->u64TimeSendStart, pstDbgTime->u64TimeSendEnd,
                          pstDbgTime->u64TimeExit, pstDbgTime->u64TimeExit - pstDbgTime->u64TimeEntry);

            }
        }
    }

    return;
}

/**
 * @function   disp_tsk_dump
 * @brief      显示调试
 * @param[in]  INT32 vodev
 * @param[in]  INT32 vochn
 * @param[in]  INT32 arg0    显示信号源
 * @param[in]  INT32 dumpcnt 循环显示
 * @param[out] None
 * @return     INT32
 */
INT32 disp_tsk_dump(INT32 vodev, INT32 vochn, INT32 arg0, INT32 dumpcnt, CHAR chDumpDir[])
{
    INT32 s32Ret = SAL_FAIL;
    INT32 width = 0;
    INT32 height = 0;
    INT32 stride = 0;
    INT32 i = 0, j = 0;
    INT32 nosignalframe = 0;
    INT32 vochnframe = 0;
    INT32 nosignalframe_before = 0;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_DEBUG_CTL *pDispDebug = NULL;
    CAPB_DISP *capb_disp = NULL;
    FILE *file = NULL;
    char name[128];
    char *vir = NULL;
    char dumppath[128] = {0};
    SYSTEM_FRAME_INFO stSystemFrame = {0};
    SAL_VideoFrameBuf salVideoFrame = {0};

    capb_disp = capb_get_disp();

    if (capb_disp == NULL)
    {
        DISP_LOGE("NULL err\n");
        return SAL_FAIL;
    }

    if (chDumpDir == NULL)
    {
        DISP_LOGE("chDumpDir == NULL err\n");
        return SAL_FAIL;
    }

    if (vodev >= DISP_MAX_DEV_NUM)
    {
        DISP_LOGE("vodev[%d] invalid!\n", vodev);
        return SAL_FAIL;
    }

    if (vochn >= capb_disp->disp_chan_cnt)
    {
        DISP_LOGE("vochn[%d] invalid!\n", vochn);
        return SAL_FAIL;
    }

    pDispDebug = &gDispDebug[vodev][vochn];
    pDispChn = disp_tskGetVoChn(vodev, vochn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("pDispChn NULL err vodev %d vochn %d\n", vodev, vochn);
        return SAL_FAIL;
    }

    if (strlen(chDumpDir) >= 128)
    {
        DISP_LOGE("chDumpDir %s len %zd\n", chDumpDir, strlen(chDumpDir));
        return SAL_FAIL;
    }

    strcpy(dumppath, chDumpDir);
    switch (arg0)
    {
        case DUMP_NOSIGNAL:
            nosignalframe = 1;
            break;

        case DUMP_VO:
            vochnframe = 1;
            break;

        case DUMP_NOSIGNAL_BEFORE:
            nosignalframe_before = 1;
            break;

        default:
            DISP_LOGE("unsupport the module: %d\n", arg0);
            break;
    }

    for (j = 0; j < dumpcnt; j++)
    {
        if (nosignalframe == 1)
        {

            if (pDispDebug->no_signal_frame == NULL)
            {
                DISP_LOGE("frame null err\n");
                return SAL_FAIL;
            }

            stSystemFrame.uiAppData = (PhysAddr)pDispDebug->no_signal_frame;

            memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
            s32Ret = sys_hal_getVideoFrameInfo(&stSystemFrame, &salVideoFrame);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
            }

            width = salVideoFrame.frameParam.width;
            height = salVideoFrame.frameParam.height;
            stride = salVideoFrame.stride[0];
            vir = (void *)(salVideoFrame.virAddr[0]);
            sprintf(name, "/mnt/Dispnosignalvodev_%d_chn%d_%d_%d_%d.yuv", vodev, vochn, j, width, height);
            strcat(dumppath, name);
            file = fopen(dumppath, "wb");
            if (file == NULL)
            {
                DISP_LOGE("file null err %s\n", dumppath);
                break;
            }

            for (i = 0; i < height; i++)
            {
                fwrite(vir + i * stride, 1, width, file);
            }

            vir = (void *)(salVideoFrame.virAddr[1]);
            stride = salVideoFrame.stride[1];
            for (i = 0; i < height / 2; i++)
            {
                fwrite(vir + i * stride, 1, width, file);
            }

            DISP_LOGI("no signal frame dump success\n");
            fclose(file);
        }

        if (vochnframe == 1)
        {

            if (pDispDebug->vo_frame == NULL)
            {
                DISP_LOGE("frame null err\n");
                return SAL_FAIL;
            }

            stSystemFrame.uiAppData = (PhysAddr)pDispDebug->vo_frame;
            memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
            s32Ret = sys_hal_getVideoFrameInfo(&stSystemFrame, &salVideoFrame);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
            }

            width = salVideoFrame.frameParam.width;
            height = salVideoFrame.frameParam.height;
            stride = salVideoFrame.stride[0];
#if 0
            s32Ret = HI_MPI_VB_GetBlockVirAddr(salVideoFrame.poolId, salVideoFrame.phyAddr[0], (void **)&vir);
            if (s32Ret != SAL_SOK)
            {
                DISP_LOGE("poolid %d mmap failed\n", salVideoFrame.poolId);
                break;
            }

#else
            vir = (char *)salVideoFrame.virAddr[0];
#endif
            sprintf(name, "/mnt/Dispvochnpicvodev_%d_chn%d_%d_%d_%d.yuv", vodev, vochn, j, width, height);
            file = fopen(name, "wb");
            if (file == NULL)
            {
                DISP_LOGE("file null err %s\n", name);
                break;
            }

            for (i = 0; i < height * 3 / 2; i++)
            {
                fwrite(vir + i * stride, 1, width, file);
            }

            DISP_LOGI("vo chn frame dump success\n");
            fclose(file);
        }

        if (nosignalframe_before == 1)
        {

            if (pDispDebug->no_signal_frame_before == NULL)
            {
                DISP_LOGE("frame null err\n");
                return SAL_FAIL;
            }

            stSystemFrame.uiAppData = (PhysAddr)pDispDebug->no_signal_frame_before;
            memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
            s32Ret = sys_hal_getVideoFrameInfo(&stSystemFrame, &salVideoFrame);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
            }

            width = salVideoFrame.frameParam.width;
            height = salVideoFrame.frameParam.height;
            stride = salVideoFrame.stride[0];
            vir = (void *)(salVideoFrame.virAddr[0]);
            sprintf(name, "/mnt/Dispnosignal_beforevodev_%d_chn%d_%d_%d_%d.yuv", vodev, vochn, j, width, height);
            file = fopen(name, "wb");
            if (file == NULL)
            {
                DISP_LOGE("file null err %s\n", name);
                break;
            }

            for (i = 0; i < height; i++)
            {
                fwrite(vir + i * stride, 1, width, file);
            }

            vir = (void *)(salVideoFrame.virAddr[1]);
            stride = salVideoFrame.stride[1];
            for (i = 0; i < height / 2; i++)
            {
                fwrite(vir + i * stride, 1, width, file);
            }

            DISP_LOGI("no signal frame before dump success\n");
            fclose(file);
        }
    }

    DISP_LOGI("vodev %d chn %d arg0 %d dumpcnt %d success\n", vodev, vochn, arg0, dumpcnt);
    return SAL_SOK;
}

