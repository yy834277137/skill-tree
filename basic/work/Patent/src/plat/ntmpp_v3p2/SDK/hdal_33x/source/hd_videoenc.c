/**
	@brief Header file of video encode module.\n
	This file contains the functions which is related to video output in the chip.

	@file videoenc.c

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
#include <pthread.h>
#include "hd_type.h"
#include "hdal.h"
#include "bind.h"
#include "dif.h"
#include "pif.h"
#include "videoenc.h"
#include "videoout.h"
#include "logger.h"
#include "cif_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define VIDEOENC_CHECK_SELF_ID(self_id)                                            \
	do {                                                                           \
		if ((self_id) >= (HD_DAL_VIDEOENC_BASE) && (self_id) < (HD_DAL_VIDEOENC_BASE + MAX_VDOENC_DEV)) { \
		} else {                                                                   \
			HD_VIDEOENC_ERR("Error self_id(%d)\n", (self_id));                     \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define VIDEOENC_CHECK_CTRL_ID(ctrl_id)                                            \
	do {                                                                           \
		if ((ctrl_id) != HD_CTRL) {                                                \
		} else {                                                                   \
			HD_VIDEOENC_ERR("Error ctrl_id(%d)\n", (ctrl_id));                     \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define VIDEOENC_CHECK_IN_ID(in_id)                                                \
	do {                                                                           \
		if ((in_id) >= HD_IN_BASE && (in_id) < (HD_IN_BASE + HD_VIDEOENC_MAX_IN)) { \
		} else {                                                                   \
			HD_VIDEOENC_ERR("Error in_id(%d)\n", (in_id));                         \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define VIDEOENC_CHECK_OUT_ID(out_id, is_ctrlport_valid)                             \
	do {                                                                             \
		if ((out_id) >= HD_OUT_BASE && (out_id) < (HD_OUT_BASE + HD_VIDEOENC_MAX_OUT)) { \
		} else if (is_ctrlport_valid && out_id == HD_CTRL) {                         \
		} else {                                                                     \
			HD_VIDEOENC_ERR("Error out_id(%d)\n", (out_id));                         \
			return HD_ERR_NG;                                                        \
		}                                                                            \
	} while (0)

#define VIDEOENC_CHECK_P_PARAM(p_param)                                            \
	do {                                                                           \
		if ((p_param) == NULL) {                                                   \
			HD_VIDEOENC_ERR("Error p_param(%p)\n", (p_param));                     \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define VIDEOENC_CHECK_VIDEO_FRAME_SIZE(p_video_frame)     \
	do {                                                   \
		if (p_video_frame == NULL) {                       \
			ret = HD_ERR_PARAM;                      \
			goto exit;                                     \
		}                                                  \
	} while (0)

#define ISVALID_VDOENC_STAMP_INID(in_id) ((in_id) >= (HD_IN_BASE + HD_STAMP_BASE) && \
                                          (in_id) < (HD_IN_BASE + HD_STAMP_BASE + VDOENC_STAMP_NU))
#define ISVALID_VDOENC_MASK_INID(in_id) ((in_id) >= (HD_IN_BASE + HD_MASK_BASE) && \
                                         (in_id) < (HD_IN_BASE + HD_MASK_BASE + VDOENC_MASK_NU))
 #define ALIGN4_DOWN(a)              ALIGN_ROUND_4(a)
#define ALIGN2_DOWN(a)              ALIGN_FLOOR(a, 2)
#define ENCODE_OSG_W_ALIGN	4
#define ENCODE_OSG_H_ALIGN	2
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern VDOENC_INTERNAL_PARAM vdoenc_dev_param[MAX_VDOENC_DEV];
extern UINT16 osg_select[MAX_VDOENC_DEV][HD_VIDEOENC_MAX_OUT] ;
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
static UINT32 videoenc_max_device_nr;
static UINT32 videoenc_max_device_in;
static UINT32 videoenc_max_device_out;
VDOENC_INFO videoenc_info[MAX_VDOENC_DEV];
static pthread_mutex_t videoenc_mutex_lock = PTHREAD_MUTEX_INITIALIZER;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_videoenc_init(VOID)
{
	HD_RESULT ret = HD_OK;

	videoenc_max_device_nr = MAX_VDOENC_DEV;
	videoenc_max_device_in = HD_VIDEOENC_MAX_IN;
	videoenc_max_device_out = HD_VIDEOENC_MAX_OUT;
	videoenc_init();
	ret = bd_device_init(HD_DAL_VIDEOENC_BASE, videoenc_max_device_nr, videoenc_max_device_in, videoenc_max_device_out);
	HD_VIDEOENC_FLOW("hd_videoenc_init\n");
	return ret;
}

HD_RESULT hd_videoenc_uninit(VOID)
{
	HD_RESULT ret;

	ret = bd_device_uninit(HD_DAL_VIDEOENC_BASE);

	videoenc_uninit();
	HD_VIDEOENC_FLOW("hd_videoenc_uninit\n");
	return ret;
}

HD_RESULT hd_videoenc_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL out_dev_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT ret = HD_OK;
	INT dev_idx = VDOENC_DEVID(out_dev_id);
	INT port_idx = BD_GET_OUT_SUBID(out_id);
	VDOENC_INFO_PRIV *p_enc_priv = NULL;

	if (p_path_id == NULL) {
		HD_VIDEOENC_ERR(" The p_path_id is NULL.\n");
		return HD_ERR_PARAM;
	}

	VIDEOENC_CHECK_SELF_ID(out_dev_id);
	VIDEOENC_CHECK_OUT_ID(out_id, FALSE);

	pthread_mutex_lock(&videoenc_mutex_lock);

	if (bd_get_dev_baseid(out_dev_id) != HD_DAL_VIDEOENC_BASE) {
		HD_VIDEOENC_ERR(" The device of path_id(%#x) is not videoenc.\n", *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}
	if (in_dev_id != out_dev_id) {
		HD_VIDEOENC_ERR("videoenc: in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}

	if (in_id <= HD_IN_MAX) {
		if (in_id != out_id) {
			HD_VIDEOENC_ERR("videoenc: in(%#x) and out(%#x) aren't the same port.\n", _in_id, _out_id);
			ret = HD_ERR_PARAM;
			goto ext;
		}

		/* check and set state */
		p_enc_priv = &videoenc_info[dev_idx].priv[port_idx];
		if ((p_enc_priv->push_state == HD_PATH_ID_OPEN) ||
		    (p_enc_priv->pull_state == HD_PATH_ID_OPEN)) {
			if ((ret = bd_get_already_open_pathid(out_dev_id, out_id, in_id, p_path_id)) != HD_OK) {
				goto ext;
			} else {
				ret = HD_ERR_ALREADY_OPEN;
				goto ext;
			}
		} else if (p_enc_priv->push_state == HD_PATH_ID_ONGOING ||
			   p_enc_priv->pull_state == HD_PATH_ID_ONGOING) {
			HD_VIDEOENC_ERR("videoenc: Fail to open, path_id(%#x) is busy. state(%d,%d)\n",
					GET_VENC_PATHID(out_dev_id, _in_id, _out_id),
					p_enc_priv->push_state, p_enc_priv->pull_state);
			ret = HD_ERR_STATE;
			goto ext;
		} else {
			p_enc_priv->push_state = HD_PATH_ID_ONGOING;
			p_enc_priv->pull_state = HD_PATH_ID_ONGOING;
		}

		ret = bd_device_open(out_dev_id, out_id, in_id, p_path_id);
		if (ret != HD_OK) {
			HD_VIDEOENC_ERR("bd_device_open fail\n");
			goto enc_exit;
		}
		p_enc_priv->path_id = *p_path_id;

enc_exit:
		p_enc_priv->push_state = HD_PATH_ID_OPEN;
		p_enc_priv->pull_state = HD_PATH_ID_OPEN;
		HD_VIDEOENC_FLOW("hd_videoenc_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u), ret(%d)\n",
				 dif_return_dev_string(*p_path_id), *p_path_id, out_dev_id, out_id, in_id, ret);
	} else if (in_id > HD_STAMP_BASE) {
		HD_IO tmp_out_id;
		INT outport_idx;

		*p_path_id = 0;
		*p_path_id = BD_SET_PATH(out_dev_id, in_id, out_id);
		HD_OSG_FLOW("hd_videoenc_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
			    dif_return_dev_string(*p_path_id), *p_path_id, out_dev_id, out_id, in_id);

		tmp_out_id = HD_GET_OUT(*p_path_id);
		outport_idx = VDOENC_OUTID(tmp_out_id);
		p_enc_priv = &videoenc_info[dev_idx].priv[outport_idx];
	} else {
		HD_VIDEOENC_ERR("Unsupport id=%x\n", in_id);
		ret = HD_ERR_IO;
	}

ext:
	if (ret == HD_OK) {
		HD_RESULT init_path_result = HD_OK;
		if (p_enc_priv == NULL) {
			HD_VIDEOENC_ERR("Invalid encoder private data.\n");
			ret = HD_ERR_NG;
		} else if (p_enc_priv->path_init_count == 0) {
			init_path_result = videoenc_init_path(*p_path_id);
			if (init_path_result != HD_OK) {
				HD_VIDEOENC_ERR("videoenc_init_path fail\n");
				ret = init_path_result;
			}
		}
		if (init_path_result == HD_OK) {
			p_enc_priv->path_init_count ++;
		}
	}
	pthread_mutex_unlock(&videoenc_mutex_lock);

	return ret;
}

