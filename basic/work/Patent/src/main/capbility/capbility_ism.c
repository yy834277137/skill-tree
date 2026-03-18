/**
 * @file   capbility_ism.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  安检机设备能力集
 * @author dsp
 * @date   2022/8/1
 * @note
 * @note \n History
   1.日    期: 2022/8/1
     作    者: liwenbin
     修改历史: 创建文件
 */

#include "capbility.h"
#include "system_prm_api.h"

#line __LINE__ "capbility_ism.c"

/* ============================ 宏/枚举 ============================ */


/* ========================= 结构体/联合体 ========================== */


/* =================== 函数申明，static && extern =================== */


/* =================== 全局变量，static && extern =================== */


/**
 * @fn      capb_xrayin
 * @brief   CAPB_XRAY_IN能力参数初始化
 *
 * @param   p_xrayin[OUT] CAPB_XRAY_IN能力参数
 */
static void capb_xrayin(CAPB_XRAY_IN *p_xrayin, DSPINITPARA *pstInit)
{
    UINT32 i = 0, j = 0;
    UINT32 u32TmpSourceDist = 0;
    UINT32 integral_time_trend = 0, line_per_frame_trend = 0;
    FLOAT32 belt_speed_trend = 0.0;

    if (NULL == p_xrayin || NULL == pstInit)
    {
        return;
    }

    p_xrayin->xray_in_chan_cnt = pstInit->xrayChanCnt;
    for (j = 0; j < XRAY_FORM_NUM; j++)
    {
        integral_time_trend = 0;
        line_per_frame_trend = 0;
        belt_speed_trend = 0.0;
        for (i = 0; i < XRAY_SPEED_NUM; i++)
        {
            /* 若XRAY_SPEED_PARAM_ST中某个参数为0值，则认为该速度无效，并在切换速度时校验 */
            if (pstInit->dspCapbPar.xray_speed[j][i].integral_time > 0 && 
                pstInit->dspCapbPar.xray_speed[j][i].slice_height > 0 && 
                pstInit->dspCapbPar.xray_speed[j][i].belt_speed > 0.0)
            {
                p_xrayin->st_xray_speed[j][i].integral_time = pstInit->dspCapbPar.xray_speed[j][i].integral_time;
                p_xrayin->st_xray_speed[j][i].line_per_slice = pstInit->dspCapbPar.xray_speed[j][i].slice_height;
                p_xrayin->st_xray_speed[j][i].belt_speed = pstInit->dspCapbPar.xray_speed[j][i].belt_speed;
                if (integral_time_trend > 0)
                {
                    if (p_xrayin->st_xray_speed[j][i].integral_time < integral_time_trend && /* 积分时间严格递减 */
                        p_xrayin->st_xray_speed[j][i].line_per_slice >= line_per_frame_trend && /* 条带高度递增 */
                        p_xrayin->st_xray_speed[j][i].belt_speed > belt_speed_trend) /* 传送带速度严格递增 */
                    {
                        /* 正常情况 */
                    }
                    else
                    {
                        SAL_LOGE("XRay speed parameters are invalid:integral_time[%u].slice_height[%u].belt_speed[%2f]! Check again\n",
                                 pstInit->dspCapbPar.xray_speed[j][i].integral_time, 
                                 pstInit->dspCapbPar.xray_speed[j][i].slice_height, 
                                 pstInit->dspCapbPar.xray_speed[j][i].belt_speed);
                    }
                }

                integral_time_trend = p_xrayin->st_xray_speed[j][i].integral_time;
                line_per_frame_trend = p_xrayin->st_xray_speed[j][i].line_per_slice;
                belt_speed_trend = p_xrayin->st_xray_speed[j][i].belt_speed;
            }
        }
    }

    /* 计算各个速度下最大条带高度 */
    p_xrayin->line_per_slice_max = 0;
    p_xrayin->line_per_slice_min = 0xFFFFFFFF;
    for (i = 0; i < XRAY_FORM_NUM; i++)
    {
        for (j = 0; j < XRAY_SPEED_NUM; j++)
        {
            if (p_xrayin->st_xray_speed[i][j].line_per_slice > p_xrayin->line_per_slice_max)
            {
                p_xrayin->line_per_slice_max = p_xrayin->st_xray_speed[i][j].line_per_slice;
            }

            if ((p_xrayin->st_xray_speed[i][j].line_per_slice > 0) && (p_xrayin->st_xray_speed[i][j].line_per_slice < p_xrayin->line_per_slice_min))
            {
                p_xrayin->line_per_slice_min = p_xrayin->st_xray_speed[i][j].line_per_slice;
            }

            if (0 != p_xrayin->st_xray_speed[i][j].belt_speed)
            {
                /* 0.3为两个视角源间距 */
                u32TmpSourceDist = (UINT32)(0.3 * 1000000 / (p_xrayin->st_xray_speed[i][j].belt_speed * p_xrayin->st_xray_speed[i][j].integral_time));
                if (u32TmpSourceDist > p_xrayin->u32MaxSourceDist)
                {
                    p_xrayin->u32MaxSourceDist = u32TmpSourceDist;
                }
            }
        }
    }

    if (1 == pstInit->xrayChanCnt)
    {
        /* 单视角设备固定为100列，方便提前全屏处理 */
        p_xrayin->u32MaxSourceDist = 100;
    }

    p_xrayin->u32MaxSourceDist = (UINT32)(SAL_align(p_xrayin->u32MaxSourceDist, 2) * pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor);
    p_xrayin->xray_det_vendor = pstInit->dspCapbPar.xray_det_vendor;
    p_xrayin->xraw_width_max = pstInit->dspCapbPar.xray_width_max;
    p_xrayin->xraw_height_max = pstInit->dspCapbPar.xray_height_max;
    p_xrayin->xray_energy_num = pstInit->dspCapbPar.xray_energy_num;
    p_xrayin->xraw_bytewidth = 2;

    return;
}

