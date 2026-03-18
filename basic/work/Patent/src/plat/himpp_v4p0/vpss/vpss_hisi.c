/*******************************************************************************
* dup_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年09月19日 Create
*
* Description : 硬件平台 Dup 模块
* Modification:
*******************************************************************************/

#include <sal.h>
//#include <dspcommon.h>
#include <platform_sdk.h>
#include "capbility.h"

//#include "../include/hal_inc/dup_hal.h"
#include "dup_hal_inter.h"

static DUP_PLAT_OPS g_stDupPlatOps;


static INT32 setVpssChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{
    VPSS_CHN_ATTR_S  stVpssChnAttr;
    HI_S32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = HI_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x \n", u32GrpId, u32Chn,Ret);
        return SAL_FAIL;
    } 

    if (stVpssChnAttr.u32Width == u32W && stVpssChnAttr.u32Height == u32H)
    {
        return SAL_SOK;
    }

    stVpssChnAttr.u32Width  = u32W;
    stVpssChnAttr.u32Height = u32H;

    Ret = HI_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_SetChnAttr failed with %#x\n", Ret);
        return SAL_FAIL;
    }  

    return SAL_SOK;
}

static INT32 getVpssChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    VPSS_CHN_ATTR_S  stVpssChnAttr;
    HI_S32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = HI_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x\n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    } 

	*pU32W = stVpssChnAttr.u32Width;
    *pU32H = stVpssChnAttr.u32Height;

    return SAL_SOK;
}

static INT32 setVpssExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{
    VPSS_EXT_CHN_ATTR_S  stVpssChnAttr;
    HI_S32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_EXT_CHN_ATTR_S));
    Ret = HI_MPI_VPSS_GetExtChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x \n", u32GrpId, u32Chn,Ret);
        return SAL_FAIL;
    } 

    if (stVpssChnAttr.u32Width == u32W && stVpssChnAttr.u32Height == u32H)
    {
         return SAL_SOK;
    }

    stVpssChnAttr.u32Width  = u32W;
    stVpssChnAttr.u32Height = u32H;

    Ret = HI_MPI_VPSS_SetExtChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_SetChnAttr failed with %#x\n", Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 getVpssExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    VPSS_EXT_CHN_ATTR_S  stVpssChnAttr;
    HI_S32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_EXT_CHN_ATTR_S));
    Ret = HI_MPI_VPSS_GetExtChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x\n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    }

	*pU32W = stVpssChnAttr.u32Width;
    *pU32H = stVpssChnAttr.u32Height;

    return SAL_SOK;
}


