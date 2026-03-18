/**
 * @file   capt_RK3588.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台采集模块
 * @author fangzuopeng
 * @date   2022/7/1
 * @note
 * @note \n History
   1.日    期: 2022/7/1
     作    者: fangzuopeng
     修改历史: 创建文件
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "capbility.h"
#include "capt_rk3588.h"
#include "../../hal/hal_inc_inter/capt_hal_inter.h"
#include "fmtConvert_rk.h"
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
static VI_USERPIC_ATTR_S gstUserPicAttr[CAPT_CHN_NUM_MAX] = {0};
extern int errno;
static CAPT_PLAT_OPS g_stCaptPlatOps;

/*****************************************************************************
                                函数
*****************************************************************************/


/*****************************************************************************
 函 数 名: capt_rk3588GetDevId
 功能描述  : 根据采集通道获取DEV
 输入参数  : UINT32 u32ChnId : 采集通道号
 输出参数  :   VI_DEV *pDevId : DEV ID
 返 回 值: 
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2020.05.22
    作     者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588GetDevId(UINT32 u32ChnId, VI_DEV *pDevId)
{
    static UINT32 au32DevMap[] = {0, 2};
    
    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }
    
    *pDevId = au32DevMap[u32ChnId];
    
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名: capt_rk3588GetPipeId
 功能描述  : 根据采集通道获取Pipe Id
 输入参数  : UINT32 u32ChnId : 采集通道号
 输出参数  :   VI_PIPE *pPipeId : PIPE ID
 返 回 值: 
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2020.05.22
    作     者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588GetPipeId(UINT32 u32ChnId, VI_PIPE *pPipeId)
{
    static UINT32 au32PipeMap[CAPT_CHN_NUM_MAX] = {0, 2};

    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }

    *pPipeId = au32PipeMap[u32ChnId];

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名 : capt_rk3588GetFrame
 功能描述 : 获取一帧图像
 输入参数   :  uiDev   设备号
           UINT32 u32TimeoutMs     超时时间
 输出参数   : SYSTEM_FRAME_INFO *pstFrame     帧数据
 返 回 值 : 成功:SAL_SOK
            失败:SAL_FAIL
*****************************************************************************/
INT32 capt_rk3588GetFrame(UINT32 uiDev, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32 iRet = 0;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;
    VI_PIPE ViPipe = 0;


    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    if (!pstFrame)
    {
        return SAL_FAIL;
    }

    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if (NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }

    iRet = RK_MPI_VI_GetChnFrame(ViPipe, 0, pstVideoFrame, u32TimeoutMs);
    if (RK_SUCCESS != iRet)
    {
        DUP_LOGE("Vi %d Get Frame Failed, %x, time %d !!!\n", ViPipe,  iRet, u32TimeoutMs);
        return SAL_FAIL;
    }

    pstFrame->uiDataAddr = (PhysAddr)pstVideoFrame->stVFrame.pVirAddr[0];
    pstFrame->uiDataWidth = pstVideoFrame->stVFrame.u32Width;
    pstFrame->uiDataHeight = pstVideoFrame->stVFrame.u32Height;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名 : capt_rk3588RealseFrame
 功能描述 : 释放一帧图像
 输入参数     : uiChn   通道信息
            SYSTEM_FRAME_INFO *pstFrame     帧数据
 输出参数     : 无
 返 回 值 :成功:SAL_SOK
           失败:SAL_FAIL
*****************************************************************************/
INT32 capt_rk3588RealseFrame(UINT32 uiDev, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 iRet = 0;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;
    VI_PIPE ViPipe = 0;


    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    if (!pstFrame)
    {
        return SAL_FAIL;
    }

    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if (NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }

    iRet = RK_MPI_VI_ReleaseChnFrame(ViPipe, 0, pstVideoFrame);
    if (RK_SUCCESS != iRet)
    {
        DUP_LOGE("Vi %d release Frame Failed, %x !!!\n", ViPipe, iRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名 : capt_hal_drvIsOffset
 功能描述 : 计算图像是否有偏移
 输入参数   :   uiChn 输入通道
 输出参数   : 无
 返 回 值 : 无
*****************************************************************************/
static BOOL capt_rk3588CaptIsOffset(UINT32 uiDev)
{
    RK_S32 s32Ret = RK_SUCCESS;
    VIDEO_FRAME_INFO_S stFrame;
    VI_PIPE viPipe    = 0;
    RK_VOID *pvAddr   = NULL;
    UINT8 *pu8Tmp     = NULL;
    UINT32 u32Stride  = 0;
    BOOL bLeft = SAL_TRUE, bRight = SAL_TRUE;
    BOOL bUp = SAL_TRUE, bDown = SAL_TRUE;
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32BitWide = 0;
    SAL_VideoDataFormat penSalPixelFmt = 0;

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &viPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FALSE;
    }


    s32Ret = RK_MPI_VI_GetChnFrame(viPipe, 0, &stFrame, 200);
    if (RK_SUCCESS != s32Ret)
    {
        CAPT_LOGE("capt chn[%u] get frame fail\n", uiDev);
        return SAL_FALSE;
    }

    pvAddr = stFrame.stVFrame.pVirAddr[0];
    u32Stride = stFrame.stVFrame.u32VirWidth;
    
    /* 位宽获取，RGB888有3个字节表示一个像素 */ 
    fmtConvert_rk_rk2sal(stFrame.stVFrame.enPixelFormat, &penSalPixelFmt);
    u32BitWide = SAL_getBitsPerPixel(penSalPixelFmt) >> 3;
    
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride * u32BitWide;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.stVFrame.u32Height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride*u32BitWide;
        /* 最左边的一列不校验 */
        for (j = 1; j < VGA_OFFSET_CHECK_PIXEL; j++)
        {
            /* 判断一个像素是否为黑色            */
            for (k = 0; k < u32BitWide; k++) 
            {
                if (pu8Tmp[j*u32BitWide + k] > 0x10)
                {
                    /* 左边没有黑边 */
                    bLeft = SAL_FALSE;
                    goto right;
                }
            }
        }
    }

right:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.stVFrame.u32Width - VGA_OFFSET_CHECK_PIXEL) * u32BitWide;
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride * u32BitWide;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.stVFrame.u32Height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride * u32BitWide;
        /* 最右边的一列不校验 */
        for (j = 0; j < VGA_OFFSET_CHECK_PIXEL - 1; j++)
        {
            for (k = 0; k < u32BitWide; k++)
            {
                if (pu8Tmp[j*u32BitWide + k] > 0x10)
                {
                    /* 右边没有黑边 */
                    bRight = SAL_FALSE;
                    goto up;
                }
            }
        }
    }

up:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += u32Stride * u32BitWide;
    /* 最上面的一行不校验 */
    for (i = 1; i < VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride * u32BitWide;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.stVFrame.u32Width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            for (k = 0; k < u32BitWide; k++)
            {
                if (pu8Tmp[j*u32BitWide + k] > 0x10)
                {
                    /* 上面没有黑边 */
                    bUp = SAL_FALSE;
                    goto down;
                }
            }
        }
    }

down:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.stVFrame.u32Height - VGA_OFFSET_CHECK_PIXEL) * u32Stride * u32BitWide;
    /* 最下面的一行不校验 */
    for (i = 0; i < VGA_OFFSET_CHECK_PIXEL - 1; i++)
    {
        pu8Tmp += u32Stride * u32BitWide;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.stVFrame.u32Width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            for (k = 0; k < u32BitWide; k++)
            {
                if (pu8Tmp[j*u32BitWide + k] > 0x10)
                {
                    /* 下面没有黑边 */
                    bDown = SAL_FALSE;
                    goto end;
                }
            }
        }
    }

end:

    if (RK_SUCCESS != (s32Ret = RK_MPI_VI_ReleaseChnFrame(viPipe, 0, &stFrame)))
    {
        CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
    }

    CAPT_LOGW("check vi frame left right up down [%d %d %d %d] \n", bLeft, bRight, bUp, bDown);

    /* 上下都有黑边或左右都有黑边时无需调整 */
    return ((bLeft ^ bRight) || (bUp ^ bDown));
}

/*****************************************************************************
 函 数 名  : capt_rk3588GetPoolBlock
 功能描述  : 获取内存池中的内存块
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588GetPoolBlock(UINT32 imgW,UINT32 imgH, VIDEO_FRAME_INFO_S *pOutFrm)
{
    MB_POOL pool        = MB_INVALID_POOLID;
    UINT64  u64BlkSize  = (UINT64)((UINT64)imgW * (UINT64)imgH * 3);
    MB_BLK  VbBlk       = 0;
    UINT64  *p64VirAddr    = NULL;
    UINT64  u64Size     = 0;
    MB_POOL_CONFIG_S stVbPoolCfg;

    if(MB_INVALID_POOLID == pool)
    {
        SAL_clear(&stVbPoolCfg);
        stVbPoolCfg.u64MBSize  = u64BlkSize;
        stVbPoolCfg.u32MBCnt   = 1;
        stVbPoolCfg.enRemapMode = MB_REMAP_MODE_NONE;
        
        pool = RK_MPI_MB_CreatePool(&stVbPoolCfg);

        if(MB_INVALID_POOLID == pool)
        {
            CAPT_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64BlkSize);
            return SAL_FAIL;
        }
    }
    
    u64Size = (UINT64)((UINT64)imgW * (UINT64)imgH * 3);
    VbBlk = RK_MPI_MB_GetMB(pool, u64Size, RK_TRUE);
    if(MB_INVALID_POOLID == pool)
    {
        CAPT_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    p64VirAddr = (UINT64 *) RK_MPI_MB_Handle2VirAddr(VbBlk);
    if(NULL == p64VirAddr)
    {
        CAPT_LOGE("HI_MPI_SYS_Mmap failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }
    pOutFrm->stVFrame.u32Width  = imgW;
    pOutFrm->stVFrame.u32Height = imgH;
    pOutFrm->stVFrame.u32VirWidth  = imgW;
    pOutFrm->stVFrame.u32VirHeight = imgH;
    pOutFrm->stVFrame.enField   = VIDEO_FIELD_FRAME;

    pOutFrm->stVFrame.enPixelFormat   = RK_FMT_RGB888;
    pOutFrm->stVFrame.pVirAddr[0]   = p64VirAddr;
    pOutFrm->stVFrame.pMbBlk  = VbBlk;


    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588GetUserPicStatue
 功能描述  : 采集模块使用用户图片，用于无信号时编码
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static UINT32 capt_rk3588GetUserPicStatue(UINT32 uiDev)
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


/*****************************************************************************
 函 数 名  : capt_rk3588SetCaptUserPic
 功能描述  : 采集模块配置用户图片，用于无信号时编码
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588SetCaptUserPic(UINT32 uiDev, CAPT_CHN_USER_PIC *pstUserPic)
{
    VI_USERPIC_ATTR_S  *pstUserPicAttr = &gstUserPicAttr[uiDev];
    VIDEO_FRAME_INFO_S *pstVideoFrameInfo = NULL;
    VIDEO_FRAME_S      *pVFrame           = NULL;
    INT32   iRet   = 0;
    VI_PIPE ViPipe = 0;
    UINT32  uiW    = 0;
    UINT32  uiH    = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }

    pstVideoFrameInfo = &pstUserPicAttr->unUsrPic.stUsrPicFrm;

    pVFrame = &pstVideoFrameInfo->stVFrame;

    if ((NULL == pstUserPic->pucAddr) || (0 == pstUserPic->uiW) || (0 == pstUserPic->uiH))
    {
        uiW = 1920;
        uiH = 1080;
        /* 上层无图片，底层自己填纯色 */
        pstUserPicAttr->enUsrPicMode  = VI_USERPIC_MODE_BGC;
        pstUserPicAttr->unUsrPic.stUsrPicBg.u32BgColor = 0x80;
        CAPT_LOGI("uiChn %d,use bgc\n",uiDev);
    }
    else
    {
        uiW = pstUserPic->uiW;
        uiH = pstUserPic->uiH;
        pstUserPicAttr->enUsrPicMode  = VI_USERPIC_MODE_PIC;
        CAPT_LOGI("uiChn %d,use pic(wxh %d,%d)\n",uiDev,uiW,uiH);
    }

    /* 图片宽度要16对齐，图片缩放要用到VGS，需要16对齐 否则配置成功，但无图片显示 */
    if (0x00 == pstVideoFrameInfo->stVFrame.pMbBlk || 0x00 == (UINT64)pstVideoFrameInfo->stVFrame.pVirAddr[0])
    {
        uiW = (uiW/16)*16;
        if (SAL_SOK != (iRet = capt_rk3588GetPoolBlock(uiW, uiH, pstVideoFrameInfo)))
        {
            CAPT_LOGE("Capt Hal Set User Pic Get Pool Block Failed, %d !!!\n", iRet);

            return SAL_FAIL;
        }
        CAPT_LOGI("uiChn %u,get mb %llu, addr %llu\n",uiDev,(UINT64)pstVideoFrameInfo->stVFrame.pMbBlk,(UINT64)pstVideoFrameInfo->stVFrame.pVirAddr[0]);
    }
    else
    {
        CAPT_LOGI("uiChn %du,pic mb %llu, addr %llu\n",uiDev,(UINT64)pstVideoFrameInfo->stVFrame.pMbBlk,(UINT64)pstVideoFrameInfo->stVFrame.pVirAddr[0]);
    }

    /* 纯色 */
    if (VI_USERPIC_MODE_BGC == pstUserPicAttr->enUsrPicMode)
    {
        memset(pVFrame->pVirAddr[0], 0x00, uiW * uiH * 3);
    }
    else if (VI_USERPIC_MODE_PIC == pstUserPicAttr->enUsrPicMode) /* 图片 */
    {
        memcpy(pVFrame->pVirAddr[0], pstUserPic->pucAddr, uiW * uiH * 3);
    }
    else
    {
        CAPT_LOGE("ERROR !!!\n");
        return SAL_FAIL;
    }

    if (RK_SUCCESS != (iRet = RK_MPI_VI_SetUserPic(ViPipe, 0, pstUserPicAttr)))
    {
        CAPT_LOGE("Capt ch%u Set User Pic Failed, %x (%d)!!!\n", ViPipe, iRet
            ,pstUserPicAttr->enUsrPicMode);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588EnableCaptUserPic
 功能描述  : 使能采集模块使用用户图片，用于无信号时编码
 输入参数  : -uiChn 输入设备号
         : -uiEnble 是否使能
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588EnableCaptUserPic(UINT32 uiDev, UINT32 uiEnble)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32 s32Ret = RK_SUCCESS;
    VI_PIPE ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }

    if(!uiEnble)
    {
        /* 停止使能用户图片 */
        s32Ret = RK_MPI_VI_DisableUserPic(ViPipe,0);
        if (s32Ret != RK_SUCCESS)
        {
            CAPT_LOGE("failed with %#x!\n", s32Ret);
            return SAL_FAIL;
        }
        
        pstCaptHalChnPrm->userPic = 0;
    }
    else
    {
        /* 使能用户图片 */
        s32Ret = RK_MPI_VI_EnableUserPic(ViPipe,0);
        if (s32Ret != RK_SUCCESS)
        {
            CAPT_LOGE("failed with %#x!\n", s32Ret);
            return SAL_FAIL;
        }

        pstCaptHalChnPrm->userPic = 1;

    }
    //Fpga_GetAllRegVol(uiChn);
    return SAL_SOK;
}


