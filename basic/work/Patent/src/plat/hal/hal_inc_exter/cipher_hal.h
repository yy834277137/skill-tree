/**
 * @file   cipher_hal.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  加密模块平台抽象层
 * @author sunzelin
 * @date   2021.7.15
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _CIPHER_HAL_H_
#define _CIPHER_HAL_H_

#include "sal.h"

typedef enum _CIPHER_TYPE_
{
    CIPHER_TYPE_NORMAL  = 0x0,
    CIPHER_TYPE_COPY_AVOID,
    CIPHER_TYPE_BUTT,
    CIPHER_TYPE_INVALID = 0xffffffff,
} CIPHER_TYPE_E;

typedef struct _CIPHER_ATTR_
{
	CIPHER_TYPE_E enCipherType;          /* 加密类型 */
} CIPHER_ATTR_S;

/**
 * @function:   cipher_hal_decrypt
 * @brief:      解密
 * @param[in]:  UINT32 U32Handle     
 * @param[in]:  UINT64 u64CipherKey  
 * @param[in]:  UINT64 u64PlainKey   
 * @param[in]:  UINT32 u32Length     
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_decrypt(UINT32 U32Handle, UINT64 u64CipherKey, UINT64 u64PlainKey, UINT32 u32Length);

/**
 * @function:   cipher_hal_setCipherCfg
 * @brief:      配置加解密模块参数
 * @param[in]:  UINT32 u32Handle        
 * @param[in]:  const UINT8 *pu8KeyBuf  
 * @param[in]:  const UINT8 *pu8IVBuf   
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_setCipherCfg(UINT32 u32Handle, const UINT8 *pu8KeyBuf, const UINT8 *pu8IVBuf);

/**
 * @function:   cipher_hal_destroyHandle
 * @brief:      加解密模块销毁句柄
 * @param[in]:  UINT32 u32Handle  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_destroyHandle(UINT32 u32Handle);

/**
 * @function:   cipher_hal_createHandle
 * @brief:      加解密算法创建句柄
 * @param[in]:  VOID *pHandle                       
 * @param[in]:  const CIPHER_ATTR_S *pstCipherAttr  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_createHandle(VOID *pHandle, const CIPHER_ATTR_S *pstCipherAttr);

/**
 * @function:   cipher_hal_deinit
 * @brief:      加解密算法模块去初始化
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_deinit(VOID);

/**
 * @function:   cipher_hal_init
 * @brief:      加解密算法模块初始化
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_init(VOID);

#endif

