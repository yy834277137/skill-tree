/**
 * @file   vpe_nt98336.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台 vpe 模块
 * @author yeyanzhong
 * @date   2021/12/07
 * @note
 * @note \n History
   1.日    期: 2021/12/07
     作    者: yeyanzhong
     修改历史: 创建文件
 */
#ifndef __VPE_NT9833X_H__
#define __VPE_NT9833X_H__


#include <sal.h>

typedef enum 
{
    VPE_TRIGGER_MODE = 0, /* 单步模式，pushIn/pullOut方式 */
    VPE_FLOW_MODE,        /* 流模式，即绑定方式 */
    VPE_MODE_MAX,
} VPE_MODE_E;

extern UINT32 vpe_nt9833x_getVpeDevId(UINT32 u32GrpId, UINT32 u32Chn);
extern INT32 vpe_nt9833x_setVpeMode(UINT32 u32GrpId, UINT32 u32Chn, VPE_MODE_E enVpeMode);


#endif

