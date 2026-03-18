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
    return HI_UNF_CIPHER_Decrypt(U32Handle, u64CipherKey, u64PlainKey, u32Length);
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
	HI_UNF_CIPHER_CTRL_S stCipherCtrl = {0};
	
	stCipherCtrl.enAlg = HI_UNF_CIPHER_ALG_AES;
	stCipherCtrl.enWorkMode = HI_UNF_CIPHER_WORK_MODE_ECB;
	stCipherCtrl.enBitWidth = HI_UNF_CIPHER_BIT_WIDTH_128BIT;
	stCipherCtrl.enKeyLen = HI_UNF_CIPHER_KEY_AES_128BIT;
	stCipherCtrl.bKeyByCA = HI_TRUE;
	stCipherCtrl.enCaType = HI_UNF_CIPHER_KEY_SRC_KLAD_1;
	
	if(stCipherCtrl.enWorkMode != HI_UNF_CIPHER_WORK_MODE_ECB)
	{
		//must set for CBC , CFB mode
		stCipherCtrl.stChangeFlags.bit1IV = 1;  
		memcpy(stCipherCtrl.u32IV, pu8IVBuf, 16);
	}
	
	memcpy(stCipherCtrl.u32Key, pu8KeyBuf, 16);
	
	return HI_UNF_CIPHER_ConfigHandle(u32Handle, &stCipherCtrl);
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
	return HI_UNF_CIPHER_DestroyHandle(u32Handle);
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
	HI_UNF_CIPHER_ATTS_S stCipherAttr = {0};

	stCipherAttr.enCipherType = pstCipherAttr->enCipherType;

	return HI_UNF_CIPHER_CreateHandle(pHandle, &stCipherAttr);
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
	return HI_UNF_CIPHER_DeInit();
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
	return HI_UNF_CIPHER_Init();
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


