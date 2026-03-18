
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
    return 0;
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
static INT32 svpdsp_hisiUnLoadCoreBinary(UINT32 dspCoreId)
{


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
static INT32 svpdsp_hisiLoadCoreBinary(UINT32 dspCoreId)
{

    
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



