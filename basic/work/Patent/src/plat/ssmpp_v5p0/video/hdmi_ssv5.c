/**
 * @file   hdmi_ssv5.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  显示hdmi模块
 * @author liuxianying
 * @date   2021/8/4
 * @note
 * @note \n History
   1.日    期: 2021/8/4
     作    者: liuxianying
     修改历史: 创建文件
 */

#include <platform_sdk.h>
#include <platform_hal.h>



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

#define HDMI_PRINT printf("[HDMI]%s[%d]:\t", __func__, __LINE__); printf

#define HDMIN_CHK_PAILURE_NORET(s32Ret) do { \
        if (s32Ret != TD_SUCCESS) \
        { \
            HDMI_PRINT("s32Ret=%d is not expected TD_SUCCESS!\n", s32Ret); \
        } \
} while (0);

#define HDMIN_CHK_PAILURE_RET(s32Ret) do { \
        if (s32Ret != TD_SUCCESS) \
        { \
            HDMI_PRINT("s32Ret=%d is not expected TD_SUCCESS!\n", s32Ret); \
            return TD_FAILURE; \
        } \
} while (0);

#define HDMI_CHK_FAILURE_GOTO(res, lable) do { \
        if (TD_FAILURE == res) \
        {HDMI_PRINT("return failure!\n"); goto lable; } \
} while (0);

typedef struct hiHDMI_ARGS_S
{
    ot_hdmi_id enHdmi;
    ot_hdmi_video_format eForceFmt;
} HDMI_ARGS_S;

HDMI_ARGS_S stHdmiArgs;
ot_hdmi_callback_func stCallbackFunc = {0};
ot_hdmi_video_format g_enVideoFmt = OT_HDMI_VIDEO_FORMAT_1080P_60;
ot_hdmi_id g_enHdmiId = OT_HDMI_ID_0;

/* HDMI 海思支持时序与宽、高、帧率的匹配关系 */
typedef struct tagHdmiResMap
{
    ot_hdmi_video_format enHdmiFormat;
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
} HDMI_RES_MAP_S;


/* 海思默认支持的时序，新增分辨率可以不在该数组添加，将使用自定义时序 */
/* 经过测试，将下列数组清空，即全部使用自定义模式，显示也正常 */
static const HDMI_RES_MAP_S gastHdmiResMaps[] = {
    {OT_HDMI_VIDEO_FORMAT_1080P_60, 1920, 1080, 60},
    {OT_HDMI_VIDEO_FORMAT_1080P_50, 1920, 1080, 50},
    {OT_HDMI_VIDEO_FORMAT_VESA_1280X1024_60, 1280, 1024, 60},
    {OT_HDMI_VIDEO_FORMAT_720P_60, 1280, 720, 60},
    {OT_HDMI_VIDEO_FORMAT_VESA_1024X768_60, 1024, 768, 60},
};


