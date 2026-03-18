/**
 * @file   ive_hal_inter.h
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
#ifndef _IVE_HAL_INTER_H_

#define _IVE_HAL_INTER_H_

#include "sal.h"

//#include "platform_sdk.h"
#include "platform_hal.h"

typedef struct
{
    INT32 (*pIveCSC)(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MODE_CTRL *pstIveCscCtrl);
	INT32 (*pIveQuickCopy)(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame, IVE_HAL_RECT *pstSrcRect, IVE_HAL_RECT *pstDstRect, UINT32 bCached);
    INT32 (*pIveMap)(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MEM_INFO *pstMap, IVE_HAL_MODE_CTRL *pstIveMapCtrl);
} IVE_PLAT_OPS_ST;

/**
 * @function	ive_hal_register
 * @brief		对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
IVE_PLAT_OPS_ST *ive_hal_register(void);

#endif /* _IVE_HAL_INTER_H_ */

