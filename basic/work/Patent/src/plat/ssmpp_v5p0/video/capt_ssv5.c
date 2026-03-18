/**
 * @file   capt_ssv5.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台采集模块
 * @author liuxianying
 * @date   2021/8/6
 * @note
 * @note \n History
   1.日    期: 2021/8/6
     作    者: liuxianying
     修改历史: 创建文件
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "capbility.h"
#include "capt_ssv5.h"
#include "../../hal/hal_inc_inter/capt_hal_inter.h"

/*****************************************************************************
                               宏定义
*****************************************************************************/

#define VGA_OFFSET_CHECK_PIXEL      (4)     // 计算VGA图像偏移的像素点个数

/*****************************************************************************
                               结构体定义
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/
CAPT_MOD_PRM g_stCaptModPrm = {0};
static ot_video_frame_info gstUserPicFrame[CAPT_CHN_NUM_MAX] = {0};

extern int errno;
static CAPT_PLAT_OPS g_stCaptPlatOps;

/*****************************************************************************
                                函数
*****************************************************************************/

#define NEW_LINUX_HISI_CONFIG   /*一些写死的配置需要整改*/
#ifdef NEW_LINUX_HISI_CONFIG

/**
 * @function   capt_ssv5GetIntfMode
 * @brief      根据板子类型获取输入接口类型
 * @param[in]  VOID  
 * @param[out] None
 * @return     static ot_vi_intf_mode 返回接口类型
 */
static ot_vi_intf_mode capt_ssv5GetIntfMode(VOID)
{
    static ot_vi_intf_mode enMode = OT_VI_INTF_MODE_BUTT;
    CAPB_CAPT *pstCaptCap = NULL;

    if (OT_VI_INTF_MODE_BUTT == enMode)
    {
        pstCaptCap = capb_get_capt();
        if (NULL == pstCaptCap)
        {
            CAPT_LOGE("get capt capbility fail\n");
            return OT_VI_INTF_MODE_BUTT;
        }

        /* FPGA用来做BT1120转LVDS VI_MODE_LVDS,  sd3403没有lvds选项*/
        enMode = (SAL_TRUE == pstCaptCap->bFpgaEnable) ? OT_VI_INTF_MODE_MIPI : OT_VI_INTF_MODE_BT1120;
    }
    
    return enMode;
}

/**
 * @function   capt_ssv5IsUseMipi
 * @brief      根据板子类型获取是否使用Mipi
 * @param[in]  VOID  
 * @param[out] None
 * @return     static BOOL
 */
static BOOL capt_ssv5IsUseMipi(VOID)
{
    ot_vi_intf_mode enMode = capt_ssv5GetIntfMode();
    static ot_vi_intf_mode aenUseMipiMode[] = {OT_VI_INTF_MODE_MIPI};  /*VI_MODE_LVDS,  sd3403没有lvds选项*/
    static INT32 s32UseMipi = -1;
    UINT32 u32Num = sizeof(aenUseMipiMode)/sizeof(aenUseMipiMode[0]);
    UINT32 i = 0;

    if (s32UseMipi < 0)
    {
        for (i = 0; i < u32Num; i++)
        {
            if (enMode == aenUseMipiMode[i])
            {
                s32UseMipi = 1;
                break;
            }
        }

        if (i >= u32Num)
        {
            s32UseMipi = 0;
        }
    }

    return (1 == s32UseMipi) ? SAL_TRUE : SAL_FALSE;
}

/**
 * @function   capt_ssv5GetDevId
 * @brief      根据采集通道获取DEV
 * @param[in]  UINT32 u32ChnId    采集通道号
 * @param[in]  ot_vi_dev *pDevId  DEV ID
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5GetDevId(UINT32 u32ChnId, ot_vi_dev *pDevId)
{
    static UINT32 au32CaptDevMap[CAPT_CHN_NUM_MAX] = {0, 2};
    static UINT32 au32BtCaptDevMap[CAPT_CHN_NUM_MAX] = {5, 7};       // BT1120,BT656,BT601与DEV绑定关系请参考HISI数据手册
    ot_vi_intf_mode enIntfMode = capt_ssv5GetIntfMode();

    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }
    
    if (OT_VI_INTF_MODE_BT1120 == enIntfMode)
    {
        *pDevId = au32BtCaptDevMap[u32ChnId];
    }
    else
    {
        *pDevId = au32CaptDevMap[u32ChnId];
    }

    return SAL_SOK;
}

/**
 * @function   capt_ssv5GetPipeId
 * @brief      根据采集通道获取Pipe Id
 * @param[in]  UINT32 u32ChnId      采集通道号
 * @param[in]  ot_vi_pipe *pPipeId  PIPE ID
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5GetPipeId(UINT32 u32ChnId, ot_vi_pipe *pPipeId)
{
    static UINT32 au32PipeMap[CAPT_CHN_NUM_MAX] = {0, 2};  /*videv通道和pipi编号的映射关系*/

    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }

    *pPipeId = au32PipeMap[u32ChnId];

    return SAL_SOK;
}
#endif

/**
 * @function   capt_ssv5GetPoolBlock
 * @brief      获取内存池中的内存块
 * @param[in]  UINT32 imgW                   内存块尺寸
 * @param[in]  UINT32 imgH                   内存块尺寸
 * @param[in]  ot_video_frame_info *pOutFrm  帧信息
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5GetPoolBlock(UINT32 imgW,UINT32 imgH, ot_video_frame_info *pOutFrm)
{  
    ot_vb_pool pool        = OT_VB_INVALID_POOL_ID;
    UINT64  u64BlkSize  = (UINT64)((UINT64)imgW * (UINT64)imgH * 3 / 2);
    ot_vb_blk  VbBlk       = 0;
    UINT64  u64PhyAddr  = 0;
    UINT64  *p64VirAddr    = NULL;
    UINT32  u32LumaSize = 0;
    UINT32  u32ChrmSize = 0;
    UINT64  u64Size     = 0;
    ot_vb_pool_cfg stVbPoolCfg;

    if(OT_VB_INVALID_POOL_ID == pool)
    {
        SAL_clear(&stVbPoolCfg);
        stVbPoolCfg.blk_size  = u64BlkSize;
        stVbPoolCfg.blk_cnt   = 1;
        stVbPoolCfg.remap_mode = OT_VB_REMAP_MODE_NONE;
        pool = ss_mpi_vb_create_pool(&stVbPoolCfg);

        if(OT_VB_INVALID_POOL_ID == pool)
        {
            CAPT_LOGE("ss_mpi_vb_create_pool failed size %llu!\n", u64BlkSize);
            return SAL_FAIL;
        }
    }

    u32LumaSize = (imgW * imgH);
    u32ChrmSize = (imgW * imgH / 4);
    u64Size     = (UINT64)(u32LumaSize + (u32ChrmSize << 1));

    VbBlk = ss_mpi_vb_get_blk(pool, u64Size, NULL);
    if(OT_VB_INVALID_POOL_ID == pool)
    {
        CAPT_LOGE("ss_mpi_vb_create_pool failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    u64PhyAddr = ss_mpi_vb_handle_to_phys_addr(VbBlk);
    if(0 == u64PhyAddr)
    {
        CAPT_LOGE("OT_MPI_VB_Handle2PhysAddr failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    p64VirAddr = (UINT64 *) ss_mpi_sys_mmap(u64PhyAddr, u64Size);
    if(NULL == p64VirAddr)
    {
        CAPT_LOGE("OT_MPI_SYS_Mmap failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    pOutFrm->mod_id            = OT_ID_VB;
    pOutFrm->pool_id          = pool;
    pOutFrm->video_frame.width  = imgW;
    pOutFrm->video_frame.height = imgH;
    pOutFrm->video_frame.field   = OT_VIDEO_FIELD_FRAME;

    pOutFrm->video_frame.pixel_format   = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;   /*应该和vichn配置保持一致*/
    pOutFrm->video_frame.phys_addr[0]   = (td_phys_addr_t)u64PhyAddr;
    pOutFrm->video_frame.phys_addr[1]   = (td_phys_addr_t)pOutFrm->video_frame.phys_addr[0] + u32LumaSize;
    pOutFrm->video_frame.virt_addr[0]   = (td_void *)p64VirAddr;
    pOutFrm->video_frame.virt_addr[1]   = (td_void *)((UINT64)pOutFrm->video_frame.virt_addr[0] + u32LumaSize);
    pOutFrm->video_frame.stride[0]    = pOutFrm->video_frame.width;
    pOutFrm->video_frame.stride[1]    = pOutFrm->video_frame.width;
    pOutFrm->video_frame.user_data[0]  = VbBlk;

    return SAL_SOK;
}