/**
 * @function   Hdmi_Hal_UnPlugProc
 * @brief      拔出HDMI处理函数
 * @param[in]  VOID *pPrivateData  
 * @param[out] None
 * @return     INT32 SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_UnPlugProc(VOID *pPrivateData)
{
    INT32 s32Ret = TD_SUCCESS;

    HDMI_ARGS_S *pArgs = (HDMI_ARGS_S *)pPrivateData;

    ot_hdmi_id hHdmi = pArgs->enHdmi;

    HDMI_PRINT("\n --- UnPlug envent handling. ---\n");

    s32Ret = ss_mpi_hdmi_stop(hHdmi);

    HDMIN_CHK_PAILURE_RET(s32Ret);

    return s32Ret;
}


/**
 * @function   Hdmi_Hal_HotPlugProc
 * @brief      hdmi热插拔处理函数
 * @param[in]  VOID *pPrivateData  私有数据
 * @param[out] None
 * @return     INT32 SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_HotPlugProc(VOID *pPrivateData)
{
    INT32 s32Ret = TD_SUCCESS;

    HDMI_ARGS_S *pArgs = (HDMI_ARGS_S *)pPrivateData;

    ot_hdmi_id hHdmi = pArgs->enHdmi;

    ot_hdmi_attr stHdmiAttr;

    ot_hdmi_sink_capability stSinkCap;


    HDMI_PRINT("\n --- HotPlug envent handling. ---\n");

    s32Ret = ss_mpi_hdmi_get_attr(hHdmi, &stHdmiAttr);

    HDMIN_CHK_PAILURE_RET(s32Ret);

    s32Ret = ss_mpi_hdmi_get_sink_capability(hHdmi, &stSinkCap);
    HDMIN_CHK_PAILURE_RET(s32Ret);

    if (TD_FAILURE == stSinkCap.is_connected)
    {
        HDMI_PRINT("stSinkCap.is_connected is TD_FAILURE\n");
        return TD_FAILURE;
    }

    if (TD_TRUE == stSinkCap.support_hdmi)
    {
        stHdmiAttr.hdmi_en = TD_TRUE;

        if (TD_TRUE != stSinkCap.support_ycbcr)
        {
            HDMI_PRINT("OT_HDMI_VIDEO_MODE_RGB444 \n");
            /* stHdmiAttr.enVidOutMode = OT_HDMI_VIDEO_MODE_RGB444;  / *不支持该成员，新接口没有找到对应成员* / */
        }
        else
        {
            HDMI_PRINT("OT_HDMI_VIDEO_MODE_YCBCR444 \n");
            /* stHdmiAttr.enVidOutMode = OT_HDMI_VIDEO_MODE_YCBCR444; */
        }
    }
    else
    {
        /* stHdmiAttr.enVidOutMode = OT_HDMI_VIDEO_MODE_RGB444; */
        stHdmiAttr.hdmi_en = TD_FALSE;
    }

    stSinkCap.support_hdmi = TD_TRUE;
    stHdmiAttr.hdmi_en = TD_TRUE;

    if (TD_TRUE == stHdmiAttr.hdmi_en)
    {
        HDMI_PRINT("OT_HDMI_VIDEO_MODE_YCBCR444 \n");

        stHdmiAttr.hdmi_en = TD_TRUE;

        stHdmiAttr.video_format = pArgs->eForceFmt;
        stHdmiAttr.deep_color_mode = OT_HDMI_DEEP_COLOR_24BIT;
        /* stHdmiAttr.bxvYCCMode = TD_FALSE; */
        /* stHdmiAttr.enOutCscQuantization = HDMI_QUANTIZATION_LIMITED_RANGE; */
        stHdmiAttr.audio_en = TD_TRUE;
        /*  stHdmiAttr.enSoundIntf = OT_HDMI_SND_INTERFACE_I2S; */
        /*  stHdmiAttr.bIsMultiChannel = TD_FALSE; */
        stHdmiAttr.bit_depth = OT_HDMI_BIT_DEPTH_16;
        /*   stHdmiAttr.bEnableAviInfoFrame = TD_TRUE; */
        /*   stHdmiAttr.bEnableAudInfoFrame = TD_TRUE; */
        /*   stHdmiAttr.bEnableSpdInfoFrame = TD_FALSE; */
        /*  stHdmiAttr.bEnableMpegInfoFrame = TD_FALSE; */
        /*  stHdmiAttr.bDebugFlag = TD_FALSE; */
        /*   stHdmiAttr.bHDCPEnable = TD_FALSE; */
        /*  stHdmiAttr.b3DEnable = TD_FALSE; */
        /*  stHdmiAttr.enDefaultMode = OT_HDMI_FORCE_HDMI; */
        /*  stHdmiAttr.bEnableVidModeAdapt = TD_TRUE; */
    }
    else
    {
        stHdmiAttr.audio_en = TD_FALSE;
        stHdmiAttr.hdmi_en = TD_TRUE;
        /*   stHdmiAttr.bEnableAviInfoFrame = TD_FALSE; */
        /*  stHdmiAttr.bEnableAudInfoFrame = TD_FALSE; */
        /*   stHdmiAttr.enVidOutMode = OT_HDMI_VIDEO_MODE_YCBCR444; */
        /*   stHdmiAttr.enDefaultMode = OT_HDMI_FORCE_DVI; */
    }

    if (pArgs->eForceFmt < OT_HDMI_VIDEO_FORMAT_BUTT
        && stSinkCap.support_video_format[pArgs->eForceFmt])
    {
        HDMI_PRINT("set force format=%d\n", pArgs->eForceFmt);
        stHdmiAttr.video_format = pArgs->eForceFmt;
    }
    else
    {
        HDMI_PRINT("not support format=%d we set native format=%d \n", pArgs->eForceFmt, stSinkCap.native_video_format);
        stHdmiAttr.video_format = stSinkCap.native_video_format;
    }

    s32Ret = ss_mpi_hdmi_set_attr(hHdmi, &stHdmiAttr);

    HDMIN_CHK_PAILURE_RET(s32Ret);

    s32Ret = ss_mpi_hdmi_start(hHdmi);

    HDMIN_CHK_PAILURE_RET(s32Ret);

    return s32Ret;
}

