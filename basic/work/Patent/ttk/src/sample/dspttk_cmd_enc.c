/**
 * @file    dspttk_cmd_enc.c
 * @brief
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */

#include <stdlib.h>
#include <string.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_devcfg.h"
#include "dspttk_init.h"
#include "dspttk_util.h"
#include "dspttk_cmd.h"
#include "dspttk_cmd_enc.h"
#include "dspcommon.h"
#include "dspttk_fop.h"
#include "sal_type.h"
#include "../../../src/base/media/resolution_appDsp.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define MAX_FILESIZE_FOR_STREAMSAVE (0x6000000) // 保存文件大小的阈值, 当文件超过该大小时, 新建文件
#define MAX_FILEBUF_SIZE (256)                  // 文件名字符串默认申请空间大小
/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
pthread_t startVencSaveThread[MAX_VENC_CHAN * DSPTTK_ENC_STREAMTYPE_NUM] = {0}; // 0 1 为主码流使用, 2 3为子码流使用

/**
 * @fn      dspttk_enc_check_filesize
 * @brief   检查文件大小并输出可用文件名
 *
 * @param   [IN] pFileName 输入待获取大小文件名
 * @param   [IN] streamSize 待输入码流大小
 * @param   [IN] u32Chan 通道号
 * @param   [IN] u32StreamType 码流类型(主码流或子码流)
 *
 * @param   [OUT] pFileName 输出文件名
 *
 * @return  32位表示错误号 1表示生成了新文件名 0表示没有生成新文件 -1表示错误
 */
SAL_STATUS dspttk_enc_check_filesize(CHAR *pFileName, INT64L streamSize, UINT32 u32Chan, UINT32 u32StreamType)
{
    long fileSize = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV_OUTPUT *pstEnvOutCfg = &pstDevCfg->stSettingParam.stEnv.stOutputDir;
    DSPTTK_SETTING_ENC *pstEncCfg = &pstDevCfg->stSettingParam.stEnc;
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;
    CHAR szFileName[MAX_FILEBUF_SIZE] = {0};
    SAL_STATUS sRet = SAL_SOK;

    if (!pFileName || streamSize < 0 || u32Chan > pstInitCfg->stEncDec.encChanCnt || u32StreamType > (DSPTTK_ENC_STREAMTYPE_NUM - 1))
    {
        PRT_INFO("Param In Error. pFileName is %s, streamSize is %lld, u32Chan is %d, u32StreamType is %d\n", pFileName, streamSize, u32Chan, u32StreamType);
        return SAL_FAIL;
    }

    fileSize = dspttk_get_file_size(pFileName);
    if (fileSize < 0)
    {
        PRT_INFO("get file size error. filesize is %ld, filename is %s. %s ,%d\n", fileSize, pFileName, __func__, __LINE__);
        return SAL_FAIL;
    }

    if (fileSize + streamSize > MAX_FILESIZE_FOR_STREAMSAVE)
    {
        snprintf(szFileName, sizeof(szFileName), "%s/%d/venc_%s_%s_%s_%dkbps_%dfps_%lldms.%s",
                 pstEnvOutCfg->encStream, u32Chan, (u32StreamType == DSPTTK_ENC_MAIN_STREAM) ? "main" : "sub",
                 (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.videoType == H264) ? "h264" : "h265",
                 (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.resolution == HD720p_FORMAT) ? "720p" : "1080p",
                 pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.bps, pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.fps,
                 dspttk_get_clock_ms(), (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType == RTP_STREAM) ? "rtp" : "ps");

        memcpy(pFileName, szFileName, sizeof(szFileName));
        sRet = 1;
    }

    return sRet;
}

