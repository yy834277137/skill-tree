/*******************************************************************************
 * vdec_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wutao <wutao19@hikvision.com.cn>
 * Version: V1.0.0  2019年7月13日 Create
 *
 * Description :
 * Modification:
 *******************************************************************************/
#include "vda_tsk.h"
//#include "vda_drv.h"


/*****************************************************************************
 函 数 名  : Vda_tskMdChnSet
 功能描述  : 移动侦测 参数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Host_VdaMdChnSet(UINT32 uiChn, VOID *pPrm)
{
    INT32 ret = SAL_SOK;
	MD_CONFIG *pCfgPrm = NULL;

	if (NULL == pPrm)
	{
	    SAL_ERROR("uiChn(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}
	
	pCfgPrm = pPrm;
	
    ret = Vda_DrvMdChnSet(uiChn,pCfgPrm);
	if (SAL_SOK != ret)
	{
	    SAL_ERROR("uiChn(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}
    SAL_INFO("uiChn(%u) ok !!!\n", uiChn);
    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : Vda_tskMdChnStop
 功能描述  : 移动侦测通道结束
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Host_VdaMdChnStop(UINT32 uiChn)
{
    INT32 ret = SAL_SOK;
    ret = Vda_DrvMdChnStop(uiChn);
	if (SAL_SOK != ret)
	{
	    SAL_ERROR("uiChn(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}
	SAL_INFO("uiChn(%u) ok !!!\n", uiChn);
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_tskMdChnStart
 功能描述  : 移动侦测通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Host_VdaMdChnStart(UINT32 uiChn)
{
    INT32 ret = SAL_SOK;
	
	ret = Vda_DrvMdChnStart(uiChn);
	if (SAL_SOK != ret)
	{
	    SAL_ERROR("uiChn(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}
	SAL_INFO("uiChn(%u) ok !!!\n", uiChn);
    return SAL_SOK;
}

