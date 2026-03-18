#include "capt_tsk_api.h"
#include "dspcommon.h"

#include "capt_tsk_inter.h"
#include "capbility.h"
#include "sys_tsk.h"
#include "capt_chip_drv_api.h"
#include "system_prm_api.h"
#include "../../../func/captchip/ltchip/capt_chip_lt9211.h"





/*****************************************************************************
                            宏定义
*****************************************************************************/


/*****************************************************************************
                            全局结构体
*****************************************************************************/

static CAPT_OBJ_COMMON g_stCaptObjCommon =
{
    .uiObjCnt = 0,
    .lock     = PTHREAD_MUTEX_INITIALIZER,
};

#define MAX_MALLOC_MEM  (10)
#define CAPT_MAX_DEV    (MAX_CAPTURE_CHAN_NUM)
#define CAPT_WIDTH      (1920)
#define CAPT_HEIGHT     (1080)
#define CAPT_TSAK_NAME   "capt_tsk"
/*****************************************************************************
                            全局结构体
*****************************************************************************/
static const CAPT_REDUCE_FPS_MAP_S g_astReducedFps[] = {
    {120,       60},
    {144,       72},
};


static CAPT_DRV_STATUS_S g_stCaptDrvStatus =
{
    .uiCaptNum = 0,
};


static CAPT_CHN_USER_PIC stUserPic[CAPT_MAX_DEV][MAX_NO_SIGNAL_PIC_CNT] = {0};


/*****************************************************************************
                            函数
*****************************************************************************/


/*****************************************************************************
 函 数 名  : capt_tskMutexLock
 功能描述  : Capt 模块上锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_tskMutexLock(void)
{
    pthread_mutex_t * pViMutex = &g_stCaptObjCommon.lock;

    pthread_mutex_lock(pViMutex);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tskMutexUnLock
 功能描述  : Capt 模块去锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_tskMutexUnLock(void)
{
    pthread_mutex_t * pViMutex = &g_stCaptObjCommon.lock;

    pthread_mutex_unlock(pViMutex);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tskChnrvMutexInit
 功能描述  : Capt 模块初始化锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月05日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_tskChnrvMutexInit(CAPT_OBJ_PRM * pstChnPrm)
{
    pthread_mutex_t * pMutex = NULL;

    pMutex = &pstChnPrm->mutex;

    pthread_mutex_init(pMutex,NULL);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tskChnMutexLock
 功能描述  : capt 模块 通道上锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月05日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_tskChnMutexLock(CAPT_OBJ_PRM * pstChnPrm)
{
    pthread_mutex_t * pmutex = NULL;

    pmutex = &pstChnPrm->mutex;

    pthread_mutex_lock(pmutex);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tskChnMutexUnLock
 功能描述  : Capt 模块 通道去锁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月05日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_tskChnMutexUnLock(CAPT_OBJ_PRM * pstChnPrm)
{
    pthread_mutex_t * pMutex = NULL;

    pMutex = &pstChnPrm->mutex;

    pthread_mutex_unlock(pMutex);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvStopVideoDected
 功能描述  : 采集模块 停止 无视频检测
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvStopVideoDected(UINT32 uiChn)
{
    if (uiChn >= MAX_CAPTURE_CHAN_NUM)
    {
        return SAL_FAIL;
    }
    CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];
    pthread_mutex_lock(&pstCaptChnPrm->viMutex);
    pstCaptChnPrm->uiVideoDectedStatus = 0;
    pthread_mutex_unlock(&pstCaptChnPrm->viMutex);
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_drvStartVideoDected
 功能描述  : 采集模块 开启无视频检测
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvStartVideoDected(UINT32 uiChn)
{
    if (uiChn >= MAX_CAPTURE_CHAN_NUM)
    {
        return SAL_FAIL;
    }
    CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];

    pthread_mutex_lock(&pstCaptChnPrm->viMutex);
    pstCaptChnPrm->uiVideoDectedStatus = ENABLED;
    pthread_cond_signal(&pstCaptChnPrm->vicond);
    pthread_mutex_unlock(&pstCaptChnPrm->viMutex);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvWaitForVideoDected
 功能描述  : 采集模块 等待 开启无视频检测
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvWaitForVideoDected(UINT32 uiChn)
{
    if (uiChn >= MAX_CAPTURE_CHAN_NUM)
    {
        return SAL_FAIL;
    }
    CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];

    pthread_mutex_lock(&pstCaptChnPrm->viMutex);
    if (0 == pstCaptChnPrm->uiVideoDectedStatus)
    {
        pthread_cond_wait(&pstCaptChnPrm->vicond, &pstCaptChnPrm->viMutex);
    }
    pthread_mutex_unlock(&pstCaptChnPrm->viMutex);

    return SAL_SOK;
}



#ifdef NEW_LIUXY_SENSOR


/*****************************************************************************
 函 数 名  : capt_task_drvReInitMipi
 功能描述  : 重新初始化 MIPI 与 sensor
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年4月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvReInitMipi(UINT32 uiChn)
{
    /* 初始化 MIPI */
    capt_hal_initCaptInterface(0);
    capt_hal_initCaptInterface(1);
    #if 0
    /* ISP 软复位 */
    Isp_drvIspReset(0);
    Isp_drvIspReset(1);

    Isp_drvSetIspDefPrm(0);
    Isp_drvSetIspDefPrm(1);
    #endif
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvIspGetPrm
 功能描述  : ISP 模块获取值
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvIspGetPrm(UINT32 uiChn, UINT32 uiKey, UINT32 *uiVal)
{
    //return Isp_drvGetPrm(uiChn, uiKey, uiVal);
    return 0;
}

