/**
 * @file   venc_hal.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---hal层接口封装
 * @author 
 * @date   
 * @note
 * @note \n History
   1.Date        : 2018年12月1日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

 
#include "platform_hal.h"
#include "hal_inc_inter/venc_hal_inter.h"

/*****************************************************************************
                               宏定义
*****************************************************************************/

/*****************************************************************************
                            全局结构体
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/

static VENC_PLAT_OPS_S g_stVencHalOps = {0};
static UINT32 isInit = SAL_FALSE;

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   venc_hal_getFrame
 * @brief    从指定的编码通道获取最新的一帧编码码流，保证和VencHal_putFrame成对调用
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] void *pInPrm 获取编码码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_getFrame(void *pHandle, void *pInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.GetFrame)
    {
        s32Ret = g_stVencHalOps.GetFrame(pHandle, pInPrm);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}
/**
 * @function   venc_hal_putFrame
 * @brief    释放指定通道的码流缓存，和VencHal_getFrame 成对出现
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_putFrame(void *pHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.PutFrame)
    {
        s32Ret = g_stVencHalOps.PutFrame(pHandle);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_getFrameMem
 * @brief      释放帧内存
 * @param[in]   pstSystemFrameInfo
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_putFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_SOK;
    Ptr p64VirAddr = NULL;
    SAL_VideoFrameBuf salVideoInfo;

    /* 释放资源 */
    sys_hal_getVideoFrameInfo(pstSystemFrameInfo, &salVideoInfo);

    p64VirAddr = (Ptr)(salVideoInfo.virAddr[0]);

    s32Ret = mem_hal_vbFree(p64VirAddr, "venc_hal", "venc", pstSystemFrameInfo->uiDataLen, salVideoInfo.vbBlk, 0);
    if (SAL_FAIL == s32Ret)
    {
        VENC_LOGE("failed %p\n", p64VirAddr);
        return SAL_FAIL;
    }

    sys_hal_rleaseVideoFrameInfoSt(pstSystemFrameInfo);
  
    return SAL_SOK;
}


