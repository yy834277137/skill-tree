/*******************************************************************************
* disp_hal_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月1日 Create
*
* Description :
* Modification:
*******************************************************************************/



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "platform_sdk.h"
#include "platform_hal.h"
#include "capbility.h"
#include "hal_inc_inter/disp_hal_inter.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */



/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
static DISP_PLAT_OPS *pdispHalOps = NULL;
#define DISP_HAL_MEM_NAME "disp_hal"

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @fn      disp_hal_clearFrameMem
 * @brief   清除Frame数据，置帧图像为纯色
 * 
 * @param   [IN] pstSystemFrameInfo 帧数据
 * @param   [IN] au32BgColor 背景色，YUV格式分别为Y、U、V，ARGB格式分别为B、G、R、A
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS disp_hal_clearFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 au32BgColor[4])
{
    UINT8 *pu8Y = NULL, *pu8UV = NULL;
    UINT32 u32YSize = 0, u32UVSize = 0;
    SAL_VideoFrameBuf stVideoInfo = {0};

    if (NULL == pstSystemFrameInfo)
    {
        DISP_LOGE("pstSystemFrameInfo is NULL\n");
        return SAL_FAIL;
    }

    if (0 == pstSystemFrameInfo->uiDataAddr || 0 == pstSystemFrameInfo->uiDataWidth || 0 == pstSystemFrameInfo->uiDataHeight)
    {
        DISP_LOGE("uiDataAddr(%llu) OR uiDataWidth(%u) OR uiDataHeight(%u) is invalid\n", pstSystemFrameInfo->uiDataAddr, 
                  pstSystemFrameInfo->uiDataWidth, pstSystemFrameInfo->uiDataHeight);
        return SAL_FAIL;
    }

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSystemFrameInfo, &stVideoInfo))
    {
        DISP_LOGE("sys_hal_getVideoFrameInfo failed\n");
        return SAL_FAIL;
    }

    // 暂仅支持NV21和NV12
    if (SAL_VIDEO_DATFMT_YUV420SP_UV == stVideoInfo.frameParam.dataFormat || 
        SAL_VIDEO_DATFMT_YUV420SP_VU == stVideoInfo.frameParam.dataFormat)
    {
        pu8Y = (UINT8 *)stVideoInfo.virAddr[0];
        u32YSize = stVideoInfo.stride[0] * stVideoInfo.frameParam.height;
        pu8UV = (UINT8 *)stVideoInfo.virAddr[1];
        u32UVSize = stVideoInfo.stride[1] * stVideoInfo.frameParam.height / 2;
        memset(pu8Y, au32BgColor[0], u32YSize);
        memset(pu8UV, au32BgColor[1], u32UVSize);

        return SAL_SOK;
    }
    else if (SAL_VIDEO_DATFMT_RGB24_888 == stVideoInfo.frameParam.dataFormat)
    {
        memset((void *)stVideoInfo.virAddr[0], 0XFF, stVideoInfo.stride[0] * stVideoInfo.frameParam.height * 3);
        return SAL_SOK;
    }
    else if (SAL_VIDEO_DATFMT_ARGB_8888 == stVideoInfo.frameParam.dataFormat)
    {
        memset((void *)stVideoInfo.virAddr[0], 0XFF, stVideoInfo.stride[0] * stVideoInfo.frameParam.height * 4);
        return SAL_SOK;
    }
    else
    {
        DISP_LOGE("Unsupport this data format: %d\n", stVideoInfo.frameParam.dataFormat);
        return SAL_FAIL;
    }
}


