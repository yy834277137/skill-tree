/**
 * @file   capt_hisi.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台采集模块
 * @author liuxianying
 * @date   2021/8/6
 * @note
 * @note \n History
   1.日    期: 2021/8/6
     作    者: liuxianying
     修改历史: 创建文件
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include "capbility.h"
#include "capt_hisi.h"
#include "../../hal/hal_inc_inter/capt_hal_inter.h"

/*****************************************************************************
                               宏定义
*****************************************************************************/

#define VGA_OFFSET_CHECK_PIXEL      (4)     // 计算VGA图像偏移的像素点个数

/*****************************************************************************
                               结构体定义
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/
CAPT_MOD_PRM g_stCaptModPrm = {0};
static VI_USERPIC_ATTR_S gstUserPicAttr[CAPT_CHN_NUM_MAX] = {0};
extern int errno;
static CAPT_PLAT_OPS g_stCaptPlatOps;

/*****************************************************************************
                                函数
*****************************************************************************/

#ifdef NEW_LIUXY_NULL   /*ISP安检未使用废弃函数*/

/*****************************************************************************
 函 数 名  : Hal_drvIspTskProc
 功能描述  : ISP 处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static void* Hal_drvIspTskProc(void* param)
{
    UINT32 IspDev  = 0;
    INT32  s32Ret  = 0;

    IspDev = (UINT32)((PhysAddr)param);
    CAPT_LOGI("ISP DEV %d !!!\n", IspDev);

    SAL_SET_THR_NAME();

    s32Ret = HI_MPI_ISP_Run(IspDev);

    if (HI_SUCCESS != s32Ret)
    {
        CAPT_LOGE("HI_MPI_ISP_Run failed with %#x!\n", s32Ret);
        return NULL;
    }

    CAPT_LOGI("ISP Dev %d HI_MPI_ISP_Run\n", IspDev);

    return NULL;
}

/*****************************************************************************
 函 数 名  : Hal_drvIspTsk
 功能描述  : 运行 IPS 处理线程
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Hal_drvIspTsk(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32 s32Ret = 0;

    UINT32 uiIspDev = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspDev = pstPrm->uiIspId;

    s32Ret = SAL_thrCreate(&pstPrm->IspPid, Hal_drvIspTskProc, SAL_THR_PRI_DEFAULT, SAL_THR_STACK_SIZE_DEFAULT,(HI_VOID *)((PhysAddr)uiIspDev));
    if (0 != s32Ret)
    {
        CAPT_LOGE("Create isp running thread failed!, error: %d, %s\r\n",s32Ret, strerror(s32Ret));
        return SAL_FAIL;
    }

    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : Hal_drvIspInit
 功能描述  : 初始化 ISP
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Hal_drvIspInit(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32 s32Ret    = 0;
    UINT32 uiIspDev = 0;

    //ISP_WDR_MODE_S stWdrMode;
    ISP_PUB_ATTR_S *pstPubAttr;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspDev = pstPrm->uiIspId;

    if (HI_SUCCESS != (s32Ret = HI_MPI_ISP_MemInit(uiIspDev)))
    {
        CAPT_LOGE("Init Ext memory failed, s32Ret %x !!!\n", s32Ret);
        HI_MPI_ISP_Exit(uiIspDev);

        return SAL_FAIL;
    }

    pstPubAttr = &pstPrm->stIspPubAttr;

    if (HI_SUCCESS != (s32Ret= HI_MPI_ISP_SetPubAttr(uiIspDev, pstPubAttr)))
    {
        CAPT_LOGE("Func: %s, Line: %d, SetPubAttr failed(0x%x)\n", __FUNCTION__, __LINE__,s32Ret);
        HI_MPI_ISP_Exit(uiIspDev);

        return SAL_FAIL;
    }

    if (HI_SUCCESS != HI_MPI_ISP_Init(uiIspDev))
    {
        CAPT_LOGE("Func: %s, Line: %d, ISP Init failed\n", __FUNCTION__, __LINE__);
        HI_MPI_ISP_Exit(uiIspDev);

        return SAL_FAIL;
    }

    CAPT_LOGI("End Init IspDev:%d.\n", uiIspDev);

    pstPrm->uiIspInited = 1;

    return SAL_SOK;
}



/*****************************************************************************
 函 数 名  : Hal_drvRegisterSnsLib
 功能描述  : 注册 sensor 库
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 Hal_drvRegisterSnsLib(UINT32 uiIspId)
{
    INT32 s32Ret = 0;

    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    //ALG_LIB_S stAfLib;
    ISP_SNS_COMMBUS_U uSnsBusInfo;
    ISP_SNS_OBJ_S *pstSnsObj = &g_stCaptModPrm.stCaptHalChnPrm[uiIspId].stSnsObj;

    memset(&stAeLib, 0, sizeof(ALG_LIB_S));
    memset(&stAwbLib, 0, sizeof(ALG_LIB_S));
   // memset(&stAfLib, 0, sizeof(ALG_LIB_S));

    /* 初始化sensor接口 */
    memcpy(pstSnsObj, &stSnsImx334Obj, sizeof(ISP_SNS_OBJ_S));

    /* 注册处理库*/
    stAeLib.s32Id  = uiIspId;
    stAwbLib.s32Id = uiIspId;
    //stAfLib.s32Id  = uiIspId;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
   // strncpy(stAfLib.acLibName, HI_AF_LIB_NAME, sizeof(HI_AF_LIB_NAME));

    if (pstSnsObj->pfnRegisterCallback != HI_NULL)
    {
        s32Ret = pstSnsObj->pfnRegisterCallback(uiIspId, &stAeLib, &stAwbLib);
        if (HI_SUCCESS != s32Ret)
        {
            CAPT_LOGE("sensor_register_callback failed with %#x!\n",s32Ret);
            return SAL_FAIL;
        }
    }
    
    uSnsBusInfo.s8I2cDev = uiIspId;
    if (HI_NULL != pstSnsObj->pfnSetBusInfo)
    {
        printf("s8I2cDev is %d\n",uiIspId);
        s32Ret = pstSnsObj->pfnSetBusInfo(uiIspId, uSnsBusInfo);

        if (s32Ret != HI_SUCCESS)
        {
            CAPT_LOGE("set sensor bus info failed with %#x!\n", s32Ret);
            return s32Ret;
        }
    }
    else
    {
        CAPT_LOGE("not support set sensor bus info!\n");
        return HI_FAILURE;
    }
    
    if (HI_SUCCESS != HI_MPI_AWB_Register(uiIspId, &stAwbLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AWB Register failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }
    /*
    if (HI_SUCCESS != HI_MPI_AF_Register(uiIspId, &stAfLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AF Register failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }
    */
    if (HI_SUCCESS != HI_MPI_AE_Register(uiIspId, &stAeLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AF Register failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Hal_drvRegisterIspLib
 功能描述  : 注册ISP库属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Hal_drvRegisterIspLib(CAPT_HAL_CHN_PRM * pstPrm)
{
    UINT32 uiIspId = 0;
    //UINT32 uiSnsId = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspId = pstPrm->uiIspId;
    //uiSnsId = pstPrm->uiSnsId;

    /* 注册 isp 回调与isp 库 */
    if (SAL_SOK != Hal_drvRegisterSnsLib(uiIspId))
    {
        CAPT_LOGE("HAL Regeister Isp Lib Faiiled !!!\n");
        capt_hisiUnRegisterIspLib(uiIspId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Hal_drvInitIsp
 功能描述  : 配置 sensor与 ISP属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Hal_drvIspInitAndRun(CAPT_HAL_CHN_PRM * pstPrm)
{
    //UINT32 uiIspId = 0;
    //UINT32 uiSnsId = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
    #if 0
    uiIspId = pstPrm->uiIspId;
    uiSnsId = pstPrm->uiSnsId;

    /* 初始化 isp 模块 */
    if (SAL_FAIL == Hal_drvIspInit(pstPrm))
    {
        CAPT_LOGE("Capt Hal Isp Set Failed !!!\n");
        capt_hisiIspDeInit(pstPrm);
        capt_hisiUnRegisterIspLib(uiIspId);
        return SAL_FAIL;
    }

    /* 开启 isp 运行线程 */
    Hal_drvIspTsk(pstPrm);

    g_stCaptModPrm.stCaptHalChnPrm[uiIspId].uiSnsId = uiSnsId;

    return SAL_SOK;
    #endif
}



INT32 Isp_drvSemPost(int IspDev)
{
    return SAL_SOK;
}

INT32 Isp_drvSemPend(int IspDev)
{
    return SAL_SOK;
}

INT32 Isp_drvSet3dDenoise(int IspDev,void *pParam)
{
    UINT32 uiVpssroup = 0;
    INT32  iRet       = 0;

    if (0 == IspDev)
    {
        uiVpssroup = 0;
    }
    else
    {
        uiVpssroup = 2;
    }
    
    // CAPT_LOGE("Isp Dev %d Set Prm !!!\n", IspDev);
    if (HI_SUCCESS != (iRet = HI_MPI_VPSS_SetGrpNRXParam(uiVpssroup, (VPSS_GRP_NRX_PARAM_S *)pParam)))
    {
        /* ISP的初始化在 vpss 的初始化之前，容易出现通道未初始化的错误 是该错误，则不打印*/
        if(0xa0078005 != iRet)
        {
//            CAPT_LOGE("HI_MPI_VPSS_SetNRXParam Vpss Group %d Failed, iRet %x !!!\n", uiVpssroup, iRet);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

INT32 Isp_drvGet3dDenoise(int IspDev,void *pParam)
{
    UINT32 uiVpssroup = 0;
    INT32  iRet       = 0;

    if (0 == IspDev)
    {
        uiVpssroup = 0;
    }
    else
    {
        uiVpssroup = 2;
    }
    // CAPT_LOGE("Isp Dev %d Get Prm !!!\n", IspDev);
    if (HI_SUCCESS != (iRet = HI_MPI_VPSS_GetGrpNRXParam(uiVpssroup, (VPSS_GRP_NRX_PARAM_S *)pParam)))
    {
        /* ISP的初始化在 vpss 的初始化之前，容易出现通道未初始化的错误 是该错误，则不打印*/
        if (0xa0078005 != iRet)
        {
            CAPT_LOGE("HI_MPI_VPSS_GetGrpNRXParam Vpss Group %d Failed, iRet %x !!!\n", uiVpssroup, iRet);
            return SAL_FAIL;
        }
    }
    return SAL_SOK;
}

INT32 Isp_drvEnableDenoise(int IspDev,void *pParam)
{
    UINT32 uiVpssroup = 0;
    //INT32  iRet       = 0;
    INT32 uiEnable   = *(UINT32 *)pParam;
    VPSS_GRP_ATTR_S stVpssGrpAttr;

    if (0 == IspDev)
    {
        uiVpssroup = 0;
    }
    else
    {
        uiVpssroup = 2;
    }

    memset(&stVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));

    HI_MPI_VPSS_GetGrpAttr (uiVpssroup, &stVpssGrpAttr);
    stVpssGrpAttr.bNrEn = uiEnable;
    HI_MPI_VPSS_SetGrpAttr (uiVpssroup, &stVpssGrpAttr);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : ISP_Mem_Alloc
 功能描述  : 分配内存
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
void *ISP_Mem_Alloc(int size)
{
    void        *ptr;
    ptr = (void *)memalign((32), (size));

    if(ptr == NULL)
    {
        CAPT_LOGE("memalign error.\n");
    }

    return ptr;
}


/*******************************************************************************
* 函数名  : Isp_drvInitIspFaceAe
* 描  述  : 人脸曝光初始化配置
* 输  入  : - handle:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 Isp_drvInitIspFaceAe(void *handle)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Isp_drvSendFaceYuvLumaToIsp
* 描  述  : 发送人脸亮度给ISP
* 输  入  : - ispChan: ISP通道
*         : - bright : 人脸区域的亮度均值
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 Isp_drvSendFaceYuvLumaToIsp(INT32 ispChan, INT32 bright)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Isp_drvSetIspDefPrm
* 描  述  : 配置 isp 库 默认属性
* 输  入  : - ispChan: 摄像头通道
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 Isp_drvSetIspDefPrm(INT32 ispChan)
{
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : Isp_drvGetIspPubAttr
 功能描述  : 从 ISP 模块中获取 Vi与isp 属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Isp_drvGetIspPubAttr(UINT32 uiSensorChn)
{
   #if 0
    ISP_DRV_INFO *pstIspDrvInfo = &g_stCaptModPrm.stIspDrvInfo;
    SENSOR_VI_S  *pstSensorVi   = &pstIspDrvInfo->stIspChnPrm[uiSensorChn].stSensorInfo;
    VI_ATTR_S    *pstViAttr     = &pstIspDrvInfo->stIspChnPrm[uiSensorChn].stViAttr;

    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    //ISP_PUB_ATTR_S   *pstIspPubAttr    = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiSensorChn];
    pstIspPubAttr    = &pstCaptHalChnPrm->stIspPubAttr;

//    if (ISP_LIB_S_OK != ISP_GetViAttr(SONY_IMX290, (void *)pstSensorVi,  pstViAttr, (void*)pstIspPubAttr))
//    {
//        CAPT_LOGE("ISP_GetViAttr failed\n");
//        return SAL_FAIL;
//    }
    #endif
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Isp_drvGetMipiAttr
 功能描述  : 从 ISP 模块中获取 Mipi 属性，用于初始化 MIPI
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Isp_drvGetMipiAttr(UINT32 uiSensorChn)
{
    #if 0
    ISP_DRV_INFO     *pstIspDrvInfo    = &g_stCaptModPrm.stIspDrvInfo;
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    SENSOR_VI_S      *pstSensorVi      = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiSensorChn];
    pstSensorVi      = &pstIspDrvInfo->stIspChnPrm[uiSensorChn].stSensorInfo;

//    if (ISP_LIB_S_OK !=ISP_GetLVDSAttr(SONY_IMX290, (void *)pstSensorVi, (void*)&pstCaptHalChnPrm->stcomboDevAttr))
//    {
//        CAPT_LOGE("ISP_GetLVDSAttr failed\n");
//        return SAL_FAIL;
//    }
     #endif
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : Isp_drvGetPrm
 功能描述  : 获取 ISP 模块 属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年11月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 Isp_drvGetPrm(UINT32 uiChn, UINT32 uiKey, UINT32 *uiVal)
{
    #if 0
    ISP_DRV_INFO  *pstIspDrvInfo  = &g_stCaptModPrm.stIspDrvInfo;
    ISP_PRM_ST    *pstIspPrm      = NULL;
    void          *ispHandle      = NULL;
    UINT32         uiValue        = 0;
    INT8 iRet = 0;

    if (uiChn >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("!!!\n");
        return SAL_FAIL;
    }

    pstIspPrm = &pstIspDrvInfo->stIspChnPrm[uiChn];
    ispHandle = pstIspPrm->handle;

//    iRet = ISP_GetKeyParam(ispHandle, uiKey, &uiValue);
//    if (ISP_LIB_S_OK != iRet)
//    {
//        CAPT_LOGE("ISP Get Key Prm Failed, %x !!!\n", iRet);
//        return SAL_FAIL;
//    }

    *uiVal = uiValue;

    SAL_DEBUG("CHN %d GET Key 0x%x val 0x%x !!!\n", uiChn, uiKey, *uiVal);
    #endif
    return SAL_SOK;
}

#endif

#define NEW_LINUX_HISI_CONFIG   /*一些写死的配置需要整改*/
#ifdef NEW_LINUX_HISI_CONFIG
/*****************************************************************************
 函 数 名: capt_hisiGetIntfMode
 功能描述  : 根据板子类型获取输入接口类型
 输入参数  : 无
 输出参数  : VI_INTF_MODE_E : 接口类型
 返 回 值: VI_INTF_MODE_E : 接口类型
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2020.05.22
    作     者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static VI_INTF_MODE_E capt_hisiGetIntfMode(VOID)
{
    static VI_INTF_MODE_E enMode = VI_MODE_BUTT;
    CAPB_CAPT *pstCaptCap = NULL;

    if (VI_MODE_BUTT == enMode)
    {
        pstCaptCap = capb_get_capt();
        if (NULL == pstCaptCap)
        {
            CAPT_LOGE("get capt capbility fail\n");
            return VI_MODE_BUTT;
        }

        /* FPGA用来做BT1120转LVDS */
        enMode = (SAL_TRUE == pstCaptCap->bFpgaEnable) ? VI_MODE_LVDS : VI_MODE_BT1120_STANDARD;
    }
    
    return enMode;
}


/*****************************************************************************
 函 数 名: capt_hisiIsUseMipi
 功能描述  : 根据板子类型获取是否使用Mipi
 输入参数  : 无
 输出参数  : BOOL : 
 返 回 值: BOOL : 
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2020.05.22
    作     者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static BOOL capt_hisiIsUseMipi(VOID)
{
    VI_INTF_MODE_E enMode = capt_hisiGetIntfMode();
    static VI_INTF_MODE_E aenUseMipiMode[] = {VI_MODE_LVDS};
    static INT32 s32UseMipi = -1;
    UINT32 u32Num = sizeof(aenUseMipiMode)/sizeof(aenUseMipiMode[0]);
    UINT32 i = 0;

    if (s32UseMipi < 0)
    {
        for (i = 0; i < u32Num; i++)
        {
            if (enMode == aenUseMipiMode[i])
            {
                s32UseMipi = 1;
                break;
            }
        }

        if (i >= u32Num)
        {
            s32UseMipi = 0;
        }
    }

    return (1 == s32UseMipi) ? SAL_TRUE : SAL_FALSE;
}

/*****************************************************************************
 函 数 名: capt_hisiGetMipiLaneId
 功能描述  : 根据采集通道获取Mipi lan
 输入参数  : UINT32 u32ChnId : 采集通道号
 输出参数  :   combo_dev_t *pDevId : DEV ID
 返 回 值: 
 调用函数  :
 被调函数  :

 修改历史:
  1.日     期:2021.06.22
    作     者:
    修改内容    :新生成函数
*****************************************************************************/
static INT32 capt_hisiGetMipiLaneId(UINT32 u32ChnId, combo_dev_t *pDevId, short *pLane)
{
    static const UINT32 au32MipiDevMap7[] = {0, 2, 4, 6};           // 仅限于MODE7
    CAPB_CAPT *pstCaptCap = capb_get_capt();
    UINT32 u32Mipi = 0;
    UINT32 i = 0;

    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }

    u32Mipi = pstCaptCap->auiHwMipi[u32ChnId];
    *pDevId = au32MipiDevMap7[u32Mipi];
    if (NULL != pLane)
    {
        for (; i < 4; i++)
        {
            *pLane++ = u32Mipi * 4 + i;
        }
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名: capt_hisiGetDevId
 功能描述  : 根据采集通道获取DEV
 输入参数  : UINT32 u32ChnId : 采集通道号
 输出参数  :   VI_DEV *pDevId : DEV ID
 返 回 值: 
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2020.05.22
    作     者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiGetDevId(UINT32 u32ChnId, VI_DEV *pDevId)
{
    //static UINT32 au32CaptDevMap[CAPT_CHN_NUM_MAX] = {0, 6};   /*R7新主板接口换成6，如果R6主板改成2*/
    static UINT32 au32BtCaptDevMap[CAPT_CHN_NUM_MAX] = {5, 7};       // BT1120,BT656,BT601与DEV绑定关系请参考HISI数据手册
    VI_INTF_MODE_E enIntfMode = capt_hisiGetIntfMode();
    combo_dev_t stComboDev = 0;
    
    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }
    
    if (VI_MODE_BT1120_STANDARD == enIntfMode)
    {
        *pDevId = au32BtCaptDevMap[u32ChnId];
    }
    else
    {
        capt_hisiGetMipiLaneId(u32ChnId, &stComboDev, NULL);
        *pDevId = stComboDev;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名: capt_hisiGetPipeId
 功能描述  : 根据采集通道获取Pipe Id
 输入参数  : UINT32 u32ChnId : 采集通道号
 输出参数  :   VI_PIPE *pPipeId : PIPE ID
 返 回 值: 
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2020.05.22
    作     者   : 
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiGetPipeId(UINT32 u32ChnId, VI_PIPE *pPipeId)
{
    static UINT32 au32PipeMap[CAPT_CHN_NUM_MAX] = {0, 4};

    if (u32ChnId >= CAPT_CHN_NUM_MAX)
    {
        CAPT_LOGE("capt chn[%u] id error\n", u32ChnId);
        return SAL_FAIL;
    }

    *pPipeId = au32PipeMap[u32ChnId];

    return SAL_SOK;
}
#endif
/*****************************************************************************
 函 数 名  : capt_hisiGetPoolBlock
 功能描述  : 获取内存池中的内存块
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiGetPoolBlock(UINT32 imgW,UINT32 imgH, VIDEO_FRAME_INFO_S *pOutFrm)
{
    VB_POOL pool        = VB_INVALID_POOLID;
    UINT64  u64BlkSize  = (UINT64)((UINT64)imgW * (UINT64)imgH * 3 / 2);
    VB_BLK  VbBlk       = 0;
    UINT64  u64PhyAddr  = 0;
    UINT64  *p64VirAddr    = NULL;
    UINT32  u32LumaSize = 0;
    UINT32  u32ChrmSize = 0;
    UINT64  u64Size     = 0;
    VB_POOL_CONFIG_S stVbPoolCfg;

    if(VB_INVALID_POOLID == pool)
    {
        SAL_clear(&stVbPoolCfg);
        stVbPoolCfg.u64BlkSize  = u64BlkSize;
        stVbPoolCfg.u32BlkCnt   = 1;
        stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
        pool = HI_MPI_VB_CreatePool(&stVbPoolCfg);

        if(VB_INVALID_POOLID == pool)
        {
            CAPT_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64BlkSize);
            return SAL_FAIL;
        }
    }

    u32LumaSize = (imgW * imgH);
    u32ChrmSize = (imgW * imgH / 4);
    u64Size     = (UINT64)(u32LumaSize + (u32ChrmSize << 1));

    VbBlk = HI_MPI_VB_GetBlock(pool, u64Size, NULL);
    if(VB_INVALID_POOLID == pool)
    {
        CAPT_LOGE("HI_MPI_VB_CreatePool failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    u64PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if(0 == u64PhyAddr)
    {
        CAPT_LOGE("HI_MPI_VB_Handle2PhysAddr failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    p64VirAddr = (UINT64 *) HI_MPI_SYS_Mmap(u64PhyAddr, u64Size);
    if(NULL == p64VirAddr)
    {
        CAPT_LOGE("HI_MPI_SYS_Mmap failed size %llu!\n", u64Size);
        return SAL_FAIL;
    }

    pOutFrm->enModId            = HI_ID_VB;
    pOutFrm->u32PoolId          = pool;
    pOutFrm->stVFrame.u32Width  = imgW;
    pOutFrm->stVFrame.u32Height = imgH;
    pOutFrm->stVFrame.enField   = VIDEO_FIELD_FRAME;

    pOutFrm->stVFrame.enPixelFormat   = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pOutFrm->stVFrame.u64PhyAddr[0]   = u64PhyAddr;
    pOutFrm->stVFrame.u64PhyAddr[1]   = pOutFrm->stVFrame.u64PhyAddr[0] + u32LumaSize;
    pOutFrm->stVFrame.u64PhyAddr[2]   = pOutFrm->stVFrame.u64PhyAddr[1] + u32ChrmSize;
    pOutFrm->stVFrame.u64VirAddr[0]   = HI_ADDR_P2U64(p64VirAddr);
    pOutFrm->stVFrame.u64VirAddr[1]   = (HI_U64)((UINT64)pOutFrm->stVFrame.u64VirAddr[0] + u32LumaSize);
    pOutFrm->stVFrame.u64VirAddr[2]   = (HI_U64)((UINT64)pOutFrm->stVFrame.u64VirAddr[1] + u32ChrmSize);
    pOutFrm->stVFrame.u32Stride[0]    = pOutFrm->stVFrame.u32Width;
    pOutFrm->stVFrame.u32Stride[1]    = pOutFrm->stVFrame.u32Width;
    pOutFrm->stVFrame.u32Stride[2]    = pOutFrm->stVFrame.u32Width;
    pOutFrm->stVFrame.u64PrivateData  = VbBlk;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名 : capt_hal_drvIsOffset
 功能描述 : 计算图像是否有偏移
 输入参数   :   uiChn 输入通道
 输出参数   : 无
 返 回 值 : 无
*****************************************************************************/
static BOOL capt_hisiCaptIsOffset(UINT32 uiDev)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VIDEO_FRAME_INFO_S stFrame;
    VI_PIPE viPipe    = 0;
    UINT64 u64PhyAddr = 0;
    HI_VOID *pvAddr   = NULL;
    UINT8 *pu8Tmp     = NULL;
    UINT32 u32Stride  = 0;
    BOOL bLeft = SAL_TRUE, bRight = SAL_TRUE;
    BOOL bUp = SAL_TRUE, bDown = SAL_TRUE;
    UINT32 i = 0, j = 0;

    if (SAL_SOK != capt_hisiGetPipeId(uiDev, &viPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FALSE;
    }

    s32Ret = HI_MPI_VI_GetChnFrame(viPipe, 0, &stFrame, 200);
    if (HI_SUCCESS != s32Ret)
    {
        CAPT_LOGE("capt chn[%u] get frame fail\n", uiDev);
        return SAL_FALSE;
    }

    u64PhyAddr = stFrame.stVFrame.u64PhyAddr[0];
    u32Stride  = stFrame.stVFrame.u32Stride[0];

    pvAddr = HI_MPI_SYS_Mmap(u64PhyAddr, u32Stride * stFrame.stVFrame.u32Height * 3 / 2);
    if (NULL == pvAddr)
    {
        CAPT_LOGE("capt chn[%u] map addr[0x%llx] fail\n", uiDev, u64PhyAddr);
        if (HI_SUCCESS != (s32Ret = HI_MPI_VI_ReleaseChnFrame(viPipe, 0, &stFrame)))
        {
            CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
        }
        return SAL_FALSE;
    }

    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.stVFrame.u32Height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        /* 最左边的一列不校验 */
        for (j = 1; j < VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 左边没有黑边 */
                bLeft = SAL_FALSE;
                goto right;
            }
        }
    }

right:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.stVFrame.u32Width - VGA_OFFSET_CHECK_PIXEL);
    pu8Tmp += VGA_OFFSET_CHECK_PIXEL * u32Stride;
    for (i = VGA_OFFSET_CHECK_PIXEL; i < stFrame.stVFrame.u32Height - VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        /* 最右边的一列不校验 */
        for (j = 0; j < VGA_OFFSET_CHECK_PIXEL - 1; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 右边没有黑边 */
                bRight = SAL_FALSE;
                goto up;
            }
        }
    }

up:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += u32Stride;
    /* 最上面的一行不校验 */
    for (i = 1; i < VGA_OFFSET_CHECK_PIXEL; i++)
    {
        pu8Tmp += u32Stride;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.stVFrame.u32Width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 上面没有黑边 */
                bUp = SAL_FALSE;
                goto down;
            }
        }
    }

down:
    pu8Tmp = (UINT8 *)pvAddr;
    pu8Tmp += (stFrame.stVFrame.u32Height - VGA_OFFSET_CHECK_PIXEL) * u32Stride;
    /* 最下面的一行不校验 */
    for (i = 0; i < VGA_OFFSET_CHECK_PIXEL - 1; i++)
    {
        pu8Tmp += u32Stride;
        for (j = VGA_OFFSET_CHECK_PIXEL; j < stFrame.stVFrame.u32Width - VGA_OFFSET_CHECK_PIXEL; j++)
        {
            if (pu8Tmp[j] > 0x10)
            {
                /* 下面没有黑边 */
                bDown = SAL_FALSE;
                goto end;
            }
        }
    }

end:
    if (HI_SUCCESS != (s32Ret = HI_MPI_SYS_Munmap(pvAddr, u32Stride * stFrame.stVFrame.u32Height * 3 / 2)))
    {
        CAPT_LOGW("ummap viraddr[%p] size[%u] fail:0x%X\n", pvAddr, u32Stride * stFrame.stVFrame.u32Height * 3 / 2, s32Ret);
    }
    
    if (HI_SUCCESS != (s32Ret = HI_MPI_VI_ReleaseChnFrame(viPipe, 0, &stFrame)))
    {
        CAPT_LOGW("release vi pipe[%d] frame fail:0x%X\n", viPipe, s32Ret);
    }

    /* 上下都有黑边或左右都有黑边时无需调整 */
    return ((bLeft ^ bRight) || (bUp ^ bDown));
}

/*****************************************************************************
 函 数 名  : capt_hisiGetUserPicStatue
 功能描述  : 采集模块使用用户图片，用于无信号时编码
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static UINT32 capt_hisiGetUserPicStatue(UINT32 uiDev)
{ 
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("uiChn %d error\n", uiDev);
        return SAL_FAIL;
    }
    
    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (pstCaptHalChnPrm->userPic)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


/*****************************************************************************
 函 数 名  : capt_hisiSetCaptUserPic
 功能描述  : 采集模块配置用户图片，用于无信号时编码
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiSetCaptUserPic(UINT32 uiDev, CAPT_CHN_USER_PIC *pstUserPic)
{
    VI_USERPIC_ATTR_S  *pstUserPicAttr = &gstUserPicAttr[uiDev];
    VI_PIPE_ATTR_S      stPipeAttr;
    //VI_CHN_ATTR_S       stChnAttr;
    VIDEO_FRAME_INFO_S *pstVideoFrameInfo = NULL;
    VIDEO_FRAME_S      *pVFrame           = NULL;
    INT32   iRet   = 0;
    VI_PIPE ViPipe = 0;
    UINT32  uiW    = 0;
    UINT32  uiH    = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiGetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }

    ///memset(pstUserPicAttr, 0, sizeof(VI_USERPIC_ATTR_S));
    pstVideoFrameInfo = &pstUserPicAttr->unUsrPic.stUsrPicFrm;

    pVFrame = &pstVideoFrameInfo->stVFrame;

    if ((NULL == pstUserPic->pucAddr) || (0 == pstUserPic->uiW) || (0 == pstUserPic->uiH))
    {
        uiW = 1920;
        uiH = 1080;
        /* 上层无图片，底层自己填纯色 */
        pstUserPicAttr->enUsrPicMode  = VI_USERPIC_MODE_BGC;
        pstUserPicAttr->unUsrPic.stUsrPicBg.u32BgColor = 0x80;
        CAPT_LOGI("uiChn %d,use bgc\n",uiDev);
    }
    else
    {
        uiW = pstUserPic->uiW;
        uiH = pstUserPic->uiH;
        pstUserPicAttr->enUsrPicMode  = VI_USERPIC_MODE_PIC;
        CAPT_LOGI("uiChn %d,use pic(wxh %d,%d)\n",uiDev,uiW,uiH);
    }

    /* 图片宽度要16对齐，图片缩放要用到VGS，需要16对齐 否则配置成功，但无图片显示 */
    if (0x00 == pstVideoFrameInfo->stVFrame.u64VirAddr[0])
    {
        uiW = (uiW/16)*16;
        if (SAL_SOK != (iRet = capt_hisiGetPoolBlock(uiW, uiH, pstVideoFrameInfo)))
        {
            CAPT_LOGE("Capt Hal Set User Pic Get Pool Block Failed, %d !!!\n", iRet);

            return SAL_FAIL;
        }
        CAPT_LOGI("uiChn %u,get Pool info id %u,modeId %u, addr %llu\n",uiDev,pstVideoFrameInfo->u32PoolId,(UINT32)pstVideoFrameInfo->enModId,pstVideoFrameInfo->stVFrame.u64VirAddr[0]);
    }
    else
    {
        CAPT_LOGI("uiChn %du,pic Pool info id %u,modeId %u, addr %llu\n",uiDev,pstVideoFrameInfo->u32PoolId,(UINT32)pstVideoFrameInfo->enModId,pstVideoFrameInfo->stVFrame.u64VirAddr[0]);
    }

    /* 纯色 */
    if (VI_USERPIC_MODE_BGC == pstUserPicAttr->enUsrPicMode)
    {
        memset(HI_ADDR_U642P(pVFrame->u64VirAddr[0]), 0x10, uiW * uiH);
        memset(HI_ADDR_U642P(pVFrame->u64VirAddr[1]), 0x80, uiW * uiH / 2);
    }
    else if (VI_USERPIC_MODE_PIC == pstUserPicAttr->enUsrPicMode) /* 图片 */
    {
        memcpy(HI_ADDR_U642P(pVFrame->u64VirAddr[0]), pstUserPic->pucAddr, uiW * uiH);
        memcpy(HI_ADDR_U642P(pVFrame->u64VirAddr[1]), pstUserPic->pucAddr + uiW * uiH, uiW * uiH / 2);
    }
    else
    {
        CAPT_LOGE("ERROR !!!\n");
        return SAL_FAIL;
    }

    if (HI_SUCCESS != (iRet = HI_MPI_VI_SetUserPic(ViPipe, pstUserPicAttr)))
    {
        CAPT_LOGE("Capt Hal Set User Pic Failed, %x (%d)!!!\n", iRet
            ,pstUserPicAttr->enUsrPicMode);

        return SAL_FAIL;
    }

#if 1

    SAL_clear(&stPipeAttr);
    iRet = HI_MPI_VI_GetPipeAttr(ViPipe,&stPipeAttr);
    if (iRet != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", iRet);
    }

    stPipeAttr.stFrameRate.s32DstFrameRate = 60;
    stPipeAttr.stFrameRate.s32SrcFrameRate = 60;
    iRet = HI_MPI_VI_SetPipeAttr(ViPipe,&stPipeAttr);
    if (iRet != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", iRet);
    }
#endif
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiEnableCaptUserPic
 功能描述  : 使能采集模块使用用户图片，用于无信号时编码
 输入参数  : -uiChn 输入设备号
         : -uiEnble 是否使能
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月13日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiEnableCaptUserPic(UINT32 uiDev, UINT32 uiEnble)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    INT32 s32Ret = HI_SUCCESS;
    VI_PIPE ViPipe = 0;
    VI_PIPE_ATTR_S      stPipeAttr;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_hisiGetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }



#if 0
    /* 停止中断 */
    s32Ret = HI_MPI_VI_DisablePipeInterrupt(uiVidev);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
#endif
    if(!uiEnble)
    {
        /* 停止使能用户图片 */
        s32Ret = HI_MPI_VI_DisableUserPic(ViPipe);
        if (s32Ret != HI_SUCCESS)
        {
            CAPT_LOGE("failed with %#x!\n", s32Ret);
            return SAL_FAIL;
        }

        pstCaptHalChnPrm->userPic = 0;
    }
    else
    {
        /* 使能用户图片 */
        s32Ret = HI_MPI_VI_EnableUserPic(ViPipe);
        if (s32Ret != HI_SUCCESS)
        {
            CAPT_LOGE("failed with %#x!\n", s32Ret);
            return SAL_FAIL;
        }

        pstCaptHalChnPrm->userPic = 1;

#if 0
        VI_DEV ViDev = 0;
        VI_DEV_TIMING_ATTR_S stTimingAttr = {0};

        if (SAL_SOK != capt_hisiGetDevId(uiChn, &ViDev))
        {
            CAPT_LOGE("capt chn[%u] get dev id fail\n", uiChn);
            return SAL_FAIL;
        }
        
        s32Ret = HI_MPI_VI_GetDevTimingAttr(ViDev, &stTimingAttr);
        if (s32Ret != HI_SUCCESS)
        {
            CAPT_LOGE("ViDev %d failed with %#x!\n",ViDev, s32Ret);
        }
        
        stTimingAttr.bEnable = 0;
        stTimingAttr.s32FrmRate = 60;
        s32Ret = HI_MPI_VI_SetDevTimingAttr(ViDev, &stTimingAttr);
        if (s32Ret != HI_SUCCESS)
        {
           CAPT_LOGE("ViDev %d,failed with %#x!\n",ViDev, s32Ret);
        }
#endif

        SAL_clear(&stPipeAttr);
        s32Ret = HI_MPI_VI_GetPipeAttr(ViPipe,&stPipeAttr);
        if (s32Ret != HI_SUCCESS)
        {
           CAPT_LOGE("failed with %#x!\n", s32Ret);
        }

        stPipeAttr.stFrameRate.s32DstFrameRate = 60;
        stPipeAttr.stFrameRate.s32SrcFrameRate = 60;
        s32Ret = HI_MPI_VI_SetPipeAttr(ViPipe,&stPipeAttr);
        if (s32Ret != HI_SUCCESS)
        {
           CAPT_LOGE("failed with %#x!\n", s32Ret);
        }
    }
    //Fpga_GetAllRegVol(uiChn);
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiIspDeInit
 功能描述  : 去初始化 ISP
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiIspDeInit(CAPT_HAL_CHN_PRM * pstPrm)
{
    UINT32 uiIspDev = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspDev = pstPrm->uiIspId;

    HI_MPI_ISP_Exit(uiIspDev);
    
    SAL_thrJoin(&pstPrm->IspPid);

    pstPrm->uiIspInited = 0;

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiUnRegisterIspLib
 功能描述  : 去注册 ISP 库
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年09月11日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiUnRegisterIspLib(UINT32 uiIspId)
{
    INT32 s32Ret = 0;

    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    ALG_LIB_S stAfLib;

    ISP_SNS_OBJ_S *pstSnsObj = &g_stCaptModPrm.stCaptHalChnPrm[uiIspId].stSnsObj;

    memset(&stAeLib, 0, sizeof(ALG_LIB_S));
    memset(&stAwbLib, 0, sizeof(ALG_LIB_S));
    memset(&stAfLib, 0, sizeof(ALG_LIB_S));

    stAeLib.s32Id  = uiIspId;
    stAwbLib.s32Id = uiIspId;
    stAfLib.s32Id  = uiIspId;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    strncpy(stAfLib.acLibName, HI_AF_LIB_NAME, sizeof(HI_AF_LIB_NAME));

    if (pstSnsObj->pfnUnRegisterCallback != HI_NULL)
    {
        s32Ret = pstSnsObj->pfnUnRegisterCallback(uiIspId, &stAeLib, &stAwbLib);
        if (s32Ret != HI_SUCCESS)
        {
            CAPT_LOGE("sensor_register_callback failed with %#x!\n",s32Ret);
            return SAL_FAIL;
        }
    }

    if (HI_SUCCESS != HI_MPI_AWB_UnRegister(uiIspId, &stAwbLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AWB UnRegister failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

/*
    if (HI_SUCCESS != HI_MPI_AF_UnRegister(uiIspId, &stAfLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AF UnRegister failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }
*/

    if (HI_SUCCESS != HI_MPI_AE_UnRegister(uiIspId, &stAeLib))
    {
        CAPT_LOGE("Func: %s, Line: %d, AE UnRegister failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   capt_hisiSetChnRotate
 * @brief      设置采集旋转属性
 * @param[in]  UINT32 uiDev            设备号
 * @param[in]  UINT32 uiChn            通道号
 * @param[in]  CAPT_ROTATION rotation  旋转参数
 * @param[out] None
 * @return     static INT32 SAL_SOK
 *                          SAL_FAIL
 */
static INT32 capt_hisiSetChnRotate(UINT32 uiDev, UINT32 uiChn ,CAPT_ROTATION rotation)
{
    INT32  s32Ret  = 0;
    VI_PIPE ViPipe = 0;
    ROTATION_E enRotate;

    switch(rotation)
    {
        case 0:
        {
            enRotate = ROTATION_0;
           
            break;
        }
        case 1:
        {
            enRotate = ROTATION_270;
            
            break;
        }
        case 2:
        {
            enRotate = ROTATION_180;
           ;
            break;
        }
        case 3:
        {
            enRotate = ROTATION_90;
            break;
        }
        default:
        {
            enRotate = ROTATION_0;
            break;
        }
    }

    capt_hisiGetPipeId(uiDev,&ViPipe);
    
    /*设置VI的旋转角度*/
    s32Ret = HI_MPI_VI_SetChnRotation(ViPipe,uiChn, enRotate);
    if (HI_SUCCESS != s32Ret)
    {
        CAPT_LOGE("HI_MPI_VI_SetRotate failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiGetDevStatus
 功能描述  : 查询采集通道状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiGetDevStatus(UINT32 uiChn, CAPT_HAL_CHN_STATUS *pstStatus)
{
    HI_S32             s32Ret = HI_SUCCESS;
    VI_PIPE_STATUS_S   viStat;
    VI_PIPE            ViPipe = 0;
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    memset(&viStat, 0, sizeof(VI_PIPE_STATUS_S));

    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    if (0 == pstCaptHalChnPrm->uiStarted)
    {
        return SAL_FAIL;
    }

    capt_hisiGetPipeId(pstCaptHalChnPrm->uiChn,&ViPipe);
    
    s32Ret = HI_MPI_VI_QueryPipeStatus(ViPipe, &viStat);
    if (HI_SUCCESS != s32Ret)
    {
        CAPT_LOGE("HI_MPI_VI_Query err s32Ret=%#x\n",s32Ret);
        return SAL_FAIL;
    }

    pstStatus->uiIntCnt  = viStat.u32IntCnt;
    pstStatus->FrameLoss = viStat.u32LostFrame;
    //待海思解决不同pipe可以支持不同图片后再打开
    pstStatus->captStatus= 0;  /*0:视频正常,默认视频正常*/
 
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisistopCaptDev
 功能描述  : Camera采集通道停止
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisistopCaptDev(UINT32 uiDev)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_PIPE ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_hisiGetDevId(uiDev, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiDev);
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiGetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", uiDev);
        return SAL_FAIL;
    }
    
    if (0 == pstCaptHalChnPrm->uiStarted)
    {
        CAPT_LOGW("is stoped\n");
        return SAL_SOK;
    }

    pstCaptHalChnPrm->uiStarted = 0;
    if (HI_SUCCESS != (s32Ret = HI_MPI_VI_GetDevAttr(Videv, &pstCaptHalChnPrm->stDevAttr)))
    {
        CAPT_LOGE("Capt Start Get Dev %d Attr failed with %#x!\n", Videv, s32Ret);
        return SAL_FAIL;
    }
    if (HI_SUCCESS != (s32Ret = HI_MPI_VI_GetChnAttr(ViPipe,0, &pstCaptHalChnPrm->stChnAttr)))
    {
        CAPT_LOGE("Capt Start Get Chn %d Attr failed with %#x!\n", ViPipe, s32Ret);
        return SAL_FAIL;
    }

#if 0
    s32Ret = HI_MPI_VI_DisableChnInterrupt(uiVidev);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
#endif

    s32Ret = HI_MPI_VI_DisableChn(ViPipe,0);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }
    
    s32Ret = HI_MPI_VI_StopPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_StopPipe failed with %#x!\n", s32Ret);
    }
    
    s32Ret = HI_MPI_VI_DestroyPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_DestroyPipe failed with %#x!\n", s32Ret);
    }
#if 1
    if (HI_SUCCESS != (s32Ret = HI_MPI_VI_DisableDev(Videv)))
    {
        CAPT_LOGE("HI_MPI_VI_EnableDev chn %d failed with %#x!\n", Videv, s32Ret);
        return SAL_FAIL;
    }
    pstCaptHalChnPrm->viDevEn = 0;
#endif

    //pstCaptHalChnPrm->uiStarted = 0;

    CAPT_LOGI("Capt Stop Chn %d !!!\n", uiDev);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiStartCaptDev
 功能描述  : Camera采集通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hisiStartCaptDev(UINT32 uiDev)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    INT32  s32Ret  = 0;
    //UINT32 uiVidev = 0;
    VI_PIPE ViPipe = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if (SAL_SOK != capt_hisiGetPipeId(uiDev, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiDev);
        return SAL_FAIL;
    }
    
    if (1 == pstCaptHalChnPrm->uiStarted)
    {
        CAPT_LOGW("is started\n");
        return SAL_SOK;
    }

    s32Ret = HI_MPI_VI_StartPipe(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        HI_MPI_VI_DestroyPipe(ViPipe);
        CAPT_LOGE("HI_MPI_VI_StartPipe failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VI_EnableChn(ViPipe,0);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VI_EnablePipeInterrupt(ViPipe);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    pstCaptHalChnPrm->uiStarted = 1;

    CAPT_LOGI("Capt Start Chn %d:%d !!!\n", uiDev , ViPipe);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiSetSensorAttr
 功能描述  : sensor 设置sensor参数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 capt_hisiSetSensorAttr(combo_dev_attr_t * pstcomboDevAttr, UINT32 uiReset)
{
    return SAL_SOK;
}

#ifdef CAPT_DEBUG  /*如下接口暂时不用需要时注开*/

/**
 * @function   capt_hisiSetWDRPrm
 * @brief      采集模块 配置 VI 设备宽动态属性
 * @param[in]  UINT32 uiChn     设备号
 * @param[in]  UINT32 uiEnable  是否使能
 * @param[out] None
 * @return     INT32 SAL_SOK  : 成功
 *                   SAL_FAIL : 失败
 */
static INT32 capt_hisiSetWDRPrm(UINT32 uiChn, UINT32 uiEnable)
{
    INT32 s32Ret  = HI_SUCCESS;
    VI_DEV Videv = 0;
    VI_DEV_ATTR_S  stViDevAttr;

    if (CAPT_CHN_NUM_MAX <= uiChn)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiGetDevId(uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", uiChn);
        return SAL_FAIL;
    }

    memset(&stViDevAttr, 0, sizeof(stViDevAttr));

     s32Ret = HI_MPI_VI_GetDevAttr(Videv, &stViDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
          CAPT_LOGE("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
          return SAL_FAIL;
    }

    if (TRUE == uiEnable)
    {
        stViDevAttr.stWDRAttr.u32CacheLine = 2160;
        stViDevAttr.stWDRAttr.enWDRMode = WDR_MODE_2To1_LINE;// WDR_MODE_2To1_LINE;

    }
    else
    {
        stViDevAttr.stWDRAttr.enWDRMode = WDR_MODE_NONE;
    }

    CAPT_LOGI("Set WDR Attr uiVidev %d \n", Videv);
    s32Ret = HI_MPI_VI_SetDevAttr(Videv, &stViDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_hisiMipiAttrPrint
* 描  述  : 调试 MIPI 接口时的调试属性打印
* 输  入  : - pstcomboDevAttr:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
static INT32 capt_hisiMipiAttrPrint(combo_dev_attr_t * pstcomboDevAttr)
{
    if (NULL == pstcomboDevAttr)
    {
        return SAL_FAIL;
    }

    // pstcomboDevAttr->mipi_attr.wdr_mode = HI_MIPI_WDR_MODE_DT;
//    pstcomboDevAttr->mipi_attr.lane_id[0] = 0;
//    pstcomboDevAttr->mipi_attr.lane_id[1] = 1;
//    pstcomboDevAttr->mipi_attr.lane_id[2] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[3] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[4] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[5] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[6] = -1;
//    pstcomboDevAttr->mipi_attr.lane_id[7] = -1;

//    pstcomboDevAttr->img_rect.x      = 0;
//    pstcomboDevAttr->img_rect.y      = 0;
//    pstcomboDevAttr->img_rect.width  = 1920;
//    pstcomboDevAttr->img_rect.height = 1080;

    CAPT_LOGI("devno         : %d !!!\n", pstcomboDevAttr->devno);
    CAPT_LOGI("input_mode    : %d !!!\n", pstcomboDevAttr->input_mode);
    //CAPT_LOGI("phy_clk_share : %d !!!\n", pstcomboDevAttr->phy_clk_share);
    CAPT_LOGI("img_rect      : x %d y %d w %d h %d !!!\n",
        pstcomboDevAttr->img_rect.x, pstcomboDevAttr->img_rect.y, pstcomboDevAttr->img_rect.width, pstcomboDevAttr->img_rect.height);
    CAPT_LOGI("raw_data_type : %d !!!\n", pstcomboDevAttr->mipi_attr.input_data_type);
    CAPT_LOGI("wdr_mode      : %d !!!\n", pstcomboDevAttr->mipi_attr.wdr_mode);
    
    CAPT_LOGI("lane_id       : %d %d %d %d %d %d %d %d !!!\n", 
        pstcomboDevAttr->mipi_attr.lane_id[0], 
        pstcomboDevAttr->mipi_attr.lane_id[1],
        pstcomboDevAttr->mipi_attr.lane_id[2], 
        pstcomboDevAttr->mipi_attr.lane_id[3],
        pstcomboDevAttr->mipi_attr.lane_id[4],
        pstcomboDevAttr->mipi_attr.lane_id[5], 
        pstcomboDevAttr->mipi_attr.lane_id[6],
        pstcomboDevAttr->mipi_attr.lane_id[7]);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiDisableMipi
 功能描述  : 关闭mipi_rx控制器
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static void capt_hisiDisableMipi(void)
{
    INT32 fd  = 0;
    INT32 ret  = 0;
    int i=0;
    combo_dev_t devno = 0;
    fd = open("/dev/hi_mipi", O_RDWR);
    if (fd < 0)
    {
        CAPT_LOGW("fd %d errno %s\n",fd,strerror(errno));
        return;
    }
    for (i = 0; i < 3; i++)
    {
        devno = 0;
        if (i==1)
        {
            devno = 2;
        }
        else if (i==2)
        {
            devno = 4;
        }

        ret = ioctl(fd, HI_MIPI_RESET_MIPI, &devno);
        if (HI_SUCCESS != ret)
        {
            CAPT_LOGE("RESET_MIPI %d failed\n", ret);
        }

        ret = ioctl(fd, HI_MIPI_CLEAR, &devno);
        if (HI_SUCCESS != ret)
        {
            CAPT_LOGE("HI_MIPI_CLEAR %d failed\n", ret);
        }

        ret = ioctl(fd, HI_MIPI_DISABLE_MIPI_CLOCK, &devno);
        if (HI_SUCCESS != ret)
        {
            CAPT_LOGE("HI_MIPI_DISABLE_MIPI_CLOCK %d failed\n", ret);
        }
    }
    close(fd);
}


#endif

/*****************************************************************************
 函 数 名  : capt_hisiSetMipiAttr
 功能描述  : 配置 MIPI 属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiSetMipiAttr(CAPT_HAL_CHN_PRM * pstPrm,input_mode_t  enInputMode)
{
    INT32 fd  = 0;
    INT32 ret = 0;
#ifdef HI3519A_SPC020
    lane_divide_mode_t  enHsMode    = LANE_DIVIDE_MODE_0; //
#else
    lane_divide_mode_t  enHsMode    = LANE_DIVIDE_MODE_7; //
#endif
    ///input_mode_t        enInputMode = INPUT_MODE_LVDS;    //
    sns_clk_source_t    SnsDev      = 0;
    combo_dev_attr_t    *pstcomboDevAttr = &pstPrm->stcomboDevAttr; 

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    CAPT_LOGI("Set MIPI_%d Attr Start !!!\n", pstcomboDevAttr->devno);

    /* 属性打印 */
#ifdef CAPT_DEBUG
     capt_hisiMipiAttrPrint(pstcomboDevAttr);
#endif
    /* 打开 MIPI 节点 */
    fd = open("/dev/hi_mipi", O_RDWR);
    if (fd < 0)
    {
        CAPT_LOGE("warning: open hi_mipi dev failed\n");
        return SAL_FAIL;
    }


    ret = ioctl(fd, HI_MIPI_SET_HS_MODE, &enHsMode);
    if (HI_SUCCESS != ret)
    {
        CAPT_LOGE("HI_MIPI_SET_HS_MODE failed(0x%x),\n",ret);
    }
    
    if (INPUT_MODE_SLVS == enInputMode)
    {
        ret = ioctl(fd, HI_MIPI_ENABLE_SLVS_CLOCK, &pstcomboDevAttr->devno);
    }
    else
    {
        ret = ioctl(fd, HI_MIPI_ENABLE_MIPI_CLOCK, &pstcomboDevAttr->devno);
    }

    if (HI_SUCCESS != ret)
    {
        CAPT_LOGE("MIPI_ENABLE_CLOCK %d failed(0x%x)\n", pstcomboDevAttr->devno,ret);
        close(fd);
        return SAL_FAIL;
    }
    if (INPUT_MODE_SLVS == enInputMode)
    {
        ret = ioctl(fd, HI_MIPI_RESET_SLVS, &pstcomboDevAttr->devno);
    }
    else
    {
        ret = ioctl(fd, HI_MIPI_RESET_MIPI, &pstcomboDevAttr->devno);
    }

    if (HI_SUCCESS != ret)
    {
        CAPT_LOGE("RESET_MIPI %d failed\n", pstcomboDevAttr->devno);
        close(fd);
        return SAL_FAIL;
    }

    /* 2. 重置 sensor 使用拉GPIO的方式 而不使用SOC原生的复位管脚 */
    for (SnsDev = 0; SnsDev < SNS_MAX_CLK_SOURCE_NUM; SnsDev++)
    {
        ret = ioctl(fd, HI_MIPI_ENABLE_SENSOR_CLOCK, &SnsDev);

        if (HI_SUCCESS != ret)
        {
            CAPT_LOGE("HI_MIPI_ENABLE_SENSOR_CLOCK failed\n");
            close(fd);
            return SAL_FAIL;
        }
    }

    // pstcomboDevAttr->mipi_attr.wdr_mode = HI_MIPI_WDR_MODE_DT;
    for (SnsDev = 0; SnsDev < SNS_MAX_RST_SOURCE_NUM; SnsDev++)
   {
       ret = ioctl(fd, HI_MIPI_RESET_SENSOR, &SnsDev);

       if (HI_SUCCESS != ret)
       {
           CAPT_LOGE("HI_MIPI_RESET_SENSOR failed\n");
           close(fd);
           return SAL_FAIL;
       }
   }

    /* 4. 撤销复位 MIPI */
    ret = ioctl(fd, HI_MIPI_SET_DEV_ATTR, pstcomboDevAttr);
    if (HI_SUCCESS != ret)
    {
        CAPT_LOGE("MIPI_SET_DEV_ATTR failed.(0x%x)\n",ret);
        close(fd);
        return SAL_FAIL;
    }

    /*  .................... */
    if (INPUT_MODE_SLVS == enInputMode)
    {
        ret = ioctl(fd, HI_MIPI_UNRESET_SLVS, &pstcomboDevAttr->devno);
    }
    else
    {
        ret = ioctl(fd, HI_MIPI_UNRESET_MIPI, &pstcomboDevAttr->devno);
    }

    if (HI_SUCCESS != ret)
    {
        CAPT_LOGE("UNRESET_MIPI %d failed\n", pstcomboDevAttr->devno);
        close(fd);
        return SAL_FAIL;
    }
    
    for (SnsDev = 0; SnsDev < SNS_MAX_RST_SOURCE_NUM; SnsDev++)
    {
        ret = ioctl(fd, HI_MIPI_UNRESET_SENSOR, &SnsDev);

        if (HI_SUCCESS != ret)
        {
            CAPT_LOGE("HI_MIPI_UNRESET_SENSOR failed\n");
            close(fd);
            return SAL_FAIL;
        }
    }
    CAPT_LOGI("Set MIPI fd is 0x%x,close\n", fd);
    close(fd);
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiInterface
 功能描述  : 采集模块初始化Mipi接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiInterface(UINT32 uiChn)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;
    VI_INTF_MODE_E enIntfMode = VI_MODE_BUTT;
    VI_DEV_ATTR_S *pstDevAttr = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    pstCaptHalChnPrm->uiChn   = uiChn;
    pstCaptHalChnPrm->uiIspId = uiChn;
    pstCaptHalChnPrm->uiSnsId = uiChn;

    memcpy(&pstCaptHalChnPrm->stIspPubAttr,&ISP_PUB_ATTR_fpga_60FPS, sizeof(ISP_PUB_ATTR_S));

    enIntfMode = capt_hisiGetIntfMode();
    switch (enIntfMode)
    {
        case VI_MODE_BT1120_STANDARD:
            pstDevAttr = &DEV_ATTR_BT1120_BASE;
            break;
        case VI_MODE_LVDS:
            pstDevAttr = &DEV_ATTR_LVDS_BASE;
            break;
        default:
            pstDevAttr = NULL;
            break;
    }

    if (NULL == pstDevAttr)
    {
        CAPT_LOGE("unsupport vi intf mode:%d\n", enIntfMode);
        return SAL_FAIL;
    }

    memcpy(&pstCaptHalChnPrm->stDevAttr, pstDevAttr, sizeof(VI_DEV_ATTR_S));
    
    CAPT_LOGI("stDevAttr:aram->(%d_%ux%u)\n",pstCaptHalChnPrm->stDevAttr.enIntfMode
           ,pstCaptHalChnPrm->stDevAttr.stSize.u32Width
           ,pstCaptHalChnPrm->stDevAttr.stSize.u32Height);
    
    memcpy(&pstCaptHalChnPrm->stChnAttr,&CHN_ATTR_720P_420_SDR8_LINEAR,sizeof(VI_CHN_ATTR_S));

    /* 配置 MIPI */
    if (SAL_TRUE == capt_hisiIsUseMipi())
    {
        SAL_clearSize(&pstCaptHalChnPrm->stcomboDevAttr,sizeof(combo_dev_attr_t));
        if (0 == uiChn)
        {
            memcpy(&pstCaptHalChnPrm->stcomboDevAttr,&LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR0,sizeof(combo_dev_attr_t));
        }
        else if (1 == uiChn)
        {
            memcpy(&pstCaptHalChnPrm->stcomboDevAttr,&LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR2,sizeof(combo_dev_attr_t));    /*R7新主板接口换成6，如果R6主板改成ATTR1*/
           
        }
        else
        {
             memcpy(&pstCaptHalChnPrm->stcomboDevAttr,&LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR1,sizeof(combo_dev_attr_t));
        }
        
        capt_hisiGetMipiLaneId(uiChn, &pstCaptHalChnPrm->stcomboDevAttr.devno, pstCaptHalChnPrm->stcomboDevAttr.lvds_attr.lane_id);

        if (SAL_FAIL == capt_hisiSetMipiAttr(pstCaptHalChnPrm,INPUT_MODE_LVDS))
        {
            CAPT_LOGE("Capt Hal Mipi Set Failed !!!\n");

            return SAL_FAIL;
        }
    }
    
    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiReSetMipi
 功能描述  : 采集模块初重新始化Mipi接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiReSetMipi(UINT32 uiChn,UINT32 width, UINT32 height, UINT32 fps)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    pstCaptHalChnPrm->uiChn   = uiChn;
    pstCaptHalChnPrm->uiIspId = uiChn;
    pstCaptHalChnPrm->uiSnsId = uiChn;
    width = SAL_alignDown(width,4);
    height = SAL_alignDown(height,4);
    CAPT_LOGI("uiChn %d..wxh = [%d,%d] fps %d\n",uiChn,width,height,fps);
    pstCaptHalChnPrm->stcomboDevAttr.img_rect.width  = width;
    pstCaptHalChnPrm->stcomboDevAttr.img_rect.height = height;
    pstCaptHalChnPrm->stDevAttr.stSize.u32Width      = width;
    pstCaptHalChnPrm->stDevAttr.stSize.u32Height     = height;
    pstCaptHalChnPrm->stDevAttr.stBasAttr.stSacleAttr.stBasSize.u32Width  = width;
    pstCaptHalChnPrm->stDevAttr.stBasAttr.stSacleAttr.stBasSize.u32Height = height;
    pstCaptHalChnPrm->stDevAttr.stWDRAttr.u32CacheLine = height;
    pstCaptHalChnPrm->stChnAttr.stSize.u32Width      = width;
    pstCaptHalChnPrm->stChnAttr.stSize.u32Height     = height;
    pstCaptHalChnPrm->stChnAttr.stFrameRate.s32SrcFrameRate    = fps;
    pstCaptHalChnPrm->stChnAttr.stFrameRate.s32DstFrameRate    = fps;

    if (SAL_TRUE != capt_hisiIsUseMipi())
    {
        return SAL_SOK;
    }

    /* 配置 MIPI */
    if (SAL_FAIL == capt_hisiSetMipiAttr(pstCaptHalChnPrm,INPUT_MODE_LVDS))
    {
        CAPT_LOGE("Capt Hal Mipi Set Failed !!!\n");

        return SAL_FAIL;
    }
    
    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiDeInitCaptChn
 功能描述  : 初始化 采集通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiDeInitCaptChn(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_PIPE ViPipe   = 0;
    
    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiGetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VI_DisableChn(ViPipe,Videv);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;

}



/*****************************************************************************
 函 数 名  : capt_hisiDeInitCaptDev
 功能描述  : 去初始化 采集设备
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiDeInitCaptDev(CAPT_HAL_CHN_PRM * pstPrm)
{
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiGetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VI_DisableDev(Videv);
    if (HI_SUCCESS != s32Ret)
    {
        CAPT_LOGE("HI_MPI_VI_DisableDev failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    CAPT_LOGI("Capt Hal Stop Vi Dev %d !!!\n", Videv);

    return SAL_SOK;
}

/*****************************************************************************
 函 数 名  : capt_hisiDelSensorIspAttr
 功能描述  : 反初始化 sensor与 ISP属性
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiDelSensorIspAttr(CAPT_HAL_CHN_PRM * pstPrm)
{
    UINT32 uiIspId = 0;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    uiIspId = pstPrm->uiIspId;

    if (SAL_SOK != capt_hisiIspDeInit(pstPrm))
    {
        CAPT_LOGE("Func: %s, Line: %d, Isp DeInit failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiUnRegisterIspLib(uiIspId))
    {
        CAPT_LOGE("Func: %s, Line: %d, Un RegisterIsp Lib failed\n", __FUNCTION__, __LINE__);

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiInitCaptChn
 功能描述  : 初始化 采集通道
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiInitCaptChn(CAPT_HAL_CHN_PRM * pstPrm, CAPT_CHN_ATTR *pstChnAttr)
{
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_PIPE ViPipe  = 0;
    VI_CHN_ATTR_S *pstViChnAttr = NULL;
    VI_DEV_BIND_PIPE_S  stDevBindPipe = {0};
    VI_PIPE_ATTR_S* pstPipeAttr = NULL;
    VI_INTF_MODE_E enIntfMode = capt_hisiGetIntfMode();

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstViChnAttr = &pstPrm->stChnAttr;

    if (VI_MODE_BT1120_STANDARD == enIntfMode)
    {
        pstPipeAttr = &PIPE_ATTR_720P_YUV422_3DNR_RFR;
    }
    else
    {
        pstPipeAttr = &PIPE_ATTR_720P_RAW16_420_3DNR_RFR;
    }

    if (SAL_SOK != capt_hisiGetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }
    
    if (SAL_SOK != capt_hisiGetPipeId(pstPrm->uiChn, &ViPipe))
    {
        CAPT_LOGE("capt chn[%u] get pipe id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }

    stDevBindPipe.PipeId[0] = ViPipe;//pstPrm->uiChn;
    stDevBindPipe.u32Num = 1;
    pstPipeAttr->u32MaxW = pstPrm->stcomboDevAttr.img_rect.width;
    pstPipeAttr->u32MaxH = pstPrm->stcomboDevAttr.img_rect.height;
    pstPipeAttr->stFrameRate.s32DstFrameRate = pstPrm->stChnAttr.stFrameRate.s32DstFrameRate;
    pstPipeAttr->stFrameRate.s32SrcFrameRate = pstPrm->stChnAttr.stFrameRate.s32SrcFrameRate;

    s32Ret = HI_MPI_VI_SetDevBindPipe(Videv, &stDevBindPipe);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_SetDevBindPipe failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VI_CreatePipe(ViPipe, pstPipeAttr);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("HI_MPI_VI_CreatePipe failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    /* 暂时vi chn都是0 */
    s32Ret = HI_MPI_VI_SetChnAttr(ViPipe,0, pstViChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        CAPT_LOGE("failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    HI_MPI_VI_GetChnAttr(ViPipe, 0, &pstPrm->stChnAttr);


    CAPT_LOGI("pipe attr w %d,h %d\n",pstPipeAttr->u32MaxW,pstPipeAttr->u32MaxH);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiInitCaptDev
 功能描述  : 初始化 采集设备
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2017年09月11日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiInitCaptDev(CAPT_HAL_CHN_PRM * pstPrm, CAPT_CHN_ATTR *pstChnAttr)
{
    INT32  s32Ret  = 0;
    VI_DEV Videv = 0;
    VI_DEV_ATTR_S  *pstViDevAttr = &pstPrm->stDevAttr;

    if (NULL == pstPrm)
    {
        CAPT_LOGE("Hal Chn Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != capt_hisiGetDevId(pstPrm->uiChn, &Videv))
    {
        CAPT_LOGE("capt chn[%u] get dev id fail\n", pstPrm->uiChn);
        return SAL_FAIL;
    }
    
    s32Ret = HI_MPI_VI_SetDevAttr(Videv, pstViDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
      CAPT_LOGE("HI_MPI_VI_SetDevAttr failed with %#x! with param->(%d__%ux%u)\n", s32Ret
        ,pstViDevAttr->enIntfMode
        ,pstViDevAttr->stSize.u32Width
        ,pstViDevAttr->stSize.u32Height);
      return SAL_FAIL;
    }

    if (HI_SUCCESS != (s32Ret = HI_MPI_VI_EnableDev(Videv)))
    {
      CAPT_LOGE("HI_MPI_VI_EnableDev chn %d failed with %#x!\n", Videv, s32Ret);
      return SAL_FAIL;
    }

    pstPrm->viDevEn = 1;

    s32Ret = HI_MPI_VI_GetDevAttr(Videv, &pstPrm->stDevAttr);
    if (s32Ret != HI_SUCCESS)
    {
      CAPT_LOGE("HI_MPI_VI_GetDevAttr failed with %#x!\n", s32Ret);
      return SAL_FAIL;
    }

    CAPT_LOGD("Capt Hal Start Vi Dev %d !!!\n", Videv);

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiInitViDev
 功能描述  : Camera采集通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiInitViDev(UINT32 uiDev, CAPT_CHN_ATTR *pstChnAttr)
{

    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiDev];

    if(pstCaptHalChnPrm->viDevEn == 1)
    {
        CAPT_LOGE("Capt Hal Chn %d is started !!!\n",uiDev);
        return SAL_SOK;
    }
        
    pstCaptHalChnPrm->uiChn   = uiDev;
    pstCaptHalChnPrm->uiIspId = uiDev;
    pstCaptHalChnPrm->uiSnsId = uiDev;
    pstCaptHalChnPrm->uiIspInited = 0;

    if (CAPT_CHN_NUM_MAX <= uiDev)
    {
        CAPT_LOGE("Capt Hal Chn is Not Support !!!\n");

        return SAL_FAIL;
    }

    /* 配置 vi dev */
    if (SAL_FAIL == capt_hisiInitCaptDev(pstCaptHalChnPrm, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");

        return SAL_FAIL;
    }

    /* 配置  通道号默认设置为0*/
    if (SAL_FAIL == capt_hisiInitCaptChn(pstCaptHalChnPrm, pstChnAttr))
    {
        CAPT_LOGE("Capt Hal Init Capt Chn Failed !!!\n");

        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
 函 数 名  : capt_hisiDeInitViDev
 功能描述  : Camera采集通道销毁
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日     期   : 2016年12月13日
    作     者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 capt_hisiDeInitViDev(UINT32 uiChn)
{
    CAPT_HAL_CHN_PRM *pstCaptHalChnPrm = NULL;

    pstCaptHalChnPrm = &g_stCaptModPrm.stCaptHalChnPrm[uiChn];

    /* 销毁 vi chn */
    if (SAL_FAIL == capt_hisiDeInitCaptChn(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Init Capt Chn Failed !!!\n");

        return SAL_FAIL;
    }

    /* 销毁 vi dev */
    if (SAL_FAIL == capt_hisiDeInitCaptDev(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Init Dev Failed !!!\n");

        return SAL_FAIL;
    }

    /* 销毁 sensor & isp */
    if (SAL_FAIL == capt_hisiDelSensorIspAttr(pstCaptHalChnPrm))
    {
        CAPT_LOGE("Capt Hal Sensor And Isp Set Failed !!!\n");

        return SAL_FAIL;
    }
    pstCaptHalChnPrm->uiStarted = 0;
    memset(pstCaptHalChnPrm, 0, sizeof(CAPT_HAL_CHN_PRM));

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : capt_halRegister
* 描  述  : 注册hisi disp显示函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
CAPT_PLAT_OPS *capt_halRegister(void)
{
    memset(&g_stCaptPlatOps, 0, sizeof(CAPT_PLAT_OPS));

    g_stCaptPlatOps.GetCaptPipeId =        capt_hisiGetPipeId;
    g_stCaptPlatOps.initCaptInterface =   capt_hisiInterface;
    g_stCaptPlatOps.reSetCaptInterface =       capt_hisiReSetMipi;
    g_stCaptPlatOps.initCaptDev   =       capt_hisiInitViDev;
    g_stCaptPlatOps.deInitCaptDev =       capt_hisiDeInitViDev;
    g_stCaptPlatOps.startCaptDev  =       capt_hisiStartCaptDev;
    g_stCaptPlatOps.stopCaptDev   =       capt_hisistopCaptDev;
    g_stCaptPlatOps.getCaptDevState =     capt_hisiGetDevStatus;
    g_stCaptPlatOps.setCaptChnRotate  =   capt_hisiSetChnRotate;

    g_stCaptPlatOps.enableCaptUserPic =   capt_hisiEnableCaptUserPic;
    g_stCaptPlatOps.setCaptUserPic    =   capt_hisiSetCaptUserPic;
    g_stCaptPlatOps.getCaptUserPicStatus = capt_hisiGetUserPicStatue;
    
    g_stCaptPlatOps.checkCaptIsOffset =    capt_hisiCaptIsOffset;
     
    return &g_stCaptPlatOps;
}

