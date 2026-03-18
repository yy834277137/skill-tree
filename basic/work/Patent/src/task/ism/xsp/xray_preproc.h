
#ifndef __XRAY_PREPROC_H__
#define __XRAY_PREPROC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sal.h"
#include "ximg_proc.h"

/* ============================ 宏/枚举 ============================ */


/* ========================= 结构体/联合体 ========================== */


/* =================== 函数申明，static && extern =================== */
/**
 * @fn      xray_bg_auto_corr
 * @brief   背景自动校正处理
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] pstXrawIn 采传的实时预览XRAW条带数据，包括XRAY_TYPE_NORMAL与XRAY_TYPE_PSEUDO_BLANK两种类型
 * @param   [IN] bPseudo 是否为伪空白条带，即XRAY_TYPE_PSEUDO_BLANK类型数据
 * @param   [IN] u32SliceNo 条带号，仅用于调试
 * @param   [OUT] pu16FullCorr 自适应的满载模板，仅1行数据
 * 
 * @return  SAL_STATUS SAL_SOK：满载模板有更新，SAL_FAIL：满载模板无更新
 */
SAL_STATUS xray_bg_auto_corr(void *pHandle, XIMAGE_DATA_ST *pstXrawIn, BOOL bPseudo, UINT32 u32SliceNo, UINT16 *pu16FullCorr);

/**
 * @fn      xray_bgac_update_maunal_fullcorr
 * @brief   更新手动满载校正模板
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] pstXrawIn 手动满载校正数据
 */
VOID xray_bgac_update_maunal_fullcorr(void *pHandle, XIMAGE_DATA_ST *pstXrawIn);

/**
 * @fn      xray_bgac_update_lrblank
 * @brief   更新XRAW数据左右置白区域，置白区域不做统计
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] u32BlankLeft 左置白区域，基于XRAW数据
 * @param   [IN] u32BlankRight 右置白区域，基于XRAW数据
 */
VOID xray_bgac_update_lrblank(void *pHandle, UINT32 u32BlankLeft, UINT32 u32BlankRight);

/**
 * @fn      xray_bgac_set_sensitivity
 * @brief   设置背景自动校正灵敏度，算法内部默认值为50
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] u32Sensitivity 灵敏度，范围[0, 100]
 */
VOID xray_bgac_set_sensitivity(void *pHandle, UINT32 u32Sensitivity);

/**
 * @fn      xray_bgac_get_slice_prob
 * @brief   获取指定条带是空白或包裹的概率，大于0倾向于包裹，小于0倾向于空白
 * 
 * @param   [IN] pHandle 背景自动校正算法句柄，xray_bgac_init()返回
 * @param   [IN] u32SliceNo 条带号
 * 
 * @return  INT32 是空白或包裹的概率，范围[-100, 100]，大于0倾向于包裹，小于0倾向于空白
 */
INT32 xray_bgac_get_slice_prob(void *pHandle, UINT32 u32SliceNo);

/**
 * @fn      xray_bgac_init
 * @brief   背景自动校正算法初始化
 * 
 * @return  VOID* 背景自动校正算法句柄，NULL-初始化失败
 */
VOID *xray_bgac_init(VOID);

/* =================== 全局变量，static && extern =================== */


#ifdef __cplusplus
}
#endif

#endif

