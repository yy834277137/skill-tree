#ifndef _DSPTTK_CMD_ENC_H_
#define _DSPTTK_CMD_ENC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspcommon.h"
#include "sal_type.h"


/**
 * @fn      dspttk_enc_enable_set
 * @brief   开启编码
 *
 * @param   [IN] u32StreamType 码流类型
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流开启关闭状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_enable_set(UINT32 u32StreamType);

/**
 * @fn      dspttk_enc_param_set
 * @brief   设置编码参数
 *
 * @param   [IN] u32StreamType 通道号
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流开启关闭状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */

UINT64 dspttk_enc_param_set(UINT32 u32StreamType);

/**
 * @fn      dspttk_enc_save_set
 * @brief   编码数据保存设置
 *
 * @param   [IN] u32StreamType 通道号
 * @param   [IN] DSPTTK_SETTING_ENC 隐含入参, 获取码流开启关闭状态
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_enc_save_set(UINT32 u32StreamType);

/**
 * @fn      dspttk_enc_set_init
 * @brief   对Encode Setting作初始化
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Encode默认配置
 *
 * @return  UINT32 32位表示错误号
 */
SAL_STATUS dspttk_enc_set_init(DSPTTK_DEVCFG *pstDevCfg);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_ENC_H_ */
