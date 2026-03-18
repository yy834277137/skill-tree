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
    ot_vpss_chn_attr  stVpssChnAttr;
    td_s32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(ot_vpss_chn_attr));
    Ret = ss_mpi_vpss_get_chn_attr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_get_chn_attr (%d,%d)failed with %#x \n", u32GrpId, u32Chn,Ret);
        return SAL_FAIL;
    } 
	
    if (stVpssChnAttr.width == u32W && stVpssChnAttr.height == u32H)
	{
	     return SAL_SOK;
	}
	
    stVpssChnAttr.width  = u32W;
    stVpssChnAttr.height = u32H;
    Ret = ss_mpi_vpss_set_chn_attr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_set_chn_attr failed with %#x\n", Ret);
        return SAL_FAIL;
    }  

    return SAL_SOK;
}
static INT32 getVpssChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    ot_vpss_chn_attr  stVpssChnAttr;
    td_s32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(ot_vpss_chn_attr));
    Ret = ss_mpi_vpss_get_chn_attr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_get_chn_attr (%d,%d)failed with %#x\n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    } 

	*pU32W = stVpssChnAttr.width;
    *pU32H = stVpssChnAttr.height;

    return SAL_SOK;
}

static INT32 setVpssExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{
    ot_vpss_ext_chn_attr  stVpssExtChnAttr;
    td_s32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssExtChnAttr, 0, sizeof(stVpssExtChnAttr));
    Ret = ss_mpi_vpss_get_ext_chn_attr(u32GrpId, u32Chn, &stVpssExtChnAttr);
    if (Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_get_chn_attr (%d,%d)failed with %#x \n", u32GrpId, u32Chn,Ret);
        return SAL_FAIL;
    } 
	
    if (stVpssExtChnAttr.width == u32W && stVpssExtChnAttr.height == u32H)
	{
	     return SAL_SOK;
	}
	
    stVpssExtChnAttr.width  = u32W;
    stVpssExtChnAttr.height = u32H;
    Ret = ss_mpi_vpss_set_ext_chn_attr(u32GrpId, u32Chn, &stVpssExtChnAttr);
    if (Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_set_chn_attr failed with %#x\n", Ret);
        return SAL_FAIL;
    }  

    return SAL_SOK;
}
static INT32 getVpssExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    ot_vpss_ext_chn_attr  stVpssExtChnAttr;
    td_s32 Ret;
    
    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssExtChnAttr, 0, sizeof(stVpssExtChnAttr));
    Ret = ss_mpi_vpss_get_ext_chn_attr(u32GrpId, u32Chn, &stVpssExtChnAttr);
    if (Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_get_chn_attr (%d,%d)failed with %#x\n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    } 

	*pU32W = stVpssExtChnAttr.width;
    *pU32H = stVpssExtChnAttr.height;

    return SAL_SOK;
}

