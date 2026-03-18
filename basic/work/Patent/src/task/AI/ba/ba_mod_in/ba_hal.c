/*******************************************************************************
* ba_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年9月5日 Create
*
* Description :
* Modification:
*******************************************************************************/

/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
//#include <platform_sdk.h>
#include <platform_hal.h>

#include "ba_hal.h"


/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */
#define BA_HAL_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            BA_LOGE("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define BA_HAL_CHECK_RET_NO_LOOP(ret, str) \
    { \
        if (ret) \
        { \
            BA_LOGE("%s, ret: 0x%x \n", str, ret); \
        } \
    }

#define BA_HAL_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            BA_LOGE("%s \n", str); \
            goto loop; \
        } \
    }

#define BA_HAL_CHECK_SUB_MOD_ID(idx, value) \
    { \
        if (idx >= BA_SUB_MOD_MAX_NUM) \
        { \
            BA_LOGE("Invalid sub mod idx %d \n", idx); \
            return value; \
        } \
    }

#define BA_SUB_MOD_NUM (1)                      /* 行为分析子模块个数 */

/* ========================================================================== */
/*                          数据结构区									   */
/* ========================================================================== */
static UINT32 baDbLevel = DEBUG_LEVEL0;        /* 日志级别 */
static BA_MOD_S g_stBaModPrm;                  /* 模块全局参数 */

VOID *pCbFunction[BA_SUB_MOD_MAX_NUM] = {NULL, NULL, NULL, NULL};   /* 引擎回调参数接口，由DRV层注册过来 */


/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */


/**
 * @function:   ba_HalGetInitFlag
 * @brief:      获取模式初始化状态标志
 * @param[in]:  VOID
 * @param[out]: 无
 * @return:     UINT32
*/
UINT32 Ba_HalGetInitFlag(void)
{
    return g_stBaModPrm.uiInitFlag;
}

/**
 * @function:   Ba_HalGetModPrm
 * @brief:      获取模块参数信息
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     BA_MOD_S *
 */
BA_MOD_S *Ba_HalGetModPrm(VOID)
{
    return &g_stBaModPrm;
}


/**
 * @function:   Ba_HalGetSubModPrm
 * @brief:      获取子模块参数信息
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     BA_SUB_MOD_S *
 */
BA_SUB_MOD_S *Ba_HalGetSubModPrm(UINT32 uiIdx)
{
    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, NULL);
    return g_stBaModPrm.pstBaSubMod[uiIdx];
}

/**
 * @function:   Ba_HalSetCbFunc
 * @brief:      注册回调函数
 * @param[in]:  UINT32 uiIdx
 * @param[in]:  BA_CALL_BACK_FUNC pFunc
 * @param[out]: None
 * @return:     VOID *
 */
VOID *Ba_HalSetCbFunc(UINT32 uiIdx, BA_CALL_BACK_FUNC pFunc)
{
    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, NULL);

    pCbFunction[uiIdx] = pFunc;
    return NULL;
}



/**
 * @function:   Ba_HalGetVersionInfo
 * @brief:      获取版本号信息
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalGetVersionInfo(UINT32 uiIdx)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pSubModHdl = NULL;
    BA_SUB_MOD_S *pstBaSubModPrm = NULL;
    BA_MOD_S *pstBaModPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};
    ICF_VERSION stAlgVersion = {0};
    ICF_VERSION stIcfVersion = {0};
    //ICF_VERSION stAppVersion = {0};

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");
	
    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, SAL_FAIL);
    
    pstBaSubModPrm = Ba_HalGetSubModPrm(uiIdx);
    BA_HAL_CHECK_PTR(pstBaSubModPrm, err, "pstBaSubModPrm == null!");

    pSubModHdl = pstBaSubModPrm->stHandle.pChannelHandle;//子模块句柄
    BA_HAL_CHECK_PTR(pSubModHdl, err, "pSubModHdl == null!");

	//get alg version
    stConfigParam.nNodeID = SBAE_NODE_BA;
    stConfigParam.nKey = SBAE_ALG_CONFIG_GET_VERSION;
    stConfigParam.pConfigData = &stAlgVersion;
    stConfigParam.nConfSize = sizeof(ICF_VERSION);
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
	BA_HAL_CHECK_RET(s32Ret, err, "get Config sencse error");

    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetVersion(&stIcfVersion, sizeof(stIcfVersion));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_GetVersion failed!");
#if 0	//后续如果使用stAppVersion
    //s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfAppGetVersion(&stAppVersion, sizeof(stAppVersion));
    //BA_HAL_CHECK_RET(s32Ret, err, "APP_GetVersion failed!");

    /* Save Version Info */
    /*s32Ret = snprintf((CHAR *)pstBaSubModPrm->acVerInfo,
                      sizeof(pstBaSubModPrm->acVerInfo),
                      "ICF Version: V_%d.%d.%d build %d-%d-%d, APP Version: V_%d.%d.%d build %d-%d-%d, ALG Version: V_%d.%d.%d build 20%d-%d-%d",
                      stIcfVersion.nMajorVersion, stIcfVersion.nRevisVersion, stIcfVersion.nSubVersion,
                      stIcfVersion.nVersionYear, stIcfVersion.nVersionMonth, stIcfVersion.nVersionDay,
                      stAppVersion.nMajorVersion, stAppVersion.nRevisVersion, stAppVersion.nSubVersion,
                      stAppVersion.nVersionYear, stAppVersion.nVersionMonth, stAppVersion.nVersionDay,
                      stAlgVersion.nMajorVersion, stAlgVersion.nRevisVersion, stAlgVersion.nSubVersion,
                      stAlgVersion.nVersionYear, stAlgVersion.nVersionMonth, stAlgVersion.nVersionDay);*/
