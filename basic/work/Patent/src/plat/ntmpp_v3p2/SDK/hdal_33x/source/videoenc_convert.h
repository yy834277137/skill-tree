/**
	@brief This sample demonstrates the usage of video enc functions.

	@file videoenc_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __VIDEOENC_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __VIDEOENC_CONVERT_H__

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
INT set_enc_osg_unit(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *vout_param, INT *len, INT curr_entity_idx);
INT set_venc_unit(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *venc_param, INT *len);
INT set_dataout_unit(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *venc_param, INT *len);

#ifdef  __cplusplus
}
#endif
#endif //__VIDEOENC_CONVERT_H__