/**
 * @fn      capb_xsp
 * @brief   CAPB_XSP能力参数初始化
 * 
 * @param   p_xsp[OUT] CAPB_XSP能力参数
 */
static void capb_xsp(CAPB_XSP *p_xsp, DSPINITPARA *pstInit)
{
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();

    if (NULL == p_xsp || NULL == pstInit)
    {
        return;
    }

    p_xsp->xsp_original_raw_bw = (pstInit->dspCapbPar.xray_energy_num == XRAY_ENERGY_SINGLE) ? 2 : 4;
    p_xsp->xsp_normalized_raw_bw = (pstInit->dspCapbPar.xray_energy_num == XRAY_ENERGY_SINGLE) ? 2 : 5;
    p_xsp->xsp_ori_normalized_raw_bw = (pstInit->dspCapbPar.xray_energy_num == XRAY_ENERGY_SINGLE) ? 2 : 6;
    p_xsp->resize_height_factor = pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor;
    p_xsp->resize_width_factor = pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor;
    p_xsp->xraw_width_resized = (INT32)(pstInit->dspCapbPar.xray_width_max * p_xsp->resize_width_factor);
    p_xsp->xraw_height_resized = (INT32)(pstInit->dspCapbPar.xray_height_max * p_xsp->resize_height_factor);
    p_xsp->xraw_width_resized_max = (INT32)(pstInit->dspCapbPar.xray_width_max * XSP_RESIZE_FACTOR_MAX);
    p_xsp->xraw_height_resized_max = (INT32)(pstInit->dspCapbPar.xray_height_max * XSP_RESIZE_FACTOR_MAX);
    p_xsp->xsp_package_line_max = SAL_CLIP(pstInit->dspCapbPar.package_len_max, 640, 1600);

#if defined(RK3588)
    {
        p_xsp->enDispFmt = DSP_IMG_DATFMT_BGR24;    /* RK平台算法使用rgb格式显示 */
    }
#elif defined(X86)
    {
        p_xsp->enDispFmt = DSP_IMG_DATFMT_BGRA32;     /* x86平台算法使用argb格式显示 */
    }
#else
    {
        p_xsp->enDispFmt = DSP_IMG_DATFMT_YUV420SP_VU;
    }
#endif

    p_xsp->xsp_fsc_proc_width_max = SAL_align(p_xsp->xraw_height_resized + pstCapbXrayIn->u32MaxSourceDist, 2);
    if (0 == pstCapbXrayIn->u32MaxSourceDist)
    {
        SAL_LOGW("xray capbility has not been initialized, max source distance:%u\n", pstCapbXrayIn->u32MaxSourceDist);
    }

    return;
}

