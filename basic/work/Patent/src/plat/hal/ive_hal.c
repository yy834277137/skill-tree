/**
 * @file   ive_hal.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   平台智能硬件加速接口。
 * @author
 * @date    2021.6.10
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include <platform_sdk.h>

#include "ive_hal_inter.h"

/* ========================================================================== */
/*                          全局变量定义区                                    */
/* ========================================================================== */

/* 平台相关的内存申请抽象函数结构体 */
static IVE_PLAT_OPS_ST *g_stIvePlatOps = NULL;

/**
 * @function    ive_hal_CSC
 * @brief       色彩空间转换
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_CSC(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MODE_CTRL *pstIveMapCtrl)
{
    INT32 s32Ret = SAL_SOK;

    if (pstSrc == NULL || pstDst == NULL || pstIveMapCtrl == NULL)
    {
        SAL_ERROR("ive error prm \n");
        return SAL_FALSE;
    }

    s32Ret = g_stIvePlatOps->pIveCSC(pstSrc, pstDst, pstIveMapCtrl);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("ive csc error %x \n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function    ive_hal_QuickCopy
 * @brief       ive快速拷贝
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_QuickCopy(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame, IVE_HAL_RECT *pstSrcRect, IVE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    INT32 s32Ret = SAL_SOK;

    if (NULL == pSrcSysFrame
        || NULL == pDstSysFrame
        || NULL == pstSrcRect
        || NULL == pstDstRect)
    {
        SAL_LOGE("ptr == NULL! %p, %p, %p, %p \n", pSrcSysFrame, pDstSysFrame, pstSrcRect, pstDstRect);
        return SAL_FAIL;
    }

    s32Ret = g_stIvePlatOps->pIveQuickCopy(pSrcSysFrame, pDstSysFrame, pstSrcRect, pstDstRect, bCached);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("ive csc error %x \n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function    ive_hal_Map
 * @brief       map
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_Map(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MEM_INFO *pstMap, IVE_HAL_MODE_CTRL *pstIveMapCtrl)
{
    INT32 s32Ret = SAL_SOK;

    if (pstSrc == NULL || pstDst == NULL || pstIveMapCtrl == NULL || pstMap == NULL)
    {
        SAL_ERROR("ive error prm \n");
        return SAL_FALSE;
    }

    s32Ret = g_stIvePlatOps->pIveMap(pstSrc, pstDst, pstMap,pstIveMapCtrl);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("ive csc error %x \n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

#ifdef DSP_WEAK_FUNC
/**
 * @function   ive_hal_register
 * @brief      弱函数当有平台不支持ive时保证编译通过
 * @param[in]  OSD_PLAT_OPS_S *pstOsdHalOps  
 * @param[out] None
 * @return     __attribute__((weak) ) INT32
 */
__attribute__((weak) ) IVE_PLAT_OPS_ST *ive_hal_register(void)
{
    return NULL;
}
#endif


/**
 * @function    ive_hal_init
 * @brief       初始化ive模块
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_init(void)
{

   g_stIvePlatOps = ive_hal_register( );

    return SAL_SOK;
}


