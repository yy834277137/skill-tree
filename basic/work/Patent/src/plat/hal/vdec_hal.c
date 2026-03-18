/**
 * @file   vdec_hal.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  视频解码平台hal封装
 * @author dsp
 * @date   2022/7/4
 * @note
 * @note \n History
   1.日    期: 2022/7/4
     作    者: dsp
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "platform_hal.h"
#include "hal_inc_inter/vdec_hal_inter.h"

#line __LINE__ "vdec_hal.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
typedef struct
{
    VDEC_HAL_STATUS vdecStatus;     /*解码器状态参数*/
    VDEC_BIND_CTL vdecBindCtl;      /*解码绑定机制*/
    VDEC_HAL_PRM pVdecHalPrm;       /*解码相关平台控制句柄*/
    void *mutex;                    /*锁*/
} VDEC_HAL_CHAN_CTL;


typedef struct
{
    VDEC_HAL_CHAN_CTL vdecChanCtrl[MAX_VDEC_PLAT_CHAN];
    BOOL funCreate;
    VDEC_HAL_FUNC *func;
} VDEC_HAL_CTL;

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

static VDEC_HAL_CTL gVdecHalCtl = {0};

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   vdec_hal_getFreeChan
 * @brief	   获取可用通道
 * @param[in]
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_getFreeChan()
{
    INT32 chan = -1;
    INT32 i = 0;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;

    for (i = 0; i < MAX_VDEC_PLAT_CHAN; i++)
    {
        pVdecHalChanCtl = &gVdecHalCtl.vdecChanCtrl[i];
        if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_TRUE)
        {
            continue;
        }

        chan = i;
    }

    VDEC_LOGI("Get vdec chan %d free\n", chan);
    return chan;
}

#if 0
/**
 * @function   vdec_halRegister
 * @brief      弱函数当有平台不支持ive时保证编译通过
 * @param[in]  VDEC_HAL_FUNC *pstOsdHalOps  
 * @param[out] None
 * @return     __attribute__((weak) ) INT32
 */
__attribute__((weak) ) VDEC_HAL_FUNC *vdec_halRegister()
{
    return NULL;
}
#endif


/**
 * @function   vdec_hal_funcRegister
 * @brief	   hal层注册
 * @param[in]
 * @param[out] None
 * @return	INT32
 */
static INT32 vdec_hal_funcRegister()
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;

    pVdecHalCtl = &gVdecHalCtl;

    if (pVdecHalCtl->funCreate == 0)
    {
        pVdecHalCtl->func = vdec_halRegister();
        CHECK_PTR_NULL(pVdecHalCtl->func, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
        pVdecHalCtl->funCreate = 1;
        VDEC_LOGI("VDEC register func SUCCESS\n");
    }

    return s32Ret;
}

/**
 * @function   vdec_hal_Init
 * @brief	   hal层初始化
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[out] None
 * @return	INT32
 */
INT32 vdec_hal_Init(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = vdec_hal_funcRegister();
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal init failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    memset(&gVdecHalCtl.vdecChanCtrl[u32VdecChn], 0x00, sizeof(VDEC_HAL_CHAN_CTL));

    return s32Ret;
}

/**
 * @function   vdec_hal_MemAlloc
 * @brief	   申请内存
 * @param[in]  UINT32 u32Format 格式
 * @param[in]  void *prm 入参
 * @param[out] None
 * @return
 */
VOID *vdec_hal_MemAlloc(UINT32 u32Format, void *prm)
{
    return NULL;
}

/**
 * @function   vdec_hal_MemFree
 * @brief	   申请释放
 * @param[in]  void *ptr 指针
 * @param[in]  void *prm 入参
 * @param[out] None
 * @return INT32
 */
INT32 vdec_hal_MemFree(void *ptr, void *prm)
{
    return SAL_FAIL;
}

/**
 * @function   vdec_hal_VdecJpegSupport
 * @brief	   是否支持jpeg解码
 * @param[in]
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecJpegSupport()
{
    INT32 s32Ret = SAL_FAIL;

    if (VDEC_SUPPORT_JPEG)
    {
        s32Ret = SAL_SOK;
    }

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecGetBind
 * @brief	   解码器是否需要与其他模块绑定
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_BIND_CTL *pVdecBindCtl 参数
 * @param[out] None
 * @return	 INT32
 */
