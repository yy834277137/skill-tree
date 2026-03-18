
/*******************************************************************************
* svp_dsp_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : cuifeng5
* Version: V1.0.0  2020年05月11日 Create
*
* Description :
* Modification:
*******************************************************************************/

#include "svp_dsp_drv_api.h"

#define SVP_DSP_TASK_PROCESS_TIMEOUT (20)

/* 该数组是为了方便添加新的指令，后续有新的指令添加时，在该数组后面新增项即可 */
static SVP_DSP_TASK_INIT_PARAM_S gastTaskInitParam[] =
{
    {SVP_DSP_CMD_LINE_OSD_USER, sizeof(SVP_DSP_LINE_OSD_PARAM_S)},
};

static UINT32 gu32DspCoreNum = 0;
static SVP_DSP_CORE_STATUS_E gaenDspCoreStatus[SVP_DSP_CORE_NUM_MAX] = {SVP_DSP_CORE_STATUS_FREE};
static DSA_QueHndl gstDspTaskQue;
static pthread_mutex_t gstDspTaskMutex;
static BOOL gbTimeTest = SAL_FALSE;
static UINT32 gu32TimeTestNum = 0;




/*******************************************************************************
* 函数名  : svpdsp_drvTaskMutexLock
* 描  述  :
* 输  入  :
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_drvTaskMutexLock(VOID)
{
    if (0 != pthread_mutex_lock(&gstDspTaskMutex))
    {
        SVP_LOGE("lock dsp buff mutex failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : svpdsp_drvTaskMutexUnlock
* 描  述  :
* 输  入  :
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_drvTaskMutexUnlock(VOID)
{
    if (0 != pthread_mutex_unlock(&gstDspTaskMutex))
    {
        SVP_LOGE("lock dsp buff mutex failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : svpdsp_drvSetTimeTest
* 描  述  : DSP时间测试开关
* 输  入  :
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_drvSetTimeTest(INT32 s32Test, INT32 s32Num)
{
    gbTimeTest = (s32Test > 0) ? SAL_TRUE : SAL_FALSE;
    gu32TimeTestNum = s32Num;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : svpdsp_func_init
* 描  述  : DSP模块初始化
* 输  入  :
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_func_init(UINT32 u32CoreNum, UINT32 *pu32Core)
{
    INT32 ret = SAL_SOK;
    UINT64 u64PhyAddr = 0;
    UINT64 u64TmpPhyAddr = 0;
    UINT8 *pu8Buff = NULL;
    UINT8 *pu8TmpBuff = NULL;
    UINT32 u32CmdCnt = sizeof(gastTaskInitParam) / sizeof(gastTaskInitParam[0]);
    SVP_DSP_TASK_INIT_PARAM_S *pstTaskInitParam = NULL;
    UINT32 u32ParamLen = 0;
    UINT64 u64BuffSize = 0;
    SVP_DSP_TASK_BUFF_S *pstTaskBuff = NULL;
    DSA_QueHndl *pstFreeBuffQue = NULL;
    UINT32 i = 0, j = 0;
	
	ret = svpdsp_hal_initDspCore(u32CoreNum, pu32Core);
    if (SAL_SOK != ret)
    {
        SVP_LOGE("svpdsp_hal_init fail\n");
        return SAL_FAIL;
    }

    ret = DSA_QueCreate(&gstDspTaskQue, SVP_DSP_TASK_NUM_MAX);
    if (ret != SAL_SOK)
    {
        SVP_LOGE("create dsp task que fail:0x%x\n", ret);
        return SAL_FAIL;
    }

    pstTaskInitParam = gastTaskInitParam;
    for (i = 0; i < u32CmdCnt; i++)
    {
        pstTaskInitParam->u32ParamLen = SAL_align(pstTaskInitParam->u32ParamLen, 8);
        u64BuffSize += pstTaskInitParam->u32ParamLen;

        /* 该队列用于缓存不足时，排队获取缓存 */
        ret = DSA_QueCreate(&pstTaskInitParam->stFreeBuffQue, SVP_DSP_TASK_BUFF_NUM);
        if (ret != SAL_SOK)
        {
            SVP_LOGE("create draw line buff task que fail:0x%x\n", ret);
            goto out;
        }

        pstTaskInitParam++;
    }

    /* ARM核与DSP核通信的参数必须保存在MMZ内存低地址中 */
    u64BuffSize *= SVP_DSP_TASK_BUFF_NUM;
    ret = mem_hal_mmzAlloc(u64BuffSize, "svpdsp_drv", "svpdsp", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pu8Buff);
    if (SAL_SOK != ret)
    {
        SVP_LOGE("alloc dsp buf failed:0x%x\n", ret);
        goto out;
    }

    /* 将申请的缓存分块保存 */
    pstTaskInitParam = gastTaskInitParam;
    u64TmpPhyAddr = u64PhyAddr;
    pu8TmpBuff = pu8Buff;
    for (i = 0; i < u32CmdCnt; i++)
    {
        pstTaskBuff = pstTaskInitParam->astTaskBuff;
        u32ParamLen = pstTaskInitParam->u32ParamLen;
        pstFreeBuffQue = &pstTaskInitParam->stFreeBuffQue;
        for (j = 0; j < SVP_DSP_TASK_BUFF_NUM; j++)
        {
            pstTaskBuff->u64PhyAddr = u64TmpPhyAddr;
            pstTaskBuff->pvVirAddr = (VOID *)pu8TmpBuff;
            pstTaskBuff->u32BuffLen = u32ParamLen;

            (VOID)DSA_QuePut(pstFreeBuffQue, (VOID *)pstTaskBuff, SAL_TIMEOUT_NONE);

            u64TmpPhyAddr += u32ParamLen;
            pu8TmpBuff += u32ParamLen;
            pstTaskBuff++;
        }

        pstTaskInitParam++;
    }

    pthread_mutex_init(&gstDspTaskMutex, NULL);

    gu32DspCoreNum = svpdsp_hal_getCoreNum();

    for (i = 0; i < SVP_DSP_CORE_NUM_MAX; i++)
    {
        gaenDspCoreStatus[i] = SVP_DSP_CORE_STATUS_FREE;
    }

    SVP_LOGI("svp dsp init success\n");

    return SAL_SOK;

