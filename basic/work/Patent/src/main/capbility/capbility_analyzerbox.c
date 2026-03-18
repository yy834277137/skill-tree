
/**
 * @file    capbility_analyzer.c
 * @note    Hangzhou Hikvision Digital Technology Co., Ltd. All Right Reserved 
 * @brief   分析仪能力集
 */

#include "capbility.h"
#include "system_prm_api.h"
#include <platform_hal.h>

/* ============================ 宏/枚举 ============================ */
#line __LINE__ "capbility_analyzerbox.c"


/* ========================= 结构体/联合体 ========================== */


/* =================== 函数申明，static && extern =================== */


/* =================== 全局变量，static && extern =================== */

/**
 * @fn      capb_ai
 * @brief   CAPB_AI能力参数初始化
 * 
 * @param   p_ai[OUT] CAPB_AI能力参数
 */
static void capb_ai(CAPB_AI *p_ai)
{
    if (NULL == p_ai)
    {
        return;
    }

	p_ai->uiCbJpgNum = 0;
	p_ai->uiCbBmpNum = 0;
	p_ai->uiIaMemType = 0;           /* TODO: 分析仪后面也要进行型号区分，暂时使用双路的内存分配方案 */
	p_ai->ai_chn = 0;                /* 引擎句柄数 */
	
    return;
}


/**
 * @fn      capb_disp
 * @brief   CAPB_DISP能力参数初始化
 * 
 * @param   p_disp[OUT] CAPB_DISP能力参数
 */
static void capb_disp(CAPB_DISP *p_disp)
{
    if (NULL == p_disp)
    {
        return;
    }

    return;
}


/**
 * @fn      capb_osd
 * @brief   CAPB_OSD能力参数初始化
 * 
 * @param   p_osd[OUT] CAPB_OSD能力参数
 */
static void capb_osd(CAPB_OSD *p_osd)
{
    if (NULL == p_osd)
    {
        return;
    }
    
    p_osd->enDrawMod  = DRAW_MOD_CPU;
    p_osd->enFontType = OSD_FONT_TRUETYPE;

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
    p_line->u32LineWidth = 4;
    p_line->bUseFract = SAL_FALSE;

    return;
}


/**
 * @fn      capb_venc
 * @brief   CAPB_VENC能力参数初始化
 * 
 * @param   p_venc[OUT] CAPB_VENC能力参数
 */
static void capb_venc(CAPB_VENC *p_venc)
{
    if (NULL == p_venc)
    {
        return;
    }
    
    p_venc->resolution = HD1080p_FORMAT;
    p_venc->venc_width = 1920;
    p_venc->venc_height = 1080;
    p_venc->venc_bps = 4096;
    p_venc->venc_bpsType = 1;                   /* 1:CBR, 0:VBR */
    p_venc->venc_maxwidth = 1920;
    p_venc->venc_maxheight = 1920;

    return;
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

	p_vdec->vdec_cnt = 0;
    p_vdec->vpsschn_cnt = 0;
    return;
}


/**
 * @fn		capb_audio
 * @brief	CAPB_AUDIO能力参数初始化
 * 
 * @param	p_audio[OUT] CAPB_AUDIO能力参数
 */
static void capb_audio(CAPB_AUDIO *p_audio)
{
    UINT32  u32BoardType = HARDWARE_GetBoardType();
    if (NULL == p_audio)
    {
        return;
    }


    SAL_INFO("board_type %#x audio idx %d\n", u32BoardType, p_audio->audiodevidx);

    return;
}


/**
 * @fn        capb_capt
 * @brief    CAPB_CAPT能力参数初始化
 * 
 * @param    p_capt[OUT] CAPB_CAPT能力参数
 */
