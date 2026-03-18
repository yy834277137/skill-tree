
#ifndef __MSTAR_DRV_H_
#define __MSTAR_DRV_H_

#include "capt_chip_inter.h"


#define MSTAR_WIDTH_MAX     (1920)
#define MSTAR_HEIGHT_MAX    (1080)
#define MSTAR_FPS_MAX       (144)


#define MSTAR_MESSAGE_START_LEN         (1)                 /* 起始码长度 */
#define MSTAR_MESSAGE_BYTE_LEN          (2)                 /* 一个包总长度值的长度 */
#define MSTAR_MESSAGE_CMD_LEN           (1)                 /* 一个命令字的长度 */
#define MSTAR_MESSAGE_CRC_LEN           (1)                 /* CRC长度 */
#define MSTAR_MESSAGE_CMD_START         (MSTAR_MESSAGE_START_LEN + MSTAR_MESSAGE_BYTE_LEN)
#define MSTAR_MESSAGE_HEAD_LEN          (MSTAR_MESSAGE_START_LEN + MSTAR_MESSAGE_BYTE_LEN + MSTAR_MESSAGE_CMD_LEN)
#define MSTAR_MESSAGE_DATA_START        (MSTAR_MESSAGE_HEAD_LEN)
#define MSATR_MESSAGE_MIN_LEN           (MSTAR_MESSAGE_HEAD_LEN + MSTAR_MESSAGE_CRC_LEN)



CAPT_CHIP_FUNC_S *mstar_chipRegister(void);
CAPT_CHIP_FUNC_S *mcu_func_mstarRegister(void);

#endif


