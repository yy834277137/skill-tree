/**
	@brief Header file of audio out internal of library.\n
	This file contains the logger internal functions of library.

	@file audioout.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _AUDIO_OUT_H_
#define _AUDIO_OUT_H_
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
#define MAX_AUDOUT_DEV          HD_DAL_AUDIOOUT_COUNT   ///< Indicate max audout dev
#define MAX_AUDOUT_PORT         2  ///< Indicate max input port value
#define AUDOUT_DEVID(self_id)   (self_id - HD_DAL_AUDIOOUT_BASE)
#define AUDOUT_INID(id)         (id - HD_IN_BASE)
#define AUDOUT_OUTID(id)        (id - HD_OUT_BASE)

typedef enum _AUDOUT_PORT_STATE {
	AUDOUT_STATE_INIT = 0,
	AUDOUT_STATE_UNBOUND = 1,
	AUDOUT_STATE_BOUND = 2,
} AUDOUT_PORT_STATE;

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _AUDOUT_INFO_PRIV {
	INT fd;
	//UINT queue_handle;
	uintptr_t queue_handle;
	struct {
		AUDOUT_PORT_STATE state;
		UINT vch;
	} port[MAX_AUDOUT_PORT];
	UINT ssp_no;
	HD_PATH_ID_STATE path_id_state;
} AUDOUT_INFO_PRIV;


typedef struct _AUDOUT_INFO {
	HD_AUDIOOUT_DEV_CONFIG    dev_config;        //control port only
	HD_AUDIOOUT_DRV_CONFIG    drv_config;    //control port only

	HD_AUDIOOUT_IN        param_in[MAX_AUDOUT_PORT];
	HD_AUDIOOUT_OUT       param_out;
	HD_AUDIOOUT_VOLUME    param_volume;
	HD_OUT_POOL out_pool;	///< for pool info setting

	//private data
	AUDOUT_INFO_PRIV      priv;
} AUDOUT_INFO;

typedef struct _AUDOUT_INTERNAL_PARAM {
	AUDOUT_INFO_PRIV      *p_priv_info;

	HD_AUDIOOUT_IN        *p_param_in[MAX_AUDOUT_PORT];
	HD_AUDIOOUT_OUT       *p_param_out;
	HD_AUDIOOUT_VOLUME    *p_param_volume;
	HD_OUT_POOL *p_out_pool;
} AUDOUT_INTERNAL_PARAM;


/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them

/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT audioout_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame);
HD_RESULT audioout_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms);
HD_RESULT audioout_release_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame);
HD_RESULT audioout_module_init(HD_AUDIOOUT_DRV_CONFIG *p_drv_config);
HD_RESULT audioout_init_p(void);
HD_RESULT audioout_uninit_p(void);
HD_RESULT audioout_init_dev_p(UINT dev);
HD_RESULT audioout_uninit_dev_p(UINT dev);
INT check_audioout_params_range(HD_AUDIOOUT_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len);
AUDOUT_INTERNAL_PARAM *audioout_get_param_p(HD_DAL self_id);
HD_RESULT audioout_check_path_id_p(HD_PATH_ID path_id);
VOID audioout_get_capability(HD_DAL self_id, HD_AUDIOOUT_SYSCAPS *p_auout);
UINT audioout_get_devcount(VOID);
HD_RESULT audioout_clear_buf(INT dev_idx);
HD_RESULT audioout_get_basic_param(HD_PATH_ID path_id, HD_AUDIOOUT_IN *p_param_in, HD_AUDIOOUT_OUT *p_param_out);
HD_RESULT audioout_set_basic_param(HD_PATH_ID path_id, HD_AUDIOOUT_IN *p_param_in, HD_AUDIOOUT_OUT *p_param_out);
HD_RESULT audioout_set_param(HD_PATH_ID path_id, HD_AUDIOOUT_VOLUME *p_volume);


#ifdef  __cplusplus
}
#endif


#endif  /* _AUDIO_OUT_H_ */
