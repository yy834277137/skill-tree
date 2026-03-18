/*******************************************************************************
* vda_hal_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年11月10日 Create
*
* Description : 硬件平台移动侦测模块
* Modification:
*******************************************************************************/
#include <platform_hal.h>
#include <platform_sdk.h>
#include "vda_hal_drv.h"



/*****************************************************************************
                               宏定义
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/

VDA_MOD_HAL_PRM g_stVdaModHalPrm;

static UINT32 uiLevel[7] = {1, 2, 5, 7, 9, 10, 11};
/*****************************************************************************
                                函数
*****************************************************************************/

/*****************************************************************************
 函 数 名  : hal_iveCalcStride
 功能描述  : 计算 参考图片的大小
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
UINT32 hal_iveCalcStride(UINT32 u32Width, UINT32 u8Align)
{
    return (u32Width + (u8Align - u32Width % u8Align) % u8Align);
}

/*****************************************************************************
 函 数 名  : hal_iveCreateMemInfo
 功能描述  : 创建 内存信息
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
HI_S32 hal_iveCreateMemInfo(IVE_MEM_INFO_S* pstMemInfo, HI_U32 u32Size)
{
    HI_S32 s32Ret;

    if (NULL == pstMemInfo)
    {
        SAL_ERROR("pstMemInfo is null\n");
        return SAL_FAIL;
    }
    pstMemInfo->u32Size = u32Size;
    s32Ret = HI_MPI_SYS_MmzAlloc(&pstMemInfo->u64PhyAddr, (void**)&pstMemInfo->u64VirAddr, NULL, HI_NULL, u32Size);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("Mmz Alloc fail,Error(%#x)\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : hal_iveCreateImage
 功能描述  : 创建 IVE 参考图片
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 hal_iveCreateImage(IVE_IMAGE_S* pstImg, IVE_IMAGE_TYPE_E enType, UINT32 u32Width, UINT32 u32Height)
{
    HI_U32 u32Size = 0;
    HI_S32 s32Ret;
    if (NULL == pstImg)
    {
        SAL_ERROR("pstImg is null\n");
        return SAL_FAIL;
    }

    pstImg->enType    = enType;
    pstImg->u32Width  = u32Width;
    pstImg->u32Height = u32Height;
    pstImg->au32Stride [0] = hal_iveCalcStride(pstImg->u32Width, IVE_ALIGN);

    switch (enType)
    {
        case IVE_IMAGE_TYPE_U8C1:
        {
            u32Size = pstImg->au32Stride[0] * pstImg->u32Height;
            s32Ret = HI_MPI_SYS_MmzAlloc(&pstImg->au64PhyAddr[0], (void**)&pstImg->au64VirAddr[0], NULL, HI_NULL, u32Size);
            if (s32Ret != HI_SUCCESS)
            {
                SAL_ERROR("Mmz Alloc fail,Error(%#x)\n", s32Ret);
                return SAL_FAIL;
            }
            break;
        }
        default:
        {
            break;
        }
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Vda_halMdChnCreate
 功能描述  : 移动侦测通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 hal_mdChnCreate(UINT32 uiChn)
{
    INT32 s32Ret   = 0;
    UINT32 u8WndSz = 0;
    MD_ATTR_S stMdAttr;
    UINT32 u32Size = 0;
    INT32 i = 0;

    VDA_HAL_CHN_PRM *pstChnPrm = NULL;

    pstChnPrm = &g_stVdaModHalPrm.stVdaHalChnPrm[uiChn];

    memset(&stMdAttr, 0, sizeof(MD_ATTR_S));

    /* 创建参考图片 */
    for (i = 0;i < IVE_MD_IMAGE_NUM;i++)
    {
        s32Ret = hal_iveCreateImage(&pstChnPrm->astImg[i],IVE_IMAGE_TYPE_U8C1,MAX_MD_WIDTH,MAX_MD_HEIGHT);
        if(s32Ret != HI_SUCCESS)
        {
           SAL_ERROR("hal_iveCreateImage fail\n");
           return SAL_FAIL;
        }
    }

    u32Size = sizeof(IVE_CCBLOB_S);
    s32Ret = hal_iveCreateMemInfo(&pstChnPrm->stBlob,u32Size);

    s32Ret = hal_iveCreateImage(&pstChnPrm->stIveDstImage,IVE_IMAGE_TYPE_U8C1, MAX_MD_WIDTH/4, MAX_MD_HEIGHT/4);
    if(s32Ret != HI_SUCCESS)
    {
       SAL_ERROR("hal_iveCreateImage fail\n");
       return SAL_FAIL;
    }

    /* 创建 ivs md 通道 */
    stMdAttr.enAlgMode    = MD_ALG_MODE_BG; // MD_ALG_MODE_BG; // MD_ALG_MODE_REF
    stMdAttr.enSadMode    = IVE_SAD_MODE_MB_4X4;
    stMdAttr.enSadOutCtrl = IVE_SAD_OUT_CTRL_8BIT_BOTH;// IVE_SAD_OUT_CTRL_THRESH;
    stMdAttr.u16SadThr    = 100 * (1 << 1);
    stMdAttr.u32Width     = MAX_MD_WIDTH;
    stMdAttr.u32Height    = MAX_MD_HEIGHT;

    stMdAttr.stAddCtrl.u0q16X = 32768;
    stMdAttr.stAddCtrl.u0q16Y = 32768;
    stMdAttr.stCclCtrl.enMode = IVE_CCL_MODE_4C;
    u8WndSz = ( 1 << (2 + stMdAttr.enSadMode));
    stMdAttr.stCclCtrl.u16InitAreaThr = u8WndSz * u8WndSz;
    stMdAttr.stCclCtrl.u16Step        = u8WndSz;

    s32Ret = HI_IVS_MD_CreateChn(uiChn,&stMdAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_ERROR("HI_IVS_MD_CreateChn fail,Error(%#x)\n",s32Ret);
        return SAL_FAIL;
    }

	memset((UINT8 *)pstChnPrm->mask,0,18 * sizeof(UINT32));
	
	for (i = 4;i < 18;i++)
	{
	    pstChnPrm->mask[i] = 0xffffffff;
	}
	
    pstChnPrm->uiLevel = 3;
	pstChnPrm->curIndx = 0;
	pstChnPrm->rec.x   = 0;
	pstChnPrm->rec.y   = 0;
	pstChnPrm->rec.width  = MAX_MD_WIDTH;
	pstChnPrm->rec.height = MAX_MD_HEIGHT;
	
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Vda_halMdChnCreate
 功能描述  : 移动侦测通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 hal_mdInit( )
{
    INT32 iRet = 0;
     if (HI_SUCCESS != (iRet = HI_IVS_MD_Init( )))
     {
         SAL_ERROR("IVS MD Init Failed, %x !!!\n", iRet);

         return SAL_FAIL;
     }

     return SAL_SOK;
}

