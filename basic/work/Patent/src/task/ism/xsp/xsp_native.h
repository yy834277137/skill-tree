/**
 * @file   xsp_native.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  XSP成像处理接口头文件
 * @author
 * @date   2020/07/22
 * @note \n History:
   1.Date        : 2020/07/22
     Author      :
     Modification: Created file


 */

#ifndef __XSP_NATIVE_H__
#define __XSP_NATIVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define _HIK_ARM_APP_ /* 定义平台，rin_types.h中对此会做判断 */

#include "sal.h"
#include "xsp_wrap.h"

//void *rin_native_lib_init(S32 xray_vendor, XSP_HANDLE_TYPE handle_type, XSP_VISUAL_ANGLE visual_angle, U32 xraw_width_max, U32 xraw_height_max, U32 board_type);
void *rin_native_lib_init(XSP_HANDLE_TYPE handle_type, XRAY_DEV_ATTR *p_dev_attr);
CHAR *rin_native_get_version(void *p_handle);
SAL_STATUS rin_native_lib_reload_zTable(void *p_handle, XRAY_DEV_ATTR *p_dev_attr);
SAL_STATUS rin_native_set_akey(void *p_handle, S32 s32OptNum, S32 key, S32 value1, S32 value2);
SAL_STATUS rin_native_get_akey(void *p_handle, S32 key, S32 *pImageVal, S32 *pVal1, S32 *pVal2);
SAL_STATUS rin_native_get_all_key(void *p_handle);
SAL_STATUS rin_native_set_model_index(void *p_handle, S32 s32Form, S32 s32Speed);
SAL_STATUS rin_native_set_blank_region(void *p_handle, U32 top_margin, U32 bottom_margin);
SAL_STATUS rin_native_get_correction_data(void *p_handle, XSP_CORRECT_TYPE corr_type, U16 *p_corr_data, U32 *p_corr_size);
SAL_STATUS rin_native_set_correction_data(void *p_handle, XSP_CORRECT_TYPE corr_type, U16 *p_corr_data, U32 corr_raw_width, U32 corr_raw_height);
SAL_STATUS rin_native_set_disp_color(void *p_handle, U32 type);
SAL_STATUS rin_native_set_rotate(void *p_handle, U32 type, XRAY_PROC_TYPE enType);
SAL_STATUS rin_native_get_rotate(void *p_handle, U32 *p_type);
SAL_STATUS rin_native_set_mirror(void *p_handle, U32 type);
SAL_STATUS rin_native_get_mirror(void *p_handle, U32 *p_type);
SAL_STATUS rin_native_set_pseudo_color(void *p_handle, U32 type);
SAL_STATUS rin_native_set_luminance(void *p_handle, S32 type);
SAL_STATUS rin_native_set_default_enhance(void *p_handle, XSP_DEFAULT_STATE defStyle, S32 value);
SAL_STATUS rin_native_set_anti(void *p_handle, U32 enable);
SAL_STATUS rin_native_get_anti(void *p_handle, S32 *s32GetAnti);
SAL_STATUS rin_native_set_lp(void *p_handle, U32 enable);
SAL_STATUS rin_native_set_hp(void *p_handle, U32 enable);
SAL_STATUS rin_native_set_le(void *p_handle, U32 enable);
SAL_STATUS rin_native_set_ue(void *p_handle, U32 enable, U32 value);
SAL_STATUS rin_native_set_ee(void *p_handle, BOOL bEnable, U32 value);
SAL_STATUS rin_native_set_absor(void *p_handle, U32 enable, U32 value);
SAL_STATUS rin_native_set_unpen_alert(void *p_handle, U32 enable, U32 sensitivity, U32 color);
SAL_STATUS rin_native_set_sus_alert(void *p_handle, U32 enable, U32 sensitivity, U32 color);
SAL_STATUS rin_native_set_bg(void *p_handle, U32 thr, U32 fullupdate_ratio);
SAL_STATUS rin_native_set_color_table(void *p_handle, U32 type);
SAL_STATUS rin_native_set_colors_imaging(void *p_handle, U32 type);
SAL_STATUS rin_native_set_deformity_correction(void *p_handle, U32 enable);
SAL_STATUS rin_native_set_dose_correct(void *p_handle, U32 value);
SAL_STATUS rin_native_set_color_adjust(void *p_handle, U32 value);
SAL_STATUS rin_native_set_belt_speed(void *p_handle, FLOAT32 f32BeltSpeed, UINT32 line_per_slice,XRAY_PROC_SPEED enSpeed);
SAL_STATUS rin_native_set_noise_reduction(void *p_handle, U32 level);
SAL_STATUS rin_native_set_contrast(void *p_handle, U32 level);
SAL_STATUS rin_native_set_gamma(void *p_handle, U32 mode_switch, U32 value);
SAL_STATUS rin_native_set_aixsp_rate(void *p_handle, U32 value);

SAL_STATUS rin_native_set_stretch_ratio(void *p_handle, U32 stretch_ratio);
SAL_STATUS rin_native_set_le_th_range(void *p_handle, U32 lower_limit, U32 upper_limit);
SAL_STATUS rin_native_set_dt_gray_range(void *p_handle, U32 lower_limit, U32 upper_limit);
SAL_STATUS rin_native_set_bkg_color(void *p_handle, U32 u32BkgColor);
SAL_STATUS rin_native_set_coldhot_threshold(void *p_handle, U32 level);
SAL_STATUS rin_native_get_zdata_version(void *p_handle, U32 *type);

SAL_STATUS rin_native_rtpreview_process(void *p_handle, XSP_RTPROC_IN st_xraw_realtime_original, XSP_RTPROC_OUT *pst_xraw_realtime_normalized);
SAL_STATUS rin_native_playback_process(void *p_handle, XSP_PBPROC_IN *pst_pbproc_in, XSP_PBPROC_OUT *pstPbProcOut);
SAL_STATUS rin_native_transform_process(void *p_handle, XSP_TRANS_IN *pst_trans_in, XSP_TRANS_OUT *pstTransOut);
SAL_STATUS rin_native_indentify_process(void *p_handle, XSP_IDT_IN *pstIdtIn, XSP_IDENTIFY_S **pstXspIdtResult);


#ifdef __cplusplus
}
#endif

#endif