/**
 * @function   capt_rk3588SetChnRotate
 * @brief      设置采集旋转属性
 * @param[in]  UINT32 uiDev            设备号
 * @param[in]  UINT32 uiChn            通道号
 * @param[in]  CAPT_ROTATION rotation  旋转参数
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_rk3588SetChnRotate(UINT32 uiDev, UINT32 uiChn ,CAPT_ROTATION rotation)
{

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_rk3588GetDevStatus
 功能描述  : 查询采集通道状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588GetDevStatus(UINT32 uiChn, CAPT_HAL_CHN_STATUS *pstStatus)
{

    RK_S32             s32Ret = RK_SUCCESS;
    VI_CHN_STATUS_S   viStat;
    VI_PIPE            ViPipe = 0;
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    memset(&viStat, 0, sizeof(VI_CHN_STATUS_S));

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

    if (SAL_SOK != capt_rk3588GetPipeId(pstCaptHalChnPrm->uiChn,&ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", pstCaptHalChnPrm->uiChn);
        return SAL_FAIL;
    }

     /* 暂时vi chn都是0 */
    s32Ret = RK_MPI_VI_QueryChnStatus(ViPipe, 0, &viStat);
    if (RK_SUCCESS != s32Ret)
    {
        CAPT_LOGE("HI_MPI_VI_Query err s32Ret=%#x\n",s32Ret);
        return SAL_FAIL;
    }

