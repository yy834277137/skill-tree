/*******************************************************************************
 * lt9211.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangzhenya5
 * Version: V1.0.0  2021年05月10日 Create
 *
 * Description : lt9211配置,MIPI转LVDS
 * Modification: 
 *******************************************************************************/
 
#ifndef     _LT9211_DRV_H_
#define     _LT9211_DRV_H_


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */


INT32 LT9211_Config(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn);

#endif