/**
 * @function   capb_xpack
 * @brief      CAPB_XPACK能力级参数初始化
 * @param[out] CAPB_XPACK *p_xpack   CAPB_XPACK能力级参数
 * @param[in]  DSPINITPARA *pstInit  DSP初始化参数
 * @return     static void
 */
static void capb_xpack(CAPB_XPACK *p_xpack, DSPINITPARA *pstInit)
{
    UINT32 i = 0;

    p_xpack->u32XpackSendChnNum = 2;    /* 一份数据用于显示，会进行叠框；另一份数据不叠框用于编码 */
    /*一源俩输出 需要分配三路缓存*/
    p_xpack->xpack_disp_buffer_w = SAL_align((UINT32)(pstInit->dspCapbPar.xray_height_max * pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor), 16) * 3;

    if (pstInit->xrayChanCnt == 1 && pstInit->xrayDispChanCnt == 2)
    {
        p_xpack->u32XpackSendChnNum = 3;
    }

    p_xpack->u32AiDgrHeight = pstInit->dspCapbPar.xray_width_max; // 危险品识别使用未超分的NV21图像

    for (i = 0; i < MAX_XRAY_CHAN; i++)
    {
    #ifdef RK3588
        /* RK3588共8核，xpack显示发送线程实时性要求高，跟其他算法分开绑核属性 */
        p_xpack->uiCoreDispVpss[i] = SAL_MIN(4 + i, 7);
    #else
        p_xpack->uiCoreDispVpss[i] = -1;
    #endif
    }

    for (i = 0; i < XRAY_SPEED_NUM; i++)
    {
        #ifdef NT98336 // 平台性能限制，不用分段识别
        p_xpack->u32AiDgrSegLen[i] = (UINT32)-1;
        #else
        /* 小于0.4m/s的低速才支持 */
        if (pstInit->dspCapbPar.xray_speed[XRAY_FORM_ORIGINAL][i].belt_speed < 0.4)
        {
            p_xpack->u32AiDgrSegLen[i] = 288;
        }
        else
        {
            p_xpack->u32AiDgrSegLen[i] = (UINT32)-1;
        }
        #endif
    }

    return;
}

/**
 * @fn      capb_ai
 * @brief   CAPB_AI能力参数初始化
 * 
 * @param   p_ai[OUT] CAPB_AI能力参数
 */
static void capb_ai(CAPB_AI *p_ai, DSPINITPARA *pstInit)
{
    if (NULL == p_ai || NULL == pstInit)
    {
        return;
    }

#ifdef NT98336
	p_ai->ai_chn = 1;  /* NT平台只支持一路智能 */
#else
    p_ai->ai_chn = pstInit->xrayChanCnt;
#endif

    p_ai->uiCbJpgNum = 0x1;
    p_ai->uiCbBmpNum = 0x0;
    p_ai->uiIaMemType = 2;

    return;
}

/**
 * @fn      capb_disp
 * @brief   CAPB_DISP能力参数初始化
 *
 * @param   p_disp[OUT] CAPB_DISP能力参数
 */
