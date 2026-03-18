/**
	@brief Header file of audio capture internal of library.\n
	This file contains the logger internal functions of library.

	@file audiocap.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _AUDIO_CAP_H_
#define _AUDIO_CAP_H_
#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "bind.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define MAX_AUDCAP_DEV          HD_DAL_AUDIOCAP_COUNT   ///< Indicate max audcap dev
#define MAX_AUDCAP_IN_PORT      1   ///< Indicate max input port value
#define MAX_AUDCAP_OUT_PORT     16  ///< Indicate max output port value
#define AUDCAP_DEVID(self_id)   (self_id - HD_DAL_AUDIOCAP_BASE)
#define AUDCAP_INID(id)         (id - HD_IN_BASE)
#define AUDCAP_OUTID(id)        (id - HD_OUT_BASE)

typedef enum _AUDCAP_PORT_STATE {
	AUDCAP_STATE_INIT = 0,
	AUDCAP_STATE_UNBOUND = 1,
	AUDCAP_STATE_BOUND = 2,
} AUDCAP_PORT_STATE;

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _AUDCAP_INFO_PRIV {
	INT fd;
	//UINT queue_handle;
	uintptr_t queue_handle;
	struct {
		AUDCAP_PORT_STATE state;
		UINT vch;
	} port[MAX_AUDCAP_OUT_PORT];
	UINT ssp_no;
} AUDCAP_INFO_PRIV;

typedef struct _AUDCAP_INFO {
	HD_AUDIOCAP_SYSCAPS     sys_caps;
	HD_AUDIOCAP_SYSINFO     sys_info;
	HD_AUDIOCAP_DEV_CONFIG  dev_config;
	HD_AUDIOCAP_DRV_CONFIG  drv_config;

	HD_AUDIOCAP_IN          param_in;
	struct {
		HD_AUDIOCAP_OUT     param_out;
		HD_AUDIOCAP_AEC     param_aec;
		HD_AUDIOCAP_ANR     param_anr;
	} port[MAX_AUDCAP_OUT_PORT];
	HD_AUDIOCAP_VOLUME  param_volume;
	HD_OUT_POOL out_pool;	///< for pool info setting

	//private data
	INT dev_open_count;
	AUDCAP_INFO_PRIV      priv;
} AUDCAP_INFO;

typedef struct _AUDCAP_INTERNAL_PARAM {
	AUDCAP_INFO_PRIV    *p_priv_info;

	HD_AUDIOCAP_DEV_CONFIG  *p_dev_config;
	HD_AUDIOCAP_DRV_CONFIG  *p_drv_config;

	HD_AUDIOCAP_IN          *p_param_in;
	struct {
		HD_AUDIOCAP_OUT     *p_param_out;
		HD_AUDIOCAP_AEC     *p_param_aec;
		HD_AUDIOCAP_ANR     *p_param_anr;
		UINT linefd;
	} port[MAX_AUDCAP_OUT_PORT];
	HD_AUDIOCAP_VOLUME  *p_param_volume;
	HD_OUT_POOL *p_out_pool;
} AUDCAP_INTERNAL_PARAM;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them

/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT audiocap_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms);
HD_RESULT audiocap_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame);
HD_RESULT audiocap_module_init(HD_AUDIOCAP_DRV_CONFIG *p_drv_config);
HD_RESULT audiocap_init_p(void);
HD_RESULT audiocap_uninit_p(void);
HD_RESULT audiocap_init_dev_p(UINT dev);
HD_RESULT audiocap_uninit_dev_p(UINT dev);
INT check_audiocap_params_range(HD_AUDIOCAP_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len);
AUDCAP_INTERNAL_PARAM *audiocap_get_param_p(HD_DAL self_id);
HD_RESULT audiocap_check_path_id_p(HD_PATH_ID path_id);
VOID audiocap_get_capability(HD_DAL self_id, HD_AUDIOCAP_SYSCAPS *p_aucap);
UINT audiocap_get_devcount(VOID);
UINT audiocap_get_ch_count(UINT dev_idx);
HD_RESULT audiocap_clear_buf(INT dev_idx);
HD_RESULT audiocap_get_basic_param(HD_PATH_ID path_id, HD_AUDIOCAP_IN *p_param_in);
HD_RESULT audiocap_set_basic_param(HD_PATH_ID path_id, HD_AUDIOCAP_IN *p_param_in);

#ifdef  __cplusplus
}
#endif


#endif  /* _AUDIO_ENC_H_ */