/**
 * @fn      dspttk_enc_save_rtp_stream
 * @brief   码流保存回调函数,封装格式rtp
 *
 * @param   [IN] u32StreamType 码流类型 0为主码流 1为子码流
 * @param   [IN] u32Chan 通道号
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_save_rtp_stream(UINT32 u32StreamType, UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    UINT64 u64wIdx = 0, u64rIdx = 0, u64Len1 = 0, u64Len2 = 0, u64LenTotal = 0;
    UINT8 *pData = NULL;
    NET_POOL_INFO *pNetPool = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;

    if ((u32StreamType > (DSPTTK_ENC_STREAMTYPE_NUM - 1)) || (u32Chan > (pstInitCfg->stEncDec.encChanCnt - 1)))
    {
        PRT_INFO("Param In Error. StreamType is %d, Channel Num is %d.\n", u32StreamType, u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    DSPINITPARA *pDspInitPara = dspttk_get_share_param();
    if (pDspInitPara == NULL)
    {
        PRT_INFO("dspttk_get_share_param error. pDspInitPara is NULL.\n");
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    DSPTTK_SETTING_ENC *pstEncCfg = &pstDevCfg->stSettingParam.stEnc;
    DSPTTK_SETTING_ENV_OUTPUT *pstEnvOutCfg = &pstDevCfg->stSettingParam.stEnv.stOutputDir;
    CHAR szFileName[MAX_FILEBUF_SIZE] = {0};
    CHAR szNull[16] = {0}; // 新建文件使用
    CHAR szMuxType[MAX_FILEBUF_SIZE] = {0};

    snprintf(szMuxType, sizeof(szMuxType), "rtp");

    pNetPool = (u32StreamType == DSPTTK_ENC_MAIN_STREAM) ? &pDspInitPara->NetPoolMain[u32Chan] : &pDspInitPara->NetPoolSub[u32Chan];

    if (pNetPool == NULL)
    {
        PRT_INFO("pNetPool is NULL\n");
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    u64wIdx = pNetPool->wIdx;
    u64rIdx = u64wIdx;

    snprintf(szFileName, sizeof(szFileName), "%s/%d/venc_%s_%s_%s_%dkbps_%dfps_%lldms.%s",
             pstEnvOutCfg->encStream, u32Chan, (u32StreamType == DSPTTK_ENC_MAIN_STREAM) ? "main" : "sub",
             (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.videoType == H264) ? "h264" : "h265",
             (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.resolution == HD720p_FORMAT) ? "720p" : "1080p",
             pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.bps, pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.fps,
             dspttk_get_clock_ms(), (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType == RTP_STREAM) ? "rtp" : "ps");

    sRet = dspttk_write_file(szFileName, 0, SEEK_SET, szNull, 0);

    while (pstEncCfg->stEncMultiParam[u32StreamType].bSaveEn)
    {
        u64wIdx = pNetPool->wIdx;

        if (u64wIdx == u64rIdx)
        {
            dspttk_msleep(5);
            continue;
        }

        if (u64wIdx >= u64rIdx)
        {
            u64Len1 = u64wIdx - u64rIdx;
            u64Len2 = 0;
        }
        else
        {
            u64Len1 = pNetPool->totalLen - u64rIdx;
            u64Len2 = u64wIdx;
        }

        u64LenTotal = u64Len1 + u64Len2;

        pData = pNetPool->vAddr + u64rIdx;

        if (u64Len1)
        {
            sRet = dspttk_enc_check_filesize(szFileName, u64Len1, u32Chan, u32StreamType);
            if (sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_check_filesize error, sRet is %d.filename is %s\n", sRet, szFileName);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }

            sRet = dspttk_write_file(szFileName, 0, SEEK_END, pData, u64Len1);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("dspttk_write_file error, sRet is %d. filename is %s\n", sRet, szFileName);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }
        }

        if (u64Len2)
        {
            sRet = dspttk_enc_check_filesize(szFileName, u64Len2, u32Chan, u32StreamType);
            if (sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_check_filesize error, sRet is %d.filename is %s\n", sRet, szFileName);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }

            sRet = dspttk_write_file(szFileName, 0, SEEK_END, pNetPool->vAddr, u64Len2);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("dspttk_write_file error, sRet is %d. filename is %s\n", sRet, szFileName);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }
        }

        u64rIdx = (u64rIdx + u64LenTotal) % pNetPool->totalLen;

        dspttk_msleep(5);
    }

    return CMD_RET_MIX(HOST_CMD_MODULE_VENC, sRet);
}

/**
 * @fn      dspttk_enc_save_ps_stream
 * @brief   码流保存回调函数,封装格式ps
 *
 * @param   [IN] u32StreamType 码流类型 0为主码流 1为子码流
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_save_ps_stream(UINT32 u32StreamType, UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    UINT64 u64wIdx = 0, u64rIdx = 0, u64Len1 = 0, u64Len2 = 0, u64LenTotal = 0;
    UINT8 *pData = NULL;
    REC_POOL_INFO *pRecPool = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;

    if ((u32StreamType > (DSPTTK_ENC_STREAMTYPE_NUM - 1)) || (u32Chan > (pstInitCfg->stEncDec.encChanCnt - 1)))
    {
        PRT_INFO("Param In Error. StreamType is %d, Channel Num is %d\n", u32StreamType, u32Chan);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    DSPINITPARA *pDspInitPara = dspttk_get_share_param();
    if (pDspInitPara == NULL)
    {
        PRT_INFO("dspttk_get_share_param error. pDspInitPara is NULL. \n");
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    DSPTTK_SETTING_ENC *pstEncCfg = &pstDevCfg->stSettingParam.stEnc;
    DSPTTK_SETTING_ENV_OUTPUT *pstEnvOutCfg = &pstDevCfg->stSettingParam.stEnv.stOutputDir;
    CHAR szFileName[MAX_FILEBUF_SIZE] = {0};
    CHAR szNull[16] = {0}; // 新建文件使用
    CHAR szMuxType[MAX_FILEBUF_SIZE] = {0};

    snprintf(szMuxType, sizeof(szMuxType), "ps");

    pRecPool = (u32StreamType == DSPTTK_ENC_MAIN_STREAM) ? &pDspInitPara->RecPoolMain[u32Chan] : &pDspInitPara->RecPoolSub[u32Chan];
    if (pRecPool == NULL)
    {
        PRT_INFO("pRecPool is NULL.\n");
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    u64wIdx = pRecPool->wOffset;
    u64rIdx = u64wIdx;

    snprintf(szFileName, sizeof(szFileName), "%s/%d/venc_%s_%s_%s_%dkbps_%dfps_%lldms.%s",
             pstEnvOutCfg->encStream, u32Chan, (u32StreamType == DSPTTK_ENC_MAIN_STREAM) ? "main" : "sub",
             (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.videoType == H264) ? "h264" : "h265",
             (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.resolution == HD720p_FORMAT) ? "720p" : "1080p",
             pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.bps, pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.fps,
             dspttk_get_clock_ms(), (pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType == RTP_STREAM) ? "rtp" : "ps");

    sRet = dspttk_write_file(szFileName, 0, SEEK_SET, szNull, 0);

    if (sRet < 0)
    {
        PRT_INFO("dspttk_write_file error. szFileName is %s\n", szFileName);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    while (pstEncCfg->stEncMultiParam[u32StreamType].bSaveEn)
    {
        u64wIdx = pRecPool->wOffset;
        u64rIdx = pRecPool->rOffset;

        if (u64wIdx == u64rIdx)
        {
            dspttk_msleep(5);
            continue;
        }

        if (u64wIdx >= u64rIdx)
        {
            u64Len1 = u64wIdx - u64rIdx;
            u64Len2 = 0;
        }
        else
        {
            u64Len1 = pRecPool->totalLen - u64rIdx;
            u64Len2 = u64wIdx;
        }

        u64LenTotal = u64Len1 + u64Len2;

        pData = pRecPool->vAddr + u64rIdx;
        if (u64Len1)
        {
            sRet = dspttk_enc_check_filesize(szFileName, u64Len1, u32Chan, u32StreamType);
            if (sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_check_filesize error, sRet is %d. filename is %s\n", sRet, szFileName);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }

            sRet = dspttk_write_file(szFileName, 0, SEEK_END, pData, u64Len1);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("dspttk_write_file error, sRet is %d. filename is %s\n", sRet, szFileName);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }
        }

        if (u64Len2)
        {
            sRet = dspttk_enc_check_filesize(szFileName, u64Len2, u32Chan, u32StreamType);
            if (sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_check_filesize error, sRet is %d.\n", sRet);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }

            sRet = dspttk_write_file(szFileName, 0, SEEK_END, pRecPool->vAddr, u64Len2);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("dspttk_write_file error, sRet is %d.\n", sRet);
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }
        }

        u64rIdx = (u64rIdx + u64LenTotal) % pRecPool->totalLen;
        pRecPool->rOffset = u64rIdx;

        dspttk_msleep(5);
    }
    return CMD_RET_MIX(HOST_CMD_MODULE_VENC, sRet);
}

/**
 * @fn      dspttk_enc_start_save
 * @brief   开启编码保存
 *
 * @param   [IN] u32StreamType 码流编号 (主码流/子码流)
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流状态
 * @param   [IN] DSPTTK_INIT_PARAM 隐含入参，获取初始化设置编码通道数
 * @return  SAL_STATUS 32位表示错误号
 */
