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
#include "fmtConvert_ssV5.h"

static td_u32 gU32YSize   = 0;

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
INT32 System_calcPicVbBlkSize(UINT32 uiWidth, UINT32 uiHeight, ot_pixel_format enPixFmt, td_u32 u32AlignWidth, ot_data_bit_width enBitWidth)
{
    td_u32 u32VbSize           = 0;
    td_u32 u32BitWidth         = 0;
    td_u32 u32MainStride       = 0;
    td_u32 u32MainSize         = 0;
    td_u32 u32AlignHeight      = 0;
    td_u32 u32YSize            = 0;
    //DATA_BITWIDTH_E enBitWidth = DATA_BITWIDTH_10;

    if (0 == u32AlignWidth)
    {
       u32AlignWidth = OT_DEFAULT_ALIGN;
    }
    else if (u32AlignWidth > OT_MAX_ALIGN)
    {
       u32AlignWidth = OT_MAX_ALIGN;
    }
    else
    {
       u32AlignWidth = (OT_ALIGN_UP(u32AlignWidth, OT_DEFAULT_ALIGN));
    }

    switch (enBitWidth)
    {
       case OT_DATA_BIT_WIDTH_8:
       {
           u32BitWidth = 8;
           break;
       }
       
       case OT_DATA_BIT_WIDTH_10:
       {
           u32BitWidth = 10;
           break;
       }
       
       case OT_DATA_BIT_WIDTH_12:
       {
           u32BitWidth = 12;
           break;
       }
       
       case OT_DATA_BIT_WIDTH_14:
       {
           u32BitWidth = 14;
           break;
       }
       
       case OT_DATA_BIT_WIDTH_16:
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

    u32AlignHeight = OT_ALIGN_UP(uiHeight, 2);
    u32MainStride  = OT_ALIGN_UP((uiWidth * u32BitWidth + 7) >> 3, u32AlignWidth);
    u32YSize       = u32MainStride * u32AlignHeight;

    if (OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixFmt || OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixFmt)
    {
       u32MainSize = (u32MainStride * u32AlignHeight)*3 >> 1;
    }
    else if (OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixFmt || OT_PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixFmt)
    {
       u32MainSize = u32MainStride * u32AlignHeight * 2;
    }
    else if ((OT_PIXEL_FORMAT_YUV_400 == enPixFmt) || (OT_PIXEL_FORMAT_S16C1 == enPixFmt))
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
 函 数 名  : sys_ssV5_VIVPSSModeSet
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
	ot_vi_vpss_mode   stVIVPSSMode  = {0};
	INT32            s32Ret     = TD_SUCCESS;

	s32Ret = ss_mpi_sys_get_vi_vpss_mode(&stVIVPSSMode);
    if (TD_SUCCESS != s32Ret)
    {
       SYS_LOGE("Get VI-VPSS mode Param failed with %#x!\n", s32Ret);

       return SAL_FAIL;
    }
	// 实际使用几个vi此处就应该配置几个vi的mode sd3403同时只有pipi0支持在线模式
    stVIVPSSMode.mode[0] = OT_VI_OFFLINE_VPSS_OFFLINE;
    stVIVPSSMode.mode[2] = OT_VI_OFFLINE_VPSS_OFFLINE;
    s32Ret = ss_mpi_sys_set_vi_vpss_mode(&stVIVPSSMode);
    if (TD_SUCCESS != s32Ret)
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
INT32 sys_ssV5_deInit()
{
    ss_mpi_sys_exit();
    ss_mpi_vb_exit();

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
INT32 sys_ssV5_init(UINT32 u32ViChnNum)
{
    ot_vb_cfg        stVbConf;
    td_s32           ret        = 0;
    INT32            s32Ret     = 0;    
    ot_mpp_version    stVersion;
    ot_vb_supplement_cfg supplement_conf = {0};
    ot_vi_video_mode video_mode = OT_VI_VIDEO_MODE_NORM;

    memset(&stVbConf, 0, sizeof(ot_vb_cfg));
    memset(&stVersion, 0, sizeof(ot_mpp_version));
    ss_mpi_sys_get_version(&stVersion);

    SYS_LOGD("SDK Version : [ %s ]!!!\n", stVersion.version);

    /* 1. 去初始化系统与vb */
    s32Ret = ss_mpi_sys_exit();
	if(TD_SUCCESS != s32Ret)
	{
		SYS_LOGE("sys exit fail! s32Ret 0x%x\n", s32Ret);
	}
	
    s32Ret = ss_mpi_vb_exit();
	if(TD_SUCCESS != s32Ret)
	{
		SYS_LOGE("vb exit fail! s32Ret 0x%x\n", s32Ret);
	}

    /* 2. 初始化vb内存大小 */
    stVbConf.max_pool_cnt = 100;

    ot_vb_calc_cfg calc_cfg;
    ot_pic_buf_attr buf_attr;

    buf_attr.align         = OT_DEFAULT_ALIGN;
    buf_attr.bit_width     = OT_DATA_BIT_WIDTH_8;
    buf_attr.pixel_format  = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    buf_attr.compress_mode = OT_COMPRESS_MODE_SEG;

    buf_attr.width         = 2560;
    buf_attr.height        = 1440;  
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[0].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[0].blk_cnt  = 4 * u32ViChnNum;
	
    buf_attr.width         = 1280;
    buf_attr.height        = 1024;  
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[1].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[1].blk_cnt  = 8 * u32ViChnNum;
    
    buf_attr.width    = 1280;
    buf_attr.height   = 720;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[2].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[2].blk_cnt  = 8 * u32ViChnNum;

    buf_attr.width    = 1920;
    buf_attr.height   = 1200;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[3].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[3].blk_cnt  = 4 * u32ViChnNum;

#if 0
    buf_attr.width    = 704;
    buf_attr.height   = 576;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[4].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[4].blk_cnt  = 8 * u32ViChnNum;

	buf_attr.width    = 4864;
    buf_attr.height   = 640;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[5].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[5].blk_cnt  = 4 * u32ViChnNum;

    buf_attr.width    = 1920;
    buf_attr.height   = 1920;
    ot_common_get_pic_buf_cfg(&buf_attr, &calc_cfg);
    stVbConf.common_pool[6].blk_size = calc_cfg.vb_size;
    stVbConf.common_pool[6].blk_cnt  = 4 * u32ViChnNum;
#endif

    s32Ret = ss_mpi_vb_set_cfg(&stVbConf);
    if (TD_SUCCESS != s32Ret)
    {
        SYS_LOGE("ss_mpi_vb_set_cfg failed 0x%x !\n",s32Ret);
        return SAL_FAIL;
    }

    supplement_conf.supplement_cfg = OT_VB_SUPPLEMENT_BNR_MOT_MASK;
    ret = ss_mpi_vb_set_supplement_cfg(&supplement_conf);
    if (ret != TD_SUCCESS) {
        SYS_LOGE("ss_mpi_vb_set_supplement_conf failed!\n");
        return TD_FAILURE;
    }

    /* 3. 初始化vb内存 */
    if (TD_SUCCESS != ss_mpi_vb_init())
    {
        SYS_LOGE("ss_mpi_vb_init failed!(0x%x)\n",ret);
        return SAL_FAIL;
    }


    /* 5. 初始化sys */
    ret = ss_mpi_sys_init();
    if (TD_SUCCESS != ret)
    {
        SYS_LOGE("ss_mpi_sys_init failed!(0x%x)\n",ret);
        return SAL_FAIL;
    }

    s32Ret = sys_VIVPSSModeSet();
	if (TD_SUCCESS != s32Ret)
    {
        SYS_LOGE("sys_ssV5_VIVPSSModeSet ret %#x!\n", s32Ret);
        return SAL_FAIL;
    }

#if 0
    s32Ret = ss_mpi_vpss_get_mod_param(&stModParam);
    if (TD_SUCCESS != s32Ret)
    {
        SYS_LOGE("ss_mpi_vpss_get_mod_param ret %#x!\n", s32Ret);
        return SAL_FAIL;
    }

	stModParam.u32VpssVbSource = 2;
    s32Ret = ss_mpi_vpss_set_mod_param(&stModParam);
    if (TD_SUCCESS != s32Ret)
    {
        SYS_LOGE("ss_mpi_vpss_get_mod_param ret %#x!\n", s32Ret);
        return SAL_FAIL;
    }
#endif

    video_mode = OT_VI_VIDEO_MODE_NORM;
    ret = ss_mpi_sys_set_vi_video_mode(video_mode);
    if (ret != TD_SUCCESS) {
        SYS_LOGE("set vi video mode failed!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

INT32 sys_ssV5_getPts(UINT64 *pCurPts)
{
	INT32  s32Ret = TD_SUCCESS;
	UINT64 p64CurPts = 0;
	
	if (NULL == pCurPts)
    {
        SYS_LOGE("error\n");
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_sys_get_cur_pts(&p64CurPts);
    if (s32Ret != TD_SUCCESS)
    {
        VDEC_LOGE("ss_mpi_sys_get_cur_pts error 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    *pCurPts = p64CurPts;

    return SAL_SOK;
}

INT32 sys_ssV5_bind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 uiSrcDevId, UINT32 uiSrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 uiDstDevId, UINT32 uiDstChn,UINT32 uiBind)
{
    INT32  s32Ret = TD_SUCCESS;
    ot_mpp_chn stSrcChn;
    ot_mpp_chn stDestChn;

    memset(&stSrcChn, 0, sizeof(ot_mpp_chn));
    memset(&stDestChn, 0, sizeof(ot_mpp_chn));

    if (SYSTEM_MOD_ID_DUP == uiSrcModId)
    {
        stSrcChn.mod_id  = OT_ID_VPSS;
        stSrcChn.dev_id = uiSrcDevId;
        stSrcChn.chn_id = uiSrcChn;
    }
    else if (SYSTEM_MOD_ID_CAPT == uiSrcModId)
    {
        stSrcChn.mod_id  = OT_ID_VI;
        stSrcChn.dev_id = uiSrcDevId;
        stSrcChn.chn_id = 0;

    }
    else if (SYSTEM_MOD_ID_VDEC == uiSrcModId)
    {
        stSrcChn.mod_id  = OT_ID_VDEC;
        stSrcChn.dev_id = 0;
        stSrcChn.chn_id = uiSrcChn;

    }
    else
    {
        SYS_LOGE("uiSrcModId %d not support \n", uiSrcModId);
        return SAL_FAIL;
    }

    if (SYSTEM_MOD_ID_DUP == uiDstModId)
    {
        stDestChn.mod_id  = OT_ID_VPSS;
        stDestChn.dev_id = uiDstDevId;
        stDestChn.chn_id = uiDstChn;
    }
    else if (SYSTEM_MOD_ID_DISP == uiDstModId)
    {
        stDestChn.mod_id  = OT_ID_VO;
        stDestChn.dev_id = uiDstDevId;
        stDestChn.chn_id = uiDstChn;

    }
    else if (SYSTEM_MOD_ID_VENC == uiDstModId)
    {
        stDestChn.mod_id  = OT_ID_VENC;
        stDestChn.dev_id = 0;
        stDestChn.chn_id = uiDstChn;

    }
    else
    {
        SYS_LOGE("uiDstModId %d not support \n", uiDstModId);
        return SAL_FAIL;
    }

    if (1 == uiBind)
    {
        s32Ret = ss_mpi_sys_bind(&stSrcChn, &stDestChn);
        if (s32Ret != TD_SUCCESS)
        {
            SYS_LOGE("SRC M-%d_D-%d_C-%d Bind to  M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.mod_id, stSrcChn.dev_id, stSrcChn.chn_id, stDestChn.mod_id, stDestChn.dev_id, stDestChn.chn_id, s32Ret);
            return SAL_FAIL;
        }

        SYS_LOGD("SRC M-%d_D-%d_C-%d Bind to  M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.mod_id, stSrcChn.dev_id, stSrcChn.chn_id, stDestChn.mod_id, stDestChn.dev_id, stDestChn.chn_id, s32Ret);
    }
    else
    {
        s32Ret = ss_mpi_sys_unbind(&stSrcChn, &stDestChn);
        if (s32Ret != TD_SUCCESS)
        {
            SYS_LOGE("SRC M-%d_D-%d_C-%d unBind from Venc M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.mod_id, stSrcChn.dev_id, stSrcChn.chn_id, stDestChn.mod_id, stDestChn.dev_id, stDestChn.chn_id, s32Ret);
            return SAL_FAIL;
        }

        SYS_LOGD("SRC M-%d_D-%d_C-%d unBind from Venc M-%d_D-%d_C-%d failed with %#x!\n", 
                     stSrcChn.mod_id, stSrcChn.dev_id, stSrcChn.chn_id, stDestChn.mod_id, stDestChn.dev_id, stDestChn.chn_id, s32Ret);
    }
    return SAL_SOK;
}

INT32 sys_ssV5_allocVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    ot_video_frame_info *pOutFrm = NULL;

    pOutFrm = (ot_video_frame_info *)SAL_memMalloc(sizeof(ot_video_frame_info), "sys_ssV5", "video_frame");
    if (!pOutFrm)
    {
        SYS_LOGE("alloc mem[mod:sys_ssV5, name:video_frame] fail \n");
        return SAL_FAIL;
    }

	memset(pOutFrm,0x00,sizeof(ot_video_frame_info));
    pstSystemFrameInfo->uiAppData = (PhysAddr)pOutFrm;
    //pstSystemFrameInfo->uiDataAddr = (PhysAddr)pOutFrm->stVFrame.u64VirAddr[0];

    return SAL_SOK;
}

INT32 sys_ssV5_releaseVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    ot_video_frame_info *pOutFrm = (ot_video_frame_info *)pstSystemFrameInfo->uiAppData;

    SAL_memfree(pOutFrm, "sys_ssV5", "video_frame");
    pstSystemFrameInfo->uiAppData = 0;

    return SAL_SOK;    
}

static INT32 sys_ssV5_getPhyFromVir(VOID *pVir, PhysAddr *pPhyAddr)
{
    INT32 s32Ret = SAL_SOK;

	ot_sys_virt_mem_info stVirMemInfo = {0};

    if (NULL == pVir || NULL == pPhyAddr)
    {
    	SYS_LOGE("invalid input args! %p, %p \n", pVir, pPhyAddr);
		return SAL_FAIL;
    }

	s32Ret = ss_mpi_sys_get_virt_mem_info((VOID *)pVir, &stVirMemInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Vir Mem Info Failed! \n");
        goto exit;
    }

	*pPhyAddr = (PhysAddr)stVirMemInfo.phys_addr;
	
exit:
	return s32Ret;
}

INT32 sys_ssV5_buildVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    ot_video_frame_info *pVideoFrameInfo = (ot_video_frame_info *)pstSystemFrameInfo->uiAppData;

    if (!pVideoFrameInfo)
    {
        SYS_LOGE("pVideoFrameInfo     is NULL \n");
        return SAL_FAIL;
    }
    pVideoFrameInfo->pool_id            = pVideoFrame->poolId;
    pVideoFrameInfo->video_frame.width  = pVideoFrame->frameParam.width;
    pVideoFrameInfo->video_frame.height = pVideoFrame->frameParam.height;
    pVideoFrameInfo->video_frame.field   = OT_VIDEO_FIELD_FRAME;
    pVideoFrameInfo->video_frame.video_format = OT_VIDEO_FORMAT_LINEAR;
    pVideoFrameInfo->video_frame.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    /* 引擎只给了虚拟地址，所以这里需要从虚拟地址转为物理地址。DSP其他模块物理地址需要从上层传下来 */
    pVideoFrameInfo->video_frame.phys_addr[0] = (0x00 == pVideoFrame->phyAddr[0])  \
		                                     ? (SAL_SOK != sys_ssV5_getPhyFromVir((VOID *)pVideoFrame->virAddr[0], &pVideoFrame->phyAddr[0]) ? 0x00 : pVideoFrame->phyAddr[0]) \
		                                     : pVideoFrame->phyAddr[0];
    pVideoFrameInfo->video_frame.phys_addr[1] = (0x00 == pVideoFrame->phyAddr[1])  \
		                                     ? (SAL_SOK != sys_ssV5_getPhyFromVir((VOID *)pVideoFrame->virAddr[1], &pVideoFrame->phyAddr[1]) ? 0x00 : pVideoFrame->phyAddr[1]) \
		                                     : pVideoFrame->phyAddr[1];
    pVideoFrameInfo->video_frame.virt_addr[0] = OT_ADDR_U642P(pVideoFrame->virAddr[0]);
    pVideoFrameInfo->video_frame.virt_addr[1]  = OT_ADDR_U642P(pVideoFrame->virAddr[1]);
    pVideoFrameInfo->video_frame.stride[0]  = pVideoFrame->stride[0];
    pVideoFrameInfo->video_frame.stride[1]  = pVideoFrame->stride[1];
	
	pVideoFrameInfo->video_frame.time_ref = (UINT32)pVideoFrame->frameNum;
	pVideoFrameInfo->video_frame.pts = (UINT64)pVideoFrame->pts;
    //pVideoFrameInfo->video_frame.u64PrivateData = (PhysAddr)pVideoFrame->privateDate;  //vbBlk

	pstSystemFrameInfo->uiDataWidth = pVideoFrameInfo->video_frame.width;
	pstSystemFrameInfo->uiDataHeight = pVideoFrameInfo->video_frame.height;

    return SAL_SOK; 
}

static INT32 sys_ssV5_getVideoFrameInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame)
{
    ot_video_frame_info *pVideoFrameInfo = (ot_video_frame_info *)pstSystemFrameInfo->uiAppData;
    SAL_VideoDataFormat enSalPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    
    fmtConvert_ssV5_plat2sal(pVideoFrameInfo->video_frame.pixel_format, &enSalPixFmt);
    pVideoFrame->poolId = pVideoFrameInfo->pool_id;
    pVideoFrame->frameParam.width = pVideoFrameInfo->video_frame.width;
    pVideoFrame->frameParam.height = pVideoFrameInfo->video_frame.height;
    pVideoFrame->frameParam.dataFormat = enSalPixFmt;
	pVideoFrame->phyAddr[0] = (PhysAddr)pVideoFrameInfo->video_frame.phys_addr[0];
	pVideoFrame->phyAddr[1] = (PhysAddr)pVideoFrameInfo->video_frame.phys_addr[1];
	pVideoFrame->phyAddr[2] = pVideoFrame->phyAddr[1];

    if((PhysAddr)pVideoFrameInfo->video_frame.virt_addr[0] != 0x00)
    {
        pVideoFrame->virAddr[0] = (PhysAddr)pVideoFrameInfo->video_frame.virt_addr[0];
        pVideoFrame->virAddr[1] = (PhysAddr)pVideoFrameInfo->video_frame.virt_addr[1];
        pVideoFrame->virAddr[2] = pVideoFrame->virAddr[1];
    }
    else if(pstSystemFrameInfo->uiDataAddr != 0x00)
    {
        pVideoFrame->virAddr[0] = pstSystemFrameInfo->uiDataAddr;
        pVideoFrame->virAddr[1] = pVideoFrame->virAddr[0] + pVideoFrameInfo->video_frame.stride[0] * pVideoFrameInfo->video_frame.height;
        pVideoFrame->virAddr[2] = pVideoFrame->virAddr[1];
    }
    else
    {
         SAL_LOGE("u64PhyAddr = 0x%llx GetVirAddr err!!!\n",pVideoFrameInfo->video_frame.phys_addr[0]);
    }

	pVideoFrame->stride[0] = pVideoFrameInfo->video_frame.stride[0];
	pVideoFrame->stride[1] = pVideoFrameInfo->video_frame.stride[1];

	pVideoFrame->frameNum = (UINT64)pVideoFrameInfo->video_frame.time_ref;
	pVideoFrame->pts = (UINT64)pVideoFrameInfo->video_frame.pts;
	//pVideoFrame->privateDate = (PhysAddr)pVideoFrameInfo->video_frame.u64PrivateData;

    return SAL_SOK; 
}

/**
 * @function    sys_ssV5_setVideoTimeIfno
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_ssV5_setVideoTimeIfno(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32RefTime, UINT64 u64Pts)
{
    ot_video_frame_info *pstFrame = NULL;

    if (NULL == pstSystemFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame = (ot_video_frame_info *)pstSystemFrameInfo->uiAppData;
    if (NULL == pstFrame)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame->video_frame.time_ref = u32RefTime;
    pstFrame->video_frame.pts = u64Pts;
    return SAL_SOK;
}

const SYS_PLAT_OPS *sys_plat_register(void)
{
    g_stSysPlatOps.init = sys_ssV5_init;
    g_stSysPlatOps.deInit = sys_ssV5_deInit;
    g_stSysPlatOps.bind = sys_ssV5_bind;
	g_stSysPlatOps.getPts = sys_ssV5_getPts;
	g_stSysPlatOps.getVideoFrameInfo = sys_ssV5_getVideoFrameInfo;
    g_stSysPlatOps.buildVideoFrame = sys_ssV5_buildVideoFrame;
    g_stSysPlatOps.allocVideoFrameInfoSt = sys_ssV5_allocVideoFrameInfoSt;
    g_stSysPlatOps.releaseVideoFrameInfoSt = sys_ssV5_releaseVideoFrameInfoSt;
    g_stSysPlatOps.setVideoTimeInfo = sys_ssV5_setVideoTimeIfno;

    return &g_stSysPlatOps;
}


