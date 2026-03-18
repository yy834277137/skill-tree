/**
 * @file   dsa_fixed_size_que.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  固定尺寸队列头文件
 * @author dsp
 * @date   2022/6/2
 * @note
 * @note \n History
   1.日    期: 2022/6/2
     作    者: dsp
     修改历史: 创建文件
 */

#ifndef __SAL_FIXED_SIZE_QUE_H_
#define __SAL_FIXED_SIZE_QUE_H_

#include "dsa_que.h"

typedef struct tagDsaFixedSizeQue
{
    DSA_QueHndl empty;          /* empty队列保存未使用的内存的地址 */
    DSA_QueHndl full;           /* full队列保存正在使用的内存的地址 */
    VOID *pvQue;                /* 存放整块数据缓存的地址，用于回收内存 */
} DSA_FIXED_SIZE_QUE_S;


INT32 DSA_FixedSizeQueCreate(DSA_FIXED_SIZE_QUE_S *pstQue, UINT32 u32ElenmentSize, UINT32 u32ElenmentNum);
VOID DSA_FixedSizeQueDelete(DSA_FIXED_SIZE_QUE_S *pstQue);
INT32 DSA_FixedSizeQueGetEmpty(DSA_FIXED_SIZE_QUE_S *pstQue, VOID **ppvData, UINT32 u32Timeout);
INT32 DSA_FixedSizeQuePutEmpty(DSA_FIXED_SIZE_QUE_S *pstQue, VOID *pvData, UINT32 u32Timeout);
INT32 DSA_FixedSizeQueGetFull(DSA_FIXED_SIZE_QUE_S *pstQue, VOID **ppvData, UINT32 u32Timeout);
INT32 DSA_FixedSizeQuePutFull(DSA_FIXED_SIZE_QUE_S *pstQue, VOID *pvData, UINT32 u32Timeout);
INT32 DSA_FixedSizeQuePeekFull(DSA_FIXED_SIZE_QUE_S *pstQue, VOID **ppvData);


#endif /* __SAL_EMPTY_FULL_QUE_H_ */

