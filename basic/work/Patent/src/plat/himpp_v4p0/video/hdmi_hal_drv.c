/*******************************************************************************
 * hdmi_hal_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : huangshuxin <huangshuxin@hikvision.com>
 * Version: V1.0.0  2019年3月26日 Create
 *
 * Description : 
 * Modification: 
 *******************************************************************************/

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

#define HDMI_PRINT printf("[HDMI]%s[%d]:\t",__func__,__LINE__);printf

#define HDMIN_CHK_PAILURE_NORET(s32Ret) do{\
    if(s32Ret != HI_SUCCESS)\
    {\
        HDMI_PRINT("s32Ret=%d is not expected HI_SUCCESS!\n",s32Ret);\
    }\
}while(0);

#define HDMIN_CHK_PAILURE_RET(s32Ret) do{\
    if(s32Ret != HI_SUCCESS)\
    {\
        HDMI_PRINT("s32Ret=%d is not expected HI_SUCCESS!\n",s32Ret);\
        return HI_FAILURE;\
    }\
}while(0);

#define HDMI_CHK_FAILURE_GOTO(res,lable) do{\
    if(HI_FAILURE==res)\
        {HDMI_PRINT("return failure!\n");goto lable;}\
}while(0);

typedef struct hiHDMI_ARGS_S
{
    HI_HDMI_ID_E enHdmi;
    HI_HDMI_VIDEO_FMT_E eForceFmt;
}HDMI_ARGS_S;

HDMI_ARGS_S             stHdmiArgs;
HI_HDMI_CALLBACK_FUNC_S stCallbackFunc  = {0};
HI_HDMI_VIDEO_FMT_E     g_enVideoFmt    = HI_HDMI_VIDEO_FMT_1080P_60;
HI_HDMI_ID_E            g_enHdmiId      = HI_HDMI_ID_0;

/* HDMI 海思支持时序与宽、高、帧率的匹配关系 */
typedef struct tagHdmiResMap
{
    HI_HDMI_VIDEO_FMT_E enHdmiFormat;
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
} HDMI_RES_MAP_S;

/* 海思默认支持的时序，新增分辨率可以不在该数组添加，将使用自定义时序 */
/* 经过测试，将下列数组清空，即全部使用自定义模式，显示也正常 */
static const HDMI_RES_MAP_S gastHdmiResMaps[] = {
    {HI_HDMI_VIDEO_FMT_1080P_60,            1920,   1080,   60},
    {HI_HDMI_VIDEO_FMT_1080P_50,            1920,   1080,   50},
    {HI_HDMI_VIDEO_FMT_VESA_1280X1024_60,   1280,   1024,   60},
    {HI_HDMI_VIDEO_FMT_720P_60,             1280,    720,   60},
    {HI_HDMI_VIDEO_FMT_VESA_1024X768_60,    1024,    768,   60},
};



INT32 Hdmi_Hal_UnPlugProc(HI_VOID *pPrivateData)
{
    HI_S32 s32Ret = HI_SUCCESS;

    HDMI_ARGS_S *pArgs = (HDMI_ARGS_S *)pPrivateData;

    HI_HDMI_ID_E hHdmi = pArgs->enHdmi;

    HDMI_PRINT("\n --- UnPlug envent handling. ---\n");

    s32Ret = HI_MPI_HDMI_Stop(hHdmi);
    
    HDMIN_CHK_PAILURE_RET(s32Ret);

    return s32Ret;
}

