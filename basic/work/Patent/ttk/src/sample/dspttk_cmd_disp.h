
#ifndef _DSPTTK_CMD_DISP_H_
#define _DSPTTK_CMD_DISP_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspcommon.h"
#include "sal_type.h"

/**
 * @fn      dspttk_disp_set_vi
 * @brief   配置采集预览功能
 *
 * @param   [IN] u32Vodev  显示器编号
 * @param   [IN] u32WinNo  窗口编号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_set_vi(UINT32 u32Vodev, UINT32 u32WinNo);

/**
 * @fn      dspttk_disp_set_region
 * @brief   设置窗口大小
 *
 * @param   [IN] u32Chan 显示通道号 取值范围[0-15] (无效参数)
 * @param   [IN] DSPINITPARA 隐含入参，ism_disp_mode参数获取
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_set_region(UINT32 u32Chan);

/**
 * @fn      dspttk_disp_fb_set
 * @brief   设置FrameBuffer参数,对背景图片进行绑定和show
 *
 * @param   [IN] u32Vodev 显示设备号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_fb_set(UINT32 u32Vodev);

/**
 * @fn      dspttk_disp_fb_mouse_bind
 * @brief   设置鼠标参数
 *
 * @param   [IN] u32Chan vodev号
 * @param   [IN] DSPTTK_SETTING_ENV 隐含入参，设置gui路径
 * @param   [IN] DSPTTK_SETTING_DISP 隐含入参，设置鼠标包括使能、坐标参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_fb_mouse_bind(UINT32 chan);

/**
 * @fn      dspttk_disp_vodev_win_set
 * @brief   设置输出窗口参数
 *
 * @param   [IN] u32Vodev vodev号
 * @param   [IN] DSPTTK_SETTING_DISP 隐含入参,设置窗口通道、初始坐标及窗口大小
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_vodev_win_set(UINT32 u32Vodev);


/**
 * @fn      dspttk_disp_input_src_set
 * @brief   设置输入源参数
 *
 * @param   [IN] u32Num 高16位输出设备号,低16位窗口号
 * @param   [IN] DSPTTK_SETTING_DISP 隐含入参,获取VI通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_input_src_set(UINT32 u32Num);

/**
 * @fn      dspttk_disp_enlarge_global
 * @brief   设置全局放大参数
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Disp默认配置
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_enlarge_global(UINT32 u32Num);

/**
 * @fn      dspttk_disp_enlarge_global
 * @brief   设置局部放大参数
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Disp默认配置
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_disp_enlarge_local(UINT32 u32Num);

/**
 * @fn      dspttk_disp_set_init
 * @brief   对Display Setting作初始化
 *
 * @param   [IN] DSPTTK_DEVCFG 获取Disp默认配置
 *
 * @return  UINT32 32位表示错误号
 */
SAL_STATUS dspttk_disp_set_init(DSPTTK_DEVCFG *pstDevCfg);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_DISP_H_ */

