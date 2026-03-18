/**
	@brief This sample demonstrates the usage of video decode functions.

	@file videodec_convert.h

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __VIDEODEC_CONVERT_H__    /* prevent multiple inclusion of the header file */
#define __VIDEODEC_CONVERT_H__

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
#define SUB_RATIO_DEFAULT_VALUE        2  ///< sub yuv ratio default value 1/2

/* sub yuv ratio setting (corresponds to VENDOR_VIDEODEC_SUB_RATIO) */
#define VDEC_CVT_SUB_RATIO_OFF         1  ///< turn off sub-yuv output
#define VDEC_CVT_SUB_RATIO_2x          2  ///< enable, w : 1/2  h : 1/2
#define VDEC_CVT_SUB_RATIO_3x          3  ///< enable, w : 1/3  h : 1/3
#define VDEC_CVT_SUB_RATIO_4x          4  ///< enable, w : 1/4  h : 1/4

/*-----------------------------------------------------------------------------*/
/* External Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _VDEC_CVT_CONFIG {
	unsigned int sub_yuv_ratio;
} VDEC_CVT_CONFIG;

/*-----------------------------------------------------------------------------*/
/* Internal Types Declarations                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* External Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
INT set_datain_unit(HD_GROUP *hd_group, INT line_idx, DIF_VDEC_PARAM *vdec_param, INT *len);
INT set_vdec_unit(HD_GROUP *hd_group, INT line_idx, DIF_VDEC_PARAM *vdec_param, INT *len);
HD_RESULT set_vdec_sub_yuv_setting(DIF_VDEC_PARAM *vdec_param, vpd_dec_entity_t *dec_entity);
VDEC_CVT_CONFIG *get_vdec_convert_config(HD_PATH_ID path_id);

#ifdef  __cplusplus
}
#endif
#endif //__VIDEODEC_CONVERT_H__
