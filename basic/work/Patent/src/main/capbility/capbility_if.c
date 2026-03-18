/**
 * @file   capbility_if.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  能力集接口
 * @author dsp
 * @date   2022/8/1
 * @note
 * @note \n History
   1.日    期: 2022/8/1
     作    者: liwenbin
     修改历史: dsp
 */

#include "capbility.h"
#include "task_ism.h"



/* ============================ 宏/枚举 ============================= */
#line __LINE__ "capbility_if.c"


/* ========================= 结构体/联合体 ========================== */


/* =================== 函数申明，static && extern =================== */


/* =================== 全局变量，static && extern =================== */
static CAPB_OVERALL g_capb_overall = {0};


/**
 * @fn      capbility_init
 * @brief   能力集初始化
 *
 * @param   board_type[IN] 设备型号
 *
 * @return  SAL_STATUS SAL_SOK-初始化成功，SAL_FAIL-初始化失败
 */
SAL_STATUS capbility_init(DSPINITPARA *pstInit)
{
    CAPB_OVERALL *p_overall = NULL;

    p_overall = &g_capb_overall;

    if (NULL == pstInit)
    {
        SAL_ERROR("dsp init pstInit is NULL--------\n");
        return SAL_FAIL;
    }

    switch (pstInit->dspCapbPar.dev_tpye)
    {
        case PRODUCT_TYPE_ISA:
            capb_isa_init(p_overall, pstInit->boardType);
            break;
#ifdef DSP_ISM
        case PRODUCT_TYPE_ISM:
            capb_ism_init(p_overall, pstInit);
            break;
#endif
        default:
            SAL_ERROR("dev_tpye is invalid: %d\n", pstInit->dspCapbPar.dev_tpye);
            return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      capb_get_xrayin
 * @brief   获取CAPB_XRAY_IN能力
 *
 * @return  CAPB_XRAY_IN* 能力参数
 */
inline CAPB_XRAY_IN *capb_get_xrayin(void)
{
    return &g_capb_overall.capb_xrayin;
}

/**
 * @fn      capb_get_xsp
 * @brief   获取CAPB_XSP能力
 *
 * @return  CAPB_XSP* 能力参数
 */
inline CAPB_XSP *capb_get_xsp(void)
{
    return &g_capb_overall.capb_xsp;
}

/**
 * @fn      capb_get_xpack
 * @brief   获取CAPB_XPACK能力
 *
 * @return  CAPB_XPACK* 能力参数
 */
inline CAPB_XPACK *capb_get_xpack(void)
{
    return &g_capb_overall.capb_xpack;
}

/**
 * @fn      capb_get_ai
 * @brief   获取CAPB_AI能力
 *
 * @return  CAPB_AI* 能力参数
 */
inline CAPB_AI *capb_get_ai(void)
{
    return &g_capb_overall.capb_ai;
}

/**
 * @fn      capb_get_disp
 * @brief   获取CAPB_DISP能力
 *
 * @return  CAPB_DISP* 能力参数
 */
inline CAPB_DISP *capb_get_disp(void)
{
    return &g_capb_overall.capb_disp;
}

/**
 * @fn      capb_get_line
 * @brief   获取CAPB_LINE能力
 *
 * @return  CAPB_LINE* 能力参数
 */
inline CAPB_LINE *capb_get_line(void)
{
    return &g_capb_overall.capb_line;
}

/**
 * @fn      capb_get_osd
 * @brief   获取CAPB_OSD能力
 *
 * @return  CAPB_OSD* 能力参数
 */
inline CAPB_OSD *capb_get_osd(void)
{
    return &g_capb_overall.capb_osd;
}

/**
 * @fn      capb_get_disp_chan_cnt
 * @brief   获取显示通道数
 *
 * @return  UINT32 显示通道数
 */
inline UINT32 capb_get_disp_chan_cnt(void)
{
    return g_capb_overall.capb_disp.disp_chan_cnt;
}

/**
 * @fn      capb_get_disp_height
 * @brief   获取输入VPSS未缩放时的YUV高
 *
 * @return  UINT32 输入VPSS未缩放时的YUV高
 */
inline UINT32 capb_get_disp_height(void)
{
    return g_capb_overall.capb_disp.disp_yuv_h;
}

/**
 * @fn      capb_get_venc
 * @brief   获取CAPB_VENC能力
 *
 * @return  CAPB_VENC* 能力参数
 */
inline CAPB_VENC *capb_get_venc(void)
{
    return &g_capb_overall.capb_venc;
}

/**
 * @fn      capb_get_vdec
 * @brief   获取CAPB_VDEC能力
 *
 * @return  CAPB_VDEC* 能力参数
 */
inline CAPB_VDEC *capb_get_vdec(void)
{
    return &g_capb_overall.capb_vdec;
}

/**
 * @fn      capb_get_audio
 * @brief   获取CAPB_AUDIO能力
 *
 * @return  CAPB_AUDIO* 能力参数
 */
inline CAPB_AUDIO *capb_get_audio(void)
{
    return &g_capb_overall.capb_audio;
}

/**
 * @fn      capb_get_dup
 * @brief   获取CAPB_DUP能力
 *
 * @return  CAPB_DUP* 能力参数
 */
inline CAPB_DUP *capb_get_dup(void)
{
    return &g_capb_overall.capb_dup;
}

/**
 * @fn      capb_get_vdecDup
 * @brief   CAPB_VDEC_DUP
 *
 * @return  CAPB_VDEC_DUP* 能力参数
 */
inline CAPB_VDEC_DUP *capb_get_vdecDup(void)
{
    return &g_capb_overall.capb_vdecDup;
}

/**
 * @fn      capb_get_vproc
 * @brief   获取CAPB_VPROC能力
 *
 * @return  CAPB_VPROC* 能力参数
 */
inline CAPB_VPROC *capb_get_vproc(void)
{
    return &g_capb_overall.capb_vproc;
}

/**
 * @fn      capb_get_jpeg
 * @brief   获取CAPB_JPEG能力
 *
 * @return  CAPB_JPEG* 能力参数
 */
inline CAPB_JPEG *capb_get_jpeg(void)
{
    return &g_capb_overall.capb_jpeg;
}

/**
 * @fn      capb_get_capt
 * @brief   获取CAPB_CAPT能力
 *
 * @return  CAPB_CAPT* 能力参数
 */
inline CAPB_CAPT *capb_get_capt(void)
{
    return &g_capb_overall.capb_capt;
}

/**
 * @fn      capb_get_product
 * @brief   获取CAPB_PRODUCT能力
 *
 * @return  CAPB_PRODUCT* 能力参数
 */
inline CAPB_PRODUCT *capb_get_product(void)
{
    return &g_capb_overall.capb_product;
}

/**
 * @fn      capb_get_platform
 * @brief   获取CAPB_PRODUCT能力
 *
 * @return  CAPB_PLATFORM* 能力参数
 */
inline CAPB_PLATFORM *capb_get_platform(void)
{
    return &g_capb_overall.capb_platform;
}