#endif
	s32Ret = snprintf((CHAR *)pstBaSubModPrm->acVerInfo,
                      sizeof(pstBaSubModPrm->acVerInfo),
                      "ICF Version: V_%d.%d.%d build %d-%d-%d, ALG Version: V_%d.%d.%d build 20%d-%d-%d",
                      stIcfVersion.nMajorVersion, stIcfVersion.nSubVersion, stIcfVersion.nRevisVersion,
                      stIcfVersion.nVersionYear, stIcfVersion.nVersionMonth, stIcfVersion.nVersionDay,
                      stAlgVersion.nMajorVersion, stAlgVersion.nSubVersion, stAlgVersion.nRevisVersion,
                      stAlgVersion.nVersionYear, stAlgVersion.nVersionMonth, stAlgVersion.nVersionDay);
    if (s32Ret <= 0 || s32Ret > sizeof(pstBaSubModPrm->acVerInfo))
    {
        BA_LOGE("snprintf errrrr! \n");
        return SAL_FAIL;
    }

    BA_LOGI("Version_Info: %s \n", pstBaSubModPrm->acVerInfo);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalGetIcfCfg
 * @brief:      获取当前配置
 * @param[in]:  UINT32 uiIdx
 * @param[in]:  SBAE_SENSCTRL_PARAMS_T *pstSensityPrm
 * @param[in]:  SBAE_APP_ALERT_CONFIG_T *pstAppAlertPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalGetIcfCfg(UINT32 uiIdx, SBAE_DBA_PROC_PARAMS_T *pstSensityPrm, SBAE_DBA_PROC_PARAMS_T *pstAppAlertPrm)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pSubModHdl = NULL;
    BA_SUB_MOD_S *pstSubModPrm = NULL;
    BA_MOD_S *pstBaModPrm = NULL;
	ICF_CONFIG_PARAM stConfigParam = {0};

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, SAL_FAIL);
    BA_HAL_CHECK_PTR(pstSensityPrm, err, "pstSensityPrm == null!");
    BA_HAL_CHECK_PTR(pstAppAlertPrm, err, "pstAppAlertPrm == null!");

    pstSubModPrm = Ba_HalGetSubModPrm(uiIdx);
    BA_HAL_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    pSubModHdl = pstSubModPrm->stHandle.pChannelHandle;
    BA_HAL_CHECK_PTR(pSubModHdl, err, "pSubModHdl == null!");
    stConfigParam.nNodeID = SBAE_NODE_BA;
    stConfigParam.nKey = SBAE_ALG_CONFIG_GET_DBA_PARAMS;
    stConfigParam.pConfigData = pstSensityPrm;
    stConfigParam.nConfSize = sizeof(SBAE_DBA_PROC_PARAMS_T);

    // 获取默认参数
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
	BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");
	
	stConfigParam.nNodeID = SBAE_NODE_BA;
    stConfigParam.nKey = SBAE_ALG_CONFIG_GET_DBA_PARAMS;
    stConfigParam.pConfigData = pstAppAlertPrm;
    stConfigParam.nConfSize = sizeof(SBAE_DBA_PROC_PARAMS_T);

	s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
	BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");

    //s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, SBAE_NODE_BA, SBAE_ALG_CONFIG_GET_DBA_PARAMS, pstSensityPrm, sizeof(SBAE_DBA_PROC_PARAMS_T));
    //BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");

    //s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, SBAE_NODE_BA, SBAE_ALG_CONFIG_GET_DBA_PARAMS, pstAppAlertPrm, sizeof(SBAE_DBA_PROC_PARAMS_T));
    //BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalSetSensity
 * @brief:      设置灵敏度
 * @param[in]:  void *pSubModHdl
 * @param[in]:  UINT32 *puiSensity
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ba_HalSetSensity(void *pSubModHdl, UINT32 *puiSensity)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiVal = 0;

    SBAE_DBA_PROC_PARAMS_T stSensityPrm = {0};
    BA_MOD_S *pstBaModPrm = NULL;
	ICF_CONFIG_PARAM stConfigParam = {0};

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    uiVal = *puiSensity > BA_MAX_SENSITY ? BA_MAX_SENSITY : *puiSensity < BA_MIN_SENSITY ? BA_MIN_SENSITY : *puiSensity;
	
    stConfigParam.nNodeID = SBAE_NODE_BA;
    stConfigParam.nKey = SBAE_ALG_CONFIG_GET_DBA_PARAMS;
    stConfigParam.pConfigData = &stSensityPrm;
    stConfigParam.nConfSize = sizeof(SBAE_DBA_PROC_PARAMS_T);
	s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");

    /* 当前只支持在离岗和遮挡的持续时间配置，在hcd类别下面公用参数 */
	uiVal = 6;
    stSensityPrm.stBehaviorInput[SBAE_DBA_FUNC_HCD].bev_duration = uiVal; /* 遮挡摄像头持续时间 */

	stConfigParam.nNodeID = SBAE_NODE_BA;
    stConfigParam.nKey = SBAE_ALG_CONFIG_SET_DBA_PARAMS;
    stConfigParam.pConfigData = &stSensityPrm;
    stConfigParam.nConfSize = sizeof(SBAE_DBA_PROC_PARAMS_T);
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfSetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");

    BA_LOGW("hal set sensity %d end! \n", uiVal);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalSetIcfCfg
 * @brief:      设置配置参数
 * @param[in]:  UINT32 uiIdx
 * @param[in]:  BA_CONFIG_TYPE_E enCfgType
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalSetIcfCfg(UINT32 uiIdx, BA_CONFIG_TYPE_E enCfgType, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pSubModHdl = NULL;
    BA_SUB_MOD_S *pstSubModPrm = NULL;

    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, SAL_FAIL);
    BA_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstSubModPrm = Ba_HalGetSubModPrm(uiIdx);
    BA_HAL_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    pSubModHdl = pstSubModPrm->stHandle.pChannelHandle;
    BA_HAL_CHECK_PTR(pSubModHdl, err, "pSubModHdl == null!");

    switch (enCfgType)
    {
        case SENSITY_TYPE:
            s32Ret = Ba_HalSetSensity(pSubModHdl, prm);
            BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalSetSensity failed!");
            break;
        default:
            BA_LOGW("Invalid type %d \n", enCfgType);
            break;
    }

    BA_LOGI("Set Icf Cfg End! idx %d \n", uiIdx);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalSetIcfDefaultCfg
 * @brief:      设置默认配置
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalSetIcfDefaultCfg(UINT32 uiIdx)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pSubModHdl = NULL;
	BA_MOD_S *pstBaModPrm = NULL;
    BA_SUB_MOD_S *pstSubModPrm = NULL;

    /*设置或获取设置参数的参数*/
	ICF_CONFIG_PARAM stConfigParam = {0};
    SBAE_DBA_PROC_PARAMS_T stDbaParam = {0};
	

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, SAL_FAIL);

    pstSubModPrm = Ba_HalGetSubModPrm(uiIdx);
    BA_HAL_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    pSubModHdl = pstSubModPrm->stHandle.pChannelHandle;//子模块句柄
    BA_HAL_CHECK_PTR(pSubModHdl, err, "pSubModHdl == null!");

    /* 获取默认参数 */ 
    stConfigParam.nNodeID = SBAE_NODE_BA;
    stConfigParam.nKey = SBAE_ALG_CONFIG_GET_DBA_PARAMS;
    stConfigParam.pConfigData = &stDbaParam;
    stConfigParam.nConfSize = sizeof(SBAE_DBA_PROC_PARAMS_T);

    // 获取默认参数
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Get_config failed!");
	
	// 配置行为分析参数
    //外部配置行为分析报警参数    
    //实车/过检
    //stDbaParam.stCommonParams.dba_proc_mode = SBAE_DBA_PROC_REAL;
    stDbaParam.stCommonParams.dba_proc_mode = SBAE_DBA_PROC_DEMO;
    stDbaParam.stCommonParams.dba_install_loc = SBAE_DBA_INSTALL_MIDDLE;
    // 帧率
    stDbaParam.stCommonParams.fps = 7.5;

    // 持续时间参数
    stDbaParam.stNlfParams.nlf_duration = 4;      // 不目视前方持续时间
    stDbaParam.stFddParams.fdd_duration_severe = 2;   // 疲劳持续时间
    stDbaParam.stFddParams.fdd_duration_slight = 1;   // 二级疲劳持续时间
    stDbaParam.stFddParams.fdd_duration_yawn = 3;
    stDbaParam.stFddParams.blink_fdd_num_th = 10;
    stDbaParam.stFddParams.is_blink_clear = 1;

    stDbaParam.stBehaviorNum = 8;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_HCD].type = SBAE_DBA_FUNC_HCD;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_HCD].bev_sensitive = 80;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_HCD].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_ABNORMAL].type = SBAE_DBA_FUNC_ABNORMAL;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_ABNORMAL].bev_sensitive = 80;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_ABNORMAL].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_ABNORMAL].speed_thresh = 30;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_UPD].type = SBAE_DBA_FUNC_UPD;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_UPD].bev_sensitive = 60;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_UPD].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_UPD].speed_thresh = 20;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SMD].type = SBAE_DBA_FUNC_SMD;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SMD].bev_sensitive = 60;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SMD].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SMD].speed_thresh = 20;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_BELT].type = SBAE_DBA_FUNC_BELT;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_BELT].bev_sensitive = 85;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_BELT].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_BELT].speed_thresh = 10;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SUNGLASS].type = SBAE_DBA_FUNC_SUNGLASS;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SUNGLASS].bev_sensitive = 70;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_SUNGLASS].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_NO_MASK].type = SBAE_DBA_FUNC_NO_MASK;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_NO_MASK].bev_sensitive = 90;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_NO_MASK].bev_duration = (INT32)(3);
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_XUING].type = SBAE_DBA_FUNC_XUING;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_XUING].bev_sensitive = 90;
    stDbaParam.stBehaviorInput[SBAE_DBA_FUNC_XUING].bev_duration = (INT32)(3);


    // 疲劳行为参数
    stDbaParam.stFddParams.fdd_sensitive = 50;
    stDbaParam.stFddParams.perclos_ratio = (INT32)(50);
    stDbaParam.stFddParams.perclos_range = (INT32)(3);
    stDbaParam.stFddParams.speed_thresh  = 20;	


    // 分心转头参数
    stDbaParam.stNlfParams.turn_sensitive[0] = (INT32)(20);
    stDbaParam.stNlfParams.turn_sensitive[1] = (INT32)(20);
    stDbaParam.stNlfParams.turn_sensitive[2] = (INT32)(70);
    stDbaParam.stNlfParams.turn_sensitive[3] = (INT32)(10);
    stDbaParam.stNlfParams.speed_thresh = 40;


    // 疲劳、分心行为等级参数
    // 疲劳等级数：轻度疲劳1级、轻度疲劳2级
    stDbaParam.stFddParams.bev_lev_info.level_num = 2; 
    // 轻度疲劳1级
    stDbaParam.stFddParams.bev_lev_info.lev_factor[0].speed    = 20;   // 速度阈值
    stDbaParam.stFddParams.bev_lev_info.lev_factor[0].duration = 0;   // 持续时间，单位s
    stDbaParam.stFddParams.bev_lev_info.lev_factor[0].ratio    = 80;    // 持续比例
    stDbaParam.stFddParams.bev_lev_info.lev_factor[0].perclos_range = 3;
    // 轻度疲劳2级
    stDbaParam.stFddParams.bev_lev_info.lev_factor[1].speed    = 20;   // 速度阈值
    stDbaParam.stFddParams.bev_lev_info.lev_factor[1].duration = 0;   // 持续时间，单位s
    stDbaParam.stFddParams.bev_lev_info.lev_factor[1].ratio    = 80;   // 持续比例
    stDbaParam.stFddParams.bev_lev_info.lev_factor[1].perclos_range = 7;

    // 分心等级数：分心1级、分心2级
    stDbaParam.stNlfParams.bev_lev_info.level_num = 2; 
    // 分心1级
    stDbaParam.stNlfParams.bev_lev_info.lev_factor[0].speed    = 20;   // 速度阈值
    stDbaParam.stNlfParams.bev_lev_info.lev_factor[0].duration = 5;    // 持续时间，单位s
    stDbaParam.stNlfParams.bev_lev_info.lev_factor[0].ratio    = 99;   // 持续比例
    // 分心2级
    stDbaParam.stNlfParams.bev_lev_info.lev_factor[1].speed    = 40;   // 速度阈值
    stDbaParam.stNlfParams.bev_lev_info.lev_factor[1].duration = 9;    // 持续时间，单位s
    stDbaParam.stNlfParams.bev_lev_info.lev_factor[1].ratio    = 99;   // 持续比例


    stConfigParam.nKey = SBAE_ALG_CONFIG_SET_DBA_PARAMS;

    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfSetConfig(pSubModHdl, &stConfigParam, sizeof(ICF_CONFIG_PARAM));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config failed!");

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalIcfUnRegister
 * @brief:      引擎子模块反注册
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalIcfUnRegister(UINT32 uiIdx)
{
    INT32 s32Ret = SAL_SOK;
    BA_SUB_MOD_S *pstSubModPrm = NULL;
    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    pstSubModPrm = Ba_HalGetSubModPrm(uiIdx);
    BA_HAL_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, SAL_FAIL);
	
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfDestroy(&pstSubModPrm->stHandle,sizeof(ICF_CREATE_HANDLE));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Destroy failed!");

	s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfUnloadModel(&pstSubModPrm->stModelHandle, sizeof(ICF_MODEL_HANDLE));
	BA_HAL_CHECK_RET(s32Ret, err, "ICF_Destroy failed!");

    BA_LOGI("icf unregister end! idx %d \n", uiIdx);
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalIcfRegister
 * @brief:      引擎子模块注册
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalIcfRegister(UINT32 uiIdx)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pModHdl = NULL;
    BA_MOD_S *pstBaModPrm = NULL;
    BA_SUB_MOD_S *pstBaSubModPrm = NULL;

    /*加载模型参数*/
	ICF_MODEL_HANDLE stModelHandle = {0};
    ICF_MODEL_PARAM stModelParam = {0};
	
	/*创建引擎模型参数*/
	ICF_CREATE_PARAM stCreateParam = {0};
	ICF_CREATE_HANDLE  stSubModHdl = {0};

	/*回调设置参数*/
	ICF_CALLBACK_PARAM stCallbackParam = {0};

    //ICF_APP_PARAM_INFO stIcfAppParam = {0};

    BA_HAL_CHECK_SUB_MOD_ID(uiIdx, SAL_FAIL);

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    pModHdl = pstBaModPrm->stModHandle.pInitHandle;//模块初始化句柄
    BA_HAL_CHECK_PTR(pModHdl, err, "pModHdl == null!");

    stModelParam.nGraphID = 1;
    stModelParam.nGraphType = 1;
    stModelParam.pAppParam = NULL;
    stModelParam.nMaxCacheNums = 0;
    stModelParam.pInitHandle = pModHdl;

    BA_LOGW("ICF_LoadModel begin\r\n");
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfLoadModel(&stModelParam, sizeof(ICF_MODEL_PARAM), &stModelHandle, sizeof(ICF_MODEL_HANDLE));
    BA_LOGW("ICF_LoadModel end nRet = 0x%x\r\n",s32Ret);
    if (ICF_SUCCESS != s32Ret)
    {
        BA_LOGE("ICF_GetModelMemSize 0x%x error\n", s32Ret);
        return -1;
    }
    memcpy(&(stCreateParam.modelParam),  &stModelParam,  sizeof(ICF_MODEL_PARAM_V2));
    memcpy(&(stCreateParam.modelHandle), &stModelHandle, sizeof(ICF_MODEL_HANDLE_V2));

	Ba_HalPrint("before ICF_Create");

    //创建ICF句柄
    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfCreate(&stCreateParam, sizeof(ICF_CREATE_PARAM), &stSubModHdl, sizeof(ICF_CREATE_HANDLE));
    if (ICF_SUCCESS != s32Ret)
    {
        BA_LOGE("ICF_Create Sub Mod error, idx %d, ret 0x%x \n", uiIdx, s32Ret);
        (VOID)pstBaModPrm->stIcfFuncP.Ba_IcfDestroy(&stSubModHdl,sizeof(ICF_CREATE_HANDLE));
        return SAL_FAIL;
    }   

    //stIcfAppParam.pAppParamCfgPath = (CHAR *)"./ba/config/sbae_param.json"; 

    

    /*s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfCreate(pModHdl, 1, 1, &stIcfAppParam, &pSubModHdl);
    if (ICF_SUCCESS != s32Ret)
    {
        BA_LOGE("ICF_Create Sub Mod error, idx %d, ret 0x%x \n", uiIdx, s32Ret);
        (VOID)pstBaModPrm->stIcfFuncP.Ba_IcfDestroy(pSubModHdl);
        return SAL_FAIL;
    }*/

    Ba_HalPrint("after ICF_Create");
	BA_LOGW("Finsih createICF end nRet = %d\r\n",s32Ret);

    if (NULL == g_stBaModPrm.pstBaSubMod[uiIdx])
    {
        g_stBaModPrm.pstBaSubMod[uiIdx] = SAL_memMalloc(sizeof(BA_SUB_MOD_S), "BA", "ba_SubModPrm");
        BA_HAL_CHECK_PTR(g_stBaModPrm.pstBaSubMod[uiIdx], err, "pstBaSubMod == null!");
    }

    memset(g_stBaModPrm.pstBaSubMod[uiIdx], 0x00, sizeof(BA_SUB_MOD_S));

    pstBaSubModPrm = Ba_HalGetSubModPrm(uiIdx);
    BA_HAL_CHECK_PTR(pstBaSubModPrm, err, "pstBaSubModPrm == null!");

    BA_HAL_CHECK_PTR(pCbFunction[uiIdx], err, "pCbFunction == null!");

	// ICF引擎 设置回调    
    stCallbackParam.nNodeID = SBAE_NODE_POST;//需要根据配置图变化
    stCallbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
    stCallbackParam.pCallbackFunc = (void *)(pCbFunction[uiIdx]);
    stCallbackParam.nUserSize = 0;
    stCallbackParam.pUsr = NULL;

	pstBaModPrm->stIcfFuncP.Ba_IcfSetCallback(stSubModHdl.pChannelHandle, &stCallbackParam,sizeof(ICF_CALLBACK_PARAM));
	BA_LOGW("Finsih setcallback end nRet = %d\r\n",s32Ret);

    Ba_HalPrint("ICF_Set_callback");
	sal_memcpy_s(&pstBaSubModPrm->stHandle, sizeof(ICF_CREATE_HANDLE), &stSubModHdl, sizeof(ICF_CREATE_HANDLE));//获取子模块句柄
	sal_memcpy_s(&pstBaSubModPrm->stModelHandle, sizeof(ICF_MODEL_HANDLE), &stModelHandle, sizeof(ICF_MODEL_HANDLE));//获取子模块模型句柄
    pstBaSubModPrm->uiUseFlag = SAL_TRUE;

    /* 获取内部使用内存大小 */

    //s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetMemPoolStatus(pModHdl);

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalIcfSubModDeInit
 * @brief:      引擎子模块去初始化
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalIcfSubModDeInit(UINT32 uiIdx)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Ba_HalIcfUnRegister(uiIdx);
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalIcfUnRegister failed!");

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalIcfSubModInit
 * @brief:      引擎子模块初始化
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalIcfSubModInit(UINT32 uiIdx)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Ba_HalIcfRegister(uiIdx);
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalIcfRegister failed!");

    Ba_HalPrint("Ba_HalIcfRegister");

    s32Ret = Ba_HalSetIcfDefaultCfg(uiIdx);
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalSetIcfDefaultCfg failed!");

    Ba_HalPrint("Ba_HalSetIcfDefaultCfg");

    s32Ret = Ba_HalGetVersionInfo(uiIdx);
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalGetVersionInfo failed!");

    Ba_HalPrint("Ba_HalGetVersionInfo");
    return SAL_SOK;