/*******************************************************************************
* 函数名  : disp_hal_getFrameMem
* 描  述  : 申请帧内存
* 输  入  : - imgW              :
*         : - imgH              :
*         : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_getFrameMem(UINT32 imgW, UINT32 imgH, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 au32BgColor[4] = {0}; /* 背景色 */
    UINT32 u32LumaSize = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};
    UINT64 u64BlkSize = 0;
    SAL_VideoFrameBuf salVideoInfo = {0};
    CAPB_DISP *capb_disp = capb_get_disp();
        
    if(capb_disp->enInputSalPixelFmt == SAL_VIDEO_DATFMT_RGB24_888)
    {
        u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3);
    }
    else
    {
        u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3/2);
    }
    s32Ret = sys_hal_allocVideoFrameInfoSt(pstSystemFrameInfo);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("sys_hal_allocVideoFrameInfoSt failed size %llu !\n", u64BlkSize);
        return SAL_FAIL;
    }
    
    s32Ret = mem_hal_vbAlloc(u64BlkSize, DISP_HAL_MEM_NAME, "disp", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("mem_hal_vbAlloc failed size %llu !\n", u64BlkSize);
        return SAL_FAIL;
    }

    u32LumaSize = (imgW * imgH);

    salVideoInfo.poolId = stVbInfo.u32PoolId;
    salVideoInfo.frameParam.width = imgW;
    salVideoInfo.frameParam.height = imgH;
    salVideoInfo.frameParam.dataFormat = capb_disp->enInputSalPixelFmt;
    salVideoInfo.phyAddr[0] = stVbInfo.u64PhysAddr;
    salVideoInfo.phyAddr[1] = salVideoInfo.phyAddr[0]+ u32LumaSize;
    salVideoInfo.phyAddr[2] = salVideoInfo.phyAddr[1];

    salVideoInfo.virAddr[0] = (PhysAddr)(stVbInfo.pVirAddr);
    salVideoInfo.virAddr[1] = salVideoInfo.virAddr[0]+ u32LumaSize;
    salVideoInfo.virAddr[2] = salVideoInfo.virAddr[1];

    salVideoInfo.stride[0] = salVideoInfo.frameParam.width;
    salVideoInfo.stride[1] = salVideoInfo.frameParam.width;
    salVideoInfo.stride[2] = salVideoInfo.frameParam.width;
    salVideoInfo.vbBlk = (PhysAddr)stVbInfo.u64VbBlk;

    sys_hal_buildVideoFrame(&salVideoInfo, pstSystemFrameInfo);

    // 背景置为纯白色
    if (SAL_VIDEO_DATFMT_PLANAR_Y <= salVideoInfo.frameParam.dataFormat && salVideoInfo.frameParam.dataFormat <= SAL_VIDEO_DATFMT_ARGB_8888)
    {
        au32BgColor[0] = 0xFF; // B
        au32BgColor[1] = 0xFF; // G
        au32BgColor[2] = 0xFF; // R
        au32BgColor[3] = 0xFF; // A
    }
    else // 默认为YUV
    {
        au32BgColor[0] = 0xEB; // Y
        au32BgColor[1] = 0x80; // U
        au32BgColor[2] = 0x80; // V
    }
    
    pstSystemFrameInfo->uiDataAddr = (PhysAddr)salVideoInfo.virAddr[0];
    pstSystemFrameInfo->uiDataWidth = imgW;
    pstSystemFrameInfo->uiDataHeight = imgH;
    disp_hal_clearFrameMem(pstSystemFrameInfo, au32BgColor);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_hal_putFrameMem
* 描  述  : 释放帧内存
* 输  入  : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_putFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_SOK;
    Ptr p64VirAddr = NULL;
    SAL_VideoFrameBuf salVideoInfo;

    /* 释放资源 */
    sys_hal_getVideoFrameInfo(pstSystemFrameInfo, &salVideoInfo);

    p64VirAddr = (Ptr)(salVideoInfo.virAddr[0]);

    s32Ret = mem_hal_vbFree(p64VirAddr, DISP_HAL_MEM_NAME, "disp", salVideoInfo.bufLen, salVideoInfo.vbBlk, 0);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("failed %p\n", p64VirAddr);
        return SAL_FAIL;
    }

    sys_hal_rleaseVideoFrameInfoSt(pstSystemFrameInfo);
  
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hal_voInterface
* 描  述  : 设置VO显示接口
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_voInterface(DISP_DEV_COMMON *pDispDev)
{

    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    return pdispHalOps->setVoInterface(pDispDev);
}