INT32 Vda_halGetExtFrmaeInfo(SYSTEM_FRAME_INFO  *pstSysFrameInfo)
{
    VIDEO_FRAME_INFO_S *pOutFrm = NULL;
	
    if(pstSysFrameInfo->uiAppData == 0x00)
	{
		pOutFrm = (VIDEO_FRAME_INFO_S *)SAL_memAlloc(sizeof(VIDEO_FRAME_INFO_S), "vda", NULL);
		SAL_clear(pOutFrm);

		pstSysFrameInfo->uiAppData  = (PhysAddr)pOutFrm;
		pstSysFrameInfo->uiDataAddr = (PhysAddr)pOutFrm->stVFrame.u64VirAddr[0];
	}

	return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_halIVEProc
 功能描述  : IVE 处理
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halIVEProc(UINT32 uiChn, UINT32 curIndx,UINT32 *pstResult, UINT32 *puiMoved)
{
    INT32            s32Ret    = 0;
    VDA_HAL_CHN_PRM *pstChnPrm = NULL;

    IVE_IMAGE_S     *pstIveImage = NULL;

    UINT8          *pAddr     = NULL;
    UINT32 i        = 0;
    UINT32 j        = 0;
    UINT8  sadValue = 0;
    UINT32 V_pos    = 0;
    UINT32 H_shift  = 0;
	//UINT32 start_x  = 0,start_y = 0;

    pstChnPrm = &g_stVdaModHalPrm.stVdaHalChnPrm[uiChn];

	if (curIndx >= 2)
	{
	    curIndx = 0;
		SAL_WARN("IVS MD Process curIndx %d error\n",curIndx);
	}

	if (NULL == pstResult)
	{
	    SAL_ERROR("uiChn %d pstResult is null!!!\n", uiChn);
        return SAL_FAIL;
	}
	
    s32Ret = HI_IVS_MD_Process(pstChnPrm->uiChn, &pstChnPrm->astImg[curIndx], &pstChnPrm->astImg[1-curIndx], &pstChnPrm->stIveDstImage, &pstChnPrm->stBlob);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_ERROR("IVS MD Process Failed, %x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    pstIveImage = &pstChnPrm->stIveDstImage;
	
    for(i = 0; i < pstIveImage->u32Height; i++)
    {
        V_pos = i / 8;

        pAddr = (UINT8 *)HI_ADDR_U642P(pstIveImage->au64VirAddr[0] + i * (HI_U32)pstIveImage->au32Stride[0]);
        for(j = 0; j < pstIveImage->u32Width; j++)
        {
            sadValue = (UINT8)pAddr[j];
            H_shift = j / 8;
            if (sadValue <= uiLevel[pstChnPrm->uiLevel])   /* 暂定 5 */
            {
                pstResult[V_pos]   |= (0 << H_shift);
                continue;
            }
            
            pstResult[V_pos]   |= (1 << H_shift);
			
			if (!((pstChnPrm->mask[V_pos]  >> (MD_H_SIZE - j)) & 0x01))
			{
			     continue;
			}

            *puiMoved = 1;
        }
    }

    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : Vda_halIVEDmaImage
 功能描述  : DMA 帧信息到 ive 图片
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halIVEDmaImage(UINT32 uiChn, VOID *pFrame, UINT32 index)
{
    INT32 s32Ret = 0;
    //UINT32 index = 0;

    IVE_HANDLE     hIveHandle;
    IVE_SRC_DATA_S stSrcData;
    IVE_DST_DATA_S stDstData;
    IVE_DMA_CTRL_S stCtrl = {IVE_DMA_MODE_DIRECT_COPY,0};

    HI_BOOL bFinish = HI_FALSE;
    HI_BOOL bBlock  = HI_TRUE;

    VDA_HAL_CHN_PRM    *pstChnPrm    = NULL;
    VIDEO_FRAME_INFO_S *pstFrameInfo = (VIDEO_FRAME_INFO_S *)pFrame;

    if (NULL == pstFrameInfo)
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    }

    pstChnPrm = &g_stVdaModHalPrm.stVdaHalChnPrm[uiChn];

    if ((0 == pstFrameInfo->stVFrame.u32Width) || (0 == pstFrameInfo->stVFrame.u32Height))
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    }

    /* fill src */
    stSrcData.u64VirAddr = (HI_U64)pstFrameInfo->stVFrame.u64VirAddr[0];
    stSrcData.u64PhyAddr = pstFrameInfo->stVFrame.u64PhyAddr[0];
    stSrcData.u32Width   = (HI_U16)pstFrameInfo->stVFrame.u32Width;
    stSrcData.u32Height  = (HI_U16)pstFrameInfo->stVFrame.u32Height;
    stSrcData.u32Stride  = (HI_U16)pstFrameInfo->stVFrame.u32Stride[0];

    //index = pstChnPrm->curIndx % IVE_MD_IMAGE_NUM;
    //pstChnPrm->curIndx = index;
    /* fill dst */
    stDstData.u64VirAddr = pstChnPrm->astImg[index].au64VirAddr[0];
    stDstData.u64PhyAddr = pstChnPrm->astImg[index].au64PhyAddr[0];
    stDstData.u32Width   = pstChnPrm->astImg[index].u32Width;
    stDstData.u32Height  = pstChnPrm->astImg[index].u32Height;
    stDstData.u32Stride  = pstChnPrm->astImg[index].au32Stride[0];

    s32Ret = HI_MPI_IVE_DMA(&hIveHandle,&stSrcData,&stDstData,&stCtrl,SAL_TRUE);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_ERROR("HI_MPI_IVE_DMA fail,Error %#x  bInstant %d \n",s32Ret, index);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);
    while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
    {
        usleep(100);
        s32Ret = HI_MPI_IVE_Query(hIveHandle,&bFinish,bBlock);
    }
    if (HI_SUCCESS != s32Ret)
    {
        SAL_ERROR("HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Vda_halMdChnSetLevel
 功能描述  : 移动侦测 灵敏度
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdChnSetLevel(UINT32 uiChn, UINT32 uiLevel)
{
    VDA_HAL_CHN_PRM *pstChnPrm = NULL;

    pstChnPrm = &g_stVdaModHalPrm.stVdaHalChnPrm[uiChn];

    pstChnPrm->uiLevel = uiLevel;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_halMdChnSetReg
 功能描述  : 移动侦测 检测区域
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdChnSetReg(UINT32 uiChn, UINT32 *RegMask)
{
    VDA_HAL_CHN_PRM *pstChnPrm = NULL;

    pstChnPrm = &g_stVdaModHalPrm.stVdaHalChnPrm[uiChn];

	if (NULL == RegMask)
	{
	    SAL_ERROR("uiChn %d is NULL\n",uiChn);
	    return SAL_FAIL;
	}

	memcpy((UINT8*)pstChnPrm->mask,RegMask,MD_V_SIZE * sizeof(UINT32));

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Vda_halMdChnStop
 功能描述  : 移动侦测通道结束
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdChnStop(UINT32 uiChn)
{
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_halMdChnStart
 功能描述  : 移动侦测通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdChnStart(UINT32 uiChn)
{

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Vda_halMdChnCreate
 功能描述  : 移动侦测通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdChnCreate(UINT32 uiChn)
{
    VDA_HAL_CHN_PRM *pstChnPrm = NULL;

    pstChnPrm = &g_stVdaModHalPrm.stVdaHalChnPrm[uiChn];

    if (SAL_SOK != hal_mdChnCreate(uiChn))
    {
        SAL_ERROR("!!!\n");
        return SAL_FAIL;
    }

    pstChnPrm->uiChn = uiChn;
	pstChnPrm->curIndx = 0;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_halMdExit
 功能描述  : 移动侦测模块去初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdExit()
{
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Vda_halMdInit
 功能描述  : 移动侦测模块初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Vda_halMdInit(void)
{
    //INT32 iRet = 0;

    if (SAL_SOK != hal_mdInit())
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}
