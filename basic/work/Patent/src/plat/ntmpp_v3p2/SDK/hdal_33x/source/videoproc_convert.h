/**
	@brief This sample demonstrates the usage of video process functions.

	@file videoproc_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __VIDEOPROC_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __VIDEOPROC_CONVERT_H__

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
INT set_vproc_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len, INT curr_entity_idx);
INT set_vproc_datain_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len);
INT set_vproc_osg_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len);
INT set_vproc_dataout_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len);
HD_RESULT get_vproc_crop_setting(DIF_VPROC_PARAM *vproc_param, vpd_vpe_1_entity_t *vpe_1_entity, INT prev_is_vdec);

#ifdef  __cplusplus
}
#endif
#endif //__VIDEOPROC_CONVERT_H__