static void capb_disp(CAPB_DISP *p_disp, DSPINITPARA *pstInit)
{
    if (NULL == p_disp || NULL == pstInit)
    {
        return;
    }

    p_disp->disp_delay_nums = 0;
    p_disp->disp_vodev_cnt = pstInit->xrayDispChanCnt;
    p_disp->disp_videv_cnt = pstInit->xrayChanCnt;
    p_disp->disp_yuv_w_max = (UINT32)(pstInit->dspCapbPar.xray_height_max * pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor);
    p_disp->disp_yuv_w_max = SAL_align(p_disp->disp_yuv_w_max, 16); /* 显示宽16对齐 */
    p_disp->disp_h_top_padding = (UINT32)(pstInit->dspCapbPar.padding_top * pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor);
    p_disp->disp_h_bottom_padding = (UINT32)(pstInit->dspCapbPar.padding_bottom * pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor);
    p_disp->disp_h_top_blanking = (UINT32)(pstInit->dspCapbPar.blanking_top * pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor);
    p_disp->disp_h_bottom_blanking = (UINT32)(pstInit->dspCapbPar.blanking_bottom * pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor);
    p_disp->disp_h_middle_indeed = (UINT32)(pstInit->dspCapbPar.xray_width_max * pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor);
    p_disp->disp_yuv_h = p_disp->disp_h_middle_indeed + p_disp->disp_h_top_padding + p_disp->disp_h_bottom_padding;
    p_disp->disp_yuv_h_max = p_disp->disp_yuv_h + p_disp->disp_h_top_blanking + p_disp->disp_h_bottom_blanking;
#ifdef RK3588
    p_disp->disp_screen_bg_color = 0xFFFFFFFF;   /*RK背景设置为白色ARGB*/
    p_disp->enInputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
#else
    p_disp->disp_screen_bg_color = 0xefefef;
    p_disp->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
#endif
    p_disp->disp_link_type = MOD_LINK_TYPE_USER;
    p_disp->disp_mipi_chip = MIPI_TX_CHIP_6211;
    p_disp->disp_vpss_gra_chn_0 = 0;
    p_disp->disp_vpss_gra_chn_1 = 2;

    return;
}

/**
 * @fn      capb_osd
 * @brief   CAPB_OSD能力参数初始化
 *
 * @param   p_osd[OUT] CAPB_OSD能力参数 
 * @param   pstInit[IN] DSP初始化参数，仅使用超分系数这一项
 */
static void capb_osd(CAPB_OSD *p_osd, DSPINITPARA *pstInit)
{
    FLOAT32 f32ScaleRatio = 0.0;

    if (NULL == p_osd || NULL == pstInit)
    {
        return;
    }

    p_osd->enDrawMod = DRAW_MOD_VGS;
    p_osd->enFontType = OSD_FONT_TRUETYPE;

    if (pstInit->stFontLibInfo.enEncFormat == ENC_FMT_ISO_8859_6) /* 阿拉伯语显示的OSD较小，增大一个字号 */
    {
        p_osd->TrueTypeSize[0] = 24;
        p_osd->TrueTypeSize[1] = 30;
        p_osd->TrueTypeSize[2] = 36;
    }
    else
    {
        p_osd->TrueTypeSize[0] = 18;
        p_osd->TrueTypeSize[1] = 24;
        p_osd->TrueTypeSize[2] = 30;
    }

    if(pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor > 1.0)
    {
        /* 计算叠加OSD后, 缩放比例, 当前屏幕分辨率先固定为1080P, 暂不从输出参数中获取*/
        f32ScaleRatio = (pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor * pstInit->dspCapbPar.xray_height_max) / 1920;
        
        p_osd->TrueTypeSize[0] = SAL_align((UINT32)(f32ScaleRatio * p_osd->TrueTypeSize[0]), 2);
        p_osd->TrueTypeSize[1] = SAL_align((UINT32)(f32ScaleRatio * p_osd->TrueTypeSize[1]), 2);
        p_osd->TrueTypeSize[2] = SAL_align((UINT32)(f32ScaleRatio * p_osd->TrueTypeSize[2]), 2);
    }

    return;
}

/**
 * @fn      capb_line
 * @brief   CAPB_LINE能力参数初始化
 *
 * @param   p_line[OUT] CAPB_LINE能力参数
 */
static void capb_line(CAPB_LINE *p_line)
{
    if (NULL == p_line)
    {
        return;
    }

    p_line->enDrawMod = DRAW_MOD_CPU;
    p_line->enRectType = RECT_EXCEED_TYPE_CLOSE;
    p_line->u32LineWidth = 2;
    p_line->bUseFract = SAL_FALSE;

    return;
}

/**
 * @fn      capb_venc
 * @brief   CAPB_VENC能力参数初始化
 *
 * @param   p_venc[OUT] CAPB_VENC能力参数
 */
