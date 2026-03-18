/**
 * @file   tde_hal.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   平台图形加速接口。
 * @author  wanglei100
 * @date    2021.6.1
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "platform_hal.h"
#include "platform_sdk.h"

#include "tde_hal_inter.h"


/* ========================================================================== */
/*                          全局变量定义区                                    */
/* ========================================================================== */

/* 平台相关的内存申请抽象函数结构体 */
static TDE_PLAT_OPS_ST *g_stTdePlatOps = NULL;

/**
 * @function    tde_hal_QuickResize
 * @brief       光栅位图缩放
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_QuickResize(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect)
{
    INT32 s32Ret = 0;

    if (pstSrc == NULL || pstSrcRect == NULL || pstDst == NULL || pstDstRect == NULL)
    {
        SAL_ERROR("tde error prm \n");
        return SAL_FALSE;
    }

    s32Ret = g_stTdePlatOps->pTdeQuickResize(pstSrc, pstSrcRect, pstDst, pstDstRect);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("tde quick resize error %d \n", s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function    tde_hal_QuickCopyYuv
 * @brief       快速拷贝yuv数据
 * @param[in]
 * @param[out]
 * @return
 */

INT32 tde_hal_QuickCopyYuv(TDE_HAL_SURFACE *pstSrc,
                           TDE_HAL_RECT  *pstSrcRect,
                           TDE_HAL_SURFACE *pstDst,
                           TDE_HAL_RECT *pstDstRect, 
                           UINT32 u32SkipLen,
                           UINT32 bCached)

{
      INT32 s32Ret = 0;
  
      if (NULL == pstSrc
          || NULL == pstSrcRect
          || NULL == pstDst
          || NULL == pstDstRect)
      {
          SAL_ERROR("tde error prm \n");
          return SAL_FALSE;
      }
  
      s32Ret = g_stTdePlatOps->pTdeQuickCopyYuv(pstSrc, pstSrcRect, pstDst, pstDstRect, u32SkipLen, bCached);
      if (s32Ret != SAL_SOK)
      {
          SAL_ERROR("tde quick copy yuv error %d \n", s32Ret);
          return s32Ret;
      }
  
      return SAL_SOK;

}

/**
 * @function    tde_hal_QuickCopy
 * @brief       tde快速拷贝
 * @param[in]
 * @param[out]
 * @return
 */

INT32 tde_hal_QuickCopy(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect, UINT32 bCached)
{
      INT32 s32Ret = 0;
  
      if (pstSrc == NULL || pstSrcRect == NULL || pstDst == NULL || pstDstRect == NULL)
      {
          SAL_ERROR("tde error prm \n");
          return SAL_FALSE;
      }
  
      s32Ret = g_stTdePlatOps->pTdeQuickCopy(pstSrc, pstSrcRect, pstDst, pstDstRect, bCached);
      if (s32Ret != SAL_SOK)
      {
          SAL_ERROR("tde quick copy error 0x%x \n", s32Ret);
          return s32Ret;
      }
  
      return SAL_SOK;

}

/**
 * @function    tde_hal_QuickCopy
 * @brief       tde旋转
 * @param[in]
 * @param[out]
 * @return
 */

INT32 tde_hal_QuickRotate(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, TDE_HAL_ROTATION_E enRotateType)
{
      INT32 s32Ret = 0;
  
      if (pstSrc == NULL || pstSrcRect == NULL || pstDst == NULL ||
           pstDstRect == NULL || NULL == g_stTdePlatOps->pTdeQuickRotate )
      {
          SAL_ERROR("tde error prm \n");
          return SAL_FALSE;
      }

      s32Ret = g_stTdePlatOps->pTdeQuickRotate(pstSrc, pstSrcRect, pstDst, pstDstRect, enRotateType);
      if (s32Ret != SAL_SOK)
      {
          SAL_ERROR("tde quick rotate error 0x%x \n", s32Ret);
          return s32Ret;
      }

      return SAL_SOK;

}


/**
 * @function   tde_hal_DmaCopy
 * @brief      dma视频数据拷贝
 * @param[in]  PhysAddr srcAddrPhy         
 * @param[in]  PhysAddr dstAddrPhy         
 * @param[in]  UINT32 u32Width             
 * @param[in]  UINT32 u32Height            
 * @param[in]  SAL_VideoDataFormat format  
 * @param[out] None
 * @return     INT32
 */



INT32 tde_hal_DmaCopy(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect)
{
    INT32 s32Ret = 0;

    if (pstSrc == NULL || pstSrcRect == NULL || pstDst == NULL || pstDstRect == NULL)
    {
        SAL_ERROR("tde error prm \n");
        return SAL_FALSE;
    }

    if (NULL == g_stTdePlatOps->pTdeDmaCopy)
    {
        SAL_ERROR("plat not support! \n");
        return SAL_FAIL;
    }

    g_stTdePlatOps->pTdeDmaCopy(pstSrc, pstSrcRect, pstDst, pstDstRect);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("dms quick copy error %d \n", s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}


/**
 * @function      tde_hal_Init
 * @brief     tde模块初始化
 * @param[in]
 * @param[out]
 * @return
 */

INT32 tde_hal_Init()
{
    INT32 s32Ret = 0;

    if(NULL != g_stTdePlatOps)
    {
        return SAL_SOK;
    }

    g_stTdePlatOps = tde_hal_register();

    s32Ret = g_stTdePlatOps->pTdeInit();
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("tde init error %d \n", s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

