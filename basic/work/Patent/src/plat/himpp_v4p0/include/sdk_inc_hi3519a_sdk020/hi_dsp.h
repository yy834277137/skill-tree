/******************************************************************************

  Copyright (C), 2001-2017, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : hi_dsp.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software (SVP) group
  Created       : 2017/10/17
  Description   :
  History       :
  1.Date        : 2017/10/17
    Author      :
    Modification: Created file
******************************************************************************/
#ifndef _HI_DSP_H_
#define _HI_DSP_H_

#include "hi_comm_svp.h"
/* if sdk environment,include hi_common.h,else typedef HI_S32 SVP_DSP_HANDLE */
#include "hi_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**************************************SVP_DSP Error Code*************************************************/
typedef enum hiEN_SVP_DSP_ERR_CODE_E {
    ERR_SVP_DSP_SYS_TIMEOUT = 0x40,   /* SVP_DSP process timeout */
    ERR_SVP_DSP_QUERY_TIMEOUT = 0x41, /* SVP_DSP query timeout */
    ERR_SVP_DSP_OPEN_FILE = 0x42,     /* SVP_DSP open file error */
    ERR_SVP_DSP_READ_FILE = 0x43,     /* SVP_DSP read file error */

    ERR_SVP_DSP_BUTT
} EN_SVP_DSP_ERR_CODE_E;
/* Invalid device ID */
#define HI_ERR_SVP_DSP_INVALID_DEVID HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
/* Invalid channel ID */
#define HI_ERR_SVP_DSP_INVALID_CHNID HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_CHNID)
/* At least one parameter is illegal. For example, an illegal enumeration value exists. */
#define HI_ERR_SVP_DSP_ILLEGAL_PARAM HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
/* The channel exists. */
#define HI_ERR_SVP_DSP_EXIST HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_EXIST)
/* The UN exists. */
#define HI_ERR_SVP_DSP_UNEXIST HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_UNEXIST)
/* A null point is used. */
#define HI_ERR_SVP_DSP_NULL_PTR HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
/* Try to enable or initialize the system, device, or channel before configuring attributes. */
#define HI_ERR_SVP_DSP_NOT_CONFIG HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_CONFIG)
/* The operation is not supported currently. */
#define HI_ERR_SVP_DSP_NOT_SURPPORT HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)
/* The operation, changing static attributes for example, is not permitted. */
#define HI_ERR_SVP_DSP_NOT_PERM HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_PERM)
/* A failure caused by the malloc memory occurs. */
#define HI_ERR_SVP_DSP_NOMEM HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
/* A failure caused by the malloc buffer occurs. */
#define HI_ERR_SVP_DSP_NOBUF HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)
/* The buffer is empty. */
#define HI_ERR_SVP_DSP_BUF_EMPTY HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
/* No buffer is provided for storing new data. */
#define HI_ERR_SVP_DSP_BUF_FULL HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
/* The system is not ready because it may be not initialized or loaded.
 * The error code is returned when a device file fails to be opened. */
#define HI_ERR_SVP_DSP_NOTREADY HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
/* The source address or target address is incorrect during the operations
such as calling copy_from_user or copy_to_user. */
#define HI_ERR_SVP_DSP_BADADDR HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_BADADDR)
/* The resource is busy during the operations such as destroying a VENC channel without deregistering it. */
#define HI_ERR_SVP_DSP_BUSY HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
/* SVP_DSP process timeout */
#define HI_ERR_SVP_DSP_SYS_TIMEOUT HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_SYS_TIMEOUT)
/* SVP_DSP query timeout */
#define HI_ERR_SVP_DSP_QUERY_TIMEOUT HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_QUERY_TIMEOUT)
/* SVP_DSP open file error */
#define HI_ERR_SVP_DSP_OPEN_FILE HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_OPEN_FILE)
/* SVP_DSP read file error */
#define HI_ERR_SVP_DSP_READ_FILE HI_DEF_ERR(HI_ID_SVP_DSP, EN_ERR_LEVEL_ERROR, ERR_SVP_DSP_READ_FILE)

/* SVP_DSP core id */
typedef enum hiSVP_DSP_ID_E {
    SVP_DSP_ID_0 = 0x0,

    SVP_DSP_ID_BUTT
} SVP_DSP_ID_E;

/* SVP_DSP  priority */
typedef enum hiSVP_DSP_PRI_E {
    SVP_DSP_PRI_0 = 0x0,
    SVP_DSP_PRI_1 = 0x1,
    SVP_DSP_PRI_2 = 0x2,

    SVP_DSP_PRI_BUTT
} SVP_DSP_PRI_E;

/* SVP_DSP memory type */
typedef enum hiSVP_DSP_MEM_TYPE_E {
    SVP_DSP_MEM_TYPE_SYS_DDR_DSP_0 = 0x0,
    SVP_DSP_MEM_TYPE_IRAM_DSP_0 = 0x1,
    SVP_DSP_MEM_TYPE_DRAM_0_DSP_0 = 0x2,
    SVP_DSP_MEM_TYPE_DRAM_1_DSP_0 = 0x3,

    SVP_DSP_MEM_TYPE_BUTT
} SVP_DSP_MEM_TYPE_E;

/* SVP_DSP  cmd */
typedef enum hiSVP_DSP_CMD_E {
    SVP_DSP_CMD_INIT = 0x0,
    SVP_DSP_CMD_EXIT = 0x1,
    SVP_DSP_CMD_ERODE_3X3 = 0x2,
    SVP_DSP_CMD_DILATE_3X3 = 0x3,
    
    // 孟泽民增加，DSP处理网络入口标识
    HCNN_VP6_IPCM_PROC	   = 0x4,

    // 黄斌增加，AFFINE处理网络入口标识
    HCNN_VP6_AFFINE_PROC   = 0x5,

    // 校验bin版本用
    HIA_CHECK_VER_PROC     = 0x6,

    // 比对算子
    HIA_COMPARE_PROC       = 0x8,

    // 关键点匹配算子
    HCNN_VP6_KEY_MATCH_PROC = 0x9,
    // 预研部四个算子
    HKA_DPT_DSPCMD_DEP2BPC = 0x0A, 
    HKA_DPT_DSPCMD_RESIZE  = 0x0B, 
    HKA_DPT_DSPCMD_PSEUDO  = 0x0C,
    HKA_DPT_DSPCMD_ALIGN   = 0x0D,

#ifdef CONFIG_HI_PHOTO_SUPPORT
    SVP_DSP_CMD_PHOTO_PROC,
#endif

    SVP_DSP_CMD_DRAW_LINE = 0x100,      /* DSP核画线 */
    SVP_DSP_CMD_DRAW_OSD,               /* DSP核叠OSD */
    SVP_DSP_CMD_DRAW_RECT,              /* DSP核画框 */
    SVP_DSP_CMD_LINE_OSD,               /* DSP核画线和叠OSD */
    SVP_DSP_CMD_IPCM_TEST,              /* 测试IPCM */

    SVP_DSP_CMD_BUTT
} SVP_DSP_CMD_E;

/* SVP_DSP ARM->DSP request message */
typedef struct hiSVP_DSP_MESSAGE_S {
    HI_U32 u32CMD;     /* CMD ID, user-defined SVP_DSP_CMD_BUTT + */
    HI_U32 u32MsgId;   /* Message ID */
    HI_U64 u64Body;    /* Message body */
    HI_U32 u32BodyLen; /* Length of pBody */
} SVP_DSP_MESSAGE_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* _HI_DSP_H_ */
