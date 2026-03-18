/*******************************************************************************
* svp_dsp_hal_api.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : liuxianying<liuxianying@hikvision.com>
* Version: V1.0.0  2021Äê07ÔÂ20ÈÕ Create
*
* Description :
* Modification:
*******************************************************************************/


#ifndef __SVP_DSP_HAL_H_
#define __SVP_DSP_HAL_H_

#include "sal.h"


#define SVP_DSP_CORE_NUM_MAX (4)


UINT32 svpdsp_hal_getCoreNum(VOID);
INT32 svpdsp_hal_setTask(UINT32 u32CoreId, UINT32 enCmd, UINT64 u64PhyAddr, UINT32 u32Len);
INT32 svpdsp_hal_getStatus(UINT32 u32CoreId, BOOL *pbFinish);
INT32 svpdsp_hal_loadDspBin(UINT32 u32DspCoreId);
INT32 svpdsp_hal_unloadDspBin(UINT32 u32DspCoreId);
INT32 svpdsp_hal_initDspCore(UINT32 u32CoreNum, UINT32 *pu32Core);
INT32 svpdsp_hal_init(void);

#endif


