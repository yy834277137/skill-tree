/*******************************************************************************
 * lt8619c.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangzhenya5
 * Version: V1.0.0  2021年05月10日 Create
 *
 * Description : lt8619c配置HDMI转LVDS
 * Modification: 
 *******************************************************************************/
 
#ifndef    _LT8619C_DRV_H_
#define    _LT8619C_DRV_H_



/* 函数声明区*/
INT32 LT8619C_Init(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn);



#endif
 