static void capb_venc(CAPB_VENC *p_venc, DSPINITPARA *pstInit)
{
    if (NULL == p_venc || NULL == pstInit)
    {
        return;
    }
    
    /* 编码前级输入帧率,即vpss输入帧率 */
    p_venc->srcfps = pstInit->stVoInitInfoSt.stVoInfoPrm[0].stDispDevAttr.videoOutputFps;
    
    /* 编码目的帧率 */
    p_venc->dstfps = 30; 

    p_venc->resolution = SXGA_FORMAT;
    p_venc->venc_width = 1280;
    p_venc->venc_height = 1024;
    p_venc->venc_bps = 2048;
    p_venc->venc_bpsType = 0;
    p_venc->venc_quailty = 90;
    p_venc->venc_maxwidth = 1920;
    p_venc->venc_maxheight = 1080;
    p_venc->venc_scale  = 0; /* 缩放改到vpss去做。venc的缩放是rga做的，虚宽超过了rga的限制，所以由vpss里的gpu去做 */

    return;
}

/**
 * @fn		capb_vproc
 * @brief	CAPB_DUP能力参数初始化
 * 
 * @param	p_dup[OUT] CAPB_DUP能力参数
 */
static void capb_vproc(CAPB_VPROC *p_vproc)
{
    UINT32 i =0;
    if (NULL == p_vproc)
    {
        return;
    }

    p_vproc->width  = 1920;
    p_vproc->height = 1080;

    p_vproc->vpssGrp.uiBlkCnt = 3;
    p_vproc->vpssGrp.uiDepth = 1;
    p_vproc->vpssGrp.uiChnEnable[i++] = ENABLED;
    p_vproc->vpssGrp.uiChnEnable[i++] = DISABLED;
    p_vproc->vpssGrp.uiChnEnable[i++] = DISABLED;
    p_vproc->vpssGrp.uiChnEnable[i++] = DISABLED;
#ifdef RK3588
    p_vproc->vpssGrp.enVpssDevType = VPSS_DEV_RGA;
#else
    p_vproc->vpssGrp.enVpssDevType = VPSS_DEV_VPE;
#endif

    p_vproc->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_vproc->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    
}

/**
 * @fn      capb_vdec
 * @brief   CAPB_VDEC能力参数初始化
 *
 * @param   p_vdec[OUT] CAPB_VDEC能力参数
 */
static void capb_vdec(CAPB_VDEC *p_vdec)
{
    if (NULL == p_vdec)
    {
        return;
    }

    p_vdec->vdec_cnt = 10;
    p_vdec->vpsschn_cnt = 4;

    return;
}

/**
 * @fn		capb_audio
 * @brief	CAPB_AUDIO能力参数初始化
 *
 * @param	p_audio[OUT] CAPB_AUDIO能力参数
 */
static void capb_audio(CAPB_AUDIO *p_audio, DSPINITPARA *pstInit)
{
    if (NULL == p_audio || NULL == pstInit)
    {
        return;
    }

#ifdef NT98336
    p_audio->audiodevidx = 0; /*I2S ID*/
#else
    p_audio->audiodevidx = pstInit->dspCapbPar.audio_dev_cnt;
#endif
    return;
}

/**
 * @fn		capb_capt
 * @brief	CAPB_CAPT能力参数初始化
 *
 * @param	p_capt[OUT] CAPB_CAPT能力参数
 */
static void capb_capt(CAPB_CAPT *p_capt)
{
    if (NULL == p_capt)
    {
        return;
    }

    p_capt->bFpgaEnable = SAL_FALSE;

    return;
}

/**
 * @fn		capb_dup
 * @brief	CAPB_DUP能力参数初始化
 *
 * @param	p_dup[OUT] CAPB_DUP能力参数
 */