HD_RESULT hd_videoenc_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_RESULT ret = HD_OK;
	HD_IO in_id = HD_GET_IN(path_id);
	VDOENC_INFO_PRIV *p_enc_priv = NULL;
	INT dev_idx = bd_get_dev_subid(self_id);
	INT port_idx = BD_GET_IN_SUBID(in_id);

	pthread_mutex_lock(&videoenc_mutex_lock);

	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEOENC_BASE) {
		HD_VIDEOENC_ERR(" The device of path_id(%#x) is not videoenc.\n", path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}
	if (in_id <= HD_IN_MAX) {
		if (port_idx >= HD_VIDEOENC_MAX_IN) {
			HD_VIDEOENC_ERR("The port of this path is out-of-bounds, path_id(%#x) port(%d>=%d)\n", path_id,
					port_idx, HD_VIDEOENC_MAX_IN);
			ret = HD_ERR_PATH;
			goto ext;
		}
		/* check and set state */
		p_enc_priv = &videoenc_info[dev_idx].priv[port_idx];
		if ((p_enc_priv->push_state == HD_PATH_ID_CLOSED) ||
		    (p_enc_priv->pull_state == HD_PATH_ID_CLOSED)) {
			ret = HD_ERR_NOT_OPEN;
			goto ext;
		} else if (p_enc_priv->push_state == HD_PATH_ID_ONGOING ||
			   p_enc_priv->pull_state == HD_PATH_ID_ONGOING) {
			HD_VIDEOENC_ERR("Fail to close, path_id(%#x) is busy.  state(%d,%d)\n", path_id,
					p_enc_priv->push_state, p_enc_priv->pull_state);
			ret = HD_ERR_STATE;
			goto ext;
		} else {
			p_enc_priv->push_state = HD_PATH_ID_ONGOING;
			p_enc_priv->pull_state = HD_PATH_ID_ONGOING;
		}

		ret = bd_device_close(path_id);
		if (ret == HD_OK) {
			videoenc_close(path_id);
		}
		p_enc_priv->push_state = HD_PATH_ID_CLOSED;
		p_enc_priv->pull_state = HD_PATH_ID_CLOSED;

		HD_VIDEOENC_FLOW("hd_videoenc_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	} else if (ISVALID_VDOENC_STAMP_INID(in_id)) {
		HD_IO out_id = HD_GET_OUT(path_id);
		INT outport_idx = VDOENC_OUTID(out_id);
		ret = videoenc_set_osg_win(path_id, FALSE);
		p_enc_priv = &videoenc_info[dev_idx].priv[outport_idx];
		HD_OSG_FLOW("hd_videoenc_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	} else if (ISVALID_VDOENC_MASK_INID(in_id)) {
		HD_IO out_id = HD_GET_OUT(path_id);
		INT outport_idx = VDOENC_OUTID(out_id);
		ret = videoenc_set_mask_mosaic(path_id, FALSE);
		p_enc_priv = &videoenc_info[dev_idx].priv[outport_idx];
		HD_OSG_FLOW("hd_videoenc_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	} else {
		HD_VIDEOENC_ERR("Unsupport set path id(%x)\n", path_id);
		ret = HD_ERR_PATH;
	}

	if (p_enc_priv == NULL) {
		HD_VIDEOENC_ERR("Invalid encoder private data.\n");
		ret = HD_ERR_NG;
		goto ext;
	}
	p_enc_priv->path_init_count --;
	if (p_enc_priv->path_init_count == 0) {
		videoenc_uninit_path(path_id);
	}
ext:
	pthread_mutex_unlock(&videoenc_mutex_lock);
	return ret;
}

HD_RESULT hd_videoenc_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);

	ret = bd_bind(self_id, out_id, dest_id, in_id);
	HD_VIDEOENC_FLOW("hd_videoenc_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(in_id));
	return ret;
}

HD_RESULT hd_videoenc_unbind(HD_OUT_ID _out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);

	ret = bd_unbind(self_id, out_id);
	HD_VIDEOENC_FLOW("hd_videoenc_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id));
	return ret;
}

HD_RESULT hd_videoenc_start(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = VDOENC_DEVID(self_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	HD_VIDEOENC_OUT *enc_param;
	HD_VIDEOENC_IN *enc_in;
	CHAR msg_string[256];
	INT ret;


	VIDEOENC_CHECK_SELF_ID(self_id);
	VIDEOENC_CHECK_OUT_ID(out_id, FALSE);

	enc_in = &videoenc_info[dev_idx].port[outport_idx].enc_in_dim;
	enc_param = &videoenc_info[dev_idx].port[outport_idx].enc_out_param;
	if (in_id <= HD_IN_MAX) {
		ret = verify_param_HD_VIDEOENC_IN(enc_in, enc_param, msg_string, 256);
		if (ret < 0) {
			HD_VIDEOENC_ERR("Wrong value. %s, id=%d\n", msg_string, HD_VIDEOENC_PARAM_IN);
			return HD_ERR_PARAM;
		}
	}
	return (hd_videoenc_start_list(&path_id, 1));
}

HD_RESULT hd_videoenc_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret = HD_OK;
	HD_IO in_id, out_id;
	UINT32 i;
	HD_PATH_ID this_path_id;
	HD_DAL self_id;
	INT dev_idx, outport_idx;
	HD_VIDEOENC_OUT *enc_param;
	HD_VIDEOENC_IN *enc_in;
	HD_VDOENC_STAMP_PARAM *enc_stamp;
	CHAR msg_string[256];
	INT ret_val;

	if (path_id == NULL) {
		HD_VIDEOENC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		this_path_id = path_id[i];
		self_id = HD_GET_DEV(this_path_id);
		out_id = HD_GET_OUT(this_path_id);
		in_id = HD_GET_IN(this_path_id);
		dev_idx = VDOENC_DEVID(self_id);
		outport_idx = VDOENC_OUTID(out_id);

		if (in_id <= HD_IN_MAX) {
			VIDEOENC_CHECK_SELF_ID(self_id);
			VIDEOENC_CHECK_OUT_ID(out_id, FALSE);

			enc_in = &videoenc_info[dev_idx].port[outport_idx].enc_in_dim;
			enc_param = &videoenc_info[dev_idx].port[outport_idx].enc_out_param;
			ret_val = verify_param_HD_VIDEOENC_IN(enc_in, enc_param, msg_string, 256);
			if (ret_val < 0) {
				HD_VIDEOENC_ERR("Wrong value. path_id(%#x) %s, id=%d\n",
						this_path_id, msg_string, HD_VIDEOENC_PARAM_IN);
				return HD_ERR_PARAM;
			}
		}
	}

	ret = bd_start_list(path_id, nr);
	if (ret != HD_OK) {
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		in_id = HD_GET_IN(path_id[i]);
		if (in_id <= HD_IN_MAX) {
			this_path_id = path_id[i];
			self_id = HD_GET_DEV(this_path_id);
			out_id = HD_GET_OUT(this_path_id);
			outport_idx = VDOENC_OUTID(out_id);
			dev_idx = VDOENC_DEVID(self_id);
			vdoenc_dev_param[dev_idx].port[outport_idx].get_bs_method = GET_BS_UNKNOWN;
			HD_VIDEOENC_FLOW("hd_videoenc_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
		} else if (ISVALID_VDOENC_STAMP_INID(in_id)) {
			this_path_id = path_id[i];
			self_id = HD_GET_DEV(this_path_id);
			out_id = HD_GET_OUT(this_path_id);
			outport_idx = VDOENC_OUTID(out_id);
			dev_idx = VDOENC_DEVID(self_id);
			HD_OSG_FLOW("hd_videoenc_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			enc_in = &videoenc_info[dev_idx].port[outport_idx].enc_in_dim;
			enc_stamp = &videoenc_info[dev_idx].port[outport_idx].stamp[VDOENC_STAMP_INID(in_id)];
			ret = verify_param_HD_VIDEOENC_IN_STAMP(enc_in, enc_stamp, msg_string, 256);
			if (ret < 0) {
				HD_VIDEOENC_ERR("Wrong value. %s, id=%d\n",	msg_string, HD_VIDEOENC_PARAM_IN_STAMP_IMG);
				return HD_ERR_PARAM;
			}
			ret = videoenc_set_osg_win(path_id[i], TRUE);
		} else if (ISVALID_VDOENC_MASK_INID(in_id)) {
			HD_OSG_FLOW("hd_videoenc_start:\n	%s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			ret = videoenc_set_mask_mosaic(path_id[i], TRUE);
		}  else {
			HD_VIDEOENC_ERR("Unsupport set path_id[%d] = %x\n", i, path_id[i]);
			ret = HD_ERR_PATH;
		}
	}

ext:
	return ret;
}

HD_RESULT hd_videoenc_stop(HD_PATH_ID path_id)
{
	return (hd_videoenc_stop_list(&path_id, 1));
}

HD_RESULT hd_videoenc_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	HD_IO in_id;
	UINT32 i;

	if (path_id == NULL) {
		HD_VIDEOENC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	ret = bd_stop_list(path_id, nr);
	if (ret != HD_OK) {
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		in_id = HD_GET_IN(path_id[i]);

		if (in_id <= HD_IN_MAX) {
			HD_VIDEOENC_FLOW("hd_videoenc_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			/* do bd_stop_list: SO do nothing */
		} else if (ISVALID_VDOENC_STAMP_INID(in_id)) {
			HD_OSG_FLOW("hd_videoenc_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			ret = videoenc_set_osg_win(path_id[i], FALSE);
		} else if (ISVALID_VDOENC_MASK_INID(in_id)) {
			HD_OSG_FLOW("hd_videoenc_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			ret = videoenc_set_mask_mosaic(path_id[i], FALSE);
		} else {
			HD_VIDEOENC_ERR("Unsupport set path_id[%d] = %x\n", i, path_id[i]);
			ret = HD_ERR_PATH;
		}
	}

ext:
	return ret;
}

INT16 hd_videoenc_get_index_by_buf(UINTPTR p_addr, HD_VDOENC_STAMP_PARAM *stamp_param)
{
	INT16 i;

	if (stamp_param == NULL) {
		HD_VIDEOENC_ERR(" The stamp_param is NULL.\n");
		return HD_ERR_PARAM;
	}
	for (i = 0; i < 32; i++) {
		if (p_addr == stamp_param[i].img.p_addr) {
			return i;
		}
	}
	return -1;
}

HD_RESULT hd_videoenc_get(HD_PATH_ID path_id, HD_VIDEOENC_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx, outport_idx, i;

	//check parameters
	VIDEOENC_CHECK_SELF_ID(self_id);
	if (in_id <= HD_IN_MAX) { // add this to avoid using wrong in port id (because there are encoder port and stamp port)
		switch (id) {
		case HD_VIDEOENC_PARAM_BUFINFO:
		case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
		case HD_VIDEOENC_PARAM_PATH_CONFIG:
			HD_VIDEOENC_ERR("Unsupport 'get' opertaion for this HD_VIDEOENC_PARAM_ID id(%d)\n", id);
			return HD_ERR_NG;

		case HD_VIDEOENC_PARAM_DEVCOUNT:
		case HD_VIDEOENC_PARAM_SYSCAPS:
			//check control port
			break;
		case HD_VIDEOENC_PARAM_IN:
		case HD_VIDEOENC_PARAM_IN_PALETTE_TABLE:
		case HD_VIDEOENC_PARAM_STATUS:
			VIDEOENC_CHECK_IN_ID(in_id);
			break;

		case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
		case HD_VIDEOENC_PARAM_OUT_VUI:
		case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
		case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
		case HD_VIDEOENC_PARAM_OUT_USR_QP:
		case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
		case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
		case HD_VIDEOENC_PARAM_OUT_ROI:
		case HD_VIDEOENC_PARAM_OUT_ROW_RC:
		case HD_VIDEOENC_PARAM_OUT_AQ:
			VIDEOENC_CHECK_OUT_ID(out_id, FALSE);
			break;
		default:
			HD_VIDEOENC_ERR("Unsupport set param id(%d)\n", id);
			return HD_ERR_NG;
		}
		VIDEOENC_CHECK_P_PARAM(p_param);
	}
	dev_idx = VDOENC_DEVID(self_id);
	outport_idx = VDOENC_OUTID(out_id);

	//retrieve user values
	switch (id) {
	case HD_VIDEOENC_PARAM_DEVCOUNT: {
		HD_DEVCOUNT *p_dev_count = (HD_DEVCOUNT *) p_param;
		p_dev_count->max_dev_count = 1;
		HD_VIDEOENC_FLOW("hd_videoenc_get(DEVCOUNT):\n"
				 "    %s path_id(%#x)\n"
				 "        max_count(%u)\n", dif_return_dev_string(path_id), path_id, p_dev_count->max_dev_count);
		break;
	}

	case HD_VIDEOENC_PARAM_SYSCAPS: {
		HD_VIDEOENC_SYSCAPS *p_syscaps = (HD_VIDEOENC_SYSCAPS *) p_param;
		p_syscaps->dev_id = 0;
		p_syscaps->chip_id = 0;
		p_syscaps->max_in_count = videoenc_max_device_in;
		p_syscaps->max_out_count = videoenc_max_device_out;
		p_syscaps->dev_caps = HD_CAPS_PATHCONFIG
				      | HD_CAPS_LISTFUNC;
		for (i = 0; i < HD_VIDEOENC_MAX_IN; i++) {
			p_syscaps->in_caps[i] = HD_VIDEO_CAPS_YUV420
						| HD_VIDEO_CAPS_YUV422
						| HD_VIDEO_CAPS_DIR_ROTATER
						| HD_VIDEO_CAPS_DIR_ROTATE180
						| HD_VIDEO_CAPS_DIR_ROTATEL
						| HD_VIDEO_CAPS_MASK;
		}
		for (i = 0; i < HD_VIDEOENC_MAX_OUT; i++) {
			p_syscaps->out_caps[i] = HD_VIDEOENC_CAPS_JPEG
						 | HD_VIDEOENC_CAPS_H264
						 | HD_VIDEOENC_CAPS_H265;
		}
		p_syscaps->max_in_stamp = 1;
		p_syscaps->max_in_stamp_ex = 1;
		p_syscaps->max_in_mask = 1;
		p_syscaps->max_in_mask_ex = 1;
		break;
	}

	case HD_VIDEOENC_PARAM_IN: {
		INT inport_idx = VDOENC_INID(in_id);
		HD_VIDEOENC_IN *enc_in;

		if (inport_idx >= HD_VIDEOENC_MAX_IN) {
			HD_VIDEOENC_ERR("Unsupport in_id(%d)\n", in_id);
			return HD_ERR_LIMIT;
		}
		enc_in = &videoenc_info[dev_idx].port[inport_idx].enc_in_dim;
		memcpy(p_param, enc_in, sizeof(HD_VIDEOENC_IN));
		HD_VIDEOENC_FLOW("hd_videoenc_get(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        dim(%u,%u) pxlfmt(%#x) dir(%#x)\n",
				 dif_return_dev_string(path_id), path_id, enc_in->dim.w, enc_in->dim.h, enc_in->pxl_fmt, enc_in->dir);
		break;
	}

	case HD_VIDEOENC_PARAM_PATH_CONFIG: {
		HD_VIDEOENC_PATH_CONFIG *path_cfg = &videoenc_info[dev_idx].port[outport_idx].enc_path_cfg;
		memcpy(p_param, path_cfg, sizeof(HD_VIDEOENC_PATH_CONFIG));
		HD_VIDEOENC_FLOW("hd_videoenc_get(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) max_dim(%u,%u) enc_buf_ms(%u) max_br(%u)\n"
				 "        svc_layer(%#x) ltr(%d) rotate(%d) src_out(%d)\n",
				 dif_return_dev_string(path_id), path_id, path_cfg->max_mem.codec_type,
				 path_cfg->max_mem.max_dim.w,
				 path_cfg->max_mem.max_dim.h,
				 path_cfg->max_mem.enc_buf_ms,
				 path_cfg->max_mem.bitrate,
				 path_cfg->max_mem.svc_layer,
				 path_cfg->max_mem.ltr,
				 path_cfg->max_mem.rotate,
				 path_cfg->max_mem.source_output);
		for (i = 0; i < HD_VIDEOENC_MAX_DATA_TYPE; i++) {
			HD_VIDEOENC_FLOW("        idx(%d) ddr(%u) count(%2.1f) max/min(%2.1f,%2.1f) mode(%#x)\n",
					 i, path_cfg->data_pool[i].ddr_id,
					 ((float)path_cfg->data_pool[i].counts) / 10,
					 ((float)path_cfg->data_pool[i].max_counts) / 10,
					 ((float)path_cfg->data_pool[i].min_counts) / 10,
					 path_cfg->data_pool[i].mode);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM: {
		HD_VIDEOENC_OUT *enc_param = &videoenc_info[dev_idx].port[outport_idx].enc_out_param;
		memcpy(p_param, enc_param, sizeof(HD_VIDEOENC_OUT));
		if ((enc_param->codec_type == HD_CODEC_TYPE_H264) ||
		    (enc_param->codec_type == HD_CODEC_TYPE_H265)) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_ENC_PARAM):\n"
					 "    %s path_id(%#x)\n"
					 "        codec(%#x) gop(%u) chrm_qp_idx(%u) sec_chrm_qp_idx(%u)\n"
					 "        ltr_interval(%u) ltr_pre_ref(%u) gray_en(%u) source_output(%u)\n"
					 "        profile(%#x) level_idc(%#x) svc_layer(%#x) entropy(%#x)\n",
					 dif_return_dev_string(path_id), path_id, enc_param->codec_type,
					 enc_param->h26x.gop_num,
					 enc_param->h26x.chrm_qp_idx,
					 enc_param->h26x.sec_chrm_qp_idx,
					 enc_param->h26x.ltr_interval,
					 enc_param->h26x.ltr_pre_ref,
					 enc_param->h26x.gray_en,
					 enc_param->h26x.source_output,
					 enc_param->h26x.profile,
					 enc_param->h26x.level_idc,
					 enc_param->h26x.svc_layer,
					 enc_param->h26x.entropy_mode);

		} else if (enc_param->codec_type == HD_CODEC_TYPE_JPEG) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_ENC_PARAM):\n"
					 "    %s path_id(%#x)\n"
					 "        codec(%#x) interval(%u) quality(%u)\n",
					 dif_return_dev_string(path_id), path_id, enc_param->codec_type,
					 enc_param->jpeg.retstart_interval,
					 enc_param->jpeg.image_quality);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_VUI: {
		HD_H26XENC_VUI *enc_vui = &videoenc_info[dev_idx].port[outport_idx].enc_vui;
		memcpy(p_param, enc_vui, sizeof(HD_H26XENC_VUI));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_VUI):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) sar_wh(%u,%u) matrix_coef(%u)\n"
				 "        transfer_characteristics(%u) colour_primaries(%u) video_format(%u)\n"
				 "        color_range(%u) timing_present_flag(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_vui->vui_en,
				 enc_vui->sar_width,
				 enc_vui->sar_height,
				 enc_vui->matrix_coef,
				 enc_vui->transfer_characteristics,
				 enc_vui->colour_primaries,
				 enc_vui->video_format,
				 enc_vui->color_range,
				 enc_vui->timing_present_flag);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_DEBLOCK: {
		HD_H26XENC_DEBLOCK *enc_deblock = &videoenc_info[dev_idx].port[outport_idx].enc_deblock;
		memcpy(p_param, enc_deblock, sizeof(HD_H26XENC_DEBLOCK));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_DEBLOCK):\n"
				 "    %s path_id(%#x)\n"
				 "        dis_ilf_idc(%u) db_alpha(%u) db_beta(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_deblock->dis_ilf_idc,
				 enc_deblock->db_alpha,
				 enc_deblock->db_beta);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL: {
		HD_H26XENC_RATE_CONTROL *enc_rc = &videoenc_info[dev_idx].port[outport_idx].enc_rc;
		memcpy(p_param, enc_rc, sizeof(HD_H26XENC_RATE_CONTROL));
		if (enc_rc->rc_mode == HD_RC_MODE_CBR) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(CBR) br(%u) fps(%u) fps_incr(%u)\n"
					 "        init_i_qp(%u) max_i_qp(%u) min_i_qp(%u)\n"
					 "        init_p_qp(%u) max_p_qp(%u) min_p_qp(%u)\n"
					 "        static_time(%u) ip_weight(%d)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->cbr.bitrate,
					 enc_rc->cbr.frame_rate_base,
					 enc_rc->cbr.frame_rate_incr,
					 enc_rc->cbr.init_i_qp,
					 enc_rc->cbr.max_i_qp,
					 enc_rc->cbr.min_i_qp,
					 enc_rc->cbr.init_p_qp,
					 enc_rc->cbr.max_p_qp,
					 enc_rc->cbr.min_p_qp,
					 enc_rc->cbr.static_time,
					 enc_rc->cbr.ip_weight);
		} else if (enc_rc->rc_mode == HD_RC_MODE_VBR) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(VBR) br(%u) fps(%u) fps_incr(%u)\n"
					 "        init_i_qp(%u) max_i_qp(%u) min_i_qp(%u)\n"
					 "        init_p_qp(%u) max_p_qp(%u) min_p_qp(%u)\n"
					 "        static_time(%u) ip_weight(%d) change_pos(%u)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->vbr.bitrate,
					 enc_rc->vbr.frame_rate_base,
					 enc_rc->vbr.frame_rate_incr,
					 enc_rc->vbr.init_i_qp,
					 enc_rc->vbr.max_i_qp,
					 enc_rc->vbr.min_i_qp,
					 enc_rc->vbr.init_p_qp,
					 enc_rc->vbr.max_p_qp,
					 enc_rc->vbr.min_p_qp,
					 enc_rc->vbr.static_time,
					 enc_rc->vbr.ip_weight,
					 enc_rc->vbr.change_pos);
		} else if (enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(FIX_QP) fps(%u) fps_incr(%u)\n"
					 "        fix_i_qp(%d) fix_p_qp(%d)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->fixqp.frame_rate_base,
					 enc_rc->fixqp.frame_rate_incr,
					 enc_rc->fixqp.fix_i_qp,
					 enc_rc->fixqp.fix_p_qp);
		} else if (enc_rc->rc_mode == HD_RC_MODE_EVBR) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(EVBR) br(%u) fps(%u) fps_incr(%u)\n"
					 "        init_i_qp(%u) max_i_qp(%u) min_i_qp(%u)\n"
					 "        init_p_qp(%u) max_p_qp(%u) min_p_qp(%u)\n"
					 "        static_time(%u) ip_weight(%d) key_p_period(%u) key_p_weight(%d)\n"
					 "        still_frame_cnd(%d) motion_ratio_thd(%d) motion_aq_str(%d)\n"
					 "        still_i_qp(%u) still_p_qp(%u) still_kp_qp(%u)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->evbr.bitrate,
					 enc_rc->evbr.frame_rate_base,
					 enc_rc->evbr.frame_rate_incr,
					 enc_rc->evbr.init_i_qp,
					 enc_rc->evbr.max_i_qp,
					 enc_rc->evbr.min_i_qp,
					 enc_rc->evbr.init_p_qp,
					 enc_rc->evbr.max_p_qp,
					 enc_rc->evbr.min_p_qp,
					 enc_rc->evbr.static_time,
					 enc_rc->evbr.ip_weight,
					 enc_rc->evbr.key_p_period,
					 enc_rc->evbr.kp_weight,
					 enc_rc->evbr.still_frame_cnd,
					 enc_rc->evbr.motion_ratio_thd,
					 enc_rc->evbr.motion_aq_str,
					 enc_rc->evbr.still_i_qp,
					 enc_rc->evbr.still_p_qp,
					 enc_rc->evbr.still_kp_qp);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_USR_QP: {
		HD_H26XENC_USR_QP *enc_usr_qp = &videoenc_info[dev_idx].port[outport_idx].enc_usr_qp;
		memcpy(p_param, enc_usr_qp, sizeof(HD_H26XENC_USR_QP));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_USR_QP):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) qp_map_addr(%#lx)\n",
				 dif_return_dev_string(path_id), path_id, enc_usr_qp->enable,
				 (unsigned long)enc_usr_qp->qp_map_addr);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT: {
		HD_H26XENC_SLICE_SPLIT *enc_slice_split = &videoenc_info[dev_idx].port[outport_idx].enc_slice_split;
		memcpy(p_param, enc_slice_split, sizeof(HD_H26XENC_SLICE_SPLIT));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_SLICE_SPLIT):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) slice_row_num(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_slice_split->enable,
				 enc_slice_split->slice_row_num);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ENC_GDR: {
		HD_H26XENC_GDR *enc_gdr = &videoenc_info[dev_idx].port[outport_idx].enc_gdr;
		memcpy(p_param, enc_gdr, sizeof(HD_H26XENC_GDR));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_ENC_GDR):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) period(%u) number(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_gdr->enable, enc_gdr->period, enc_gdr->number);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ROI: {
		HD_H26XENC_ROI *enc_roi = &videoenc_info[dev_idx].port[outport_idx].enc_roi;
		memcpy(p_param, enc_roi, sizeof(HD_H26XENC_ROI));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_ROI):\n"
				 "    %s path_id(%#x)\n"
				 "        mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id, enc_roi->roi_qp_mode);
		for (i = 0; i < HD_H26XENC_ROI_WIN_COUNT; i++) {
			HD_VIDEOENC_FLOW("        idx(%d) en(%u) rect(%u,%u,%u,%u) mode(%#x) qp(%u)\n",
					 i, enc_roi->st_roi[i].enable,
					 enc_roi->st_roi[i].rect.x,
					 enc_roi->st_roi[i].rect.y,
					 enc_roi->st_roi[i].rect.w,
					 enc_roi->st_roi[i].rect.h,
					 enc_roi->st_roi[i].mode,
					 enc_roi->st_roi[i].qp);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ROW_RC: {
		HD_H26XENC_ROW_RC *enc_row_rc = &videoenc_info[dev_idx].port[outport_idx].enc_row_rc;
		memcpy(p_param, enc_row_rc, sizeof(HD_H26XENC_ROW_RC));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_ROW_RC):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) i_qp_range(%u) i_qp_step(%u) i_qp_range(%u) i_qp_step(%u)\n"
				 "        min_i_qp(%u) max_i_qp(%u) min_p_qp(%u) max_p_qp(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_row_rc->enable,
				 enc_row_rc->i_qp_range,
				 enc_row_rc->i_qp_step,
				 enc_row_rc->p_qp_range,
				 enc_row_rc->p_qp_step,
				 enc_row_rc->min_i_qp,
				 enc_row_rc->max_i_qp,
				 enc_row_rc->min_p_qp,
				 enc_row_rc->max_p_qp);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_AQ: {
		HD_H26XENC_AQ *enc_aq = &videoenc_info[dev_idx].port[outport_idx].enc_aq;
		memcpy(p_param, enc_aq, sizeof(HD_H26XENC_AQ));
		HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_AQ):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) i_str(%hd) p_str(%hd) delta_qp max/min(%hd,%hd) depth(%hd)\n"
				 "        thd_table(%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd)\n"
				 "                 (%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd)\n"
				 "                 (%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd)\n",
				 dif_return_dev_string(path_id), path_id, enc_aq->enable, enc_aq->i_str, enc_aq->p_str,
				 enc_aq->max_delta_qp, enc_aq->min_delta_qp, enc_aq->depth,
				 enc_aq->thd_table[0], enc_aq->thd_table[1], enc_aq->thd_table[2],
				 enc_aq->thd_table[3], enc_aq->thd_table[4], enc_aq->thd_table[5],
				 enc_aq->thd_table[6], enc_aq->thd_table[7], enc_aq->thd_table[8],
				 enc_aq->thd_table[9], enc_aq->thd_table[10], enc_aq->thd_table[11],
				 enc_aq->thd_table[12], enc_aq->thd_table[13], enc_aq->thd_table[14],
				 enc_aq->thd_table[15], enc_aq->thd_table[16], enc_aq->thd_table[17],
				 enc_aq->thd_table[18], enc_aq->thd_table[19], enc_aq->thd_table[20],
				 enc_aq->thd_table[21], enc_aq->thd_table[22], enc_aq->thd_table[23],
				 enc_aq->thd_table[24], enc_aq->thd_table[25], enc_aq->thd_table[26],
				 enc_aq->thd_table[27], enc_aq->thd_table[28], enc_aq->thd_table[29]);
		break;
	}
	//osg cmd//
	case HD_VIDEOENC_PARAM_IN_STAMP_IMG: {
		in_id = VDOENC_STAMP_INID(in_id);
		HD_OSG_STAMP_IMG *p_stamp_img = &videoenc_info[dev_idx].port[outport_idx].stamp[in_id].img;
		memcpy(p_param, p_stamp_img, sizeof(HD_OSG_STAMP_IMG));
		HD_OSG_FLOW("hd_videoenc_get(IN_STAMP_IMG):\n"
			    "    %s path_id(%#x)\n"
			    "        pxlfmt(%#x) dim(%u,%u) ddr(%u) pa(%p)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp_img->fmt,
			    p_stamp_img->dim.w,
			    p_stamp_img->dim.h,
			    p_stamp_img->ddr_id,
			    (VOID *)p_stamp_img->p_addr);
		break;
	}
	case HD_VIDEOENC_PARAM_IN_STAMP_ATTR:
		in_id = VDOENC_STAMP_INID(in_id);
		HD_OSG_STAMP_ATTR *p_stamp_attr = &videoenc_info[dev_idx].port[outport_idx].stamp[in_id].attr;
		memcpy(p_param, p_stamp_attr, sizeof(HD_OSG_STAMP_ATTR));
		HD_OSG_FLOW("hd_videoenc_get(IN_STAMP_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) pos(%u,%u)\n"
			    "        gcac_en(%u) gcac_w(%u) gcac_h(%u)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp_attr->align_type,
			    p_stamp_attr->alpha,
			    p_stamp_attr->position.x,
			    p_stamp_attr->position.y,
			    p_stamp_attr->gcac_enable,
			    p_stamp_attr->gcac_blk_width,
			    p_stamp_attr->gcac_blk_height);
		break;
	case HD_VIDEOENC_PARAM_IN_MASK_ATTR: {
		in_id = VDOENC_MASK_INID(in_id);
		HD_OSG_MASK_ATTR *mask_param = &videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic[in_id].mask_cfg;
		memcpy(p_param, mask_param, sizeof(HD_OSG_MASK_ATTR));
		HD_OSG_FLOW("hd_videoenc_get(IN_MASK_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) color(%u)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u)\n",
			    dif_return_dev_string(path_id), path_id, mask_param->type,
			    mask_param->alpha, mask_param->color,
			    mask_param->position[0].x, mask_param->position[0].y,
			    mask_param->position[1].x, mask_param->position[1].y,
			    mask_param->position[2].x, mask_param->position[2].y,
			    mask_param->position[3].x, mask_param->position[3].y);
		break;
	}
	case HD_VIDEOENC_PARAM_IN_MOSAIC_ATTR: {
		in_id = VDOENC_MASK_INID(in_id);
		HD_OSG_MOSAIC_ATTR *mosaic_param = &videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic[in_id].mosaic_cfg;
		memcpy(p_param, mosaic_param, sizeof(HD_OSG_MOSAIC_ATTR));
		HD_OSG_FLOW("hd_videoenc_get(IN_MOSAIC_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) blk_wh(%u,%u)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u)\n",
			    dif_return_dev_string(path_id), path_id, mosaic_param->type, mosaic_param->alpha,
			    mosaic_param->mosaic_blk_w, mosaic_param->mosaic_blk_h,
			    mosaic_param->position[0].x, mosaic_param->position[0].y,
			    mosaic_param->position[1].x, mosaic_param->position[1].y,
			    mosaic_param->position[2].x, mosaic_param->position[2].y,
			    mosaic_param->position[3].x, mosaic_param->position[3].y);
		break;
	}
	case HD_VIDEOENC_PARAM_IN_PALETTE_TABLE: {
		HD_OSG_PALETTE_TBL *p_tbl = &videoenc_info[dev_idx].port[outport_idx].enc_osg_palette_tbl;
		ret = videoenc_get_mask_palette(self_id, in_id, p_tbl);
		HD_VIDEOENC_FLOW("hd_videoenc_get(IN_PALETTE_TABLE):\n"
				 "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < 8; i++) {
			HD_VIDEOENC_FLOW("        idx(%d) pal_y(%d) pal_cb(%d) pal_cr(%d)\n",
					 i, p_tbl->pal_y[i], p_tbl->pal_cb[i], p_tbl->pal_cr[i]);
		}
		memcpy(p_param, p_tbl, sizeof(HD_OSG_PALETTE_TBL));
		break;
	}
	case HD_VIDEOENC_PARAM_STATUS: {
		HD_VIDEOENC_STATUS *p_status = &videoenc_info[dev_idx].port[outport_idx].enc_status;
		ret = videoenc_get_status_p(self_id, in_id, p_status);
		memcpy(p_param, p_status, sizeof(HD_VIDEOENC_STATUS));
		HD_VIDEOENC_FLOW("hd_videoenc_get(STATUS):\n"
				 "    %s path_id(%#x)\n"
				 "        left_frames(%u)  done_frames(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_status->left_frames, p_status->done_frames);
		break;
	}
	default:
		HD_VIDEOENC_ERR("Unsupport set param id(%d)\n", id);
		return HD_ERR_NG;
	}
	return ret;
}