/*******************************************************************************
* 函数名  : disp_hal_getHdmiEdid
* 描  述  : 获取芯片hdmi EDID
* 输  入  : - u32HdmiId: hdimi号
*         :pu8Buff： edid数据
*         :pu32Len：EDID长度
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_getHdmiEdid(UINT32 u32HdmiId, UINT8 *pu8Buff, UINT32 *pu32Len)
{

    if ( NULL == pdispHalOps  || NULL == pdispHalOps->getHdmiEdid)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    return pdispHalOps->getHdmiEdid(u32HdmiId, pu8Buff, pu32Len);
}


/*******************************************************************************
* 函数名  : disp_hal_sendFrame
* 描  述  : 将数据送至vo通道
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_sendFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
   
    return pdispHalOps->sendVoChnFrame(prm, pFrame, s32MilliSec);
}

INT32 disp_hal_pullOutFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
   
    return pdispHalOps->pullOutFrame(prm, pFrame, s32MilliSec);
}

/*******************************************************************************
* 函数名  : disp_drvDeinitStartingup
* 描  述  : 开机时显示反初始化
* 输  入  : - uiDev  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_deInitStartingup(UINT32 uiDev)
{
    return pdispHalOps->deinitVoStartingup(uiDev);
}


/*******************************************************************************
* 函数名  : disp_hal_putNoSignalPicFrame
* 描  述  : 无视频数据时填充视频帧信息
* 输  入  : - videoFrame:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_putNoSignalPicFrame(SAL_VideoFrameBuf *videoFrame,VOID *pFrame)
{
    if (NULL == pdispHalOps || NULL == videoFrame 
        || NULL == pFrame || NULL == pdispHalOps->putVoFrameInfo)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }
    
    return pdispHalOps->putVoFrameInfo(videoFrame, pFrame);
}

/*******************************************************************************
* 函数名  : disp_hal_disableChn
* 描  述  : 禁止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_disableChn(VOID *prm)
{
    if (NULL == pdispHalOps || NULL == pdispHalOps->disableVoChn)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }
    return pdispHalOps->disableVoChn(prm);
}

/*******************************************************************************
* 函数名  : disp_hal_enableChn
* 描  述  : 使能vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_enableChn(VOID *prm)
{
    if (NULL == pdispHalOps || NULL == pdispHalOps->enableVoChn)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    return pdispHalOps->enableVoChn(prm);
}

/*******************************************************************************
* 函数名  : disp_hal_startChn
* 描  述  : 开始vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_startChn(VOID *prm)
{
    if (NULL == pdispHalOps)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    if (NULL != pdispHalOps->startVoChn)
    {
        return pdispHalOps->startVoChn(prm);
    }
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_hal_stopChn
* 描  述  : 停止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_stopChn(VOID *prm)
{
    if (NULL == pdispHalOps)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    if (NULL != pdispHalOps->stopVoChn)
    {
        return pdispHalOps->stopVoChn(prm);
    }
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_hal_setChnPos
* 描  述  : 设置vo参数（放大镜和小窗口）
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_setChnPos(VOID *prm)
{
    if (NULL == pdispHalOps || NULL == pdispHalOps->setVoChnParam)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    return pdispHalOps->setVoChnParam(prm);
}

/*******************************************************************************
* 函数名  : disp_hal_deleteDev
* 描  述  : 删除显示层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_deleteDev(VOID *prm)
{

    if (NULL == pdispHalOps || NULL == pdispHalOps->deleteVoDev)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    return pdispHalOps->deleteVoDev(prm);
}

/*******************************************************************************
* 函数名  : disp_hal_createLayer
* 描  述  : 创建视频层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_createLayer(VOID *prm)
{

    if (NULL == pdispHalOps || NULL == pdispHalOps->createVoLayer)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    return pdispHalOps->createVoLayer(prm);
}


/*******************************************************************************
* 函数名  : disp_hal_clearVoBuf
* 描  述  : 清除vo缓存数据
* 输  入  : - uiLayerNo  : Vo设备号
*         : - uiVoChn  : Vo通道号
          : - bFlag    : 选择模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_clearVoBuf(UINT32 uiLayerNo, UINT32 uiVoChn, UINT32 bFlag)
{
    if (NULL == pdispHalOps || NULL == pdispHalOps->clearVoChnBuf)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }
    
    return pdispHalOps->clearVoChnBuf(uiLayerNo, uiVoChn, bFlag);
}


/*******************************************************************************
* 函数名  : disp_hal_getVoChnFrame
* 描  述  : 获取vo输出帧数据
* 输  入  : VoLayer：视频层号
*       ：- VoChn: 通道号
*       : - pstFrame    :显示数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_getVoChnFrame(UINT32 VoLayer, UINT32 VoChn, SYSTEM_FRAME_INFO *pstFrame)
{
    
    if (NULL == pdispHalOps || NULL == pstFrame || NULL == pdispHalOps->getVoChnFrame)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    return pdispHalOps->getVoChnFrame(VoLayer, VoChn, pstFrame);

}


/*******************************************************************************
* 函数名  : disp_hal_setVoLayerCsc
* 描  述  : 设置视频层图像输出效果
* 输  入  : - uiChn: VO通道
*       : - prm: 视频层参数信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_setVoLayerCsc(UINT32 uiChn, VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    if (NULL == pdispHalOps || NULL == pdispHalOps->setVoLayerCsc)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    s32Ret =  pdispHalOps->setVoLayerCsc(uiChn, prm);

    return s32Ret;
}

/**
 * @function    disp_hal_setVoCommonPrm
 * @brief   显示通用设置接口
 * @param[in]    enType 设置类型
 *               prm设置参数
 * @param[out]
 * @return SAL_SOK  : 成功
 *         SAL_FAIL : 失败
 */
