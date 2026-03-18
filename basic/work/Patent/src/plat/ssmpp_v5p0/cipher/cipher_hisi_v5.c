/*******************************************************************************
 * cipher_hal_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : sunzelin <sunzelin@hikvision.com.cn>
 * Version: V1.0.0  2021年7月15日 Create
 *
 * Description : 硬件平台加密接口
 * Modification:
 *******************************************************************************/
#include "platform_sdk.h"
#include "cipher_hal_inter.h"

CIPHER_PLAT_OPS_S g_stCipherPlatOps;

/*****************************************************************************
                            宏定义
*****************************************************************************/


/*****************************************************************************
                            函数定义
*****************************************************************************/
/**
 * @function:   cipher_hisi_decrypt
 * @brief:      解密
 * @param[in]:  UINT32 U32Handle     
 * @param[in]:  UINT64 u64CipherKey  
 * @param[in]:  UINT64 u64PlainKey   
 * @param[in]:  UINT32 u32Length     
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 cipher_hisi_decrypt(UINT32 U32Handle, UINT64 u64CipherKey, UINT64 u64PlainKey, UINT32 u32Length)
{
    return 0;
}

/**
 * @function:   cipher_hisi_setCipherCfg
 * @brief:      配置加解密模块
 * @param[in]:  VOID *pHandle           
 * @param[in]:  const UINT8 *pu8KeyBuf  
 * @param[in]:  const UINT8 *pu8IVBuf   
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 cipher_hisi_setCipherCfg(UINT32 u32Handle, const UINT8 *pu8KeyBuf, const UINT8 *pu8IVBuf)
{	
	return 0;
}

/**
 * @function:   cipher_hisi_destroyHandle
 * @brief:      销毁加解密模块句柄
 * @param[in]:  VOID *pHandle  
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 cipher_hisi_destroyHandle(UINT32 u32Handle)
{
	return 0;
}

/**
 * @function:   cipher_hisi_createHandle
 * @brief:      创建加解密模块句柄
 * @param[in]:  VOID *pHandle                       
 * @param[in]:  const CIPHER_ATTR_S *pstCipherAttr  
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 cipher_hisi_createHandle(VOID *pHandle, const CIPHER_ATTR_S *pstCipherAttr)
{
	return 0;
}

/**
 * @function:   cipher_hisi_deinit
 * @brief:      安全算法模块去初始化
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 cipher_hisi_deinit(VOID)
{
	return 0;
}

/**
 * @function:   cipher_hisi_init
 * @brief:      安全算法模块初始化
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 cipher_hisi_init(VOID)
{
	return 0;
}

/**
 * @function:   cipher_plat_register
 * @brief:      安全算法功能函数注册
 * @param[in]:  VOID  
 * @param[out]: None
 * @return:     static INT32
 */
VOID cipher_plat_register(void)
{
    g_stCipherPlatOps.init = cipher_hisi_init;
	g_stCipherPlatOps.deinit = cipher_hisi_deinit;
	g_stCipherPlatOps.createHandle = cipher_hisi_createHandle;
	g_stCipherPlatOps.destroyHandle = cipher_hisi_destroyHandle;
	g_stCipherPlatOps.setCipherCfg = cipher_hisi_setCipherCfg;
	g_stCipherPlatOps.decrypt = cipher_hisi_decrypt;

	return;
}


