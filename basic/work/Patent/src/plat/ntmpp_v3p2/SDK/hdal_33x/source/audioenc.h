/**
	@brief Header file of audio encoder internal of library.\n
	This file contains the logger internal functions of library.

	@file audioenc.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _AUDIO_ENC_H_
#define _AUDIO_ENC_H_
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
#define MAX_AUDENC_DEV          4   ///< Indicate max audenc dev for NT9832x
#define MAX_AUDENC_PORT         100  ///< Indicate max input port value, 20ch * 5path
#define AUDENC_DEVID(self_id)   (self_id - HD_DAL_AUDIOENC_BASE)
#define AUDENC_INID(id)         (id - HD_IN_BASE)
#define AUDENC_OUTID(id)        (id - HD_OUT_BASE)

typedef enum _AUDENC_PORT_STATE {
	AUDENC_STATE_INIT = 0,
	AUDENC_STATE_UNBOUND = 1,
	AUDENC_STATE_BOUND = 2,
} AUDENC_PORT_STATE;

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _AUDENC_INFO_PRIV {
	struct {
		HD_DAL audcap_self_id;
		HD_IO audcap_out_id;
		AUDENC_PORT_STATE state;
	} port[MAX_AUDENC_PORT];
} AUDENC_INFO_PRIV;


typedef struct _AUDENC_INFO {
	struct {
		HD_AUDIOENC_PATH_CONFIG    path_config;
		HD_AUDIOENC_IN             enc_in;
		HD_AUDIOENC_OUT            enc_out;
		HD_OUT_POOL out_pool;	///< for pool info setting
	} port[MAX_AUDENC_PORT];
	//private data
	AUDENC_INFO_PRIV      priv;
} AUDENC_INFO;

typedef struct _AUDENC_INTERNAL_PARAM {
	struct {
		HD_AUDIOENC_PATH_CONFIG    *p_path_config;
		HD_AUDIOENC_IN             *p_enc_in;
		HD_AUDIOENC_OUT            *p_enc_out;
		HD_OUT_POOL *p_out_pool;
		UINT linefd;
	} port[MAX_AUDENC_PORT];
} AUDENC_INTERNAL_PARAM;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them

/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT audioenc_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame);
HD_RESULT audioenc_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms);
HD_RESULT audioenc_release_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame);
HD_RESULT audioenc_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_BS *p_audio_bitstream, INT32 wait_ms);
HD_RESULT audioenc_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_BS *p_audio_bitstream);
HD_RESULT audioenc_init_p(void);
HD_RESULT audioenc_uninit_p(void);
INT check_audioenc_params_range(HD_AUDIOENC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len);
AUDENC_INTERNAL_PARAM *audioenc_get_param_p(HD_DAL self_id);
HD_RESULT audioenc_check_path_id_p(HD_PATH_ID path_id);


#ifdef  __cplusplus
}
#endif


#endif  /* _AUDIO_ENC_H_ */