#if 0
    pstStatus->uiIntCnt  = viStat.u32IntCnt;
#endif
    pstStatus->FrameLoss = viStat.u32InputLostFrame;
    //待RK不同pipe可以支持不同图片后再打开
    pstStatus->captStatus= 0;  /*0:视频正常,默认视频正常*/
 
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_rk3588stopCaptDev
 功能描述  : Camera采集通道停止
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588stopCaptDev(UINT32 uiDev)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_PIPE ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_rk3588GetDevId(uiDev, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiDev);
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &ViPipe))
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

    s32Ret = RK_MPI_VI_DisableChn(ViPipe,0);
    if (s32Ret != RK_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
    
    if (RK_SUCCESS != (s32Ret = RK_MPI_VI_DisableDev(Videv)))
    {
        CAPT_LOGE("HI_MPI_VI_EnableDev chn %d failed with %#x!\n", Videv, s32Ret);
        return SAL_FAIL;
    }
    
    pstCaptHalChnPrm->viDevEn = 0;

    CAPT_LOGI("Capt Stop Chn %d ,ViPipe %u ,Videv %u!!!\n", uiDev,ViPipe,Videv);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588StartCaptDev
 功能描述  : Camera采集通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_rk3588StartCaptDev(UINT32 uiDev)
{

    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    INT32  s32Ret  = 0;
    //UINT32 uiVidev = 0;
    VI_PIPE ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_rk3588GetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiDev);
        return SAL_FAIL;
    }
    
    if (1 == pstCaptHalChnPrm->uiStarted)
    {
        CAPT_LOGW("is started\n");
        return SAL_SOK;
    }

    s32Ret = RK_MPI_VI_EnableChn(ViPipe,0);
    if (s32Ret != RK_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    pstCaptHalChnPrm->uiStarted = 1;

    CAPT_LOGI("Capt Start Chn %d:%d !!!\n", uiDev , ViPipe);

    return SAL_SOK;
}