/**
 * @function   capt_ssv5CaptIsOffset
 * @brief      计算图像是否有偏移
 * @param[in]  UINT32 uiDev  输入通道
 * @param[out] None
 * @return     static BOOL  SAL_TRUE
 *                          SAL_FALSE
 */
static BOOL capt_ssv5CaptIsOffset(UINT32 uiDev)
{
    INT32 s32Ret = TD_SUCCESS;
    ot_video_frame_info stFrame;
    ot_vi_pipe viPipe    = 0;
    UINT64 u64PhyAddr = 0;
    void *pvAddr   = NULL;
    UINT8 *pu8Tmp     = NULL;
    UINT32 u32Stride  = 0;
    BOOL bLeft = SAL_TRUE, bRight = SAL_TRUE;
    BOOL bUp = SAL_TRUE, bDown = SAL_TRUE;
    UINT32 i = 0, j = 0;

    if (SAL_SOK != capt_ssv5GetPipeId(uiDev, &viPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FALSE;
    }

    s32Ret = ss_mpi_vi_get_chn_frame(viPipe, 0, &stFrame, 200);
    if (SAL_SOK != s32Ret)
    {
        CAPT_LOGE("capt chn[%u] get frame fail s32Ret = 0x%x \n", uiDev, s32Ret);
        return SAL_FALSE;
    }

    u64PhyAddr = stFrame.video_frame.phys_addr[0];
    u32Stride  = stFrame.video_frame.stride[0];

    pvAddr = ss_mpi_sys_mmap(u64PhyAddr, u32Stride * stFrame.video_frame.height * 3 / 2);
    if (NULL == pvAddr)
    {
        CAPT_LOGE("capt chn[%u] map addr[0x%llx] fail\n", uiDev, u64PhyAddr);
        if (TD_SUCCESS != (s32Ret = ss_mpi_vi_release_chn_frame(viPipe, 0, &stFrame)))
        {
            CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
        }
        return SAL_FALSE;
    }

    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.video_frame.height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        /* 最左边的一列不校验 */
        for (j = 1; j < VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 左边没有黑边 */
                bLeft = SAL_FALSE;
                goto right;
            }
        }
    }

right:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.video_frame.width - VGA_OFFSET_CHECK_PIXEL);
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.video_frame.height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        /* 最右边的一列不校验 */
        for (j = 0; j < VGA_OFFSET_CHECK_PIXEL - 1; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 右边没有黑边 */
                bRight = SAL_FALSE;
                goto up;
            }
        }
    }

up:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += u32Stride;
    /* 最上面的一行不校验 */
    for (i = 1; i < VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.video_frame.width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 上面没有黑边 */
                bUp = SAL_FALSE;
                goto down;
            }
        }
    }

down:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.video_frame.height - VGA_OFFSET_CHECK_PIXEL) * u32Stride;
    /* 最下面的一行不校验 */
    for (i = 0; i < VGA_OFFSET_CHECK_PIXEL - 1; i++)
    {
        pu8Tmp += u32Stride;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.video_frame.width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 下面没有黑边 */
                bDown = SAL_FALSE;
                goto end;
            }
        }
    }

end:
    if (TD_SUCCESS != (s32Ret = ss_mpi_sys_munmap(pvAddr, u32Stride * stFrame.video_frame.height * 3 / 2)))
    {
        CAPT_LOGW("ummap viraddr[%p] size[%u] fail:0x%X\n", pvAddr, u32Stride * stFrame.video_frame.height * 3 / 2, s32Ret);
    }
    
    if (TD_SUCCESS != (s32Ret = ss_mpi_vi_release_chn_frame(viPipe, 0, &stFrame)))
    {
        CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
    }

    /* 上下都有黑边或左右都有黑边时无需调整 */
    return ((bLeft ^ bRight) || (bUp ^ bDown));
}

/**
 * @function   capt_ssv5GetUserPicStatue
 * @brief      采集模块使用用户图片，用于无信号时编码
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     static UINT32 SAL_SOK
 *                           SAL_FAIL
 */
