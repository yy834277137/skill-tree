/**
	@brief This sample demonstrates the usage of video process functions.

	@file audiodec_convert.c

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
#include "audiodec_convert.h"
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

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_adec_unit(HD_GROUP *hd_group, INT line_idx, DIF_ADEC_PARAM *adec_param, INT *len)
{
	INT i;
	pif_group_t *group;
	vpd_entity_t *adec = NULL;
	vpd_din_entity_t *adec_entity;
	INT adec_fd, adec_path, aout_vch, aout_path;
	HD_AUDIODEC_POOL  *p_out_pool;
	HD_AUDIODEC_IN *p_dec_in;
	HD_PATH_ID path_id = 0;
	vpd_au_tx_sys_info_t  *p_au_tx_info;

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
	if (adec_param == NULL) {
		GMLIB_ERROR_P("NULL adec_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	adec_path = adec_param->dev.out_subid;
	path_id = dif_get_path_id(&adec_param->dev);
	adec_fd = utl_get_datain_entity_fd(path_id);
	if (adec_fd == 0) {
		GMLIB_ERROR_P("<adec: utl_get_datain_entity_fd fail. line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	if (dif_get_aout_ch_path(hd_group, line_idx, &aout_vch, &aout_path) != HD_OK) {
		GMLIB_ERROR_P("<adec: No aout unit bind! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	p_au_tx_info = &platform_sys_Info.au_tx_info[aout_vch];

	p_dec_in = adec_param->param->port[adec_path].p_dec_in;
	if (p_dec_in == NULL) {
		GMLIB_ERROR_P("<adec dec_in param is not set! out_subid(%d) >\n", adec_path);
		goto err_exit;
	}

	adec = pif_lookup_entity_buf(VPD_DIN_ENTITY_TYPE, adec_fd, group->encap_buf);
	if (adec != NULL) {
		*len = 0;
		goto exit;
	}

	*len = pif_set_EntityBuf(VPD_DIN_ENTITY_TYPE, &adec, group->encap_buf);
	if (adec == NULL) {
		GMLIB_ERROR_P("<adec: NULL entity buffer!>\n");
		goto err_exit;
	}
	adec->sn = adec_fd;
	adec->ap_bindfd = BD_SET_PATH(adec_param->dev.device_baseid + adec_param->dev.device_subid,
				      adec_param->dev.in_subid + HD_IN_BASE,
				      adec_param->dev.out_subid + HD_OUT_BASE);
	memset(adec->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		p_out_pool = &adec_param->param->port[adec_path].p_path_config->data_pool[i];
		if (p_out_pool == NULL) {
			GMLIB_ERROR_P("<acap out pool is not set! out_subid(%d) >\n", adec_path);
			goto err_exit;
		}
		if (p_out_pool->mode == HD_AUDIODEC_POOL_DISABLE) {
			adec->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->mode == HD_AUDIODEC_POOL_ENABLE) {
			adec->pool[i].enable = TRUE;
			adec->pool[i].ddr_id = p_out_pool->ddr_id;
			adec->pool[i].size = p_out_pool->frame_sample_size;
			adec->pool[i].count_decuple = p_out_pool->counts;
			adec->pool[i].max_counts = p_out_pool->max_counts / 10;
			adec->pool[i].min_counts = p_out_pool->min_counts / 10;
		} else { //HD_AUDIOCAP_POOL_AUTO
			if (i == 0) {
				adec->pool[i].enable = TRUE;
				adec->pool[i].ddr_id = 0;
				adec->pool[i].size = 1024 * (p_dec_in->sample_bit / 8);
				adec->pool[i].count_decuple = 100;
				adec->pool[i].max_counts = 6;
				adec->pool[i].min_counts = 0;
			} else {
				adec->pool[i].enable = FALSE;
				continue;
			}
		}

		adec->pool[i].type = VPD_AU_DEC_AU_RENDER_IN_POOL;
		snprintf(adec->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(adec->pool[i].type), adec_fd);
		adec->pool[i].width = 0;
		adec->pool[i].height = 0;
		adec->pool[i].fps = 30;
		adec->pool[i].vch = p_au_tx_info->dev_no;
		adec->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		adec->pool[i].is_duplicate = 0;
		adec->pool[i].dst_fmt = VPD_BUFTYPE_DATA;
		adec->pool[i].win_size = 0;

		GMLIB_L2_P("set_adec_unit i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, adec->pool[i].enable, adec->pool[i].name, adec->pool[i].type, adec->pool[i].ddr_id, adec->pool[i].size,
			   adec->pool[i].count_decuple, adec->pool[i].max_counts, adec->pool[i].dst_fmt, adec->pool[i].is_duplicate);
	}

	adec_entity = (vpd_din_entity_t *)adec->e;
	adec_entity->pch = adec_path;
	adec_entity->attr_vch = p_au_tx_info->dev_no;
	adec_entity->is_audio = 1;
	adec_entity->fmt = VPD_BUFTYPE_DATA;
	SET_ENTITY_FEATURE(adec->feature, VPD_FLAG_OUTBUF_MAX_CNT | VPD_FLAG_STOP_WHEN_APPLY);


exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, adec);

err_exit:
	return -1;
}
