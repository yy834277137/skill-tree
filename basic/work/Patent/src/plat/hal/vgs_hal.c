/*******************************************************************************
* vgs_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2018年03月13日 Create
*
* Description : 硬件平台 vgs 模块
* Modification:
*******************************************************************************/
#include "platform_hal.h"
#include "vgs_hal_inter.h"

#line __LINE__ "vgs_hal.c"

/*****************************************************************************
                               宏定义
*****************************************************************************/


/*****************************************************************************
                               结构体定义
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/
VGS_PLAT_OPS *pvgsHalOps = NULL;

/*****************************************************************************
   函 数 名  : vgs_hal_drawLineArray
   功能描述  : 使用 VGS 画多条线
   输入参数  : stTask vgs画线任务参数
                             pstLinePrm 画线的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawLineArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = 0;
    if(NULL == pvgsHalOps || NULL == pvgsHalOps->drawLineArray)
    {
       return SAL_FAIL;
    }
  
    s32Ret = pvgsHalOps->drawLineArray(pstSystemFrame, pstLinePrm);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("vgs_hal_drawLineArray err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : vgs_hal_scaleFrame
   功能描述  : 使用 VGS 缩放
   输入参数  : stTask vgs缩放任务参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_scaleFrame(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH)
{
    INT32 s32Ret = 0;
    if(NULL == pvgsHalOps || NULL == pvgsHalOps->scaleFrame)
    {
       return SAL_FAIL;
    }
  
    s32Ret = pvgsHalOps->scaleFrame(pDstSystemFrame, pSrcSystemFrame, dstW, dstH);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("vgs_hal_scaleFrame err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
   函 数 名  : vgs_hal_drawOSDArray
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = 0;
    if(NULL == pvgsHalOps || NULL == pvgsHalOps->drawOSDArray)
    {
       return SAL_FAIL;
    }
  
    s32Ret = pvgsHalOps->drawOSDArray(pstSystemFrame, pstOsdPrm);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("vgs_hal_drawOSDArray err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : vgs_hal_drawOsdSingle
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawOsdSingle(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = 0;
    if(NULL == pvgsHalOps || NULL == pvgsHalOps->drawOsdSingle)
    {
        return SAL_FAIL;
    }
    
    s32Ret = pvgsHalOps->drawOsdSingle(pstSystemFrame, pstOsdPrm);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("vgs_hal_drawOsdSingle err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : vgs_hal_drawLineOSDArray
   功能描述  : 使用 VGS 画osd和框
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
                             pstLinePrm画框参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawLineOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    if(NULL == pstSystemFrame || NULL == pvgsHalOps || NULL == pvgsHalOps->drawLineOSDArray)
    {
        return SAL_FAIL;
    }
    
    pvgsHalOps->drawLineOSDArray(pstSystemFrame, pstOsdPrm, pstLinePrm);
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_halRegister
* 描  述  : 弱函数当有平台不支持vgs时保证编译通过
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
******************************************************************************
__attribute__((weak)) VGS_PLAT_OPS *vgs_halRegister(void)
{
    return NULL;
}
*/

/*******************************************************************************
* 函数名  : vgs_hal_init
* 描  述  : 初始化vgs模块
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 vgs_hal_init(void)
{
    if(NULL != pvgsHalOps)
    {
        return SAL_SOK;
    }
    
    pvgsHalOps = vgs_halRegister();
    if(NULL == pvgsHalOps)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

