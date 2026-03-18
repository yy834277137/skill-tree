/**
	@brief This sample demonstrates the usage of audio encode functions.

	@file audioenc_convert.c

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "bind.h"
#include "logger.h"
#include "cif_common.h"
#include "dif.h"
#include "pif.h"
#include "utl.h"
#include "audioenc_convert.h"
#include "vg_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern vpd_sys_info_t platform_sys_Info;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#define	MAX_POOL_NUM			 1

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_aenc_unit(HD_GROUP *hd_group, INT line_idx, DIF_AENC_PARAM *aenc_param, INT *len)
{
	INT i;
	pif_group_t *group;
	vpd_entity_t *aenc = NULL;
	vpd_dout_entity_t *aenc_entity;
	INT aenc_fd, aenc_vch, aenc_path, acap_dev, acap_ch;

	if (len == NULL) {
		GMLIB_ERROR_P("NULL len\n");
		return 0;
	} else {
		*len = 0;
	}
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (aenc_param == NULL) {
		GMLIB_ERROR_P("NULL aenc_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}
	aenc_vch = aenc_param->dev.device_subid;
	aenc_path = aenc_param->dev.out_subid;
	aenc_fd = utl_get_dataout_entity_fd(aenc_vch, aenc_path, (void *)EM_TYPE_AUDIO_DEVICE);

	if (dif_get_acap_ch_path(hd_group, line_idx, &acap_dev, &acap_ch) != HD_OK) {
		GMLIB_ERROR_P("No acap unit bind for aenc\n");
		goto err_exit;
	}

	aenc = pif_lookup_entity_buf(VPD_DOUT_ENTITY_TYPE, aenc_fd, group->encap_buf);
	if (aenc != NULL) {
		*len = 0;
		goto exit;
	}

	*len = pif_set_EntityBuf(VPD_DOUT_ENTITY_TYPE, &aenc, group->encap_buf);
	if (aenc == NULL) {
		GMLIB_ERROR_P("<aenc: NULL entity buffer!>\n");
		goto err_exit;
	}
	aenc->sn = aenc_fd;
	aenc->ap_bindfd = BD_SET_PATH(aenc_param->dev.device_baseid + aenc_param->dev.device_subid,
				      aenc_param->dev.in_subid + HD_IN_BASE,
				      aenc_param->dev.out_subid + HD_OUT_BASE);
	memset(aenc->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < MAX_POOL_NUM; i++) {
		aenc->pool[i].enable = TRUE;
		aenc->pool[i].type = VPD_END_POOL;
		aenc->pool[i].width = 0;
		aenc->pool[i].height = 0;
		aenc->pool[i].fps = 30;
		aenc->pool[i].vch = 0;
		aenc->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		aenc->pool[i].is_duplicate = 0;
		aenc->pool[i].dst_fmt = VPD_BUFTYPE_UNKNOWN;
		aenc->pool[i].ddr_id = 0;
		aenc->pool[i].size = 0;
		aenc->pool[i].win_size = 0;
		aenc->pool[i].count_decuple = 0;
		aenc->pool[i].max_counts = 0;
		aenc->pool[i].min_counts = 0;
	}

	aenc_entity = (vpd_dout_entity_t *)aenc->e;
	aenc_entity->id = acap_ch;
	aenc_entity->is_raw_out = 0;

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, aenc);

err_exit:
	return -1;
}
