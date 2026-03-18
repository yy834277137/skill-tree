/***
 * @file   sal_thr.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  Ïß³Ì²Ù×÷
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_THR_H_
#define _SAL_THR_H_

#include <sched.h>
#include <pthread.h>

#include "sal.h"

#define SAL_THR_NAME_LEN_MAX (16)

#define SAL_THR_PRI_MAX sched_get_priority_max(SCHED_FIFO)
#define SAL_THR_PRI_MIN sched_get_priority_min(SCHED_FIFO)

#define SAL_THR_PRI_DEFAULT (SAL_THR_PRI_MIN + (SAL_THR_PRI_MAX - SAL_THR_PRI_MIN) / 2)

#define SAL_THR_STACK_SIZE_DEFAULT (1024 * 1024)


#define SAL_THR_NORMAL_EXIT NULL
typedef void *SAL_THR_RET_TYPE;

typedef SAL_THR_RET_TYPE (*SAL_ThrEntryFunc)(void *);

typedef struct
{
    pthread_t hndl;
} SAL_ThrHndl;



INT32 SAL_setThrName(const CHAR *thrName);
INT32 SAL_thrCreate(SAL_ThrHndl *hndl, SAL_ThrEntryFunc entryFunc, UINT32 pri, UINT32 stackSize, VOID *prm);
INT32 SAL_thrDelete(SAL_ThrHndl *hndl);
INT32 SAL_thrDetach(SAL_ThrHndl *hndl);
INT32 SAL_thrJoin(SAL_ThrHndl *hndl);
INT32 SAL_thrChangePri(SAL_ThrHndl *hndl, UINT32 pri);
INT32 SAL_SetThreadCoreBind(UINT32 uiCoreNum);
INT32 SAL_SetThreadCoreBindExt(UINT32 uiCoreNum, UINT32 uiCoreId[8]);
INT32 SAL_thrExit(VOID *returnVal);

#define SAL_SET_THR_NAME() \
    { \
        /*SAL_LOGI("%s tsk is running, pid is %ld \n", __FUNCTION__, syscall(SYS_gettid));*/ \
        SAL_setThrName(__FUNCTION__); \
    }


#endif /* _SAL_THR_H_ */

