/**
	@brief This sample demonstrates the usage of audio out functions.

	@file audioout_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __AUDIOOUT_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __AUDIOOUT_CONVERT_H__

#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "vpd_ioctl.h"
#include "bind.h"
#include "dif.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* External Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Types Declarations                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* External Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
INT set_aout_unit(HD_GROUP *hd_group, INT line_idx, DIF_AOUT_PARAM *aout_param, INT *len);

#ifdef  __cplusplus
}
#endif
#endif //__AUDIOOUT_CONVERT_H__