static UINT32 capt_ssv5GetUserPicStatue(UINT32 uiDev)
{ 
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("uiChn %d error\n", uiDev);
        return SAL_FAIL;
    }
    
    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (pstCaptHalChnPrm->userPic)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @function   capt_ssv5SetCaptUserPic
 * @brief      采集模块配置用户图片，用于无信号时编码
 * @param[in]  UINT32 uiDev                   设备号
 * @param[in]  CAPT_CHN_USER_PIC *pstUserPic  用户数据
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5SetCaptUserPic(UINT32 uiDev, CAPT_CHN_USER_PIC *pstUserPic)
{

    ot_vi_pipe_attr      stPipeAttr;
    //ot_vi_chn_attr       stChnAttr;
    ot_video_frame_info *pstVideoFrameInfo = NULL;
    ot_video_frame      *pVFrame           = NULL;
    INT32   iRet   = 0;
    ot_vi_pipe ViPipe = 0;
    UINT32  uiW    = 0;
    UINT32  uiH    = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }

    ///memset(pstUserPicAttr, 0, sizeof(VI_USERPIC_ATTR_S));
    pstVideoFrameInfo = &gstUserPicFrame[uiDev];

    pVFrame = &pstVideoFrameInfo->video_frame;

    if ((NULL == pstUserPic->pucAddr) || (0 == pstUserPic->uiW) || (0 == pstUserPic->uiH))
    {
        uiW = 1920;
        uiH = 1080;
        /* 上层无图片，底层自己填纯色 */
        CAPT_LOGI("uiChn %d,use bgc\n",uiDev);
         return SAL_FAIL;
    }
    else
    {
        uiW = pstUserPic->uiW;
        uiH = pstUserPic->uiH;
        CAPT_LOGI("uiChn %d,use pic(wxh %d,%d)\n",uiDev,uiW,uiH);
    }

    /* 图片宽度要16对齐，图片缩放要用到VGS，需要16对齐 否则配置成功，但无图片显示 */
    if (0x00 == pstVideoFrameInfo->video_frame.virt_addr[0])
    {
        uiW = (uiW/16)*16;
        if (SAL_SOK != (iRet = capt_ssv5GetPoolBlock(uiW, uiH, pstVideoFrameInfo)))
        {
            CAPT_LOGE("Capt Hal Set User Pic Get Pool Block Failed, %d !!!\n", iRet);

            return SAL_FAIL;
        }
        CAPT_LOGI("uiChn %u,get Pool info id %u,modeId %u, addr %p\n",uiDev,pstVideoFrameInfo->pool_id,(UINT32)pstVideoFrameInfo->mod_id,pstVideoFrameInfo->video_frame.virt_addr[0]);
    }
    else
    {
        CAPT_LOGI("uiChn %du,pic Pool info id %u,modeId %u, addr %p\n",uiDev,pstVideoFrameInfo->pool_id,(UINT32)pstVideoFrameInfo->mod_id,pstVideoFrameInfo->video_frame.virt_addr[0]);
    }

    /* 纯色 */

    memcpy(OT_ADDR_U642P(pVFrame->virt_addr[0]), pstUserPic->pucAddr, uiW * uiH);
    memcpy(OT_ADDR_U642P(pVFrame->virt_addr[1]), pstUserPic->pucAddr + uiW * uiH, uiW * uiH / 2);


    iRet = ss_mpi_vi_set_user_pic(ViPipe, pstVideoFrameInfo);//lxy 参数和文档及v30不一致；
    if (iRet != TD_SUCCESS)
    {
        CAPT_LOGE("Capt Hal Set User Pic Failed, Ret %#x !!!\n", iRet);
        return SAL_FAIL;
    }

#if 1

    SAL_clear(&stPipeAttr);
    iRet = ss_mpi_vi_get_pipe_attr(ViPipe,&stPipeAttr);
    if (iRet != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", iRet);
    }

    stPipeAttr.frame_rate_ctrl.dst_frame_rate = 60;
    stPipeAttr.frame_rate_ctrl.src_frame_rate = 60;
    iRet = ss_mpi_vi_set_pipe_attr(ViPipe,&stPipeAttr);
    if (iRet != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", iRet);
    }
#endif
    return SAL_SOK;
}

/**
 * @function   capt_ssv5EnableCaptUserPic
 * @brief      使能采集模块使用用户图片，用于无信号时编-
               码
 * @param[in]  UINT32 uiDev    输入设备号
 * @param[in]  UINT32 uiEnble  是否使能
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5EnableCaptUserPic(UINT32 uiDev, UINT32 uiEnble)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32 s32Ret = TD_SUCCESS;
    ot_vi_pipe ViPipe = 0;
    ot_vi_pipe_attr      stPipeAttr;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_ssv5GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }



#if 0
    /* 停止中断 */
    s32Ret = OT_MPI_VI_DisablePipeInterrupt(uiVidev);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
#endif
    if(!uiEnble)
    {
        /* 停止使能用户图片 */
        s32Ret = ss_mpi_vi_disable_user_pic(ViPipe);
        if (s32Ret != TD_SUCCESS)
        {
            CAPT_LOGE("failed with %#x!\n", s32Ret);
            return SAL_FAIL;
        }

        pstCaptHalChnPrm->userPic = 0;
    }
    else
    {
        /* 使能用户图片 */
        s32Ret = ss_mpi_vi_enable_user_pic(ViPipe);
        if (s32Ret != TD_SUCCESS)
        {
            CAPT_LOGE("failed with %#x!\n", s32Ret);
            return SAL_FAIL;
        }

        pstCaptHalChnPrm->userPic = 1;

#if 0
        ot_vi_dev ViDev = 0;
        VI_DEV_TIMING_ATTR_S stTimingAttr = {0};

        if (SAL_SOK != capt_ssv5GetDevId(uiChn, &ViDev))
        {
            CAPT_LOGE("capt chn[%u] get dev id fail\n", uiChn);
            return SAL_FAIL;
        }
        
        s32Ret = OT_MPI_VI_GetDevTimingAttr(ViDev, &stTimingAttr);
        if (s32Ret != TD_SUCCESS)
        {
            CAPT_LOGE("ViDev %d failed with %#x!\n",ViDev, s32Ret);
        }
        
        stTimingAttr.bEnable = 0;
        stTimingAttr.s32FrmRate = 60;
        s32Ret = OT_MPI_VI_SetDevTimingAttr(ViDev, &stTimingAttr);
        if (s32Ret != TD_SUCCESS)
        {
           CAPT_LOGE("ViDev %d,failed with %#x!\n",ViDev, s32Ret);
        }
#endif

        SAL_clear(&stPipeAttr);
        s32Ret = ss_mpi_vi_get_pipe_attr(ViPipe,&stPipeAttr);
        if (s32Ret != TD_SUCCESS)
        {
           CAPT_LOGE("failed with %#x!\n", s32Ret);
        }

        stPipeAttr.frame_rate_ctrl.dst_frame_rate = 60;
        stPipeAttr.frame_rate_ctrl.src_frame_rate = 60;
        s32Ret = ss_mpi_vi_set_pipe_attr(ViPipe,&stPipeAttr);
        if (s32Ret != TD_SUCCESS)
        {
           CAPT_LOGE("failed with %#x!\n", s32Ret);
        }
    }
    //Fpga_GetAllRegVol(uiChn);
    return SAL_SOK;
}

#ifdef SS928_ISP   /*isp库编译存在问题先屏蔽*/

