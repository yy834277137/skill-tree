/**
 * @file    xsp_wrap.c
 * @brief   不同XSP算法实现接口的注册，外部以函数指针的方式调用，无需关心具体XSP接口的调用流程与参数 
 * @warning 不同XSP算法实现有较大差异，会存在：①某一功能有些算法有，有些算法没有，②同一功能各算法要求的参数有差异 
 *          此时以“最大化”为处理原则：即不支持的功能需要写虚函数，函数实现可直接返回，确保函数指针不为空；
 *          若参数个数存有差异，以最多参数为准，多余的参数可忽略，并在注释中说明该参数不使用
 */

#include "xsp_wrap.h"
#include "xsp_native.h"
#include "system_prm_api.h"

/* ============================ 宏/枚举 ============================ */
#line __LINE__ "xsp_wrap.c"

/* ========================= 结构体/联合体 ========================== */


/* =================== 函数申明，static && extern =================== */


/* =================== 全局变量，static && extern =================== */
XSP_LIB_API g_xsp_lib_api;


/**
 * @fn      hka_calc_img_col_avg_2b
 * @brief   计算图像列均值，仅适用于图像单个像素数据占2个字节的情况 
 *  
 * @param   p_col_avg[OUT] 图像列均值
 * @param   p_img_data[IN] 图像数据Buffer
 * @param   width[IN] 图像宽
 * @param   height[IN] 图像高 
 *  
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS hka_calc_img_col_avg_2b(U16 *p_col_avg, U16 *p_img_data, U32 width, U32 height)
{
    U32 i = 0, j = 0, k = 0;
    U32 *p_col_sum = NULL;
    U32 sum_buf_size = 0;

    if (NULL == p_img_data || NULL == p_col_avg)
    {
        return SAL_FAIL;
    }

    if (0 == width || 0 == height)
    {
        return SAL_FAIL;
    }

    /* 当高度大于65535时，求每列的和会有溢出风险，直接返回 */
    if (height > 0xFFFFFFFF / 0xFFFF)
    {
        return SAL_FAIL;
    }

    sum_buf_size = sizeof(U32) * width;
    p_col_sum = (U32 *)SAL_memMalloc(sum_buf_size, "xsp", "tmp");
    if (NULL != p_col_sum)
    {
        memset(p_col_sum, 0, sum_buf_size);
    }
    else
    {
        return SAL_FAIL;
    }

    /* 分别计算每列数据平均值 */
    for (i = 0; i < height; i++) /* 计算每列数据的和 */
    {
        for (j = 0; j < width; j++)
        {
            p_col_sum[j] += p_img_data[k++];
        }
    }

    for (j = 0; j < width; j++) /* 计算每列数据的平均值 */
    {
        p_col_avg[j] = p_col_sum[j] / height;
    }

    SAL_memfree(p_col_sum, "xsp", "tmp");

    return SAL_SOK;
}


/**
 * @fn      hka_xsp_lib_reg
 * @brief   不同XSP算法实现接口的注册，外部以函数指针的方式调用，无需关心具体XSP接口的调用流程与参数 
 * @warning 不同XSP算法实现有较大差异，会存在：①某一功能有些算法有，有些算法没有，②同一功能各算法要求的参数有差异 
 *          此时以“最大化”为处理原则：即不支持的功能需要写虚函数，函数实现可直接返回，确保函数指针不为空；
 *          若参数个数存有差异，以最多参数为准，多余的参数可忽略，并在注释中说明该参数不使用
 * 
 * @param   [IN] lib_src XSP算法类型
 * 
 * @return  SAL_STATUS SAL_SOK：注册成功，SAL_FAIL：注册失败
 */