INT32 disp_hal_setVoCommonPrm(DISP_CFG_TYPE_E enType, VOID *prm)
{

    if (NULL == pdispHalOps || NULL == pdispHalOps->setVoCommonPrm)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != pdispHalOps->setVoCommonPrm(enType, prm))
    {
        DISP_LOGE("Disp_halSetVoChnPriority err !\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 创建显示设备
* 输  入  : - pDispDev:设备参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_createDev(DISP_DEV_COMMON *pDispDev)
{
    INT32 s32Ret = SAL_SOK;

    if (pDispDev == NULL || NULL == pdispHalOps || NULL == pdispHalOps->createVoDev)
    {
     DISP_LOGE("null pointer\n");
     return SAL_FAIL;
    }

    s32Ret = pdispHalOps->createVoDev(pDispDev);

    
    return s32Ret;
}

/*******************************************************************************
* 函数名  : disp_hal_modInit
* 描  述  : 显示模块初始化
* 输  入  : VOID *prm
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_modInit(VOID *prm)
{
    if (NULL != pdispHalOps->modInit)
    {
        return pdispHalOps->modInit(prm);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_halSetVoChnPriority
* 描  述  : 设置窗口显示优先级
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Disp_halSetVoChnPriority(VOID *prm)
{
    if (NULL == prm )
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != pdispHalOps->setVoChnPriority(prm))
    {
        DISP_LOGE("Disp_halSetVoChnPriority err !\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 弱函数当有平台不支持VO显示时保证编译通过
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
******************************************************************************
__attribute__((weak)) DISP_PLAT_OPS *disp_halRegister(void)
{
    return NULL;
}
*/


/*******************************************************************************
* 函数名  : disp_hal_enableDispWbc
* 描  述  : 回写使能开启关闭操作
* 输  入  : VoLayer   *pParam
* 输  出  : 
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
******************************************************************************/


INT32 disp_hal_enableDispWbc(UINT32 VoLayer,unsigned int *pParam)
{
    if (SAL_SOK != pdispHalOps->enableDispWbc(VoLayer,pParam))
    {
        DISP_LOGE("disp_hal_getDispWbc err !\n");
        return SAL_FAIL;
    }
    return SAL_SOK;

}


/*******************************************************************************
* 函数名  : disp_hal_getDispWbc
* 描  述  : 回写操作
* 输  入  : VoLayer:
* 输  出  : pstWbcSources
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
******************************************************************************/

INT32 disp_hal_getDispWbc(UINT32 VoLayer,VOID *prm)
{
    VO_FRAME_INFO_ST *pstVFrame=(VO_FRAME_INFO_ST *)prm;
    if (SAL_SOK != pdispHalOps->getDispWbc(VoLayer,pstVFrame))
    {
        DISP_LOGE("disp_hal_getDispWbc err !\n");
        return SAL_FAIL;
    }
    return SAL_SOK;

}

/*******************************************************************************
* 函数名  : disp_hal_createDev
* 描  述  : 初始化disp hal层函结构体
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hal_Init(VOID)
{

    if(NULL != pdispHalOps)
    {
        return SAL_SOK;
    }

    pdispHalOps = disp_halRegister();

    if(NULL == pdispHalOps)
    {
        DISP_LOGE("disp_hal_Init failed!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
    
}


