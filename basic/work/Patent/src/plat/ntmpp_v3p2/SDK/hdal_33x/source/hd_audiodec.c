/**
	@brief Header file of audio decode module.\n
	This file contains the functions which is related to video output in the chip.

	@file audiodec.c

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
#include "hd_type.h"
#include "hdal.h"
#include "bind.h"
#include "dif.h"
#include "audiodec.h"
#include "logger.h"
#include "kflow_audioio.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define AUDIODEC_CHECK_PATH_ID(path_id)                                            \
	do {                                                                           \
		if (audiodec_check_path_id_p(path_id) != HD_OK) {                          \
			HD_AUDIODEC_ERR("Error self_id(%d)\n", (self_id));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIODEC_CHECK_OUT_ID(_out_id)                                             \
	do {                                                                           \
		HD_DAL self_id = HD_GET_DEV(_out_id);                                      \
		HD_IO out_id = HD_GET_OUT(_out_id);                                        \
		if (AUDDEC_DEVID(self_id) > MAX_AUDDEC_DEV || ((int)AUDDEC_DEVID(self_id)) < 0) { \
			HD_AUDIODEC_ERR("Error self_id(%#x) from out_id(%#x)\n", (self_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
		if ((out_id) >= HD_OUT_BASE && (out_id) < (HD_OUT_BASE + MAX_AUDDEC_PORT)) {        \
		} else {                                                                   \
			HD_AUDIODEC_ERR("Error out_port(%#x) from out_id(%#x)\n", (out_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIODEC_CHECK_P_PARAM(p_param)                                            \
	do {                                                                           \
		if ((p_param) == NULL) {                                                   \
			HD_AUDIODEC_ERR("Error p_param(%p)\n", (p_param));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern AUDDEC_INTERNAL_PARAM auddec_dev_param[MAX_AUDDEC_DEV];
extern KFLOW_AUDIOIO_HW_SPEC_INFO audioio_hw_spec_info;


/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
UINT32 audiodec_max_device_nr;
UINT32 audiodec_max_device_in;
UINT32 audiodec_max_device_out;
AUDDEC_INFO audiodec_info[MAX_AUDDEC_DEV];
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
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

HD_RESULT hd_audiodec_init(VOID)
{
	HD_RESULT ret;

	audiodec_max_device_nr = 1;
	audiodec_max_device_in = MAX_AUDDEC_PORT;
	audiodec_max_device_out = MAX_AUDDEC_PORT;
	audiodec_init_p();
	ret = bd_device_init(HD_DAL_AUDIODEC_BASE, audiodec_max_device_nr, audiodec_max_device_in, audiodec_max_device_out);
	HD_AUDIODEC_FLOW("hd_audiodec_init\n");
	return ret;
}

HD_RESULT hd_audiodec_uninit(VOID)
{
	HD_RESULT ret;

	ret = bd_device_uninit(HD_DAL_AUDIODEC_BASE);

	audiodec_uninit_p();
	HD_AUDIODEC_FLOW("hd_audiodec_uninit\n");
	return ret;
}

HD_RESULT hd_audiodec_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT ret;

	if (p_path_id == NULL) {
		HD_AUDIODEC_ERR(" The p_path_id is NULL.\n");
		return HD_ERR_PARAM;
	}

	if (bd_get_dev_baseid(self_id) != HD_DAL_AUDIODEC_BASE) {
		HD_AUDIODEC_ERR(" The device of path_id(%#x) is not audiodec.\n", *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	if (in_dev_id != self_id) {
		HD_AUDIODEC_ERR(" in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}
	ret = bd_device_open(self_id, out_id, in_id, p_path_id);

ext:
	HD_AUDIODEC_FLOW("hd_audiodec_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
			 dif_return_dev_string(*p_path_id), *p_path_id, self_id, _out_id, _in_id);
	return ret;
}

HD_RESULT hd_audiodec_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_RESULT ret;

	AUDIODEC_CHECK_PATH_ID(path_id);
	ret = bd_device_close(path_id);
	HD_AUDIODEC_FLOW("hd_audiodec_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audiodec_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	INT dev_idx = AUDDEC_DEVID(self_id);
	INT outport_idx = AUDDEC_OUTID(out_id);
	AUDDEC_INFO_PRIV *p_auddec_priv;

	AUDIODEC_CHECK_OUT_ID(_out_id);
	p_auddec_priv = &audiodec_info[dev_idx].priv;

	ret = bd_bind(self_id, out_id, dest_id, in_id);
	if (ret == HD_OK) {
		p_auddec_priv->port[outport_idx].state = AUDDEC_STATE_BOUND;
	}
	HD_AUDIODEC_FLOW("hd_audiodec_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(in_id));
	return ret;
}

HD_RESULT hd_audiodec_unbind(HD_OUT_ID _out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	INT dev_idx = AUDDEC_DEVID(self_id);
	INT outport_idx = AUDDEC_OUTID(out_id);
	AUDDEC_INFO_PRIV *p_auddec_priv;

	AUDIODEC_CHECK_OUT_ID(_out_id);

	p_auddec_priv = &audiodec_info[dev_idx].priv;

	ret = bd_unbind(self_id, out_id);
	if (ret == HD_OK) {
		p_auddec_priv->port[outport_idx].state = AUDDEC_STATE_UNBOUND;
	}
	HD_AUDIODEC_FLOW("hd_audiodec_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id));
	return ret;
}

HD_RESULT hd_audiodec_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIODEC_CHECK_PATH_ID(path_id);
	ret = bd_start_list(&path_id, 1);
	HD_AUDIODEC_FLOW("hd_audiodec_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audiodec_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIODEC_CHECK_PATH_ID(path_id);
	ret = bd_stop_list(&path_id, 1);
	HD_AUDIODEC_FLOW("hd_audiodec_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audiodec_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	if (path_id == NULL) {
		HD_AUDIODEC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		ret = audiodec_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}
	ret = bd_start_list(path_id, nr);
ext:
	return ret;
}

HD_RESULT hd_audiodec_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	if (path_id == NULL) {
		HD_AUDIODEC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		ret = audiodec_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}

	ret = bd_stop_list(path_id, nr);

ext:
	return ret;
}


HD_RESULT hd_audiodec_set(HD_PATH_ID path_id, HD_AUDIODEC_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = AUDDEC_DEVID(self_id);
	INT inport_idx, ret, i;
	AUDDEC_INFO *p_auddec_info;
	CHAR msg_string[256];

	//check parameters
	AUDIODEC_CHECK_PATH_ID(path_id);
	AUDIODEC_CHECK_P_PARAM(p_param);

	ret = check_audiodec_params_range(id, p_param, msg_string, 256);
	if (ret < 0) {
		HD_AUDIODEC_ERR("Wrong value. %s\n", msg_string);
		return HD_ERR_PARAM;
	}

	//store user values
	p_auddec_info = &audiodec_info[dev_idx];
	inport_idx = AUDDEC_INID(in_id);
	switch (id) {
	case HD_AUDIODEC_PARAM_DEVCOUNT:
		HD_AUDIODEC_ERR("audiodec doesn't support set opertion for HD_AUDIODEC_PARAM_DEVCOUNT param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIODEC_PARAM_SYSCAPS:
		HD_AUDIODEC_ERR("audiodec doesn't support set opertion for HD_AUDIODEC_PARAM_SYSCAPS param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIODEC_PARAM_PATH_CONFIG:
		memcpy(&p_auddec_info->port[inport_idx].path_config, p_param, sizeof(HD_AUDIODEC_PATH_CONFIG));
		auddec_dev_param[dev_idx].port[inport_idx].p_path_config = &p_auddec_info->port[inport_idx].path_config;
		HD_AUDIODEC_FLOW("hd_audiodec_set(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id,  p_auddec_info->port[inport_idx].path_config.max_mem.codec_type,
				 p_auddec_info->port[inport_idx].path_config.max_mem.sample_rate,
				 p_auddec_info->port[inport_idx].path_config.max_mem.sample_bit,
				 p_auddec_info->port[inport_idx].path_config.max_mem.mode);
		for (i = 0; i < HD_AUDIODEC_MAX_DATA_TYPE; i++) {
			p_auddec_info->port[inport_idx].path_config.data_pool[i].max_counts = p_auddec_info->port[inport_idx].path_config.data_pool[i].counts;
			p_auddec_info->port[inport_idx].path_config.data_pool[i].min_counts = 0;
			HD_AUDIODEC_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					 i, p_auddec_info->port[inport_idx].path_config.data_pool[i].ddr_id,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].counts,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].max_counts,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].min_counts,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].mode);
		}
		break;

	case HD_AUDIODEC_PARAM_IN:
		memcpy(&p_auddec_info->port[inport_idx].dec_in, p_param, sizeof(HD_AUDIODEC_IN));
		auddec_dev_param[dev_idx].port[inport_idx].p_dec_in = &p_auddec_info->port[inport_idx].dec_in;
		HD_AUDIODEC_FLOW("hd_audiodec_set(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id,  p_auddec_info->port[inport_idx].dec_in.codec_type,
				 p_auddec_info->port[inport_idx].dec_in.sample_rate,
				 p_auddec_info->port[inport_idx].dec_in.sample_bit,
				 p_auddec_info->port[inport_idx].dec_in.mode);
		break;
	default:
		break;
	}

	return HD_OK;
}

HD_RESULT hd_audiodec_get(HD_PATH_ID path_id, HD_AUDIODEC_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = AUDDEC_DEVID(self_id);
	INT inport_idx;
	UINT32 i;
	AUDDEC_INFO *p_auddec_info;

	//check parameters
	AUDIODEC_CHECK_PATH_ID(path_id);
	AUDIODEC_CHECK_P_PARAM(p_param);

	//retrieve user values
	p_auddec_info = &audiodec_info[dev_idx];
	inport_idx = AUDDEC_INID(in_id);
	switch (id) {
	case HD_AUDIODEC_PARAM_DEVCOUNT: {
		HD_DEVCOUNT *p_devcount = (HD_DEVCOUNT *) p_param;
		p_devcount->max_dev_count = 1;
		break;
	}

	case HD_AUDIODEC_PARAM_SYSCAPS: {
		HD_AUDIODEC_SYSCAPS *p_aucap = (HD_AUDIODEC_SYSCAPS *) p_param;
		p_aucap->dev_id = self_id;
		p_aucap->chip_id = 0;
		p_aucap->max_in_count = audiodec_max_device_in;
		p_aucap->max_out_count = audiodec_max_device_out;
		p_aucap->dev_caps = HD_CAPS_LISTFUNC;
		for (i = 0; i < audiodec_max_device_in; i ++) {
			p_aucap->in_caps[i] = HD_AUDIODEC_CAPS_PCM
					      | HD_AUDIODEC_CAPS_ULAW
					      | HD_AUDIODEC_CAPS_ALAW
					      | HD_AUDIO_CAPS_BITS_16
					      | HD_AUDIO_CAPS_CH_MONO;
		}
		break;
	}

	case HD_AUDIODEC_PARAM_PATH_CONFIG:
		memcpy(p_param, &p_auddec_info->port[inport_idx].path_config, sizeof(HD_AUDIODEC_PATH_CONFIG));
		HD_AUDIODEC_FLOW("hd_audiodec_get(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id,  p_auddec_info->port[inport_idx].path_config.max_mem.codec_type,
				 p_auddec_info->port[inport_idx].path_config.max_mem.sample_rate,
				 p_auddec_info->port[inport_idx].path_config.max_mem.sample_bit,
				 p_auddec_info->port[inport_idx].path_config.max_mem.mode);
		for (i = 0; i < HD_AUDIODEC_MAX_DATA_TYPE; i++) {
			HD_AUDIODEC_FLOW("    idx(%u) ddr(%d) count(%u) max/min(%u,%u) mode(%#x)\n",
					 i,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].ddr_id,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].counts,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].max_counts,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].min_counts,
					 p_auddec_info->port[inport_idx].path_config.data_pool[i].mode);

		}
		break;

	case HD_AUDIODEC_PARAM_IN:
		memcpy(p_param, &p_auddec_info->port[inport_idx].dec_in, sizeof(HD_AUDIODEC_IN));
		HD_AUDIODEC_FLOW("hd_audiodec_get(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id,  p_auddec_info->port[inport_idx].dec_in.codec_type,
				 p_auddec_info->port[inport_idx].dec_in.sample_rate,
				 p_auddec_info->port[inport_idx].dec_in.sample_bit,
				 p_auddec_info->port[inport_idx].dec_in.mode);
		break;
	default:
		break;
	}

	return HD_OK;
}

extern HD_RESULT _hd_common_mem_cache_sync_dma_to_device(void *virt_addr, unsigned int size);

HD_RESULT hd_audiodec_push_in_buf(HD_PATH_ID path_id, HD_AUDIO_BS *p_in_audio_bitstream, HD_AUDIO_FRAME *p_user_out_audio_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_AUDIODEC_SEND_LIST send_list = {0};
	VOID *send_list_va = 0;

	ret = dif_check_flow_mode(self_id, out_id);
	if (ret == HD_ERR_ALREADY_BIND) {
		send_list_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE,
						  p_in_audio_bitstream->phy_addr,
						  p_in_audio_bitstream->size);
		if (send_list_va == NULL) {
			goto exit;
		}
		send_list.path_id = path_id;
		send_list.user_bs.p_user_buf = (CHAR *)send_list_va;
		send_list.user_bs.user_buf_size = p_in_audio_bitstream->size;
		if ((ret = _hd_common_mem_cache_sync_dma_to_device(send_list_va, p_in_audio_bitstream->size)) != HD_OK) {
			goto exit;
		}
		if ((ret = hd_audiodec_send_list(&send_list, 1, wait_ms)) != HD_OK) {
			goto exit;
		}
	} else if (ret == HD_ERR_NOT_AVAIL) {
		HD_AUDIODEC_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
	} else {
		HD_AUDIODEC_ERR("hd_audiodec_push_in_buf fail, ret(%d)\n", ret);
	}

exit:
	if (send_list_va) {
		if (hd_common_mem_munmap(send_list_va, p_in_audio_bitstream->size) != HD_OK) {
			HD_AUDIODEC_ERR("munmap fail\n");
		}
	}
	return ret;
}


HD_RESULT hd_audiodec_pull_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_AUDIODEC_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiodec_release_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_AUDIODEC_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiodec_send_list(HD_AUDIODEC_SEND_LIST *p_audiodec_bs, UINT32 nr, INT32 wait_ms)
{
	UINT i;
	if (p_audiodec_bs == NULL) {
		HD_AUDIODEC_ERR(" The p_audiodec_bs is NULL.\n");
		return HD_ERR_PARAM;
	}

	for (i = 0; i < nr; i++) {
		HD_DAL self_id = HD_GET_DEV(p_audiodec_bs[i].path_id);
		AUDIODEC_CHECK_PATH_ID(p_audiodec_bs[i].path_id);
	}

	for (i = 0; i < nr; i++) {
		if ((((unsigned long)p_audiodec_bs[i].user_bs.p_user_buf) % audioio_hw_spec_info.out_align) != 0) {
			HD_AUDIODEC_ERR("The bitstream address is not %d-bytes alignment.\n", audioio_hw_spec_info.out_align);
			return HD_ERR_PARAM;
		}
	}

#if 0
	for (i = 0; i < nr; i++) {
		HD_AUDIODEC_USER_BS *p_user_bs = &p_audiodec_bs[i].user_bs;
		char *buf = ((char *)p_user_bs->p_user_buf);
		int dbg_bs_length = p_user_bs->user_buf_size;
		HD_AUDIODEC_FLOW("audio_send_list i(%d) ptr(%p) len(%d) data: %02x%02x%02x%02x %02x%02x%02x%02x "
				 "%02x%02x%02x%02x %02x%02x%02x%02x ... %02x%02x%02x%02x %02x%02x%02x%02x\n",
				 i, buf, dbg_bs_length,
				 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
				 buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],
				 buf[dbg_bs_length - 8], buf[dbg_bs_length - 7], buf[dbg_bs_length - 6], buf[dbg_bs_length - 5],
				 buf[dbg_bs_length - 4], buf[dbg_bs_length - 3], buf[dbg_bs_length - 2], buf[dbg_bs_length - 1]);
	}
#endif

	return audio_send_list(p_audiodec_bs, nr, wait_ms);
}

