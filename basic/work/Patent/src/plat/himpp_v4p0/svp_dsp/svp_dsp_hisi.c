
/*******************************************************************************
 * svp_dsp_hal.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2020年05月11日 Create
 *
 * Description : 
 * Modification: 
 *******************************************************************************/

#include "sal.h"
#include "../include/platform_sdk.h"
#include "platform_hal.h"
#include "../../hal/hal_inc_inter/svp_dsp_hal_inter.h"


static UINT32 gu32UseCoreNum = 0;
static SVP_DSP_ID_E gaenDspUseId[SVP_DSP_CORE_NUM_MAX] = {0};
static SVP_DSP_HANDLE gaHandles[SVP_DSP_CORE_NUM_MAX];
static SVP_DSP_PLAT_OPS svpDspOps;

/*******************************************************************************
* 函数名  : svpdsp_hisiGetCoreNum
* 描  述  : 获取可使用核的数目
* 输  入  : 
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_hisiGetCoreNum(void)
{
    return gu32UseCoreNum;
}


/*******************************************************************************
* 函数名  : svpdsp_hisiProcessTsk
* 描  述  : 调用DSP核处理指定任务
* 输  入  : UINT32 u32CoreId
          SVP_DSP_CMD_E enCmd
          UINT64 u64PhyAddr
          UINT32 u32Len
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_hisiProcessTsk(UINT32 u32CoreId, UINT32 enCmd, UINT64 u64PhyAddr, UINT32 u32Len)
{
    HI_S32 s32Ret = HI_SUCCESS;
    SVP_DSP_ID_E enDspId    = gaenDspUseId[u32CoreId];
    SVP_DSP_HANDLE *pHandle = &gaHandles[u32CoreId];
    SVP_DSP_MESSAGE_S stMsg = {0};

    stMsg.u32CMD     = enCmd;
    stMsg.u64Body    = u64PhyAddr;
    stMsg.u32BodyLen = u32Len;
    stMsg.u32MsgId   = 0;

    s32Ret = HI_MPI_SVP_DSP_RPC(pHandle, &stMsg, enDspId, SVP_DSP_PRI_0);
    if (HI_SUCCESS != s32Ret)
    {
        SVP_LOGE("HI_MPI_SVP_DSP_RPC faid:0x%x, DspId:%d, CmdId:%d\n", s32Ret, enDspId, enCmd);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : svpdsp_hisiQueryStatus
* 描  述  : 查询对应核的任务是否完成
* 输  入  : UINT32 u32CoreId
* 输  出  : BOOL *pbFinish
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_hisiQueryStatus(UINT32 u32CoreId, BOOL *pbFinish)
{
    HI_S32 s32Ret   = HI_SUCCESS;
    HI_BOOL bBlock  = HI_FALSE;
    HI_BOOL bFinish = HI_FALSE;
    SVP_DSP_HANDLE hHandle = gaHandles[u32CoreId];
    SVP_DSP_ID_E enDspId   = gaenDspUseId[u32CoreId];

    s32Ret = HI_MPI_SVP_DSP_Query(enDspId, hHandle, &bFinish, bBlock);
    if (HI_SUCCESS != s32Ret)
    {
        SVP_LOGE("HI_MPI_SVP_DSP_Query fail:0x%x, DspId:%d, handle:%d\n", s32Ret, enDspId, hHandle);
        return SAL_FAIL;
    }

    *pbFinish = (bFinish == HI_TRUE) ? SAL_TRUE : SAL_FALSE;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : svpdsp_hisiUnLoadCoreBinary
* 描  述  : DSP卸载函数
* 输  入  : SVP_DSP_ID_E enCoreId
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 svpdsp_hisiUnLoadCoreBinary(SVP_DSP_ID_E enCoreId)
{
    HI_S32 s32Ret = -1;

    s32Ret = HI_MPI_SVP_DSP_DisableCore(enCoreId);
    if (HI_SUCCESS != s32Ret)
    {
        SVP_LOGE("disable core:%d fail\n", enCoreId);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_SVP_DSP_PowerOff(enCoreId);
    if (HI_SUCCESS != s32Ret)
    {
        SVP_LOGE("pownoff core:%d fail\n", enCoreId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : svpdsp_hisiLoadCoreBinary
* 描  述  : DSP加载函数
* 输  入  : SVP_DSP_ID_E enCoreId
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_hisiLoadCoreBinary(SVP_DSP_ID_E enCoreId)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UINT32 i = 0;
    static HI_CHAR * aszBin[4][4] = 
    {
        {"./dsp_bin/dsp0/hi_sram.bin","./dsp_bin/dsp0/hi_iram0.bin","./dsp_bin/dsp0/hi_dram0.bin","./dsp_bin/dsp0/hi_dram1.bin"},
        {"./dsp_bin/dsp1/hi_sram.bin","./dsp_bin/dsp1/hi_iram0.bin","./dsp_bin/dsp1/hi_dram0.bin","./dsp_bin/dsp1/hi_dram1.bin"},
        {"./dsp_bin/dsp2/hi_sram.bin","./dsp_bin/dsp2/hi_iram0.bin","./dsp_bin/dsp2/hi_dram0.bin","./dsp_bin/dsp2/hi_dram1.bin"},
        {"./dsp_bin/dsp3/hi_sram.bin","./dsp_bin/dsp3/hi_iram0.bin","./dsp_bin/dsp3/hi_dram0.bin","./dsp_bin/dsp3/hi_dram1.bin"},
    };

    s32Ret = HI_MPI_SVP_DSP_PowerOn(enCoreId);
    if (HI_SUCCESS != s32Ret)
    {
        SVP_LOGE("dsp power on failed:0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    for (i = 0; i < 4; i++)
    {
        s32Ret = HI_MPI_SVP_DSP_LoadBin(aszBin[enCoreId][i], enCoreId * 4 + i);
        if (HI_SUCCESS != s32Ret)
        {
            SVP_LOGE("load dsp bin:%s failed:0x%x\n", aszBin[enCoreId][i], s32Ret);
            return SAL_FAIL;
        }
    }

    s32Ret = HI_MPI_SVP_DSP_EnableCore(enCoreId);
    if (HI_SUCCESS != s32Ret)
    {
        SVP_LOGE("enable dsp core failed:0x%x\n", s32Ret);
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : svpdsp_hisiInit
* 描  述  : DSP核初始化
* 输  入  : 
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 svpdsp_hisiInit(UINT32 u32CoreNum, UINT32 *pu32Core)
{
    UINT32 i = 0;
    UINT32 u32LoadNum = 0;
    INT32 ret = 0;
    SVP_DSP_ID_E enDspId;
    BOOL bFinish = SAL_FALSE;

    gu32UseCoreNum = u32CoreNum;
    for (i = 0; i < u32CoreNum; i++)
    {
        gaenDspUseId[i] = pu32Core[i];
        enDspId = gaenDspUseId[i];
        ret = svpdsp_hisiUnLoadCoreBinary(enDspId);
        if (SAL_SOK != ret)
        {
            SVP_LOGE("svpdsp_hisiUnLoadCoreBinary fail, DspId:%d\n", enDspId);
        }

        ret = svpdsp_hisiLoadCoreBinary(enDspId);
        if (SAL_SOK != ret)
        {
            SVP_LOGE("svpdsp_hisiLoadCoreBinary fail, DspId:%d\n", enDspId);
            break;
        }
    }

    if (i < u32CoreNum)
    {
        u32LoadNum = i;
        for (i = 0; i < u32LoadNum; i++)
        {
            enDspId = gaenDspUseId[i];
            (VOID)svpdsp_hisiUnLoadCoreBinary(enDspId);
        }

        return SAL_FAIL;
    }

    /* 第一次运行DSP耗时较长，所以初始化时先执行一次 */
    for (i = 0; i < u32CoreNum; i++)
    {
        ret = svpdsp_hisiProcessTsk(i, SVP_DSP_CMD_IPCM_TEST, 0, 0);
        if (SAL_SOK != ret)
        {
            SVP_LOGE("svpdsp_hisiProcessTsk fail, chn:%d, Cmd:%d\n", i, SVP_DSP_CMD_IPCM_TEST);
            continue;
        }

        while (SAL_SOK == (ret = svpdsp_hisiQueryStatus(i, &bFinish)))
        {
            if (SAL_TRUE == bFinish)
            {
                break;
            }

            usleep(1000);
        }

        if (SAL_SOK != ret)
        {
            SVP_LOGE("svpdsp_hisiQueryStatus fail, chn:%d\n", i);
        }
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : svpdsp_halRegister
* 描  述  : 注册svp dsp函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
SVP_DSP_PLAT_OPS *svpdsp_halRegister(void)
{
    memset(&svpDspOps, 0, sizeof(SVP_DSP_PLAT_OPS));

    svpDspOps.init  =       svpdsp_hisiInit;
    svpDspOps.setTask      = svpdsp_hisiProcessTsk;
    svpDspOps.getStatus    = svpdsp_hisiQueryStatus;
    svpDspOps.getCoreNum =   svpdsp_hisiGetCoreNum;
	svpDspOps.unloadDspbin = svpdsp_hisiUnLoadCoreBinary;
	svpDspOps.loadDspbin   = svpdsp_hisiLoadCoreBinary;

    return &svpDspOps;
}



