/**
 * @file   cipher_hal.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  加解密算法模块平台抽象层
 * @author sunzelin
 * @date   2021.7.15
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#include "cipher_hal_inter.h"
#include "hal_inc_exter/cipher_hal.h"

extern CIPHER_PLAT_OPS_S g_stCipherPlatOps;

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
INT32 cipher_hal_decrypt(UINT32 U32Handle, UINT64 u64CipherKey, UINT64 u64PlainKey, UINT32 u32Length)
{
    return g_stCipherPlatOps.decrypt(U32Handle, u64CipherKey, u64PlainKey, u32Length);
}

/**
 * @function:   cipher_hal_setCipherCfg
 * @brief:      配置加解密模块参数
 * @param[in]:  UINT32 u32Handle        
 * @param[in]:  const UINT8 *pu8KeyBuf  
 * @param[in]:  const UINT8 *pu8IVBuf   
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_setCipherCfg(UINT32 u32Handle, const UINT8 *pu8KeyBuf, const UINT8 *pu8IVBuf)
{
    return g_stCipherPlatOps.setCipherCfg(u32Handle, pu8KeyBuf, pu8IVBuf);
}

/**
 * @function:   cipher_hal_destroyHandle
 * @brief:      加解密模块销毁句柄
 * @param[in]:  UINT32 u32Handle  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_destroyHandle(UINT32 u32Handle)
{
    return g_stCipherPlatOps.destroyHandle(u32Handle);
}

/**
 * @function:   cipher_hal_createHandle
 * @brief:      加解密算法创建句柄
 * @param[in]:  VOID *pHandle                       
 * @param[in]:  const CIPHER_ATTR_S *pstCipherAttr  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_createHandle(VOID *pHandle, const CIPHER_ATTR_S *pstCipherAttr)
{
	if (NULL == pHandle || NULL == pstCipherAttr)
	{
		SAL_LOGE("invalid ptr %p %p \n", pHandle, pstCipherAttr);
		return SAL_FAIL;
	}

	return g_stCipherPlatOps.createHandle(pHandle, pstCipherAttr);
}

/**
 * @function:   cipher_hal_deinit
 * @brief:      加解密算法模块去初始化
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_deinit(VOID)
{
    return g_stCipherPlatOps.deinit();
}


#ifdef DSP_WEAK_FUNC

CIPHER_PLAT_OPS_S g_stCipherPlatOps;
/**
 * @function   cipher_plat_register
 * @brief      弱函数当有平台不支持osd时保证编译通过
 * @param[in]  void  
 * @param[out] None
 * @return     __attribute__((weak) ) VOID
 */
__attribute__((weak) ) VOID cipher_plat_register(void)
{
   
    g_stCipherPlatOps.init = NULL;
    return;
}
#endif


/**
 * @function:   cipher_hal_init
 * @brief:      加解密算法模块初始化
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     INT32
 */
INT32 cipher_hal_init(VOID)
{
    cipher_plat_register();
    if(NULL == g_stCipherPlatOps.init)
    {
        return SAL_FAIL;
    }
    
    return g_stCipherPlatOps.init();
}