/**
 * @function   capt_ssv5IspDeInit
 * @brief      去初始化 ISP
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  通道信息
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5IspDeInit(CAPT_HAL_CHN_PRM * pstPrm)
{
    UINT32 uiIspDev = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspDev = pstPrm->uiIspId;

    ss_mpi_isp_exit(uiIspDev);

    pthread_join(pstPrm->IspPid, NULL);

    pstPrm->uiIspInited = 0;

    return SAL_SOK;
}

/**
 * @function   capt_ssv5UnRegisterIspLib
 * @brief      去注册 ISP 库
 * @param[in]  UINT32 uiIspId  isp通道号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5UnRegisterIspLib(UINT32 uiIspId)
{
    INT32 s32Ret = 0;

    ot_isp_3a_alg_lib stAeLib;
    ot_isp_3a_alg_lib stAwbLib;
    ot_isp_3a_alg_lib stAfLib;

    ot_isp_sns_obj *pstSnsObj = &g_stCaptModPrm.stCaptHalChnPrm[uiIspId].stSnsObj;
    

    memset(&stAeLib, 0, sizeof(ot_isp_3a_alg_lib));
    memset(&stAwbLib, 0, sizeof(ot_isp_3a_alg_lib));
    memset(&stAfLib, 0, sizeof(ot_isp_3a_alg_lib));

    stAeLib.id  = uiIspId;
    stAwbLib.id = uiIspId;
    stAfLib.id  = uiIspId;
    strncpy(stAeLib.lib_name, OT_AE_LIB_NAME, sizeof(OT_AE_LIB_NAME));
    strncpy(stAwbLib.lib_name, OT_AWB_LIB_NAME, sizeof(OT_AWB_LIB_NAME));
    strncpy(stAfLib.lib_name, OT_AF_LIB_NAME, sizeof(OT_AF_LIB_NAME));

    if (pstSnsObj->pfn_un_register_callback != TD_NULL)
    {
        s32Ret = pstSnsObj->pfn_un_register_callback(uiIspId, &stAeLib, &stAwbLib);
        if (s32Ret != TD_SUCCESS)
        {
            CAPT_LOGE("sensor_register_callback failed with %#x!\n",s32Ret);
            return SAL_FAIL;
        }
    }

    if (TD_SUCCESS != ss_mpi_awb_unregister(uiIspId, &stAwbLib))
    { 
        CAPT_LOGE("Func: %s, Line: %d, AWB UnRegister failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

/*
    if (TD_SUCCESS != OT_MPI_AF_UnRegister(uiIspId, &stAfLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AF UnRegister failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }
*/
    if (TD_SUCCESS != ss_mpi_ae_unregister(uiIspId, &stAeLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AE UnRegister failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    return SAL_SOK;
}

#endif
/**
 * @function   capt_ssv5SetChnRotate
 * @brief      设置采集通道选择
 * @param[in]  UINT32 uiDev            设备号
 * @param[in]  UINT32 uiChn            通道号
 * @param[in]  CAPT_ROTATION rotation  旋转属性
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5SetChnRotate(UINT32 uiDev, UINT32 uiChn ,CAPT_ROTATION rotation)
{
    INT32  s32Ret  = 0;
    ot_vi_pipe ViPipe = 0;
    ot_rotation_attr rotation_attr;

    rotation_attr.enable = ENABLED;
    rotation_attr.rotation_type = OT_ROTATION_ANG_FIXED;

    switch(rotation)
    {
        case 0:
        {
            rotation_attr.rotation_fixed = OT_ROTATION_0;
           
            break;
        }
        case 1:
        {
            rotation_attr.rotation_fixed = OT_ROTATION_270;
            break;
        }
        case 2:
        {
            rotation_attr.rotation_fixed = OT_ROTATION_180;
            break;
        }
        case 3:
        {
            rotation_attr.rotation_fixed = OT_ROTATION_90;
            break;
        }
        default:
        {
            rotation_attr.rotation_fixed = OT_ROTATION_0;
            break;
        }
    }
    ViPipe = uiDev;

    if (1 == uiDev)
    {
        ViPipe = CHN1_VIPIPENUM;// lxy20210803 为什么需要映射
    }

    capt_ssv5GetPipeId(uiDev,&ViPipe);
    
    /*设置VI的旋转角度，pipi通道号当前只有0*/
    s32Ret = ss_mpi_vi_set_chn_rotation(ViPipe, 0, &rotation_attr);
    if (TD_SUCCESS != s32Ret)
    {
        CAPT_LOGE("OT_MPI_VI_SetRotate failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   capt_ssv5GetDevStatus
 * @brief      查询采集通道状态
 * @param[in]  UINT32 uiChn                    设备号
 * @param[in]  CAPT_HAL_CHN_STATUS *pstStatus  通道状态
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5GetDevStatus(UINT32 uiChn, CAPT_HAL_CHN_STATUS *pstStatus)
{
    INT32             s32Ret = TD_SUCCESS;
    ot_vi_pipe_status   viStat = {0}; 
    ot_vi_pipe            ViPipe = 0;
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    memset(&viStat, 0, sizeof(ot_vi_pipe_status));

    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    if (0 == pstCaptHalChnPrm->uiStarted)
    {
        return SAL_FAIL;
    }

    capt_ssv5GetPipeId(pstCaptHalChnPrm->uiChn,&ViPipe);

    s32Ret = ss_mpi_vi_query_pipe_status(ViPipe, &viStat);
    if (TD_SUCCESS != s32Ret)
    {
         CAPT_LOGE("ss_mpi_vi_query_pipe_status err ViPipe = %d s32Ret=%#x enable = %d\n",ViPipe, s32Ret, viStat.enable);
        return SAL_FAIL;
    }

    pstStatus->uiIntCnt  = viStat.interrupt_cnt;
    pstStatus->FrameLoss = viStat.lost_frame_cnt;
    //待海思解决不同pipe可以支持不同图片后再打开
    pstStatus->captStatus= 0;  /*0:视频正常,默认视频正常*/

    CAPT_LOGD("ss_mpi_vi_query_pipe_status ViPipe = %d s32Ret=%#x enable = %d\n",ViPipe, s32Ret, viStat.enable);
    return SAL_SOK;
}

/**
 * @function   capt_ssv5stopCaptDev
 * @brief      Camera采集通道停止
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5stopCaptDev(UINT32 uiDev)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32  s32Ret  = 0;
    ot_vi_dev Videv = 0;
    ot_vi_pipe ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_ssv5GetDevId(uiDev, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiDev);
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }
    
    if (0 == pstCaptHalChnPrm->uiStarted)
    {
        CAPT_LOGW("is stoped\n");
        return SAL_SOK;
    }

    pstCaptHalChnPrm->uiStarted = 0;
    if (TD_SUCCESS != (s32Ret = ss_mpi_vi_get_dev_attr(Videv, &pstCaptHalChnPrm->stDevAttr)))
    {
        CAPT_LOGE("Capt Start Get Dev %d Attr failed with %#x!\n", Videv, s32Ret);
        return SAL_FAIL;
    }

    if (TD_SUCCESS != (s32Ret = ss_mpi_vi_get_chn_attr(ViPipe,0, &pstCaptHalChnPrm->stChnAttr)))
    {
        CAPT_LOGE("Capt Start Get Chn %d Attr failed with %#x!\n", ViPipe, s32Ret);
        return SAL_FAIL;
    }

#if 0
    s32Ret = ss_mpi_vi_disable_pipe_interrupt(ViPipe);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
#endif


    s32Ret = ss_mpi_vi_disable_chn(ViPipe,0);  /*写死通道0*/
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
    
    s32Ret = ss_mpi_vi_stop_pipe(ViPipe);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("ss_mpi_vi_stop_pipe failed with %#x!\n", s32Ret);
    }
    
    s32Ret = ss_mpi_vi_destroy_pipe(ViPipe);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("ss_mpi_vi_destroy_pipe failed with %#x!\n", s32Ret);
    }
#if 1
    s32Ret = ss_mpi_vi_unbind(Videv, ViPipe);   /*sd3403和hi3559a差异需要解绑videv和pipi*/
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("ss_mpi_vi_unbind failed with %#x!\n", s32Ret);
    }

    s32Ret = ss_mpi_vi_disable_dev(Videv);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("ss_mpi_vi_enable_dev chn %d failed with %#x!\n", Videv, s32Ret);
        return SAL_FAIL;
    }
    pstCaptHalChnPrm->viDevEn = 0;