/**
 * @function   HDMI_Hal_EventProc
 * @brief      hdmi注册回调函数
 * @param[in]  ot_hdmi_event_type event  事件类型
 * @param[in]  VOID *pPrivateData        私有数据
 * @param[out] None
 * @return     VOID
 */
VOID HDMI_Hal_EventProc(ot_hdmi_event_type event, VOID *pPrivateData)
{
    return;
    #if 0
    switch (event)
    {
        case OT_HDMI_EVENT_HOTPLUG:
        {
            Hdmi_HotPlugProc(pPrivateData);

            break;
        }
        case OT_HDMI_EVENT_NO_PLUG:
        {
            Hdmi_UnPlugProc(pPrivateData);

            break;
        }

        case OT_HDMI_EVENT_EDID_FAIL:
        {

            break;
        }

        case OT_HDMI_EVENT_HDCP_FAIL:
        {
            break;
        }
        case OT_HDMI_EVENT_HDCP_SUCCESS:
        {
            break;
        }
        case OT_HDMI_EVENT_HDCP_USERSETTING:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    #endif
}


/**
 * @function   Hdmi_Hal_Start
 * @brief      启动hdmi显示设备
 * @param[in]  ot_hdmi_video_format enVideoFmt 视频格式
 * @param[in]  ot_dynamic_range enDyRg         动态范围
 * @param[in]  const BT_TIMING_S *pstTiming    hdmi显示参数
 * @param[out] None
 * @return     INT32 SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_Start(ot_hdmi_video_format enVideoFmt, ot_dynamic_range enDyRg, const BT_TIMING_S *pstTiming)
{
    ot_hdmi_attr stAttr;
    INT32 s32Ret = TD_SUCCESS;
    ot_vo_hdmi_param hdmi_param = {0};


    s32Ret = ss_mpi_vo_get_hdmi_param(0, &hdmi_param);
    if (TD_SUCCESS != s32Ret)
    {
       SAL_LOGE("ss_mpi_vo_get_hdmi_param failed 0x%x !!!\n", s32Ret);
       return SAL_FAIL;
    }

    ss_mpi_hdmi_stop(OT_HDMI_ID_0);

    /*设置hdmi输出转换矩阵是否必要待确认，设置该接口前需要停止HDMI*/
    hdmi_param.csc.csc_matrix = OT_VO_CSC_MATRIX_BT709FULL_TO_RGBLIMIT;
    s32Ret = ss_mpi_vo_set_hdmi_param(0, &hdmi_param);
    if (TD_SUCCESS != s32Ret)
    {
       SAL_LOGE("ss_mpi_vo_set_hdmi_param failed 0x%x !!!\n", s32Ret);
       return SAL_FAIL;
    }

    s32Ret = ss_mpi_hdmi_get_attr(g_enHdmiId, &stAttr);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("ss_mpi_hdmi_get_attr failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    stAttr.hdmi_en = TD_TRUE;
    stAttr.video_format = enVideoFmt;

    switch (enDyRg)
    {
        case OT_DYNAMIC_RANGE_SDR8:
            stAttr.deep_color_mode = OT_HDMI_DEEP_COLOR_24BIT;
            break;
        case OT_DYNAMIC_RANGE_HDR10:
            /*需要确认设置是否正确*/
            /* stAttr.enVidOutMode = OT_HDMI_VIDEO_MODE_YCBCR422; */
            stAttr.deep_color_mode = OT_HDMI_DEEP_COLOR_24BIT;
            break;
        default:
            stAttr.deep_color_mode = OT_HDMI_DEEP_COLOR_24BIT;
            break;
    }

    stAttr.audio_en = TD_FALSE;
    stAttr.bit_depth = OT_HDMI_BIT_DEPTH_16;
    stAttr.sample_rate = OT_HDMI_SAMPLE_RATE_48K;

    if (OT_HDMI_VIDEO_FORMAT_VESA_CUSTOMER_DEFINE == enVideoFmt)
    {
        stAttr.pix_clk = pstTiming->u32PixelClock / 1000;     /* kHz */
    }
    
    s32Ret = ss_mpi_hdmi_set_attr(g_enHdmiId, &stAttr);
    if (TD_SUCCESS != s32Ret)
    {
       SAL_LOGE("ss_mpi_hdmi_set_attr failed 0x%x !!!\n", s32Ret);
       return SAL_FAIL;
    }

    s32Ret = ss_mpi_hdmi_start(g_enHdmiId);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("OT_MPI_HDMI_Start failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Hdmi_Hal_Stop
 * @brief      停止hdmi输出
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32 SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_Stop(VOID)
{
    INT32 s32Ret = TD_SUCCESS;

    s32Ret = ss_mpi_hdmi_stop(g_enHdmiId);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("ss_mpi_hdmi_stop failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   Hdmi_Hal_SetHdmi
 * @brief      设置hdmi输出设备参数
 * @param[in]  UINT32 uiVoLayer              
 * @param[in]  const BT_TIMING_S *pstTiming  hdmi设备信息
 * @param[out] None
 * @return     INT32 SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_SetHdmi(UINT32 uiVoLayer, const BT_TIMING_S *pstTiming)
{
    INT32 s32Ret = SAL_FAIL;
    ot_hdmi_video_format enVideoFmt = OT_HDMI_VIDEO_FORMAT_VESA_CUSTOMER_DEFINE;
    UINT32 i = 0;
    UINT32 u32ResNum = sizeof(gastHdmiResMaps) / sizeof(gastHdmiResMaps[0]);
    const HDMI_RES_MAP_S *pstHdmiMap = NULL;

    for (i = 0; i < u32ResNum; i++)
    {
        pstHdmiMap = gastHdmiResMaps + i;
        if ((pstHdmiMap->u32Width == pstTiming->u32Width)
            && (pstHdmiMap->u32Height == pstTiming->u32Height)
            && (pstHdmiMap->u32Fps == pstTiming->u32Fps))
        {
            enVideoFmt = pstHdmiMap->enHdmiFormat;
        }
    }

    s32Ret = Hdmi_Hal_Start(enVideoFmt, OT_DYNAMIC_RANGE_SDR8, pstTiming);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("error!\n");
        return SAL_FAIL;
    }

#if 0
    /*安检R7新方案设置8162通过MCU设置这里不需要设*/
    s32Ret = Hdmi_Hal_Set8612();
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("error!\n");
        return SAL_FAIL;
    }
#endif

    return SAL_SOK;
}

/**
 * @function    Hdmi_Hal_Init
 * @brief       初始化hdmi设备
 * @param[in]
 * @param[out]
 * @return           SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_Init(VOID)
{
    INT32 s32Ret = TD_SUCCESS;
    ot_hdmi_sink_capability stSinkCap = {0};
    ot_hdmi_edid stEdidData = {0};

    s32Ret = ss_mpi_hdmi_init();
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("OT_MPI_HDMI_Init failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_hdmi_open(g_enHdmiId);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("OT_MPI_HDMI_Open failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_hdmi_force_get_edid(g_enHdmiId, &stEdidData);
    if (TD_SUCCESS != s32Ret)
    {
       SAL_LOGE("ss_mpi_hdmi_force_get_edid failed 0x%x !!!\n", s32Ret);
    }

    s32Ret = ss_mpi_hdmi_get_sink_capability(g_enHdmiId, &stSinkCap);
    if (TD_SUCCESS != s32Ret)
    {
       SAL_LOGE("ss_mpi_hdmi_get_sink_capability failed 0x%x !!!\n", s32Ret);
    }

    SAL_LOGI("SupportHdmi %d EdidValid %d Edidlength %d \n", stSinkCap.support_hdmi, stEdidData.edid_valid, stEdidData.edid_len);

    stCallbackFunc.hdmi_event_callback = HDMI_Hal_EventProc;

    stHdmiArgs.enHdmi = g_enHdmiId;

    stHdmiArgs.eForceFmt = g_enVideoFmt;

    stCallbackFunc.private_data = &stHdmiArgs;

    s32Ret = ss_mpi_hdmi_register_callback(g_enHdmiId, &stCallbackFunc);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("ss_mpi_hdmi_register_callback failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Hdmi_Hal_DeInit
 * @brief      去初始化hdmi设备
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32 SAL_SOK 
 *                   SAL_FAIL
 */
INT32 Hdmi_Hal_DeInit(VOID)
{
    INT32 s32Ret = TD_SUCCESS;

    stCallbackFunc.hdmi_event_callback = HDMI_Hal_EventProc;

    stCallbackFunc.private_data = &stHdmiArgs;

    ss_mpi_hdmi_unregister_callback(g_enHdmiId, &stCallbackFunc);

    s32Ret = ss_mpi_hdmi_close(g_enHdmiId);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("OT_MPI_HDMI_Close failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_hdmi_deinit();
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("OT_MPI_HDMI_DeInit failed 0x%x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