#ifdef CAPT_DEBUG  /*如下接口暂时不用需要时注开*/

/**
 * @function   capt_rk3588SetWDRPrm
 * @brief      采集模块 配置 VI 设备宽动态属性
 * @param[in]  UINT32 uiChn     设备号
 * @param[in]  UINT32 uiEnable  是否使能
 * @param[out] None
 * @return     INT32 SAL_SOK  : 成功
 *                   SAL_FAIL : 失败
 */
static INT32 capt_rk3588SetWDRPrm(UINT32 uiChn, UINT32 uiEnable)
{
    INT32 s32Ret  = RK_SUCCESS;
    VI_DEV Videv = 0;
    VI_DEV_ATTR_S  stViDevAttr;

    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetDevId(uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiChn);
        return SAL_FAIL;
    }

    memset(&stViDevAttr, 0, sizeof(stViDevAttr));

     s32Ret = RK_MPI_VI_GetDevAttr(Videv, &stViDevAttr);
    if (s32Ret != RK_SUCCESS)
    {
          CAPT_LOGE("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
          return SAL_FAIL;
    }

    if (TRUE == uiEnable)
    {
        stViDevAttr.stWDRAttr.u32CacheLine = 2160;
        stViDevAttr.stWDRAttr.enWDRMode = WDR_MODE_2To1_LINE;// WDR_MODE_2To1_LINE;

    }
    else
    {
        stViDevAttr.stWDRAttr.enWDRMode = WDR_MODE_NONE;
    }

    s32Ret = HI_MPI_VI_SetDevAttr(Videv, &stViDevAttr);
    if (s32Ret != RK_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#endif



/*****************************************************************************
 函 数 名  : capt_rk3588Interface
 功能描述  : 采集模块初始化Mipi接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588Interface(UINT32 uiChn)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    VI_DEV_ATTR_S *pstDevAttr = NULL;
    VI_PIPE_ATTR_S* pstPipeAttr = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    pstCaptHalChnPrm->uiChn   = uiChn;
    pstCaptHalChnPrm->uiIspId = uiChn;
    pstCaptHalChnPrm->uiSnsId = uiChn;

    /* dev attr */
    pstDevAttr = &DEV_ATTR_MIPI_BASE;
    memcpy(&pstCaptHalChnPrm->stDevAttr, pstDevAttr, sizeof(VI_DEV_ATTR_S));
    
    CAPT_LOGI("stDevAttr:aram->(%d_%ux%u)\n",pstCaptHalChnPrm->stDevAttr.enIntfMode
           ,pstCaptHalChnPrm->stDevAttr.stMaxSize.u32Width
           ,pstCaptHalChnPrm->stDevAttr.stMaxSize.u32Height);

    /* pipe attr */
    pstPipeAttr = &PIPE_ATTR_1080P_RGB888;
    memcpy(&pstCaptHalChnPrm->stPipeAttr, pstPipeAttr, sizeof(VI_PIPE_ATTR_S));

    /* chn attr */
    memcpy(&pstCaptHalChnPrm->stChnAttr,&CHN_ATTR_1080P_RGB888_SDR8_LINEAR,sizeof(VI_CHN_ATTR_S));
    if(uiChn == 0)
    {
        memcpy(&pstCaptHalChnPrm->stChnAttr.stIspOpt, &ISP_OPT_1080P_VIDEO0, sizeof(VI_ISP_OPT_S));
    }
    else
    {
        memcpy(&pstCaptHalChnPrm->stChnAttr.stIspOpt, &ISP_OPT_1080P_VIDEO1, sizeof(VI_ISP_OPT_S));
    }

    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : capt_rk3588SetMipiAttr
 功能描述  : 配置 MIPI 属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588SetMipiAttr(CAPT_HAL_CHN_PRM * pstPrm,
                                            struct v4l2_subdev_format *pstv412SubdevFormat, 
                                            struct v4l2_subdev_selection *pstV412SubdevCrop)
{
    INT32 fd  = 0;
    INT32 ret = 0;
    struct v4l2_subdev_format stV412SubdevFormat = {0};
    
    char devName[32];

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if(pstPrm->uiChn == 0)
    {
        sprintf(devName,INPUT_V4L_SUBDEV0);
    }
    else
    {
        sprintf(devName,INPUT_V4L_SUBDEV1);
    }
    /* 打开 MIPI 节点 */
    fd = open(devName, O_RDWR);
    if (fd < 0)
    {
        CAPT_LOGE("warning: open v4l-subdev dev failed\n");
        return SAL_FAIL;
    }


    /*设置通道大小，固定1080P*/
    ret = ioctl(fd, VIDIOC_SUBDEV_G_FMT, &stV412SubdevFormat);
    if (RK_SUCCESS != ret)
    {
        CAPT_LOGE("VIDIOC_SUBDEV_S_FMT failed(0x%x),\n",ret);
    }

    stV412SubdevFormat.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    stV412SubdevFormat.format.width = pstv412SubdevFormat->format.width;
    stV412SubdevFormat.format.height = pstv412SubdevFormat->format.height;
    
    ret = ioctl(fd, VIDIOC_SUBDEV_S_FMT, &stV412SubdevFormat);
    if (RK_SUCCESS != ret)
    {
        CAPT_LOGE("VIDIOC_SUBDEV_S_FMT failed(0x%x),\n",ret);
    }

    /* 设置裁剪 */
    pstV412SubdevCrop->which = V4L2_SUBDEV_FORMAT_ACTIVE;
    pstV412SubdevCrop->target = V4L2_SEL_TGT_CROP_BOUNDS;
    
    ret = ioctl(fd, VIDIOC_SUBDEV_S_SELECTION, pstV412SubdevCrop);
    if (RK_SUCCESS != ret)
    {
        CAPT_LOGE("VIDIOC_SUBDEV_S_FMT failed(0x%x),\n",ret);
    }
    
    close(fd);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588ReSetMipi
 功能描述  : 采集模块初重新始化Mipi接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588ReSetMipi(UINT32 uiChn,UINT32 width, UINT32 height, UINT32 fps)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    pstCaptHalChnPrm->uiChn   = uiChn;
    pstCaptHalChnPrm->uiIspId = uiChn;
    pstCaptHalChnPrm->uiSnsId = uiChn;

    width = SAL_alignDown(width,64);
    height = SAL_alignDown(height,2);

    pstCaptHalChnPrm->stDevAttr.stMaxSize.u32Width      = width;
    pstCaptHalChnPrm->stDevAttr.stMaxSize.u32Height     = height;
    pstCaptHalChnPrm->stChnAttr.stSize.u32Width      = width;
    pstCaptHalChnPrm->stChnAttr.stSize.u32Height     = height;
    pstCaptHalChnPrm->stChnAttr.stFrameRate.s32SrcFrameRate    = -1;
    pstCaptHalChnPrm->stChnAttr.stFrameRate.s32DstFrameRate    = -1;

    pstCaptHalChnPrm->stV412SubdevFormat.format.width  = 1920;
    pstCaptHalChnPrm->stV412SubdevFormat.format.height = 1080;
    
    pstCaptHalChnPrm->stV412SubdevCrop.r.left = 0;
    pstCaptHalChnPrm->stV412SubdevCrop.r.top = 0;
    pstCaptHalChnPrm->stV412SubdevCrop.r.width = width;
    pstCaptHalChnPrm->stV412SubdevCrop.r.height = height;

    /* 配置 MIPI */
    if (SAL_FAIL == capt_rk3588SetMipiAttr(pstCaptHalChnPrm,&pstCaptHalChnPrm->stV412SubdevFormat,
                                           &pstCaptHalChnPrm->stV412SubdevCrop))
    {
        CAPT_LOGE("Capt Hal Mipi Set Failed !!!\n");

        return SAL_FAIL;
    }
    
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588DeInitCaptChn
 功能描述  : 初始化 采集通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588DeInitCaptChn(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_PIPE ViPipe   = 0;
    
    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VI_DisableChn(ViPipe,Videv);
    if (s32Ret != RK_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : capt_rk3588DeInitCaptDev
 功能描述  : 去初始化 采集设备
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588DeInitCaptDev(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VI_DisableDev(Videv);
    if (RK_SUCCESS != s32Ret)
    {
        CAPT_LOGE("HI_MPI_VI_DisableDev failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    CAPT_LOGI("Capt Hal Stop Vi Dev %d !!!\n", Videv);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_rk3588InitCaptChn
 功能描述  : 初始化 采集通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588InitCaptChn(CAPT_HAL_CHN_PRM * pstPrm, CAPT_CHN_ATTR *pstChnAttr)
{
    
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_PIPE ViPipe  = 0;
    VI_CHN_ATTR_S *pstViChnAttr = NULL;
    VI_DEV_BIND_PIPE_S  stDevBindPipe = {0};
    VI_PIPE_ATTR_S* pstPipeAttr = NULL;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstViChnAttr = &pstPrm->stChnAttr;

    pstPipeAttr = &pstPrm->stPipeAttr;

    if (SAL_SOK != capt_rk3588GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }
    
    if (SAL_SOK != capt_rk3588GetPipeId(pstPrm->uiChn, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    stDevBindPipe.PipeId[0] = ViPipe;//pstPrm->uiChn;
    stDevBindPipe.u32Num = 1;

    pstPipeAttr->stFrameRate.s32DstFrameRate = pstPrm->stChnAttr.stFrameRate.s32DstFrameRate;
    pstPipeAttr->stFrameRate.s32SrcFrameRate = pstPrm->stChnAttr.stFrameRate.s32SrcFrameRate;

    s32Ret = RK_MPI_VI_SetDevBindPipe(Videv, &stDevBindPipe);
    if (s32Ret != RK_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_SetDevBindPipe failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    /* 暂时vi chn都是0 */
    s32Ret = RK_MPI_VI_SetChnAttr(ViPipe,0, pstViChnAttr);
    if (s32Ret != RK_SUCCESS)
    {
        CAPT_LOGE("ViPipe: %u RK_MPI_VI_SetChnAttr failed  with %#x!\n",ViPipe, s32Ret);
        return SAL_FAIL;
    }

    RK_MPI_VI_GetChnAttr(ViPipe, 0, &pstPrm->stChnAttr);


    CAPT_LOGI("pipe attr w %d,h %d\n",pstPipeAttr->u32MaxW,pstPipeAttr->u32MaxH);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588InitCaptDev
 功能描述  : 初始化 采集设备
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588InitCaptDev(CAPT_HAL_CHN_PRM * pstPrm, CAPT_CHN_ATTR *pstChnAttr)
{

    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_DEV_ATTR_S  *pstViDevAttr = &pstPrm->stDevAttr;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_rk3588GetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VI_SetDevAttr(Videv, pstViDevAttr);
    if (s32Ret != RK_SUCCESS)
    {
      CAPT_LOGE("HI_MPI_VI_SetDevAttr failed with %#x! with param->(%d__%ux%u)\n", s32Ret
        ,pstViDevAttr->enIntfMode
        ,pstViDevAttr->stMaxSize.u32Width
        ,pstViDevAttr->stMaxSize.u32Height);
      return SAL_FAIL;
    }

    if (RK_SUCCESS != (s32Ret = RK_MPI_VI_EnableDev(Videv)))
    {
      CAPT_LOGE("RK_MPI_VI_EnableDev chn %d failed with %#x!\n", Videv, s32Ret);
      return SAL_FAIL;
    }

    pstPrm->viDevEn = 1;

    s32Ret = RK_MPI_VI_GetDevAttr(Videv, &pstPrm->stDevAttr);
    if (s32Ret != RK_SUCCESS)
    {
      CAPT_LOGE("RK_MPI_VI_GetDevAttr failed with %#x!\n", s32Ret);
      return SAL_FAIL;
    }

    CAPT_LOGI("Capt Hal Start Vi Dev %d success!!!\n", Videv);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588InitViDev
 功能描述  : Camera采集通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588InitViDev(UINT32 uiDev, CAPT_CHN_ATTR *pstChnAttr)
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
    if (SAL_FAIL == capt_rk3588InitCaptDev(pstCaptHalChnPrm, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");

        return SAL_FAIL;
    }

    /* 配置  通道号默认设置为0*/
    if (SAL_FAIL == capt_rk3588InitCaptChn(pstCaptHalChnPrm, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Capt Chn Failed !!!\n");

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_rk3588DeInitViDev
 功能描述  : Camera采集通道销毁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_rk3588DeInitViDev(UINT32 uiChn)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    /* 销毁 vi chn */
    if (SAL_FAIL == capt_rk3588DeInitCaptChn(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Init Capt Chn Failed !!!\n");

        return SAL_FAIL;
    }

    /* 销毁 vi dev */
    if (SAL_FAIL == capt_rk3588DeInitCaptDev(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm->uiStarted = 0;
    memset(pstCaptHalChnPrm, 0, sizeof(CAPT_HAL_CHN_PRM));

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_halRegister
* 描  述  : 注册rk3588 capt 输入函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
CAPT_PLAT_OPS *capt_halRegister(void)
{
    memset(&g_stCaptPlatOps, 0, sizeof(CAPT_PLAT_OPS));

    g_stCaptPlatOps.GetCaptPipeId =        capt_rk3588GetPipeId;
    g_stCaptPlatOps.initCaptInterface =    capt_rk3588Interface;
    g_stCaptPlatOps.reSetCaptInterface =   capt_rk3588ReSetMipi;
    g_stCaptPlatOps.initCaptDev   =        capt_rk3588InitViDev;
    g_stCaptPlatOps.deInitCaptDev =        capt_rk3588DeInitViDev;
    g_stCaptPlatOps.startCaptDev  =        capt_rk3588StartCaptDev;
    g_stCaptPlatOps.stopCaptDev   =        capt_rk3588stopCaptDev;
    g_stCaptPlatOps.getCaptDevState =      capt_rk3588GetDevStatus;
    g_stCaptPlatOps.setCaptChnRotate  =    capt_rk3588SetChnRotate;
    g_stCaptPlatOps.enableCaptUserPic =    capt_rk3588EnableCaptUserPic;
    g_stCaptPlatOps.setCaptUserPic    =    capt_rk3588SetCaptUserPic;
    g_stCaptPlatOps.getCaptUserPicStatus = capt_rk3588GetUserPicStatue;
    g_stCaptPlatOps.checkCaptIsOffset =    capt_rk3588CaptIsOffset;
    g_stCaptPlatOps.getCaptFrame =    capt_rk3588GetFrame;
    g_stCaptPlatOps.realseCaptFrame =    capt_rk3588RealseFrame;
     
    return &g_stCaptPlatOps;
}

