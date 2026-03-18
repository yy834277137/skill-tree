/*******************************************************************************
* sys_hostcmd_proc.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年10月12日 Create
*
* Description : 系统模块主机命令处理模块
* Modification:
*******************************************************************************/

#include "sal.h"
#include "system_common_api.h"
#include "dspcommon.h"
#include "system_prm_api.h"
#include "sys_hostcmd_proc_api.h"
#include "capbility.h"
#include "platform_hal.h"
#include "disp_tsk_api.h"
#include "disp_tsk_inter.h"
#include "osd_tsk_api.h"
#include "task_ism.h"
#include "dspdebug_host.h"
#include "svp_dsp_drv_api.h"
#include "link_task_api.h"

/*****************************************************************************
                                宏定义
*****************************************************************************/
#define SYS_HOSTCMD_PROC_NAME	"sys_hostcmd_proc"
#define POOL_SIZE				(2 * 1024 * 1024)
#define NET_POOL_SIZE			(POOL_SIZE)
#define REC_POOL_SIZE			(POOL_SIZE)
#define DEC_POOL_SIZE			(POOL_SIZE)
#define MIN_POOL_SIZE			(POOL_SIZE)

#define XRAY_RT_SHARE_BUF_SIZE	(4 * 1024 * 1024)
#define XRAY_PB_SHARE_BUF_SIZE	(8 * 1024 * 1024)

typedef struct DSP_VERSION_PARAM
{
    CHAR szOrigin[80];
    CHAR szBuildTime[40];
    CHAR szDSPVersion[40];
    CHAR szBuilder[40];
}DSP_VERSION_PARAM;

#line __LINE__ "sys_hostcmd_proc.c"

/*****************************************************************************
                                全局结构体
*****************************************************************************/

/*****************************************************************************
                                函数
*****************************************************************************/