static void capb_capt(CAPB_CAPT *p_capt, const BOARD_TYPE board_type)
{
    if (NULL == p_capt)
    {
        return;
    }

    /*产品型号未定义占时写死*/

    p_capt->bMcuEnable = SAL_FALSE;
    p_capt->auiHwMipi[0] = 0;
    p_capt->auiHwMipi[1] = 1;

    p_capt->uiUserFps  = 0;
    p_capt->bFpgaEnable = SAL_FALSE;
    p_capt->uiViCnt = 1;             /* TODO: 分析仪后续也根据型号进行区分 */

    p_capt->stCaptChip[0].u8ChipNum = 3;
    p_capt->stCaptChip[0].astChipInfo[0].enCaptChip = CAPT_CHIP_LT86102SXE;
    p_capt->stCaptChip[0].astChipInfo[0].stChipI2c.u8BusIdx = 3;
    p_capt->stCaptChip[0].astChipInfo[0].stChipI2c.u8ChipAddr = 0x70;

    p_capt->stCaptChip[0].astChipInfo[1].enCaptChip = CAPT_CHIP_MST91A4;
    p_capt->stCaptChip[0].astChipInfo[1].stChipUart.u8BusIdx = UART_ID_4;
    
    p_capt->stCaptChip[0].astChipInfo[2].enCaptChip = CAPT_CHIP_LT9211_MIPI;
    p_capt->stCaptChip[0].astChipInfo[2].stChipI2c.u8BusIdx = 0;
    p_capt->stCaptChip[0].astChipInfo[2].stChipI2c.u8ChipAddr = 0x5A;

    
    return;
}

/**
 * @fn		capb_dup
 * @brief	CAPB_DUP能力参数初始化
 * 
 * @param	p_dup[OUT] CAPB_DUP能力参数
 */
static void capb_dup(CAPB_DUP *p_dup)
{
    if (NULL == p_dup)
    {
        return;
    }

    p_dup->width  = 1920;
    p_dup->height = 1080;
    
#ifdef RK3588
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
#else
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
#endif

	return;
}

/**
 * @fn		capb_dup
 * @brief	CAPB_DUP能力参数初始化
 * 
 * @param	p_dup[OUT] CAPB_DUP能力参数
 */
static void capb_vdecDup(CAPB_VDEC_DUP *p_dup)
{
    if (NULL == p_dup)
    {
        return;
    }

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
	return;
}

/**
 * @fn		capb_jpeg
 * @brief	CAPB_JPEG能力参数初始化
 * 
 * @param	p_jpeg[OUT] CAPB_JPEG能力参数
 */
static void capb_jpeg(CAPB_JPEG *p_jpeg)
{
    if (NULL == p_jpeg)
    {
        return;
    }

    /* fixme, 这里单路板子和从片也都按照双路板子来配置*/
	p_jpeg->videv_cnt = 1;
	p_jpeg->maxdev_num = 1;
	p_jpeg->maxchn_num = 1; /*  vi抓图支持的最大个数*/

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

    pstProduct->enInputType  = VIDEO_INPUT_OUTSIDE;
    pstProduct->bXPackEnable = SAL_FALSE;
	pstProduct->bXRayEnable  = SAL_FALSE;
    pstProduct->bXspEnable   = SAL_FALSE;

    return;
}

/**
 * @fn		capb_platform
 * @brief	CAPB_PLATFORM能力参数初始化
 *
 * @param	pstPlatform[OUT] CAPB_PLATFORM能力参数
 */
static void capb_platform(CAPB_PLATFORM *pstPlatform) /* 20220810目前只在xpack里会用到 */
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
        pstPlatform->bDiscontAddrOsd = SAL_TRUE;    //是否支持不连续地址画osd
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
 * @fn		capb_dsp
 * @brief	dsp能力参数初始化
 * 
 * @param	p_dsp[OUT] CAPB_DSP能力集参数
 */
static void capb_dsp(CAPB_DSP *p_dsp)
{
	if (NULL == p_dsp)
	{
		return;
	}

    p_dsp->u32UseCoreNum = 0;
    p_dsp->au32Core[0]   = 0;
    p_dsp->au32Core[1]   = 0;

    return;
}

/**
 * @fn      capb_analyzer_init
 * @brief   分析仪能力集初始化
 * 
 * @param   p_overall[OUT] 能力集参数
 */
void capb_isa_box_init(CAPB_OVERALL *p_overall, const BOARD_TYPE board_type)
{
    if (NULL == p_overall)
    {
        return;
    }

	capb_product(&p_overall->capb_product);
    capb_ai(&p_overall->capb_ai);
    capb_disp(&p_overall->capb_disp);
    capb_osd(&p_overall->capb_osd);
    capb_line(&p_overall->capb_line);
    capb_venc(&p_overall->capb_venc);
	capb_vdec(&p_overall->capb_vdec);
	capb_audio(&p_overall->capb_audio);
	capb_capt(&p_overall->capb_capt, board_type);
	capb_dup(&p_overall->capb_dup);
    capb_vdecDup(&p_overall->capb_vdecDup);
    capb_vproc(&p_overall->capb_vproc);
	capb_jpeg(&p_overall->capb_jpeg);
    capb_dsp(&p_overall->capb_dsp);

    return;
}