err:
    return SAL_FAIL;
}



/**
 * @function:   Ba_HalIcfDeInit
 * @brief:      引擎资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalIcfDeInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfFinit(&pstBaModPrm->stModHandle,sizeof(ICF_INIT_HANDLE));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Finit failed!");

    BA_LOGI("Icf Deinit end! \n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}
#if 0
static CHAR g_ba_cfg_path[4][64] =
{
    {"./ba/config/AlgCfgTest.json"},             /* AlgCfgPath */
    {"./ba/config/TaskCfg.json"},                   /* TaskCfgPath */
    {"./ba/config/ToolkitCfg.json"},             /* ToolkitCfgPath */
    {"./sva/config/icf_secdet_param.json"},       /* AppParamCfgPath */
};

/**
 * @function:   Sva_HalGetRunTimeLimit
 * @brief:      获取引擎使用的内存大小
 * @param[in]:  ICF_INIT_PARAM *pstInitParam
 * @param[out]: None
 * @return:     static VOID
 */
INT32 Ba_HalGetRunTimeLimit(ICF_INIT_PARAM *pstInitParam)
{

    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 uiGraphNum = 1;  /* 分析仪目前仅有一个通道，安检机是两个，按需调整 */

    ICF_LIMIT_INPUT stRunLimit = { 0 };
    ICF_LIMIT_OUTPUT stRunOut = { 0 };
    ICF_APP_PARAM_INFO stAppParam = { 0 };

    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_RETURN(pstBaModPrm == NULL, SAL_FAIL);

    stAppParam.pAppParamCfgPath = (char *)g_ba_cfg_path[3];

    stRunLimit.nGraphType = 1;
    stRunLimit.nGraphIDNum = uiGraphNum;
    stRunLimit.pInitParam = pstInitParam;
    stRunLimit.nGraphID[0] = 1;
    stRunLimit.pAppParam[0]	= &stAppParam;
    stRunLimit.nMaxCacheNum[0] = 0;

    s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfGetRunTimeLimit(&stRunLimit, &stRunOut);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("ICF_GetRuntimeLimit error, ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    for (i = 0; i < ICF_MEM_TYPE_NUM; i++)
    {
        if (stRunOut.stMemTotal.stMemTotal[i].nMemSize > 0)
        {
            stRunOut.stMemTotal.stMemTotal[i].nMemSize = stRunOut.stMemTotal.stMemTotal[i].nMemSize * (1 + 0.1);  /* 引擎内部计算使用内存需要上浮0.1 */
            pstInitParam->stMemConfig.stMemInfo[i].eMemType = stRunOut.stMemTotal.stMemTotal[i].eMemType;
            pstInitParam->stMemConfig.stMemInfo[i].nMemSize = stRunOut.stMemTotal.stMemTotal[i].nMemSize;

            SAL_ERROR("eMemType:\t%d\t, nMemSize:\t%11lld\tByte\t%f\tMB \n",
                      stRunOut.stMemTotal.stMemTotal[i].eMemType,
                      stRunOut.stMemTotal.stMemTotal[i].nMemSize,
                      stRunOut.stMemTotal.stMemTotal[i].nMemSize / 1024.0 / 1024.0);

            pstInitParam->stMemConfig.nNum++;
        }
    }

    /* 以DSP内部统计的内存为准 */
    (VOID)Ia_PrintMemInfo(IA_MOD_SVA);

    /* 当前调用引擎接口获取运行时内存使用情况时需要在接口返回后清空内存，因为当前注册给引擎的内存操作回调接口不支持内存动态分配 */
    (VOID)Ia_ResetModMem(IA_MOD_SVA);

    return SAL_SOK;
}
#endif
#if 0 //申请系统内存的回调函数,调试使用，后期无用则删除
/*******************************************************************************
* 函数名  : Ba_HalMemPoolSystemMallocCb
* 描  述  : 申请系统内存的回调函数
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  :
*******************************************************************************/
INT32 Ba_HalMemPoolSystemMallocCb(void *pInitHandle, UINT32 uBufSize, INT32 nMemSpace, ICF_MEM_BUFFER *stMemBuffer)
{
    IA_MEM_PRM_S stIaMemPrm = {0};

    switch (nMemSpace)
    {
        case ICF_HISI_MEM_MMZ_NO_CACHE:
        {
            if (SAL_SOK != Ia_GetXsiFreeMem(IA_MOD_BA, IA_HISI_MMZ_NO_CACHE, uBufSize, &stIaMemPrm))
            {
                SVA_LOGE("Get Xsi Free Mem Failed! size %d, space %d \n", uBufSize, nMemSpace);
                return SAL_FAIL;
            }

            goto exit;
        }
        case ICF_HISI_MEM_MMZ_NO_CACHE_PRIORITY:
        {
            if (SAL_SOK != Ia_GetXsiFreeMem(IA_MOD_BA, IA_HISI_MMZ_CACHE_NO_PRIORITY, uBufSize, &stIaMemPrm))
            {
                SVA_LOGE("Get Xsi Free Mem Failed! size %d, space %d \n", uBufSize, nMemSpace);
                return SAL_FAIL;
            }

            goto exit;
        }

        case ICF_MEM_MALLOC:
        {
            if (SAL_SOK != Ia_GetXsiFreeMem(IA_MOD_BA, IA_MALLOC, uBufSize, &stIaMemPrm))
            {
                SVA_LOGE("Get Xsi Free Mem Failed! size %d, space %d \n", uBufSize, nMemSpace);
                return SAL_FAIL;
            }

            goto exit;
        }
        case ICF_HISI_MEM_MMZ_WITH_CACHE:
        {
            if (SAL_SOK != Ia_GetXsiFreeMem(IA_MOD_BA, IA_HISI_MMZ_CACHE, uBufSize, &stIaMemPrm))
            {
                SVA_LOGE("Get Xsi Free Mem Failed! size %d, space %d \n", uBufSize, nMemSpace);
                return SAL_FAIL;
            }

            goto exit;
        }
        case ICF_HISI_MEM_MMZ_WITH_CACHE_PRIORITY:
        {
            if (SAL_SOK != Ia_GetXsiFreeMem(IA_MOD_BA, IA_HISI_MMZ_CACHE_PRIORITY, uBufSize, &stIaMemPrm))
            {
                SVA_LOGE("Get Xsi Free Mem Failed! size %d, space %d \n", uBufSize, nMemSpace);
                return SAL_FAIL;
            }

            goto exit;
        }
        default:
        {
            return ICF_MEMPOOL_ERR_SPACE_TYPE;
        }
    }

exit:
    stMemBuffer->pPhyMemory = (VOID *)stIaMemPrm.PhyAddr;
    stMemBuffer->pVirMemory = (VOID *)stIaMemPrm.VirAddr;

    return ICF_SUCCESS;
}

/*******************************************************************************
* 函数名  : Ba_HalMemPoolSystemFreeCb
* 描  述  : 释放系统内存的回调函数
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  :
*******************************************************************************/
INT32 Ba_HalMemPoolSystemFreeCb(void *pInitHandle, void *pMemory, INT32 nSpace)
{
    if (NULL == pMemory)
    {
        SVA_LOGE("MemPoolSystemFree input pMemory = NULL, nRet = %x", ICF_MEMPOOL_NULLPOINTER);
    }

    switch (nSpace)
    {
        case ICF_MEM_MALLOC:
        case ICF_HISI_MEM_MMZ_NO_CACHE:
        case ICF_HISI_MEM_MMZ_WITH_CACHE:
        case ICF_HISI_MEM_MMZ_NO_CACHE_PRIORITY:
        case ICF_HISI_MEM_MMZ_WITH_CACHE_PRIORITY:
        {
            return ICF_SUCCESS;
        }
        default:
        {
            return ICF_MEMPOOL_ERR_SPACE_TYPE;
        }
    }

    return ICF_SUCCESS;
}
#endif


#if 0//申请系统内存的回调函数,调试使用，后期无用则删除
void* hik_aligned_malloc(size_t size, size_t alignment)
{
	if (alignment & (alignment - 1))
	{
		return NULL;
	}
	else
	{
		void *praw = SAL_memMalloc(sizeof(void*) + size + alignment, "BA", "ba_debug");

		if (praw)
		{
			void *pbuf = (void *)((size_t)(praw)+sizeof(void*));

			void *palignedbuf = (void *)(((size_t)(pbuf) | (alignment - 1)) + 1);
			((void**)palignedbuf)[-1] = praw;
			return palignedbuf;
		}
		else
		{
			printf("fail to malloc\r\n");
			return NULL;
		}
	}
}

void hik_aligned_free(void *palignedmem)
{
	SAL_memfree((void*)(((void**)palignedmem)[-1]));
}

/**********************************************************************************************************************
* 功  能: 引擎底层内存申请接口实现
* 参  数: 
*        uBufSize          - I 内存大小
*        nMemSpace         - I 内存类型
*        stMemBuffer       - O 内存buffer
* 返回值: 状态码
**********************************************************************************************************************/
int MemPoolSystemAllocCallback(void               *pInitHandle,
                               ICF_MEM_INFO              *pMemInfo,                             
                               ICF_MEM_BUFFER  			*stMemBuffer)
{
    int                 nRet       = 0;
    //unsigned long long  u64PhyAddr = 0;
    void                *pVirAddr   = NULL;

    long long     			uBufSize = pMemInfo->nMemSize;
    int              		nMemSpace = pMemInfo->eMemType;

    printf("****************************************************in user's MemPoolSystemAllocCallback function\n");
	printf("nMemSpace = %d,uBufSize = %lld\n",nMemSpace,uBufSize);

    switch (nMemSpace) 
    {

        case ICF_MEM_MALLOC:
        {
            pVirAddr = (void *)hik_aligned_malloc(uBufSize,128);
			if (NULL == pVirAddr)
            {
                printf("alloc ICF_MEM_MALLOC memory failed, nRet = %x\n", ICF_MEMPOOL_ERR_MALLOC);
                return ICF_MEMPOOL_ERR_MALLOC;
            }
            memset((void *)pVirAddr, 0, uBufSize);

            // 对于没有物理地址的，将虚拟地址赋给物理地址指针
            stMemBuffer->pVirMemory = (void *)pVirAddr;
            stMemBuffer->pPhyMemory = (void *)pVirAddr;
            stMemBuffer->nMemSize = uBufSize;
            printf("P4_PLAT&T4_PLAT ICF_MEM_MALLOC:size = %lld,va = %p,pa = %p\r\n",uBufSize,stMemBuffer->pVirMemory,stMemBuffer->pPhyMemory);
            return ICF_SUCCESS;     
        }

        case ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE: 
		{
            MB_BLK mb = NULL;
            nRet = RK_MPI_MMZ_Alloc(&mb, uBufSize, RK_MMZ_ALLOC_TYPE_IOMMU|RK_MMZ_ALLOC_CACHEABLE);
            if (nRet < 0)
            {
                printf("alloc ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE memory failed, nRet = %x\n", nRet);
                return ICF_MEMPOOL_ERR_MALLOC;
            }
            pVirAddr = RK_MPI_MMZ_Handle2VirAddr(mb);
            if (NULL == pVirAddr)
            {
                printf("alloc ICF_MEM_MALLOC memory failed, nRet = %x\n", ICF_MEMPOOL_ERR_MALLOC);
                return ICF_MEMPOOL_ERR_MALLOC;
            }
            stMemBuffer->pVirMemory = (void *)pVirAddr;
            stMemBuffer->pPhyMemory = (void *)mb;
            stMemBuffer->nMemSize = uBufSize;
            printf("ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE ICF_SUCCESS,pVirMemory = %p\n",stMemBuffer->pVirMemory);
            return ICF_SUCCESS;
		}
		
        default:
        {
            return ICF_MEMPOOL_ERR_SPACE_TYPE;
        }
    }
    return ICF_SUCCESS;
}

/**********************************************************************************************************************
* 功  能: 引擎底层内存释放接口实现
* 参  数: 
*        pMemory          - I 内存指针
*        nMemSpace        - I 内存类型
* 返回值: 状态码
**********************************************************************************************************************/
int MemPoolSystemFreeCallback(void            *pMemPool, 
                              const ICF_MEM_INFO    *pMemInfo,
                              ICF_MEM_BUFFER  *stMemBuffer)
{
    //int                  nRet = 0;

    void  *pMemory = stMemBuffer->pVirMemory;
    int nMemSpace = pMemInfo->eMemType;
    printf("***************************************************in user's MemPoolSystemFreeCallback function\n");
    printf("nMemSize = %d,pMemory = %p,nMemSpace = %d\r\n",pMemInfo->nVbMemBlkCnt,pMemory,nMemSpace);

	#if (defined HISI_PLAT)
    SYS_VIRMEM_INFO_S       stSrcMmzMemInfo = { 0 };
	#endif

    if (NULL == pMemory)
    {
        printf("MemPoolSystemFree input pMemory = NULL, nRet = %x\n", ICF_MEMPOOL_NULLPOINTER);
    }

    //根据内存块的所处位置来选择释放操作
    switch (nMemSpace)
    {

        case ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE: 
		{
            RK_MPI_MMZ_Free(stMemBuffer->pPhyMemory);
            pMemory = NULL;
            return ICF_SUCCESS;
		}
        
        case ICF_MEM_MALLOC:
        {
            hik_aligned_free(pMemory);
            pMemory = NULL;
            return ICF_SUCCESS;
        }

        default:
        {
            return ICF_MEMPOOL_ERR_SPACE_TYPE;
        }
    }
    return ICF_SUCCESS;
}
#endif
#if 1
/**
 * @function   Ba_HalMemPoolSystemMallocCb
 * @brief      申请系统内存的回调函数
 * @param[in]  void *pInitHandle
 * @param[in]  ICF_MEM_INFO_V2    *pMemInfo
 * @param[in]  ICF_MEM_BUFFER_V2  *stMemBuffer
 * @param[out] None
 * @return     INT32
 */
INT32 Ba_HalMemPoolSystemMallocCb(void *pInitHandle,
                                    ICF_MEM_INFO_V2 *pMemInfo,
                                    ICF_MEM_BUFFER_V2 *pstMemBuffer)
{
    INT32 s32Ret = SAL_FAIL;
    VOID *va = NULL;

    ALLOC_VB_INFO_S stVbInfo = {0};

    switch (pMemInfo->eMemType)
    {
        case ICF_MEM_MALLOC:
        {
            va = SAL_memZalloc(pMemInfo->nMemSize, "ba", "ba_engine");
            if (NULL == va)
            {
                BA_LOGE("malloc failed! \n");
                return SAL_FAIL;
            }

            pstMemBuffer->pVirMemory = (void *)va;
            pstMemBuffer->pPhyMemory = (VOID *)va;
			pstMemBuffer->nMemSize = pMemInfo->nMemSize;
            break;
        }
        case ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE:
        {
            BA_LOGI("RK MEM ALLOC, lBufSize13 = %llu \n", pMemInfo->nMemSize);
            s32Ret = mem_hal_iommuMmzAlloc(pMemInfo->nMemSize, "ba", "ba_engine", NULL, SAL_TRUE, &stVbInfo);
            if (SAL_SOK != s32Ret)
            {
                BA_LOGE("rk cma Malloc err!!\n");
                return SAL_FAIL;
            }

            pstMemBuffer->pVirMemory = (void *)stVbInfo.pVirAddr;
            pstMemBuffer->pPhyMemory = (VOID *)stVbInfo.u64VbBlk;  /* 当前引擎复用物理地址传递MB */
			pstMemBuffer->nMemSize = pMemInfo->nMemSize;

            goto exit;
        }
        default:
        {
            BA_LOGE("invalid mem type %d \n", pMemInfo->eMemType);
            return SAL_FAIL;
        }
    }

exit:
    return SAL_SOK;
}

/**
 * @function   Ba_HalMemPoolSystemFreeCb
 * @brief      释放系统内存的回调函数
 * @param[in]  IA_MEM_TYPE_E enMemType
 * @param[in]  UINT32 u32MemSize
 * @param[in]  IA_MEM_PRM_S *pstMemBuf
 * @param[out] None
 * @return     static INT32
 */
INT32 Ba_HalMemPoolSystemFreeCb(void *pInitHandle,
                                  ICF_MEM_INFO_V2 *pMemInfo,
                                  ICF_MEM_BUFFER_V2 *pstMemBuffer)

{
    INT32 s32Ret = SAL_FAIL;

    switch (pMemInfo->eMemType)
    {
        case ICF_MEM_MALLOC:
        {
            SAL_memfree(pstMemBuffer->pVirMemory, "ba", "ba_engine");
            break;
        }
        case ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE:
        {
            BA_LOGI("RK MEM ALLOC, lBufSize13 = %llu \n", pstMemBuffer->nMemSize);

            /* 当前引擎复用物理地址传递MB */
            s32Ret = mem_hal_iommuMmzFree(pstMemBuffer->nMemSize, "ba", "ba_engine", (UINT64)pstMemBuffer->pPhyMemory, pstMemBuffer->pVirMemory, (UINT64)pstMemBuffer->pPhyMemory);
            if (SAL_SOK != s32Ret)
            {
                BA_LOGE("rk iommu mmz free err!!\n");
                return SAL_FAIL;
            }

            return SAL_SOK;
        }
        default:
        {
            BA_LOGE("invalid mem type %d \n", pMemInfo->eMemType);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}
#endif

/**
 * @function:   Ba_HalIcfInit
 * @brief:      引擎资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalIcfInit(VOID)
{
    INT32 s32Ret = SAL_SOK;
    BA_MOD_S *pstBaModPrm = NULL;
	
    ICF_INIT_PARAM stInitParam = {0};
    ICF_INIT_HANDLE stModHdl = {0};

    UINT32 auiInitMemSize[IA_MEM_TYPE_NUM] = {0};

    (VOID)IA_GetModMemInitSize(IA_MOD_BA, auiInitMemSize);

#ifdef TRACE_LOG
    BA_LOGW("ba: get init mem size %d, %d, %d, %d, %d \n",
            auiInitMemSize[0], auiInitMemSize[1], auiInitMemSize[2], auiInitMemSize[3], auiInitMemSize[4]);
#endif
	
    stInitParam.stConfigInfo.bAlgCfgEncry     = 0;
    stInitParam.stConfigInfo.bTaskCfgEncry    = 0;
    stInitParam.stConfigInfo.bToolkitCfgEncry = 0;		
    stInitParam.stMemConfig.pMemSytemMallocInter  = (void *)Ba_HalMemPoolSystemMallocCb;
    stInitParam.stMemConfig.pMemSytemFreeInter    = (void *)Ba_HalMemPoolSystemFreeCb;

    stInitParam.stConfigInfo.pAlgCfgPath      = (char *)"./ba/config/AlgCfgTest.json";
    stInitParam.stConfigInfo.pTaskCfgPath     = (char *)"./ba/config/TaskCfg.json";
    stInitParam.stConfigInfo.pToolkitCfgPath  = (char *)"./ba/config/ToolkitCfg.json";

    stInitParam.stMemConfig.reserved[0] = 0;                 //内存越界检测
    
    //传入调度器句柄
    stInitParam.pScheduler = IA_GetBaScheHndl();
    BA_HAL_CHECK_PTR(stInitParam.pScheduler, err, "pstBaModPrm == null!");

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

	s32Ret = pstBaModPrm->stIcfFuncP.Ba_IcfInit(&stInitParam, sizeof(stInitParam), &stModHdl,sizeof(ICF_INIT_HANDLE));
    BA_HAL_CHECK_RET(s32Ret, err, "ICF_Init failed!");

    Ba_HalPrint("ICF_Init");
	sal_memcpy_s(&pstBaModPrm->stModHandle, sizeof(ICF_INIT_HANDLE), &stModHdl, sizeof(ICF_INIT_HANDLE));//获取模块初始化句柄结构体

    BA_LOGI("ICF_Init Finish\n");

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalEngineDeInit
 * @brief:      引擎资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalEngineDeInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    for (i = 0; i < pstBaModPrm->uiSubModCnt; i++)
    {
        s32Ret = Ba_HalIcfSubModDeInit(i);
        BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalIcfSubModDeInit failed!");
    }

    s32Ret = Ba_HalIcfDeInit();
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalIcfDeInit failed!");

    if (SAL_SOK != Ia_ResetModMem(IA_MOD_BA))
    {
        BA_LOGE("BA Reset Xsi Mem Fail!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalEngineInit
 * @brief:      引擎资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalEngineInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_PTR(pstBaModPrm, err, "pstBaModPrm == null!");

    s32Ret = IA_InitEncrypt(&g_stBaModPrm.pEncryptHandle);
    BA_HAL_CHECK_RET(s32Ret, err, "IA_InitEncrypt failed!");

    Ba_HalPrint("IA_InitEncrypt");

    s32Ret = IA_InitHwCore();	
    BA_HAL_CHECK_RET(s32Ret, err, "IA_InitHwCore failed!");

    Ba_HalPrint("IA_InitHwCore");

    s32Ret = Ba_HalIcfInit();
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalIcfInit failed!");

    Ba_HalPrint("Ba_HalIcfInit");

    pstBaModPrm->uiSubModCnt = BA_SUB_MOD_NUM;

    for (i = 0; i < pstBaModPrm->uiSubModCnt; i++)
    {
        s32Ret = Ba_HalIcfSubModInit(i);
        BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalIcfSubModInit failed!");

        Ba_HalPrint("Ba_HalIcfSubModInit");
    }
    
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalPrint
 * @brief:      调试信息打印（用于跟踪流程是否走通）
 * @param[in]:  CHAR *pcInfo
 * @param[out]: None
 * @return:     VOID *
 */
VOID *Ba_HalPrint(CHAR *pcInfo)
{
    BA_HAL_CHECK_PTR(pcInfo, exit, "pcInfo == null!");

#ifdef TRACE_LOG
    BA_LOGW("----------- %s --------- \n", pcInfo);
#endif

exit:
    return NULL;
}

/**
 * @function:   Ba_HalFree
 * @brief:      释放一段malloc内存
 * @param[in]:  VOID *p
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ba_HalFree(VOID *p)
{
    BA_HAL_CHECK_PTR(p, exit, "no need free!");

    SAL_memfree(p, "BA", "ba_malloc");
    p = NULL;

exit:
    return SAL_SOK;
}

/**
 * @function:   Ba_HalMalloc
 * @brief:      申请一段malloc内存
 * @param[in]:  VOID **pp
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ba_HalMalloc(VOID **pp, UINT32 uiSize)
{
    if (NULL != *pp)
    {
        BA_LOGI("no need malloc! \n");
        goto exit;
    }

    *pp = SAL_memMalloc(uiSize,"BA", "ba_malloc");
    BA_HAL_CHECK_PTR(*pp, err, "malloc failed!");

    sal_memset_s(*pp, uiSize, 0x00, uiSize);

exit:
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalDebugDumpData
 * @brief:      Ba模块dump数据接口
 * @param[in]:  CHAR *pData
 * @param[in]:  CHAR *pPath
 * @param[in]:  UINT32 uiSize
 * @param[in]:  UINT32 uiStopFlg
 * @param[out]: None
 * @return:     void
 */
void Ba_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiStopFlg)
{
    INT32 ret = 0;
    FILE *fp = NULL;
    CHAR flag[2][16] = {{"w+"}, {"a+"}};

    fp = fopen(pPath, flag[uiStopFlg]);
    if (!fp)
    {
        BA_LOGE("fopen %s failed! \n", pPath);
        goto exit;
    }

    ret = fwrite(pData, uiSize, 1, fp);
    if (ret < 0)
    {
        BA_LOGE("fwrite err! \n");
        goto exit;
    }

    fflush(fp);

exit:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return;
}

/**
 * @function:   Ba_HalSetDbLevel
 * @brief:      设置行为分析日志调试等级
 * @param[in]:  INT32 level
 * @param[out]: None
 * @return:     void
 */
void Ba_HalSetDbLevel(INT32 level)
{
    baDbLevel = (level > 0) ? level : 0;
    BA_LOGI("ba debugLevel %d\n", baDbLevel);

    return;
}

/**
 * @function:   Ba_HalGetDbLevel
 * @brief:      获取行为分析调试日志等级
 * @param[in]:  void
 * @param[out]: None
 * @return:     UINT32
 */
UINT32 Ba_HalGetDbLevel(void)
{
    return baDbLevel;
}

/*******************************************************************************
* 函数名  : Ba_HalGetDev
* 描  述  : 获取算法结果全局变量
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
BA_MOD_S *Ba_HalGetXbaCommon(void)
{
    /* converity检测到此处必然不为空，所以后面不需要加return NULL */
    return &g_stBaModPrm;
}

/**
 * @function:   Sva_HalInitAlgApi
 * @brief:      加载引擎应用层动态库符号(libalg.so)
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ba_HalInitAlgApi(VOID)
{
    INT32 s32Ret = SAL_SOK;
    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_RETURN(pstBaModPrm == NULL, SAL_FAIL);

    s32Ret = Sal_GetLibHandle("libSbaeAlg.so", &pstBaModPrm->pIcfAlgHandle);

    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    /* ICF_Interface.h */
    //s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfAlgHandle, "ICF_APP_GetVersion_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfAppGetVersion);
    //BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    return SAL_SOK;
}


