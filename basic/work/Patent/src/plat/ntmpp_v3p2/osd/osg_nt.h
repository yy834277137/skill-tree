
/**
 * @file   rgn_hisi.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  osg叠加osd
 * @author 
 * @date   2022年02月12日 Create
 * @note
 * @note \n History
 */

#ifndef __OSG_NT_H_
#define __OSG_NT_H_

#include "sal.h"
#include "platform_hal.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct
{
    UINT32 u32Idx;
    HD_PATH_ID id;
    BOOL bStarted;
    BOOL bCreated;
} OSG_CHN_S;

#ifdef __cplusplus
 }
#endif /* __cplusplus */

#endif
