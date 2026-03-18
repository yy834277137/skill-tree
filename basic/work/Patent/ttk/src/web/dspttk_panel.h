
#ifndef _DSPTTK_PANEL_H_
#define _DSPTTK_PANEL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspttk_devcfg.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @fn      dspttk_build_panel_init
 * @brief   根据配置文件中的参数构建初始化区块的参数
 * 
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstInitParam 全局配置参数中的初始化参数
 * 
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_init(VOID *pHtmlHandle, DSPTTK_INIT_PARAM *pstInitParam);

/**
 * @fn      dspttk_build_panel_setting_env
 * @brief   根据配置文件中的参数构建Setting-Env区块的参数
 * 
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstEnv 全局配置参数中的Env参数
 * 
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_env(VOID *pHtmlHandle, DSPTTK_SETTING_ENV *pstEnv);

/**
 * @fn      dspttk_build_panel_setting_osd
 * @brief   根据配置文件中的参数构建Setting-OSD区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgOsd 全局配置参数中的OSD参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_osd(VOID *pHtmlHandle, DSPTTK_SETTING_OSD *pstCfgOsd);

/**
 * @fn      dspttk_build_panel_setting_sva
 * @brief   根据配置文件中的参数构建Setting-SVA区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgOsd 全局配置参数中的OSD参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_sva(VOID *pHtmlHandle, DSPTTK_SETTING_SVA *pstCfgSva);

/**
 * @fn      dspttk_build_panel_setting_xray
 * @brief   根据配置文件中的参数构建Setting-XRay区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgXray 全局配置参数中的XRay参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_xray(VOID *pHtmlHandle, DSPTTK_SETTING_XRAY *pstCfgXray);

/**
 * @fn      dspttk_build_panel_setting_disp
 * @brief   根据配置文件中的参数构建Setting-Disp区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgXray 全局配置参数中的Disp参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_disp(VOID *pHtmlHandle, DSPTTK_SETTING_DISP *pstCfgDisp);

/**
 * @fn      dspttk_build_panel_setting_enc
 * @brief   根据配置文件中的参数构建Setting-Disp区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgEnc 全局配置参数中的Disp参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_enc(VOID *pHtmlHandle, DSPTTK_SETTING_ENC *pstCfgEnc);

/**
 * @fn      dspttk_build_panel_setting_dec
 * @brief   根据配置文件中的参数构建Setting-Dec区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgEnc 全局配置参数中的Dec参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_dec(VOID *pHtmlHandle, DSPTTK_SETTING_DEC *pstCfgDec);

/**
 * @fn      dspttk_build_panels
 * @brief   根据配置文件中的参数构建各块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstDevCfg 全局配置参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panels(VOID *pHtmlHandle, DSPTTK_DEVCFG *pstDevCfg);

/**
 * @fn      dspttk_build_panel_setting_xpack
 * @brief   根据配置文件中的参数构建Setting-OSD区块的参数
 *
 * @param   [IN] pHtmlHandle HTML句柄，dspttk_html_create_handle()的返回值
 * @param   [IN] pstCfgOsd 全局配置参数中的OSD参数
 *
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS dspttk_build_panel_setting_xpack(VOID *pHtmlHandle, DSPTTK_SETTING_XPACK *pstCfgXpack);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_PANEL_H_ */

