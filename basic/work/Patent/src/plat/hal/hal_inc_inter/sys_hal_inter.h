/**
 * @file   dup_hal_inter.h
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
#ifndef _SYS_HAL_INTER_H_

#define _SYS_HAL_INTER_H_

#include "sal.h"
#include "../hal_inc_exter/sys_hal_api.h"


typedef struct _SYS_PLAT_OPS
{
    INT32 (*init)(UINT32 u32ViChnNum);
    INT32 (*deInit)(void);
    INT32 (*getPts)(UINT64 *pCurPts);
    INT32 (*bind)(SYSTEM_MOD_ID_E uiSrcModId, UINT32 uiSrcDevId, UINT32 uiSrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 uiDstDevId, UINT32 uiDstChn,UINT32 uiBind);
    INT32 (*getVideoFrameInfo)(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame);	
    UINT64 (*getMBbyVirAddr)(VOID *pstVirAddr);
    INT32 (*buildVideoFrame)(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo);
    INT32 (*allocVideoFrameInfoSt)(SYSTEM_FRAME_INFO *pstSystemFrameInfo);
    INT32 (*releaseVideoFrameInfoSt)(SYSTEM_FRAME_INFO *pstSystemFrameInfo);
    INT32 (*setVideoTimeInfo)( SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32TimeRef, UINT64 u64Pts);
} SYS_PLAT_OPS;

const SYS_PLAT_OPS *sys_plat_register(void);


#endif /* _SYS_HAL_INTER_H_ */


