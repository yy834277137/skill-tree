/**
 * @file   cipher_hal_inter.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _CIPHER_HAL_INTER_H_
#define _CIPHER_HAL_INTER_H_

#include "sal.h"
#include "../hal_inc_exter/cipher_hal.h"


typedef struct _CIPHER_PLAT_OPS_
{
    INT32 (*init)(VOID);
	INT32 (*deinit)(VOID);
	INT32 (*createHandle)(VOID *pHandle, const CIPHER_ATTR_S *pstCipherAttr);
	INT32 (*destroyHandle)(UINT32 u32Handle);
	INT32 (*setCipherCfg)(UINT32 u32Handle, const UINT8 *pu8KeyBuf, const UINT8 *pu8IVBuf);
	INT32 (*decrypt)(UINT32 U32Handle, UINT64 u64CipherKey, UINT64 u64PlainKey, UINT32 u32Length);
} CIPHER_PLAT_OPS_S;

/**
 * @function:   cipher_plat_register
 * @brief:      安全算法功能函数注册
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     static INT32
 */
VOID cipher_plat_register(void);


#endif