static void capb_dup(CAPB_DUP *p_dup, DSPINITPARA *pstInit)
{
    UINT32 i = 0;

    if (NULL == p_dup || NULL == pstInit)
    {
        return;
    }

    p_dup->width = pstInit->dspCapbPar.xray_height_max;
    p_dup->height = pstInit->dspCapbPar.xray_width_max + pstInit->dspCapbPar.padding_top + pstInit->dspCapbPar.padding_bottom;
    p_dup->width = p_dup->width > 1920 ? p_dup->width : 1920;
    p_dup->height = p_dup->height > 1080 ? p_dup->height : 1080;

    p_dup->u32FrontBindRear = 0;
    for (i = 0; i < DUP_VPSS_CHN_MAX_NUM; i++)
    {
        p_dup->frontVpssGrp.uiChnEnable[i] = SAL_TRUE;
        p_dup->rearVpssGrp.uiChnEnable[i] = SAL_TRUE;
    }

#ifdef RK3588
    //p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_ARGB_8888;
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
    p_dup->frontVpssGrp.enVpssDevType = VPSS_DEV_GPU;
    p_dup->rearVpssGrp.enVpssDevType = VPSS_DEV_GPU;
    p_dup->frontVpssGrp.uiBlkCnt = 0; /* RK里passthrouht及绑定模式，vb不需要用到；如果是USER模式，BlkCnt不能为0 */
    p_dup->frontVpssGrp.uiDepth = 0;
    /* 1、venc对应的vpss chan会被改成USER模式，因为虚宽超过了venc rga缩放的限制，depth 为1，vb>=3；或者depth改为0，但如果为0，就无法使用jpeg抓图。
    2、暂时给jpeg抓图预留1个VB; 安检机里jpeg抓图暂时没在使用，这里先配置实际不会分配物理内存 */
    p_dup->rearVpssGrp.uiBlkCnt = 4; 
    p_dup->rearVpssGrp.uiDepth = 1;
#else
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->frontVpssGrp.enVpssDevType = VPSS_DEV_VPE;
    p_dup->rearVpssGrp.enVpssDevType = VPSS_DEV_VPE;
    p_dup->frontVpssGrp.uiBlkCnt = 12;
    p_dup->frontVpssGrp.uiDepth = 3;
    p_dup->rearVpssGrp.uiBlkCnt = 12;
    p_dup->rearVpssGrp.uiDepth = 2;
#endif

    return;
}

/**
 * @fn		capb_dup
 * @brief	capb_vdecDup用于解码的DUP能力集参数配置
 *
 * @param	p_dup[OUT] 用于解码的DUP能力集参数
 */
static void capb_vdecDup(CAPB_VDEC_DUP *p_dup)
{
    if (NULL == p_dup)
    {
        return;
    }

    p_dup->width = 1920;
    p_dup->height = 1080;
    p_dup->u32HasFrontDupScale = 0;
    p_dup->frontVpssGrp.uiDepth = 0; /* 除了ss928其他平台没有用前级vpss做放大，ss928同时只有一个chan能放大 */
    p_dup->rearVpssGrp.uiDepth = 3;
    p_dup->rearVpssGrp.uiBlkCnt = 10; /* chn0 3个，passthrouht模式下根据后级VO CACHE配置; ch1暂时不用；ch2 ch3各2个；预留3个，实际没有用到的vb不会分配物理内存*/

#ifdef RK3588
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
    p_dup->rearVpssGrp.enVpssDevType = VPSS_DEV_RGA;
#else
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->rearVpssGrp.enVpssDevType = VPSS_DEV_VPE;
#endif

    return;
}

/**
 * @fn		capb_jpeg
 * @brief	CAPB_JPEG能力参数初始化
 *
 * @param	p_jpeg[OUT] CAPB_JPEG能力参数
 */
static void capb_jpeg(CAPB_JPEG *p_jpeg, DSPINITPARA *pstInit)
{
    if (NULL == p_jpeg || NULL == pstInit)
    {
        return;
    }

    p_jpeg->videv_cnt = pstInit->xrayChanCnt;
    p_jpeg->maxdev_num = 2 + 8;
    p_jpeg->maxchn_num = 2 + 8 + 2 + 2;

    return;
}

/**
 * @fn		capb_product
 * @brief	CAPB_PRODUCT能力参数初始化
 *
 * @param	pstProduct[OUT] CAPB_PRODUCT能力参数
 */
