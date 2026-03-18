
/**
 * @file    capbility_analyzer.c
 * @note    Hangzhou Hikvision Digital Technology Co., Ltd. All Right Reserved 
 * @brief   分析仪能力集
 */

#include "capbility.h"
#include "system_prm_api.h"
#include <platform_hal.h>

/* ============================ 宏/枚举 ============================ */
#line __LINE__ "capbility_analyzer.c"


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

	p_ai->uiCbJpgNum = 0x11;
	p_ai->uiCbBmpNum = 0x1;
	p_ai->uiIaMemType = 1;           /* TODO: 分析仪后面也要进行型号区分，暂时使用双路的内存分配方案 */
	p_ai->ai_chn = 1;                /* 引擎句柄数 */
	
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
    
    p_disp->disp_screen_bg_color = 0;
    p_disp->disp_delay_nums = 5;
    p_disp->disp_link_type = MOD_LINK_TYPE_USER;
    p_disp->disp_mipi_chip = MIPI_TX_CHIP_6211;
    
#ifdef RK3588
        p_disp->enInputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
#else
        p_disp->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
#endif

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

    p_osd->enDrawMod  = DRAW_MOD_VGS;
    p_osd->enFontType = OSD_FONT_TRUETYPE;
    p_osd->TrueTypeSize[0] = 18;
    p_osd->TrueTypeSize[1] = 24;
    p_osd->TrueTypeSize[2] = 30;

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
    p_line->bUseFract = SAL_TRUE;
    p_line->u32DispLineWidth = 2;

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
    p_venc->venc_scale  = 1;              /* rk分析仪的缩放放在编码模块内部做 */


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
static void capb_audio(CAPB_AUDIO *p_audio)
{
    UINT32  u32BoardType = HARDWARE_GetBoardType();
    if (NULL == p_audio)
    {
        return;
    }
	
    if(DB_TS3637_V1_0 == u32BoardType)
    {
        p_audio->audiodevidx = 0;

    }
    else
    {
	    p_audio->audiodevidx = 1;
    }

    SAL_LOGI("board_type %#x audio idx %d\n", u32BoardType, p_audio->audiodevidx);

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
    UINT32  u32BoardType = HARDWARE_GetBoardType();

    if (NULL == p_capt)
    {
        return;
    }

    /*产品型号未定义占时写死*/
    if((u32BoardType == DB_RS20016_V1_0) || (u32BoardType == DB_RS20016_V1_1) 
        || (u32BoardType == DB_RS20016_V1_1_F303) || (u32BoardType == DB_TS3637_V1_0))
    {
        p_capt->bMcuEnable = SAL_TRUE;
        p_capt->auiHwMipi[0] = 0;
        p_capt->auiHwMipi[1] = 3;
    }
    else if(u32BoardType == DB_TS3719_V1_0 || u32BoardType == DB_RS20046_V1_0)
    {
        p_capt->bMcuEnable = SAL_TRUE;
        p_capt->auiHwMipi[0] = 0;
        p_capt->auiHwMipi[1] = 1;

        
        p_capt->stCaptChip[0].u8ChipNum = 1;
        p_capt->stCaptChip[0].astChipInfo[0].enCaptChip = CAPT_CHIP_LT9211_MIPI;
        p_capt->stCaptChip[0].astChipInfo[0].stChipI2c.u8BusIdx = 2;
        p_capt->stCaptChip[0].astChipInfo[0].stChipI2c.u8ChipAddr = 0x5A;

        p_capt->stCaptChip[1].u8ChipNum = 1;
        p_capt->stCaptChip[1].astChipInfo[0].enCaptChip = CAPT_CHIP_LT9211_MIPI;
        p_capt->stCaptChip[1].astChipInfo[0].stChipI2c.u8BusIdx = 7;
        p_capt->stCaptChip[1].astChipInfo[0].stChipI2c.u8ChipAddr = 0x5A;
    }
    else
    {
        p_capt->bMcuEnable = SAL_FALSE;
        p_capt->auiHwMipi[0] = 0;
        p_capt->auiHwMipi[1] = 1;
    }
    p_capt->uiUserFps  = 0;
    p_capt->bFpgaEnable = SAL_TRUE;
    p_capt->uiViCnt = 1;             /* TODO: 分析仪后续也根据型号进行区分 */

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
    UINT32 i = 0;
    if (NULL == p_dup)
    {
        return;
    }

    p_dup->width  = 1920;
    p_dup->height = 1080;
#ifndef SS928V100_SPC010
    p_dup->u32FrontBindRear = 1;
#endif

    p_dup->frontVpssGrp.uiBlkCnt = 56; //后级是disp0(缓存7+depth2+冗余2),disp1,sva（缓存12(需等算法处理完毕之后编图)+depth2+冗余2）,jpeg(depth1+冗余2)，具体见link_task.c里实例节点的配置
    p_dup->frontVpssGrp.uiDepth = 2;
    p_dup->rearVpssGrp.uiBlkCnt =  12;  //(depth1+冗余2)x4
    p_dup->rearVpssGrp.uiDepth = 1;

    for (i = 0; i < DUP_VPSS_CHN_MAX_NUM; i++)
    {
        p_dup->frontVpssGrp.uiChnEnable[i] = ENABLED;  //是否使能已经不再使用，通过link_task.c里实例配置信息里的节点个数来使能对应的vpssChn
        p_dup->rearVpssGrp.uiChnEnable[i] = ENABLED;
    }

#ifdef RK3588
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_RGB24_888;
    p_dup->frontVpssGrp.enVpssDevType = VPSS_DEV_GPU;
    p_dup->rearVpssGrp.enVpssDevType = VPSS_DEV_GPU;
#else
    p_dup->enInputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->enOutputSalPixelFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    p_dup->frontVpssGrp.enVpssDevType = VPSS_DEV_VPE;
    p_dup->rearVpssGrp.enVpssDevType = VPSS_DEV_VPE;
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

    p_dup->width  = 1920;
    p_dup->height = 1080;
    p_dup->u32HasFrontDupScale = 0;
    p_dup->frontVpssGrp.uiDepth = 1;
    p_dup->rearVpssGrp.uiDepth = 1;

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
	p_jpeg->videv_cnt = 2;
	p_jpeg->maxdev_num = 2 + 10;
	p_jpeg->maxchn_num = 2 + 10 + 2 + 2; /*  vi抓图支持的最大个数+ 解码路数+ 2个智能抓图的通道+2个转存通道*/

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
static void capb_platform(CAPB_PLATFORM *pstPlatform)  /* 20220810目前只在xpack里会用到 */
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

    p_dsp->u32UseCoreNum = 2;
    p_dsp->au32Core[0]   = 2;
    p_dsp->au32Core[1]   = 3;

    return;
}

/**
 * @fn      capb_analyzer_init
 * @brief   分析仪能力集初始化
 * 
 * @param   p_overall[OUT] 能力集参数
 */
static void capb_isaInitDefault(CAPB_OVERALL *p_overall, const BOARD_TYPE board_type)
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

/**
 * @fn      capb_analyzer_init
 * @brief   分析仪能力集初始化
 * 
 * @param   p_overall[OUT] 能力集参数
 */
void capb_isa_init(CAPB_OVERALL *p_overall, const BOARD_TYPE board_type)
{
    UINT32  u32BoardType = HARDWARE_GetBoardType();

    if (NULL == p_overall)
    {
        return;
    }

    switch(u32BoardType)
    {
        case DB_RS20032_V1_0:
            capb_isa_box_init(p_overall,board_type);
            break;
        default:
            capb_isaInitDefault(p_overall,board_type);
            break;
    }

    return;
}

