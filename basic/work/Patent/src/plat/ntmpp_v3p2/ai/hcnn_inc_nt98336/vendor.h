/**********************************************************************************************************************
* 版权信息：版权所有 (c) 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：vendor.h
* 摘    要：9852x Linux下驱动整理
*
** 当前版本：1.1.0
* 作    者：颜奉丽
* 日    期：2020-1-18
* 备    注：整理9852x平台初始化驱动
**********************************************************************************************************************/
#ifndef _NT_VENDOR_H_
#define _NT_VENDOR_H_

#include "hd_common.h"
#include "nt_mem.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define CORE_MAX   16
typedef struct _VENDOR_AI_CORE_UT {
    CHAR   name[8];  ///< name
    UINT32 time;     ///< time
    UINT32 util;     ///< utility
} VENDOR_AI_CORE_UT;
typedef struct _VENDOR_AI_PERF_UT {
    UINT32 core_count;
    VENDOR_AI_CORE_UT core[CORE_MAX];
} VENDOR_AI_PERF_UT;
typedef enum _VENDOR_AI_CFG_ID {

    //set before init
    VENDOR_AI_CFG_PLUGIN_ENGINE       = 0,	///< set/get, config engine pluign
    VENDOR_AI_CFG_PROC_SCHD           = 1,  ///< set/get, config scheduler for all proc handle
    VENDOR_AI_CFG_PROC_COUNT          = 2,  ///< get    , max proc count , using VENDOR_AI_NET_CFG_PROC_COUNT struct
    VENDOR_AI_CFG_IMPL_VERSION        = 3,  ///< get    , implement version, using VENDOR_AI_NET_CFG_IMPL_VERSION struct
    VENDOR_AI_CFG_PROC_CHK_INTERVAL   = 4,  ///< set/get, config timeout check interval (ms) (default 100, should > 100)
    VENDOR_AI_CFG_PERF_UT             = 5,  ///< set/get, support perf ut between set and get
    ENUM_DUMMY4WORD(VENDOR_AI_CFG_ID)
} VENDOR_AI_CFG_ID;


typedef enum {
	VENDOR_AI_PROC_SCHD_FAIR          = 0,  ///< overlapping with fair core (default)
	VENDOR_AI_PROC_SCHD_CAPACITY      = 1,  ///< overlapping with max rate (TODO)
	VENDOR_AI_PROC_SCHD_FIFO          = 2,	///< first in first out
} VENDOR_AI_PROC_SCHD;


typedef struct _VENDOR_AIS_FLOW_MEM_PARM {
	size_t pa;
	size_t va;
	UINT32 size;
} VENDOR_AIS_FLOW_MEM_PARM;


#define VENDOR_OK                   1                              // 与算法保持一致
#define MBYTE					(1024 * 1024)
#define USER_DEF1_MEM_SIZE      (1000 * MBYTE)


int    VENDOR_Init(void);
int    VENDOR_Uninit(void);
int    VENDOR_NUE_Init(void);
int    VENDOR_NUE_Uninit(void);
int    VENDOR_Videoproc_Init(void);
int    VENDOR_Videoproc_Uninit(void);