#endif

    //pstCaptHalChnPrm->uiStarted = 0;

    CAPT_LOGI("Capt Stop Chn %d !!!\n", uiDev);

    return SAL_SOK;
}

/**
 * @function   capt_ssv5StartCaptDev
 * @brief      Camera采集通道开始
 * @param[in]  UINT32 uiDev  设备号
 * @param[out] None
 * @return     INT32 SAL_SOK
 *                   SAL_FAIL
 */
INT32 capt_ssv5StartCaptDev(UINT32 uiDev)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    INT32  s32Ret  = 0;
    //UINT32 uiVidev = 0;
    ot_vi_pipe ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_ssv5GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiDev);
        return SAL_FAIL;
    }
    
    if (1 == pstCaptHalChnPrm->uiStarted)
    {
        CAPT_LOGW("is started\n");
        return SAL_SOK;
    }

    s32Ret = ss_mpi_vi_start_pipe(ViPipe);
    if (s32Ret != TD_SUCCESS)
    {
        ss_mpi_vi_destroy_pipe(ViPipe);
        CAPT_LOGE("ss_mpi_vi_start_pipe  ViPipe = %d failed with %#x!\n",ViPipe , s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_enable_chn(ViPipe,0);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_enable_pipe_interrupt(ViPipe);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    pstCaptHalChnPrm->uiStarted = 1;

    CAPT_LOGI("Capt Start Chn %d:%d !!!\n", uiDev , ViPipe);

    return SAL_SOK;
}

/**
 * @function   capt_ssv5SetSensorAttr
 * @brief      sensor 设置sensor参数
 * @param[in]  combo_dev_attr_t * pstcomboDevAttr  dev模块控制器属性
 * @param[in]  UINT32 uiReset                      
 * @param[out] None
 * @return     INT32        SAL_SOK
 *                          SAL_FAIL
 */
INT32 capt_ssv5SetSensorAttr(combo_dev_attr_t * pstcomboDevAttr, UINT32 uiReset)
{
    return SAL_SOK;
}

#ifdef CAPT_DEBUG

/**
 * @function   capt_ssv5SetWDRPrm
 * @brief      采集模块 配置 VI 设备宽动态属性
 * @param[in]  UINT32 uiChn     设备号
 * @param[in]  UINT32 uiEnable  使能
 * @param[out] None
 * @return     static INT32
 */
static INT32 capt_ssv5SetWDRPrm(UINT32 uiChn, UINT32 uiEnable)
{
    INT32 s32Ret  = TD_SUCCESS;
    ot_vi_grp fusion_grp = -1;
    ot_vi_wdr_fusion_grp_attr grp_attr;
    ot_vi_dev uiDev = 0;
    ot_vi_bind_pipe bind_pipe;


    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5GetDevId(uiChn, &uiDev))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiChn);
        return SAL_FAIL;
    }

    memset(&grp_attr, 0, sizeof(grp_attr));


    s32Ret = ss_mpi_vi_get_bind_by_dev(uiDev, &bind_pipe);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("ss_mpi_vi_get_bind_by_dev failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    grp_attr.pipe_id[0] = bind_pipe.pipe_id[0]; //lxy ???

    if (1 == uiEnable)
    {
        grp_attr.cache_line = 2160;
        grp_attr.wdr_mode = OT_WDR_MODE_2To1_LINE;
    }
    else
    {
         grp_attr.wdr_mode = OT_WDR_MODE_NONE;
    }

    
    s32Ret = ss_mpi_vi_set_wdr_fusion_grp_attr(fusion_grp, &grp_attr);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("ss_mpi_vi_set_dev_attr failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }


     s32Ret = ss_mpi_vi_get_wdr_fusion_grp_attr(fusion_grp, &grp_attr);
    if (s32Ret != TD_SUCCESS)
    {
          CAPT_LOGE("ss_mpi_vi_set_dev_attr failed with %#x!\n", s32Ret);
          return SAL_FAIL;
    }

    CAPT_LOGI("Set WDR Attr uiVidev %d pipe_id = %d cache_line = %d wdr_mode = %d\n", fusion_grp, grp_attr.pipe_id, grp_attr.cache_line, grp_attr.wdr_mode);

    return SAL_SOK;
}


/**
 * @function   capt_ssv5MipiAttrPrint
 * @brief      调试 MIPI 接口时的调试属性打印
 * @param[in]  combo_dev_attr_t * pstcomboDevAttr  mipi属性
 * @param[out] None
 * @return     static INT32  SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5MipiAttrPrint(combo_dev_attr_t * pstcomboDevAttr)
{
    if (NULL == pstcomboDevAttr)
    {
        return SAL_FAIL;
    }

    // pstcomboDevAttr->mipi_attr.wdr_mode = OT_MIPI_WDR_MODE_DT;
//    pstcomboDevAttr->mipi_attr.lane_id[0] = 0;
//    pstcomboDevAttr->mipi_attr.lane_id[1] = 1;
//    pstcomboDevAttr->mipi_attr.lane_id[2] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[3] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[4] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[5] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[6] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[7] = -1;

//    pstcomboDevAttr->img_rect.x      = 0;
//    pstcomboDevAttr->img_rect.y      = 0;
//    pstcomboDevAttr->img_rect.width  = 1920;
//    pstcomboDevAttr->img_rect.height = 1080;

    CAPT_LOGI("devno         : %d !!!\n", pstcomboDevAttr->devno);
    CAPT_LOGI("input_mode    : %d !!!\n", pstcomboDevAttr->input_mode);
    //CAPT_LOGI("phy_clk_share : %d !!!\n", pstcomboDevAttr->phy_clk_share);
    CAPT_LOGI("img_rect      : x %d y %d w %d h %d !!!\n",
        pstcomboDevAttr->img_rect.x, pstcomboDevAttr->img_rect.y, pstcomboDevAttr->img_rect.width, pstcomboDevAttr->img_rect.height);
    CAPT_LOGI("raw_data_type : %d !!!\n", pstcomboDevAttr->mipi_attr.input_data_type);
    CAPT_LOGI("wdr_mode      : %d !!!\n", pstcomboDevAttr->mipi_attr.wdr_mode);
    
    CAPT_LOGI("lane_id       : %d %d %d %d %d %d %d %d !!!\n", 
        pstcomboDevAttr->mipi_attr.lane_id[0], 
        pstcomboDevAttr->mipi_attr.lane_id[1],
        pstcomboDevAttr->mipi_attr.lane_id[2], 
        pstcomboDevAttr->mipi_attr.lane_id[3],
        pstcomboDevAttr->mipi_attr.lane_id[4],
        pstcomboDevAttr->mipi_attr.lane_id[5], 
        pstcomboDevAttr->mipi_attr.lane_id[6],
        pstcomboDevAttr->mipi_attr.lane_id[7]);

    return SAL_SOK;
}

/**
 * @function   capt_ssv5DisableMipi
 * @brief      关闭mipi_rx控制器
 * @param[in]  void  
 * @param[out] None
 * @return     static void
 */
static void capt_ssv5DisableMipi(void)
{
    INT32 fd  = 0;
    INT32 ret  = 0;
    int i=0;
    combo_dev_t devno = 0;
    fd = open("/dev/ot_mipi", O_RDWR);
    if (fd < 0)
    {
        CAPT_LOGW("fd %d errno %s\n",fd,strerror(errno));
        return;
    }
    for (i = 0; i < 3; i++)
    {
        devno = 0;
        if (i==1)
        {
            devno = 2;
        }
        else if (i==2)
        {
            devno = 4;
        }

        ret = ioctl(fd, OT_MIPI_RESET_MIPI, &devno);
        if (TD_SUCCESS != ret)
        {
            CAPT_LOGE("RESET_MIPI %d failed\n", ret);
        }

        ret = ioctl(fd, OT_MIPI_DISABLE_MIPI_CLOCK, &devno);
        if (TD_SUCCESS != ret)
        {
            CAPT_LOGE("OT_MIPI_DISABLE_MIPI_CLOCK %d failed\n", ret);
        }
    }
    close(fd);
}


#endif

/**
 * @function   capt_ssv5SetMipiAttr
 * @brief      配置 MIPI 属性
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  通道信息
 * @param[in]  input_mode_t  enInputMode  输入接口类型
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5SetMipiAttr(CAPT_HAL_CHN_PRM * pstPrm,input_mode_t  enInputMode)
{
    INT32 fd  = 0;
    INT32 ret = 0;
    lane_divide_mode_t  enHsMode    = LANE_DIVIDE_MODE_1; // lxy20210803  mipi接线方式
    ///input_mode_t        enInputMode = INPUT_MODE_LVDS;    //
    sns_clk_source_t    SnsDev      = 0;
    combo_dev_attr_t    *pstcomboDevAttr = &pstPrm->stcomboDevAttr; 

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    CAPT_LOGI("Set MIPI_%d Attr Start !!!\n", pstcomboDevAttr->devno);

    /* 属性打印 */
#ifdef CAPT_DEBUG
     capt_ssv5MipiAttrPrint(pstcomboDevAttr);
#endif
    /* 打开 MIPI 节点 */
    fd = open("/dev/ot_mipi_rx", O_RDWR);
    if (fd < 0)
    {
        CAPT_LOGE("warning: open ot_mipi dev failed\n");
        return SAL_FAIL;
    }


    ret = ioctl(fd, OT_MIPI_SET_HS_MODE, &enHsMode);
    if (TD_SUCCESS != ret)
    {
        CAPT_LOGE("OT_MIPI_SET_HS_MODE failed(0x%x),\n",ret);
    }
    
    ret = ioctl(fd, OT_MIPI_ENABLE_MIPI_CLOCK, &pstcomboDevAttr->devno);
    if (TD_SUCCESS != ret)
    {
        CAPT_LOGE("MIPI_ENABLE_CLOCK %d failed(0x%x)\n", pstcomboDevAttr->devno,ret);
        close(fd);
        return SAL_FAIL;
    }

    ret = ioctl(fd, OT_MIPI_RESET_MIPI, &pstcomboDevAttr->devno);
    if (TD_SUCCESS != ret)
    {
        CAPT_LOGE("RESET_MIPI %d failed\n", pstcomboDevAttr->devno);
        close(fd);
        return SAL_FAIL;
    }

    /* 2. 重置 sensor 使用拉GPIO的方式 而不使用SOC原生的复位管脚 */
    for (SnsDev = 0; SnsDev < SNS_MAX_CLK_SOURCE_NUM; SnsDev++)
    {
        ret = ioctl(fd, OT_MIPI_ENABLE_SENSOR_CLOCK, &SnsDev);

        if (TD_SUCCESS != ret)
        {
            CAPT_LOGE("OT_MIPI_ENABLE_SENSOR_CLOCK failed\n");
            close(fd);
            return SAL_FAIL;
        }
    }

    // pstcomboDevAttr->mipi_attr.wdr_mode = OT_MIPI_WDR_MODE_DT;
    for (SnsDev = 0; SnsDev < SNS_MAX_RST_SOURCE_NUM; SnsDev++)
   {
       ret = ioctl(fd, OT_MIPI_RESET_SENSOR, &SnsDev);

       if (TD_SUCCESS != ret)
       {
           CAPT_LOGE("OT_MIPI_RESET_SENSOR failed\n");
           close(fd);
           return SAL_FAIL;
       }
   }

    /* 4. 撤销复位 MIPI */
    ret = ioctl(fd, OT_MIPI_SET_DEV_ATTR, pstcomboDevAttr);
    if (TD_SUCCESS != ret)
    {
        CAPT_LOGE("MIPI_SET_DEV_ATTR failed.(0x%x)\n",ret);
        close(fd);
        return SAL_FAIL;
    }

    /*  ......不支持INPUT_MODE_SLVS.............. */
    ret = ioctl(fd, OT_MIPI_UNRESET_MIPI, &pstcomboDevAttr->devno);
    if (TD_SUCCESS != ret)
    {
        CAPT_LOGE("UNRESET_MIPI %d failed\n", pstcomboDevAttr->devno);
        close(fd);
        return SAL_FAIL;
    }
    
    for (SnsDev = 0; SnsDev < SNS_MAX_RST_SOURCE_NUM; SnsDev++)
    {
        ret = ioctl(fd, OT_MIPI_UNRESET_SENSOR, &SnsDev);

        if (TD_SUCCESS != ret)
        {
            CAPT_LOGE("OT_MIPI_UNRESET_SENSOR failed\n");
            close(fd);
            return SAL_FAIL;
        }
    }
    CAPT_LOGI("Set MIPI fd is 0x%x,close\n", fd);
    close(fd);
    return SAL_SOK;
}