static INT32 Ba_HalInitIcfApi(VOID)
{
    INT32 s32Ret = SAL_SOK;
    BA_MOD_S *pstBaModPrm = NULL;

    pstBaModPrm = Ba_HalGetModPrm();
    BA_HAL_CHECK_RETURN(pstBaModPrm == NULL, SAL_FAIL);

	if (NULL != pstBaModPrm->pIcfLibHandle)
	{
		BA_LOGI("ba icf lib handle has been created! return success! \n");
		return SAL_SOK;
	}

    s32Ret = Sal_GetLibHandle("libicf.so", &pstBaModPrm->pIcfLibHandle);

    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    /* ICF_Interface.h */
    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_Init_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfInit);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_Finit_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfFinit);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

	s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_LoadModel_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfLoadModel);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

	s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_UnloadModel_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfUnloadModel);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_Create_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfCreate);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_Destroy_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfDestroy);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_SetConfig_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfSetConfig);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_GetConfig_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfGetConfig);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_SetCallback_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfSetCallback);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_GetVersion_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfGetVersion);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_InputData_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfInputData);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    //s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_GetRuntimeLimit", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfGetRunTimeLimit);
    //BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    /* ICF_toolkit.h */
    //s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_SetCoreAffinity", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfSetCoreAffinity);
    //BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    //s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_GetMemPoolStatus", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfGetMemPoolStatus);
    //BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_Package_GetState_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfGetPackageStatus);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "ICF_GetDataPtrFromPkg_V2", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfGetPackageDataPtr);
    BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    /* ICF_mempool.h */
    //s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "MemPoolAllocInter", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfMemPoolAlloc);
    //BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    //s32Ret = Sal_GetLibSymbol(pstBaModPrm->pIcfLibHandle, "MemPoolFreeInter", (VOID **)&pstBaModPrm->stIcfFuncP.Ba_IcfMemPoolFree);
    
    //BA_HAL_CHECK_RETURN(s32Ret != SAL_SOK, SAL_FAIL);

    return SAL_SOK;
}