typedef enum _VENDOR_AI_NET_PARAM_ID {

    VENDOR_AI_NET_PARAM_STATE                   = 0,  ///<     get, proc state, using VENDOR_AI_PROC_STATE type, 1=ready, 2=open, 3=start

    //set before open
    VENDOR_AI_NET_PARAM_CFG_MODEL               = 1,  ///< set/get, config model , using VENDOR_AI_NET_CFG_MODEL struct
    VENDOR_AI_NET_PARAM_CFG_MODEL_RESINFO       = 2,  ///< set	  , set difference resolution model bin, using VENDOR_AI_NET_CFG_MODEL struct
    VENDOR_AI_NET_PARAM_CFG_SHAREMODEL          = 3,  ///< set/get, config share model  , using VENDOR_AI_NET_CFG_MODEL struct
    VENDOR_AI_NET_PARAM_CFG_JOB_OPT             = 4,  ///< set/get, config job optimize , using VENDOR_AI_NET_CFG_JOB_OPT struct
    VENDOR_AI_NET_PARAM_CFG_BUF_OPT             = 5,  ///< set/get, config buf optimize , using VENDOR_AI_NET_CFG_BUF_OPT struct
    VENDOR_AI_NET_PARAM_CFG_USER_POSTPROC       = 6,  ///< set    , config user postproc, using void* (get_user_postproc)(void) to return VENDOR_AI_ENGINE_PLUGIN struct pointer

    //get after open
    VENDOR_AI_NET_PARAM_INFO                    = 20, ///< get	  , get net info, using VENDOR_AI_NET_INFO struct
    VENDOR_AI_NET_PARAM_IN_PATH_LIST            = 21, ///< get    , get path_id of multiple input buffer, using UINT32 array
    VENDOR_AI_NET_PARAM_OUT_PATH_LIST           = 22, ///< get	  , get path_id of multiple output buffer, using UINT32 array
    VENDOR_AI_NET_PARAM_OUT_PATH_BY_NAME        = 24, ///< get	  , get output path by output name, using VENDOR_AI_BUF_NAME struct
    VENDOR_AI_NET_PARAM_LAST_LAYER_LABELNUM     = 25, ///< get	  , get last layer label number

    //set after open ~ before start
    VENDOR_AI_NET_PARAM_CFG_WORKBUF             = 41, ///< get	  , set work memory, using VENDOR_AI_NET_CFG_WORKBUF struct
    VENDOR_AI_NET_PARAM_RES_ID                  = 42, ///< set/get, set resolution by id (0 means default resolution)
    VENDOR_AI_NET_PARAM_RES_DIM                 = 43, ///< set/get, set resolution by dim, using HD_DIM struct
    
    //get/set after start ~ before stop
    //VENDOR_AI_NET_PARAM_IN(layer_id, in_id),        ///< set    , set single input buffer, using VENDOR_AI_BUF struct
                                                      ///< get    , get single input buffer, using VENDOR_AI_BUF struct
    //VENDOR_AI_NET_PARAM_OUT(layer_id, out_id),      ///< get    , get single output buffer, using VENDOR_AI_BUF struct
    //VENDOR_AI_NET_PARAM_DEP(dep_id),                ///< get    , get dependent buffer size, using UINT32
                                                      ///< push_in, push_in dependent buffer for signal "ready to use"
                                                      ///< push_out, push_out dependent buffer for waiting "use finished"
    VENDOR_AI_NET_PARAM_IN_BUF_LIST             = 51, ///< set    , set multiple input buffer, using related output struct array (TODO)
                                                      ///< get    , get multiple input buffer, using related output struct array (TODO)
    VENDOR_AI_NET_PARAM_OUT_BUF_LIST            = 52, ///< get	  , get multiple output buffer, using related output struct array (TODO)
    VENDOR_AI_NET_PARAM_CUSTOM_INFO             = 53, ///< set	  , set user info
	
    ENUM_DUMMY4WORD(VENDOR_AI_NET_PARAM_ID)
} VENDOR_AI_NET_PARAM_ID;

// extern HD_RESULT vendor_ai_cfg_set (VENDOR_AI_CFG_ID cfg_id, void* p_param);
// extern HD_RESULT vendor_ai_cfg_get (VENDOR_AI_CFG_ID cfg_id, void* p_param);
// extern void* vendor_ai_cpu1_get_engine(void);
// extern HD_RESULT vendor_ai_init (VOID);
// extern HD_RESULT vendor_ai_uninit (VOID);
extern HD_RESULT vendor_common_clear_pool_blk(HD_COMMON_MEM_POOL_TYPE pool_type, INT ddrid);

#ifdef __cplusplus
}
#endif

#endif//_NT_VENDOR_H_