HD_RESULT hd_videoenc_set(HD_PATH_ID path_id, HD_VIDEOENC_PARAM_ID id, VOID *p_param)
{

	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx, outport_idx, ret;
	CHAR msg_string[256] = "\0";
	HD_VDOENC_STAMP_PARAM *p_stamp;
	HD_VDOENC_MASK_MOSAIC *p_mask_mosaic;
	HD_OSG_MASK_ATTR *p_mask_param = NULL;
	HD_OSG_MOSAIC_ATTR *p_mosaic_param = NULL;
	UINTPTR buf_addr = 0;
	HD_OUT_POOL *p_out_pool;
	INT i;

	//check parameters
	VIDEOENC_CHECK_SELF_ID(self_id);
	VIDEOENC_CHECK_P_PARAM(p_param);

	if (in_id <= HD_IN_MAX) { // add this to avoid using wrong in port id (there are encoder port and stamp port)
		switch (id) {
		case HD_VIDEOENC_PARAM_BUFINFO:
		case HD_VIDEOENC_PARAM_STATUS:
			HD_VIDEOENC_ERR("Unsupport 'set' opertaion for this HD_VIDEOENC_PARAM_ID id(%d)\n", id);
			return HD_ERR_NG;

		case HD_VIDEOENC_PARAM_IN:
		case HD_VIDEOENC_PARAM_IN_PALETTE_TABLE:
			//VIDEOENC_CHECK_IN_ID(io_id);
			break;

		case HD_VIDEOENC_PARAM_PATH_CONFIG:
			VIDEOENC_CHECK_IN_ID(in_id);
			VIDEOENC_CHECK_OUT_ID(out_id, FALSE);
			break;

		case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
		case HD_VIDEOENC_PARAM_OUT_VUI:
		case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
		case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
		case HD_VIDEOENC_PARAM_OUT_USR_QP:
		case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
		case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
		case HD_VIDEOENC_PARAM_OUT_ROI:
		case HD_VIDEOENC_PARAM_OUT_ROW_RC:
		case HD_VIDEOENC_PARAM_OUT_AQ:
		case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
			VIDEOENC_CHECK_OUT_ID(out_id, FALSE);
			break;
		default:
			HD_VIDEOENC_ERR("Unsupport set param id(%d)\n", id);
			return HD_ERR_NG;
		}

		ret = check_params_range(id, p_param, msg_string, 256);
		if (ret < 0) {
			HD_VIDEOENC_ERR("Wrong value. %s, id=%d\n", msg_string, id);
			return HD_ERR_PARAM;
		}
	}
	dev_idx = VDOENC_DEVID(self_id);
	outport_idx = VDOENC_OUTID(out_id);

	//store user values
	switch (id) {
	case HD_VIDEOENC_PARAM_IN: {
		INT inport_idx = VDOENC_INID(in_id);
		HD_VIDEOENC_IN *enc_in;

		if (inport_idx >= HD_VIDEOENC_MAX_IN) {
			HD_VIDEOENC_ERR("Unsupport in_id(%d)\n", in_id);
			return HD_ERR_LIMIT;
		}

		enc_in = &videoenc_info[dev_idx].port[inport_idx].enc_in_dim;
		if (memcmp(enc_in, p_param, sizeof(HD_VIDEOENC_IN))) {
			memcpy(enc_in, p_param, sizeof(HD_VIDEOENC_IN));
			vdoenc_dev_param[dev_idx].port[inport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[inport_idx].p_enc_in_param = enc_in;
		HD_VIDEOENC_FLOW("hd_videoenc_set(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        dim(%u,%u) pxlfmt(%#x) dir(%#x)\n",
				 dif_return_dev_string(path_id), path_id, enc_in->dim.w, enc_in->dim.h, enc_in->pxl_fmt, enc_in->dir);
		break;
	}

	case HD_VIDEOENC_PARAM_PATH_CONFIG: {
		HD_VIDEOENC_PATH_CONFIG *enc_param = &videoenc_info[dev_idx].port[outport_idx].enc_path_cfg;
		memcpy(enc_param, p_param, sizeof(HD_VIDEOENC_PATH_CONFIG));
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_path_cfg = enc_param;

		p_out_pool = &videoenc_info[dev_idx].port[outport_idx].out_pool;
		for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
			p_out_pool->buf_info[i].ddr_id = enc_param->data_pool[i].ddr_id;
			p_out_pool->buf_info[i].counts = enc_param->data_pool[i].counts;
			p_out_pool->buf_info[i].max_counts = enc_param->data_pool[i].max_counts;
			p_out_pool->buf_info[i].min_counts = enc_param->data_pool[i].min_counts;
			p_out_pool->buf_info[i].enable = enc_param->data_pool[i].mode;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_out_pool = p_out_pool;
		HD_VIDEOENC_FLOW("hd_videoenc_set(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) max_dim(%u,%u) enc_buf_ms(%u) max_br(%u)\n"
				 "        svc_layer(%#x) ltr(%d) rotate(%d) src_out(%d)\n",
				 dif_return_dev_string(path_id), path_id, enc_param->max_mem.codec_type,
				 enc_param->max_mem.max_dim.w,
				 enc_param->max_mem.max_dim.h,
				 enc_param->max_mem.enc_buf_ms,
				 enc_param->max_mem.bitrate,
				 enc_param->max_mem.svc_layer,
				 enc_param->max_mem.ltr,
				 enc_param->max_mem.rotate,
				 enc_param->max_mem.source_output);
		for (i = 0; i < HD_VIDEOENC_MAX_DATA_TYPE; i++) {
			HD_VIDEOENC_FLOW("        idx(%d) ddr(%u) count(%2.1f) max/min(%2.1f,%2.1f) mode(%#x)\n",
					 i, enc_param->data_pool[i].ddr_id,
					 ((float)enc_param->data_pool[i].counts) / 10,
					 ((float)enc_param->data_pool[i].max_counts) / 10,
					 ((float)enc_param->data_pool[i].min_counts) / 10,
					 enc_param->data_pool[i].mode);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM: {
		HD_VIDEOENC_OUT *enc_param = &videoenc_info[dev_idx].port[outport_idx].enc_out_param;
		if (memcmp(enc_param, p_param, sizeof(HD_VIDEOENC_OUT))) {
			memcpy(enc_param, p_param, sizeof(HD_VIDEOENC_OUT));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_out_param = enc_param;
		if ((enc_param->codec_type == HD_CODEC_TYPE_H264) ||
		    (enc_param->codec_type == HD_CODEC_TYPE_H265)) {
			HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_ENC_PARAM):\n"
					 "    %s path_id(%#x)\n"
					 "        codec(%#x) gop(%u) chrm_qp_idx(%u) sec_chrm_qp_idx(%u)\n"
					 "        ltr_interval(%u) ltr_pre_ref(%u) gray_en(%u) source_output(%u)\n"
					 "        profile(%#x) level_idc(%#x) svc_layer(%#x) entropy(%#x)\n",
					 dif_return_dev_string(path_id), path_id, enc_param->codec_type,
					 enc_param->h26x.gop_num,
					 enc_param->h26x.chrm_qp_idx,
					 enc_param->h26x.sec_chrm_qp_idx,
					 enc_param->h26x.ltr_interval,
					 enc_param->h26x.ltr_pre_ref,
					 enc_param->h26x.gray_en,
					 enc_param->h26x.source_output,
					 enc_param->h26x.profile,
					 enc_param->h26x.level_idc,
					 enc_param->h26x.svc_layer,
					 enc_param->h26x.entropy_mode);

		} else if (enc_param->codec_type == HD_CODEC_TYPE_JPEG) {
			HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_ENC_PARAM):\n"
					 "    %s path_id(%#x)\n"
					 "        codec(%#x) interval(%u) quality(%u)\n",
					 dif_return_dev_string(path_id), path_id, enc_param->codec_type,
					 enc_param->jpeg.retstart_interval,
					 enc_param->jpeg.image_quality);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_VUI: {
		HD_H26XENC_VUI *enc_vui = &videoenc_info[dev_idx].port[outport_idx].enc_vui;
		if (memcmp(enc_vui, p_param, sizeof(HD_H26XENC_VUI))) {
			memcpy(enc_vui, p_param, sizeof(HD_H26XENC_VUI));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_vui = enc_vui;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_VUI):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) sar_wh(%u,%u) matrix_coef(%u)\n"
				 "        transfer_characteristics(%u) colour_primaries(%u) video_format(%u)\n"
				 "        color_range(%u) timing_present_flag(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_vui->vui_en,
				 enc_vui->sar_width,
				 enc_vui->sar_height,
				 enc_vui->matrix_coef,
				 enc_vui->transfer_characteristics,
				 enc_vui->colour_primaries,
				 enc_vui->video_format,
				 enc_vui->color_range,
				 enc_vui->timing_present_flag);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_DEBLOCK: {
		HD_H26XENC_DEBLOCK *enc_deblock = &videoenc_info[dev_idx].port[outport_idx].enc_deblock;
		if (memcmp(enc_deblock, p_param, sizeof(HD_H26XENC_DEBLOCK))) {
			memcpy(enc_deblock, p_param, sizeof(HD_H26XENC_DEBLOCK));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_deblock = enc_deblock;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_DEBLOCK):\n"
				 "    %s path_id(%#x)\n"
				 "        dis_ilf_idc(%u) db_alpha(%u) db_beta(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_deblock->dis_ilf_idc,
				 enc_deblock->db_alpha,
				 enc_deblock->db_beta);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL: {
		HD_H26XENC_RATE_CONTROL *enc_rc = &videoenc_info[dev_idx].port[outport_idx].enc_rc;
		if (memcmp(enc_rc, p_param, sizeof(HD_H26XENC_RATE_CONTROL))) {
			memcpy(enc_rc, p_param, sizeof(HD_H26XENC_RATE_CONTROL));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_rc = enc_rc;
		if (enc_rc->rc_mode == HD_RC_MODE_CBR) {
			HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(CBR) br(%u) fps(%u) fps_incr(%u)\n"
					 "        init_i_qp(%u) max_i_qp(%u) min_i_qp(%u)\n"
					 "        init_p_qp(%u) max_p_qp(%u) min_p_qp(%u)\n"
					 "        static_time(%u) ip_weight(%d)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->cbr.bitrate,
					 enc_rc->cbr.frame_rate_base,
					 enc_rc->cbr.frame_rate_incr,
					 enc_rc->cbr.init_i_qp,
					 enc_rc->cbr.max_i_qp,
					 enc_rc->cbr.min_i_qp,
					 enc_rc->cbr.init_p_qp,
					 enc_rc->cbr.max_p_qp,
					 enc_rc->cbr.min_p_qp,
					 enc_rc->cbr.static_time,
					 enc_rc->cbr.ip_weight);
		} else if (enc_rc->rc_mode == HD_RC_MODE_VBR) {
			HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(VBR) br(%u) fps(%u) fps_incr(%u)\n"
					 "        init_i_qp(%u) max_i_qp(%u) min_i_qp(%u)\n"
					 "        init_p_qp(%u) max_p_qp(%u) min_p_qp(%u)\n"
					 "        static_time(%u) ip_weight(%d) change_pos(%u)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->vbr.bitrate,
					 enc_rc->vbr.frame_rate_base,
					 enc_rc->vbr.frame_rate_incr,
					 enc_rc->vbr.init_i_qp,
					 enc_rc->vbr.max_i_qp,
					 enc_rc->vbr.min_i_qp,
					 enc_rc->vbr.init_p_qp,
					 enc_rc->vbr.max_p_qp,
					 enc_rc->vbr.min_p_qp,
					 enc_rc->vbr.static_time,
					 enc_rc->vbr.ip_weight,
					 enc_rc->vbr.change_pos);
		} else if (enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
			HD_VIDEOENC_FLOW("hd_videoenc_get(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(FIX_QP) fps(%u) fps_incr(%u)\n"
					 "        fix_i_qp(%d) fix_p_qp(%d)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->fixqp.frame_rate_base,
					 enc_rc->fixqp.frame_rate_incr,
					 enc_rc->fixqp.fix_i_qp,
					 enc_rc->fixqp.fix_p_qp);
		} else if (enc_rc->rc_mode == HD_RC_MODE_EVBR) {
			HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_RATE_CONTROL):\n"
					 "    %s path_id(%#x)\n"
					 "        rc_mode(EVBR) br(%u) fps(%u) fps_incr(%u)\n"
					 "        init_i_qp(%u) max_i_qp(%u) min_i_qp(%u)\n"
					 "        init_p_qp(%u) max_p_qp(%u) min_p_qp(%u)\n"
					 "        static_time(%u) ip_weight(%d) key_p_period(%u) key_p_weight(%d)\n"
					 "        still_frame_cnd(%d) motion_ratio_thd(%d) motion_aq_str(%d)\n"
					 "        still_i_qp(%u) still_p_qp(%u) still_kp_qp(%u)\n",
					 dif_return_dev_string(path_id), path_id,
					 enc_rc->evbr.bitrate,
					 enc_rc->evbr.frame_rate_base,
					 enc_rc->evbr.frame_rate_incr,
					 enc_rc->evbr.init_i_qp,
					 enc_rc->evbr.max_i_qp,
					 enc_rc->evbr.min_i_qp,
					 enc_rc->evbr.init_p_qp,
					 enc_rc->evbr.max_p_qp,
					 enc_rc->evbr.min_p_qp,
					 enc_rc->evbr.static_time,
					 enc_rc->evbr.ip_weight,
					 enc_rc->evbr.key_p_period,
					 enc_rc->evbr.kp_weight,
					 enc_rc->evbr.still_frame_cnd,
					 enc_rc->evbr.motion_ratio_thd,
					 enc_rc->evbr.motion_aq_str,
					 enc_rc->evbr.still_i_qp,
					 enc_rc->evbr.still_p_qp,
					 enc_rc->evbr.still_kp_qp);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_USR_QP: {
		HD_H26XENC_USR_QP *enc_usr_qp = &videoenc_info[dev_idx].port[outport_idx].enc_usr_qp;
		if (memcmp(enc_usr_qp, p_param, sizeof(HD_H26XENC_USR_QP))) {
			memcpy(enc_usr_qp, p_param, sizeof(HD_H26XENC_USR_QP));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_usr_qp = enc_usr_qp;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_USR_QP):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) qp_map_addr(%#lx)\n",
				 dif_return_dev_string(path_id), path_id, enc_usr_qp->enable,
				 (unsigned long)enc_usr_qp->qp_map_addr);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT: {
		HD_H26XENC_SLICE_SPLIT *enc_slice_split = &videoenc_info[dev_idx].port[outport_idx].enc_slice_split;
		if (memcmp(enc_slice_split, p_param, sizeof(HD_H26XENC_SLICE_SPLIT))) {
			memcpy(enc_slice_split, p_param, sizeof(HD_H26XENC_SLICE_SPLIT));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_slice_split = enc_slice_split;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_SLICE_SPLIT):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) slice_row_num(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_slice_split->enable,
				 enc_slice_split->slice_row_num);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ENC_GDR: {
		HD_H26XENC_GDR *enc_gdr = &videoenc_info[dev_idx].port[outport_idx].enc_gdr;
		if (memcmp(enc_gdr, p_param, sizeof(HD_H26XENC_GDR))) {
			memcpy(enc_gdr, p_param, sizeof(HD_H26XENC_GDR));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_gdr = enc_gdr;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_ENC_GDR):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) period(%u) number(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_gdr->enable, enc_gdr->period, enc_gdr->number);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ROI: {
		HD_H26XENC_ROI *enc_roi = &videoenc_info[dev_idx].port[outport_idx].enc_roi;
		if (memcmp(enc_roi, p_param, sizeof(HD_H26XENC_ROI))) {
			memcpy(enc_roi, p_param, sizeof(HD_H26XENC_ROI));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_roi = enc_roi;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_ROI):\n"
				 "    %s path_id(%#x)\n"
				 "        mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id, enc_roi->roi_qp_mode);
		for (i = 0; i < HD_H26XENC_ROI_WIN_COUNT; i++) {
			HD_VIDEOENC_FLOW("        idx(%d) en(%u) rect(%u,%u,%u,%u) mode(%#x) qp(%u)\n",
					 i, enc_roi->st_roi[i].enable,
					 enc_roi->st_roi[i].rect.x,
					 enc_roi->st_roi[i].rect.y,
					 enc_roi->st_roi[i].rect.w,
					 enc_roi->st_roi[i].rect.h,
					 enc_roi->st_roi[i].mode,
					 enc_roi->st_roi[i].qp);
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ROW_RC: {
		HD_H26XENC_ROW_RC *enc_row_rc = &videoenc_info[dev_idx].port[outport_idx].enc_row_rc;
		if (memcmp(enc_row_rc, p_param, sizeof(HD_H26XENC_ROW_RC))) {
			memcpy(enc_row_rc, p_param, sizeof(HD_H26XENC_ROW_RC));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_row_rc = enc_row_rc;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_ROW_RC):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) i_qp_range(%u) i_qp_step(%u) i_qp_range(%u) i_qp_step(%u)\n"
				 "        min_i_qp(%u) max_i_qp(%u) min_p_qp(%u) max_p_qp(%u)\n",
				 dif_return_dev_string(path_id), path_id, enc_row_rc->enable,
				 enc_row_rc->i_qp_range,
				 enc_row_rc->i_qp_step,
				 enc_row_rc->p_qp_range,
				 enc_row_rc->p_qp_step,
				 enc_row_rc->min_i_qp,
				 enc_row_rc->max_i_qp,
				 enc_row_rc->min_p_qp,
				 enc_row_rc->max_p_qp);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_AQ: {
		HD_H26XENC_AQ *enc_aq = &videoenc_info[dev_idx].port[outport_idx].enc_aq;
		if (memcmp(enc_aq, p_param, sizeof(HD_H26XENC_AQ))) {
			memcpy(enc_aq, p_param, sizeof(HD_H26XENC_AQ));
			vdoenc_dev_param[dev_idx].port[outport_idx].set_count++;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].p_enc_aq = enc_aq;
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_AQ):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d) i_str(%hd) p_str(%hd) delta_qp max/min(%hd,%hd) depth(%hd)\n"
				 "        thd_table(%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd)\n"
				 "                 (%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd)\n"
				 "                 (%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd)\n",
				 dif_return_dev_string(path_id), path_id, enc_aq->enable, enc_aq->i_str, enc_aq->p_str,
				 enc_aq->max_delta_qp, enc_aq->min_delta_qp, enc_aq->depth,
				 enc_aq->thd_table[0], enc_aq->thd_table[1], enc_aq->thd_table[2],
				 enc_aq->thd_table[3], enc_aq->thd_table[4], enc_aq->thd_table[5],
				 enc_aq->thd_table[6], enc_aq->thd_table[7], enc_aq->thd_table[8],
				 enc_aq->thd_table[9], enc_aq->thd_table[10], enc_aq->thd_table[11],
				 enc_aq->thd_table[12], enc_aq->thd_table[13], enc_aq->thd_table[14],
				 enc_aq->thd_table[15], enc_aq->thd_table[16], enc_aq->thd_table[17],
				 enc_aq->thd_table[18], enc_aq->thd_table[19], enc_aq->thd_table[20],
				 enc_aq->thd_table[21], enc_aq->thd_table[22], enc_aq->thd_table[23],
				 enc_aq->thd_table[24], enc_aq->thd_table[25], enc_aq->thd_table[26],
				 enc_aq->thd_table[27], enc_aq->thd_table[28], enc_aq->thd_table[29]);
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME: {
		HD_H26XENC_REQUEST_IFRAME *request_keyframe = &videoenc_info[dev_idx].port[outport_idx].request_keyframe;
		memcpy(request_keyframe, p_param, sizeof(HD_H26XENC_REQUEST_IFRAME));
		vdoenc_dev_param[dev_idx].port[outport_idx].p_request_keyframe = request_keyframe;
		videoenc_request_keyframe_p(self_id, out_id);
		HD_VIDEOENC_FLOW("hd_videoenc_set(OUT_REQUEST_IFRAME):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d)\n",
				 dif_return_dev_string(path_id), path_id, request_keyframe->enable);
		break;
	}
	//osg cmd//
	case HD_VIDEOENC_PARAM_IN_STAMP_IMG: {
		in_id = VDOENC_STAMP_INID(in_id);
		p_stamp = &videoenc_info[dev_idx].port[outport_idx].stamp[in_id];
		buf_addr = ((HD_OSG_STAMP_IMG *)p_param)->p_addr;
		p_stamp->img.fmt = ((HD_OSG_STAMP_IMG *)p_param)->fmt;
		if ((((HD_OSG_STAMP_IMG *)p_param)->dim.w % ENCODE_OSG_W_ALIGN) || (((HD_OSG_STAMP_IMG *)p_param)->dim.h % ENCODE_OSG_H_ALIGN))
			HD_VIDEOENC_ERR("Check encode osg stamp dim not align to (%d %d),stamp dim(%d %d)\n", ENCODE_OSG_W_ALIGN, ENCODE_OSG_H_ALIGN,
			((HD_OSG_STAMP_IMG *)p_param)->dim.w, ((HD_OSG_STAMP_IMG *)p_param)->dim.h);
		p_stamp->img.dim.w = ALIGN4_DOWN(((HD_OSG_STAMP_IMG *)p_param)->dim.w);
		p_stamp->img.dim.h = ALIGN2_DOWN(((HD_OSG_STAMP_IMG *)p_param)->dim.h);
		p_stamp->img.ddr_id = ((HD_OSG_STAMP_IMG *)p_param)->ddr_id;
		p_stamp->img.p_addr = buf_addr;
		if (pif_address_ddr_sanity_check(p_stamp->img.p_addr, p_stamp->img.ddr_id) < 0) {
			HD_VIDEOENC_ERR("%s:Check encode osg stamp pa(%#lx) ddrid(%d) error\n", __func__, p_stamp->img.p_addr, p_stamp->img.ddr_id);
			ret = HD_ERR_PARAM;
		}
		HD_OSG_FLOW("hd_videoenc_set(IN_STAMP_IMG):\n"
			    "    %s path_id(%#x)\n"
			    "        pxlfmt(%#x) dim(%u,%u) ddr(%u) pa(%p)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp->img.fmt,
			    p_stamp->img.dim.w,
			    p_stamp->img.dim.h,
			    p_stamp->img.ddr_id,
			    (VOID *)p_stamp->img.p_addr);
		break;
	}
	case HD_VIDEOENC_PARAM_IN_STAMP_ATTR:
		in_id = VDOENC_STAMP_INID(in_id);
		p_stamp = &videoenc_info[dev_idx].port[outport_idx].stamp[in_id];
		if (((HD_OSG_STAMP_ATTR *)p_param)->align_type != HD_OSG_ALIGN_TYPE_TOP_LEFT) {
			HD_VIDEOENC_ERR("Only supports HD_OSG_ALIGN_TYPE_TOP_LEFT\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if (((HD_OSG_STAMP_ATTR *)p_param)->alpha > 7) {
			HD_VIDEOENC_ERR("enc_osg_alpha_value:0~7\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if (((((HD_OSG_STAMP_ATTR *)p_param)->position.x + p_stamp->img.dim.w) > videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.w) || \
		    ((((HD_OSG_STAMP_ATTR *)p_param)->position.y + p_stamp->img.dim.h) > videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.h)) {
			HD_VIDEOENC_ERR("Check enc osg stamp region(%d-%d)>enc_dim(%d-%d)\n", (((HD_OSG_STAMP_ATTR *)p_param)->position.x + p_stamp->img.dim.w),
					(((HD_OSG_STAMP_ATTR *)p_param)->position.y + p_stamp->img.dim.h), videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.w,
					videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.h);
			return HD_ERR_NG;
		}
		p_stamp->attr.align_type = ((HD_OSG_STAMP_ATTR *)p_param)->align_type;
		p_stamp->attr.alpha = ((HD_OSG_STAMP_ATTR *)p_param)->alpha;
		p_stamp->attr.position.x = ((HD_OSG_STAMP_ATTR *)p_param)->position.x;
		p_stamp->attr.position.y = ((HD_OSG_STAMP_ATTR *)p_param)->position.y;
		p_stamp->attr.gcac_enable = ((HD_OSG_STAMP_ATTR *)p_param)->gcac_enable;
		if (p_stamp->attr.gcac_enable == 1) {
			HD_VIDEOENC_ERR("Not support osg gcac function\n");
		}
		p_stamp->attr.gcac_blk_width = ((HD_OSG_STAMP_ATTR *)p_param)->gcac_blk_width;
		p_stamp->attr.gcac_blk_height = ((HD_OSG_STAMP_ATTR *)p_param)->gcac_blk_height;
		HD_OSG_FLOW("hd_videoenc_set(IN_STAMP_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) pos(%u,%u)\n"
			    "        gcac_en(%u) gcac_w(%u) gcac_h(%u)\n"
			    "        pa(%p) ddr(%u)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp->attr.align_type,
			    p_stamp->attr.alpha,
			    p_stamp->attr.position.x,
			    p_stamp->attr.position.y,
			    p_stamp->attr.gcac_enable,
			    p_stamp->attr.gcac_blk_width,
			    p_stamp->attr.gcac_blk_height,
			    (VOID *)p_stamp->img.p_addr,
			    p_stamp->img.ddr_id);
		break;
	case HD_VIDEOENC_PARAM_IN_MASK_ATTR: {
		in_id = VDOENC_MASK_INID(in_id);
		outport_idx = VDOENC_OUTID(out_id);
		p_mask_mosaic = &videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic[in_id];
		p_mask_param = (HD_OSG_MASK_ATTR *)p_param;
		VDOENC_CHECK_H_POSIT(p_mask_param->position[0], p_mask_param->position[1]);
		VDOENC_CHECK_V_POSIT(p_mask_param->position[1], p_mask_param->position[2]);
		VDOENC_CHECK_H_POSIT(p_mask_param->position[3], p_mask_param->position[2]);
		VDOENC_CHECK_V_POSIT(p_mask_param->position[0], p_mask_param->position[3]);
		if ((p_mask_param->position[1].x - p_mask_param->position[0].x) < 8) {
			HD_VIDEOENC_ERR("mask width must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if ((p_mask_param->position[2].y - p_mask_param->position[1].y) < 8) {
			HD_VIDEOENC_ERR("mask height must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if (p_mask_param->position[2].x > videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.w ||
		    p_mask_param->position[2].y > videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.h) {
			HD_VIDEOENC_ERR("Check enc mask region(%d-%d)>enc_dim(%d-%d)\n", p_mask_param->position[2].x, p_mask_param->position[2].y,
					videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.w, videoenc_info[dev_idx].port[outport_idx].enc_in_dim.dim.h);
			return HD_ERR_NG;
		}
		p_mask_mosaic->is_mask = 1;
		p_mask_mosaic->mask_cfg.alpha = p_mask_param->alpha;
		p_mask_mosaic->mask_cfg.color = p_mask_param->color;
		p_mask_mosaic->mask_cfg.type = p_mask_param->type;
		p_mask_mosaic->mask_cfg.position[0].x = p_mask_param->position[0].x;
		p_mask_mosaic->mask_cfg.position[0].y = p_mask_param->position[0].y;
		p_mask_mosaic->mask_cfg.position[1].x = p_mask_param->position[1].x;
		p_mask_mosaic->mask_cfg.position[1].y = p_mask_param->position[1].y;
		p_mask_mosaic->mask_cfg.position[2].x = p_mask_param->position[2].x;
		p_mask_mosaic->mask_cfg.position[2].y = p_mask_param->position[2].y;
		p_mask_mosaic->mask_cfg.position[3].x = p_mask_param->position[3].x;
		p_mask_mosaic->mask_cfg.position[3].y = p_mask_param->position[3].y;
		HD_OSG_FLOW("hd_videoenc_set(IN_MASK_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) color(%u)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u)\n",
			    dif_return_dev_string(path_id), path_id, p_mask_param->type,
			    p_mask_param->alpha, p_mask_param->color,
			    p_mask_param->position[0].x, p_mask_param->position[0].y,
			    p_mask_param->position[1].x, p_mask_param->position[1].y,
			    p_mask_param->position[2].x, p_mask_param->position[2].y,
			    p_mask_param->position[3].x, p_mask_param->position[3].y);
		break;
	}
	case HD_VIDEOENC_PARAM_IN_MOSAIC_ATTR: {
		in_id = VDOENC_MASK_INID(in_id);
		outport_idx = VDOENC_OUTID(out_id);
		p_mask_mosaic = &videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic[in_id];
		p_mosaic_param = (HD_OSG_MOSAIC_ATTR *)p_param;
		VDOENC_CHECK_H_POSIT(p_mosaic_param->position[0], p_mosaic_param->position[1]);
		VDOENC_CHECK_V_POSIT(p_mosaic_param->position[1], p_mosaic_param->position[2]);
		VDOENC_CHECK_H_POSIT(p_mosaic_param->position[3], p_mosaic_param->position[2]);
		VDOENC_CHECK_V_POSIT(p_mosaic_param->position[0], p_mosaic_param->position[3]);
		if ((p_mosaic_param->position[1].x - p_mosaic_param->position[0].x) < 8) {
			HD_VIDEOENC_ERR("mosaic width must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if ((p_mosaic_param->position[2].y - p_mosaic_param->position[1].y) < 8) {
			HD_VIDEOENC_ERR("mosaic height must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if ((p_mosaic_param->type != HD_OSG_MASK_TYPE_SOLID) &&
		    (p_mosaic_param->type != HD_OSG_MASK_TYPE_INVERSE)) {
			HD_VIDEOENC_ERR("mosaic mask type 0x%X not supported\n", p_mosaic_param->type);
			return HD_ERR_NOT_SUPPORT;
		}
		p_mask_mosaic->is_mask = 0;
		p_mask_mosaic->mosaic_cfg.alpha = p_mosaic_param->alpha;
		p_mask_mosaic->mosaic_cfg.type = p_mosaic_param->type;
		p_mask_mosaic->mosaic_cfg.mosaic_blk_w = p_mosaic_param->mosaic_blk_w;
		p_mask_mosaic->mosaic_cfg.mosaic_blk_h = p_mosaic_param->mosaic_blk_h;
		p_mask_mosaic->mosaic_cfg.position[0].x = ALIGN2_DOWN(p_mosaic_param->position[0].x);
		p_mask_mosaic->mosaic_cfg.position[0].y = ALIGN2_DOWN(p_mosaic_param->position[0].y);
		p_mask_mosaic->mosaic_cfg.position[1].x = ALIGN2_DOWN(p_mosaic_param->position[1].x);
		p_mask_mosaic->mosaic_cfg.position[1].y = ALIGN2_DOWN(p_mosaic_param->position[1].y);
		p_mask_mosaic->mosaic_cfg.position[2].x = ALIGN2_DOWN(p_mosaic_param->position[2].x);
		p_mask_mosaic->mosaic_cfg.position[2].y = ALIGN2_DOWN(p_mosaic_param->position[2].y);
		p_mask_mosaic->mosaic_cfg.position[3].x = ALIGN2_DOWN(p_mosaic_param->position[3].x);
		p_mask_mosaic->mosaic_cfg.position[3].y = ALIGN2_DOWN(p_mosaic_param->position[3].y);
		HD_OSG_FLOW("hd_videoenc_set(IN_MOSAIC_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) blk_wh(%u,%u)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u)\n",
			    dif_return_dev_string(path_id), path_id, p_mosaic_param->type, p_mosaic_param->alpha,
			    p_mosaic_param->mosaic_blk_w, p_mosaic_param->mosaic_blk_h,
			    p_mosaic_param->position[0].x, p_mosaic_param->position[0].y,
			    p_mosaic_param->position[1].x, p_mosaic_param->position[1].y,
			    p_mosaic_param->position[2].x, p_mosaic_param->position[2].y,
			    p_mosaic_param->position[3].x, p_mosaic_param->position[3].y);
		break;
	}
	case HD_VIDEOENC_PARAM_IN_PALETTE_TABLE: {
		HD_OSG_PALETTE_TBL *p_tbl = (HD_OSG_PALETTE_TBL *)p_param;
		hd_ret = videoenc_set_mask_palette(self_id, in_id, p_param);
		HD_VIDEOENC_FLOW("hd_videoenc_set(IN_PALETTE_TABLE):\n"
				 "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < 8; i++) {
			HD_VIDEOENC_FLOW("        idx(%d) pal_y(%d) pal_cb(%d) pal_cr(%d)\n",
					 i, p_tbl->pal_y[i], p_tbl->pal_cb[i], p_tbl->pal_cr[i]);
		}
		break;
	}

	///////////
	default:
		HD_VIDEOENC_ERR("Unsupport set param id(%d)\n", id);
		return HD_ERR_NG;
	}
	return hd_ret;
}

static char bind_src_name[64] = "-";
HD_RESULT hd_videoenc_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, HD_VIDEOENC_BS *p_usr_out_video_bitstream, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	char *sign = NULL;
	INT dev_idx, inport_idx;
	VDOENC_INFO_PRIV *p_enc_priv;
#ifdef VENC_CHECK_PUSH_PARAM
	HD_VIDEOENC_OUT *enc_param;
	HD_VIDEOENC_IN *enc_in;
	CHAR msg_string[256];
#endif
	VIDEOENC_CHECK_SELF_ID(self_id);
	VIDEOENC_CHECK_IN_ID(in_id);
	VIDEOENC_CHECK_VIDEO_FRAME_SIZE(p_video_frame);

	if (wait_ms < 0 || wait_ms > 10000) {
		HD_VIDEOENC_ERR("Check the value wait_ms(%d) is out-of-range(0~%d).\n", wait_ms, 10000);
		ret = HD_ERR_NOT_SUPPORT;
		goto exit;
	}
	if (p_video_frame->phy_addr[0] == 0) {
		HD_VIDEOENC_ERR("Check HD_VIDEO_FRAME phy_addr[0] is NULL.\n");
		ret = HD_ERR_NOT_SUPPORT;
		goto exit;
	}

	dev_idx = VDOENC_DEVID(self_id);
	inport_idx = VDOENC_INID(in_id);
#ifdef VENC_CHECK_PUSH_PARAM
	enc_in = &videoenc_info[dev_idx].port[inport_idx].enc_in_dim;
	enc_param = &videoenc_info[dev_idx].port[inport_idx].enc_out_param;
	if (in_id <= HD_IN_MAX) {
		ret = verify_param_HD_VIDEOENC_IN(enc_in, enc_param, msg_string, 256);
		if (ret < 0) {
			HD_VIDEOENC_ERR("Wrong value. %s, id=%d\n", msg_string, HD_VIDEOENC_PARAM_IN);
		}
	}
#endif
	/* check and set state */
	p_enc_priv = &videoenc_info[dev_idx].priv[inport_idx];
	if (p_enc_priv->push_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto exit;
	} else if (p_enc_priv->push_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto exit;
	} else {
		p_enc_priv->push_state = HD_PATH_ID_ONGOING;
	}

	if ((videoenc_info[dev_idx].port[inport_idx].push_in_used == 1) ||
	    (bd_get_prev(self_id, in_id, bind_src_name, sizeof(bind_src_name)) == HD_OK)) {
		HD_VIDEOENC_ERR("Videoenc path(0x%x) is already used(%d). in_id(%d) src(%s)\n",
				(int)path_id, videoenc_info[dev_idx].port[inport_idx].push_in_used, (int)in_id, bind_src_name);
		system("echo videoenc all > /proc/hdal/setting");
		system("cat /proc/hdal/setting");
		ret = HD_ERR_NOT_SUPPORT;
		p_enc_priv->push_state = HD_PATH_ID_OPEN;
		goto exit;
	}

	videoenc_info[dev_idx].port[inport_idx].push_in_used = 1;
	if (osg_select[dev_idx][inport_idx]) {
		p_video_frame->reserved[0] = path_id;
		ret = videoenc_set_osg_for_push_pull(path_id, p_video_frame, 500);//use hard code to timeout,sample maybe set 0ms
		if (ret != HD_OK) {
			HD_VIDEOENC_ERR("Fail to set osg for push pull.\n");
			goto exit_in_used;
		}
	}
	HD_TRIGGER_FLOW("hd_videoenc_push_in_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	if (p_video_frame->sign) {
		sign = (char *)&p_video_frame->sign;
		HD_TRIGGER_FLOW("       in_buf_info:sign(%C%C%C%C) ddrid(%d) dim(%dx%d) fmt(%#x) pa(%p)\n", sign[0], sign[1], sign[2], sign[3], \
				p_video_frame->ddr_id, p_video_frame->dim.w, p_video_frame->dim.h, p_video_frame->pxlfmt, (VOID *)p_video_frame->phy_addr[0]);
	} else {
		HD_TRIGGER_FLOW("       in_buf_info:ddrid(%d) dim(%dx%d) fmt(%#x) pa(%p)\n", p_video_frame->ddr_id, \
				p_video_frame->dim.w, p_video_frame->dim.h, p_video_frame->pxlfmt, (VOID *)p_video_frame->phy_addr[0]);
	}
	if (p_usr_out_video_bitstream) {
		HD_TRIGGER_FLOW("       out_buf_info:ddrid(%d) pa(%p)\n", p_usr_out_video_bitstream->ddr_id, \
				(VOID *)p_usr_out_video_bitstream->video_pack[0].phy_addr);
	}
	p_video_frame->reserved[0] = path_id;
	ret = videoenc_push_in_buf(self_id, in_id, p_video_frame, p_usr_out_video_bitstream,  wait_ms);

exit_in_used:
	videoenc_info[dev_idx].port[inport_idx].push_in_used = 0;
	p_enc_priv->push_state = HD_PATH_ID_OPEN;

exit:
	return ret;
}


HD_RESULT hd_videoenc_pull_out_buf(HD_PATH_ID path_id, HD_VIDEOENC_BS *p_video_bitstream, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	char *sign = NULL;
	INT dev_idx, outport_idx;
	VDOENC_INFO_PRIV *p_enc_priv;

	VIDEOENC_CHECK_SELF_ID(self_id);
	VIDEOENC_CHECK_OUT_ID(out_id, FALSE);

	if (p_video_bitstream == NULL) {
		HD_VIDEOENC_ERR("Check HD_VIDEOENC_BS struct is NULL.\n");
		ret = HD_ERR_PARAM;
		goto exit;
	}

	/* check and set state */
	dev_idx = VDOENC_DEVID(self_id);
	outport_idx = VDOENC_OUTID(out_id);

	p_enc_priv = &videoenc_info[dev_idx].priv[outport_idx];
	if (p_enc_priv->pull_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto exit;
	} else if (p_enc_priv->pull_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto exit;
	} else {
		p_enc_priv->pull_state = HD_PATH_ID_ONGOING;
	}

	ret = videoenc_pull_out_buf(self_id, out_id, p_video_bitstream, wait_ms);
	HD_TRIGGER_FLOW("hd_videoenc_pull_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	if (p_video_bitstream->sign) {
		sign = (char *)&p_video_bitstream->sign;
		HD_TRIGGER_FLOW("       sign(%c%c%c%c) vcodec_format(%d) frame_type(%d) pack_num(%d)pa(%p-%p-%p) qp(%d)\n", \
				sign[0], sign[1], sign[2], sign[3], p_video_bitstream->vcodec_format, p_video_bitstream->frame_type, \
				p_video_bitstream->pack_num, (VOID *)p_video_bitstream->video_pack[0].phy_addr, (VOID *)p_video_bitstream->video_pack[1].phy_addr, \
				(VOID *)p_video_bitstream->video_pack[2].phy_addr, (p_video_bitstream->vcodec_format == HD_CODEC_TYPE_H264 || \
						p_video_bitstream->vcodec_format == HD_CODEC_TYPE_H265) ? (INT32) p_video_bitstream->qp : -1);
	} else {
		HD_TRIGGER_FLOW("       vcodec_format(%d) frame_type(%d) pack_num(%d) pa(%p-%p-%p) qp(%d)\n", \
				p_video_bitstream->vcodec_format, p_video_bitstream->frame_type, p_video_bitstream->pack_num, \
				(VOID *)p_video_bitstream->video_pack[0].phy_addr, (VOID *)p_video_bitstream->video_pack[1].phy_addr, \
				(VOID *)p_video_bitstream->video_pack[2].phy_addr, (p_video_bitstream->vcodec_format == HD_CODEC_TYPE_H264 || \
						p_video_bitstream->vcodec_format == HD_CODEC_TYPE_H265) ? (INT32) p_video_bitstream->qp : -1);
	}
	p_enc_priv->pull_state = HD_PATH_ID_OPEN;
exit:
	return ret;
}

HD_RESULT hd_videoenc_release_out_buf(HD_PATH_ID path_id, HD_VIDEOENC_BS *p_video_bitstream)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	VIDEOENC_CHECK_SELF_ID(self_id);
	VIDEOENC_CHECK_OUT_ID(out_id, FALSE);

	if (p_video_bitstream == NULL) {
		HD_VIDEOENC_ERR("Check HD_VIDEOENC_BS struct is NULL.\n");
		ret = HD_ERR_PARAM;
		goto exit;
	}
	if (p_video_bitstream->video_pack[0].phy_addr == 0) {
		HD_VIDEOENC_ERR("Check HD_VIDEOENC_BS video_pack[0].phy_addr is NULL.\n");
		ret = HD_ERR_PARAM;
		goto exit;
	}
	HD_TRIGGER_FLOW("hd_videoenc_release_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_TRIGGER_FLOW("       ddrid(%d) pa(%p)\n", p_video_bitstream->ddr_id, (VOID *)p_video_bitstream->video_pack[0].phy_addr);
	ret = videoenc_release_out_buf(self_id, out_id, p_video_bitstream);

exit:
	return ret;
}


HD_RESULT hd_videoenc_poll_list(HD_VIDEOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms)
{
	if (p_poll == NULL) {
		HD_VIDEOENC_ERR(" The p_poll is NULL.\n");
		return HD_ERR_PARAM;
	}
	if (wait_ms < 0) {
		HD_VIDEOENC_ERR(" Not supported for wait_ms < 0.\n");
		return HD_ERR_PARAM;
	}
	return video_poll_list(p_poll, nr, wait_ms);
}


HD_RESULT hd_videoenc_recv_list(HD_VIDEOENC_RECV_LIST *p_videoenc_bs, UINT32 nr)
{
	if (p_videoenc_bs == NULL) {
		HD_VIDEOENC_ERR(" The p_videoenc_bs is NULL.\n");
		return HD_ERR_PARAM;
	}
	return video_recv_list(p_videoenc_bs, nr);
}



