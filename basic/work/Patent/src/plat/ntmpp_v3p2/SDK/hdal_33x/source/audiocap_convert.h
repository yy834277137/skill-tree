/**
	@brief This sample demonstrates the usage of audio cap functions.

	@file audiocap_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __AUDIOCAP_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __AUDIOCAP_CONVERT_H__

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
INT set_acap_unit(HD_GROUP *hd_group, INT line_idx, DIF_ACAP_PARAM *acap_param, INT *len, INT member_idx);

#ifdef  __cplusplus
}
#endif
#endif //__AUDIOCAP_CONVERT_H__
