/*******************************************************************************
 * sys_hal_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangweiqin <wangweiqin@hikvision.com.cn>
 * Version: V1.0.0  2017年09月08日 Create
 *
 * Description : 硬件平台系统初始化配置文件
 * Modification:
 *******************************************************************************/
#include "platform_sdk.h"
#include "sys_hal_inter.h"
#include "fmtConvert_hisi.h"

static HI_U32 gU32YSize   = 0;

static SYS_PLAT_OPS g_stSysPlatOps;

/*****************************************************************************
                            宏定义
*****************************************************************************/
#define SYSTEM_ALIGN_WIDTH      (64)

/*****************************************************************************
 函 数 名  : System_calcPicVbBlkSize
 功能描述  : 计算vb大小
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月08日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 System_calcPicVbBlkSize(UINT32 uiWidth, UINT32 uiHeight, PIXEL_FORMAT_E enPixFmt, HI_U32 u32AlignWidth,DATA_BITWIDTH_E enBitWidth)
{
    HI_U32 u32VbSize           = 0;
    HI_U32 u32BitWidth         = 0;
    HI_U32 u32MainStride       = 0;
    HI_U32 u32MainSize         = 0;
    HI_U32 u32AlignHeight      = 0;
    HI_U32 u32YSize            = 0;
    //DATA_BITWIDTH_E enBitWidth = DATA_BITWIDTH_10;

    if (0 == u32AlignWidth)
    {
       u32AlignWidth = DEFAULT_ALIGN;
    }
    else if (u32AlignWidth > MAX_ALIGN)
    {
       u32AlignWidth = MAX_ALIGN;
    }
    else
    {
       u32AlignWidth = (ALIGN_UP(u32AlignWidth, DEFAULT_ALIGN));
    }

    switch (enBitWidth)
    {
       case DATA_BITWIDTH_8:
       {
           u32BitWidth = 8;
           break;
       }
       
       case DATA_BITWIDTH_10:
       {
           u32BitWidth = 10;
           break;
       }
       
       case DATA_BITWIDTH_12:
       {
           u32BitWidth = 12;
           break;
       }
       
       case DATA_BITWIDTH_14:
       {
           u32BitWidth = 14;
           break;
       }
       
       case DATA_BITWIDTH_16:
       {
           u32BitWidth = 16;
           break;
       }
       
       default:
       {
           u32BitWidth = 0;
           break;
       }
    }

    u32AlignHeight = ALIGN_UP(uiHeight, 2);
    u32MainStride  = ALIGN_UP((uiWidth * u32BitWidth + 7) >> 3, u32AlignWidth);
    u32YSize       = u32MainStride * u32AlignHeight;

    if (PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixFmt || PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixFmt)
    {
       u32MainSize = (u32MainStride * u32AlignHeight)*3 >> 1;
    }
    else if (PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixFmt || PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixFmt)
    {
       u32MainSize = u32MainStride * u32AlignHeight * 2;
    }
    else if ((PIXEL_FORMAT_YUV_400 == enPixFmt) || (PIXEL_FORMAT_S16C1 == enPixFmt))
    {
       u32MainSize = u32MainStride * u32AlignHeight;
    }
    else
    {
       u32MainSize = u32MainStride * u32AlignHeight * 3;
    }
    gU32YSize = u32YSize;
    u32VbSize   = u32MainSize;

    return u32VbSize;
}



/*****************************************************************************
 函 数 名  : sys_hisi_VIVPSSModeSet
 功能描述  : VI-VPSS模式设置
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月08日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 sys_VIVPSSModeSet()
{
	VI_VPSS_MODE_S   stVIVPSSMode  = {0};
	INT32            s32Ret     = HI_SUCCESS;

	s32Ret = HI_MPI_SYS_GetVIVPSSMode(&stVIVPSSMode);
    if (HI_SUCCESS != s32Ret)
    {
       SYS_LOGE("Get VI-VPSS mode Param failed with %#x!\n", s32Ret);

       return SAL_FAIL;
    }
	// 实际使用几个vi此处就应该配置几个vi的mode 
    stVIVPSSMode.aenMode[0] = VI_ONLINE_VPSS_OFFLINE;
    //stVIVPSSMode.aenMode[1] = VI_OFFLINE_VPSS_OFFLINE;
	stVIVPSSMode.aenMode[4] = VI_ONLINE_VPSS_OFFLINE;
    s32Ret = HI_MPI_SYS_SetVIVPSSMode(&stVIVPSSMode);
    if (HI_SUCCESS != s32Ret)
    {
        SYS_LOGE("Set VI-VPSS mode Param failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
	return s32Ret;
}

/*****************************************************************************
 函 数 名  : SysHal_drvDeInit
 功能描述  : 去初始化硬件平台系统
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月08日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 sys_hisi_deInit()
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : SysHal_drvInit
 功能描述  : 初始化硬件平台系统
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月08日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 sys_hisi_init(UINT32 u32ViChnNum)
{
    HI_U64           u64BlkSize = 0;
    VB_CONFIG_S      stVbConf;
    HI_S32           ret        = 0;
    INT32            s32Ret     = 0;    
    MPP_VERSION_S    stVersion;
	VPSS_MOD_PARAM_S stModParam    = {0};

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    memset(&stVersion, 0, sizeof(MPP_VERSION_S));
    HI_MPI_SYS_GetVersion(&stVersion);

    SYS_LOGD("SDK Version : [ %s ]!!!\n", stVersion.aVersion);

    /* 1. 去初始化系统与vb */
    s32Ret = HI_MPI_SYS_Exit();
	if(HI_SUCCESS != s32Ret)
	{
		SYS_LOGE("sys exit fail! s32Ret 0x%x\n", s32Ret);
	}
	
    s32Ret = HI_MPI_VB_Exit();
	if(HI_SUCCESS != s32Ret)
	{
		SYS_LOGE("vb exit fail! s32Ret 0x%x\n", s32Ret);
	}

    /* 2. 初始化vb内存大小 */
    stVbConf.u32MaxPoolCnt = 100;
    
	UINT32 u32Width	   = 2560;
	UINT32 u32Height   = 1440;
    
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    
    stVbConf.astCommPool[0].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt  = 4 * u32ViChnNum;
	
    u32Width    = 1280;
    u32Height   = 1024;
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt  = 8 * u32ViChnNum;
    
    u32Width    = 1280;
    u32Height   = 720;
    
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    stVbConf.astCommPool[2].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt  = 8 * u32ViChnNum;

    u32Width    = 704;
    u32Height   = 576;
    
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    stVbConf.astCommPool[3].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[3].u32BlkCnt  = 8 * u32ViChnNum;

	u32Width    = 4864;
    u32Height   = 640;
    
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    stVbConf.astCommPool[4].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[4].u32BlkCnt  = 4 * u32ViChnNum;


    u32Width    = 1920;
    u32Height   = 1200;
    
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    stVbConf.astCommPool[5].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[5].u32BlkCnt  = 4 * u32ViChnNum;

    u32Width    = 1920;
    u32Height   = 1920;
    
    u64BlkSize = COMMON_GetPicBufferSize( u32Width, u32Height,PIXEL_FORMAT_YUV_SEMIPLANAR_420,DATA_BITWIDTH_8,COMPRESS_MODE_SEG,DEFAULT_ALIGN);
    stVbConf.astCommPool[6].u64BlkSize = u64BlkSize;
    stVbConf.astCommPool[6].u32BlkCnt  = 4 * u32ViChnNum;

    s32Ret = HI_MPI_VB_SetConfig(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SYS_LOGE("HI_MPI_VB_SetConfig failed 0x%x !\n",s32Ret);
        return SAL_FAIL;
    }

    /* 3. 初始化vb内存 */
    if (HI_SUCCESS != HI_MPI_VB_Init())
    {
        SYS_LOGE("HI_MPI_VB_Init failed!(0x%x)\n",ret);
        return SAL_FAIL;
    }


    /* 5. 初始化sys */
    ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != ret)
    {
        SYS_LOGE("HI_MPI_SYS_Init failed!(0x%x)\n",ret);
        return SAL_FAIL;
    }

	s32Ret = sys_VIVPSSModeSet();
	if (HI_SUCCESS != s32Ret)
    {
        SYS_LOGE("sys_hisi_VIVPSSModeSet ret %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VPSS_GetModParam(&stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        SYS_LOGE("HI_MPI_VPSS_GetModParam ret %#x!\n", s32Ret);
        return SAL_FAIL;
    }

	stModParam.u32VpssVbSource = 2;
    s32Ret = HI_MPI_VPSS_SetModParam(&stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        SYS_LOGE("HI_MPI_VPSS_GetModParam ret %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

INT32 sys_hisi_getPts(UINT64 *pCurPts)
{
	INT32  s32Ret = HI_SUCCESS;
	UINT64 p64CurPts = 0;
	
	if (NULL == pCurPts)
    {
        SYS_LOGE("error\n");
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_SYS_GetCurPTS(&p64CurPts);
    if (s32Ret != HI_SUCCESS)
    {
        VDEC_LOGE("HI_MPI_SYS_GetCurPTS error 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    *pCurPts = p64CurPts;

    return SAL_SOK;
}

INT32 sys_hisi_bind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 uiSrcDevId, UINT32 uiSrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 uiDstDevId, UINT32 uiDstChn,UINT32 uiBind)
{
    INT32  s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    memset(&stSrcChn, 0, sizeof(MPP_CHN_S));
    memset(&stDestChn, 0, sizeof(MPP_CHN_S));

    if (SYSTEM_MOD_ID_DUP == uiSrcModId)
    {
        stSrcChn.enModId  = HI_ID_VPSS;
        stSrcChn.s32DevId = uiSrcDevId;
        stSrcChn.s32ChnId = uiSrcChn;
    }
    else if (SYSTEM_MOD_ID_CAPT == uiSrcModId)
    {
        stSrcChn.enModId  = HI_ID_VI;
        stSrcChn.s32DevId = uiSrcDevId;
        stSrcChn.s32ChnId = 0;

    }
    else if (SYSTEM_MOD_ID_VDEC == uiSrcModId)
    {
        stSrcChn.enModId  = HI_ID_VDEC;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = uiSrcChn;

    }
    else
    {
        SYS_LOGE("uiSrcModId %d not support \n", uiSrcModId);
        return SAL_FAIL;
    }

    if (SYSTEM_MOD_ID_DUP == uiDstModId)
    {
        stDestChn.enModId  = HI_ID_VPSS;
        stDestChn.s32DevId = uiDstDevId;
        stDestChn.s32ChnId = uiDstChn;
    }
    else if (SYSTEM_MOD_ID_DISP == uiDstModId)
    {
        stDestChn.enModId  = HI_ID_VO;
        stDestChn.s32DevId = uiDstDevId;
        stDestChn.s32ChnId = uiDstChn;

    }
    else if (SYSTEM_MOD_ID_VENC == uiDstModId)
    {
        stDestChn.enModId  = HI_ID_VENC;
        stDestChn.s32DevId = 0;
        stDestChn.s32ChnId = uiDstChn;

    }
    else
    {
        SYS_LOGE("uiDstModId %d not support \n", uiDstModId);
        return SAL_FAIL;
    }

    if (1 == uiBind)
    {
        s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        if (s32Ret != HI_SUCCESS)
        {
            SYS_LOGE("SRC M-%d_D-%d_C-%d Bind to  M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
            return SAL_FAIL;
        }

        SYS_LOGD("SRC M-%d_D-%d_C-%d Bind to  M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
    }
    else
    {
        s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
        if (s32Ret != HI_SUCCESS)
        {
            SYS_LOGE("SRC M-%d_D-%d_C-%d unBind from Venc M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
            return SAL_FAIL;
        }

        SYS_LOGD("SRC M-%d_D-%d_C-%d unBind from Venc M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
    }
    return SAL_SOK;
}

INT32 sys_hisi_allocVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    VIDEO_FRAME_INFO_S *pOutFrm = NULL;

    pOutFrm = (VIDEO_FRAME_INFO_S *)SAL_memMalloc(sizeof(VIDEO_FRAME_INFO_S), "sys_hisi", "video_frame");
    if (!pOutFrm)
    {
        SYS_LOGE("alloc mem[mod:sys_hisi, name:video_frame] fail \n");
        return SAL_FAIL;
    }

	memset(pOutFrm,0x00,sizeof(VIDEO_FRAME_INFO_S));
    pstSystemFrameInfo->uiAppData = (PhysAddr)pOutFrm;
    //pstSystemFrameInfo->uiDataAddr = (PhysAddr)pOutFrm->stVFrame.u64VirAddr[0];

    return SAL_SOK;
}

INT32 sys_hisi_releaseVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    VIDEO_FRAME_INFO_S *pOutFrm = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;

    SAL_memfree(pOutFrm, "sys_hisi", "video_frame");
    pstSystemFrameInfo->uiAppData = 0;

    return SAL_SOK;    
}

static INT32 sys_hisi_getPhyFromVir(VOID *pVir, PhysAddr *pPhyAddr)
{
    INT32 s32Ret = SAL_SOK;

	SYS_VIRMEM_INFO_S stVirMemInfo = {0};

    if (NULL == pVir || NULL == pPhyAddr)
    {
    	SYS_LOGE("invalid input args! %p, %p \n", pVir, pPhyAddr);
		return SAL_FAIL;
    }

	s32Ret = HI_MPI_SYS_GetVirMemInfo((VOID *)pVir, &stVirMemInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Vir Mem Info Failed! \n");
        goto exit;
    }

	*pPhyAddr = (PhysAddr)stVirMemInfo.u64PhyAddr;
	
exit:
	return s32Ret;
}

INT32 sys_hisi_buildVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;

    if (!pVideoFrameInfo)
    {
        SYS_LOGE("pVideoFrameInfo     is NULL \n");
        return SAL_FAIL;
    }
    pVideoFrameInfo->u32PoolId          = pVideoFrame->poolId;
    pVideoFrameInfo->stVFrame.u32Width  = pVideoFrame->frameParam.width;
    pVideoFrameInfo->stVFrame.u32Height = pVideoFrame->frameParam.height;
    pVideoFrameInfo->stVFrame.enField   = VIDEO_FIELD_FRAME;
    pVideoFrameInfo->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    pVideoFrameInfo->stVFrame.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    /* 引擎只给了虚拟地址，所以这里需要从虚拟地址转为物理地址。DSP其他模块物理地址需要从上层传下来 */
    pVideoFrameInfo->stVFrame.u64PhyAddr[0] = (0x00 == pVideoFrame->phyAddr[0])  \
		                                     ? (SAL_SOK != sys_hisi_getPhyFromVir((VOID *)pVideoFrame->virAddr[0], &pVideoFrame->phyAddr[0]) ? 0x00 : pVideoFrame->phyAddr[0]) \
		                                     : pVideoFrame->phyAddr[0];
    pVideoFrameInfo->stVFrame.u64PhyAddr[1] = (0x00 == pVideoFrame->phyAddr[1])  \
		                                     ? (SAL_SOK != sys_hisi_getPhyFromVir((VOID *)pVideoFrame->virAddr[1], &pVideoFrame->phyAddr[1]) ? 0x00 : pVideoFrame->phyAddr[1]) \
		                                     : pVideoFrame->phyAddr[1];
    pVideoFrameInfo->stVFrame.u64PhyAddr[2] = (0x00 == pVideoFrame->phyAddr[2])  \
		                                     ? (SAL_SOK != sys_hisi_getPhyFromVir((VOID *)pVideoFrame->virAddr[2], &pVideoFrame->phyAddr[2]) ? 0x00 : pVideoFrame->phyAddr[2]) \
		                                     : pVideoFrame->phyAddr[2];
    pVideoFrameInfo->stVFrame.u64VirAddr[0] = HI_ADDR_P2U64(pVideoFrame->virAddr[0]);
    pVideoFrameInfo->stVFrame.u64VirAddr[1]  = HI_ADDR_P2U64(pVideoFrame->virAddr[1]);
    pVideoFrameInfo->stVFrame.u64VirAddr[2]  = HI_ADDR_P2U64(pVideoFrame->virAddr[2]);
    pVideoFrameInfo->stVFrame.u32Stride[0]  = pVideoFrame->stride[0];
    pVideoFrameInfo->stVFrame.u32Stride[1]  = pVideoFrame->stride[1];
    pVideoFrameInfo->stVFrame.u32Stride[2]  = pVideoFrame->stride[2];
	
	pVideoFrameInfo->stVFrame.u32TimeRef = (UINT32)pVideoFrame->frameNum;
	pVideoFrameInfo->stVFrame.u64PTS = (UINT64)pVideoFrame->pts;
    pVideoFrameInfo->stVFrame.u64PrivateData = (PhysAddr)pVideoFrame->privateDate;  //vbBlk

	pstSystemFrameInfo->uiDataWidth = pVideoFrameInfo->stVFrame.u32Width;
	pstSystemFrameInfo->uiDataHeight = pVideoFrameInfo->stVFrame.u32Height;

    return SAL_SOK; 
}

static INT32 sys_hisi_getVideoFrameInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame)
{
    VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;
    SAL_VideoDataFormat enSalPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    
    fmtConvert_hisi_hisi2sal(pVideoFrameInfo->stVFrame.enPixelFormat, &enSalPixFmt);
    pVideoFrame->poolId = pVideoFrameInfo->u32PoolId;
    pVideoFrame->frameParam.width = pVideoFrameInfo->stVFrame.u32Width;
    pVideoFrame->frameParam.height = pVideoFrameInfo->stVFrame.u32Height;
    pVideoFrame->frameParam.dataFormat = enSalPixFmt;
	pVideoFrame->phyAddr[0] = (PhysAddr)pVideoFrameInfo->stVFrame.u64PhyAddr[0];
	pVideoFrame->phyAddr[1] = (PhysAddr)pVideoFrameInfo->stVFrame.u64PhyAddr[1];
	pVideoFrame->phyAddr[2] = (PhysAddr)pVideoFrameInfo->stVFrame.u64PhyAddr[2];

    if((PhysAddr)pVideoFrameInfo->stVFrame.u64VirAddr[0] != 0x00)
    {
        pVideoFrame->virAddr[0] = (PhysAddr)pVideoFrameInfo->stVFrame.u64VirAddr[0];
        pVideoFrame->virAddr[1] = (PhysAddr)pVideoFrameInfo->stVFrame.u64VirAddr[1];
        pVideoFrame->virAddr[2] = (PhysAddr)pVideoFrameInfo->stVFrame.u64VirAddr[2];
    }
    else if(pstSystemFrameInfo->uiDataAddr != 0x00)
    {
        pVideoFrame->virAddr[0] = pstSystemFrameInfo->uiDataAddr;
        pVideoFrame->virAddr[1] = pVideoFrame->virAddr[0] + pVideoFrameInfo->stVFrame.u32Stride[0] * pVideoFrameInfo->stVFrame.u32Height;
        pVideoFrame->virAddr[2] = pVideoFrame->virAddr[1];
    }
    else
    {
         SAL_LOGE("u64PhyAddr = 0x%llx GetVirAddr err!!!\n",pVideoFrameInfo->stVFrame.u64PhyAddr[0]);
    }

	pVideoFrame->stride[0] = pVideoFrameInfo->stVFrame.u32Stride[0];
	pVideoFrame->stride[1] = pVideoFrameInfo->stVFrame.u32Stride[1];
	pVideoFrame->stride[2] = pVideoFrameInfo->stVFrame.u32Stride[2];

	pVideoFrame->frameNum = (UINT64)pVideoFrameInfo->stVFrame.u32TimeRef;
	pVideoFrame->pts = (UINT64)pVideoFrameInfo->stVFrame.u64PTS;
	pVideoFrame->privateDate = (PhysAddr)pVideoFrameInfo->stVFrame.u64PrivateData;

    return SAL_SOK; 
}

/**
 * @function    sys_hisi_setVideoTimeInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_hisi_setVideoTimeInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32RefTime, UINT64 u64Pts)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    if (NULL == pstSystemFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;
    if (NULL == pstFrame)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame->stVFrame.u32TimeRef = u32RefTime;
    pstFrame->stVFrame.u64PTS = u64Pts;
    return SAL_SOK;
}

/**
 * @function    sys_plat_register
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
const SYS_PLAT_OPS *sys_plat_register(void)
{
    g_stSysPlatOps.init = sys_hisi_init;
    g_stSysPlatOps.deInit = sys_hisi_deInit;
    g_stSysPlatOps.bind = sys_hisi_bind;
    g_stSysPlatOps.getPts = sys_hisi_getPts;
    g_stSysPlatOps.getVideoFrameInfo = sys_hisi_getVideoFrameInfo;
    g_stSysPlatOps.buildVideoFrame = sys_hisi_buildVideoFrame;
    g_stSysPlatOps.allocVideoFrameInfoSt = sys_hisi_allocVideoFrameInfoSt;
    g_stSysPlatOps.releaseVideoFrameInfoSt = sys_hisi_releaseVideoFrameInfoSt;
    g_stSysPlatOps.setVideoTimeInfo = sys_hisi_setVideoTimeInfo;

    return &g_stSysPlatOps;
}