SAL_STATUS dspttk_enc_start_save(UINT32 u32StreamType)
{
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENC *pstEncCfg = &pstDevCfg->stSettingParam.stEnc;
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;
    DSPTTK_INIT_ENCDEC *pstEncInitCfg = &pstInitCfg->stEncDec;
    UINT32 u32ThreadOffset = 0, i = 0;

    if (u32StreamType > DSPTTK_ENC_STREAMTYPE_NUM - 1)
    {
        PRT_INFO("StreamType is %d Error.\n", u32StreamType);
        return SAL_FAIL;
    }

    /* 线程数组有四个, 当码流为主码流时, 默认使用前两个, 当码流为子码流时, 默认使用后两个 */
    if (DSPTTK_ENC_MAIN_STREAM == u32StreamType)
    {
        u32ThreadOffset = 0;
    }
    else if (DSPTTK_ENC_SUB_STREAM == u32StreamType)
    {
        u32ThreadOffset = 2;
    }
    else
    {
        PRT_INFO("stream type error, stream type is %d.\n", u32StreamType);
        return SAL_FAIL;
    }

    if (RTP_STREAM == pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType)
    {
        for (i = 0; i < pstEncInitCfg->encChanCnt; i++)
        {
            sRet = dspttk_pthread_spawn(&startVencSaveThread[i + u32ThreadOffset], DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_enc_save_rtp_stream, 2, u32StreamType, i);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("dspttk_pthread_spawn error! return value is %d. stream type is %s, packet type is rtp.\n",
                         sRet, (u32StreamType == DSPTTK_ENC_MAIN_STREAM) ? "main" : "sub");
            }
        }
    }
    else if (PS_STREAM == pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType)
    {
        for (i = 0; i < pstEncInitCfg->encChanCnt; i++)
        {

            if (pstEncCfg->stEncMultiParam[u32StreamType].bSaveEn)
            {
                sRet = dspttk_pthread_spawn(&startVencSaveThread[i + u32ThreadOffset], DSPTTK_PTHREAD_SCHED_OTHER, 50, 0x100000, (void *)dspttk_enc_save_ps_stream, 2, u32StreamType, i);
                if (sRet != SAL_SOK)
                {
                    PRT_INFO("dspttk_pthread_spawn error! return value is %d. stream type is %s, packet type is ps.\n",
                             sRet, (u32StreamType == DSPTTK_ENC_MAIN_STREAM ? "main" : "sub"));
                }
            }
        }
    }
    else
    {
        PRT_INFO("Main Stream Packet Type Error. Packet Type is %d.\n", pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType);
        return SAL_FAIL;
    }
    return sRet;
}