/**
 * @function:   Ba_HalInitRtld
 * @brief:      ba模块加载动态库符号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalInitRtld(VOID)
{
    INT32 s32Ret = SAL_SOK;

    /* libicf.so */
    s32Ret = Ba_HalInitIcfApi();
    BA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    /* libSbaeAlg.so */
    //s32Ret = Ba_HalInitAlgApi();
    //BA_HAL_CHECK_RETURN(s32Ret, SAL_FAIL);

    SAL_INFO("init run time loader end! \n");
    return SAL_SOK;
}


/**
 * @function:   Ba_HalInit
 * @brief:      HAL层资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalInit(VOID)
{
    INT32 s32Ret = SAL_SOK;
    if (g_stBaModPrm.uiInitFlag)
    {
        SAL_WARN("Ba hal is initialized! return Success! \n");
        return SAL_SOK;
    }

    s32Ret = Ba_HalEngineInit();
    BA_HAL_CHECK_RET(s32Ret, err, "Ba_HalEngineInit failed!");

    g_stBaModPrm.uiInitFlag = SAL_TRUE;
    return SAL_SOK;

err:
    (VOID)Ba_HalEngineDeInit();
    return SAL_FAIL;
}

/**
 * @function:   Ba_HalDeinit
 * @brief:      HAL层资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_HalDeinit(VOID)
{
    if (!g_stBaModPrm.uiInitFlag)
    {
        SAL_WARN("BA hal is deinitialized! return Success! \n");
        return SAL_SOK;
    }

    (VOID)Ba_HalEngineDeInit();

    g_stBaModPrm.uiInitFlag = SAL_FALSE;
    return SAL_SOK;
}


