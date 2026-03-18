
#ifndef __HARDWARE_HAL_H_
#define __HARDWARE_HAL_H_

/* ∑÷Œˆ“«≤…ºØ–æ∆¨ */
typedef enum
{
    HARDWARE_INPUT_CHIP_ADV7842,            /* adv7842 */
    HARDWARE_INPUT_CHIP_MSTAR,              /* mstar–æ∆¨ */
    HARDWARE_INPUT_CHIP_MCU_MSTAR,          /* mstar–æ∆¨ */
    HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR,   /* mcu+mstar–æ∆¨+LT9211 */
    HARDWARE_INPUT_CHIP_NONE,               /* Œﬁ≤…ºØ–æ∆¨ */
    HARDWARE_INPUT_CHIP_BUTT,
} HARDWARE_INPUT_CHIP_E;

UINT32 HARDWARE_GetBoardType(VOID);
HARDWARE_INPUT_CHIP_E HARDWARE_GetInputChip(VOID);
INT32 HARDWARE_GetInputEdidIIC(UINT32 u32Chn, IIC_DEV_S *pstIIC);

#endif


