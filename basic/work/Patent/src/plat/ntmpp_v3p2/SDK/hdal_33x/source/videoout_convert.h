/**
	@brief This sample demonstrates the usage of video process functions.

	@file videoout_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __VIDEOOUT_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __VIDEOOUT_CONVERT_H__

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
INT set_vout_unit(HD_GROUP *hd_group, INT line_idx, DIF_VOUT_PARAM *vout_param, INT *len);
INT set_osg_unit(HD_GROUP *hd_group, INT line_idx, DIF_VOUT_PARAM *vout_param, INT *len);
#ifdef  __cplusplus
}
#endif
#endif //__VIDEOOUT_CONVERT_H__