INT32 Hdmi_Hal_HotPlugProc(HI_VOID *pPrivateData)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    HDMI_ARGS_S *pArgs = (HDMI_ARGS_S *)pPrivateData;

    HI_HDMI_ID_E hHdmi = pArgs->enHdmi;

    HI_HDMI_ATTR_S  stHdmiAttr;

    HI_HDMI_SINK_CAPABILITY_S   stSinkCap;
    
    HDMI_PRINT("\n --- HotPlug envent handling. ---\n");

    s32Ret = HI_MPI_HDMI_GetAttr(hHdmi,&stHdmiAttr);

    HDMIN_CHK_PAILURE_RET(s32Ret);

    s32Ret = HI_MPI_HDMI_GetSinkCapability(hHdmi,&stSinkCap);
    HDMIN_CHK_PAILURE_RET(s32Ret);

    if(HI_FAILURE == stSinkCap.bConnected)
    {
        HDMI_PRINT("stSinkCap.bConnected is HI_FAILURE\n");
        return HI_FAILURE;
    }
    
    if(HI_TRUE == stSinkCap.bSupportHdmi)
    {
        stHdmiAttr.bEnableHdmi = HI_TRUE;

        if(HI_TRUE != stSinkCap.bSupportYCbCr)
        {
            HDMI_PRINT("HI_HDMI_VIDEO_MODE_RGB444 \n");
            stHdmiAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
        }
        else
        {
            HDMI_PRINT("HI_HDMI_VIDEO_MODE_YCBCR444 \n");
            stHdmiAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_YCBCR444;
        }
    }
    else
    {    
        stHdmiAttr.enVidOutMode = HI_HDMI_VIDEO_MODE_RGB444;
        stHdmiAttr.bEnableHdmi = HI_FALSE;
    }

    stSinkCap.bSupportHdmi = HI_TRUE;
    stHdmiAttr.bEnableHdmi = HI_TRUE;

    if(HI_TRUE == stHdmiAttr.bEnableHdmi)
    {
        HDMI_PRINT("HI_HDMI_VIDEO_MODE_YCBCR444 \n");

        stHdmiAttr.bEnableHdmi           = HI_TRUE;
        stHdmiAttr.bEnableVideo          = HI_TRUE;
        stHdmiAttr.enVideoFmt            = pArgs->eForceFmt;    
        stHdmiAttr.enVidOutMode          = HI_HDMI_VIDEO_MODE_YCBCR444;
        stHdmiAttr.enDeepColorMode       = HI_HDMI_DEEP_COLOR_24BIT;
        stHdmiAttr.bxvYCCMode            = HI_FALSE;
        stHdmiAttr.enOutCscQuantization  = HDMI_QUANTIZATION_LIMITED_RANGE;
        stHdmiAttr.bEnableAudio          = HI_TRUE;
        stHdmiAttr.enSoundIntf           = HI_HDMI_SND_INTERFACE_I2S;
        stHdmiAttr.bIsMultiChannel       = HI_FALSE;
        stHdmiAttr.enBitDepth            = HI_HDMI_BIT_DEPTH_16;
        stHdmiAttr.bEnableAviInfoFrame   = HI_TRUE;
        stHdmiAttr.bEnableAudInfoFrame   = HI_TRUE;
        stHdmiAttr.bEnableSpdInfoFrame   = HI_FALSE;
        stHdmiAttr.bEnableMpegInfoFrame  = HI_FALSE;
        stHdmiAttr.bDebugFlag            = HI_FALSE;
        stHdmiAttr.bHDCPEnable           = HI_FALSE;
        stHdmiAttr.b3DEnable             = HI_FALSE;
        stHdmiAttr.enDefaultMode         = HI_HDMI_FORCE_HDMI;
        stHdmiAttr.bEnableVidModeAdapt   = HI_TRUE;
    }
    else
    {
        stHdmiAttr.bEnableAudio          = HI_FALSE;        
        stHdmiAttr.bEnableVideo          = HI_TRUE;        
        stHdmiAttr.bEnableAviInfoFrame   = HI_FALSE;
        stHdmiAttr.bEnableAudInfoFrame   = HI_FALSE;        
        stHdmiAttr.enVidOutMode          = HI_HDMI_VIDEO_MODE_YCBCR444;       
        stHdmiAttr.enDefaultMode         = HI_HDMI_FORCE_DVI;              
    }

    if(pArgs->eForceFmt < HI_HDMI_VIDEO_FMT_BUTT 
        && stSinkCap.bVideoFmtSupported[pArgs->eForceFmt])
    {
        HDMI_PRINT("set force format=%d\n",pArgs->eForceFmt);  
        stHdmiAttr.enVideoFmt = pArgs->eForceFmt;
    }
    else
    {
        HDMI_PRINT("not support format=%d we set native format=%d \n",pArgs->eForceFmt,stSinkCap.enNativeVideoFormat);
        stHdmiAttr.enVideoFmt = stSinkCap.enNativeVideoFormat;        
    }
            
    s32Ret = HI_MPI_HDMI_SetAttr(hHdmi,&stHdmiAttr);

    HDMIN_CHK_PAILURE_RET(s32Ret);

    s32Ret = HI_MPI_HDMI_Start(hHdmi);
        
    HDMIN_CHK_PAILURE_RET(s32Ret);

    return s32Ret;
}

