/*******************************************************************************
* vda_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao19 <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年7月10日 Create
*
* Description : 硬件平台移动侦测模块
* Modification:
*******************************************************************************/
#include "vda_drv.h"

#include "system_prm_api.h"

/*****************************************************************************
                               宏定义
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/

static VDA_MOD_DRV_PRM g_stVdaModDrvPrm = {0};

/*****************************************************************************
                                函数
*****************************************************************************/
/*******************************************************************************
* 函数名  : Vda_DrvMdWait
* 描  述  : 
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*			HIK_FAIL : 失败
*******************************************************************************/
INT32 Vda_DrvMdWait(UINT32 chan)
{
	VDA_DRV_DEV_INFO *pstVdaDevInfo = NULL;


	if (chan >= DRV_MAX_VDA_CHN_NUM)
	{
		SAL_ERROR("chan(%u) error !!!\n", chan);
		return SAL_FAIL;
	}

	pstVdaDevInfo = &g_stVdaModDrvPrm.stVdaDrvChnPrm[chan];


	SAL_mutexLock(pstVdaDevInfo->MutexHandle);

	if (SAL_FALSE == pstVdaDevInfo->isStart)
	{
		SAL_mutexWait(pstVdaDevInfo->MutexHandle);
	}
			
	SAL_mutexUnlock(pstVdaDevInfo->MutexHandle);

	return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Vda_DrvMdChnSet
 功能描述  : 移动侦测 参数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnSet(UINT32 uiChn, MD_CONFIG *pCfgPrm)
{
    INT32 ret = SAL_SOK;

	if (NULL == pCfgPrm)
	{
	    SAL_ERROR("chan(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}
	
    ret = Vda_halMdChnSetLevel(uiChn,pCfgPrm->level);
    if (SAL_SOK != ret)
	{
	    SAL_ERROR("chan(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}

	ret = Vda_halMdChnSetReg(uiChn,pCfgPrm->mask);
    if (SAL_SOK != ret)
	{
	    SAL_ERROR("chan(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}
	
    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : Vda_DrvMdChnStop
 功能描述  : 移动侦测通道结束
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnStop(UINT32 uiChn)
{
    VDA_DRV_DEV_INFO *pstVdaDevInfo = NULL;

	if (uiChn >= DRV_MAX_VDA_CHN_NUM)
	{
		SAL_ERROR("uiChn(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}

	pstVdaDevInfo = &g_stVdaModDrvPrm.stVdaDrvChnPrm[uiChn];
	
    SAL_mutexLock(pstVdaDevInfo->MutexHandle);
    pstVdaDevInfo->isStart= SAL_FALSE;  
    SAL_mutexUnlock(pstVdaDevInfo->MutexHandle);
	
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_DrvMdChnStart
 功能描述  : 移动侦测通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnStart(UINT32 uiChn)
{
	VDA_DRV_DEV_INFO *pstVdaDevInfo = NULL;


	if (uiChn >= DRV_MAX_VDA_CHN_NUM)
	{
		SAL_ERROR("uiChn(%u) error !!!\n", uiChn);
		return SAL_FAIL;
	}

	pstVdaDevInfo = &g_stVdaModDrvPrm.stVdaDrvChnPrm[uiChn];
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstVdaDevInfo->MutexHandle);
	
    SAL_mutexLock(pstVdaDevInfo->MutexHandle);
    pstVdaDevInfo->isStart= SAL_TRUE;
    SAL_mutexSignal(pstVdaDevInfo->MutexHandle);
    SAL_mutexUnlock(pstVdaDevInfo->MutexHandle);
	
    return SAL_SOK;
}


static INT32 Vda_DrvMdGetDupData(VDA_DRV_DEV_INFO *pstVdaDevInfo, DUP_COPY_DATA_BUFF *stDupCopyInfo)
{
    DUP_ChanHandle    *dupOutChnHandle = NULL;

    
    if ((NULL == stDupCopyInfo)||(NULL == pstVdaDevInfo))
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    } 

    dupOutChnHandle = (DUP_ChanHandle *)pstVdaDevInfo->DupHandle;

    if (SAL_SOK != dupOutChnHandle->dupOps.OpDupGetBlit((Ptr)dupOutChnHandle, stDupCopyInfo))
    {                        
		SAL_ERROR("!!!\n");
		return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 Vda_DrvMdPutDupData(VDA_DRV_DEV_INFO *pstVdaDevInfo, DUP_COPY_DATA_BUFF *stDupCopyInfo)
{
    DUP_ChanHandle    *dupOutChnHandle = NULL;

    
    if ((NULL == stDupCopyInfo)||(NULL == pstVdaDevInfo))
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    } 

    dupOutChnHandle = (DUP_ChanHandle *)pstVdaDevInfo->DupHandle;
    if (NULL == dupOutChnHandle)
    {                   
		SAL_ERROR("!!!\n");
		return SAL_FAIL;
    }
	
    if (SAL_SOK != dupOutChnHandle->dupOps.OpDupPutBlit((Ptr)dupOutChnHandle, stDupCopyInfo))
    {                        
		SAL_ERROR("!!!\n");
		return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_halIVEProc
 功能描述  : IVE 处理
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void* Vda_DrvMainProc(void * prm)
{
    INT32  s32Ret   = 0;
	UINT32 uiChn;
	VDA_DRV_DEV_INFO *pstVdaDevInfo = NULL; 
	DUP_COPY_DATA_BUFF stDupCopyInfo = {0};
	SYSTEM_FRAME_INFO  stSysFrameInfo;
	MD_CB_RESULT_ST    stMdCbResult;
	STREAM_ELEMENT     stStreamEle;
	VOID *pFrame;
	UINT32 *pstResult = NULL;
	//UINT32 bInstant;
	UINT32 uiMoved;
	UINT32 curIndx = 0;

	

	uiChn = (UINT32)((PhysAddr)prm);
    if (DRV_MAX_VDA_CHN_NUM  <= uiChn)
	{
	    SAL_ERROR("error %d!!!\n",uiChn);
        return NULL;
	}
	
	memset(&stSysFrameInfo,0x00,sizeof(SYSTEM_FRAME_INFO));

	Vda_halGetExtFrmaeInfo(&stSysFrameInfo);

	stDupCopyInfo.stDupDataCopyPrm.uiRotate = 0xFFFF;
	stDupCopyInfo.pstDstSysFrame = &stSysFrameInfo;
	pstVdaDevInfo = &g_stVdaModDrvPrm.stVdaDrvChnPrm[uiChn];
    SAL_SET_THR_NAME();
	
	while(1)
	{
	    Vda_DrvMdWait(uiChn);
		uiMoved = 0;
		//SAL_INFO("add %x\n",pstVdaDevInfo->DupHandle);
	   	s32Ret = Vda_DrvMdGetDupData(pstVdaDevInfo, &stDupCopyInfo);
		if (SAL_SOK != s32Ret)
		{
		    SAL_ERROR("error %d !!!\n",uiChn);
	        continue;
		}
		
	    pFrame = (VOID *)stDupCopyInfo.pstDstSysFrame->uiAppData;
	    
        if (pstVdaDevInfo->firstFrame)
        {
            curIndx = 0;
            s32Ret = Vda_halIVEDmaImage(uiChn,pFrame, curIndx);
			if (SAL_SOK != s32Ret)
			{
			    SAL_ERROR("error %d!!!\n",uiChn);
		        goto putdata;
			}
			
			pstVdaDevInfo->firstFrame = SAL_FALSE;
        }
		else
		{
		    s32Ret = Vda_halIVEDmaImage(uiChn,pFrame, 1 - curIndx);
			if (SAL_SOK != s32Ret)
			{
			    SAL_ERROR("error %d!!!\n",uiChn);
		        goto putdata;
			}
            pstResult = (UINT32 *)&stMdCbResult.uiMdResult;
			memset((UINT8*)pstResult, 0, sizeof(UINT32) * MD_MAX_LINE_CNT);
		    s32Ret = Vda_halIVEProc(uiChn,curIndx, pstResult, &uiMoved);
			if (SAL_SOK != s32Ret)
			{
			    SAL_ERROR("error %d!!!\n",uiChn);
			}

		    memset(&stStreamEle, 0, sizeof(STREAM_ELEMENT));
            stStreamEle.type = STREAM_ELEMENT_MD_RESULT;
            stStreamEle.chan = uiChn;
            stMdCbResult.uiMoved = uiMoved;
            SystemPrm_cbFunProc(&stStreamEle, (UINT8 *)&stMdCbResult, sizeof(MD_CB_RESULT_ST));
			
		    curIndx = 1 - curIndx;
		}
		
		putdata:
		s32Ret = Vda_DrvMdPutDupData(pstVdaDevInfo, &stDupCopyInfo);
		if (SAL_SOK != s32Ret)
		{
		    SAL_ERROR("error %d!!!\n",uiChn);
		}

	    usleep(200*1000);
	}


    
    return NULL;
}

/*****************************************************************************
 函 数 名  : Vda_DrvMdChnCreateMode
 功能描述  : 移动侦测通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnCreateMode(UINT32 uiChn,void *DupHandle)
{
    INT32 iRet = 0;
    DUP_ChanHandle    *dupOutChnHandle = NULL;
	DUP_OBJ_PRM     *pstDupObjPrm  = NULL; 
	UINT32          uiMod         = 0;
	UINT32          dupChn        = 0;
    PARAM_INFO_S stParamInfo;
	
	VDA_DRV_DEV_INFO *pstVdaDevInfo = NULL;
	
    if (NULL == DupHandle)
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    }

	if (uiChn >= DRV_MAX_VDA_CHN_NUM)
	{
	    SAL_ERROR("uiDev %d error!!!\n",uiChn);
        return SAL_FAIL;
	}
	
    pstVdaDevInfo = &g_stVdaModDrvPrm.stVdaDrvChnPrm[uiChn];
    pstVdaDevInfo->DupHandle = DupHandle;
	
    iRet = Vda_halMdChnCreate(uiChn);
	if (SAL_SOK != iRet)
	{
	    SAL_ERROR("!!!\n");
        return SAL_FAIL;
	}

	pstVdaDevInfo->width      = DRV_MAX_MD_WIDTH;
	pstVdaDevInfo->height     = DRV_MAX_MD_HEIGHT;
	pstVdaDevInfo->firstFrame = SAL_TRUE;
	pstVdaDevInfo->isStart    = SAL_FALSE;
	
    dupOutChnHandle = (DUP_ChanHandle *)pstVdaDevInfo->DupHandle;

	/* 获取通道属性 */
	if (SAL_SOK != Dup_tskGetChnPrm(dupOutChnHandle, &pstDupObjPrm))
	{
	   SAL_ERROR("Dup Get Chn Prm Failed !!!\n");
	   return SAL_FAIL;
	}
		   
    uiMod = dupOutChnHandle->u32Grp;
    dupChn = dupOutChnHandle->u32Chn;

	SAL_INFO("%d,pvssChn:%d.stHalVpssGrpPrm0.uiVpssChn %d stHalVpssGrpPrm1.uiVpssChn %d \n",uiMod, dupChn,pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm0.uiVpssChn,pstDupObjPrm->stDupChnCTrlPrm.stHalVpssGrpPrm1.uiVpssChn);

    memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
    stParamInfo.enType = IMAGE_SIZE_CFG;
    
    stParamInfo.stImgSize.u32Width = pDispChn->uiW;
    stParamInfo.stImgSize.u32Height = pDispChn->uiH;
    iRet = dupOutChnHandle->dupOps.OpDupSetBlitPrm((Ptr)dupOutChnHandle, &stParamInfo);
    if (SAL_isFail(iRet))
    {
        DISP_LOGE("OpDupSetBlitPrm Chn err !!!\n");
        return SAL_FAIL;
    }

	SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstVdaDevInfo->MutexHandle);

	SAL_thrCreate(&pstVdaDevInfo->hndl, Vda_DrvMainProc, SAL_THR_PRI_DEFAULT, 0, (void *)((PhysAddr)uiChn));
	
	
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_DrvMdDeInit
 功能描述  : 移动侦测模块去初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdDeInit()
{
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_DrvMdInit
 功能描述  : 移动侦测模块初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdInit(void)
{
    INT32 iRet = 0;
    iRet = Vda_halMdInit();
	if (SAL_SOK != iRet)
	{
	    SAL_ERROR("!!!\n");
        return SAL_FAIL;
	}
	
    return SAL_SOK;
}
