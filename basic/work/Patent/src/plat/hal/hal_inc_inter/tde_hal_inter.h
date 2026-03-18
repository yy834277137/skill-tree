/**
 * @file   tde_hal_inter.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _TDE_HAL_INTER_H_

#define _TDE_HAL_INTER_H_

#include "sal.h"

#include "platform_sdk.h"
#include "platform_hal.h"


typedef struct
{
    INT32 (*pTdeInit)();
    INT32 (*pTdeQuickResize)(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect);
	INT32 (*pTdeQuickCopy)(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect, UINT32 bCached);
    INT32 (*pTdeQuickCopyYuv)(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect, UINT32 u32SkipLen, UINT32 bCached);
    INT32 (*pTdeDmaCopy)(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect);
    INT32 (*pTdeQuickRotate)(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, TDE_HAL_ROTATION_E enRotateType);
} TDE_PLAT_OPS_ST;

/**
 * @function	tde_hal_register
 * @brief		对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
TDE_PLAT_OPS_ST *tde_hal_register(void);


#endif /* _MEM_HAL_INTER_H_ */