/*******************************************************************************
* 函数名  : HostCmd_deinitDspPrm
* 描  述  :
* 输  入  : - :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 HostCmd_deinitDspPrm()
{
    int i = 0;
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    REC_POOL_INFO *pstRecPool = NULL;
    NET_POOL_INFO *pstNetPool = NULL;
    DEC_SHARE_BUF *pstDecShare = NULL;

    /* 编码码流 */
    for (i = 0; i < pstInit->encChanCnt; i++)
    {
        /* 分配结构体空间 */
        pstNetPool = &pstInit->NetPoolMain[i];
        if (NULL != pstNetPool)
        {
            if (NULL != pstNetPool->vAddr)
            {
                SAL_memfree(pstNetPool->vAddr, SYS_HOSTCMD_PROC_NAME, "venc_main");
                pstNetPool->vAddr = NULL;
            }
        }

        pstNetPool = &pstInit->NetPoolSub[i];
        if (NULL != pstNetPool)
        {
            if (NULL != pstNetPool->vAddr)
            {
                SAL_memfree(pstNetPool->vAddr, SYS_HOSTCMD_PROC_NAME, "venc_sub");
                pstNetPool->vAddr = NULL;
            }
        }

        pstRecPool = &pstInit->RecPoolMain[i];
        if (NULL != pstRecPool)
        {
            if (NULL != pstRecPool->vAddr)
            {
                SAL_memfree(pstRecPool->vAddr, SYS_HOSTCMD_PROC_NAME, "rec_main");
                pstRecPool->vAddr = NULL;
            }
        }

        pstRecPool = &pstInit->RecPoolSub[i];
        if (NULL != pstRecPool)
        {
            if (NULL != pstRecPool->vAddr)
            {
                SAL_memfree(pstRecPool->vAddr, SYS_HOSTCMD_PROC_NAME, "rec_sub");
                pstRecPool->vAddr = NULL;
            }
        }
    }

    /* 解码码流 */
    for (i = 0; i < pstInit->decChanCnt; i++)
    {
        pstDecShare = &pstInit->decShareBuf[i];

        if (NULL != pstDecShare)
        {
            if (NULL != pstDecShare->pVirtAddr)
            {
                SAL_memfree(pstRecPool->vAddr, SYS_HOSTCMD_PROC_NAME, "dec");
                pstDecShare->pVirtAddr = NULL;
            }
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : HostCmd_initDspPrm
* 描  述  :
* 输  入  : - :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 HostCmd_initDspPrm()
{
    int i = 0;
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();
    REC_POOL_INFO *pstRecPool = NULL;
    NET_POOL_INFO *pstNetPool = NULL;
    DEC_SHARE_BUF *pstDecShare = NULL;

    #ifndef DOUBLE_CHIP_SLAVE_CHIP
    XRAY_SHARE_BUF *pstXrayShare = NULL;
    #endif

    for (i = 0; i < MAX_NO_SIGNAL_PIC_CNT; i++)
    {
        SYS_LOGW("  stCaptNoSignalInfo.uiNoSignalImgSize[%d] size=%d vAddr=%p\n", i, pstInit->stCaptNoSignalInfo.uiNoSignalImgSize[i], \
                    pstInit->stCaptNoSignalInfo.uiNoSignalImgAddr[i]);
    }

    /* 编码码流 */
    SYS_LOGW("pstInit->encChanCnt: %u **********\n",        pstInit->encChanCnt); 
    #ifndef DOUBLE_CHIP_SLAVE_CHIP
    for (i = 0; i < pstInit->encChanCnt; i++)
    {
        /* 分配实时预览码流的空间 */
        pstNetPool = &pstInit->NetPoolMain[i];

        if (NULL == pstNetPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstNetPool->totalLen = (pstNetPool->totalLen > MIN_POOL_SIZE) ? pstNetPool->totalLen : NET_POOL_SIZE;

            pstNetPool->vAddr = (PUINT8) SAL_memZalloc(pstNetPool->totalLen, SYS_HOSTCMD_PROC_NAME, "venc_main");
            if (NULL == pstNetPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: NetPoolMain[%d].totalLen = %d vAddr=%p\n", i, pstNetPool->totalLen, pstNetPool->vAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: NetPoolMain[%d].totalLen = %d vAddr=%p\n", i, pstNetPool->totalLen, pstNetPool->vAddr);
        }

        pstNetPool = &pstInit->NetPoolSub[i];

        if (NULL == pstNetPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstNetPool->totalLen = (pstNetPool->totalLen > MIN_POOL_SIZE) ? pstNetPool->totalLen : NET_POOL_SIZE;

            pstNetPool->vAddr = (PUINT8) SAL_memZalloc(pstNetPool->totalLen, SYS_HOSTCMD_PROC_NAME, "venc_sub");
            if (NULL == pstNetPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }

            SYS_LOGW("DSP malloc: NetPoolSub[%d].totalLen  = %d vAddr=%p \n", i, pstNetPool->totalLen, pstNetPool->vAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: NetPoolSub[%d].totalLen  = %d vAddr=%p \n", i, pstNetPool->totalLen, pstNetPool->vAddr);
        }

        /* 分配录像码流的空间 */
        pstRecPool = &pstInit->RecPoolMain[i];
        if (NULL == pstRecPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstRecPool->totalLen = (pstRecPool->totalLen > MIN_POOL_SIZE) ? pstRecPool->totalLen : REC_POOL_SIZE;

            pstRecPool->vAddr = (PUINT8) SAL_memZalloc(pstRecPool->totalLen, SYS_HOSTCMD_PROC_NAME, "rec_main");
            if (NULL == pstRecPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: RecPoolMain[%d].totalLen = %d vAddr=%p \n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
        else
        {

            SYS_LOGW("APP malloc: RecPoolMain[%d].totalLen = %d vAddr=%p \n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
        
        pstRecPool = &pstInit->RecPoolSub[i];
        if (NULL == pstRecPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstRecPool->totalLen = (pstRecPool->totalLen > MIN_POOL_SIZE) ? pstRecPool->totalLen : REC_POOL_SIZE;

            pstRecPool->vAddr = (PUINT8) SAL_memZalloc(pstRecPool->totalLen, SYS_HOSTCMD_PROC_NAME, "rec_sub");
            if (NULL == pstRecPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: RecPoolSub[%d].totalLen  = %d vAddr=%p \n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: RecPoolSub[%d].totalLen  = %d vAddr=%p \n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }

        pstRecPool = &pstInit->RecPoolThird[i];

        if (NULL == pstRecPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstRecPool->totalLen = (pstRecPool->totalLen > MIN_POOL_SIZE) ? pstRecPool->totalLen : REC_POOL_SIZE;

            pstRecPool->vAddr = (PUINT8) SAL_memZalloc(pstRecPool->totalLen, SYS_HOSTCMD_PROC_NAME, "rec_third");
            if (NULL == pstRecPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: RecPoolThird[%d].totalLen= %d vAddr=%p\n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: RecPoolThird[%d].totalLen= %d vAddr=%p\n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
    }

    #endif
     
    /* 解码码流 */
    SYS_LOGW("pstInit->decChanCnt: %u **********\n", pstInit->decChanCnt); 
    for (i = 0; i < pstInit->decChanCnt; i++)
    {
        pstDecShare = &pstInit->decShareBuf[i];

        if (NULL == pstDecShare->pVirtAddr)
        {
            /* 管理解码复合流冲区的长度 */
            pstDecShare->bufLen = (pstDecShare->bufLen > MIN_POOL_SIZE) ? pstDecShare->bufLen : DEC_POOL_SIZE;

            pstDecShare->pVirtAddr = (PUINT8) SAL_memZalloc(pstDecShare->bufLen + 256 * 2 * 1024, SYS_HOSTCMD_PROC_NAME, "dec");
            if (NULL == pstDecShare->pVirtAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: decShareBuf[%d].bufLen   = %d vAddr=%p \n", i, pstDecShare->bufLen, pstDecShare->pVirtAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: decShareBuf[%d].bufLen   = %d vAddr=%p \n", i, pstDecShare->bufLen, pstDecShare->pVirtAddr);
        }
    }

    /* 转码码流 */
    SYS_LOGW("pstInit->ipcChanCnt: %u **********\n", pstInit->ipcChanCnt);   
    for (i = 0; i < pstInit->ipcChanCnt; i++)
    {
        pstDecShare = &pstInit->ipcShareBuf[i];
        if (NULL == pstDecShare->pVirtAddr)
        {
            /* 管理解码复合流冲区的长度 */
            pstDecShare->bufLen = (pstDecShare->bufLen > MIN_POOL_SIZE) ? pstDecShare->bufLen : DEC_POOL_SIZE;

            pstDecShare->pVirtAddr = (PUINT8) SAL_memZalloc(pstDecShare->bufLen + 256 * 2 * 1024, SYS_HOSTCMD_PROC_NAME, "ipc");
            if (NULL == pstDecShare->pVirtAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: ipcShareBuf[%d].bufLen   = %d vAddr=%p \n", i, pstDecShare->bufLen, pstDecShare->pVirtAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: ipcShareBuf[%d].bufLen   = %d vAddr=%p \n", i, pstDecShare->bufLen, pstDecShare->pVirtAddr);
        }

        pstNetPool = &pstInit->NetPoolRecode[i];
        if (NULL == pstNetPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstNetPool->totalLen = (pstNetPool->totalLen > MIN_POOL_SIZE) ? pstNetPool->totalLen : NET_POOL_SIZE;

            pstNetPool->vAddr = (PUINT8) SAL_memZalloc(pstNetPool->totalLen, SYS_HOSTCMD_PROC_NAME, "ipc_rec");
            if (NULL == pstNetPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: NetPoolRecode[%d].totalLen = %d vAddr=%p\n", i, pstNetPool->totalLen, pstNetPool->vAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: NetPoolRecode[%d].totalLen = %d vAddr=%p\n", i, pstNetPool->totalLen, pstNetPool->vAddr);
        }

        pstRecPool = &pstInit->RecPoolRecode[i];
        if (NULL == pstRecPool->vAddr)
        {
            /* 管理实时流缓冲区的长度 */
            pstRecPool->totalLen = (pstRecPool->totalLen > MIN_POOL_SIZE) ? pstRecPool->totalLen : REC_POOL_SIZE;

            pstRecPool->vAddr = (PUINT8) SAL_memZalloc(pstRecPool->totalLen, SYS_HOSTCMD_PROC_NAME, "ipc_rec");
            if (NULL == pstRecPool->vAddr)
            {
                SYS_LOGE("No Enough Mem For Rec Pool %d !!!\n", i);
                return SAL_FAIL;
            }
            SYS_LOGW("DSP malloc: RecPoolRecode[%d].totalLen  = %d vAddr=%p \n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: RecPoolRecode[%d].totalLen  = %d vAddr=%p \n", i, pstRecPool->totalLen, pstRecPool->vAddr);
        }
    }

    #ifndef SLAVE_CHIP

    /* 实时预览/回拉缓冲 */
    SYS_LOGW("pstInit->xrayChanCnt: %u **********\n",        pstInit->xrayChanCnt); 
    for (i = 0; i < pstInit->xrayChanCnt; i++)
    {
        pstXrayShare = &pstInit->xrayShareBuf[i]; /* 实时预览的Share Buffer */
        if (NULL == pstXrayShare->pVirtAddr) /* 若应用未申请，则DSP申请内存给应用返回 */
        {
            pstXrayShare->bufLen = (pstXrayShare->bufLen > 0) ? pstXrayShare->bufLen : XRAY_RT_SHARE_BUF_SIZE * 2;
            pstXrayShare->pVirtAddr = (PUINT8) SAL_memZalloc(pstXrayShare->bufLen, SYS_HOSTCMD_PROC_NAME, "xray");
            if (NULL == pstXrayShare->pVirtAddr)
            {
                SYS_LOGE("SAL_memAlloc for 'XRAY_SHARE_BUF_RT' failed, chan: %d, size: %u\n", i, pstXrayShare->bufLen);
                return SAL_FAIL;
            }

            SYS_LOGW("DSP malloc: xrayShareBuf[i], chan: %d, size: %u, vAddr: %p\n", \
                     i, pstXrayShare->bufLen, pstXrayShare->pVirtAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: xrayShareBuf[i], chan: %d, size: %u, vAddr: %p\n", \
                     i, pstXrayShare->bufLen, pstXrayShare->pVirtAddr);
        }

        pstXrayShare = &pstInit->xrayPbBuf[i]; /* 回拉的Share Buffer */
        if (NULL == pstXrayShare->pVirtAddr) /* 若应用未申请，则DSP申请内存给应用返回 */
        {
            pstXrayShare->bufLen = (pstXrayShare->bufLen > 0) ? pstXrayShare->bufLen : XRAY_PB_SHARE_BUF_SIZE * 2;
            pstXrayShare->pVirtAddr = (PUINT8) SAL_memZalloc(pstXrayShare->bufLen, SYS_HOSTCMD_PROC_NAME, "xray");
            if (NULL == pstXrayShare->pVirtAddr)
            {
                SYS_LOGE("SAL_memAlloc for 'XRAY_SHARE_BUF_PB' failed, chan: %d, size: %u\n", i, pstXrayShare->bufLen);
                return SAL_FAIL;
            }

            SYS_LOGW("DSP malloc: xrayPbBuf[i], chan: %d, size: %u, vAddr: %p\n", \
                     i, pstXrayShare->bufLen, pstXrayShare->pVirtAddr);
        }
        else
        {
            SYS_LOGW("APP malloc: xrayPbBuf[i], chan: %d, size: %u, vAddr: %p\n", \
                     i, pstXrayShare->bufLen, pstXrayShare->pVirtAddr);
        }
    }

    #endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : FUN_HOST_CMD_DSP_DEINIT
* 描  述  :
* 输  入  : - chan:
*         : - pBuf:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 FUN_HOST_CMD_DSP_DEINIT(UINT32 chan, VOID *pBuf)
{
    /* 释放分配的内存 */
    if (SAL_SOK != HostCmd_deinitDspPrm())
    {
        SYS_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != System_commonSysDeInit())
    {
        SYS_LOGE("!!!\n");
        return SAL_FAIL;
    }

    SYS_LOGD("DSP DeInit Done !!!\n");

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : FUN_HOST_CMD_DSP_GET_VERSION
* 描  述  :
* 输  入  : - chan:
*         : - pBuf:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 FUN_HOST_CMD_DSP_GET_VERSION(UINT32 chan, VOID *pBuf)
{
    DSP_VERSION_PARAM *pstDstVersionParam = NULL;

    if (pBuf == NULL)
    {
        SYS_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstDstVersionParam = (DSP_VERSION_PARAM *)pBuf;
    snprintf(pstDstVersionParam->szOrigin, sizeof(pstDstVersionParam->szOrigin),  
                "MODULE DSP : %d.%d.%d BUILD TIME : %s %s", 
                    DSP_COMMON_MAIN_VER, DSP_COMMON_SUB_VER, SVN_NUM, __DATE__, __TIME__);

    snprintf(pstDstVersionParam->szBuilder, sizeof(pstDstVersionParam->szBuilder), "%s", SAL_COMPILE_USER);
    snprintf(pstDstVersionParam->szBuildTime, sizeof(pstDstVersionParam->szBuildTime), 
                "%s %s", __DATE__, __TIME__);
    snprintf(pstDstVersionParam->szDSPVersion, sizeof(pstDstVersionParam->szDSPVersion), 
                "DSP %d.%d.%d", DSP_COMMON_MAIN_VERSION, DSP_COMMON_SUB_VERSION, SVN_NUM);

    return SAL_SOK;
}

/**
 * @function    DSP_SLAVE_INIT
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 DSP_SLAVE_INIT(VOID)
{
    /* link 模块初始化*/
    if (SAL_SOK != link_drvInit())
    {
        SAL_ERROR("link_drvInit err !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != System_commonSlaveVideoInit())
    {
        SYS_LOGE("%s failed! \n", __func__);
    }

    return SAL_SOK;
}

/**
 * @function    DSP_MASTER_INIT
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 DSP_MASTER_INIT(VOID)
{
    UINT32 uiCoreNum = 0;
    UINT32 uiCoreIdx = 0;
    UINT32 auiCoreId[SVP_DSP_CORE_NUM_MAX] = {0};
    DSPINITPARA *pstInit = NULL;

    pstInit = SystemPrm_getDspInitPara();
    SYS_LOGW("ViChn:%u, VoCnt:%u, xrayChanCnt:%u, xrayDispChanCnt:%u, encChanCnt:%u, decChanCnt:%u, ipcChanCnt:%u\n",
             pstInit->stViInitInfoSt.uiViChn, pstInit->stVoInitInfoSt.uiVoCnt, pstInit->xrayChanCnt, pstInit->xrayDispChanCnt,
             pstInit->encChanCnt, pstInit->decChanCnt, pstInit->ipcChanCnt);

    /* link 模块初始化*/
    if (SAL_SOK != link_drvInit())
    {
        SAL_ERROR("link_drvInit err !!!\n");
        return SAL_FAIL;
    }

    /* dsp 画osd初始化*/
    if ((DRAW_MOD_DSP == capb_get_line()->enDrawMod) || (DRAW_MOD_DSP == capb_get_osd()->enDrawMod))
    {
        auiCoreId[uiCoreIdx++] = 2;
        auiCoreId[uiCoreIdx++] = 3;
        uiCoreNum = uiCoreIdx;
        if (SAL_SOK != svpdsp_func_init(uiCoreNum, auiCoreId))
        {
            SYS_LOGE("Dsp_drvTaskInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /* osd 相关业务的初始化 */
    if (SAL_SOK != osd_tsk_init())
    {
        SYS_LOGE("osd_tsk_init err !!!\n");
        return SAL_FAIL;

    }

    /* system 初始化 */
    if (SAL_SOK != System_SysInit())
    {
        SYS_LOGE("System_SysInit err !!!\n");
        return SAL_FAIL;
    }

    /* system common 初始化 */
    if (SAL_SOK != System_commonInit())
    {
        SYS_LOGE("Sys_tskInit err !!!\n");
        return SAL_FAIL;
    }

    /*成像模块初始化*/
    if (pstInit->xrayChanCnt)
    {
        if (SAL_SOK != System_XrayInit())
        {
            SYS_LOGE("System_XrayInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /* 显示相关业务的初始化 */
    if (SAL_SOK != System_commonVideoInit())
    {
        SYS_LOGE("System_commonVideoInit err !!!\n");
        return SAL_FAIL;
    }

    /* 视频输入初始化,安检机单芯片+安检机双芯片主片才执行 */
    if ((pstInit->dspCapbPar.dev_tpye == PRODUCT_TYPE_ISM)
        && ((SINGLE_CHIP_TYPE == pstInit->deviceType) || (DOUBLE_CHIP_MASTER_TYPE == pstInit->deviceType)))
    {
        if (SAL_SOK != System_videoInputInit())
        {
            SYS_LOGE("System_videoInputInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /* 音频相关业务的初始化 */
    if (pstInit->dspCapbPar.audio_dev_cnt)
    {

        if (SAL_SOK != System_commonAudioInit())
        {
            SYS_LOGE("System_commonAudioInit err !!!\n");
            /* return SAL_FAIL; */
        }
    }

    /* 解码转包相关业务的初始化 */
    if (pstInit->decChanCnt)
    {
        if (SAL_SOK != System_commonVdecInit())
        {
            SYS_LOGE("System_commonVdecInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /* 分析仪，智能视频关联初始化 */
    if ((pstInit->boardType > SECURITY_ANALYSIS_START) && (pstInit->boardType < SECURITY_ANALYSIS_END))
    {
        if (SAL_SOK != System_commonIaVprocInit())
        {
            SYS_LOGE("System_commonIaVprocInit err !!!\n");
            return SAL_FAIL;
        }
    }

    /* link绑定参数*/
    link_taskBindMods();

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : FUN_HOST_CMD_DSP_INIT
   功能描述  : 处理主机命令 HOST_CMD_DSP_INIT
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
INT32 FUN_HOST_CMD_DSP_INIT(UINT32 chan, VOID *pBuf)
{
    UINT32 time1, time2;
    DSPINITPARA *pstInit = NULL;

    time1 = SAL_getCurMs();

    /* DISP_INIT_ATTR_ST stDispInitAttr; */
    pstInit = SystemPrm_getDspInitPara();

    if (SAL_SOK != capbility_init(pstInit))
    {
        SYS_LOGE("capbility_init err !!!\n");
        return SAL_FAIL;
    }


    if (SAL_SOK != plat_hal_init())
    {
        SYS_LOGE("plat_hal_init err !!!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == capb_get_product()->bXPackEnable)
    {
        /*定时器初始化*/
        SAL_TimerHpInit();

#ifdef HI3559A_SPC030
        /*VPSS 绑核操作绑2核上*/
        system("hik_echo 4 > /proc/irq/217/smp_affinity");
        system("hik_echo 4 > /proc/irq/218/smp_affinity");
#endif
    }

    /* 完成共享内存的分配 */
    if (SAL_SOK != HostCmd_initDspPrm())
    {
        SYS_LOGE("HostCmd_initDspPrm err !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != System_commonSysInit())
    {
        SYS_LOGE("System_commonSysInit err !!!\n");
        return SAL_FAIL;
    }

    SYS_LOGW("from app: boardType 0x%x, device type %d, model type %d, viChn %d \n",
             pstInit->boardType, pstInit->deviceType, pstInit->modelType, pstInit->stViInitInfoSt.uiViChn);

    #ifndef SVA_NO_USE
    /* 打印智能模块初始化信息 */
    UINT32 i = 0;
    for(i = 0; i < IA_MOD_MAX_NUM; i++)
    {
        IA_PrIaInitMapInfo(i);
    }
    #endif

    if (SINGLE_CHIP_TYPE == pstInit->deviceType
        || DOUBLE_CHIP_MASTER_TYPE == pstInit->deviceType)
    {
        SYS_LOGW("Master Init Enter! \n");

        if (SAL_SOK != DSP_MASTER_INIT())
        {
            SYS_LOGE("DSP Master Init Failed! \n");
            return SAL_FAIL;
        }
    }
    else if (DOUBLE_CHIP_SLAVE_TYPE == pstInit->deviceType)
    {
        SYS_LOGW("Slave Init Enter! \n");
        if (SAL_SOK != DSP_SLAVE_INIT())
        {
            SYS_LOGE("DSP Slave Init Failed! \n");
            return SAL_FAIL;
        }
    }
    else
    {
        SYS_LOGE("Invalid dev type %d \n", pstInit->deviceType);
        return SAL_FAIL;
    }

    /* 系统调试任务初始化 */
    dspdebug_init();

    time2 = SAL_getCurMs();

    SAL_INFO(NONE"\n");
    SAL_INFO(PURPLE "*****************************************************\n");
    SAL_INFO(PURPLE "[%u]DSP Init Done, time: %u ms!!!\n", time2, time2 - time1);
    SAL_INFO(PURPLE "*****************************************************\n");
    SAL_INFO(NONE"\n");

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : CmdProc_sysCmdProc
* 描  述  :
* 输  入  : - cmd :
*         : - chan:
*         : - pBuf:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 CmdProc_sysCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet = SAL_FAIL;
    static char dsp_init_flag = SAL_FALSE;

    DSPINITPARA *pDspInitParm = SystemPrm_getDspInitPara();

    switch (cmd)
    {
        case HOST_CMD_DSP_INIT:
        {
            if (dsp_init_flag == SAL_FALSE)
            {
                if (SAL_FAIL != FUN_HOST_CMD_DSP_INIT(chan, pBuf))
                {

                    SYS_LOGD("capt 1 type is %d(0: PAL, 1: NTSC)\n", pDspInitParm->stViInitInfoSt.stViInitPrm[0].eViStandard);
                    SYS_LOGD("capt 2 type is %d(0: PAL, 1: NTSC)\n", pDspInitParm->stViInitInfoSt.stViInitPrm[1].eViStandard);
                    dsp_init_flag = SAL_TRUE;
                    iRet = SAL_SOK;
                }
            }

            break;
        }
        case HOST_CMD_DSP_DEINIT:
        {
            if (dsp_init_flag == SAL_TRUE)
            {
                if (SAL_FAIL != FUN_HOST_CMD_DSP_DEINIT(chan, pBuf))
                {
                    iRet = SAL_SOK;
                    dsp_init_flag = SAL_FALSE;
                }
            }
            else
            {
                iRet = SAL_SOK;
            }

            break;
        }
        case HOST_CMD_DSP_GET_SYS_VERISION:
        {
            if (SAL_FAIL != FUN_HOST_CMD_DSP_GET_VERSION(chan, pBuf))
            {
                iRet = SAL_SOK;
            }

            break;
        }
        default:
        {
            SYS_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            iRet = SAL_FAIL;
            break;
        }
    }

    return iRet;
}