static void capb_product(CAPB_PRODUCT *pstProduct)
{
    if (NULL == pstProduct)
    {
        return;
    }

    pstProduct->enInputType = VIDEO_INPUT_INSIDE;
    pstProduct->bXPackEnable = SAL_TRUE;
    pstProduct->bXRayEnable = SAL_TRUE;
    pstProduct->bXspEnable = SAL_TRUE;

    return;
}

/**
 * @fn		capb_platform
 * @brief	CAPB_PLATFORM能力参数初始化
 *
 * @param	pstPlatform[OUT] CAPB_PLATFORM能力参数
 */
static void capb_platform(CAPB_PLATFORM *pstPlatform)
{
    if (NULL == pstPlatform)
    {
        return;
    }
#ifdef HI3559A_SPC030
    pstPlatform->bHardCopy = SAL_TRUE;
    pstPlatform->bHardMmap = SAL_TRUE;
    pstPlatform->bFrameCrop = SAL_FALSE;
    pstPlatform->bDiscontAddrOsd = SAL_TRUE;     //是否支持不连续地址画osd
    pstPlatform->bTdeCoordiateTrans = SAL_FALSE;
#elif NT98336
    pstPlatform->bHardCopy = SAL_TRUE;
    pstPlatform->bHardMmap = SAL_FALSE;
    pstPlatform->bFrameCrop = SAL_TRUE;
    pstPlatform->bDiscontAddrOsd = SAL_FALSE;   //是否支持不连续地址画osd
    pstPlatform->bTdeCoordiateTrans = SAL_FALSE;
#elif RK3588
    pstPlatform->bHardCopy = SAL_TRUE;
    pstPlatform->bHardMmap = SAL_FALSE;
    pstPlatform->bFrameCrop = SAL_FALSE;
    pstPlatform->bDiscontAddrOsd = SAL_FALSE;    // 包裹回拉时mb的offset为0，叠第2~4个包裹时叠到第一张图上，所以要先拷出来进行叠加
    pstPlatform->bTdeCoordiateTrans = SAL_TRUE;
#else
    pstPlatform->bHardCopy = SAL_TRUE;
    pstPlatform->bHardMmap = SAL_TRUE;
    pstPlatform->bFrameCrop = SAL_TRUE;
    pstPlatform->bDiscontAddrOsd = SAL_TRUE;    //是否支持不连续地址画osd
    pstPlatform->bTdeCoordiateTrans = SAL_FALSE;
#endif
    return;
}


/**
 * @fn      capb_ism_init
 * @brief   安检机能力集初始化
 *
 * @param   p_overall[OUT] 能力集参数
 */