/**
 * @fn      dspttk_enc_stop_save
 * @brief   关闭编码保存
 *
 * @param   [IN] u32StreamType 码流编号 (主码流/子码流)
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流状态
 *
 * @return  SAL_STATUS dspttk_enc_start_save32位表示错误号
 */
SAL_STATUS dspttk_enc_stop_save(UINT32 u32StreamType)
{
    SAL_STATUS sRet = SAL_FAIL;
    UINT32 u32ThreadOffset = 0, i = 0;
    DSPINITPARA *pDspInitPara = dspttk_get_share_param();

    if (u32StreamType > (DSPTTK_ENC_STREAMTYPE_NUM - 1))
    {
        PRT_INFO("Param In Error! u32StreamType is %d.\n", u32StreamType);
        return SAL_FAIL;
    }

    if (!pDspInitPara)
    {
        PRT_INFO("pDspInitPara is NULL.\n");
        return SAL_FAIL;
    }
    /* 线程数组有四个, 当码流为主码流时, 默认使用前两个, 当码流为子码流时, 默认使用后两个 */
    if (DSPTTK_ENC_MAIN_STREAM == u32StreamType)
    {
        u32ThreadOffset = 0;
    }
    else if (DSPTTK_ENC_SUB_STREAM == u32StreamType)
    {
        u32ThreadOffset = 2;
    }
    else
    {
        PRT_INFO("stream type error, stream type is %d.\n", u32StreamType);
        return SAL_FAIL;
    }

    for (i = 0; i < pDspInitPara->encChanCnt; i++)
    {
        sRet = dspttk_pthread_is_alive(startVencSaveThread[u32ThreadOffset + i]);
        if (!sRet)
        {
            PRT_INFO("dspttk_pthread_is_alive error. u32ThreadOffset is %d, i is %d, sRet is %d.\n", u32ThreadOffset, i, sRet);
            return SAL_FAIL;
        }
    }

    return sRet;
}

