#ifndef _IOTCL_H_
#define _IOTCL_H_

#include <string.h>
#include <stdio.h>
//#include <sal_type.h>
#include "sal.h"


INT32 SND_userDrvInit();
VOID SND_userDrvDeinit();
INT32 SND_setRateFormat(UINT32 rate);
INT32 SND_setLoopBack(UINT32 isLoop);
INT32 SND_setAdcVolume(UINT32 channelMask, UINT32 volume);
INT32 SND_setDacVolume(UINT32 channelMask, UINT32 volume);

#endif