/**
 * @function   capt_ssv5Interface
 * @brief      采集模块初始化Mipi接口
 * @param[in]  UINT32 uiChn  通道号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5Interface(UINT32 uiChn)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    ot_vi_intf_mode enIntfMode = OT_VI_INTF_MODE_BUTT;
    ot_vi_dev_attr *pstDevAttr = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    pstCaptHalChnPrm->uiChn   = uiChn;
    pstCaptHalChnPrm->uiIspId = uiChn;
    pstCaptHalChnPrm->uiSnsId = uiChn;

    memcpy(&pstCaptHalChnPrm->stIspPubAttr,&ISP_PUB_ATTR_fpga_60FPS, sizeof(ot_isp_pub_attr));

    enIntfMode = capt_ssv5GetIntfMode();
    switch (enIntfMode)
    {
        case OT_VI_INTF_MODE_BT1120:
            pstDevAttr = &DEV_ATTR_BT1120_BASE;
            break;
        case OT_VI_INTF_MODE_MIPI:   /*VI_MODE_LVDS,  sd3403没有lvds选项*/
            pstDevAttr = &DEV_ATTR_LVDS_BASE;
            break;
        default:
            pstDevAttr = NULL;
            break;
    }

    if (NULL == pstDevAttr)
    {
        CAPT_LOGE("unsupport vi intf mode:%d\n", enIntfMode);
        return SAL_FAIL;
    }

    memcpy(&pstCaptHalChnPrm->stDevAttr, pstDevAttr, sizeof(ot_vi_dev_attr));
    
    CAPT_LOGI("stDevAttr:aram->(%d_%ux%u)\n",pstCaptHalChnPrm->stDevAttr.intf_mode
           ,pstCaptHalChnPrm->stDevAttr.in_size.width
           ,pstCaptHalChnPrm->stDevAttr.in_size.height);
    
    memcpy(&pstCaptHalChnPrm->stChnAttr,&CHN_ATTR_720P_420_SDR8_LINEAR,sizeof(ot_vi_chn_attr));

    /* 配置 MIPI */
    if (SAL_TRUE == capt_ssv5IsUseMipi())
    {
        SAL_clearSize(&pstCaptHalChnPrm->stcomboDevAttr,sizeof(combo_dev_attr_t));
        if (0 == uiChn)
        {
            memcpy(&pstCaptHalChnPrm->stcomboDevAttr,&LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR0,sizeof(combo_dev_attr_t));
        }
        else if (1 == uiChn)
        {
            memcpy(&pstCaptHalChnPrm->stcomboDevAttr,&LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR1,sizeof(combo_dev_attr_t));
        }
        else
        {
            CAPT_LOGE("Capt Hal Mipi Set Failed !!!\n");
            return SAL_FAIL;
        }

        if (SAL_FAIL == capt_ssv5SetMipiAttr(pstCaptHalChnPrm,INPUT_MODE_LVDS))
        {
            CAPT_LOGE("Capt Hal Mipi Set Failed !!!\n");

            return SAL_FAIL;
        }
    }
    
    return SAL_SOK;
}

