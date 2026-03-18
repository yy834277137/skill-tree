/**
	@brief This sample demonstrates the usage of video cap functions.

	@file videocap_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __VIDEOCAP_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __VIDEOCAP_CONVERT_H__

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
INT set_vcap_unit(HD_GROUP *hd_group, INT line_idx, DIF_VCAP_PARAM *vcap_param, INT *len);
INT set_di_unit(HD_GROUP *hd_group, INT line_idx, DIF_VCAP_PARAM *vcap_param,
		VDOPROC_INTL_PARAM *p_vproc_intl_param, INT *len);
HD_RESULT get_vcap_crop_setting(HD_VIDEOCAP_CROP *p_cap_crop, INT cap_vch, vpd_cap_entity_t *cap_entity);

#ifdef  __cplusplus
}
#endif
#endif //__VIDEOCAP_CONVERT_H__
