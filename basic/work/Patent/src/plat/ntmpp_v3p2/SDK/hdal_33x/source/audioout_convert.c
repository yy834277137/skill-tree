/**
	@brief This sample demonstrates the usage of audio out functions.

	@file audioout_convert.c

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
#include "audioout_convert.h"
#include "vg_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define	MAX_POOL_NUM	1

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define ADD_AU_CH_TO_BMP(bmp, ch)	((bmp) | (1 << (ch)))
#define AUDIO_FRAME_SAMPLES			1024

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
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
HD_RESULT get_aout_vpd_enc_type(HD_GROUP *p_group, INT line_idx, vpd_au_enc_type_t *p_vpd_enc_type)
{
	HD_AUDIO_CODEC enc_type;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_vpd_enc_type == NULL) {
		GMLIB_ERROR_P("NULL p_vpd_enc_type\n");
		goto err_exit;
	}

	if (dif_get_adec_codec_type(p_group, line_idx, &enc_type) == HD_OK) {
		switch (enc_type) {
		case HD_AUDIO_CODEC_AAC:
			*p_vpd_enc_type = VPD_AU_ACC;
			break;
		case HD_AUDIO_CODEC_ADPCM:
			*p_vpd_enc_type = VPD_AU_ADPCM;
			break;
		case HD_AUDIO_CODEC_ALAW:
			*p_vpd_enc_type = VPD_AU_G711_ALAW;
			break;
		case HD_AUDIO_CODEC_ULAW:
			*p_vpd_enc_type = VPD_AU_G711_ULAW;
			break;
		case HD_AUDIO_CODEC_PCM:
		default:
			*p_vpd_enc_type = VPD_AU_PCM;
			break;
		}
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_aout_tracks_setting(HD_AUDIO_SOUND_MODE mode, INT *p_track)
{
	if (p_track == NULL) {
		GMLIB_ERROR_P("NULL p_track\n");
		goto err_exit;
	}

	if (mode == HD_AUDIO_SOUND_MODE_MONO) {
		*p_track = 1;
	} else if (mode == HD_AUDIO_SOUND_MODE_STEREO) {
		*p_track = 2;
	} else {
		GMLIB_ERROR_P("HD_AUDIO_SOUND_MODE mode(%d) is not supported.\n", mode);
		goto err_exit;;
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_aout_blocks_setting(HD_AUDIODEC_IN *p_dec_in, INT *p_block_size)
{
	if (p_dec_in == NULL) {
		GMLIB_ERROR_P("NULL p_dec_in\n");
		goto err_exit;
	}
	if (p_block_size == NULL) {
		GMLIB_ERROR_P("NULL p_block_size\n");
		goto err_exit;
	}

	if (p_dec_in->codec_type == HD_AUDIO_CODEC_ADPCM) {
		if (p_dec_in->mode == HD_AUDIO_SOUND_MODE_MONO) {
			*p_block_size = (AUDIO_FRAME_SAMPLES - 1) / 2 + 4;
		} else {
			*p_block_size = (AUDIO_FRAME_SAMPLES - 2) / 2 + 8;
		}
	} else {
		*p_block_size = AUDIO_FRAME_SAMPLES;
	}

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_aout_unit(HD_GROUP *hd_group, INT line_idx, DIF_AOUT_PARAM *aout_param, INT *len)
{
	INT i, tracks = 0, block_size = 0;
	pif_group_t *group;
	vpd_entity_t *aout = NULL;
	vpd_au_render_entity_t *aout_entity;
	INT aout_fd, aout_vch, acap_vch, acap_path;
	vpd_au_enc_type_t vpd_enc_type;
	HD_AUDIODEC_IN dec_in;
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
	if (aout_param == NULL) {
		GMLIB_ERROR_P("NULL aout_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
	}
	aout_vch = aout_param->dev.device_subid;
	if (dif_get_acap_ch_path(hd_group, line_idx, &acap_vch, &acap_path) == HD_OK) {
		/* livesound */
		*len = 0;
		goto null_exit;
	}

	p_au_tx_info = &platform_sys_Info.au_tx_info[aout_vch];

	aout_fd = utl_get_au_render_entity_fd(p_au_tx_info->chipid, p_au_tx_info->dev_no, 0/*needless*/, NULL);

	aout = pif_lookup_entity_buf(VPD_AU_RENDER_ENTITY_TYPE, aout_fd, group->encap_buf);
	if (aout != NULL) {
		*len = 0;
		goto exit;
	}

	if (dif_get_adec_in_param(hd_group, line_idx, &dec_in) != HD_OK) {
		GMLIB_ERROR_P("<aout: no adec unit bind! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (get_aout_vpd_enc_type(hd_group, line_idx, &vpd_enc_type) != HD_OK) {
		GMLIB_ERROR_P("<aout: get_aout_vpd_enc_type fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (get_aout_tracks_setting(dec_in.mode, &tracks) != HD_OK) {
		GMLIB_ERROR_P("<aout: get_aout_tracks_setting fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (get_aout_blocks_setting(&dec_in, &block_size) != HD_OK) {
		GMLIB_ERROR_P("<aout: get_aout_blocks_setting fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}

	*len = pif_set_EntityBuf(VPD_AU_RENDER_ENTITY_TYPE, &aout, group->encap_buf);
	if (aout == NULL) {
		GMLIB_ERROR_P("<aout: NULL entity buffer!>\n");
		goto err_exit;
	}

	aout->sn = aout_fd;
	aout->ap_bindfd = BD_SET_PATH(aout_param->dev.device_baseid + aout_param->dev.device_subid,
				      aout_param->dev.in_subid + HD_IN_BASE,
				      aout_param->dev.out_subid + HD_OUT_BASE);
	memset(aout->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < MAX_POOL_NUM; i++) {
		aout->pool[i].enable = TRUE;
		aout->pool[i].type = VPD_END_POOL;
		aout->pool[i].width = 0;
		aout->pool[i].height = 0;
		aout->pool[i].fps = 30;
		aout->pool[i].vch = 0;
		aout->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		aout->pool[i].is_duplicate = 0;
		aout->pool[i].dst_fmt = VPD_BUFTYPE_UNKNOWN;
		aout->pool[i].ddr_id = 0;
		aout->pool[i].size = 0;
		aout->pool[i].win_size = 0;
		aout->pool[i].count_decuple = 0;
		aout->pool[i].max_counts = 0;
		aout->pool[i].min_counts = 0;
	}

	aout_entity = (vpd_au_render_entity_t *)aout->e;
	aout_entity->ch_bmp = ADD_AU_CH_TO_BMP(0, p_au_tx_info->dev_no);
	aout_entity->ch_bmp2 = 0;
	aout_entity->resample_ratio = 100;
	aout_entity->encode_type = vpd_enc_type;
	aout_entity->block_size = block_size;
	aout_entity->sync_with_lcd_vch = GM_LCD0;
	aout_entity->sample_rate = dec_in.sample_rate;
	aout_entity->sample_bits = dec_in.sample_bit;
	aout_entity->tracks = tracks;

	//src_vch是用來區分不同的'audio pb串'，本可用file的vch，但會有用戶亂填的風險
	// 所以改取第一個au render的vch代替 (同源的多au render vch輸出，也只有一個au_dec entity)
	VG_FIND_U32_FIRST_BIT(aout_entity->ch_bmp, aout_entity->src_vch);
	SET_ENTITY_FEATURE(aout->feature, VPD_FLAG_SYNC_TUNING);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, aout);

err_exit:
	return -1;

null_exit:
	return 0;
}