/**
 * @function   capt_ssv5ReSetMipi
 * @brief      采集模块初重新始化Mipi接口
 * @param[in]  UINT32 uiChn   通道
 * @param[in]  UINT32 width   宽
 * @param[in]  UINT32 height  高
 * @param[in]  UINT32 fps     帧率
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5ReSetMipi(UINT32 uiChn,UINT32 width, UINT32 height, UINT32 fps)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    pstCaptHalChnPrm->uiChn   = uiChn;
    pstCaptHalChnPrm->uiIspId = uiChn;
    pstCaptHalChnPrm->uiSnsId = uiChn;
    width = SAL_alignDown(width,4);
    height = SAL_alignDown(height,4);
    CAPT_LOGI("uiChn %d..wxh = [%d,%d] fps %d\n",uiChn,width,height,fps);
    pstCaptHalChnPrm->stcomboDevAttr.img_rect.width  = width;
    pstCaptHalChnPrm->stcomboDevAttr.img_rect.height = height;
    pstCaptHalChnPrm->stDevAttr.in_size.width      = width;
    pstCaptHalChnPrm->stDevAttr.in_size.height     = height;
    pstCaptHalChnPrm->stChnAttr.size.width      = width;
    pstCaptHalChnPrm->stChnAttr.size.height     = height;
    pstCaptHalChnPrm->stChnAttr.frame_rate_ctrl.src_frame_rate    = fps;
    pstCaptHalChnPrm->stChnAttr.frame_rate_ctrl.dst_frame_rate    = fps;

    if (SAL_TRUE != capt_ssv5IsUseMipi())
    {
        return SAL_SOK;
    }

    /* 配置 MIPI */
    if (SAL_FAIL == capt_ssv5SetMipiAttr(pstCaptHalChnPrm,INPUT_MODE_LVDS))
    {
        CAPT_LOGE("Capt Hal Mipi Set Failed !!!\n");

        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

/**
 * @function   capt_ssv5DeInitCaptChn
 * @brief      初始化 采集通道
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  通道信息
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5DeInitCaptChn(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32  s32Ret  = 0;
    ot_vi_pipe ViPipe   = 0;
    
    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5GetPipeId(pstPrm->uiChn, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_disable_chn(ViPipe, 0);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function   capt_ssv5DeInitCaptDev
 * @brief      去初始化 采集设备
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  通道信息
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5DeInitCaptDev(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32  s32Ret  = 0;
    ot_vi_dev Videv = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_disable_dev(Videv);
    if (TD_SUCCESS != s32Ret)
    {
        CAPT_LOGE("ss_mpi_vi_disable_dev failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    CAPT_LOGI("Capt Hal Stop Vi Dev %d !!!\n", Videv);

    return SAL_SOK;
}

#ifdef SS928_ISP   /*isp库编译存在问题先屏蔽*/
/**
 * @function   capt_ssv5DelSensorIspAttr
 * @brief      反初始化 sensor与 ISP属性
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  通道信息
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5DelSensorIspAttr(CAPT_HAL_CHN_PRM * pstPrm)
{
    UINT32 uiIspId = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspId = pstPrm->uiIspId;

    if (SAL_SOK != capt_ssv5IspDeInit(pstPrm))
    {
        CAPT_LOGE("Func: %s, Line: %d, Isp DeInit failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5UnRegisterIspLib(uiIspId))
    {
        CAPT_LOGE("Func: %s, Line: %d, Un RegisterIsp Lib failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    return SAL_SOK;
}
#endif

/**
 * @function   capt_ssv5InitCaptChn
 * @brief      初始化 采集通道
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  通道信息
 * @param[in]  CAPT_CHN_ATTR *pstChnAttr  通道属性
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5InitCaptChn(CAPT_HAL_CHN_PRM * pstPrm, CAPT_CHN_ATTR *pstChnAttr)
{
    INT32  s32Ret  = 0;
    ot_vi_dev Videv = 0;
    ot_vi_pipe ViPipe  = 0;
    ot_vi_chn_attr *pstViChnAttr = NULL;
    ot_vi_pipe_attr* pstPipeAttr = NULL;
    ot_vi_intf_mode enIntfMode = capt_ssv5GetIntfMode();

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstViChnAttr = &pstPrm->stChnAttr;

    if (OT_VI_INTF_MODE_BT1120 == enIntfMode)
    {
        pstPipeAttr = &PIPE_ATTR_720P_YUV422_3DNR_RFR;
    }
    else
    {
        pstPipeAttr = &PIPE_ATTR_720P_RAW16_420_3DNR_RFR;
    }

    if (SAL_SOK != capt_ssv5GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }
    
    if (SAL_SOK != capt_ssv5GetPipeId(pstPrm->uiChn, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    pstPipeAttr->size.width = pstPrm->stcomboDevAttr.img_rect.width;
    pstPipeAttr->size.height = pstPrm->stcomboDevAttr.img_rect.height;
    pstPipeAttr->frame_rate_ctrl.dst_frame_rate = pstPrm->stChnAttr.frame_rate_ctrl.dst_frame_rate;
    pstPipeAttr->frame_rate_ctrl.src_frame_rate = pstPrm->stChnAttr.frame_rate_ctrl.src_frame_rate;

    s32Ret = ss_mpi_vi_bind(Videv, ViPipe);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("OT_MPI_VI_SetDevBindPipe failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_create_pipe(ViPipe, pstPipeAttr);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("OT_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    /* 暂时vi chn都是0 */
    s32Ret = ss_mpi_vi_set_chn_attr(ViPipe,0, pstViChnAttr);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_get_chn_attr(ViPipe, 0, &pstPrm->stChnAttr);
    if (s32Ret != TD_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    CAPT_LOGI("pipe = %d attr w %d,h %d\n", ViPipe, pstPipeAttr->size.width,pstPipeAttr->size.height);

    return SAL_SOK;
}

/**
 * @function   capt_ssv5InitCaptDev
 * @brief      初始化 采集设备
 * @param[in]  CAPT_HAL_CHN_PRM * pstPrm  设备信息
 * @param[in]  CAPT_CHN_ATTR *pstChnAttr  通道属性
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5InitCaptDev(CAPT_HAL_CHN_PRM * pstPrm, CAPT_CHN_ATTR *pstChnAttr)
{
    INT32  s32Ret  = 0;
    ot_vi_dev Videv = 0;
    ot_vi_dev_attr  *pstViDevAttr = &pstPrm->stDevAttr;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_ssv5GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }
    
    s32Ret = ss_mpi_vi_set_dev_attr(Videv, pstViDevAttr);
    if (s32Ret != TD_SUCCESS)
    {
      CAPT_LOGE("ss_mpi_vi_set_dev_attr failed with %#x! with param->(%d__%ux%u)\n", s32Ret
        ,pstViDevAttr->intf_mode
        ,pstViDevAttr->in_size.width
        ,pstViDevAttr->in_size.height);
      return SAL_FAIL;
    }

    s32Ret = ss_mpi_vi_enable_dev(Videv);
    if (s32Ret != TD_SUCCESS)
    {
      CAPT_LOGE("ss_mpi_vi_enable_dev chn %d failed with %#x!\n", Videv, s32Ret);
      return SAL_FAIL;
    }

    pstPrm->viDevEn = 1;

    s32Ret = ss_mpi_vi_get_dev_attr(Videv, &pstPrm->stDevAttr);
    if (s32Ret != TD_SUCCESS)
    {
      CAPT_LOGE("ss_mpi_vi_get_dev_attr failed with %#x!\n", s32Ret);
      return SAL_FAIL;
    }

    CAPT_LOGI("Capt Hal Start Vi Dev %d !!!\n", Videv);

    return SAL_SOK;
}

/**
 * @function   capt_ssv5InitViDev
 * @brief      Camera采集通道创建
 * @param[in]  UINT32 uiDev               采集设备
 * @param[in]  CAPT_CHN_ATTR *pstChnAttr  采集通道参数
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5InitViDev(UINT32 uiDev, CAPT_CHN_ATTR *pstChnAttr)
{

    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if(pstCaptHalChnPrm->viDevEn == 1)
    {
        CAPT_LOGE("Capt Hal Chn %d is started !!!\n",uiDev);
        return SAL_SOK;
    }
        
    pstCaptHalChnPrm->uiChn   = uiDev;
    pstCaptHalChnPrm->uiIspId = uiDev;
    pstCaptHalChnPrm->uiSnsId = uiDev;
    pstCaptHalChnPrm->uiIspInited = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    /* 配置 vi dev */
    if (SAL_FAIL == capt_ssv5InitCaptDev(pstCaptHalChnPrm, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");

        return SAL_FAIL;
    }

    /* 配置  通道号默认设置为0*/
    if (SAL_FAIL == capt_ssv5InitCaptChn(pstCaptHalChnPrm, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Capt Chn Failed !!!\n");

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   capt_ssv5DeInitViDev
 * @brief      Camera采集通道销毁 
 * @param[in]  UINT32 uiChn  设备通道号
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_ssv5DeInitViDev(UINT32 uiChn)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    /* 销毁 vi chn */
    if (SAL_FAIL == capt_ssv5DeInitCaptChn(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Init Capt Chn Failed !!!\n");

        return SAL_FAIL;
    }

    /* 销毁 vi dev */
    if (SAL_FAIL == capt_ssv5DeInitCaptDev(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");

        return SAL_FAIL;
    }

#ifdef SS928_ISP   /*isp库编译存在问题先屏蔽*/
    /* 销毁 sensor & isp */
    if (SAL_FAIL == capt_ssv5DelSensorIspAttr(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Sensor And Isp Set Failed !!!\n");

        return SAL_FAIL;
    }
#endif
    pstCaptHalChnPrm->uiStarted = 0;
    memset(pstCaptHalChnPrm, 0, sizeof(CAPT_HAL_CHN_PRM));

    return SAL_SOK;
}


/**
 * @function   capt_halRegister
 * @brief      注册hisi disp显示函数
 * @param[in]  void  
 * @param[out] None
 * @return     CAPT_PLAT_OPS * 返回capt函数指针
 */
CAPT_PLAT_OPS *capt_halRegister(void)
{
    memset(&g_stCaptPlatOps, 0, sizeof(CAPT_PLAT_OPS));

    g_stCaptPlatOps.GetCaptPipeId =       capt_ssv5GetPipeId;
    g_stCaptPlatOps.initCaptInterface =   capt_ssv5Interface;
    g_stCaptPlatOps.reSetCaptInterface =  capt_ssv5ReSetMipi;
    g_stCaptPlatOps.initCaptDev   =       capt_ssv5InitViDev;
    g_stCaptPlatOps.deInitCaptDev =       capt_ssv5DeInitViDev;
    g_stCaptPlatOps.startCaptDev  =       capt_ssv5StartCaptDev;
    g_stCaptPlatOps.stopCaptDev   =       capt_ssv5stopCaptDev;
    g_stCaptPlatOps.getCaptDevState =     capt_ssv5GetDevStatus;
    g_stCaptPlatOps.setCaptChnRotate  =   capt_ssv5SetChnRotate;

    g_stCaptPlatOps.enableCaptUserPic =   capt_ssv5EnableCaptUserPic;
    g_stCaptPlatOps.setCaptUserPic    =   capt_ssv5SetCaptUserPic;
    g_stCaptPlatOps.getCaptUserPicStatus = capt_ssv5GetUserPicStatue;
    
    g_stCaptPlatOps.checkCaptIsOffset =    capt_ssv5CaptIsOffset;
     
    return &g_stCaptPlatOps;
}


