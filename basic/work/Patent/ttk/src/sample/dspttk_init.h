
#ifndef _DSPTTK_INIT_H_
#define _DSPTTK_INIT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sal_type.h"
#include "dspttk_devcfg.h"


/**
 * @fn      dspttk_sys_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 * 
 * @param   [IN] pstDevCfg Bin配置文件参数
 * 
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_sys_init(DSPTTK_DEVCFG *pstDevCfg);

/**
 * @fn      dspttk_xray_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_xray_init(DSPTTK_DEVCFG *pstDevCfg);


/**
 * @fn      dspttk_osd_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_osd_init(DSPTTK_DEVCFG *pstDevCfg);

/**
 * @fn      dspttk_xpack_init
 * @brief   根据Bin配置文件构建共享全局结构体中的参数，并初始化DSP
 *
 * @param   [IN] pstDevCfg Bin配置文件参数
 *
 * @return  SAL_STATUS SAL_SOK：初始化DSP成功，SAL_FAIL：初始化DSP失败
 */
SAL_STATUS dspttk_xpack_init(DSPTTK_DEVCFG *pstDevCfg);

/**
 * @fn      dspttk_get_share_param
 * @brief   获取与DSP共享的全局结构体参数
 *
 * @return  DSPINITPARA* 与DSP共享的全局结构体参数
 */
DSPINITPARA *dspttk_get_share_param(void);


#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_INIT_H_ */