out:
    pstTaskInitParam = gastTaskInitParam;
    u32CmdCnt = i;
    for (i = 0; i < u32CmdCnt; i++)
    {
        (VOID)DSA_QueDelete(&pstTaskInitParam->stFreeBuffQue);
        pstTaskInitParam++;
    }

    (VOID)DSA_QueDelete(&gstDspTaskQue);

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : svpdsp_func_getBuff
* 描  述  : 获取用于存放向DSP发送参数的缓存
* 输  入  : UINT32 enCmd : 向DSP核发送的指令
* 输  出  : SVP_DSP_TASK_BUFF_S **ppDspTaskBuff : 获取到的缓存的地址
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_func_getBuff(UINT32 enCmd, SVP_DSP_TASK_S *pstDspTask, SVP_DSP_BUFF_MODE_E enBuffMode)
{
    DSA_QueHndl *pstFreeBuffQue = NULL;
    SVP_DSP_TASK_INIT_PARAM_S *pstTaskInitParam = NULL;
    UINT32 u32CmdCnt = sizeof(gastTaskInitParam) / sizeof(gastTaskInitParam[0]);
    UINT32 i = 0;

    pstTaskInitParam = gastTaskInitParam;
    for (i = 0; i < u32CmdCnt; i++)
    {
        if (enCmd == pstTaskInitParam->enCmd)
        {
            pstFreeBuffQue = &pstTaskInitParam->stFreeBuffQue;
            break;
        }

        pstTaskInitParam++;
    }

    if (i >= u32CmdCnt)
    {
        SVP_LOGE("unsupport cmd:%d\n", enCmd);
        pstDspTask->pstTaskBuff = NULL;
        return SAL_FAIL;
    }

    (VOID)DSA_QueGet(pstFreeBuffQue, (VOID *)&pstDspTask->pstTaskBuff, SAL_TIMEOUT_FOREVER);

    pstDspTask->enDspCmd = enCmd;
    pstDspTask->pstTaskBuff->enBuffMode = enBuffMode;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : svpdsp_func_releaseBuff
* 描  述  : 释放缓存
* 输  入  : SVP_DSP_TASK_S *pstDspTask: DSP任务结构体指针
* 输  出  :
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_func_releaseBuff(SVP_DSP_TASK_S *pstDspTask)
{
    DSA_QueHndl *pstFreeBuffQue = NULL;
    SVP_DSP_TASK_INIT_PARAM_S *pstTaskInitParam = NULL;
    UINT32 u32CmdCnt = sizeof(gastTaskInitParam) / sizeof(gastTaskInitParam[0]);
    UINT32 i = 0;

    pstTaskInitParam = gastTaskInitParam;
    for (i = 0; i < u32CmdCnt; i++)
    {
        if (pstDspTask->enDspCmd == pstTaskInitParam->enCmd)
        {
            pstFreeBuffQue = &pstTaskInitParam->stFreeBuffQue;
            break;
        }

        pstTaskInitParam++;
    }

    if (i >= u32CmdCnt)
    {
        SVP_LOGE("unsupport cmd:%d\n", pstDspTask->enDspCmd);
        return SAL_FAIL;
    }

    (VOID)DSA_QuePut(pstFreeBuffQue, (VOID *)pstDspTask->pstTaskBuff, SAL_TIMEOUT_FOREVER);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : svpdsp_drvTaskRunContinue
* 描  述  : DSP执行完一次指令后，从队列中取出下条指令继续执行
* 输  入  : SVP_DSP_TASK_S *pstLastTask : 上次执行完成的任务指针
          UINT32 u32RlsFlag : 释放资源标志
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_drvTaskRunContinue(SVP_DSP_TASK_S *pstLastTask, UINT32 u32RlsFlag)
{
    SVP_DSP_TASK_S *pstDspTask = NULL;
    DSA_QueHndl *pstFreeBuffQue = NULL;
    SVP_DSP_TASK_INIT_PARAM_S *pstTaskInitParam = NULL;
    UINT32 u32CmdCnt = sizeof(gastTaskInitParam) / sizeof(gastTaskInitParam[0]);
    UINT32 i = 0;

    pstTaskInitParam = gastTaskInitParam;
    for (i = 0; i < u32CmdCnt; i++)
    {
        if (pstLastTask->enDspCmd == pstTaskInitParam->enCmd)
        {
            pstFreeBuffQue = &pstTaskInitParam->stFreeBuffQue;
            break;
        }

        pstTaskInitParam++;
    }

    if (i >= u32CmdCnt)
    {
        SVP_LOGE("unsupport cmd:%d\n", pstLastTask->enDspCmd);
        return SAL_FAIL;
    }

    /* 释放缓存 */
    if ((u32RlsFlag & SVP_DSP_RLS_BUFF_FLAG) > 0)
    {
        if (SVP_DSP_BUFF_MODE_ONCE == pstLastTask->pstTaskBuff->enBuffMode)
        {
            (VOID)DSA_QuePut(pstFreeBuffQue, (VOID *)pstLastTask->pstTaskBuff, SAL_TIMEOUT_FOREVER);
        }
    }

    if ((u32RlsFlag & SVP_DSP_RLS_CORE_FLAG) > 0)
    {
        if (SAL_SOK == DSA_QueGet(&gstDspTaskQue, (VOID **)&pstDspTask, SAL_TIMEOUT_NONE))
        {
            /* 有等待执行的任务 */
            pstDspTask->u32CoreId = pstLastTask->u32CoreId;
            pstDspTask->enTaskStatus = SVP_DSP_TASK_STATUS_READY;
        }
        else
        {
            /* 无等待执行的任务 */
            (VOID)svpdsp_drvTaskMutexLock();
            gaenDspCoreStatus[pstLastTask->u32CoreId] = SVP_DSP_CORE_STATUS_FREE;
            (VOID)svpdsp_drvTaskMutexUnlock();
        }
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : svpdsp_func_runTask
* 描  述  : 调用DSP执行任务
* 输  入  : SVP_DSP_TASK_S *pstDspTask : 任务指针
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_func_runTask(SVP_DSP_TASK_S *pstDspTask)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;

    (VOID)svpdsp_drvTaskMutexLock();
    for (i = 0; i < gu32DspCoreNum; i++)
    {
        if (SVP_DSP_CORE_STATUS_FREE == gaenDspCoreStatus[i])
        {
            pstDspTask->u32CoreId = i;
            gaenDspCoreStatus[i] = SVP_DSP_CORE_STATUS_BUSY;
            break;
        }
    }
    (VOID)svpdsp_drvTaskMutexUnlock();

    /* 存在DSP核处于空闲状态，直接运行 */
    if (i < gu32DspCoreNum)
    {
        s32Ret = svpdsp_hal_setTask(pstDspTask->u32CoreId, pstDspTask->enDspCmd, pstDspTask->pstTaskBuff->u64PhyAddr, pstDspTask->pstTaskBuff->u32BuffLen);
        if (SAL_SOK != s32Ret)
        {
            SVP_LOGE("svpdsp_hal_setTask fail, CoreId:%u, Cmd:%d\n", pstDspTask->u32CoreId, pstDspTask->enDspCmd);
            return SAL_FAIL;
        }

        pstDspTask->enTaskStatus = SVP_DSP_TASK_STATUS_RUN;
        return SAL_SOK;
    }

    /* 当前无空闲DSP核，添加到队列 */
    pstDspTask->enTaskStatus = SVP_DSP_TASK_STATUS_WAITTING;
    DSA_QuePut(&gstDspTaskQue, (VOID *)pstDspTask, SAL_TIMEOUT_NONE);

    return s32Ret;
}


/*******************************************************************************
* 函数名  : svpdsp_func_waitTaskFinish
* 描  述  : 等待DSP执行任务完成
* 输  入  : SVP_DSP_TASK_S *pstDspTask : 任务指针
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_func_waitTaskFinish(SVP_DSP_TASK_S *pstDspTask)
{
    BOOL bFinish = SAL_FALSE;

    while (SVP_DSP_TASK_STATUS_WAITTING == pstDspTask->enTaskStatus)
    {
        usleep(1000);
    }

    while (SAL_SOK == svpdsp_hal_getStatus(pstDspTask->u32CoreId, &bFinish))
    {
        if (SAL_TRUE == bFinish)
        {
            break;
        }

        usleep(1000);
    }

    (VOID)svpdsp_drvTaskRunContinue(pstDspTask, SVP_DSP_RLS_BUFF_FLAG | SVP_DSP_RLS_CORE_FLAG);
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : svpdsp_func_getVideoFrame
* 描  述  : 获取送往DSP的Frame格式
* 输  入  : VIDEO_FRAME_INFO_S *pstVideoFrame : ARM端帧
* 输  出  : SVP_DSP_VIDEO_FRAME_S *pstDspFrame : DSP端帧
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline VOID svpdsp_func_getVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SVP_DSP_VIDEO_FRAME_S *pstDspFrame)
{
    UINT32 i = 0;

    pstDspFrame->u32Width = pVideoFrame->frameParam.width;
    pstDspFrame->u32Height = pVideoFrame->frameParam.height;
    for (i = 0; i < 3; i++)
    {
        pstDspFrame->au64PhyAddr[i] = pVideoFrame->phyAddr[i];
        pstDspFrame->au64VirAddr[i] = pVideoFrame->virAddr[i];
        pstDspFrame->au32Stride[i] = pVideoFrame->stride[i];
    }

    return;
}

/*******************************************************************************
* 函数名  : svpdsp_func_getLineType
* 描  述  : 获取DSP画线的类型
* 输  入  : DISPLINE_PRM *pstSrc : ARM端
* 输  出  : SVP_DSP_LINE_TYPE_S *pstDst : DSP端
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline VOID svpdsp_func_getLineType(DISPLINE_PRM *pstSrc, SVP_DSP_LINE_TYPE_S *pstDst)
{
    pstDst->u32FrameType = pstSrc->frametype;
    pstDst->u32FullLength = pstSrc->fulllinelength;
    pstDst->u32Gaps = pstSrc->gaps;
    pstDst->u32Node = pstSrc->node;

    return;
}

