
/*******************************************************************************
 * adv7842_drv.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2020年09月07日 Create
 *
 * Description : ADV7842配置
 * Modification: 
 *******************************************************************************/

#ifndef __ADV7842_DRV_H_
#define __ADV7842_DRV_H_

#include "capt_chip_inter.h"

/* 7842错误类型 */
#define ADV_ERR_IIC_FAIL                0x80000001      /* IIC总线错误 */
#define ADV_ERR_SIG_CHAOS               0x80000002      /* 信号不稳定 */
#define ADV_ERR_NO_TDMS                 0x80000003      /* HDMI输入无TDMS时钟 */
#define ADV_ERR_UNSUPPORT_TIMING        0x80000004      /* 不支持的时序 */

/* HDMI连接的PORT口 */
typedef enum
{
    ADV_HDMI_PORT_A,
    ADV_HDMI_PORT_B,
    ADV_HDMI_PORT_BUTT,
} ADV_HDMI_PORT_E;

/* 对应地址寄存器写入的数据映射表 */
typedef struct
{
    UINT8 u8DevAddr;
    UINT8 u8RegAddr;
    UINT8 u8Value;
} ADV_REG_MAP_S;

/* STDI检测结果，相关定义请参考adv7842数据手册 */
typedef struct
{
    UINT16 u16BL;
    UINT16 u16LCF;
    UINT16 u16LCVS;
    UINT16 u16FCL;
    CAPT_POL_E enHSPol;
    CAPT_POL_E enVSPol;
    BOOL bInterlaced;       /* 是否隔行 */
} ADV_STDI_S;

/* 7842支持的时序和寄存器对应关系 */
typedef struct
{
    BT_TIMING_E enTiming;    /* 输入时序 */
    UINT8       u8RegVal;    /* 7842寄存器值 */
} ADV_REG_TIMING_MAP_S;

/* 7842当前检测的状态 */
typedef struct
{
    BT_TIMING_E enTiming;
    CAPT_CABLE_E enCable;
} ADV_DETECT_STATUS_S;


CAPT_CHIP_FUNC_S *ADV_chipRegister(void);

#endif

