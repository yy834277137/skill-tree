/**
	@brief Header file of audio decoder internal of library.\n
	This file contains the logger internal functions of library.

	@file audiodec.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _AUDIO_DEC_H_
#define _AUDIO_DEC_H_
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
#define MAX_AUDDEC_DEV          4   ///< Indicate max auddec dev for NT9832x
#define MAX_AUDDEC_PORT         32  ///< Indicate max input port value
#define AUDDEC_DEVID(self_id)   (self_id - HD_DAL_AUDIODEC_BASE)
#define AUDDEC_INID(id)         (id - HD_IN_BASE)
#define AUDDEC_OUTID(id)        (id - HD_OUT_BASE)

typedef enum _AUDDEC_PORT_STATE {
	AUDDEC_STATE_INIT = 0,
	AUDDEC_STATE_UNBOUND = 1,
	AUDDEC_STATE_BOUND = 2,
} AUDDEC_PORT_STATE;

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _AUDDEC_INFO_PRIV {
	struct {
		HD_DAL auddec_self_id;
		HD_IO auddec_out_id;
		AUDDEC_PORT_STATE state;
	} port[MAX_AUDDEC_PORT];
} AUDDEC_INFO_PRIV;

typedef struct _AUDDEC_INFO {
	struct {
		HD_AUDIODEC_PATH_CONFIG    path_config;
		HD_AUDIODEC_IN             dec_in;
		HD_OUT_POOL out_pool;	///< for pool info setting
	} port[MAX_AUDDEC_PORT];
	//private data
	AUDDEC_INFO_PRIV      priv;
} AUDDEC_INFO;

typedef struct _AUDDEC_INTERNAL_PARAM {
	struct {
		HD_AUDIODEC_PATH_CONFIG    *p_path_config;
		HD_AUDIODEC_IN             *p_dec_in;
		HD_OUT_POOL *p_out_pool;
	} port[MAX_AUDDEC_PORT];

} AUDDEC_INTERNAL_PARAM;


/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them

/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT audiodec_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream);
HD_RESULT audiodec_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream, INT32 wait_ms);
HD_RESULT audiodec_release_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream);
HD_RESULT audiodec_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms);
HD_RESULT audiodec_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame);
HD_RESULT audiodec_init_p(void);
HD_RESULT audiodec_uninit_p(void);
INT check_audiodec_params_range(HD_AUDIODEC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len);
AUDDEC_INTERNAL_PARAM *audiodec_get_param_p(HD_DAL self_id);
HD_RESULT audiodec_check_path_id_p(HD_PATH_ID path_id);




#ifdef  __cplusplus
}
#endif


#endif  /* _AUDIO_DEC_H_ */
