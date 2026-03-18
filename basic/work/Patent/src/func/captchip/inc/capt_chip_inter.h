/**
 * @file   capt_chip_inter.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  采集芯片内部头文件
 * @author liuxianying
 * @date   2021/10/13
 * @note
 * @note \n History
   1.日    期: 2021/10/13
     作    者: liuxianying
     修改历史: 创建文件
 */
 
#ifndef __CAPT_CHIP_INTER_H_
#define __CAPT_CHIP_INTER_H_
#include "sal.h"
#include <platform_hal.h>
//
#include "../capt_chip_drv_api.h"
#include "edid.h"


/* 采集芯片状态信息 */
typedef struct
{
    CAPT_STATUS_S    stCaptStatus;
    pthread_mutex_t  captChipMutex;
}CAPT_CHIP_STATUS;


typedef INT32 (*Capt_Chip_Init)(UINT32 u32Chn);
typedef INT32 (*Capt_Chip_Reset)(UINT32 u32Chn);
typedef INT32 (*Capt_Chip_HotPlug)(UINT32 u32Chn, CAPT_CABLE_E enCable);
typedef INT32 (*Capt_Chip_Detect)(UINT32 u32Chn, CAPT_STATUS_S *pstStatus);
typedef INT32 (*Capt_Chip_SetRes)(UINT32 u32Chn, CAPT_RESOLUTION_S *pstRes);
typedef INT32 (*Capt_Chip_SetCsc)(UINT32 u32Chn, VIDEO_CSC_S *pstCsc);
typedef INT32 (*Capt_Chip_AutoAdjust)(UINT32 u32Chn, CAPT_CABLE_E enCable);
typedef UINT32 (*Capt_Chip_GetEdid)(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32BuffLen, CAPT_CABLE_E enCable);
typedef INT32 (*Capt_Chip_SetEdid)(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff, CAPT_CABLE_E enCable);
typedef INT32 (*Capt_Chip_Scale)(CAPT_RESOLUTION_S *pstDst, CAPT_RESOLUTION_S *pstSrc);
typedef INT32 (*Capt_Chip_Update)(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len);


/* 对采集芯片操作函数 */
typedef struct
{
    Capt_Chip_Init pfuncInit;
    Capt_Chip_Reset pfuncReset;
    Capt_Chip_Detect pfuncDetect;
    Capt_Chip_SetRes pfuncSetRes;
    Capt_Chip_SetCsc pfuncSetCsc;
    Capt_Chip_AutoAdjust pfuncAutoAdjust;
    Capt_Chip_GetEdid pfuncGetEdid;
    Capt_Chip_SetEdid pfuncSetEdid;
    Capt_Chip_HotPlug pfuncHotPlug;
    Capt_Chip_Scale pfuncScale;
    Capt_Chip_Update pfuncUpdate;
} CAPT_CHIP_FUNC_S;

#endif


