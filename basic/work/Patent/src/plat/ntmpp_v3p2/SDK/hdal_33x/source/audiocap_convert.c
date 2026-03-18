/**
	@brief This sample demonstrates the usage of audio cap functions.

	@file audiocap_convert.c

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
#include "audiocap_convert.h"
#include "vg_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define ADD_AU_CH_TO_BMP(bmp, ch)	((bmp) | (1 << (ch)))
#define IS_AU_CH_EXISTED(bmp, ch)   ((bmp) & (1 << (ch)))

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

HD_RESULT get_acap_ch_count(INT dev_no, INT *p_count)
{
	vpd_au_rx_sys_info_t *p_au_rx_info;

	if (dev_no >= VPD_MAX_AU_RX_NUM) {
		GMLIB_ERROR_P("Exceed max RX count(%d>=%d)\n", dev_no, VPD_MAX_AU_RX_NUM);
		goto err_exit;
	}

	p_au_rx_info = &platform_sys_Info.au_rx_info[dev_no];
	if (p_au_rx_info->ch < 0) {
		GMLIB_ERROR_P("Wrong rx_info\n");
		goto err_exit;
	}
	if (p_count) {
		*p_count = p_au_rx_info->channel_num;
	} else {
		GMLIB_ERROR_P("NULL p_count\n");
		goto err_exit;
	}

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_acap_tracks_setting(HD_AUDIO_SOUND_MODE mode, INT *p_track)
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

HD_RESULT get_acap_vpd_enc_type(HD_GROUP *p_group, INT line_idx, vpd_au_enc_type_t *p_vpd_enc_type)
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

	if (dif_get_aenc_codec_type(p_group, line_idx, &enc_type) == HD_OK) {
		switch (enc_type) {
		case GM_AAC:
			*p_vpd_enc_type = VPD_AU_ACC;
			break;
		case GM_ADPCM:
			*p_vpd_enc_type = VPD_AU_ADPCM;
			break;
		case GM_G711_ALAW:
			*p_vpd_enc_type = VPD_AU_G711_ALAW;
			break;
		case GM_G711_ULAW:
			*p_vpd_enc_type = VPD_AU_G711_ULAW;
			break;
		case GM_PCM:
		default:
			*p_vpd_enc_type = VPD_AU_PCM;
			break;
		}
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_acap_unit(HD_GROUP *hd_group, INT line_idx, DIF_ACAP_PARAM *acap_param, INT *len, INT member_idx)
{
	INT i, ch_count = 0, tracks = 0, one_ch_buf_size = 0, bind_count = 0, next_devid = 0;
	UINT32  next_in_sub_id, next_out_sub_id, auenc_dataout_id;
	pif_group_t *group;
	vpd_entity_t *acap = NULL;
	vpd_au_grab_entity_t *acap_entity;
	INT acap_fd, acap_dev, acap_ch, aout_vch, aout_path;
	HD_AUDIOCAP_POOL *p_out_pool;
	HD_AUDIOCAP_IN  *p_audcap_in;
	AUDCAP_INFO_PRIV  *p_priv_info;
	vpd_au_enc_type_t vpd_enc_type;
	vpd_au_rx_sys_info_t  *p_au_rx_info;

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
	if (acap_param == NULL) {
		GMLIB_ERROR_P("NULL acap_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	acap_dev = acap_param->dev.device_subid;
	acap_ch = acap_param->dev.out_subid;

	p_au_rx_info = &platform_sys_Info.au_rx_info[acap_dev];

	if (dif_get_aout_ch_path(hd_group, line_idx, &aout_vch, &aout_path) == HD_OK) {
		/* livesound */
		*len = 0;
		goto null_exit;
	}

	acap_fd = utl_get_au_grab_entity_fd(p_au_rx_info->chipid, p_au_rx_info->dev_no, 0/*needless*/, NULL);

	p_audcap_in = ((AUDCAP_INTERNAL_PARAM *)acap_param->param)->p_param_in;
	if (p_audcap_in == NULL) {
		GMLIB_ERROR_P("<acap in param is not set! out_subid(%d) >\n", acap_ch);
		goto err_exit;
	}
	p_priv_info = ((AUDCAP_INTERNAL_PARAM *)acap_param->param)->p_priv_info;
	if (p_priv_info == NULL) {
		GMLIB_ERROR_P("<acap priv_info is NULL! out_subid(%d) >\n", acap_ch);
		goto err_exit;
	}

	if (dif_get_next_member_id(hd_group, line_idx, member_idx,
				   &next_devid, &next_in_sub_id, &next_out_sub_id) != HD_OK) {
		GMLIB_ERROR_P("<acap: dif_get_next_member_id fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (bd_get_dev_baseid(next_devid) != HD_DAL_AUDIOENC_BASE) {
		GMLIB_ERROR_P("Wrong next device(%#x), this(%#x)\n", next_devid, acap_param->dev.device_subid);
		goto err_exit;
	}
	auenc_dataout_id = next_in_sub_id;

	if (get_acap_ch_count(acap_dev, &ch_count) != HD_OK) {
		GMLIB_ERROR_P("<acap: get_acap_ch_count fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (get_acap_tracks_setting(p_audcap_in->mode, &tracks) != HD_OK) {
		GMLIB_ERROR_P("<acap: get_acap_tracks_setting fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (get_acap_vpd_enc_type(hd_group, line_idx, &vpd_enc_type) != HD_OK) {
		GMLIB_ERROR_P("<acap: get_acap_vpd_enc_type fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	if (dif_get_audio_record_bind_num(hd_group, acap_param->dev.p_bind, &bind_count) != HD_OK) {
		GMLIB_ERROR_P("<acap: dif_get_audio_record_bind_num fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}

	acap = pif_lookup_entity_buf(VPD_AU_GRAB_ENTITY_TYPE, acap_fd, group->encap_buf);
	if (acap != NULL) {
		*len = 0;
		goto set_param; // set parameters for another channel
	}

	*len = pif_set_EntityBuf(VPD_AU_GRAB_ENTITY_TYPE, &acap, group->encap_buf);
	if (acap == NULL) {
		GMLIB_ERROR_P("<acap: NULL entity buffer!>\n");
		goto err_exit;
	}
	acap->sn = acap_fd;
	acap->ap_bindfd = BD_SET_PATH(acap_param->dev.device_baseid + acap_param->dev.device_subid,
				      acap_param->dev.in_subid + HD_IN_BASE,
				      acap_param->dev.out_subid + HD_OUT_BASE);
	memset(acap->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		p_out_pool = &acap_param->param->p_dev_config->data_pool[i];
		if (p_out_pool == NULL) {
			GMLIB_ERROR_P("<acap out pool is not set! out_subid(%d) >\n", acap_ch);
			goto err_exit;
		}
		if (p_out_pool->mode == HD_AUDIOCAP_POOL_DISABLE) {
			acap->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->mode == HD_AUDIOCAP_POOL_ENABLE) {
			acap->pool[i].enable = TRUE;
			acap->pool[i].ddr_id = p_out_pool->ddr_id;
			acap->pool[i].size = p_out_pool->frame_sample_size * ch_count * tracks;
			acap->pool[i].count_decuple = p_out_pool->counts;
			acap->pool[i].max_counts = p_out_pool->max_counts / 10;
			acap->pool[i].min_counts = p_out_pool->min_counts / 10;
		} else { //HD_AUDIOCAP_POOL_AUTO
			if (i == 0) {
				acap->pool[i].enable = TRUE;
				acap->pool[i].ddr_id = 0;
				acap->pool[i].size = p_audcap_in->frame_sample * (p_audcap_in->sample_bit / 8) * ch_count * tracks;
				acap->pool[i].count_decuple = 200;
				acap->pool[i].max_counts = 20;
				acap->pool[i].min_counts = 0;
			} else {
				acap->pool[i].enable = FALSE;
				continue;
			}
		}

		acap->pool[i].type = VPD_AU_ENC_AU_GRAB_OUT_POOL;
		snprintf(acap->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(acap->pool[i].type), acap_fd);
		acap->pool[i].width = 0;
		acap->pool[i].height = 0;
		acap->pool[i].fps = 30;
		acap->pool[i].vch = acap_ch;
		acap->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		acap->pool[i].is_duplicate = 0;
		acap->pool[i].dst_fmt = VPD_BUFTYPE_DATA;
		acap->pool[i].win_size = 0;

		GMLIB_L2_P("set_acap_unit i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, acap->pool[i].enable, acap->pool[i].name, acap->pool[i].type, acap->pool[i].ddr_id, acap->pool[i].size,
			   acap->pool[i].count_decuple, acap->pool[i].max_counts, acap->pool[i].dst_fmt, acap->pool[i].is_duplicate);
	}


set_param:
	acap_entity = (vpd_au_grab_entity_t *) acap->e;


	if (acap_entity->ch_bmp == (UINT)GMLIB_NULL_VALUE) {
		//first channel
		acap_entity->ch_bmp = ADD_AU_CH_TO_BMP(0, acap_ch);
	} else {
		if (IS_AU_CH_EXISTED(acap_entity->ch_bmp, acap_ch)) {
			goto done_exit;
		}
		//other channel(come here for the second time)
		acap_entity->ch_bmp |= ADD_AU_CH_TO_BMP(0, acap_ch);
	}

	acap_entity->ch_bmp2 = 0;
	acap_entity->sample_rate = p_audcap_in->sample_rate;
	acap_entity->sample_bits = p_audcap_in->sample_bit;
	acap_entity->tracks = tracks;
	acap_entity->frame_samples = p_audcap_in->frame_sample;

	one_ch_buf_size = acap_entity->frame_samples * (acap_entity->sample_bits / 8) * tracks;

	if (acap_entity->pch_count == GMLIB_NULL_VALUE) {
		acap_entity->pch_count = 0;
	}
	i = acap_entity->pch_count;
	acap_entity->pch_param[i].pch = auenc_dataout_id;
	acap_entity->pch_param[i].enc_type = vpd_enc_type;
	acap_entity->pch_param[i].vch = p_priv_info->port[acap_ch].vch;
	acap_entity->pch_param[i].buf_size = one_ch_buf_size;
	acap_entity->pch_param[i].buf_offset = one_ch_buf_size * i;
	acap_entity->pch_count ++;

done_exit:
	SET_ENTITY_FEATURE(acap->feature, VPD_FLAG_OUTBUF_MAX_CNT);

	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, acap);

err_exit:
	return -1;

null_exit:
	return 0;
}