/*****************************************************************************
 函 数 名  : capt_task_drvIspSetPrm
 功能描述  : ISP 模块设置值
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvIspSetPrm(UINT32 uiChn, UINT32 uiKey, UINT32 uiVal)
{
    //return Isp_drvSetPrm(uiChn, uiKey, uiVal);
    return 0;
}



/*******************************************************************************
* 函数名  : capt_task_drvSetVideoWdr
* 描  述  : 开启关闭ISP的宽动态
* 输  入  : - ispChan: 0 可见光摄像头 1红外摄像头
*         : - isOpen : 打开关闭
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
static INT32 capt_task_drvSetVideoWdr(UINT32 ispChan, UINT32 isOpen)
{
    #if 0
    INT32               status          = 0;
    UINT32              uiValue         = 0;
    ISP_CREATE_ATTR     stIspCreateAttr;
    ISP_CHAN_ATTR      *pstIspChnAttr   = NULL;
    CAPT_CHN_PRM       *pstCaptChnPrm_0 = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[0];
    CAPT_CHN_PRM       *pstCaptChnPrm_1 = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[1];

    memset(&stIspCreateAttr, 0, sizeof(ISP_CREATE_ATTR));
    stIspCreateAttr.uiChnCnt = g_stCaptDrvStatus.uiCaptNum;

    status = capt_task_drvStopVideoDected(0);
    if (SAL_isFail(status))
    {
        CAPT_LOGE("Capt Drv Stop Chn %d Video Dected Failed !!!\n", 0);
        return SAL_FAIL;
    }

    status = capt_task_drvStopVideoDected(1);
    if (SAL_isFail(status))
    {
        CAPT_LOGE("Capt Drv Stop Chn %d Video Dected Failed !!!\n", 1);
        return SAL_FAIL;
    }

    if(ispChan == 0)
    {
        if(isOpen == SAL_TRUE)
        {
            pstCaptChnPrm_0->isOpenWdr = SAL_TRUE;
        }
        else
        {
            pstCaptChnPrm_0->isOpenWdr = SAL_FALSE;
        }
    }
    else
    {
        if(isOpen == SAL_TRUE)
        {
            pstCaptChnPrm_1->isOpenWdr = SAL_TRUE;
        }
        else
        {
            pstCaptChnPrm_1->isOpenWdr = SAL_FALSE;
        }
    }

    CAPT_LOGW("Camera_0 Set WDR %d \n", pstCaptChnPrm_0->isOpenWdr);
    CAPT_LOGW("Camera_1 Set WDR %d \n", pstCaptChnPrm_1->isOpenWdr);
    
    capt_hal_stopCaptDev(0);
    capt_hal_stopCaptDev(1);

    Isp_drvIspSetDefWDRPrm(0, pstCaptChnPrm_0->isOpenWdr);
    Isp_drvIspSetDefWDRPrm(1, pstCaptChnPrm_1->isOpenWdr);
    
    Isp_drvIspSetWDRMod(0, pstCaptChnPrm_0->isOpenWdr);
    Isp_drvIspSetWDRMod(1, pstCaptChnPrm_1->isOpenWdr);

    /* 初始化 MIPI 接口 */
    capt_hal_initCaptInterface(0);
    capt_hal_initCaptInterface(1);

    #if 0
    /* ISP 软复位 */
    Isp_drvIspReset(0);
    Isp_drvIspReset(1);
    #endif
    /* 配置 VI 通道宽动态 */
    capt_hal_drvSetWDRPrm(0, pstCaptChnPrm_0->isOpenWdr);
    capt_hal_drvSetWDRPrm(1, pstCaptChnPrm_1->isOpenWdr);

    capt_hal_startCaptDev(0);
    capt_hal_startCaptDev(1);

    Isp_drvSetIspDefPrm(0);
    Isp_drvSetIspDefPrm(1);

    status = capt_task_drvStartVideoDected(0);
    if (SAL_isFail(status))
    {
        CAPT_LOGE("Capt Drv Start Chn %d Video Dected Failed !!!\n", 0);
        return SAL_FAIL;
    }

    status = capt_task_drvStartVideoDected(1);
    if (SAL_isFail(status))
    {
        CAPT_LOGE("Capt Drv Start Chn %d Video Dected Failed !!!\n", 1);
        return SAL_FAIL;
    }
    #endif    
    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : capt_task_drvStopSensor
 功能描述  : 采集模块采集通道 停止 sensor 输出
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年4月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvStopSensor( )
{
    CAPT_LOGI("HOST_CMD_SENSOR_STOP \n");

    system("hik_echo D > /proc/driver/sen_wl");
    system("hik_echo D > /proc/driver/sen_ir");
    system("hik_echo O > /proc/driver/sen_wl");
    system("hik_echo O > /proc/driver/sen_ir");

    system("himm 0x1214e400 4");
    system("himm 0x1214e3fc 0");

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_drvStartSensor
 功能描述  : 采集模块采集通道 开启 sensor 输出
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年4月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvStartSensor( )
{
    CAPT_LOGI("HOST_CMD_SENSOR_START \n");

    #if 1
    system("himm 0x1214e400 4");
    system("himm 0x1214e3fc 4");

    system("hik_echo P > /proc/driver/sen_wl");
    system("hik_echo P > /proc/driver/sen_ir");
    system("hik_echo E > /proc/driver/sen_wl");
    system("hik_echo E > /proc/driver/sen_ir");
    #endif
    usleep(10000);

    capt_task_drvReInitMipi(0);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_drvSelect
 功能描述  : 采集模块查询HAL层数据
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月10日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvSelect(UINT32 uiChn)
{
    if (SAL_FAIL == capt_hal_checkCaptBuffer(uiChn))
    {
        CAPT_LOGE("Capt Drv Select Chn %d Failed !!!\n",uiChn);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvDelete
 功能描述  : 采集模块采集通道销毁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvDelete(UINT32 uiChn)
{
    CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];

    if (pstCaptChnPrm != NULL)
    {
        SAL_memFree(pstCaptChnPrm);
        pstCaptChnPrm = NULL;

        g_stCaptDrvStatus.uiCaptChnPrm[uiChn] = 0;
    }

    return SAL_SOK;
}

#endif



/*****************************************************************************
 函 数 名  : capt_task_drvSetVideoPos
 功能描述  : 采集模块采集通道偏移位置属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvSetVideoPos(UINT32 viChn, POSITION_MODE posInfo, UINT32 offset)
{
     return capt_hal_setCaptPos(viChn, posInfo, offset);
}


/*******************************************************************************
* 函数名  : capt_task_drvDetcVideoIn
* 描  述  : 用于adv7842视频检测
* 输  入  : - prm    : 输入参数 
* 输  出  : - 无
* 返回值  : 0        : 成功
*          -1        : 失败
*******************************************************************************/
static INT32 capt_task_drvDetcVideoIn(UINT32 chnId, INPUT_VIDEO_ST * outPrm, UINT32 *inpuChagFlg)
{
    INT32 s32Ret = SAL_FAIL;
    CAPT_STATUS_S pstLastStatus;
    CAPT_STATUS_S stStatus;
    CAPT_RESOLUTION_S stRes;
    const CAPT_REDUCE_FPS_MAP_S *pstReduceFps = g_astReducedFps;
    static UINT32 u32ReduceFpsNum = sizeof(g_astReducedFps)/sizeof(g_astReducedFps[0]);
    CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[chnId];
    UINT32 i = 0;
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();
    DUP_BIND_PRM stSrcBuf = {0};
     

#if 1
    if (NULL == inpuChagFlg)
    {
        CAPT_LOGE("chnId %d inpuChagFlg is null\n",chnId);
        return SAL_FAIL;
    }
    
    *inpuChagFlg = 0;
    
    if (chnId >= 2)
    {
        CAPT_LOGE("chnId %d\n",chnId);
        return SAL_FAIL;
    }

    if (NULL == outPrm)
    {
        CAPT_LOGE("chnId %d outPrm is null\n",chnId);
        return SAL_FAIL;
    }
    
    /* 采集未开启不需要检测 */
    if (0 == pstCaptChnPrm->uiStart)
    {
        return SAL_SOK;
    }

    memset(&pstLastStatus, 0 ,sizeof(CAPT_STATUS_S));

    if(SAL_SOK != capt_func_chipGetStatus(chnId, &pstLastStatus))
    {
        CAPT_LOGE("capt_func_chipGetStatus chnId = %u failed\n",chnId);
        return SAL_FAIL;
    }

#endif

    memset(&stStatus, 0 ,sizeof(CAPT_STATUS_S));
    if(SAL_SOK !=capt_func_chipDetect(chnId, &stStatus))
    {
        CAPT_LOGE("capt_func_chipDetect chnId = %u failed\n",chnId);
        return SAL_FAIL;
    }

    if (stStatus.stRes.u32Width > CAPT_WIDTH_MAX || stStatus.stRes.u32Height > CAPT_HEIGHT_MAX || stStatus.stRes.u32Fps > CAPT_FPS_MAX + CAPT_FPS_ERROR)
    {
        return SAL_SOK;
    }

    if ((0 == stStatus.stRes.u32Fps) || (0 == stStatus.stRes.u32Width) || (0 == stStatus.stRes.u32Height))
    {
        pstCaptChnPrm->captStatus = (SAL_TRUE == stStatus.bVideoDetected) ? 2 : 1;
    }
    else
    {
        pstCaptChnPrm->captStatus = 0;
    }
    
    if ((stStatus.stRes.u32Fps != pstLastStatus.stRes.u32Fps) ||
        (stStatus.stRes.u32Width != pstLastStatus.stRes.u32Width) ||
        (stStatus.stRes.u32Height != pstLastStatus.stRes.u32Height) ||
        (stStatus.enCable != pstLastStatus.enCable) || 
        (stStatus.bVideoDetected != pstLastStatus.bVideoDetected))
    {
        /* ADV 采集视频信息发生变化 */
        /* 重新配置HI3559A的mipi/vi */
        /* 配置FPGA 的分辨率*/
        CAPT_LOGW("capt(%d) change [%u*%u@%u] to [%u*%u@%u], cable[%d %d] detected[%u %u]\n",
                   chnId, pstLastStatus.stRes.u32Width, pstLastStatus.stRes.u32Height, pstLastStatus.stRes.u32Fps,
                   stStatus.stRes.u32Width, stStatus.stRes.u32Height, stStatus.stRes.u32Fps,
                   pstLastStatus.enCable, stStatus.enCable, pstLastStatus.bVideoDetected, stStatus.bVideoDetected);

        sal_memcpy_s(&stRes, sizeof(stRes), &stStatus.stRes, sizeof(stStatus.stRes));
   
        capt_func_chipScale(chnId, &stRes, &stStatus.stRes);

        if ((0 == stRes.u32Width) || (0 == stRes.u32Height) || (0 == stRes.u32Fps))
        {
            stRes.u32Width  = 1920;
            stRes.u32Height = 1080;
            stRes.u32Fps    = 60;
        }

        if (SAL_SOK == capt_func_chipGetOutputRes(chnId, &stRes))
        {
            for (i = 0; i < u32ReduceFpsNum; i++)
            {
                if ((stStatus.stRes.u32Fps > (pstReduceFps->u32Fps - CAPT_FPS_ERROR)) && (stStatus.stRes.u32Fps < (pstReduceFps->u32Fps + CAPT_FPS_ERROR)))
                {
                    stRes.u32Fps = pstReduceFps->u32ReducedFps;
                    capt_func_chipSetOutputRes(chnId, &stRes);
                    break;
                }

                pstReduceFps++;
            }
        }
        
        stSrcBuf.mod = SYSTEM_MOD_ID_CAPT;
        capt_hal_getCaptPipeId(chnId, (INT32 *)&stSrcBuf.modChn);
        s32Ret = dup_task_bindToDup(&stSrcBuf, *g_stCaptObjCommon.pstObj[chnId]->pChanHandle->pfrontDupHandle, SAL_FALSE);
        if ( SAL_SOK != s32Ret)
        {
            CAPT_LOGE("dup_task_unbindToDup failed\n");
        }

        s32Ret = capt_hal_stopCaptDev(chnId);
        if ( SAL_SOK != s32Ret)
        {
            CAPT_LOGE("capt_hal_stopCaptDev chnId = %u failed\n",chnId);
        }
        
        capt_func_chipFpgaSetResolution(chnId, stRes.u32Width, stRes.u32Height, stRes.u32Fps);
        capt_hal_reSetCaptInterface(chnId, &stRes);
        capt_hal_reInitCaptDev(chnId);
        capt_hal_startCaptDev(chnId);
        capt_hal_drvSetFlag(chnId, 1);

        /*配置LT9211*/
        if(HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
        {
            capt_chip_lt9211Config(chnId,1920,1080,stRes.u32Fps,SAL_TRUE);
        }
        
        s32Ret = dup_task_bindToDup(&stSrcBuf, *g_stCaptObjCommon.pstObj[chnId]->pChanHandle->pfrontDupHandle, SAL_TRUE);
        if ( SAL_SOK != s32Ret)
        {
            CAPT_LOGE("dup_task_bindToDup failed\n");
        }

        outPrm->inputMode = (stStatus.enCable >= CAPT_CABLE_BUTT) ? INPUT_VIDEO_NONE : stStatus.enCable;
        outPrm->stVidoeRes.uiWidth  = stStatus.stRes.u32Width;
        outPrm->stVidoeRes.uiHeight = stStatus.stRes.u32Height;
        outPrm->stVidoeRes.uiFps    = stStatus.stRes.u32Fps;

        *inpuChagFlg = 1;

        if ((0 != stStatus.stRes.u32Width) && (0 != stStatus.stRes.u32Height) && (0 != stStatus.stRes.u32Fps))
        {
            if (SAL_TRUE == capt_func_chipIsFpgaNeedReset())
            {
                capt_func_chipFpgaDectedAndReset(chnId);
            }
            /* 检测图像正确性 */
            if (SAL_SOK == capt_func_chipGetAdjustStatus(chnId))
            {
                i = 0;
                while ((i++) < 5)
                {
                    SAL_msleep(1 * 1000);
                    if (SAL_TRUE == capt_hal_captIsOffset(chnId))
                    {
                        /* 自动调整，VGA会偏 */
                        if(CAPT_CABLE_VGA == stStatus.enCable)
                        {
                            capt_func_chipAutoAdjust(chnId, stStatus.enCable);
                        }
                        
                        /* LT9211存在时序不对，导致画面异常，重新配置LT9211 */
                        if(HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
                        {
                            capt_chip_lt9211Config(chnId,1920,1080,stRes.u32Fps,SAL_TRUE);
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                CAPT_LOGW("capt chn[%u] auto adjust %u times\n", chnId, i - 1);
            }
        }
        if(SAL_SOK != capt_func_chipSetStatus(chnId, &stStatus))
        {
            CAPT_LOGE("capt_func_chipGetStatus chnId = %u failed\n",chnId);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvDetectUserPicThread
 功能描述  : 采集模块视频信号检测线程无视频信号
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月22日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static void *capt_task_drvDetectUserPicThread(void * prm)
{
    UINT32  uiChn = (UINT32)((PhysAddr)prm);

    UINT32  uiLastViIntCnt = 0;
    UINT32  uiCurViIntCnt  = 0;
    UINT32  LostFrameCnt   = 0;
    UINT32  LastLostFrame  = 0;
    BOOL    bVideoLoss     = SAL_FALSE;
    BOOL    needSet        = SAL_FALSE;
    UINT32  picId          = 1;
    UINT32  errorCnt       = 0;
    UINT32  checkFlg       = 0;
    CAPT_OBJ_PRM    *pstObjPrm   = NULL;

    CAPT_HAL_CHN_STATUS stCaptChnStatus;

    CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];
    pstObjPrm = g_stCaptObjCommon.pstObj[uiChn];
    
    SAL_SET_THR_NAME();

    /* 从未开启过，则一直获取状态，不切换无视频信号 */
    while (0 ==  pstCaptChnPrm->uiStart)
    {
        memset(&stCaptChnStatus, 0, sizeof(CAPT_HAL_CHN_STATUS));
        if (SAL_SOK != capt_hal_getCaptStatus(uiChn, &stCaptChnStatus))
        {
            CAPT_LOGD("Capt Chn %d Query Status Failed !!!\n", uiChn);
            usleep(100 * 1000);
            continue;
        }

        uiLastViIntCnt = stCaptChnStatus.uiIntCnt;
        LostFrameCnt  = stCaptChnStatus.FrameLoss;
        usleep(100 * 1000);
    }
    
    while (1)
    {
        usleep(100 * 1000);
        capt_task_drvWaitForVideoDected(uiChn);
        (void)capt_tskChnMutexLock(pstObjPrm);
        memset(&stCaptChnStatus, 0, sizeof(CAPT_HAL_CHN_STATUS));
        if (SAL_SOK != capt_hal_getCaptStatus(uiChn, &stCaptChnStatus))
        {
            CAPT_LOGD("Capt Chn %d Query Status Failed !!!\n", uiChn);
            usleep(100 * 1000);
            needSet = SAL_TRUE;
            uiLastViIntCnt = stCaptChnStatus.uiIntCnt;
            LastLostFrame  = stCaptChnStatus.FrameLoss;
            (void)capt_tskChnMutexUnLock(pstObjPrm);
            continue;
        }
        /*采集无视频信号存在问题待排查内存问题导致VI用户填充视频异常，
           初次调试时可以屏蔽无视频信号或者通过无视频信号验证后端问题;*/

        uiCurViIntCnt = stCaptChnStatus.uiIntCnt;
        LostFrameCnt  = stCaptChnStatus.FrameLoss;
        if (LostFrameCnt > LastLostFrame)
        {
            
            CAPT_LOGW("CAPT is ERROR chn %d uiCurViIntCnt %d uiLastViIntCnt %d LostFrameCnt %d LastLostFrame %d .bVideoLoss %d captStatus %d\n\n", pstCaptChnPrm->uiChn,
            uiCurViIntCnt, uiLastViIntCnt, LostFrameCnt,LastLostFrame, bVideoLoss,pstCaptChnPrm->captStatus); 
            if (checkFlg > 0)
            {
                errorCnt++;
                if (errorCnt > 30)
                {
                    errorCnt = 0;
                    system("cat /proc/umap/hi_mipi");
                    system("cat /proc/umap/vi");
                    capt_func_chipInit(uiChn);
                }
            }
            
        }
        //CAPT_LOGW("chn %d uiCurViIntCnt %d uiLastViIntCnt %d LostFrameCnt %d LastLostFrame %d .bVideoLoss %d \n\n", pstCaptChnPrm->uiChn,
        //uiCurViIntCnt, uiLastViIntCnt, LostFrameCnt,LastLostFrame, bVideoLoss);
        //存在fpga一直给3559a数据，但是数据是错误的被3559a丢掉了
        if (1 != capt_func_chipCheckStatus(uiChn) || (0 != pstCaptChnPrm->captStatus))//((0 != pstCaptChnPrm->captStatus) && ((0 == (uiCurViIntCnt - uiLastViIntCnt)) || ((LostFrameCnt - LastLostFrame) >= (uiCurViIntCnt - uiLastViIntCnt))))
        {
            #if 0
            if ((pstCaptChnPrm->captStatus != picId))
            {
                needSet = SAL_TRUE;
                picId   = 1;
                if ((pstCaptChnPrm->captStatus > 0) && (pstCaptChnPrm->captStatus < 3))
                {
                    picId = pstCaptChnPrm->captStatus;
                }
                
                CAPT_LOGI("Vi chn %d Enable User Pic needSet %d, picId %d!!!\n", uiChn,needSet,picId);
            }
            #endif

            if ((SAL_FALSE == bVideoLoss) && (uiCurViIntCnt == uiLastViIntCnt) && (uiCurViIntCnt > 0))
            {
                needSet = SAL_TRUE;
            }
            
            if ((SAL_FALSE == bVideoLoss) || (SAL_TRUE == needSet))
            {
                /* */
                checkFlg = 0;
                errorCnt = 0;
               
                if (capt_hal_drvGetFlag(uiChn))
                {
                    
                    CAPT_LOGI("Cat chn %d is adv change !!!\n", uiChn);
                    uiLastViIntCnt = uiCurViIntCnt;
                    LastLostFrame  = LostFrameCnt;
                    (void)capt_tskChnMutexUnLock(pstObjPrm);
                    continue;
                }
                
                //if (SAL_TRUE == needSet)
                {
                    if (SAL_SOK != capt_hal_setCaptUserPic(uiChn, &stUserPic[uiChn][picId]))
                    {
                        CAPT_LOGE("Cat chn %d Set User Pic Failed !!!\n", uiChn);
                     
                        uiLastViIntCnt = uiCurViIntCnt;
                        LastLostFrame  = LostFrameCnt;
                        (void)capt_tskChnMutexUnLock(pstObjPrm);
                        continue;
                    }
                }
                
                CAPT_LOGI("Vi chn %d Enable User Pic needSet %d,picId %d,captStatus %d,%p!!!\n", uiChn,needSet,picId,pstCaptChnPrm->captStatus,stUserPic[uiChn][picId].pucAddr);
                CAPT_LOGW("chn %d uiCurViIntCnt %d uiLastViIntCnt %d LostFrameCnt %d LastLostFrame %d .bVideoLoss %d \n\n", pstCaptChnPrm->uiChn,
                uiCurViIntCnt, uiLastViIntCnt, LostFrameCnt,LastLostFrame, bVideoLoss);
                if (SAL_SOK != capt_hal_enableCaptUserPic(uiChn, SAL_TRUE))
                {
                    CAPT_LOGE("Capt Hal Chn %d  Enable User Pic Failed !!\n", uiChn);
                    uiLastViIntCnt = uiCurViIntCnt;
                    LastLostFrame  = LostFrameCnt;
                    (void)capt_tskChnMutexUnLock(pstObjPrm);
                    continue;
                }
                
               
            }
            bVideoLoss = SAL_TRUE;
        }
        else
        {
            if ((SAL_TRUE == bVideoLoss) || (SAL_TRUE == needSet))
            {
                /* */
                CAPT_LOGI("Vi chn %d disable Camera Pic !!!\n", uiChn);
                if (SAL_SOK != capt_hal_enableCaptUserPic(uiChn, SAL_FALSE))
                {
                    CAPT_LOGE("Capt Hal Chn %d  Enable User Pic Failed !!\n", uiChn);
                }
                errorCnt = 0;
                checkFlg = 1;
            }
            bVideoLoss = SAL_FALSE;
        }
        needSet        = SAL_FALSE;
        uiLastViIntCnt = uiCurViIntCnt;
        LastLostFrame  = LostFrameCnt;
        (void)capt_tskChnMutexUnLock(pstObjPrm);
    }

    return NULL;
}


/*****************************************************************************
 函 数 名  : capt_task_drvDetectUserPicThread
 功能描述  : 采集模块视频信号检测线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月22日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static void *capt_task_drvDetectThread(void * prm)
{
     UINT32  uiChn = (UINT32)((PhysAddr)prm);
     INPUT_VIDEO_ST   outPrm;
     UINT32 inpuChagFlg = 0;
     INT32    ret = 0;
     STREAM_ELEMENT    stStreamEle;
     INPUT_VIDEO_ST    stInputResult;
     CAPT_OBJ_PRM    *pstObjPrm   = NULL;
     
     SAL_clear(&outPrm);
     SAL_clear(&stStreamEle);
     stStreamEle.type = STREAM_ELEMENT_VI_VIDEOSTAND;
     stStreamEle.chan = uiChn;
     stInputResult.inputMode  = INPUT_VIDEO_NONE;
     stInputResult.stVidoeRes.uiWidth  = 0;
     stInputResult.stVidoeRes.uiHeight = 0;
     stInputResult.stVidoeRes.uiFps    = 0;
     SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stInputResult, sizeof(INPUT_VIDEO_ST));
     
     capt_func_chipInit(uiChn);
     pstObjPrm = g_stCaptObjCommon.pstObj[uiChn];

     SAL_SET_THR_NAME();

     while(1)
     {
         (void)capt_tskChnMutexLock(pstObjPrm);
         ret = capt_task_drvDetcVideoIn(uiChn,&outPrm,&inpuChagFlg);
         (void)capt_tskChnMutexUnLock(pstObjPrm);
         if (ret == SAL_SOK)
         {
             if (inpuChagFlg)
             {
                memset(&stStreamEle, 0, sizeof(STREAM_ELEMENT));
                stStreamEle.type = STREAM_ELEMENT_VI_VIDEOSTAND;
                stStreamEle.chan = uiChn;
                stInputResult.inputMode  = outPrm.inputMode;
                stInputResult.stVidoeRes.uiWidth  = outPrm.stVidoeRes.uiWidth;
                stInputResult.stVidoeRes.uiHeight = outPrm.stVidoeRes.uiHeight;
                stInputResult.stVidoeRes.uiFps    = outPrm.stVidoeRes.uiFps;
                SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stInputResult, sizeof(INPUT_VIDEO_ST));
             }
         }
         SAL_msleep(1000);
     }
     
     return NULL;
}



/*****************************************************************************
 函 数 名  : capt_task_drvSetVideoPrm
 功能描述  : 采集模块采集通道 配置属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvSetVideoPrm(UINT32 uiChn)
{
    //CAPT_CHN_PRM *pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];

    if (SAL_FAIL == capt_hal_stopCaptDev(uiChn))
    {
        CAPT_LOGE("Capt Drv Set Video Prm Chn %d Failed !!!\n",uiChn);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvStop
 功能描述  : 采集模块采集通道采集停止
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvStop(UINT32 uiChn)
{
    CAPT_CHN_PRM *pstCaptChnPrm = NULL;
    
    if(uiChn >= MAX_CAPTURE_CHAN_NUM)
    {
        return SAL_FAIL;
    }

    pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];

    pstCaptChnPrm->uiStart = 0;

    if (SAL_FAIL == capt_task_drvStopVideoDected(uiChn))
    {
        CAPT_LOGE("Capt Drv Stop Chn %d Video Dected Failed !!!\n", uiChn);

        return SAL_FAIL;
    }

    if (SAL_FAIL == capt_hal_stopCaptDev(uiChn))
    {
        CAPT_LOGE("Capt Drv Stop Chn %d Failed !!!\n", uiChn);

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_drvStart
 功能描述  : 采集模块采集通道采集开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvStart(UINT32 uiChn)
{
    CAPT_CHN_PRM *pstCaptChnPrm = NULL;

    if(uiChn >= MAX_CAPTURE_CHAN_NUM)
    {
        return SAL_FAIL;
    }
    
    pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];     
    
    if (SAL_FAIL == capt_hal_reInitCaptDev(uiChn))
    {
        CAPT_LOGE("Capt Drv Create Chn %d Failed !!!\n", uiChn);

        return SAL_FAIL;
    }

    if (SAL_FAIL == capt_hal_startCaptDev(uiChn))
    {
        CAPT_LOGE("Capt Drv Start Chn %d Failed !!!\n", uiChn);

        return SAL_FAIL;
    }

    pstCaptChnPrm->uiStart = 1;

    if (SAL_FAIL == capt_task_drvStartVideoDected(uiChn))
    {
        CAPT_LOGE("Capt Drv Start Chn %d Video Dected Failed !!!\n", uiChn);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvCreate
 功能描述  : 采集模块采集通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvCreate(UINT32 uiChn, CAPT_CREATE_ATTR *pstCaptCreatePrm)
{
    CAPT_CHN_PRM  *pstCaptChnPrm  = NULL;
    CAPT_CHN_ATTR *pstCaptChnAttr = NULL;
    UINT32         i              = 0;
    //CAPT_CHN_USER_PIC stUserPic;
    //UINT32            uiDevFb    = 0;

    if (NULL == pstCaptCreatePrm)
    {
        CAPT_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (g_stCaptDrvStatus.uiCaptNum <= uiChn)
    {
        CAPT_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[uiChn];
    
    pstCaptChnAttr = &pstCaptChnPrm->stCaptChnAttr;

    pstCaptChnAttr->uiCaptWid = pstCaptCreatePrm->stCaptChnPicAttr.uiCaptWid;
    pstCaptChnAttr->uiCaptHei = pstCaptCreatePrm->stCaptChnPicAttr.uiCaptHei;
    pstCaptChnAttr->uiCaptFps = pstCaptCreatePrm->stCaptChnPicAttr.uiCaptFps;

    if (SAL_FAIL == capt_func_chipCreate(uiChn))
    {
         CAPT_LOGE("capt_func_chipCreate Chn %d Failed !!!\n", uiChn);
        return SAL_FAIL;
    }

    if (SAL_FAIL == capt_hal_createCaptDev(uiChn, pstCaptChnAttr))
    {
        CAPT_LOGE("Capt Drv Create Chn %d Failed !!!\n", uiChn);
        return SAL_FAIL;
    }
    
    SAL_thrCreate(&pstCaptChnPrm->videoCaptHndl, capt_task_drvDetectThread, SAL_THR_PRI_DEFAULT,  0,  (void *)((PhysAddr)uiChn));
    /* 无视频信号时图像信息 */
    for (i = 0; i < MAX_NO_SIGNAL_PIC_CNT; i++)
    {
        memset(&stUserPic[uiChn][i], 0, sizeof(CAPT_CHN_USER_PIC));

        stUserPic[uiChn][i].uiW     = pstCaptCreatePrm->stCaptChnNoSignalAttr.uiNoSignalPicW[i];
        stUserPic[uiChn][i].uiH     = pstCaptCreatePrm->stCaptChnNoSignalAttr.uiNoSignalPicH[i];
        stUserPic[uiChn][i].pucAddr = pstCaptCreatePrm->stCaptChnNoSignalAttr.pucNoSignalPicAddr[i];
        stUserPic[uiChn][i].uiSize  = pstCaptCreatePrm->stCaptChnNoSignalAttr.uiNoSignalPicSize[i];
    }
    
    /* 默认是无视频信号 */
    if (SAL_SOK != capt_hal_setCaptUserPic(uiChn, &stUserPic[uiChn][1]))
    {
        CAPT_LOGE("Cat chn %d Set User Pic Failed !!!\n", uiChn);
        return SAL_FAIL;
    }

    pthread_mutex_init(&pstCaptChnPrm->viMutex, NULL);
    pthread_cond_init(&pstCaptChnPrm->vicond, NULL);
    
    //SAL_thrCreate(&pstCaptChnPrm->videoCaptHndl, capt_task_drvDetectThread, SAL_THR_PRI_DEFAULT,  0,  (void *)uiChn);
    /* 创建视频信号检测线程 */
    SAL_thrCreate(&pstCaptChnPrm->videoDetectHndl, capt_task_drvDetectUserPicThread, SAL_THR_PRI_DEFAULT,  0,  (void *)((PhysAddr)uiChn));

    CAPT_LOGI("capt_task_drvCreate chn %d  success !!!\n", uiChn);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_drvInit
 功能描述  : 采集模块采集模块初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_drvInit(void)
{
    int i = 0;
    CAPT_CHN_PRM    *pstCaptChnPrm = NULL;

    ISP_CREATE_ATTR  stIspCreateAttr;
    ISP_CHAN_ATTR   *pstIspChnAttr = NULL;

    /*可以根据capbility能力集配置*/
    g_stCaptDrvStatus.uiCaptNum = CAPT_MAX_DEV; 

    /* 采集参数初始化 */
    for(i = 0; i < g_stCaptDrvStatus.uiCaptNum; i++)
    {
        pstCaptChnPrm = (CAPT_CHN_PRM *)SAL_memZalloc(sizeof(CAPT_CHN_PRM), CAPT_TSAK_NAME, CAPT_MEM_NAME);
        if (NULL == pstCaptChnPrm)
        {
            CAPT_LOGE("!!!\n");
            return SAL_FAIL;
        }

        g_stCaptDrvStatus.uiCaptChnPrm[i] = (PhysAddr)pstCaptChnPrm;

        memset(pstCaptChnPrm, 0, sizeof(CAPT_CHN_PRM));

        if (SAL_SOK == SystemPrm_getSysVideoFormat( ))
        {
            pstCaptChnPrm->stCaptChnAttr.uiCaptFps = 25;
        }
        else
        {
            pstCaptChnPrm->stCaptChnAttr.uiCaptFps = 30;
        }

        pstCaptChnPrm->stCaptChnAttr.uiCaptWid = CAPT_WIDTH;
        pstCaptChnPrm->stCaptChnAttr.uiCaptHei = CAPT_HEIGHT;

        pstCaptChnPrm->uiChn   = i;
        pstCaptChnPrm->uiStart = 0;
    }

    memset(&stIspCreateAttr, 0, sizeof(ISP_CREATE_ATTR));
    stIspCreateAttr.uiChnCnt = g_stCaptDrvStatus.uiCaptNum;

    for(i = 0; i < stIspCreateAttr.uiChnCnt; i++)
    {
        pstIspChnAttr = &stIspCreateAttr.stIspChanAttr[i];
        pstCaptChnPrm = (CAPT_CHN_PRM *)g_stCaptDrvStatus.uiCaptChnPrm[i];
        pstIspChnAttr->uiIspChnWid = pstCaptChnPrm->stCaptChnAttr.uiCaptWid;
        pstIspChnAttr->uiIspChnHei = pstCaptChnPrm->stCaptChnAttr.uiCaptHei;
        pstIspChnAttr->uiIspChnFps = pstCaptChnPrm->stCaptChnAttr.uiCaptFps;
        pstIspChnAttr->uiWdrMod    = 0; /* 初始化不使能宽动态 */
    }

    /* 初始化 MIPI 接口 */
    for(i = 0; i < g_stCaptDrvStatus.uiCaptNum; i++)
    {
        if (SAL_SOK != capt_hal_initCaptInterface(i))
        {
            CAPT_LOGE("Mipi Init Failed !!!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : CmdProc_captCmdProc
 功能描述  : DSP 模块capt的处理命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年10月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 CmdProc_captCmdProc(HOST_CMD cmd, UINT32 uiChn, void *prm)
{
    int                     iRet            = SAL_SOK;
    VIDEO_ROTATE_ATTR_ST    *pstVideoRotate = NULL;
    VIDEO_POSITION_ATTR_ST  *pstVideoPosInfo = NULL;
    DSPINITPARA *pstDspParam =  NULL;
    VIDEO_CHIP_BUILD_TIME_S *pstTime = NULL;
    CAPB_PRODUCT *pstProduct = capb_get_product();
    MEMORY_BUFF_S *pstMemory = NULL;

    if (NULL == pstProduct)
    {
        CAPT_LOGE("get platform capbility fail\n");
        return SAL_FAIL;
    }

    if (VIDEO_INPUT_INSIDE == pstProduct->enInputType)
    {
        return SAL_SOK;
    }

    CAPT_LOGI("set capt chn[%u] cmd[0x%x]\n", uiChn, cmd);

    switch(cmd)
    {
        case HOST_CMD_MODULE_VI: /* 采集模块的能力级 */
        {
            break;
        }
        case HOST_CMD_START_VIDEO_INPUT: /* 启动视频采集 */
        {
            if (SAL_SOK != capt_task_drvStart(uiChn))
            {
                CAPT_LOGE("Chn %d Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_STOP_VIDEO_INPUT: /* 停止视频采集 */
        {
            if (SAL_SOK != capt_task_drvStop(uiChn))
            {
                CAPT_LOGE("Chn %d Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_SET_VIDEO_PARM: /* 配置视频采集属性 */
        {
            if (SAL_SOK != capt_task_drvSetVideoPrm(uiChn))
            {
                CAPT_LOGE("Chn %d Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_SET_VIDEO_ROTATE: /* 设置视频输入的旋转属性 */
        {
            pstVideoRotate = (VIDEO_ROTATE_ATTR_ST *)prm;
            if (SAL_SOK != capt_hal_setCaptRotate(pstVideoRotate->uiChan, pstVideoRotate->eRotate))
            {
                CAPT_LOGE("Chn %d Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_SET_VIDEO_MIRROR: /* 设置视频镜像属性 */
        {
            break;
        }

        case HOST_CMD_SET_VIDEO_POS: /*  设置视频偏移位置属性 */
        {
            pstVideoPosInfo = (VIDEO_POSITION_ATTR_ST *)prm;
            if (SAL_SOK != capt_task_drvSetVideoPos(pstVideoPosInfo->uiChan, pstVideoPosInfo->posMode,pstVideoPosInfo->offset))
            {
                CAPT_LOGE("Chn %d Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_SET_EDID:     /* 配置EDID */
        {   
            pstDspParam = SystemPrm_getDspInitPara();
            if(NULL != pstDspParam)
            {
                iRet = capt_func_chipSetEdid(uiChn,pstDspParam->stViInitInfoSt.uiViChn, (CAPT_EDID_INFO_ST *)prm);
            }
            break;
        }
        case HOST_CMD_SET_CSC:
        {
            iRet = capt_func_chipSetCsc(uiChn, (VIDEO_CSC_S *)prm);
            break;
        }
        case HOST_CMD_SET_VIDEO_DISP_EDID:
        {
            pstDspParam = SystemPrm_getDspInitPara();
            if(NULL != pstDspParam)
            {
                iRet = capt_func_chipSetDispEdid(uiChn, pstDspParam->stViInitInfoSt.uiViChn, (CAPT_EDID_DISP_S *)prm);
            }
            break;
        }
        case HOST_CMD_GET_VIDEO_EDID_INFO:
        {
            iRet = capt_func_chipGetEdidInfo(uiChn, (CAPT_EDID_INFO_S *)prm);
            break;
        }
        
        case HOST_CMD_MCU_UPDATE:
        {
            pstMemory = (MEMORY_BUFF_S *)prm;
            iRet = capt_func_chipMcuFirmwareUpdate(uiChn, (const UINT8 *)pstMemory->pVirAddr, pstMemory->uiSize);
            break;
        }
        
        case HOST_CMD_MCU_BEAT_HEART_START:
        {
            iRet = capt_func_chipMcuheartBeatStart(uiChn);
            break;
        }
        case HOST_CMD_MCU_BEAT_HEART_STOP:
        {
            iRet = capt_func_chipMcuheartBeatStop(uiChn);
            break;
        }
        case HOST_CMD_GET_CHIP_BUILD_TIME:
        {
            pstTime = (VIDEO_CHIP_BUILD_TIME_S *)prm;
            iRet = capt_func_chipGetBuildTime(uiChn, pstTime);
            break;
        }
        case HOST_CMD_CHIP_RESET: /* 复位视频相关芯片MCU MSTAR FPGA */
        {
            iRet = capt_func_chipReset(uiChn);
            break;
        }
        #if 0
        case HOST_CMD_SET_ISP_PARAM:
        {
            pstIspPrm = (ISP_PARAM *)prm;
            if (SAL_SOK != capt_task_drvIspSetPrm(uiChn, pstIspPrm->cmd, pstIspPrm->value))
            {
                CAPT_LOGE("Chn %x Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        
        case HOST_CMD_GET_ISP_PARAM:
        {
            pstIspPrm = (ISP_PARAM *)prm;
            if (SAL_SOK != capt_task_drvIspGetPrm(uiChn,pstIspPrm->cmd, &pstIspPrm->value))
            {
                CAPT_LOGE("Chn %x Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        
        case  HOST_CMD_SENSOR_START:
        {
            capt_task_drvStartSensor( );
            iRet = SAL_SOK;
            break;
        }
        case  HOST_CMD_SENSOR_STOP:
        {
            capt_task_drvStopSensor( );
            iRet = SAL_SOK;
            break;
        }
        
        case  HOST_CMD_SET_VIDEO_WDR:
        {
            pstWdrPrm = (VI_WDR_PRM_ST *)prm;
            if (SAL_SOK != capt_task_drvSetVideoWdr(uiChn, pstWdrPrm->uiEnable))
            {
                CAPT_LOGE("Chn %x Host Cmd %x Failed !!!\n", uiChn, cmd);
                break;
            }
            iRet = SAL_SOK;
            break;
        }
        #endif
        default:
        {
            CAPT_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            iRet = SAL_FAIL;
            break;
        }
    }
    return iRet;
}


/*****************************************************************************
 函 数 名  : capt_task_opGetBlit
 功能描述  : Capt 模块数据的获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_tskGetChnPrm(CAPT_ChanHandle * pUsrArgs, CAPT_OBJ_PRM **ppstObjPrm)
{
    CAPT_OBJ_PRM      *pstObjPrm          = NULL;
    //SYSTEM_FRAME_INFO *pstSystemFrameInfo = NULL;

    CAPT_ChanHandle * pstChnPrm = NULL;
    UINT32            uiModChn  = 0;

    pstChnPrm = (CAPT_ChanHandle *)pUsrArgs;
    uiModChn  = sys_task_getModChn(pstChnPrm->module);
    
    pstObjPrm = g_stCaptObjCommon.pstObj[uiModChn];

    *ppstObjPrm = pstObjPrm;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tskobjInit
 功能描述  : Capt tsk obj 初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年10月09日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_tskobjInit(UINT32 uiIdx)
{
    CAPT_OBJ_PRM   *pstObjPrm = NULL;

    CAPT_LOGI("1.Capt tsk create start !!!\n");

    pstObjPrm = SAL_memZalloc(sizeof(CAPT_OBJ_PRM), CAPT_TSAK_NAME, CAPT_MEM_NAME);
    if (NULL == pstObjPrm)
    {
        CAPT_LOGE("Capt Obj Alloc Failed !!!\n");

        return SAL_FAIL;
    }

    memset(pstObjPrm, 0, sizeof(CAPT_OBJ_PRM));

    g_stCaptObjCommon.pstObj[uiIdx] = pstObjPrm;

    CAPT_LOGI("2.Capt tsk create done !!!\n");

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_task_opGetBlit
 功能描述  : Capt 模块数据的获取
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_task_opGetBlit(Ptr pUsrArgs, Ptr pBuf)
{
    CAPT_OBJ_PRM    *pstObjPrm = NULL;
    CAPT_ChanHandle *pstChnPrm = NULL;

    if (NULL == pUsrArgs)
    {
        CAPT_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pBuf)
    {
        CAPT_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    pstChnPrm = (CAPT_ChanHandle *)pUsrArgs;

    /* 获取通道属性 */
    if (SAL_SOK != capt_tskGetChnPrm(pstChnPrm, &pstObjPrm))
    {
        CAPT_LOGE("Capt Get Chn Prm Failed !!!\n");
        return SAL_FAIL;
    }

#if 0
    if (SAL_SOK != capt_task_drvGetFullBuff(uiChn, pBuffer))
    {
        CAPT_LOGE("!!!\n");
        return SAL_FAIL;
    }
#endif

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_opPutBlit
 功能描述  : Capt 模块数据释放
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_opPutBlit(Ptr pUsrArgs, Ptr pBuf)
{
    CAPT_OBJ_PRM    *pstObjPrm = NULL;
    CAPT_ChanHandle *pstChnPrm = NULL;

    if (NULL == pUsrArgs)
    {
        CAPT_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pBuf)
    {
        CAPT_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    pstChnPrm = (CAPT_ChanHandle *)pUsrArgs;

    /* 获取通道属性 */
    if (SAL_SOK != capt_tskGetChnPrm(pstChnPrm, &pstObjPrm))
    {
        CAPT_LOGE("Capt Get Chn Prm Failed !!!\n");
        return SAL_FAIL;
    }

#if 0
    if (SAL_SOK != capt_task_drvPutEmptBuff(uiChn, pBuffer))
    {
        CAPT_LOGE("!!!\n");
        return SAL_FAIL;
    }
#endif

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_opBindBlit
 功能描述  : Capt 模块绑定
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_opBindBlit(Ptr pUsrArgs, Ptr pDstBuf, UINT32 isBind)
{
    //CAPT_OBJ_PRM    *pstCaptObjPrm  = NULL;
    //CAPT_ChanHandle *pChanHandle   = NULL;

    ///CAPT_BIND_PRM   *pstCaptBindPrm = NULL;

    if (NULL == pUsrArgs)
    {
        CAPT_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        CAPT_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    //pChanHandle = (CAPT_ChanHandle *)pUsrArgs;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_task_opSetBlitPrm
 功能描述  : Capt 模块设置属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_task_opSetBlitPrm(Ptr pUsrArgs, Ptr pDstBuf)
{
    //CAPT_OBJ_PRM    *pstCaptObjPrm = NULL;
    //CAPT_ChanHandle *pstCaptChnPrm = NULL;

    if (NULL == pUsrArgs)
    {
        CAPT_LOGE("Chn Handle has no Obj !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pDstBuf)
    {
        CAPT_LOGE("Buff is NULL !!!\n");
        return SAL_FAIL;
    }

    //pstCaptChnPrm = (CAPT_ChanHandle *)pUsrArgs;

#if 0
    /* 获取通道属性 */
    if (SAL_SOK != capt_tskGetChnPrm(pstCaptChnPrm, &pstCaptObjPrm))
    {
        CAPT_LOGE("Capt Get Chn Prm Failed !!!\n");
        return SAL_FAIL;
    }
#endif

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tsk_askCreateChan
 功能描述  : Capt 模块 申请输出通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_tsk_askCreateChan(CAPT_ChanCreate *pChanArgs, CAPT_ChanHandle **pCaptChanHandle)
{
    CAPT_OBJ_PRM    *pstObjPrm   = NULL;
    CAPT_ChanHandle *pChanHandle = NULL;
    UINT32           uiCur       = 0;
    UINT32           uiModChn    = 0;

    if (NULL == pChanArgs)
    {
        CAPT_LOGE("Obj Handle is NULL !!!\n");
        return SAL_FAIL;
    }

    /* 获取通道属性 */
    uiModChn  = sys_task_getModChn(pChanArgs->module);
    pstObjPrm = g_stCaptObjCommon.pstObj[uiModChn];
    
    capt_tskChnMutexLock(pstObjPrm);
    
    if (0 == pstObjPrm->uiOutCnt)
    {
        uiCur = pstObjPrm->uiOutCnt;
        pstObjPrm->uiOutCnt++;
    }
    else
    {
        /* 只有一个通道 */
        CAPT_LOGE("Capt Out Chn Had Been Created , Out Cnt %d !!!\n", pstObjPrm->uiOutCnt);
        capt_tskChnMutexUnLock(pstObjPrm);
        return SAL_FAIL;
    }
    capt_tskChnMutexUnLock(pstObjPrm);

    pChanHandle = (CAPT_ChanHandle *)SAL_memZalloc(sizeof(CAPT_ChanHandle), CAPT_TSAK_NAME, CAPT_MEM_NAME);
    if (NULL == pChanHandle)
    {
        CAPT_LOGE("Capt Obj Create Out Chn Failed !!!\n");
        return SAL_FAIL;
    }
    pChanHandle->nMgicNum = uiCur;
    pstObjPrm->uiOutChn++;

    pChanHandle->module = pChanArgs->module;

    pChanHandle->ops.OpGetBlit    = capt_task_opGetBlit;
    pChanHandle->ops.OpPutBlit    = capt_task_opPutBlit;

    pChanHandle->ops.OpBindBlit   = capt_task_opBindBlit;
    pChanHandle->ops.OpSetBlitPrm = capt_task_opSetBlitPrm;

    pstObjPrm->pChanHandle      = pChanHandle;
    *pCaptChanHandle  = pChanHandle;

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_tsk_moduleDelete
 功能描述  : Capt 模块通道销毁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_tsk_moduleDelete(CaptHandle module)
{
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tsk_moduleCreate
 功能描述  : Capt 模块通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2018年11月06日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_tsk_moduleCreate(CAPT_MOD_CREATE_PRM * pCreate, CaptHandle *pHandle)
{
    CAPT_OBJ_PRM         *pstObjPrm = NULL;
    UINT32                uiObjIdx  = 0;
    CAPT_MOD_CREATE_PRM  *pCtrl     = (CAPT_MOD_CREATE_PRM *)pCreate;

    capt_tskMutexLock( );

    uiObjIdx = g_stCaptObjCommon.uiObjCnt;
    g_stCaptObjCommon.uiObjCnt++;

    capt_tskMutexUnLock( );

    if (NULL == pCreate)
    {
        CAPT_LOGE("Capt Obj Create, ModulesArgs is NULL !!!\n");
        return SAL_FAIL;
    }

    /* 1. obj 的创建 */
    if (SAL_SOK != capt_tskobjInit(uiObjIdx))
    {
        CAPT_LOGE("capt_tskobjInit Failed !!!\n");
        return SAL_FAIL;
    }

    pstObjPrm = g_stCaptObjCommon.pstObj[uiObjIdx];
    capt_tskChnrvMutexInit(pstObjPrm);

    /* 2. 采集通道的初始化 drv 层初始化 */
    if (SAL_SOK != capt_task_drvCreate(uiObjIdx, &pCtrl->stCaptCreateAttr))
    {
        CAPT_LOGE("capt_task_drvCreate err !!!\n");
        return SAL_FAIL;
    }

    /* 3. 保存信息 */
    pstObjPrm->uiModChn = uiObjIdx;
    pstObjPrm->uiOutChn = 0;
    *(UINT32 *) pHandle = sys_task_makeTskID(SYSTEM_MOD_ID_CAPT, uiObjIdx);
    
    CAPT_LOGI("capt_tsk_moduleCreate success !!!\n");

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : capt_tsk_deInit
* 描  述  : 删除采集模块
*         : - pHandle: CAPT模块句柄
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 capt_tsk_deInit( )
{
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_tsk_init
 功能描述  : capt 模块初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 capt_tsk_init(void)
{

    if (SAL_SOK != capt_task_drvInit())
    {
        CAPT_LOGE("capt_task_drvInit err !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