/**
 * @function   venc_hal_getFrameMem
 * @brief      申请帧内存
 * @param[in]   imgW imgH
 * @param[in]   pstSystemFrameInfo
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_getFrameMem(UINT32 imgW, UINT32 imgH, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_FAIL;
   
    UINT32 u32LumaSize = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};
    UINT64 u64BlkSize = 0;
    SAL_VideoFrameBuf salVideoInfo = {0};
    SAL_VideoDataFormat enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_UV;
        

        u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3/2);

    s32Ret = sys_hal_allocVideoFrameInfoSt(pstSystemFrameInfo);
    if (SAL_FAIL == s32Ret)
    {
        VENC_LOGE("sys_hal_allocVideoFrameInfoSt failed size %llu !\n", u64BlkSize);
        return SAL_FAIL;
    }
    
    s32Ret = mem_hal_vbAlloc(u64BlkSize, "venc_hal", "venc", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_FAIL == s32Ret)
    {
        VENC_LOGE("mem_hal_vbAlloc failed size %llu !\n", u64BlkSize);
        return SAL_FAIL;
    }

    u32LumaSize = (imgW * imgH);

    salVideoInfo.poolId = stVbInfo.u32PoolId;
    salVideoInfo.frameParam.width = imgW;
    salVideoInfo.frameParam.height = imgH;
    salVideoInfo.frameParam.dataFormat = enInputSalPixelFmt;
    salVideoInfo.phyAddr[0] = stVbInfo.u64PhysAddr;
    salVideoInfo.phyAddr[1] = salVideoInfo.phyAddr[0]+ u32LumaSize;
    salVideoInfo.phyAddr[2] = salVideoInfo.phyAddr[1];

    salVideoInfo.virAddr[0] = (PhysAddr)(stVbInfo.pVirAddr);
    salVideoInfo.virAddr[1] = salVideoInfo.virAddr[0]+ u32LumaSize;
    salVideoInfo.virAddr[2] = salVideoInfo.virAddr[1];

    salVideoInfo.stride[0] = salVideoInfo.frameParam.width;
    salVideoInfo.stride[1] = salVideoInfo.frameParam.width;
    salVideoInfo.stride[2] = salVideoInfo.frameParam.width;
    salVideoInfo.bufLen    = stVbInfo.u32Size;
    salVideoInfo.vbBlk = (PhysAddr)stVbInfo.u64VbBlk;
    
    sys_hal_buildVideoFrame(&salVideoInfo, pstSystemFrameInfo);

    pstSystemFrameInfo->uiDataAddr = (PhysAddr)salVideoInfo.virAddr[0];
    pstSystemFrameInfo->uiDataWidth = imgW;
    pstSystemFrameInfo->uiDataHeight = imgH;
    pstSystemFrameInfo->uiDataLen = salVideoInfo.bufLen;

    return SAL_SOK;
}


/**
 * @function   venc_hal_encJpeg
 * @brief      JPEG编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  CROP_S *pstCropInfo 编码crop信息
 * @param[in]  BOOL bSetPrm 编码参数更新，用于重配编码器
 * @param[out] INT8 *pJpeg 编码输出JPEG码流
 * @param[out] INT8 *pJpegSize 编码输出JPEG码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_encJpeg(void *pHandle, SYSTEM_FRAME_INFO *pstFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.EncJpegProcess)
    {
        s32Ret = g_stVencHalOps.EncJpegProcess(pHandle, pstFrame, pJpeg, pJpegSize, pstCropInfo, bSetPrm);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }

    return SAL_SOK;
}

/**
 * @function   venc_hal_sendVideoFrm
 * @brief      往编码器送数据(非JPEG)
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_sendVideoFrm(void *pHandle, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.EncSendFrm)
    {
        s32Ret = g_stVencHalOps.EncSendFrm(pHandle, pstFrame);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }

    return SAL_SOK;
}

/**
 * @function   venc_hal_getChn
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] UINT32 *pOutChan plat层通道号指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_getChn(void * pHandle, UINT32 *pOutChan)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.GetHalChn)
    {
        s32Ret = g_stVencHalOps.GetHalChn(pHandle, pOutChan);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_setEncPrm
 * @brief    设置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_setEncPrm(void *pHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.SetParam)
    {
        s32Ret = g_stVencHalOps.SetParam(pHandle, pstInPrm);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_requestIDR
 * @brief    强制I帧编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_requestIDR(void *pHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.ForceIFrame)
    {
        s32Ret = g_stVencHalOps.ForceIFrame(pHandle);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;

}

/**
 * @function   venc_hal_setStatus
 * @brief    设置编码状态，方便drv层管理编码、取流流程
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 状态信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_setStatus(void *pHandle, void *pStr)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.SetVencStatues)
    {
        s32Ret = g_stVencHalOps.SetVencStatues(pHandle, pStr);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_stop
 * @brief    停止编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_stop(void * pHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.Stop)
    {
        s32Ret = g_stVencHalOps.Stop(pHandle);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_start
 * @brief    开始编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_start(void * pHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.Start)
    {
        s32Ret = g_stVencHalOps.Start(pHandle);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_delete
 * @brief    销毁编码器
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_delete(void *pHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.Delete)
    {
        s32Ret = g_stVencHalOps.Delete(pHandle);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

/**
 * @function   venc_hal_create
 * @brief    创建编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stVencHalOps.Create)
    {
        s32Ret = g_stVencHalOps.Create(ppHandle, pstInPrm);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("fun no register\n");
    }
    
    return SAL_SOK;
}

#ifdef DSP_WEAK_FUNC
/**
 * @function   venc_hal_register
 * @brief    弱函数当有平台不支持venc时保证编译通过
 * @param[in]  None
 * @param[out] AUDIO_PLAT_OPS_S *回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
*/

__attribute__((weak) ) INT32 venc_hal_register(VENC_PLAT_OPS_S *pstVencHalOps)
{
    return SAL_FAIL;
}
#endif

/**
 * @function   venc_hal_init
 * @brief    编码模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_init(void)
{
    INT32 s32Ret = SAL_FAIL;

    if (SAL_TRUE == isInit)
    {
        return SAL_SOK;
    }

    memset(&g_stVencHalOps, 0x00, sizeof(VENC_PLAT_OPS_S));

    if (SAL_FAIL == venc_hal_register(&g_stVencHalOps))
    {
        SAL_ERROR("plat reigister fail\n");
        return SAL_SOK;
    }

    if (NULL != g_stVencHalOps.Init)
    {
        s32Ret = g_stVencHalOps.Init();
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }

    isInit = SAL_TRUE;

    return SAL_SOK;
}

