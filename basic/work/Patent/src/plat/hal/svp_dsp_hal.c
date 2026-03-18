
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
#include "platform_hal.h"
#include "../../hal/hal_inc_inter/svp_dsp_hal_inter.h"

#line __LINE__ "svp_dsp_hal.c"


static SVP_DSP_PLAT_OPS *pSvpDspOps = NULL;

/*******************************************************************************
* 函数名  : svpdsp_hal_getCoreNum
* 描  述  : 获取可使用核的数目
* 输  入  : 
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
UINT32 svpdsp_hal_getCoreNum(VOID)
{

    if(NULL == pSvpDspOps || NULL == pSvpDspOps->getCoreNum)
    {
        return SAL_FAIL;
    }
    
    return pSvpDspOps->getCoreNum();
}


/*******************************************************************************
* 函数名  : svpdsp_hal_setTask
* 描  述  : 调用DSP核处理指定任务
* 输  入  : UINT32 u32CoreId
          enCmd 为hisisdk SVP_DSP_CMD_E 宏定义
          UINT64 u64PhyAddr
          UINT32 u32Len
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_hal_setTask(UINT32 u32CoreId, UINT32 enCmd, UINT64 u64PhyAddr, UINT32 u32Len)
{
    INT32 s32Ret = SAL_FAIL;

    if(NULL == pSvpDspOps || NULL == pSvpDspOps->getStatus)
    {
        return SAL_FAIL;
    }
    
    s32Ret = pSvpDspOps->setTask(u32CoreId, enCmd, u64PhyAddr, u32Len);
    if (SAL_SOK != s32Ret)
    {
        SVP_LOGE("svp dsp setTask fail:0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/*******************************************************************************
* 函数名  : svpdsp_hal_getStatus
* 描  述  : 查询对应核的任务是否完成
* 输  入  : UINT32 u32CoreId
* 输  出  : BOOL *pbFinish
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_hal_getStatus(UINT32 u32CoreId, BOOL *pbFinish)
{
    INT32 s32Ret = SAL_FAIL;

    if(NULL == pSvpDspOps || NULL == pSvpDspOps->getStatus)
    {
        return SAL_FAIL;
    }
    
    s32Ret = pSvpDspOps->getStatus(u32CoreId, pbFinish);
    if (SAL_SOK != s32Ret)
    {
        SVP_LOGE("svp dsp getStatus fail:0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/*******************************************************************************
* 函数名  : svpdsp_hal_loadDspBin
* 描  述  : DSP加载函数
* 输  入  : - u32DspCoreId:dspid
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 svpdsp_hal_loadDspBin(UINT32 u32DspCoreId)
{
    return pSvpDspOps->loadDspbin(u32DspCoreId);
}

/*******************************************************************************
* 函数名  : svpdsp_hal_unloadDspBin
* 描  述  : DSP卸载函数
* 输  入  : - u32DspCoreId:dspid
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 svpdsp_hal_unloadDspBin(UINT32 u32DspCoreId)
{
    return pSvpDspOps->unloadDspbin(u32DspCoreId);
}

/*******************************************************************************
* 函数名  : svpdsp_hal_initDspCore
* 描  述  : 初始化dsp核
* 输  入  : - u32CoreNum:核心数
*       : - pu32Core ：
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 svpdsp_hal_initDspCore(UINT32 u32CoreNum, UINT32 *pu32Core)
{
    INT32 s32Ret = SAL_FAIL;
    
    s32Ret = pSvpDspOps->init(u32CoreNum, pu32Core);
    if (SAL_SOK != s32Ret)
    {
        SVP_LOGE("svp dsp init fail:0x%x\n", s32Ret);
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}


#ifdef DSP_WEAK_FUNC

/*******************************************************************************
* 函数名  : svpdsp_halRegister
* 描  述  : 弱函数当有平台不支持svpdsp时保证编译通过
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
__attribute__((weak)) SVP_DSP_PLAT_OPS *svpdsp_halRegister(void)
{
    return NULL;
}
#endif

/*******************************************************************************
* 函数名  : svpdsp_hal_init
* 描  述  : DSP核初始化
* 输  入  : 
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 svpdsp_hal_init(void)
{
    
    if(NULL == pSvpDspOps)
    {
        pSvpDspOps = svpdsp_halRegister();
        if(NULL == pSvpDspOps)
        {
            SVP_LOGE("svpdsp_halRegister fail!!!\n");
            return SAL_SOK;
        }
    }
    
    return SAL_SOK;
}