VOID HDMI_Hal_EventProc(HI_HDMI_EVENT_TYPE_E event,HI_VOID *pPrivateData)
{   
    return ;
    #if 0
    switch(event)
    {
        case HI_HDMI_EVENT_HOTPLUG:
        {
            Hdmi_HotPlugProc(pPrivateData);

            break;
        }
        case HI_HDMI_EVENT_NO_PLUG:
        {
            Hdmi_UnPlugProc(pPrivateData);
            
            break;
        }
        
        case HI_HDMI_EVENT_EDID_FAIL:
        {
            
            break;
        }
        
        case HI_HDMI_EVENT_HDCP_FAIL:
        {            
            break;
        }
        case HI_HDMI_EVENT_HDCP_SUCCESS:
        {            
            break;
        }
        case HI_HDMI_EVENT_HDCP_USERSETTING:
        {            
            break;
        }
        default :
        {
            break;
        }
    }
    #endif
}


INT32 Hdmi_Hal_Start(HI_HDMI_VIDEO_FMT_E enVideoFmt, DYNAMIC_RANGE_E enDyRg, const BT_TIMING_S *pstTiming)
{
    HI_HDMI_ATTR_S          stAttr;
	HI_S32                  s32Ret      = HI_SUCCESS;
	s32Ret = HI_MPI_HDMI_GetAttr(g_enHdmiId, &stAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_GetAttr failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }
    stAttr.bEnableHdmi           = HI_FALSE;
    stAttr.bEnableVideo          = HI_TRUE;
    stAttr.enVideoFmt            = enVideoFmt;    
    //stAttr.enVidOutMode          = HI_HDMI_VIDEO_MODE_YCBCR444;    
    stAttr.enVidOutMode          = HI_HDMI_VIDEO_MODE_RGB444;    
    switch(enDyRg)
    {
        case DYNAMIC_RANGE_SDR8:
            stAttr.enDeepColorMode = HI_HDMI_DEEP_COLOR_24BIT;
            break;
        case DYNAMIC_RANGE_HDR10:
            stAttr.enVidOutMode    = HI_HDMI_VIDEO_MODE_YCBCR422;
            break;
        default:
            stAttr.enDeepColorMode = HI_HDMI_DEEP_COLOR_24BIT;
            break;
    }
    
    stAttr.bxvYCCMode            = HI_FALSE;
    stAttr.enOutCscQuantization  = HDMI_QUANTIZATION_LIMITED_RANGE;  
    stAttr.bEnableAudio          = HI_FALSE;
    stAttr.enSoundIntf           = HI_HDMI_SND_INTERFACE_I2S;
    stAttr.bIsMultiChannel       = HI_FALSE;
    stAttr.enBitDepth            = HI_HDMI_BIT_DEPTH_16;
    stAttr.bEnableAviInfoFrame   = HI_TRUE;
    stAttr.bEnableAudInfoFrame   = HI_TRUE;
    stAttr.bEnableSpdInfoFrame   = HI_FALSE;
    stAttr.bEnableMpegInfoFrame  = HI_FALSE;
    stAttr.bDebugFlag            = HI_FALSE;
    stAttr.bHDCPEnable           = HI_FALSE;
    stAttr.b3DEnable             = HI_FALSE;
    stAttr.enDefaultMode         = HI_HDMI_FORCE_DVI;

    if (HI_HDMI_VIDEO_FMT_VESA_CUSTOMER_DEFINE == enVideoFmt)
    {
        stAttr.u32PixClk = pstTiming->u32PixelClock / 1000;     // kHz
    }

    //stAttr.bAuthMode             = HI_TRUE;
   
	s32Ret = HI_MPI_HDMI_SetAttr(g_enHdmiId, &stAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_SetAttr failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }
	
	s32Ret = HI_MPI_HDMI_Start(g_enHdmiId);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_Start failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


INT32 Hdmi_Hal_Stop(VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;
        
    s32Ret = HI_MPI_HDMI_Stop(g_enHdmiId);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_Stop failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

INT32 Hdmi_Hal_SetHdmi(UINT32 uiVoLayer, const BT_TIMING_S *pstTiming)
{
    INT32 s32Ret = SAL_FAIL;
    HI_HDMI_VIDEO_FMT_E enVideoFmt = HI_HDMI_VIDEO_FMT_VESA_CUSTOMER_DEFINE;
    UINT32 i = 0;
    UINT32 u32ResNum = sizeof(gastHdmiResMaps)/sizeof(gastHdmiResMaps[0]);
    const HDMI_RES_MAP_S *pstHdmiMap = NULL;

    for (i = 0; i < u32ResNum; i++)
    {
        pstHdmiMap = gastHdmiResMaps + i;
        if ((pstHdmiMap->u32Width == pstTiming->u32Width) &&
            (pstHdmiMap->u32Height == pstTiming->u32Height) &&
            (pstHdmiMap->u32Fps == pstTiming->u32Fps))
        {
            enVideoFmt = pstHdmiMap->enHdmiFormat;
        }
    }

    s32Ret = Hdmi_Hal_Start(enVideoFmt,DYNAMIC_RANGE_SDR8, pstTiming);
    if(s32Ret != SAL_SOK)
    {
        SAL_LOGE("error!\n");
        return SAL_FAIL;
    }
#if 0
    /*R7新方案不需*/
    s32Ret = Hdmi_Hal_Set8612();    
    if(s32Ret != SAL_SOK)
    {
        SAL_LOGE("error!\n");
        return SAL_FAIL;
    }
#endif
    return SAL_SOK;
}

INT32 Hdmi_Hal_Init(VOID)
{
	HI_S32 s32Ret = HI_SUCCESS;

	s32Ret = HI_MPI_HDMI_Init();
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_Init failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }
	
	s32Ret = HI_MPI_HDMI_Open(g_enHdmiId);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_Open failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }
    
    HI_HDMI_EDID_S stEdidData;
    
    HI_MPI_HDMI_Force_GetEDID(g_enHdmiId,&stEdidData);
        
    HI_HDMI_SINK_CAPABILITY_S stSinkCap;
    
    s32Ret = HI_MPI_HDMI_GetSinkCapability(g_enHdmiId,&stSinkCap);

    SAL_LOGI("SupportHdmi %d EdidValid %d Edidlength %d \n",stSinkCap.bSupportHdmi,stEdidData.bEdidValid,stEdidData.u32Edidlength);

    stCallbackFunc.pfnHdmiEventCallback = HDMI_Hal_EventProc;

    stHdmiArgs.enHdmi = g_enHdmiId;

    stHdmiArgs.eForceFmt = g_enVideoFmt;

    stCallbackFunc.pPrivateData = &stHdmiArgs;

    HI_MPI_HDMI_RegCallbackFunc(g_enHdmiId,&stCallbackFunc);
    
    return SAL_SOK;
}

INT32 Hdmi_Hal_DeInit(VOID)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    stCallbackFunc.pfnHdmiEventCallback = HDMI_Hal_EventProc;

    stCallbackFunc.pPrivateData = &stHdmiArgs;

    HI_MPI_HDMI_UnRegCallbackFunc(g_enHdmiId,&stCallbackFunc);

    s32Ret = HI_MPI_HDMI_Close(g_enHdmiId);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_Close failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }
    
    s32Ret = HI_MPI_HDMI_DeInit();
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_HDMI_DeInit failed 0x%x !!!\n",s32Ret);
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

