/*******************************************************************************
 * lt8612ex.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : Coordinates
 * Version: V1.0.0  2019年1月5日 Create
 *
 * Description : lt8612ex HDMI 转VGA和HDMI
 * Modification:
 *******************************************************************************/


#ifndef _LT8612EX_DRV_H_
#define _LT8612EX_DRV_H_



typedef enum
{
    MODE_VGA,
    MODE_HDMI,
    MODE_DVI,
    MODE_ALL,
    MODE_MAX
} LT8612_MODE_E;
typedef enum
{
    HDMI_TO_HDMI = 0xc2,
    HDMI_TO_DVI = 0XC4,
    DVI_TO_DVI = 0x80,
    DP_MODE_MAX
}LT8612EX_DP_MODE_E;



INT32 LT8612EX_SetResolution(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn);


#endif 