/**
 * @fn      dspttk_enc_save_set
 * @brief   编码数据保存设置
 *
 * @param   [IN] u32StreamType 码流编号 (主码流/子码流)
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流开启关闭状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_save_set(UINT32 u32StreamType)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENC *pstCfgEnc = &pstDevCfg->stSettingParam.stEnc;
    SAL_STATUS sRet = SAL_FAIL;

    if (u32StreamType > DSPTTK_ENC_STREAMTYPE_NUM - 1)
    {
        PRT_INFO("StreamType is %d Error! \n", u32StreamType);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    if (pstCfgEnc->stEncMultiParam[u32StreamType].bStartEn)
    {
        if (pstCfgEnc->stEncMultiParam[u32StreamType].bSaveEn)
        {
            sRet = dspttk_enc_start_save(u32StreamType);
            if (sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_start_save failed.\n");
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }
        }
        else
        {
            sRet = dspttk_enc_stop_save(u32StreamType);
            if (sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_stop_save failed.\n");
                return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
            }
        }
    }
    return CMD_RET_MIX(HOST_CMD_MODULE_VENC, sRet);
}

/**
 * @fn      dspttk_enc_start
 * @brief   开启编码
 *
 * @param   [IN] u32TypeMixChan 码流编号(主码流/子码流) + 通道号[0,1], 高16位为码流编号，低16位为通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_start(UINT32 u32TypeMixChan)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32StreamType = U32_FIRST_OF(u32TypeMixChan);
    UINT32 u32ChanNum = U32_LAST_OF(u32TypeMixChan);
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;

    if ((u32StreamType > DSPTTK_ENC_STREAMTYPE_NUM - 1) || (u32ChanNum > (pstInitCfg->stEncDec.encChanCnt - 1)))
    {
        PRT_INFO("Param In Error. StreamType is %d, Channel Num is %d \n", u32StreamType, u32ChanNum);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    switch (u32StreamType)
    {
    case DSPTTK_ENC_MAIN_STREAM:
    {
        sRet = SendCmdToDsp(HOST_CMD_START_ENCODE, u32ChanNum, NULL, NULL);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("SendCmdToDsp HOST_CMD_START_ENCODE Error.\n");
            return CMD_RET_MIX(HOST_CMD_START_ENCODE, sRet);
        }
    }
    break;

    case DSPTTK_ENC_SUB_STREAM:
    {
        sRet = SendCmdToDsp(HOST_CMD_START_SUB_ENCODE, u32ChanNum, NULL, NULL);

        if (sRet != SAL_SOK)
        {
            PRT_INFO("SendCmdToDsp HOST_CMD_START_ENCODE Error.\n");
            return CMD_RET_MIX(HOST_CMD_START_ENCODE, sRet);
        }
    }
    break;

    default:
    {
        return CMD_RET_MIX(HOST_CMD_START_ENCODE, SAL_FAIL);
    }
    break;
    }

    return CMD_RET_MIX(HOST_CMD_START_ENCODE, sRet);
}

/**
 * @fn      dspttk_enc_stop
 * @brief   关闭编码
 *
 * @param   [IN] u32TypeMixChan 码流编号(主码流/子码流) + 通道号[0,1], 高16位为码流编号，低16位为通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_stop(UINT32 u32TypeMixChan)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32StreamType = U32_FIRST_OF(u32TypeMixChan);
    UINT32 u32ChanNum = U32_LAST_OF(u32TypeMixChan);
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;

    if ((u32StreamType > DSPTTK_ENC_STREAMTYPE_NUM - 1) || (u32ChanNum > (pstInitCfg->stEncDec.encChanCnt - 1)))
    {
        PRT_INFO("Param In Error. StreamType is %d, Channel Num is %d \n", u32StreamType, u32ChanNum);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    switch (u32StreamType)
    {
    case DSPTTK_ENC_MAIN_STREAM:
    {
        sRet = SendCmdToDsp(HOST_CMD_STOP_ENCODE, u32ChanNum, NULL, NULL);
        return CMD_RET_MIX(HOST_CMD_STOP_ENCODE, sRet);
    }
    break;

    case DSPTTK_ENC_SUB_STREAM:
    {
        sRet = SendCmdToDsp(HOST_CMD_STOP_SUB_ENCODE, u32ChanNum, NULL, NULL);
        return CMD_RET_MIX(HOST_CMD_STOP_SUB_ENCODE, sRet);
    }
    break;

    default:
    {
        return CMD_RET_MIX(HOST_CMD_STOP_ENCODE, SAL_FAIL);
    }
    break;
    }

    return CMD_RET_MIX(HOST_CMD_STOP_ENCODE, SAL_FAIL);
}

/**
 * @fn      dspttk_enc_enable_set
 * @brief   开启编码
 *
 * @param   [IN] u32StreamType 码流类型
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流开启关闭状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_enable_set(UINT32 u32StreamType)
{

    UINT64 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENC *pstEncCfg = &pstDevCfg->stSettingParam.stEnc;
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;
    UINT32 i = 0, u32TypeMixChan = 0;

    if (u32StreamType > DSPTTK_ENC_STREAMTYPE_NUM - 1)
    {
        PRT_INFO("StreamType is %d Error! \n", u32StreamType);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    if (pstEncCfg->stEncMultiParam[u32StreamType].bStartEn)
    {
        for (i = 0; i < pstInitCfg->stEncDec.encChanCnt; i++)
        {
            U32_COMBINE(u32StreamType, i, u32TypeMixChan);
            sRet |= dspttk_enc_start(u32TypeMixChan);
        }
    }
    else if (0 == pstEncCfg->stEncMultiParam[u32StreamType].bStartEn)
    {
        for (i = 0; i < pstInitCfg->stEncDec.encChanCnt; i++)
        {
            U32_COMBINE(u32StreamType, i, u32TypeMixChan);
            sRet |= dspttk_enc_stop(u32TypeMixChan);
        }
    }
    else
    {
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }

    return sRet;
}

/**
 * @fn      dspttk_enc_param_set
 * @brief   设置编码参数
 *
 * @param   [IN] u32StreamType 码流编号(主码流/子码流)
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流开启关闭状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_param_set(UINT32 u32StreamType)
{
    SAL_STATUS sRet = SAL_SOK;
    ENCODER_PARAM stEncParam = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_INIT_PARAM *pstInitCfg = &pstDevCfg->stInitParam;
    UINT32 i = 0;

    if (u32StreamType > DSPTTK_ENC_STREAMTYPE_NUM - 1)
    {
        PRT_INFO("StreamType is %d Error! \n", u32StreamType);
        return CMD_RET_MIX(HOST_CMD_MODULE_VENC, SAL_FAIL);
    }
    DSPTTK_SETTING_ENC *pstEncCfg = &pstDevCfg->stSettingParam.stEnc;

    stEncParam.streamType = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.streamType;
    stEncParam.videoType = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.videoType;
    stEncParam.audioType = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.audioType;
    stEncParam.resolution = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.resolution;
    stEncParam.bpsType = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.bpsType;
    stEncParam.bps = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.bps;
    stEncParam.quant = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.quant;
    stEncParam.fps = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.fps;
    stEncParam.IFrameInterval = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.IFrameInterval;
    stEncParam.muxType = pstEncCfg->stEncMultiParam[u32StreamType].stEncParam.muxType;

    switch (u32StreamType)
    {
    case DSPTTK_ENC_MAIN_STREAM:
    {
        for (i = 0; i < pstInitCfg->stEncDec.encChanCnt; i++)
        {
            sRet |= SendCmdToDsp(HOST_CMD_SET_ENCODER_PARAM, i, NULL, &stEncParam);
            if (sRet != SAL_SOK)
            {
                return CMD_RET_MIX(HOST_CMD_SET_ENCODER_PARAM, sRet);
            }
        }
        return CMD_RET_MIX(HOST_CMD_SET_ENCODER_PARAM, sRet);
    }
    break;

    case DSPTTK_ENC_SUB_STREAM:
    {
        for (i = 0; i < pstInitCfg->stEncDec.encChanCnt; i++)
        {
            sRet |= SendCmdToDsp(HOST_CMD_SET_SUB_ENCODER_PARAM, i, NULL, &stEncParam);
        }
        return CMD_RET_MIX(HOST_CMD_SET_ENCODER_PARAM, sRet);
    }
    break;

    default:
        return CMD_RET_MIX(HOST_CMD_SET_ENCODER_PARAM, SAL_FAIL);
        break;
    }

    return CMD_RET_MIX(HOST_CMD_SET_ENCODER_PARAM, SAL_FAIL);
}

/**
 * @fn      dspttk_enc_set_init
 * @brief   对Encode Setting作初始化
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Encode默认配置
 *
 * @return  UINT32 32位表示错误号
 */
SAL_STATUS dspttk_enc_set_init(DSPTTK_DEVCFG *pstDevCfg)
{
    UINT64 sRet = SAL_SOK;
    UINT32 i = 0;

    if (pstDevCfg == NULL)
    {
        PRT_INFO("Param Error. pstDevCfg is %p.", pstDevCfg);
        return SAL_FAIL;
    }

    for (i = 0; i < DSPTTK_ENC_STREAMTYPE_NUM; i++)
    {
        sRet = dspttk_enc_param_set(i);
        if (CMD_RET_OF(sRet) != SAL_SOK)
        {
            PRT_INFO("dspttk_enc_param_set error. streamLevel is %s\n", (i == DSPTTK_ENC_MAIN_STREAM) ? "main" : "sub");
            return CMD_RET_OF(sRet);
        }
    }

    return CMD_RET_OF(sRet);
}