INT32 vdec_hal_VdecGetBind(UINT32 u32VdecChn, VDEC_BIND_CTL *pVdecBindCtl)
{
    INT32 s32Ret = SAL_FAIL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(pVdecBindCtl, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    if (VDEC_BIND)
    {
        pVdecBindCtl->needBind = 1;
        pVdecBindCtl->modeDataRes = MODE_SRC;
    }

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecCreate
 * @brief	   解码器创建
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_PRM *vdecPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecCreate(UINT32 u32VdecChn, VDEC_PRM *vdecPrm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(vdecPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_TRUE)
    {
        VDEC_LOGE("chn %d has been used\n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN);
    }

    CHECK_PTR_NULL(pstVdecHalFunc->VdecCreate, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalPrm->vdecPrm.decWidth = vdecPrm->decWidth;
    pVdecHalPrm->vdecPrm.decHeight = vdecPrm->decHeight;
    pVdecHalPrm->vdecPrm.encType = vdecPrm->encType;

    s32Ret = pstVdecHalFunc->VdecCreate(u32VdecChn, pVdecHalPrm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec Create failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pVdecHalChanCtl->mutex);
    pVdecHalChanCtl->vdecStatus.bcreate = SAL_TRUE;

    vdec_hal_VdecGetBind(u32VdecChn, &vdecPrm->vdecBindCtl);

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecStart
 * @brief	   启动解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecStart(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_FALSE)
    {
        VDEC_LOGE("chn %d has not been used\n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN);
    }

    CHECK_PTR_NULL(pstVdecHalFunc->VdecStart, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecStart(pVdecHalPrm->decPlatHandle);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec start failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    pVdecHalChanCtl->vdecStatus.bstart = SAL_TRUE;

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecStop
 * @brief	   停止解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecStop(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecStop, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecStop(pVdecHalPrm->decPlatHandle);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec stop failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    pVdecHalChanCtl->vdecStatus.bstart = SAL_FALSE;

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecDeinit(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecDeinit, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecDeinit(pVdecHalPrm->decPlatHandle);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec deinit failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    pVdecHalChanCtl->vdecStatus.curChnDecoding = 0;
    pVdecHalChanCtl->vdecStatus.bcreate = SAL_FALSE;
    SAL_mutexDelete(pVdecHalChanCtl->mutex);
    memset(pVdecHalChanCtl, 0x00, sizeof(VDEC_HAL_CHAN_CTL));

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_PRM *vdecPrm 设置解码器参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecSetPrm(UINT32 u32VdecChn, VDEC_PRM *vdecPrm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(vdecPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_FALSE)
    {
        VDEC_LOGE("chn %d has not been used\n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN);
    }

    pVdecHalPrm->vdecPrm.decWidth = vdecPrm->decWidth;
    pVdecHalPrm->vdecPrm.decHeight = vdecPrm->decHeight;
    pVdecHalPrm->vdecPrm.encType = vdecPrm->encType;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecSetPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecSetPrm(u32VdecChn, pVdecHalPrm);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec SetPrm failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecReset
 * @brief	   复位解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecReset(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_FALSE)
    {
        VDEC_LOGE("chn %d has not been used\n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN);
    }

    CHECK_PTR_NULL(pstVdecHalFunc->VdecReset, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecReset(pVdecHalPrm->decPlatHandle);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec Reset failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecClear
 * @brief	   清除解码器内缓存
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecClear(UINT32 u32VdecChn)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_FALSE)
    {
        VDEC_LOGE("chn %d has not been used\n", u32VdecChn);
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_INVALID_CHAN);
    }

    CHECK_PTR_NULL(pstVdecHalFunc->VdecClear, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecClear(pVdecHalPrm->decPlatHandle);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("chn %d hal Vdec Clear failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    VDEC_LOGI("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecDecframe
 * @brief	   解码一帧数据
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in] SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in] SAL_VideoFrameBuf *pFrameOut 解码后数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecDecframe(UINT32 u32VdecChn, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut, UINT32 u32TimeOut)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));

    CHECK_PTR_NULL(pFrameIn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pFrameOut, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_FALSE || pVdecHalChanCtl->vdecStatus.bstart == SAL_FALSE)
    {
        SAL_msleep(u32TimeOut * 10);
        VDEC_LOGD("chn %d has not been used\n", u32VdecChn);   /*hal层不加打印只返回错误码*/
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNREADY);
    }

    CHECK_PTR_NULL(pstVdecHalFunc->VdecDecframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecDecframe(pVdecHalPrm->decPlatHandle, pFrameIn, pFrameOut);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        SAL_msleep(u32TimeOut);
        VDEC_LOGD("chn %d hal Vdec dec frame failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    pVdecHalChanCtl->vdecStatus.curChnDecoding = 1;
    VDEC_LOGD("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecMakeframe
 * @brief	   生成一帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据
 * @param[in] void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstframe)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;

    CHECK_PTR_NULL(pPicFramePrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecMakeframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    s32Ret = pstVdecHalFunc->VdecMakeframe(pPicFramePrm, pstframe);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("hal Vdec Cpyframe failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
};

/**
 * @function   vdec_hal_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcframe, void *pstDstframe)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;

    CHECK_PTR_NULL(pstSrcframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pstDstframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecCpyframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    s32Ret = pstVdecHalFunc->VdecCpyframe(copyEn, pstSrcframe, pstDstframe);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("hal Vdec Cpyframe failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecGetframe
 * @brief	   获取数据帧
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecGetframe(UINT32 u32VdecChn, UINT32 dstChn, void *pstframe,UINT32 u32TimeOut)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(pstframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    if (pVdecHalChanCtl->vdecStatus.bcreate == SAL_FALSE || pVdecHalChanCtl->vdecStatus.bstart == SAL_FALSE)
    {
        VDEC_LOGD("chn %d has not been used\n", u32VdecChn);
#ifndef DISP_VIDEO_FUSION
        SAL_msleep(u32TimeOut * 10);    /*未初始化等待增加10帧等待周期*/
#else
        SAL_msleep(10); /* 规避NT平台通道0送无信号帧时，休眠时间过长导致的帧率过低问题 */
#endif
        return DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_UNREADY);
    }

    CHECK_PTR_NULL(pstVdecHalFunc->VdecGetframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));


    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecGetframe(pVdecHalPrm->decPlatHandle, dstChn, pstframe);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("chn %d hal Vdec Getframe failed ret 0x%x\n", u32VdecChn, s32Ret);
        SAL_msleep(u32TimeOut);
        return s32Ret;
    }

    VDEC_LOGD("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecReleaseframe
 * @brief	   释放数据帧
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecReleaseframe(UINT32 u32VdecChn, UINT32 dstChn, void *pstframe)
{
    INT32 s32Ret = SAL_SOK;

    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;
    VDEC_HAL_PRM *pVdecHalPrm = NULL;


    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(pstframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];
    pVdecHalPrm = &pVdecHalChanCtl->pVdecHalPrm;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecGetframe, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pVdecHalPrm->decPlatHandle, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    SAL_mutexLock(pVdecHalChanCtl->mutex);
    s32Ret = pstVdecHalFunc->VdecReleaseframe(pVdecHalPrm->decPlatHandle, dstChn, pstframe);
    SAL_mutexUnlock(pVdecHalChanCtl->mutex);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("chn %d hal Vdec Releaseframe failed ret 0x%x\n", u32VdecChn, s32Ret);
        return s32Ret;
    }

    VDEC_LOGD("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_VdecChnStatus
 * @brief	   当前解码器状态
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_STATUS *pvdecStatus 状态
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecChnStatus(UINT32 u32VdecChn, VDEC_HAL_STATUS *pvdecStatus)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_CHAN_CTL *pVdecHalChanCtl = NULL;

    CHECK_CHAN(u32VdecChn, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_ILLEGAL_PARAM));
    CHECK_PTR_NULL(pvdecStatus, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pVdecHalChanCtl = &pVdecHalCtl->vdecChanCtrl[u32VdecChn];

    memcpy(pvdecStatus, &pVdecHalChanCtl->vdecStatus, sizeof(VDEC_HAL_STATUS));

    VDEC_LOGD("chn %d [%s %d]\n", u32VdecChn, __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_MallocYUVMem
 * @brief	   申请yuv内存
 * @param[in]  UINT32 u32width 宽
 * @param[in]  UINT32 u32height 高
 * @param[in]  HAL_MEM_PRM *addrPrm 数据地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_MallocYUVMem(UINT32 u32width, UINT32 u32height, HAL_MEM_PRM *addrPrm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;

    CHECK_PTR_NULL(addrPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pstVdecHalFunc = pVdecHalCtl->func;

    s32Ret = pstVdecHalFunc->VdecMallocYUVMem(u32width, u32height, addrPrm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("hal Vdec alloc yuv mem failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    VDEC_LOGI("[%s %d]\n", __func__, __LINE__);

    return s32Ret;
}

/**
 * @function   vdec_hal_FreeYUVMem
 * @brief	   释放yuv内存
 * @param[in]  HAL_MEM_PRM *addrPrm 数据地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_FreeYUVMem(HAL_MEM_PRM *addrPrm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;

    CHECK_PTR_NULL(addrPrm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecFreeYuvPoolBlockMem, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    s32Ret = pstVdecHalFunc->VdecFreeYuvPoolBlockMem(addrPrm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("hal Vdec alloc yuv mem failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function   vdec_hal_Mmap
 * @brief	   帧数据映射地址
 * @param[in]  HAL_MEM_PRM *addrPrm 数据地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_Mmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm)
{
    INT32 s32Ret = SAL_SOK;
    VDEC_HAL_CTL *pVdecHalCtl = NULL;
    VDEC_HAL_FUNC *pstVdecHalFunc = NULL;

    CHECK_PTR_NULL(pVideoFrame, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    CHECK_PTR_NULL(pPicprm, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));

    pVdecHalCtl = &gVdecHalCtl;
    pstVdecHalFunc = pVdecHalCtl->func;

    CHECK_PTR_NULL(pstVdecHalFunc->VdecMmap, DSP_DEF_ERR(MOD_MEDIA_DEC, 0, ERR_DSP_NULL_PTR));
    s32Ret = pstVdecHalFunc->VdecMmap(pVideoFrame, pPicprm);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGD("hal Vdec alloc yuv mem failed ret 0x%x\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