SAL_STATUS hka_xsp_lib_reg(const XSP_LIB_SRC lib_src)
{
    if (lib_src <= XSP_LIB_RAYIN_AI_OTH)
    {
        g_xsp_lib_api.create_handle            = rin_native_lib_init;
        g_xsp_lib_api.reload_zTable            = rin_native_lib_reload_zTable;
        g_xsp_lib_api.get_version              = rin_native_get_version;
        g_xsp_lib_api.set_akey                 = rin_native_set_akey;
        g_xsp_lib_api.get_akey                 = rin_native_get_akey;
        g_xsp_lib_api.get_all_key              = rin_native_get_all_key;
        g_xsp_lib_api.set_blank_region         = rin_native_set_blank_region;
        g_xsp_lib_api.get_correction_data      = rin_native_get_correction_data;
        g_xsp_lib_api.set_correction_data      = rin_native_set_correction_data;
        g_xsp_lib_api.set_disp_color           = rin_native_set_disp_color;
        g_xsp_lib_api.set_rotate               = rin_native_set_rotate;
        g_xsp_lib_api.get_rotate               = rin_native_get_rotate;
        g_xsp_lib_api.set_mirror               = rin_native_set_mirror;
        g_xsp_lib_api.get_mirror               = rin_native_get_mirror;
        g_xsp_lib_api.set_pseudo_color         = rin_native_set_pseudo_color;
        g_xsp_lib_api.set_luminance            = rin_native_set_luminance;
        g_xsp_lib_api.set_default_enhance      = rin_native_set_default_enhance;
        g_xsp_lib_api.set_anti                 = rin_native_set_anti;
        g_xsp_lib_api.get_anti                 = rin_native_get_anti;
        g_xsp_lib_api.set_lp                   = rin_native_set_lp;
        g_xsp_lib_api.set_hp                   = rin_native_set_hp;
        g_xsp_lib_api.set_le                   = rin_native_set_le;
        g_xsp_lib_api.set_ue                   = rin_native_set_ue;
        g_xsp_lib_api.set_ee                   = rin_native_set_ee;
        g_xsp_lib_api.set_absor                = rin_native_set_absor;
        g_xsp_lib_api.set_unpen_alert          = rin_native_set_unpen_alert;
        g_xsp_lib_api.set_sus_alert            = rin_native_set_sus_alert;
        g_xsp_lib_api.set_bg                   = rin_native_set_bg;
        g_xsp_lib_api.set_color_table          = rin_native_set_color_table;
        g_xsp_lib_api.set_colors_imaging       = rin_native_set_colors_imaging;
        g_xsp_lib_api.set_deformity_correction = rin_native_set_deformity_correction;
        g_xsp_lib_api.set_dose_correct         = rin_native_set_dose_correct;
        g_xsp_lib_api.set_color_adjust         = rin_native_set_color_adjust;
        g_xsp_lib_api.set_belt_speed           = rin_native_set_belt_speed;
        g_xsp_lib_api.set_noise_reduction      = rin_native_set_noise_reduction;
        g_xsp_lib_api.set_contrast             = rin_native_set_contrast;
        g_xsp_lib_api.set_gamma                = rin_native_set_gamma;
        g_xsp_lib_api.set_aixsp_rate           = rin_native_set_aixsp_rate;
        g_xsp_lib_api.set_stretch_ratio        = rin_native_set_stretch_ratio;
        g_xsp_lib_api.set_le_th_range          = rin_native_set_le_th_range;
        g_xsp_lib_api.set_dt_gray_range        = rin_native_set_dt_gray_range;
        g_xsp_lib_api.set_bkg_color            = rin_native_set_bkg_color;
        g_xsp_lib_api.set_coldhot_threshold    = rin_native_set_coldhot_threshold;
        g_xsp_lib_api.get_zdata_version        = rin_native_get_zdata_version;

        g_xsp_lib_api.rtpreview_process        = rin_native_rtpreview_process;
        g_xsp_lib_api.playback_process         = rin_native_playback_process;
        g_xsp_lib_api.transform_process        = rin_native_transform_process;
        g_xsp_lib_api.identify_process         = rin_native_indentify_process;
    }
    else
    {
        XSP_LOGE("XSP lib src[%d] is not supported\n", lib_src);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

