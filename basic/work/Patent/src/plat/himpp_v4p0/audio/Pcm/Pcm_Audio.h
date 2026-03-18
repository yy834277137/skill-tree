#ifndef _PCM_AUDIO_H_
#define _PCM_AUDIO_H_


#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "hal_inc_inter/audio_hal_inter.h"

INT32 HAL_PCM_AoCreate(HAL_AIO_ATTR_S *pAIOAttr);
VOID HAL_PCM_AoDestroy(HAL_AIO_ATTR_S *pAIOAttr);
INT32 HAL_PCM_AoSendFrame(HAL_AIO_FRAME_S *pAudioFrame);

INT32 HAL_PCM_AiCreate(HAL_AIO_ATTR_S *pAIOAttr);
VOID HAL_PCM_AiDestroy(HAL_AIO_ATTR_S *pAIOAttr);
INT32 HAL_PCM_AiGetFrame(HAL_AIO_FRAME_S *pAudioFrame);

INT32 HAL_PCM_AoSetVolume(UINT32 Channel, UINT32 Volume);
INT32 HAL_PCM_AiSetVolume(UINT32 Channel, UINT32 Volume);

#endif