void capb_ism_init(CAPB_OVERALL *p_overall, DSPINITPARA *pstInit)
{
    UINT32 i = 0, j = 0;
    CHAR sDbgInfoSliceHegiht[256] = {0}, sDbgInfoIntTime[256] = {0}, sDbgInfoBeltSpeed[256] = {0};
    XRAY_SPEED_PARAM_ST *pstSpeedParam = NULL;

    if (NULL == p_overall)
    {
        return;
    }

    /* 参数错误修正 */
    if (0.0 == pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor)
    {
        pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor = 1.0;
    }
    if (0.0 == pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor)
    {
        pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor = 1.0;
    }

    pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor = 1.5;
    pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor = 1.5;

    capb_product(&p_overall->capb_product);
    capb_platform(&p_overall->capb_platform);
    capb_xrayin(&p_overall->capb_xrayin, pstInit);
    capb_xsp(&p_overall->capb_xsp, pstInit);        // 保证其在capb_xrayin之后初始化，因为capb_xsp内部使用到capb_xrayin的u32MaxSourceDist成员
    capb_xpack(&p_overall->capb_xpack, pstInit);
    capb_ai(&p_overall->capb_ai, pstInit);
    capb_disp(&p_overall->capb_disp, pstInit);
    capb_osd(&p_overall->capb_osd, pstInit);
    capb_line(&p_overall->capb_line);
    capb_venc(&p_overall->capb_venc, pstInit);
    capb_vdec(&p_overall->capb_vdec);
    capb_capt(&p_overall->capb_capt);
    capb_audio(&p_overall->capb_audio, pstInit);
    capb_dup(&p_overall->capb_dup, pstInit);
    capb_vdecDup(&p_overall->capb_vdecDup);
    capb_jpeg(&p_overall->capb_jpeg, pstInit);
    capb_vproc(&p_overall->capb_vproc);

    SYS_LOGW("APP Capbility:\n");
    SYS_LOGW("  xray_in_chan_cnt %d, xraw_width_max %d, xraw_height_max %d, xray_energy_num %d devType 0x%x detType %d xraySrc[0x%x 0x%x]\n",
             pstInit->xrayChanCnt, pstInit->dspCapbPar.xray_width_max, pstInit->dspCapbPar.xray_height_max, pstInit->dspCapbPar.xray_energy_num, 
             pstInit->dspCapbPar.ism_dev_type, pstInit->dspCapbPar.xray_det_vendor, 
             pstInit->dspCapbPar.aenXraySrcType[0], pstInit->dspCapbPar.aenXraySrcType[1]);
    for (i = 0; i < XRAY_FORM_NUM; i++)
    {
        pstSpeedParam = pstInit->dspCapbPar.xray_speed[i];
        for (j = 0; j < XRAY_SPEED_NUM; j++)
        {
            snprintf(sDbgInfoSliceHegiht + strlen(sDbgInfoSliceHegiht), sizeof(sDbgInfoSliceHegiht) - strlen(sDbgInfoBeltSpeed), "%u ", pstSpeedParam[j].slice_height);
            snprintf(sDbgInfoIntTime + strlen(sDbgInfoIntTime), sizeof(sDbgInfoIntTime) - strlen(sDbgInfoSliceHegiht), "%u ", pstSpeedParam[j].integral_time);
            snprintf(sDbgInfoBeltSpeed + strlen(sDbgInfoBeltSpeed), sizeof(sDbgInfoBeltSpeed) - strlen(sDbgInfoIntTime), "%.2f ", pstSpeedParam[j].belt_speed);
        }

        SYS_LOGW("  FORM_%u: slice_height [%s], integral_time [%s]us, belt_speed [%s]m/s\n", i,
                 sDbgInfoSliceHegiht, sDbgInfoIntTime, sDbgInfoBeltSpeed);
        memset(sDbgInfoSliceHegiht, 0, sizeof(sDbgInfoSliceHegiht));
        memset(sDbgInfoIntTime, 0, sizeof(sDbgInfoIntTime));
        memset(sDbgInfoBeltSpeed, 0, sizeof(sDbgInfoBeltSpeed));
    }

    SYS_LOGW("  dispChanCnt %d, uiUserFps %d, padding [%d %d], blanking [%u %u], resize_factor [%.2f %.2f]\n", 
             pstInit->xrayDispChanCnt, pstInit->stVoInitInfoSt.stVoInfoPrm[0].stDispDevAttr.videoOutputFps,
             pstInit->dspCapbPar.padding_top, pstInit->dspCapbPar.padding_bottom,
             pstInit->dspCapbPar.blanking_top, pstInit->dspCapbPar.blanking_bottom,
             pstInit->dspCapbPar.xsp_resize_factor.resize_width_factor, pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor);
    SYS_LOGW("DSP Capbility:\n");
    SYS_LOGW("  xray_in_chan_cnt %d, xsp_package_line_max %d, xraw_width_resized %d, xraw_height_resized %d\n",
             p_overall->capb_xrayin.xray_in_chan_cnt, p_overall->capb_xsp.xsp_package_line_max, 
             p_overall->capb_xsp.xraw_width_resized, p_overall->capb_xsp.xraw_height_resized);
    SYS_LOGW("  vichn %d vochn %d disp_mode %d\n", pstInit->stViInitInfoSt.uiViChn, pstInit->stVoInitInfoSt.uiVoCnt, pstInit->dspCapbPar.ism_disp_mode); 
    SYS_LOGW("  osd size: %d-%d-%d.\n", p_overall->capb_osd.TrueTypeSize[0], p_overall->capb_osd.TrueTypeSize[1], p_overall->capb_osd.TrueTypeSize[2]);

    return;
}