static INT32 setVpssChnMirror(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32Mirror,UINT32 u32Flip)
{
    INT32 s32Ret = 0;
    ot_vpss_chn_attr  pstChnAttr;

    /* 需要做此操作 */
    s32Ret = ss_mpi_vpss_get_chn_attr(u32GrpId, u32Chn, &pstChnAttr);
    if(TD_SUCCESS != s32Ret)
    {
        DUP_LOGE("ss_mpi_vpss_get_chn_attr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    pstChnAttr.mirror_en = u32Mirror;
    pstChnAttr.flip_en   = u32Flip;

    s32Ret = ss_mpi_vpss_set_chn_attr(u32GrpId, u32Chn, &pstChnAttr);
    if (TD_SUCCESS != s32Ret)
    {
        DUP_LOGE("ss_mpi_vpss_set_chn_attr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssChnFps(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32Fps)
{
    INT32 s32Ret = 0;
    ot_vpss_chn_attr pstChnAttr;
    
    /* 需要做此操作 */
    s32Ret = ss_mpi_vpss_get_chn_attr(u32GrpId, u32VpssChn, &pstChnAttr);
    if(TD_SUCCESS != s32Ret)
    {
        DUP_LOGE("ss_mpi_vpss_get_chn_attr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }
    
    DUP_LOGD("src_frame_rate = %d dst_frame_rate = %d \n",pstChnAttr.frame_rate.src_frame_rate, pstChnAttr.frame_rate.dst_frame_rate);

    pstChnAttr.frame_rate.dst_frame_rate = u32Fps;

    s32Ret = ss_mpi_vpss_set_chn_attr(u32GrpId, u32VpssChn, &pstChnAttr);
    if (TD_SUCCESS != s32Ret)
    {
        DUP_LOGE("ss_mpi_vpss_set_chn_attr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssChnRotate(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32Rotate)
{
    INT32 s32Ret      = 0;
    ot_rotation_attr  rotation_attr = {0};
    rotation_attr.enable = 1;
    rotation_attr.rotation_type = OT_ROTATION_ANG_FIXED;
    rotation_attr.rotation_fixed = u32Rotate;

    s32Ret = ss_mpi_vpss_set_chn_rotation(u32GrpId, u32VpssChn, &rotation_attr);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_set_chn_rotation failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssChnCrop(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32CropEnable, UINT32 u32X, UINT32 u32Y, UINT32 u32W, UINT32 u32H)
{
    INT32 s32Ret      = 0;

    ot_vpss_crop_info stCropParam = {0};
    stCropParam.enable              = u32CropEnable;
    stCropParam.crop_mode           = OT_COORD_ABS;
    stCropParam.crop_rect.x     = SAL_align(u32X,2);
    stCropParam.crop_rect.y     = SAL_align(u32Y,2);
    stCropParam.crop_rect.width = SAL_align(u32W,2);
    stCropParam.crop_rect.height= SAL_align(u32H,2);
    
    DUP_LOGD("grp %d,chn %d,x %d y %d w %d h %d[%d,%d]\n", u32GrpId, u32VpssChn,
		stCropParam.crop_rect.x, stCropParam.crop_rect.y,
		stCropParam.crop_rect.width, stCropParam.crop_rect.height,
		stCropParam.crop_rect.x + (stCropParam.crop_rect.width / 2),
		stCropParam.crop_rect.y + (stCropParam.crop_rect.height / 2));

    s32Ret = ss_mpi_vpss_set_chn_crop(u32GrpId, u32VpssChn, &stCropParam);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_set_chn_crop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 setVpssGrpCrop(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32CropEnable, UINT32 u32X, UINT32 u32Y, UINT32 u32W, UINT32 u32H)
{
    INT32 s32Ret      = 0;
    ot_vpss_crop_info stCropParam = {0};
	
    s32Ret = ss_mpi_vpss_get_grp_crop(u32GrpId, &stCropParam);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_get_grp_crop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }
	
	stCropParam.enable             = u32CropEnable;
    stCropParam.crop_mode           = OT_COORD_ABS;
    stCropParam.crop_rect.x     = u32X > 0 ? u32X : 0;
    stCropParam.crop_rect.y     = u32Y > 0 ? u32Y : 0;
    stCropParam.crop_rect.width = u32W > 64 ? u32W : 64;
    stCropParam.crop_rect.height= u32H > 64 ? u32H : 64;
    
    DUP_LOGD("grp %d,chn %d,x %d y %d w %d h %d[%d,%d]\n", u32GrpId, u32VpssChn,
		stCropParam.crop_rect.x, stCropParam.crop_rect.y,
		stCropParam.crop_rect.width, stCropParam.crop_rect.height,
		stCropParam.crop_rect.x + (stCropParam.crop_rect.width / 2),
		stCropParam.crop_rect.y + (stCropParam.crop_rect.height / 2));
	s32Ret = ss_mpi_vpss_set_grp_crop(u32GrpId, &stCropParam);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_set_grp_crop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 vpss_ssV5_getPhyChnNum()
{
    return OT_VPSS_MAX_PHYS_CHN_NUM;
}

static INT32 vpss_ssV5_getExtChnNum()
{
    return OT_VPSS_MAX_EXT_CHN_NUM;
}

static INT32 vpss_ssV5_getFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32 iRet = 0;
    ot_video_frame_info *pstVideoFrame = NULL;
    
    if (!pstFrame)
    {
        return SAL_FAIL;
    }
    pstVideoFrame = (ot_video_frame_info *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }
    
    iRet = ss_mpi_vpss_get_chn_frame(u32GrpId, u32Chn, pstVideoFrame, u32TimeoutMs);
    if (TD_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d Get vpss Chn %d Frame Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }

    pstFrame->uiDataAddr = (PhysAddr)ss_mpi_sys_mmap(pstVideoFrame->video_frame.phys_addr[0], \
                pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height * 3/2);;
    pstFrame->uiDataWidth = pstVideoFrame->video_frame.width;
    pstFrame->uiDataHeight = pstVideoFrame->video_frame.height;

	pstVideoFrame->video_frame.virt_addr[0] = (td_void *)pstFrame->uiDataAddr;
	pstVideoFrame->video_frame.virt_addr[1] = (td_void *)((CHAR *)pstVideoFrame->video_frame.virt_addr[0] + pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height);
		
    return SAL_SOK;

}

static INT32 vpss_ssV5_releaseFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 iRet = 0;
    ot_video_frame_info *pstVideoFrame = NULL;
    UINT32 u32Size = 0;

    if (!pstFrame)
    {
        return SAL_FAIL;
    }
    pstVideoFrame = (ot_video_frame_info *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    u32Size = pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height * 3/2;

    /*获取vpss数据的时候mmap相应的释放的时候虚拟地址需要释放*/
    iRet = ss_mpi_sys_munmap((td_void *)pstFrame->uiDataAddr, u32Size);
    if (TD_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d  vpss Chn %d ss_mpi_sys_munmap Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }

    iRet = ss_mpi_vpss_release_chn_frame(u32GrpId, u32Chn, pstVideoFrame);
    if (TD_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d release vpss Chn %d Frame Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }

    return SAL_SOK;

}

static INT32 vpss_ssV5_sendFrame(UINT32 dupChan, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32  iRet = 0;
	UINT32 VpssGroupID = 0;
	ot_video_frame_info *pstVideoFrame   = NULL;

	if (NULL == pstFrame)
	{
	     DUP_LOGE("Chn %d is null!!!\n", dupChan);
         return SAL_FAIL;
	}
	pstVideoFrame = (ot_video_frame_info *)pstFrame->uiAppData;
    if(NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    VpssGroupID = dupChan;
    iRet = ss_mpi_vpss_send_frame(VpssGroupID, pstVideoFrame, u32TimeoutMs);
    if (TD_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d Send vpss Frame Failed, %x !!!\n", VpssGroupID, iRet);
        return SAL_FAIL;
    }
	
    return SAL_SOK;
}

static INT32 vpss_ssV5_enableChn(UINT32 u32GrpId, UINT32 u32Chn, CHN_SWITH_E enSwith)
{
    td_s32 s32Ret      = 0;

    if (ENABLE == enSwith)
    {
        s32Ret = ss_mpi_vpss_enable_chn(u32GrpId, u32Chn);
        if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("OT_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }
    else if (DISABLE == enSwith)
    {
        s32Ret = ss_mpi_vpss_disable_chn(u32GrpId, u32Chn);
        if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("OT_MPI_VPSS_DisableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }
    else
    {
        DUP_LOGE("error: u32GrpId %d, u32Chn %d, enSwith %d\n", u32GrpId, u32Chn, enSwith);
        return SAL_FAIL;       
    }

    return SAL_SOK;
}

static INT32 vpss_ssV5_setParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
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
            s32Ret = setVpssChnCrop(u32GrpId, u32Chn, pParamInfo->stCrop.u32CropEnable, \
                pParamInfo->stCrop.u32X, pParamInfo->stCrop.u32Y, pParamInfo->stCrop.u32W, pParamInfo->stCrop.u32H);
                break;
        case GRP_CROP_CFG:
            s32Ret = setVpssGrpCrop(u32GrpId, u32Chn, pParamInfo->stCrop.u32CropEnable, \
                pParamInfo->stCrop.u32X, pParamInfo->stCrop.u32Y, pParamInfo->stCrop.u32W, pParamInfo->stCrop.u32H);            
                break;
        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}

static INT32 vpss_ssV5_getParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
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
static INT32 vpss_ssV5_init(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm, DUP_HAL_CHN_INIT_ATTR *pstChnInitAttr)
{
    ot_vpss_grp_attr stVpssGrpAttr = {0};
    ot_vpss_chn_attr stVpssChnAttr = {0};
    DUP_HAL_CHN_ADDR      addrPrm = {0};
    td_s32 s32Ret      = 0;
    UINT32 u32VpssGroup = 0;
    INT32  i           = 0;
	UINT32 u32Depth     = 0;
    VB_INFO_S stVbInfo = {0};
    char *pcMmzName = NULL;

	ot_vpss_ext_chn_attr stVpssExtChnAttr = {0};
	DUP_HAL_OUT_CHN_ATTR *pstOutChnAttr = NULL;

    CAPB_LINE *capb_line = capb_get_line();
    CAPB_OSD *capb_osd = capb_get_osd();

    u32VpssGroup = pstHalVpssGrpPrm->uiVpssChn;
    u32Depth = pstHalVpssGrpPrm->uiChnDepth;

    /*1080x1920编码出错，图像宽高先按16自己对齐*/
    stVbInfo.u32BlkWidth = SAL_align(pstHalVpssGrpPrm->uiGrpWidth, 16);
    stVbInfo.u32BlkHeight = SAL_align(pstHalVpssGrpPrm->uiGrpHeight, 16);
    stVbInfo.u32BlkCnt = pstHalVpssGrpPrm->u32BlkCnt;

    if ((capb_line->enDrawMod == DRAW_MOD_DSP) || (capb_osd->enDrawMod == DRAW_MOD_DSP))
    {
        pcMmzName = "dsp";
    }
    else
    {
        pcMmzName = NULL;
    }

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
     SAL_clear(&stVpssGrpAttr);
    stVpssGrpAttr.max_width = pstHalVpssGrpPrm->uiGrpWidth;
    stVpssGrpAttr.max_height = pstHalVpssGrpPrm->uiGrpHeight;
    stVpssGrpAttr.pixel_format  = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    stVpssGrpAttr.dynamic_range = OT_DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.frame_rate.dst_frame_rate = -1;
    stVpssGrpAttr.frame_rate.src_frame_rate = -1;

#if 0
    /*关闭nr_en会导致经过vpss的数据变暗亮度不够,是否开启需要图像调试*/
    stVpssGrpAttr.nr_en   = OT_TRUE;
    stVpssGrpAttr.nr_attr.nr_type                = OT_VPSS_NR_TYPE_VIDEO_NORM;
    stVpssGrpAttr.nr_attr.compress_mode          = OT_COMPRESS_MODE_NONE;
    stVpssGrpAttr.nr_attr.nr_motion_mode          = OT_VPSS_NR_MOTION_MODE_NORM;
#endif

    /*关闭VPSS部分功能*/
    stVpssGrpAttr.dei_mode                  = OT_VPSS_DEI_MODE_OFF;
    stVpssGrpAttr.ie_en                     = TD_FALSE;
    stVpssGrpAttr.dci_en                    = TD_FALSE;
    stVpssGrpAttr.buf_share_en              = TD_FALSE;
    stVpssGrpAttr.mcf_en                    = TD_FALSE;
    stVpssGrpAttr.nr_en                     = TD_FALSE;

    s32Ret = ss_mpi_vpss_create_grp(u32VpssGroup, &stVpssGrpAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_create_grp failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vpss_start_grp(u32VpssGroup);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_start_grp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    /* 创建 vpss chn 4个物理通道 */
    for(i = 0; i < OT_VPSS_MAX_PHYS_CHN_NUM; i++)
    {
        if (ENABLED != pstChnInitAttr->uiOutChnEnable[i])
        {
            continue;
        }
		
        SAL_clear(&stVpssChnAttr);
        stVpssChnAttr.width                     = pstChnInitAttr->stOutChnAttr[i].uiOutChnW;
        stVpssChnAttr.height                    = pstChnInitAttr->stOutChnAttr[i].uiOutChnH;
        stVpssChnAttr.chn_mode                  = OT_VPSS_CHN_MODE_USER;
        stVpssChnAttr.compress_mode             = OT_COMPRESS_MODE_NONE; //COMPRESS_MODE_SEG;
        stVpssChnAttr.dynamic_range             = stVpssGrpAttr.dynamic_range;
        stVpssChnAttr.pixel_format              = stVpssGrpAttr.pixel_format;
        stVpssChnAttr.frame_rate.src_frame_rate = -1;
        stVpssChnAttr.frame_rate.dst_frame_rate = -1;
        stVpssChnAttr.depth                       = u32Depth;
        stVpssChnAttr.mirror_en                   = TD_FALSE;
        stVpssChnAttr.flip_en                     = TD_FALSE;
        stVpssChnAttr.border_en                   = TD_FALSE;
        stVpssChnAttr.video_format                = OT_VIDEO_FORMAT_LINEAR;
        stVpssChnAttr.aspect_ratio.mode           = OT_ASPECT_RATIO_NONE;
        s32Ret = ss_mpi_vpss_set_chn_attr(u32VpssGroup, i, &stVpssChnAttr);
        if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("ss_mpi_vpss_set_chn_attr failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }
		
        s32Ret = ss_mpi_vpss_attach_vb_pool(u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID );
        if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("ss_mpi_vpss_attach_vb_pool failed with %#x (Grp:%d,chn:%d),u32PoolId %d\n", s32Ret,u32VpssGroup, i, addrPrm.poolId);
            return SAL_FAIL;
        }
        else
        {
            DUP_LOGD("ss_mpi_vpss_attach_vb_pool succeed!!! (%d,%d),u32PoolId %d\n",u32VpssGroup, i, addrPrm.poolId);
        }
        
        s32Ret = ss_mpi_vpss_enable_chn(u32VpssGroup, i);
        if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("ss_mpi_vpss_enable_chn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }   
    }

    /* 创建拓展通道 */
    for(i = 0; i < pstHalVpssGrpPrm->uiExtChnNum; i++)
    {
        if (ENABLED != pstChnInitAttr->uiOutChnEnable[i + OT_VPSS_MAX_PHYS_CHN_NUM])
        {
            continue;
        }
		
        memset(&stVpssExtChnAttr, 0, sizeof(ot_vpss_ext_chn_attr));
		
        stVpssExtChnAttr.compress_mode   = OT_COMPRESS_MODE_NONE;
		stVpssExtChnAttr.video_format    = OT_VIDEO_FORMAT_LINEAR;
        stVpssExtChnAttr.pixel_format    = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        stVpssExtChnAttr.bind_chn       = i;    /* */
        stVpssExtChnAttr.depth         = 1;//pstChnInitAttr->uiDepth;    /* */
        stVpssExtChnAttr.frame_rate.dst_frame_rate  = -1;
        stVpssExtChnAttr.frame_rate.src_frame_rate  = -1;
       
        pstOutChnAttr = &pstChnInitAttr->stOutChnAttr[i + OT_VPSS_MAX_PHYS_CHN_NUM];

        stVpssExtChnAttr.width        = pstOutChnAttr->uiOutChnH;
        stVpssExtChnAttr.height         = pstOutChnAttr->uiOutChnW;

        s32Ret = ss_mpi_vpss_set_ext_chn_attr(u32VpssGroup, i + OT_VPSS_MAX_PHYS_CHN_NUM, &stVpssExtChnAttr);
        if (s32Ret != SAL_SOK)
        {
            DUP_LOGE("HI_MPI_VPSS_SetExtChnAttr failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }

        s32Ret = ss_mpi_vpss_enable_chn(u32VpssGroup, i + OT_VPSS_MAX_PHYS_CHN_NUM);
        if (s32Ret != SAL_SOK)
        {
            DUP_LOGE("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }		
    }
	
/* 扩展通道与物理通道绑定 */
#if defined(SVA_USE_EXT_CHN)

    if ((u32VpssGroup == 0) || (u32VpssGroup == 2))
    {
          ot_vpss_ext_chn_attr stExtVpssChnAttr = {0};
          stExtVpssChnAttr.bind_chn = 2;
          stExtVpssChnAttr.src_type = OT_EXT_CHN_SRC_TYPE_TAIL;
          stExtVpssChnAttr.width = 1920;
          stExtVpssChnAttr.height = 1080;
          stExtVpssChnAttr.depth = 1;
          stExtVpssChnAttr.compress_mode             = OT_COMPRESS_MODE_NONE; //COMPRESS_MODE_SEG;
          stExtVpssChnAttr.dynamic_range             = stVpssGrpAttr.dynamic_range;
          stExtVpssChnAttr.pixel_format              = stVpssGrpAttr.pixel_format;
          stExtVpssChnAttr.frame_rate.src_frame_rate = -1;
          stExtVpssChnAttr.frame_rate.dst_frame_rate = -1;
          UINT32 u32ExtChn = u32VpssGroup + 4;
          ss_mpi_vpss_set_ext_chn_attr(u32VpssGroup, u32ExtChn, &stExtVpssChnAttr);

          s32Ret = ss_mpi_vpss_attach_vb_pool(u32VpssGroup, u32ExtChn, pstHalVpssGrpPrm->u32PoolID);
          if (s32Ret != TD_SUCCESS)
          {
              DUP_LOGE("ss_mpi_vpss_attach_vb_pool failed with %#x (Grp:%d,chn:%d),u32PoolId %d\n", s32Ret,u32VpssGroup, i, addrPrm.poolId);
              return SAL_FAIL;
          }
          else
          {
              DUP_LOGD("ss_mpi_vpss_attach_vb_pool succeed!!! (%d,%d),u32PoolId %d\n",u32VpssGroup, i, addrPrm.poolId);
          }

          s32Ret = ss_mpi_vpss_enable_chn(u32VpssGroup, u32ExtChn);
          if (s32Ret != TD_SUCCESS)
          {
              DUP_LOGE("ss_mpi_vpss_enable_chn failed with %#x\n", s32Ret);
              return SAL_FAIL;
          }   
          DUP_LOGW("vpss extChn, group %d, ext chn %d \n", u32VpssGroup, u32ExtChn);
    }
#endif
    DUP_LOGD("Create Vpss Group Chn %d SUC!!!poolId %d\n", u32VpssGroup, pstHalVpssGrpPrm->u32PoolID);

    pstHalVpssGrpPrm->uiVpssChn = u32VpssGroup;
    return SAL_SOK;
}

static INT32 vpss_ssV5_deinit(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm)
{
    td_s32 s32Ret      = 0;
    UINT32 i = 0;
    VB_INFO_S stVbInfo = {0};

    for(i = 0; i < OT_VPSS_MAX_PHYS_CHN_NUM; i++)
    {
        if (ENABLED != pstHalVpssGrpPrm->uiChnEnable[i])
        {
            continue;
        }
		
		s32Ret = ss_mpi_vpss_disable_chn(pstHalVpssGrpPrm->uiVpssChn, i);
        if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("ss_mpi_vpss_enable_chn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }
        
		s32Ret = ss_mpi_vpss_detach_vb_pool(pstHalVpssGrpPrm->uiVpssChn, i);
		if (s32Ret != TD_SUCCESS)
        {
            DUP_LOGE("ss_mpi_vpss_detach_vb_pool failed with %#x (Grp:%d,chn:%d)\n", s32Ret, pstHalVpssGrpPrm->uiVpssChn, i);
            return SAL_FAIL;
        }
		else
		{
		    DUP_LOGD("ss_mpi_vpss_detach_vb_pool succeed!!! (%d,%d)\n", pstHalVpssGrpPrm->uiVpssChn, i);
		}
    }

    s32Ret = ss_mpi_vpss_stop_grp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_stop_grp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vpss_reset_grp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_reset_grp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vpss_destroy_grp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != TD_SUCCESS)
    {
        DUP_LOGE("ss_mpi_vpss_destroy_grp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    stVbInfo.u32VbPoolID = pstHalVpssGrpPrm->u32PoolID;
    stVbInfo.u32PoolSize = pstHalVpssGrpPrm->u32PoolSize;
    mem_hal_vbPoolFree(&stVbInfo, "vpss_rk", "vpss_vb");

    return SAL_SOK;
}

const DUP_PLAT_OPS *dup_hal_register()
{
    g_stDupPlatOps.init = vpss_ssV5_init;
    g_stDupPlatOps.deinit = vpss_ssV5_deinit;
    g_stDupPlatOps.getPhyChnNum = vpss_ssV5_getPhyChnNum;
    g_stDupPlatOps.getExtChnNum = vpss_ssV5_getExtChnNum;
    g_stDupPlatOps.getFrame = vpss_ssV5_getFrame;
    g_stDupPlatOps.releaseFrame = vpss_ssV5_releaseFrame;
    g_stDupPlatOps.sendFrame = vpss_ssV5_sendFrame;
    g_stDupPlatOps.setParam= vpss_ssV5_setParam;
    g_stDupPlatOps.getParam= vpss_ssV5_getParam;

    return &g_stDupPlatOps;
}




