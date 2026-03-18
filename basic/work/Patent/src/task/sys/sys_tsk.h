#ifndef _SYS_TSK_H_
#define _SYS_TSK_H_


#include "sal.h"
#include "platform_hal.h"

UINT32 sys_task_makeTskID(SYSTEM_MOD_ID_E modID, UINT32 uiModChn);
UINT32 sys_task_getModID(UINT32 uiTskID);
UINT32 sys_task_getModChn(UINT32 uiTskID);
UINT32 sys_task_getModSubChn(UINT32 uiTskID);

INT32 sys_task_init(UINT32 u32ViChnNum);
INT32 sys_task_deInit( );

#endif /* _SYS_TSK_H_ */
