
#ifndef _DSPTTK_CMD_SVA_H_
#define _DSPTTK_CMD_SVA_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "dspcommon.h"
#include "sal_type.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */


/**
 * @fn      dspttk_sva_set_init_switch
 * @brief   设置SVA算法初始化 打开引擎智能识别
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_INIT
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_init_switch(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_dect_switch
 * @brief   设置危险物检测总开关
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_DECT_SWITCH
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_dect_switch(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_set_confidence_switch
 * @brief   设置置信度显示开关
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_SET_EXT_TYPE
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_confidence_switch(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_set_scale_level
 * @brief   设置报警物osd缩放等级
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_OSD_SCALE_LEAVE
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_scale_level(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_set_osd_border_type
 * @brief   设置OSD 叠框类型
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_OSD_BORDER_TYPE
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_osd_border_type(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_set_alert_color
 * @brief   设置危险品框颜色
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SET_DISPLINETYPE_PRM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_alert_color(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_set_disp_line_type
 * @brief   设置同名危险品合并
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SET_DISPLINETYPE_PRM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_disp_line_type(UINT32 u32Chan);

/**
 * @fn      dspttk_sva_set_alert_sensitivity    
 * @brief   设置灵敏度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_ALERT_SENSITY
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
// UINT64 dspttk_sva_set_alert_sensitivity(UINT32 u32Chan);


#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_CMD_SVA_H_ */