static INT32 setVpssChnMirror(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32Mirror,UINT32 u32Flip)
{
    INT32 s32Ret = 0;
    VPSS_CHN_ATTR_S  pstChnAttr;

    /* 需要做此操作 */
    s32Ret = HI_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &pstChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        DUP_LOGE("HI_MPI_VPSS_GetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    pstChnAttr.bMirror = u32Mirror;
    pstChnAttr.bFlip   = u32Flip;

    s32Ret = HI_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &pstChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        DUP_LOGE("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssChnFps(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32Fps)
{
    INT32 s32Ret = 0;
    VPSS_CHN_ATTR_S pstChnAttr;
    
    /* 需要做此操作 */
    s32Ret = HI_MPI_VPSS_GetChnAttr(u32GrpId, u32VpssChn, &pstChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        DUP_LOGE("HI_MPI_VPSS_GetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }
    
    DUP_LOGD("s32SrcFrameRate = %d s32DstFrameRate = %d \n",pstChnAttr.stFrameRate.s32SrcFrameRate, pstChnAttr.stFrameRate.s32DstFrameRate);

    pstChnAttr.stFrameRate.s32DstFrameRate = u32Fps;

    s32Ret = HI_MPI_VPSS_SetChnAttr(u32GrpId, u32VpssChn, &pstChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        DUP_LOGE("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssChnRotate(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32Rotate)
{
    INT32 s32Ret      = 0;

    s32Ret = HI_MPI_VPSS_SetChnRotation(u32GrpId, u32VpssChn, u32Rotate);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_SetChnRotation failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssChnCrop(UINT32 u32GrpId, UINT32 u32VpssChn, PARAM_INFO_S *pParamInfo)
{
    INT32 s32Ret      = 0;

    VPSS_CROP_INFO_S stCropParam = {0};
    stCropParam.bEnable             = pParamInfo->stCrop.u32CropEnable;
    stCropParam.enCropCoordinate    = VPSS_CROP_ABS_COOR;
    stCropParam.stCropRect.s32X     = SAL_align(pParamInfo->stCrop.u32X,2);
    stCropParam.stCropRect.s32Y     = SAL_align(pParamInfo->stCrop.u32Y,2);
    stCropParam.stCropRect.u32Width = SAL_align(pParamInfo->stCrop.u32W,2);
    stCropParam.stCropRect.u32Height= SAL_align(pParamInfo->stCrop.u32H,2);
    
    DUP_LOGD("grp %d,chn %d,x %d y %d w %d h %d[%d,%d]\n", u32GrpId, u32VpssChn,
		stCropParam.stCropRect.s32X, stCropParam.stCropRect.s32Y,
		stCropParam.stCropRect.u32Width, stCropParam.stCropRect.u32Height,
		stCropParam.stCropRect.s32X + (stCropParam.stCropRect.u32Width / 2),
		stCropParam.stCropRect.s32Y + (stCropParam.stCropRect.u32Height / 2));

    s32Ret = HI_MPI_VPSS_SetChnCrop(u32GrpId, u32VpssChn, &stCropParam);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_SetChnCrop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssGrpCrop(UINT32 u32GrpId, UINT32 u32VpssChn, PARAM_INFO_S *pParamInfo)
{
    INT32 s32Ret      = 0;
    VPSS_CROP_INFO_S stCropParam = {0};
	
    s32Ret = HI_MPI_VPSS_GetGrpCrop(u32GrpId, &stCropParam);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_GetGrpCrop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }
	
	stCropParam.bEnable             = pParamInfo->stCrop.u32CropEnable;
    stCropParam.enCropCoordinate    = VPSS_CROP_ABS_COOR;
    stCropParam.stCropRect.s32X     = pParamInfo->stCrop.u32X > 0 ? pParamInfo->stCrop.u32X : 0;
    stCropParam.stCropRect.s32Y     = pParamInfo->stCrop.u32Y > 0 ? pParamInfo->stCrop.u32Y : 0;
    stCropParam.stCropRect.u32Width = pParamInfo->stCrop.u32W > 64 ? pParamInfo->stCrop.u32W : 64;
    stCropParam.stCropRect.u32Height= pParamInfo->stCrop.u32H > 64 ? pParamInfo->stCrop.u32H : 64;
    
    DUP_LOGD("grp %d,chn %d,x %d y %d w %d h %d[%d,%d]\n", u32GrpId, u32VpssChn,
		stCropParam.stCropRect.s32X, stCropParam.stCropRect.s32Y,
		stCropParam.stCropRect.u32Width, stCropParam.stCropRect.u32Height,
		stCropParam.stCropRect.s32X + (stCropParam.stCropRect.u32Width / 2),
		stCropParam.stCropRect.s32Y + (stCropParam.stCropRect.u32Height / 2));
	s32Ret = HI_MPI_VPSS_SetGrpCrop(u32GrpId, &stCropParam);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_SetGrpCrop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 vpss_hisi_getPhyChnNum()
{
    return VPSS_MAX_PHY_CHN_NUM;
}

static INT32 vpss_hisi_getExtChnNum()
{
    return VPSS_MAX_EXT_CHN_NUM;
}

static INT32 vpss_hisi_getFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32 iRet = 0;
    //UINT32 u32Size = 0;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;
    void *pVirAddr = NULL;
    
    if (!pstFrame)
    {
        return SAL_FAIL;
    }
    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    iRet = HI_MPI_VPSS_GetChnFrame(u32GrpId, u32Chn, pstVideoFrame, u32TimeoutMs);
    if (HI_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d Get vpss Chn %d Frame Failed, %x, time %d !!!\n", u32GrpId, u32Chn, iRet, u32TimeoutMs);
        return SAL_FAIL;
    }

#if 1
    iRet = HI_MPI_VB_GetBlockVirAddr(pstVideoFrame->u32PoolId, pstVideoFrame->stVFrame.u64PhyAddr[0], (void **)&pVirAddr);
    if(iRet != HI_SUCCESS)
    {
        DUP_LOGD("pVirAddr is NUll, poolid %d, viraddr %llu, phyaddr %llu, w %d, h %d  \n", 
                 pstVideoFrame->u32PoolId,(PhysAddr)pVirAddr, pstVideoFrame->stVFrame.u64PhyAddr[0],
                 pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
    }
#endif
#if 0
    u32Size = pstVideoFrame->stVFrame.u32Width * pstVideoFrame->stVFrame.u32Height *3/2;
    pVirAddr = HI_MPI_SYS_Mmap(pstVideoFrame->stVFrame.u64PhyAddr[0], u32Size);
    if (SAL_SOK == pVirAddr)
    {
        DUP_LOGW("ywn,hal mmap failed size %d!, phyaddr %llu \n", u32Size, pstVideoFrame->stVFrame.u64PhyAddr[0]);
        return SAL_FAIL;
    }
    pstVideoFrame->stVFrame.u64VirAddr[0] = (PhysAddr)pVirAddr;
    pstVideoFrame->stVFrame.u64VirAddr[1] = (PhysAddr)((CHAR *)pVirAddr + pstVideoFrame->stVFrame.u32Stride[0] * pstVideoFrame->stVFrame.u32Height);
    pstVideoFrame->stVFrame.u64VirAddr[2] = pstVideoFrame->stVFrame.u64VirAddr[1];
    DUP_LOGD("pVirAddr is NUll, poolid %d, viraddr[0] %llu, viraddr[1] %llu, phyaddr %llu, w %d, h %d  \n", 
        pstVideoFrame->u32PoolId,(PhysAddr)pVirAddr,pstVideoFrame->stVFrame.u64VirAddr[1], pstVideoFrame->stVFrame.u64PhyAddr[0],
        pstVideoFrame->stVFrame.u32Stride[0], pstVideoFrame->stVFrame.u32Height);
#endif
    pstFrame->uiDataAddr = (PhysAddr)pVirAddr;
    pstFrame->uiDataWidth = pstVideoFrame->stVFrame.u32Width;
    pstFrame->uiDataHeight = pstVideoFrame->stVFrame.u32Height;
	
    return SAL_SOK;

}

static INT32 vpss_hisi_releaseFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 iRet = 0;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;
    
    if (!pstFrame)
    {
        return SAL_FAIL;
    }
    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }
    
    iRet = HI_MPI_VPSS_ReleaseChnFrame(u32GrpId, u32Chn, pstVideoFrame);
    if (HI_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d release vpss Chn %d Frame Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }
	
    return SAL_SOK;

}

static INT32 vpss_hisi_sendFrame(UINT32 dupChan, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32  iRet = 0;
	UINT32 VpssGroupID = 0;
	VIDEO_FRAME_INFO_S *pstVideoFrame   = NULL;

	if (NULL == pstFrame)
	{
	     DUP_LOGE("Chn %d is null!!!\n", dupChan);
         return SAL_FAIL;
	}
	pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    VpssGroupID = dupChan;
    iRet = HI_MPI_VPSS_SendFrame(VpssGroupID, 0, pstVideoFrame, u32TimeoutMs);
    if (HI_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d Send vpss Frame Failed, %x !!!\n", VpssGroupID, iRet);
        return SAL_FAIL;
    }
	
    return SAL_SOK;
}

static INT32 vpss_hisi_setParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    CFG_TYPE_E enCfgType = 0;
    INT32 s32Ret = 0;

  	if (NULL == pParamInfo)
	{
	     DUP_LOGE("pParamInfo is null!!!\n");
         return SAL_FAIL;
	}

    enCfgType = pParamInfo->enType;
    if (enCfgType <= MIN_CFG || enCfgType >= MAX_CFG)
    {
        DUP_LOGE("s32CfgType is %d, illgal \n", enCfgType);
        return SAL_FAIL;
    }

    switch (enCfgType)
    {
        case IMAGE_SIZE_CFG:
            s32Ret = setVpssChnSize(u32GrpId, u32Chn, pParamInfo->stImgSize.u32Width, pParamInfo->stImgSize.u32Height);
            break;
        
        case IMAGE_SIZE_EXT_CFG:
            s32Ret = setVpssExtChnSize(u32GrpId, u32Chn, pParamInfo->stImgSize.u32Width, pParamInfo->stImgSize.u32Height);
            break;
        
        case MIRROR_CFG:
            s32Ret = setVpssChnMirror(u32GrpId, u32Chn, pParamInfo->stMirror.u32Mirror, pParamInfo->stMirror.u32Flip);
            break;
            
        case FPS_CFG:
            s32Ret = setVpssChnFps(u32GrpId, u32Chn, pParamInfo->u32Fps);
            break;
            
        case ROTATE_CFG:
            s32Ret = setVpssChnRotate(u32GrpId, u32Chn, pParamInfo->u32Rotate);
            break;
    
        case CHN_CROP_CFG:
            s32Ret = setVpssChnCrop(u32GrpId, u32Chn, pParamInfo);
                break;
        case GRP_CROP_CFG:
            s32Ret = setVpssGrpCrop(u32GrpId, u32Chn, pParamInfo);            
                break;
        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}

static INT32 vpss_hisi_getParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    CFG_TYPE_E enCfgType = 0;
    INT32 s32Ret = 0;

  	if (NULL == pParamInfo)
	{
	     DUP_LOGE("pParamInfo is null!!!\n");
         return SAL_FAIL;
	}

    enCfgType = pParamInfo->enType;
    if (enCfgType <= MIN_CFG || enCfgType >= MAX_CFG)
    {
        DUP_LOGE("s32CfgType is %d, illgal \n", enCfgType);
        return SAL_FAIL;
    }

    switch (enCfgType)
    {
        case IMAGE_SIZE_CFG:
            s32Ret = getVpssChnSize(u32GrpId, u32Chn, &(pParamInfo->stImgSize.u32Width), &(pParamInfo->stImgSize.u32Height));
            break;
        case IMAGE_SIZE_EXT_CFG:
            s32Ret = getVpssExtChnSize(u32GrpId, u32Chn, &(pParamInfo->stImgSize.u32Width), &(pParamInfo->stImgSize.u32Height));
            break;

        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}



/*****************************************************************************
 函 数 名  : DfrHal_drvInitVpssGroup
 功能描述  : 初始化 Vpss Group 通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史	   :
  1.日	  期   : 2018年07月02日
	作	  者   : wwq
	修改内容   : 新生成函数
*****************************************************************************/
static INT32 vpss_hisi_create(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm, DUP_HAL_CHN_INIT_ATTR *pstChnInitAttr)
{
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    HI_S32 s32Ret      = 0;
    UINT32 u32VpssGroup = 0;
    INT32  i           = 0;
	UINT32 u32Depth     = 0;
    VB_INFO_S stVbInfo = {0};
    char *pcMmzName = NULL;

    VPSS_EXT_CHN_ATTR_S stVpssExtChnAttr = {0};
	DUP_HAL_OUT_CHN_ATTR *pstOutChnAttr = NULL;


    u32VpssGroup = pstHalVpssGrpPrm->uiVpssChn;
    u32Depth = pstHalVpssGrpPrm->uiChnDepth;

    /*1080x1920编码出错，图像宽高先按16自己对齐*/
    stVbInfo.u32BlkWidth = SAL_align(pstHalVpssGrpPrm->uiGrpWidth, 16);
    stVbInfo.u32BlkHeight = SAL_align(pstHalVpssGrpPrm->uiGrpHeight, 16);
    stVbInfo.u32BlkCnt = pstHalVpssGrpPrm->u32BlkCnt;

     pcMmzName = NULL; /* 用hisi DSP画osd时需要用匿名内存*/
    if (SAL_SOK != mem_hal_vbPoolAlloc(&stVbInfo, "vpss_hisi", "vpss_vb", pcMmzName))
    {
        DUP_LOGW("alloc buff from dsp zone fail, try anonymous\n");
        pcMmzName = NULL;
        if (SAL_SOK != mem_hal_vbPoolAlloc(&stVbInfo, "vpss_hisi", "vpss_vb", pcMmzName))
        {
            DUP_LOGW("mem_hal_vbPoolAlloc fail\n");
        }
    }
    pstHalVpssGrpPrm->u32PoolID = stVbInfo.u32VbPoolID;
    pstHalVpssGrpPrm->u32PoolSize = stVbInfo.u32PoolSize;

    /* 创建 vpss group */
    stVpssGrpAttr.u32MaxW = pstHalVpssGrpPrm->uiGrpWidth;
    stVpssGrpAttr.u32MaxH = pstHalVpssGrpPrm->uiGrpHeight;
    stVpssGrpAttr.enPixelFormat  = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.bNrEn   = HI_TRUE;

    stVpssGrpAttr.stNrAttr.enNrType                = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enCompressMode          = COMPRESS_MODE_NONE;
    stVpssGrpAttr.stNrAttr.enNrMotionMode          = NR_MOTION_MODE_NORMAL;
    s32Ret = HI_MPI_VPSS_CreateGrp(u32VpssGroup, &stVpssGrpAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VPSS_StartGrp(u32VpssGroup);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    /* 创建 vpss chn 4个物理通道 + 2个扩展通道 */
    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if (ENABLED != pstChnInitAttr->uiOutChnEnable[i])
        {
            continue;
        }
		
        SAL_clear(&stVpssChnAttr);
        stVpssChnAttr.u32Width                     = pstChnInitAttr->stOutChnAttr[i].uiOutChnW;
        stVpssChnAttr.u32Height                    = pstChnInitAttr->stOutChnAttr[i].uiOutChnH;
        stVpssChnAttr.enChnMode                    = VPSS_CHN_MODE_USER;
        stVpssChnAttr.enCompressMode               = COMPRESS_MODE_NONE;//COMPRESS_MODE_SEG;
        stVpssChnAttr.enDynamicRange               = stVpssGrpAttr.enDynamicRange;
        stVpssChnAttr.enPixelFormat                = stVpssGrpAttr.enPixelFormat;
        stVpssChnAttr.stFrameRate.s32SrcFrameRate  = -1;
        stVpssChnAttr.stFrameRate.s32DstFrameRate  = -1;
        stVpssChnAttr.u32Depth                     = u32Depth;
        stVpssChnAttr.bMirror                      = HI_FALSE;
        stVpssChnAttr.bFlip                        = HI_FALSE;
        stVpssChnAttr.enVideoFormat                = VIDEO_FORMAT_LINEAR;
        stVpssChnAttr.stAspectRatio.enMode         = ASPECT_RATIO_NONE;
        s32Ret = HI_MPI_VPSS_SetChnAttr(u32VpssGroup, i, &stVpssChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }
		
		s32Ret = HI_MPI_VPSS_AttachVbPool(u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID );
		if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_AttachVbPool failed with %#x (Grp:%d,chn:%d),u32PoolId %d\n", s32Ret,u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID);
            return SAL_FAIL;
        }
		else
		{
		    DUP_LOGD("HI_MPI_VPSS_AttachVbPool succeed!!! (%d,%d),u32PoolId %d\n",u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID);
		}

        s32Ret = HI_MPI_VPSS_EnableChn(u32VpssGroup, i);
        if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }	
    }

    /* 创建拓展通道 */
    for(i = 0; i < pstHalVpssGrpPrm->uiExtChnNum; i++)
    {
        if (ENABLED != pstChnInitAttr->uiOutChnEnable[i + VPSS_MAX_PHY_CHN_NUM])
        {
            continue;
        }
		
        memset(&stVpssExtChnAttr, 0, sizeof(VPSS_EXT_CHN_ATTR_S));
		
        stVpssExtChnAttr.enCompressMode   = COMPRESS_MODE_NONE;
		stVpssExtChnAttr.enVideoFormat    = VIDEO_FORMAT_LINEAR;
        stVpssExtChnAttr.enPixelFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        stVpssExtChnAttr.s32BindChn       = i;    /* */
        stVpssExtChnAttr.u32Depth         = 1;//pstChnInitAttr->uiDepth;    /* */
        stVpssExtChnAttr.stFrameRate.s32DstFrameRate  = -1;
        stVpssExtChnAttr.stFrameRate.s32SrcFrameRate  = -1;
       
        pstOutChnAttr = &pstChnInitAttr->stOutChnAttr[i + VPSS_MAX_PHY_CHN_NUM];

        stVpssExtChnAttr.u32Height        = pstOutChnAttr->uiOutChnH;
        stVpssExtChnAttr.u32Width         = pstOutChnAttr->uiOutChnW;

        s32Ret = HI_MPI_VPSS_SetExtChnAttr(u32VpssGroup, i + VPSS_MAX_PHY_CHN_NUM, &stVpssExtChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_SetExtChnAttr failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }

        s32Ret = HI_MPI_VPSS_EnableChn(u32VpssGroup, i + VPSS_MAX_PHY_CHN_NUM);
        if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }		
    }
	
    DUP_LOGD("Create Vpss Group Chn %d SUC!!!poolId %d\n", u32VpssGroup, pstHalVpssGrpPrm->u32PoolID);

    pstHalVpssGrpPrm->uiVpssChn = u32VpssGroup;
    return SAL_SOK;
}

static INT32 vpss_hisi_start(UINT32 u32GrpId, UINT32 u32Chn)
{
    HI_S32 s32Ret      = 0;

    s32Ret = HI_MPI_VPSS_EnableChn(u32GrpId, u32Chn);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 vpss_hisi_stop(UINT32 u32GrpId, UINT32 u32Chn)
{
    HI_S32 s32Ret      = 0;

    s32Ret = HI_MPI_VPSS_DisableChn(u32GrpId, u32Chn);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_DisableChn failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


static INT32 vpss_hisi_destroy(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm)
{
    HI_S32 s32Ret      = 0;
    UINT32 i = 0;
    VB_INFO_S stVbInfo = {0};

    for(i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if (ENABLED != pstHalVpssGrpPrm->uiChnEnable[i])
        {
            continue;
        }
		
		s32Ret = HI_MPI_VPSS_DisableChn(pstHalVpssGrpPrm->uiVpssChn, i);
        if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_DisableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }
        
		s32Ret = HI_MPI_VPSS_DetachVbPool(pstHalVpssGrpPrm->uiVpssChn, i);
		if (s32Ret != HI_SUCCESS)
        {
            DUP_LOGE("HI_MPI_VPSS_DetachVbPool failed with %#x (Grp:%d,chn:%d)\n", s32Ret, pstHalVpssGrpPrm->uiVpssChn, i);
            return SAL_FAIL;
        }
		else
		{
		    DUP_LOGD("HI_MPI_VPSS_DetachVbPool succeed!!! (%d,%d)\n", pstHalVpssGrpPrm->uiVpssChn, i);
		}
    }

    s32Ret = HI_MPI_VPSS_StopGrp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_StopGrp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }
    s32Ret = HI_MPI_VPSS_ResetGrp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("hi_mpi_vpss_reset_grp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VPSS_DestroyGrp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        DUP_LOGE("HI_MPI_VPSS_DestroyGrp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    stVbInfo.u32VbPoolID = pstHalVpssGrpPrm->u32PoolID;
    stVbInfo.u32PoolSize = pstHalVpssGrpPrm->u32PoolSize;
    mem_hal_vbPoolFree(&stVbInfo, "vpss_hisi", "vpss_vb");

    return SAL_SOK;
}

static INT32 vpss_hisi_init()
{
    return SAL_SOK;   
}

static INT32 vpss_hisi_deinit()
{
    return SAL_SOK; 
}

const DUP_PLAT_OPS *dup_hal_register()
{
    g_stDupPlatOps.init = vpss_hisi_init;
    g_stDupPlatOps.deinit = vpss_hisi_deinit;
    g_stDupPlatOps.create = vpss_hisi_create;
    g_stDupPlatOps.start = vpss_hisi_start; 
    //g_stDupPlatOps.enableChn = vpss_hisi_enableChn;
    g_stDupPlatOps.stop = vpss_hisi_stop;
    g_stDupPlatOps.destroy = vpss_hisi_destroy;

    g_stDupPlatOps.getFrame = vpss_hisi_getFrame;
    g_stDupPlatOps.releaseFrame = vpss_hisi_releaseFrame;
    g_stDupPlatOps.sendFrame = vpss_hisi_sendFrame;
    g_stDupPlatOps.setParam= vpss_hisi_setParam;
    g_stDupPlatOps.getParam= vpss_hisi_getParam;

    g_stDupPlatOps.getPhyChnNum = vpss_hisi_getPhyChnNum;
	g_stDupPlatOps.getExtChnNum = vpss_hisi_getExtChnNum;

    if (g_stDupPlatOps.init)
    {
        g_stDupPlatOps.init();
    }

    return &g_stDupPlatOps;
}




