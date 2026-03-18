/**
 * @file    dspttk_devcfg.c
 * @brief   DSP组件Web测试工具集配置参数
 * @note    Bin配置文件由Json文件解析生成，Json文件中的Value导入DSPTTK_DEVCFG对应的参数中
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_fop.h"
#include "dspttk_cjson_wrap.h"
#include "dspttk_devcfg.h"
#include "dspttk_util.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define DSPTTK_DEVCFG_PATH "devcfg/devcfg.bin"        // 配置文件默认路径
#define DSPTTK_DEFAULT_DEV_JSON "devcfg/SC6550D.json" // 若配置文件不存在，默认生成配置的Json文件


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
static DSPTTK_DEVCFG gstDevCfg;
pthread_mutex_t gMutexDevcfg; // 读写DSP TTK的配置参数的互斥锁
pthread_t gOprDirId = 0;

// 全局状态信息
DSPTTK_GLOBAL_STATUS gstGlobalStatus = {
    .enRunStatus = DSPTTK_RUNSTAT_READY,
    .enRunStatus = DSPTTK_STATUS_NONE,
    .stPbInfo.stPbPack.pu8PbSpliceDataDst = NULL,
    .stPbInfo.stPbPack.pu8PbSpliceDataSrc = NULL,
};


/**
 * @fn      dspttk_devcfg_get
 * @brief   获取DSP TTK的配置文件
 *
 * @return  DSPTTK_DEVCFG* 配置文件Buffer地址
 */
DSPTTK_DEVCFG *dspttk_devcfg_get(void)
{
    return &gstDevCfg;
}


/**
 * @fn      dspttk_devcfg_save
 * @brief   保存DSP TTK的配置文件
 * @warning 修改配置文件中参数与调用该接口前，均需要加锁gMutexDevcfg
 *
 * @return  SAL_STATUS SAL_SOK：保存成功，SAL_FAIL：失败
 */
SAL_STATUS dspttk_devcfg_save(void)
{
    SAL_STATUS sRet = SAL_FAIL;

    sRet = dspttk_write_file(DSPTTK_DEVCFG_PATH, 0, SEEK_SET, &gstDevCfg, sizeof(DSPTTK_DEVCFG));
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_write_file '%s' failed\n", DSPTTK_DEVCFG_PATH);
    }

    return sRet;
}


/**
 * @fn     dspttk_devcfg_read
 * @brief  读取DSP TTK的配置文件
 *
 * @return SAL_STATUS SAL_SOK：读取成功，SAL_FAIL：失败
 */
static SAL_STATUS dspttk_devcfg_read(void)
{
    SAL_STATUS sRet = SAL_FAIL;
    UINT32 u32Size = sizeof(DSPTTK_DEVCFG);

    sRet = dspttk_read_file(DSPTTK_DEVCFG_PATH, 0, &gstDevCfg, &u32Size);
    if (SAL_SOK != sRet || sizeof(DSPTTK_DEVCFG) != u32Size)
    {
        PRT_INFO("dspttk_read_file '%s' failed, ret: %d, actual size: %u, expect: %zu\n",
                 DSPTTK_DEVCFG_PATH, sRet, u32Size, sizeof(DSPTTK_DEVCFG));
        sRet = SAL_FAIL;
    }

    return sRet;
}


/**
 * @fn      devcfg_load_init_common
 * @brief   获取配置文件Initialization中common的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init_common(CJSON *pstJsonInit)
{
    INT32 s32JsonType = 0;
    CJSON *pstJsonComm = NULL;
    CHAR *pString = NULL;

    if (NULL == pstJsonInit)
    {
        PRT_INFO("the pstJsonInit is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonInit, "common", &pstJsonComm);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonComm)
    {
        PRT_INFO("dspttk_cjson_get_object 'common' failed, s32JsonType: %d, pstJsonComm: %p\n", s32JsonType, pstJsonComm);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonComm, "cfgVersion", &pString);
    if (s32JsonType == CJSON_STRING && NULL != pString)
    {
        snprintf(gstDevCfg.stInitParam.stCommon.cfgVersion, sizeof(gstDevCfg.stInitParam.stCommon.cfgVersion), "%s", pString);
        pString = NULL; // 重置为NULL
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'cfgVersion' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonComm, "boardType", &pString);
    if (s32JsonType == CJSON_STRING && NULL != pString)
    {
        snprintf(gstDevCfg.stInitParam.stCommon.boardType, sizeof(gstDevCfg.stInitParam.stCommon.boardType), "%s", pString);
        pString = NULL; // 重置为NULL
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'boardType' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      devcfg_load_init_xray
 * @brief   获取配置文件Initialization中xray的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init_xray(CJSON *pstJsonInit)
{
    long lNum = 0;
    char sName[16];
    UINT32 u32Chan = 0;
    FLOAT64 f64Num = 0;
    INT32 s32JsonType = 0, j = 0, i = 0, s32ArrLen = 0, s32SonLen = 0;
    CJSON *pstJsonXray = NULL, *pstJsonOutArry = NULL, *pstJsonRangeArry = NULL, *pstJsonInArry = NULL, *pstJsonInArryval = NULL;

    DSPTTK_INIT_XRAY *pstXray = &gstDevCfg.stInitParam.stXray;
    if (NULL == pstJsonInit)
    {
        PRT_INFO("the pstJsonInit is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonInit, "xray", &pstJsonXray);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonXray)
    {
        PRT_INFO("dspttk_cjson_get_object 'xray' failed, s32JsonType: %d, pstJsonXray: %p\n", s32JsonType, pstJsonXray);
        return SAL_FAIL;
    }

    dspttk_cjson_get_value(pstJsonXray, NULL, "xrayChanCnt", CJSON_LONG, &pstXray->xrayChanCnt, 0); // XRay射线源数量
    dspttk_cjson_get_value(pstJsonXray, "xrayDetVendor", "value", CJSON_STRING, pstXray->xrayDetVendor, sizeof(pstXray->xrayDetVendor)); // 探测板
    dspttk_cjson_get_value(pstJsonXray, NULL, "xrayEnergy", CJSON_STRING, pstXray->xrayEnergy, sizeof(pstXray->xrayEnergy)); // 单双能
    dspttk_cjson_get_value(pstJsonXray, "xspLibSrc", "value", CJSON_LONG, &pstXray->enXspLibSrc, 0); // XSP算法库
    dspttk_cjson_get_value(pstJsonXray, NULL, "xrayWidthMax", CJSON_LONG, &pstXray->xrayWidthMax, 0); // XRaw宽
    dspttk_cjson_get_value(pstJsonXray, NULL, "xrayHeightMax", CJSON_LONG, &pstXray->xrayHeightMax, 0); // XRaw高
    dspttk_cjson_get_value(pstJsonXray, NULL, "resizeFactor", CJSON_DOUBLE, &pstXray->resizeFactor, 0); // XSP超分系数
    dspttk_cjson_get_value(pstJsonXray, NULL, "packageLenMax", CJSON_LONG, &pstXray->packageLenMax, 0); // 包裹强制分割长度
    dspttk_cjson_get_value(pstJsonXray, "xraySourceType0", "value", CJSON_STRING, pstXray->xraySourceType[0], sizeof(pstXray->xraySourceType[0])); // 主源型号
    if (pstXray->xrayChanCnt > 1)
    {
        dspttk_cjson_get_value(pstJsonXray, "xraySourceType1", "value", CJSON_STRING, pstXray->xraySourceType[1], sizeof(pstXray->xraySourceType[1])); // 侧源型号
    }

    /* 填充射线源范围 */
    for ( u32Chan = 0; u32Chan < pstXray->xrayChanCnt; u32Chan++)
    {
        snprintf(sName, sizeof(sName), "xraySourceType%d", u32Chan);
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, sName, &pstJsonOutArry);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonOutArry)
        {
            PRT_INFO("dspttk_cjson_get_object '%s' failed, s32JsonType: %d, pstJsonOutArry: %p\n",sName, s32JsonType, pstJsonOutArry);
            return SAL_FAIL;
        }
        s32JsonType = dspttk_cjson_get_object(pstJsonOutArry, "range", &pstJsonRangeArry);
        if (s32JsonType == CJSON_ARRAY && NULL != pstJsonRangeArry)
        {
            for (i = 0; i < dspttk_cjson_get_array_size(pstJsonRangeArry); i++)
            {
                pstJsonInArry = dspttk_cjson_get_array_item(pstJsonRangeArry, i);
                if (pstJsonInArry != NULL)
                {
                    strcpy(pstXray->szXraySrcTypeRang[u32Chan][i], pstJsonInArry->valuestring);
                }
            }
        }
    }

    /* 过包速度、积分时间、条带高度、显示帧率等与速度相关的参数 */
    s32JsonType = dspttk_cjson_get_object(pstJsonXray, "xraySpeed", &pstJsonOutArry);
    if (s32JsonType != CJSON_ARRAY || NULL == pstJsonOutArry)
    {
        PRT_INFO("dspttk_cjson_get_object 'xraySpeed' failed, s32JsonType: %d, pstJsonOutArry: %p\n", s32JsonType, pstJsonOutArry);
        return SAL_FAIL;
    }
    s32ArrLen = dspttk_cjson_get_array_size(pstJsonOutArry); // 获取外围数组大小
    if (s32ArrLen >= XRAY_FORM_NUM)
    {
        s32ArrLen = XRAY_FORM_NUM;
    }
    else if (s32ArrLen <= 0)
    {
        PRT_INFO("dspttk_cjson_get_array_size 'pstJsonOutArry' failed, s32ArrLen: %d, pstJsonOutArry: %p\n", s32ArrLen, pstJsonOutArry);
        return SAL_FAIL;
    }

    for (i = 0; i < s32ArrLen; i++)
    {
        pstJsonInArry = dspttk_cjson_get_array_item(pstJsonOutArry, i);
        if (NULL == pstJsonInArry)
        {
            PRT_INFO("dspttk_cjson_get_array_item failed, i: %d, pstJsonOutArry: %p\n", i, pstJsonOutArry);
            return SAL_FAIL;
        }
        s32SonLen = dspttk_cjson_get_array_size(pstJsonInArry); // 获取子数组大小
        if (s32SonLen >= XRAY_SPEED_NUM)
        {
            s32SonLen = XRAY_SPEED_NUM;
        }
        else if (s32SonLen <= 0)
        {
            PRT_INFO("dspttk_cjson_get_array_size 'pstJsonInArry' failed, s32SonLen: %d, pstJsonInArry: %p\n", s32SonLen, pstJsonInArry);
            return SAL_FAIL;
        }

        for (j = 0; j < s32SonLen; j++)
        {
            pstJsonInArryval = dspttk_cjson_get_array_item(pstJsonInArry, j);
            if (pstJsonInArryval == NULL)
            {
                PRT_INFO("dspttk_cjson_get_array_item failed, j: %d pstJsonOutArry: %p\n", j, pstJsonInArry);
                return SAL_FAIL;
            }

            s32JsonType = dspttk_cjson_get_object(pstJsonInArryval, "integralTime", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stInitParam.stXray.stSpeed[i][j].integralTime = (UINT32)lNum;
                lNum = 0; // 重置为0
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'integralTime' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }

            s32JsonType = dspttk_cjson_get_object(pstJsonInArryval, "sliceHeight", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stInitParam.stXray.stSpeed[i][j].sliceHeight = (UINT32)lNum;
                lNum = 0; // 重置为0
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'sliceHeight' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }

            s32JsonType = dspttk_cjson_get_object(pstJsonInArryval, "beltSpeed", &f64Num);
            if (s32JsonType == CJSON_DOUBLE)
            {
                gstDevCfg.stInitParam.stXray.stSpeed[i][j].beltSpeed = (FLOAT32)f64Num;
                f64Num = 0; // 重置为0
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'beltSpeed' failed, s32JsonType: %d, lNum: %lf i %d j %d\n", s32JsonType, f64Num, i, j);
                return SAL_FAIL;
            }

            s32JsonType = dspttk_cjson_get_object(pstJsonInArryval, "dispfps", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stInitParam.stXray.stSpeed[i][j].dispfps = (UINT32)lNum;
                lNum = 0; // 重置为0
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'dispfps' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_init_display
 * @brief   获取配置文件Initialization中display的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init_display(CJSON *pstJsonInit)
{
    INT32 s32JsonType = 0;
    INT32 i = 0;
    INT32 s32ArrLen = 0;
    long lNum = 0;
    BOOL bEnable = SAL_FALSE;
    CJSON *pstJsondisplay = NULL;
    CJSON *pstJsonOutArry = NULL;
    CJSON *pstJsonOutArryVal = NULL;
    CHAR *pString = NULL;

    if (NULL == pstJsonInit)
    {
        PRT_INFO("the pstJsonInit is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonInit, "display", &pstJsondisplay);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsondisplay)
    {
        PRT_INFO("dspttk_cjson_get_object 'display' failed, s32JsonType: %d, pstJsondisplay: %p\n", s32JsonType, pstJsondisplay);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondisplay, "voDevCnt", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stDisplay.voDevCnt = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'voDevCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondisplay, "paddingTop", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stDisplay.paddingTop = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'paddingTop' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondisplay, "paddingBottom", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stDisplay.paddingBottom = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'paddingBottom' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondisplay, "blankingTop", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stDisplay.blankingTop = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'blankingTop' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondisplay, "blankingBottom", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stDisplay.blankingBottom = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'blankingBottom' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondisplay, "voDevDispAttr", &pstJsonOutArry);
    if (s32JsonType != CJSON_ARRAY || NULL == pstJsonOutArry)
    {
        PRT_INFO("dspttk_cjson_get_object 'voDevDispAttr' failed, s32JsonType: %d, pstJsonOutArry: %p\n", s32JsonType, pstJsonOutArry);
        return SAL_FAIL;
    }

    s32ArrLen = dspttk_cjson_get_array_size(pstJsonOutArry); // 获取外围数组大小
    if (s32ArrLen >= 2)
    {
        s32ArrLen = 2;
    }
    else if (s32ArrLen <= 0)
    {
        PRT_INFO("dspttk_cjson_get_array_size 'pstJsonOutArry' failed, s32ArrLen: %d, pstJsonOutArry: %p\n", s32ArrLen, pstJsonOutArry);
        return SAL_FAIL;
    }

    for (i = 0; i < s32ArrLen; i++)
    {
        pstJsonOutArryVal = dspttk_cjson_get_array_item(pstJsonOutArry, i);
        if (NULL == pstJsonOutArryVal)
        {
            PRT_INFO("dspttk_cjson_get_array_item failed, i: %d, pstJsonOutArry: %p\n", i, pstJsonOutArry);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "enable", &bEnable);
        if (s32JsonType == CJSON_BOOLEAN)
        {
            gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].enable = bEnable;
            bEnable = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enable' failed, s32JsonType: %d, lbNum: %d\n", s32JsonType, bEnable);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "voChnCnt", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].voChnCnt = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'voChnCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "voInterface", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].voInterface, sizeof(gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].voInterface), "%s", pString);
            pString = NULL; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'voInterface' failed, s32JsonType: %d, pString: %s\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "width", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].width = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'width' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "height", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].height = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'height' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "fpsDefault", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stDisplay.stVoDevAttr[i].fpsDefault = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'fpsDefault' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_init_enc_dec
 * @brief   获取配置文件Initialization中enc_dec的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init_enc_dec(CJSON *pstJsonInit)
{
    INT32 s32JsonType = 0;
    INT32 i = 0;
    INT32 s32ArrLen = 0;
    long lNum = 0;
    CJSON *pstJsondEncDec = NULL;
    CJSON *pstJsonOutArry = NULL;
    CJSON *pstJsonOutArryVal = NULL;

    if (NULL == pstJsonInit)
    {
        PRT_INFO("the pstJsonInit is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonInit, "enc_dec", &pstJsondEncDec);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsondEncDec)
    {
        PRT_INFO("dspttk_cjson_get_object 'enc_dec' failed, s32JsonType: %d, pstJsonEncDec: %p\n", s32JsonType, pstJsondEncDec);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "encChanCnt", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stEncDec.encChanCnt = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'encChanCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "encRecPsBufLen", &pstJsonOutArry);
    if (s32JsonType != CJSON_ARRAY || NULL == pstJsonOutArry)
    {
        PRT_INFO("dspttk_cjson_get_object 'encRecPsBufLen' failed, s32JsonType: %d, pstJsonOutArry: %p\n", s32JsonType, pstJsonOutArry);
        return SAL_FAIL;
    }

    s32ArrLen = dspttk_cjson_get_array_size(pstJsonOutArry); // 获取外围数组大小
    if (s32ArrLen >= MAX_VENC_CHAN)
    {
        s32ArrLen = MAX_VENC_CHAN;
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_array_size 'pstJsonOutArry' failed, s32ArrLen: %d, pstJsonOutArry: %p\n", s32ArrLen, pstJsonOutArry);
        return SAL_FAIL;
    }

    for (i = 0; i < s32ArrLen; i++)
    {
        pstJsonOutArryVal = dspttk_cjson_get_array_item(pstJsonOutArry, i);
        if (NULL == pstJsonOutArryVal)
        {
            PRT_INFO("dspttk_cjson_get_array_item failed, i: %d, pstJsonOutArry: %p\n", i, pstJsonOutArry);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "main", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.encRecPsBufLen[i][0] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'main' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "sub", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.encRecPsBufLen[i][1] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'sub' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "third", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.encRecPsBufLen[i][2] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'third' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "encNetRtpBufLen", &pstJsonOutArry);
    if (s32JsonType != CJSON_ARRAY || NULL == pstJsonOutArry)
    {
        PRT_INFO("dspttk_cjson_get_object 'encNetRtpBufLen' failed, s32JsonType: %d, pstJsonOutArry: %p\n", s32JsonType, pstJsonOutArry);
        return SAL_FAIL;
    }

    s32ArrLen = dspttk_cjson_get_array_size(pstJsonOutArry); // 获取外围数组大小
    if (s32ArrLen >= MAX_VENC_CHAN)
    {
        s32ArrLen = MAX_VENC_CHAN;
    }
    else if (s32ArrLen <= 0)
    {
        PRT_INFO("dspttk_cjson_get_array_size 'pstJsonOutArry' failed, s32ArrLen: %d, pstJsonOutArry: %p\n", s32ArrLen, pstJsonOutArry);
        return SAL_FAIL;
    }

    for (i = 0; i < s32ArrLen; i++)
    {
        pstJsonOutArryVal = dspttk_cjson_get_array_item(pstJsonOutArry, i);
        if (NULL == pstJsonOutArryVal)
        {
            PRT_INFO("dspttk_cjson_get_array_item failed, i: %d, pstJsonOutArry: %p\n", i, pstJsonOutArry);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "main", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.encNetRtpBufLen[i][0] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'main' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "sub", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.encNetRtpBufLen[i][1] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'sub' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "decChanCnt", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stEncDec.decChanCnt = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'decChanCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "decBufLen", &pstJsonOutArry);
    if (s32JsonType != CJSON_ARRAY || NULL == pstJsonOutArry)
    {
        PRT_INFO("dspttk_cjson_get_object 'decBufLen' failed, s32JsonType: %d, pstJsonOutArry: %p\n", s32JsonType, pstJsonOutArry);
        return SAL_FAIL;
    }

    s32ArrLen = dspttk_cjson_get_array_size(pstJsonOutArry); // 获取外围数组大小
    if (s32ArrLen >= MAX_VDEC_CHAN)
    {
        s32ArrLen = MAX_VDEC_CHAN;
    }
    else if (s32ArrLen <= 0)
    {
        PRT_INFO("dspttk_cjson_get_array_size 'pstJsonOutArry' failed, s32ArrLen: %d, pstJsonOutArry: %p\n", s32ArrLen, pstJsonOutArry);
        return SAL_FAIL;
    }

    for (i = 0; i < s32ArrLen; i++)
    {
        pstJsonOutArryVal = dspttk_cjson_get_array_item(pstJsonOutArry, i);
        if (NULL == pstJsonOutArryVal)
        {
            PRT_INFO("dspttk_cjson_get_array_item failed, i: %d, pstJsonOutArry: %p\n", i, pstJsonOutArry);
            return SAL_FAIL;
        }

        gstDevCfg.stInitParam.stEncDec.decBufLen[i] = pstJsonOutArryVal->valuelong;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "ipcStreamPackTransChanCnt", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stEncDec.ipcStreamPackTransChanCnt = (UINT32)lNum;
        lNum = 0; // 重置为0
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'ipcStreamPackTransChanCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsondEncDec, "ipcStreamPackTransBufLen", &pstJsonOutArry);
    if (s32JsonType != CJSON_ARRAY || NULL == pstJsonOutArry)
    {
        PRT_INFO("dspttk_cjson_get_object 'ipcStreamPackTransBufLen' failed, s32JsonType: %d, pstJsonOutArry: %p\n", s32JsonType, pstJsonOutArry);
        return SAL_FAIL;
    }

    s32ArrLen = dspttk_cjson_get_array_size(pstJsonOutArry); // 获取外围数组大小
    if (s32ArrLen >= MAX_VIPC_CHAN)
    {
        s32ArrLen = MAX_VIPC_CHAN;
    }
    else if (s32ArrLen <= 0)
    {
        PRT_INFO("dspttk_cjson_get_array_size 'pstJsonOutArry' failed, s32ArrLen: %d, pstJsonOutArry: %p\n", s32ArrLen, pstJsonOutArry);
        return SAL_FAIL;
    }

    for (i = 0; i < s32ArrLen; i++)
    {
        pstJsonOutArryVal = dspttk_cjson_get_array_item(pstJsonOutArry, i);
        if (NULL == pstJsonOutArryVal)
        {
            PRT_INFO("dspttk_cjson_get_array_item failed, i: %d, pstJsonOutArry: %p\n", i, pstJsonOutArry);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "in", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.ipcStreamPackTransBufLen[i][0] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'in' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "outRecPs", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.ipcStreamPackTransBufLen[i][1] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'outRecPs' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOutArryVal, "outNetRtp", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stInitParam.stEncDec.ipcStreamPackTransBufLen[i][2] = (UINT32)lNum;
            lNum = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'outNetRtp' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_init_sva
 * @brief   获取配置文件Initialization中sva的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init_sva(CJSON *pstJsonInit)
{
    INT32 s32JsonType = 0, i = 0;
    CJSON *pstJsonSva = NULL;
    CJSON *pstSvaDevChipType = NULL;
    CHAR *pString = NULL;
    struct {
        DEVICE_CHIP_TYPE_E enChip;
        CHAR sJson[32]; // Json配置文件中的字符串
    } astChipType[] = {
        {SINGLE_CHIP_TYPE, "Single"},
        {DOUBLE_CHIP_MASTER_TYPE, "Double-Master"},
        {DOUBLE_CHIP_SLAVE_TYPE, "Double-Slave"}
    };

    if (NULL == pstJsonInit)
    {
        PRT_INFO("the pstJsonInit is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonInit, "sva", &pstJsonSva);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonSva)
    {
        PRT_INFO("dspttk_cjson_get_object 'sva' failed, s32JsonType: %d, pstJsonSva: %p\n", s32JsonType, pstJsonSva);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonSva, "devChipType", &pstSvaDevChipType);
    if (s32JsonType != CJSON_OBJECT || NULL == pstSvaDevChipType)
    {
        PRT_INFO("dspttk_cjson_get_object 'devChipType' failed, s32JsonType: %d, pstSvaDevChipType: %p\n", s32JsonType, pstSvaDevChipType);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstSvaDevChipType, "value", &pString);
    if (s32JsonType == CJSON_STRING && NULL != pString)
    {
        for (i = 0; i < SAL_arraySize(astChipType); i++)
        {
            if (0 == strcmp(astChipType[i].sJson, pString))
            {
                gstDevCfg.stInitParam.stSva.enDevChipType = astChipType[i].enChip;
                break;
            }
        }
        pString = NULL; // 重置为NULL
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'value' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_init_audio
 * @brief   获取配置文件Initialization中audio的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init_audio(CJSON *pstJsonInit)
{
    INT32 s32JsonType = 0;
    CJSON *pstJsonAudio = NULL;
    long lNum = 0;

    if (NULL == pstJsonInit)
    {
        PRT_INFO("the pstJsonInit is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonInit, "audio", &pstJsonAudio);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonAudio)
    {
        PRT_INFO("dspttk_cjson_get_object 'audio' failed, s32JsonType: %d, pstJsonAudio: %p\n", s32JsonType, pstJsonAudio);
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonAudio, "aoDevCnt", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stInitParam.stAudio.aoDevCnt = (INT32)lNum;
        lNum = 0; // 重置为0；
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'aoDevCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_init
 * @brief   加载DSP TTK的配置文件Initialization的参数
 *
 * @param   [IN] pstJsonInit 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：加载成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_init(CJSON *pstJsonInit)
{
    SAL_STATUS sRet = SAL_SOK;
    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    sRet |= devcfg_load_init_common(pstJsonInit);
    sRet |= devcfg_load_init_xray(pstJsonInit);
    sRet |= devcfg_load_init_display(pstJsonInit);
    sRet |= devcfg_load_init_enc_dec(pstJsonInit);
    sRet |= devcfg_load_init_sva(pstJsonInit);
    sRet |= devcfg_load_init_audio(pstJsonInit);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    return sRet;
}

/**
 * @fn      devcfg_load_setting_env
 * @brief   获取配置文件Initialization中env的参数
 *
 * @param   [IN] pstJsonSetting 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_setting_env(CJSON *pstJsonSetting)
{
    INT32 s32JsonType = 0;
    long lNum = 0;
    CHAR *pString = NULL;
    CJSON *pstJsonEnv = NULL;
    CJSON *pstJsonInputArry = NULL;
    CJSON *pstJsonOutputArry = NULL;

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    /*json中evn参数本地化*/
    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "env", &pstJsonEnv);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonEnv)
    {
        PRT_INFO("dspttk_cjson_get_object 'env' failed, s32JsonType: %d, pstJsonEnv: %p\n", s32JsonType, pstJsonEnv);
        return SAL_FAIL;
    }

    /* 1.获取Runtime Local Directory */
    s32JsonType = dspttk_cjson_get_object(pstJsonEnv, "runtimeDir", &pString);
    if (s32JsonType == CJSON_STRING && NULL != pString)
    {
        snprintf(gstDevCfg.stSettingParam.stEnv.runtimeDir, sizeof(gstDevCfg.stSettingParam.stEnv.runtimeDir), "%s", pString);
        pString = NULL; // 重置为NULL
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'runtimeDir' failed, s32JsonType: %d, RunTimeDir: %s\n", s32JsonType, pString);
        return SAL_FAIL;
    }

    /* 2.获取Input Data Directory */
    s32JsonType = dspttk_cjson_get_object(pstJsonEnv, "inputDir", &pstJsonInputArry);
    if (s32JsonType == CJSON_OBJECT && NULL != pstJsonInputArry)
    {
        /* XRay Raw path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "rtXraw", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.rtXraw, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.rtXraw), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'rtXraw' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* Tip Raw path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "tipNraw", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.tipNraw, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.tipNraw), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'tipNraw' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* IPC Stream path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "ipcStream", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.ipcStream, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.ipcStream), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'ipcStream' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* AI Model path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "aiModel", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.aiModel, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.aiModel), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'aiModel' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* gui path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "gui", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.gui, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.gui), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'gui' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* Alarm Audio path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "alramAudio", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.alramAudio, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.alramAudio), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'alramAudio' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* noSignalImg path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "noSignalImg", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.noSignalImg, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.noSignalImg), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'noSignalImg' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* font path */
        s32JsonType = dspttk_cjson_get_object(pstJsonInputArry, "font", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stInputDir.font, sizeof(gstDevCfg.stSettingParam.stEnv.stInputDir.font), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'font' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'inputDir' failed, s32JsonType: %d, inputDir: %p\n", s32JsonType, pstJsonInputArry);
        return SAL_FAIL;
    }

    /* 3.获取Output Data Directory */
    s32JsonType = dspttk_cjson_get_object(pstJsonEnv, "outputDir", &pstJsonOutputArry);
    if (s32JsonType == CJSON_OBJECT && NULL != pstJsonOutputArry)
    {
        /*Raw Slice path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "sliceNraw", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.sliceNraw, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.sliceNraw), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'sliceNraw' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* 包裹JPG path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "packageImg", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.packageImg, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.packageImg), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'packageImg' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /*Save path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "saveScreen", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.saveScreen, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.saveScreen), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'saveScreen' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /*包裹分片 path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "packageSeg", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.packageSeg, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.packageSeg), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'packageSeg' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /*编码 path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "encStream", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.encStream, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.encStream), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'encStream' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* 包裹转存 path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "packageTrans", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.packageTrans, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.packageTrans), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'packageTrans' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* 转封装 path*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "streamTrans", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.streamTrans, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.streamTrans), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'streamTrans' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }

        /* xraw 回调保存路径*/
        s32JsonType = dspttk_cjson_get_object(pstJsonOutputArry, "xrawStore", &pString);
        if (s32JsonType == CJSON_STRING && NULL != pString)
        {
            snprintf(gstDevCfg.stSettingParam.stEnv.stOutputDir.xrawStore, sizeof(gstDevCfg.stSettingParam.stEnv.stOutputDir.xrawStore), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'xrawStore' failed, s32JsonType: %d, pString: %p\n", s32JsonType, pString);
            return SAL_FAIL;
        }
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'outputDir' failed, s32JsonType: %d, outputDir: %p\n", s32JsonType, pstJsonOutputArry);
        return SAL_FAIL;
    }

    /* 4.获取实时过包的xraw数据默认是否循环 */
    s32JsonType = dspttk_cjson_get_object(pstJsonEnv, "bRtXrawLoop", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stSettingParam.stEnv.bInputDirRtLoop =  (INT32)lNum;
        lNum = 0; //重置为0；
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'bInputDirRtLoop' failed, s32JsonType: %d, bInputDirRtLoop: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }

    /* 5.获取输出路径默认是否删除 */
    s32JsonType = dspttk_cjson_get_object(pstJsonEnv, "bOutputDirClear", &lNum);
    if (s32JsonType == CJSON_LONG)
    {
        gstDevCfg.stSettingParam.stEnv.bOutputDirAutoClear =  (INT32)lNum;
        lNum = 0; //重置为0；
    }
    else
    {
        PRT_INFO("dspttk_cjson_get_object 'bOutputDirAutoClear' failed, s32JsonType: %d, bOutputDirAutoClear: %ld\n", s32JsonType, lNum);
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_xray
 * @brief   获取配置文件Setting中xray的参数
 *
 * @param   [IN] pstJsonSetting 配置文件Setting栏xray参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_setting_xray(CJSON *pstJsonSetting)
{
    INT32 s32JsonType = 0;
    long lNum = 0;
    CJSON *pstJsonXray = NULL;
    CJSON *pstJsonXrayTransType = NULL;
    CJSON *pstJsonXrayRmSliceBlank = NULL;
    CJSON *pstJsonXrayTip = NULL;
    DSPTTK_DEVCFG *pstDevCfenvg = &gstDevCfg;
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfenvg->stSettingParam.stXray;

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    /*json中xray参数本地化*/
    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "xray", &pstJsonXray);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonXray)
    {
        PRT_INFO("dspttk_cjson_get_object 'xray' failed, s32JsonType: %d, pstJsonXray: %p\n",s32JsonType, pstJsonXray);
        return SAL_FAIL;
    }
    else
    {
        /* 1.获取speed */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "speed", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stSpeedParam.enSpeedUsr = (XRAY_PROC_SPEED)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'aoDevCnt' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 2.获取form */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "form", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stFillinBlank[1].marginTop =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 3.获取dispDir */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "dispDir", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->enDispDir =  (XRAY_PROC_DIRECTION)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enDispDir' failed, s32JsonType: %d, enDispDir: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 4.获取enhancedScan */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enhancedScan", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stEnhancedScan.bEnable =  (INT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stEnhancedScan.bEnable' failed, s32JsonType: %d, stEnhancedScan.bEnable: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 5.获取verticalMirrorCh0 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "verticalMirrorCh0", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stVMirror[0].bEnable =  (INT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stVMirror[0].bEnable' failed, s32JsonType: %d, stVMirror[0].bEnable: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 6.获取verticalMirrorCh1 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "verticalMirrorCh1", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stVMirror[1].bEnable =  (INT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stVMirror[1].bEnable' failed, s32JsonType: %d, stVMirror[1].bEnable: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 7.获取fillBlankLeftCh0*/
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "fillBlankLeftCh0", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stFillinBlank[0].marginTop =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[0].marginTop' failed, s32JsonType: %d, stFillinBlank[0].marginTop: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 8.获取fillBlankRightCh0*/
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "fillBlankRightCh0", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stFillinBlank[0].marginBottom =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[0].marginBottom' failed, s32JsonType: %d, stFillinBlank[0].marginBottom: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 9.获取fillBlankLeftCh1 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "fillBlankLeftCh1", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stFillinBlank[1].marginTop  =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 10.获取fillBlankRightCh1 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "fillBlankRightCh1", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stFillinBlank[1].marginTop =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 11.获取colorTab */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "colorTab", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stColorTable.enConfigueTable =  (XSP_COLOR_TABLE_T)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enColorTab' failed, s32JsonType: %d, enColorTab: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 12.获取pseudoColor */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enPseudoColor", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stPseudoColor.pseudo_color =  (XSP_PSEUDO_COLOR_T)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enPseudoColor' failed, s32JsonType: %d, enPseudoColor: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 13.获取enXspStyle */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enXspStyle", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stXspStyle.xsp_style =  (XSP_DEFAULT_STYLE_T)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enXspStyle' failed, s32JsonType: %d, enXspStyle: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 14.获取enUnpen */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enUnpen", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stXspUnpen.uiEnable =  (UINT32)lNum;

        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stXspUnpen.uiEnable' failed, s32JsonType: %d, stXspUnpen.uiEnable: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 15.获取enSusorg */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enSusorg", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stSusAlert.uiEnable =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stSusAlert.uiEnable' failed, s32JsonType: %d, stSusAlert.uiEnable: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 16.获取unpenSensitivity*/
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "unpenSensitivity", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stXspUnpen.uiSensitivity  =  (XSP_SENSITIVITY_T)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stXspUnpen.uiSensitivity' failed, s32JsonType: %d, stXspUnpen.uiSensitivity: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 17.获取susorgSensitivity */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "susorgSensitivity", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stSusAlert.uiSensitivity  =  (XSP_SENSITIVITY_T)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stSusAlert.uiSensitivity' failed, s32JsonType: %d, stSusAlert.uiSensitivity: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "tip", &pstJsonXrayTip);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonXrayTip)
        {
            PRT_INFO("dspttk_cjson_get_object 'tip' failed, s32JsonType: %d, pstJsonXray: %p\n",s32JsonType, pstJsonXray);
            return SAL_FAIL;
        }
        else
        {
            /* 18.获取tip使能 */
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTip, "enable", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->stTipSet.stTipData.uiEnable =  (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stTipSet.stTipData.uiTime' failed, s32JsonType: %d, stTipSet.stTipData.uiTime: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
            /* 获取tip超时时间 */
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTip, "timeOut", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->stTipSet.stTipData.uiTime =  (UINT64)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stTipSet.stTipData.uiTime' failed, s32JsonType: %d, stTipSet.stTipData.uiTime: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
            /* 19.获取uiPackScan */
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTip, "uiPackScan", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->stTipSet.stTipData.uiPackScan =  (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stTipSet.stTipData.uiPackScan' failed, s32JsonType: %d, stTipSet.stTipData.uiPackScan: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
        }

        /* 20.获取uiBkgDetTh */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "uiBkgDetTh", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stBkg.uiBkgDetTh =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stBkg.uiBkgDetTh' failed, s32JsonType: %d, stBkg.uiBkgDetTh: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 21.获取uiFullUpdateRatio */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "uiFullUpdateRatio", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stBkg.uiFullUpdateRatio =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stBkg.uiFullUpdateRatio' failed, s32JsonType: %d, stBkg.uiFullUpdateRatio: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 22.获取enDeformityCorrection */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enDeformityCorrection", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stDeformityCorrection.bEnable =  (INT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stDeformityCorrection.bEnable' failed, s32JsonType: %d, stDeformityCorrection.bEnable: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 23.获取enDeformityCorrection */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enDeformityCorrection", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stFillinBlank[1].marginTop =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        
        /* 24.获取colorAdjust */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enDeformityCorrection", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stColorAdjust.uiLevel =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object '>stColorAdjust.uiLevel' failed, s32JsonType: %d, >stColorAdjust.uiLevel: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 25.获取包裹分割方式默认参数设置 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "packageDivideMethod", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stPackagePrm.enDivMethod =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'stPackagePrm.enDivMethod' failed, s32JsonType: %d, stPackagePrm.enDivMethod: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 25.获取包裹分割方式默认参数设置 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "inputDelayTime", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->u32InputDelayTime =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'u32InputDelayTime' failed, s32JsonType: %d, u32InputDelayTime: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        /* 26.获取空白消除json默认参数 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "rmBlankSlice", &pstJsonXrayRmSliceBlank);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonXrayRmSliceBlank)
        {
            PRT_INFO("dspttk_cjson_get_object 'rmBlankSlice' failed, s32JsonType: %d, pstJsonXrayTransType: %p\n",s32JsonType, pstJsonXrayRmSliceBlank);
            return SAL_FAIL;
        }
        else
        {
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayRmSliceBlank, "enable", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->stRmBlankSlice.bEnable = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stRmBlankSlice.bEnable' failed, s32JsonType: %d, stRmBlankSlice.bEnable: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayRmSliceBlank, "ratio", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->stRmBlankSlice.uiLevel = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stRmBlankSlice.uiLevel' failed, s32JsonType: %d, stRmBlankSlice.uiLevel: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
        }

        /* 27.获取转存enImgType*/
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enImgType", &pstJsonXrayTransType);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonXrayTransType)
        {
            PRT_INFO("dspttk_cjson_get_object 'enImgType' failed, s32JsonType: %d, pstJsonXrayTransType: %p\n",s32JsonType, pstJsonXrayTransType);
            return SAL_FAIL;
        }
        else
        {
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTransType, "bmp", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->bTransTypeEn[0] = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTransType, "jpg", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->bTransTypeEn[1] = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }

            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTransType, "gif", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->bTransTypeEn[2] = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
            
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTransType, "png", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->bTransTypeEn[3] = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
            
            s32JsonType = dspttk_cjson_get_object(pstJsonXrayTransType, "tiff", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXray->bTransTypeEn[4] = (UINT32)lNum;
                lNum = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stFillinBlank[1].marginTop' failed, s32JsonType: %d, stFillinBlank[1].marginTop: %ld\n", s32JsonType, lNum);
                return SAL_FAIL;
            }
        }
        /* 28.获取三色或六色显示 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "enImageColor", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stColorImage.enColorsImaging =  (INT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enColorsImaging.bEnable' failed, s32JsonType: %d, enColorsImaging: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
        /* 29.获取包裹分割依据 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXray, "divideSegMaster", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXray->stPackagePrm.u32segMaster =  (UINT32)lNum;
            lNum = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'divideSegMaster' failed, s32JsonType: %d, divideSegMaster: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_osd
 * @brief   获取配置文件Initialization中osd
 *
 * @param   [IN] pstJsonSetting 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败?
 */
static SAL_STATUS devcfg_load_setting_osd(CJSON *pstJsonSetting)
{
    INT32 s32JsonType = 0;
    long lNum = 0;
    CJSON *pstJsonOsd = NULL;
    CHAR *pString = NULL;
    DSPTTK_DEVCFG *pstDevCfg = &gstDevCfg;
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "osd", &pstJsonOsd);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonOsd)
    {
        PRT_INFO("dspttk_cjson_get_object 'osd' failed, s32JsonType: %d, pstJsonOsd: %p\n",s32JsonType, pstJsonOsd);
        return SAL_FAIL;
    }
    else
    {

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "enAllDisplay", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgOsd->enOsdAllDisp =  (UINT32)lNum;
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enOsdAllDisp' failed, s32JsonType: %d, enOsdAllDisp: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "emDispAIDrawType", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgOsd->stDispLinePrm.emDispAIDrawType = (XRAY_PROC_SPEED)lNum;
            lNum = 0; 
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'emDispAIDrawType' failed, s32JsonType: %d, lNum: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "dispColor", &pString);
        if (s32JsonType == CJSON_STRING  && NULL != pString)
        {
            snprintf(pstCfgOsd->cDispColor, sizeof(pstCfgOsd->cDispColor), "%s", pString);
            pString = NULL; // 重置为NULL
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'dispColor' failed, s32JsonType: %d, dispColor: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "lineAttr", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgOsd->stDispLinePrm.frametype =  (UINT32)lNum;
            lNum = 0; 
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'frametype' failed, s32JsonType: %d, frametype: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "scaleLevel", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgOsd->u32ScaleLevel =  (UINT32)lNum;
            lNum = 0; 
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'u32ScaleLevel' failed, s32JsonType: %d, u32ScaleLevel: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "emDispAIDrawType", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgOsd->stDispLinePrm.emDispAIDrawType=  (UINT32)lNum;
            lNum = 0; 
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'emDispAIDrawType' failed, s32JsonType: %d, emDispAIDrawType: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

        s32JsonType = dspttk_cjson_get_object(pstJsonOsd, "enConfDisplay", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgOsd->enOsdConfDisp =  (UINT32)lNum;
            lNum = 0; 
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enConfDisplay' failed, s32JsonType: %d, enConfDisplay: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }

    }
    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_sva
 * @brief   获取配置文件Initialization中sva
 *
 * @param   [IN] pstJsonSetting 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败?
 */
static SAL_STATUS devcfg_load_setting_sva(CJSON *pstJsonSetting)
{
    INT32 s32JsonType = 0;
    long lNum = 0;
    CJSON *pstJsonSva = NULL;
    DSPTTK_DEVCFG *pstDevCfg = &gstDevCfg;
    DSPTTK_SETTING_SVA *pstCfgSva = &pstDevCfg->stSettingParam.stSva;

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "sva", &pstJsonSva);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonSva)
    {
        PRT_INFO("dspttk_cjson_get_object 'sva' failed, s32JsonType: %d, pstJsonSva: %p\n",s32JsonType, pstJsonSva);
        return SAL_FAIL;
    }
    else
    {
        s32JsonType = dspttk_cjson_get_object(pstJsonSva, "enSvaInit", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgSva->enSvaInit =  (UINT32)lNum;
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enSvaInit' failed, s32JsonType: %d, enSvaInit: %ld\n", s32JsonType, lNum);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_xpack
 * @brief   获取配置文件Initialization中osd
 *
 * @param   [IN] pstJsonSetting 配置文件Initialization栏参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败?
 */
static SAL_STATUS devcfg_load_setting_xpack(CJSON *pstJsonSetting)
{
    INT32 s32JsonType = 0;
    long lNum = 0;
    FLOAT64 f64Num = 0;
    CJSON *pstJsonXpack = NULL;
    CJSON *pstJsonSegment = NULL;
    CJSON *pstJsonYuv = NULL;
    CJSON *pstJsonJpg = NULL;
    DSPTTK_DEVCFG *pstDevCfg = &gstDevCfg;
    DSPTTK_SETTING_XPACK *pstCfgXpack = &pstDevCfg->stSettingParam.stXpack;

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "xpack", &pstJsonXpack);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonXpack)
    {
        PRT_INFO("dspttk_cjson_get_object 'xpack' failed, s32JsonType: %d, pstJsonXpack: %p\n",s32JsonType, pstJsonXpack);
        return SAL_FAIL;
    }
    else
    {
        /* 分片上传 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXpack, "segment", &pstJsonSegment);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonSegment)
        {
            PRT_INFO("dspttk_cjson_get_object 'segment' failed, s32JsonType: %d, pstJsonXpack: %p\n",s32JsonType, pstJsonXpack);
            return SAL_FAIL;
        }
        else
        {
            s32JsonType = dspttk_cjson_get_object(pstJsonSegment, "bSegFlag", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stSegmentSet.bSegFlag =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'bSegFlag' failed, s32JsonType: %d, bSegFlag: %ld\n", s32JsonType, lNum);
                
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonSegment, "width", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stSegmentSet.SegmentWidth =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'SegmentWidth' failed, s32JsonType: %d, SegmentWidth: %ld\n", s32JsonType, lNum);
                
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonSegment, "type", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stSegmentSet.SegmentDataTpyeTpye =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'SegmentDataTpyeTpye' failed, s32JsonType: %d, SegmentDataTpyeTpye: %ld\n", s32JsonType, lNum);
                
            }
        }
        /* 偏移量设置 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXpack, "yuv", &pstJsonYuv);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonYuv)
        {
            PRT_INFO("dspttk_cjson_get_object 'yuv' failed, s32JsonType: %d, pstJsonXpack: %p\n",s32JsonType, pstJsonXpack);
            return SAL_FAIL;
        }
        else
        {
            s32JsonType = dspttk_cjson_get_object(pstJsonYuv, "reset", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stXpackYuvOffset.reset =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'reset' failed, s32JsonType: %d, reset: %ld\n", s32JsonType, lNum);
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonYuv, "offset", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stXpackYuvOffset.offset =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'offset' failed, s32JsonType: %d, offset: %ld\n", s32JsonType, lNum);
                
            }
        }
        /* jpg回调属性设置 */
        s32JsonType = dspttk_cjson_get_object(pstJsonXpack, "jpg", &pstJsonJpg);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonJpg)
        {
            PRT_INFO("dspttk_cjson_get_object 'jpg' failed, s32JsonType: %d, pstJsonXpack: %p\n",s32JsonType, pstJsonXpack);
            return SAL_FAIL;
        }
        else
        {
            s32JsonType = dspttk_cjson_get_object(pstJsonJpg, "width", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stXpackJpgSet.u32JpgBackWidth =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'u32JpgBackWidth' failed, s32JsonType: %d, u32JpgBackWidth: %ld\n", s32JsonType, lNum);
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonJpg, "height", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stXpackJpgSet.u32JpgBackHeight =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'u32JpgBackHeight' failed, s32JsonType: %d, u32JpgBackHeight: %ld\n", s32JsonType, lNum);
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonJpg, "drawSwitch", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stXpackJpgSet.bJpgDrawSwitch =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'bJpgDrawSwitch' failed, s32JsonType: %d, bJpgDrawSwitch: %ld\n", s32JsonType, lNum);
            }
            s32JsonType = dspttk_cjson_get_object(pstJsonJpg, "CropSwitch", &lNum);
            if (s32JsonType == CJSON_LONG)
            {
                pstCfgXpack->stXpackJpgSet.bJpgCropSwitch =  (UINT32)lNum;
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'bJpgCropSwitch' failed, s32JsonType: %d, bJpgCropSwitch: %ld\n", s32JsonType, lNum);
            }
            dspttk_cjson_get_value(pstJsonJpg, NULL, "f32WidthRatio", CJSON_DOUBLE, &pstCfgXpack->stXpackJpgSet.f32WidthResizeRatio, 0); // XSP超分系数
            dspttk_cjson_get_value(pstJsonJpg, NULL, "f32HeightRatio", CJSON_DOUBLE, &pstCfgXpack->stXpackJpgSet.f32HeightResizeRatio, 0); // XSP超分系数
        }
        s32JsonType = dspttk_cjson_get_object(pstJsonXpack, "syncTimeOut", &lNum);
        if (s32JsonType == CJSON_LONG)
        {
            pstCfgXpack->u32DispSyncTimeSet = (UINT32)lNum;
            f64Num = 0; // 重置为0
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'syncTimeOut' failed, s32JsonType: %d, syncTimeOut: %lf\n", s32JsonType, f64Num);
        }

    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_disp
 * @brief   获取配置文件Setting中disp的参数
 *
 * @param   [IN] pstJsonSetting 配置文件Setting栏display参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_setting_disp(CJSON *pstJsonSetting)
{

    INT32 s32JsonType = 0, j = 0, i = 0;
    CJSON *pstJsonDisp = NULL;
    CJSON *pstJsonVodev = NULL;
    CJSON *pstJsonMouse = NULL;
    CJSON *pstJsonWin = NULL;
    CJSON *pstJsonWinNum = NULL;
    INT32 s32Num = 0;
    char szWin[64] = {0};
    char szVodev[64] = {0};
    UINT32 u32CurrentWinNum = 8;        //表示当前每个vodev下支持8个可配置窗口
    UINT32 u32CurrentVodevNum = 2;      //表示当前支持2个可配置vo device

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    for (i = 0; i < u32CurrentVodevNum; i++)
    {
        /* 1.json中disp参数本地化*/
        s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "disp", &pstJsonDisp);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonDisp)
        {
            PRT_INFO("dspttk_cjson_get_object 'disp' failed, s32JsonType: %d, pstJsonDisp: %p\n", s32JsonType, pstJsonDisp);
            return SAL_FAIL;
        }

        /* 2. disp下vodev参数栏 */
        snprintf(szVodev, sizeof(szVodev), "vodev%d", i);
        s32JsonType = dspttk_cjson_get_object(pstJsonDisp, szVodev, &pstJsonVodev);

        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonVodev)
        {
            PRT_INFO("dspttk_cjson_get_object '%s' failed, s32JsonType: %d, pstJsonDisp: %p\n", szVodev, s32JsonType, pstJsonVodev);
            return SAL_FAIL;
        }

        /* 3. 使能FB*/
        s32JsonType = dspttk_cjson_get_object(pstJsonVodev, "enFB", &s32Num);

        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDisp.stFBParam[i].bFBEn = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enFB' failed, vodev is %s, s32JsonType: %d, s32Num: %d\n", szVodev, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 4. vodev下mouse参数栏 */
        s32JsonType = dspttk_cjson_get_object(pstJsonVodev, "mouse", &pstJsonMouse);
        if (s32JsonType == CJSON_OBJECT || NULL != pstJsonMouse)
        {
            /* 5. 使能mouse */
            s32JsonType = dspttk_cjson_get_object(pstJsonMouse, "enMouse", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stFBParam[i].bMouseEn = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'enMouse' failed, vodev is %s, s32JsonType: %d, s32Num: %d\n", szVodev, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 6. mouse的x坐标 */
            s32JsonType = dspttk_cjson_get_object(pstJsonMouse, "mouseX", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stFBParam[i].stMousePos.x = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stMousePos.x' failed, vodev is %s, s32JsonType: %d, s32Num: %d\n", szVodev, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 7. mouse的y坐标*/
            s32JsonType = dspttk_cjson_get_object(pstJsonMouse, "mouseY", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stFBParam[i].stMousePos.y = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'stMousePos.y' failed, vodev is %s, s32JsonType: %d, s32Num: %d\n", szVodev, s32JsonType, s32Num);
                return SAL_FAIL;
            }
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'mouse' failed, vodev is %s, s32JsonType: %d, pstJsonMouse: %p\n", szVodev, s32JsonType, pstJsonMouse);
            return SAL_FAIL;
        }

        /* 8. window参数栏配置 */
        s32JsonType = dspttk_cjson_get_object(pstJsonVodev, "window", &pstJsonWin);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonWin)
        {
            PRT_INFO("dspttk_cjson_get_object 'window' failed, vodev is %s, s32JsonType: %d, pstJsonWin: %p\n", szVodev, s32JsonType, pstJsonWin);
            return SAL_FAIL;
        }

        for (j = 0; j < u32CurrentWinNum; j++)
        {
            snprintf(szWin, sizeof(szWin), "win%d", j);

            /* 9. win0-7参数栏配置 */
            s32JsonType = dspttk_cjson_get_object(pstJsonWin, szWin, &pstJsonWinNum);
            if (s32JsonType != CJSON_OBJECT || pstJsonWinNum == NULL)
            {
                PRT_INFO("dspttk_cjson_get_object '%s' failed, vodev is %s, s32JsonType: %d, pstJsonWinNum: %p\n", szWin, szVodev, s32JsonType, pstJsonWinNum);
                return SAL_FAIL;
            }

            /* 10. win-enable 窗口使能 */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "enable", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stWinParam[i][j].enable = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'enable' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 11. win-channel 当前窗口通道 */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "channel", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stWinParam[i][j].stDispRegion.uiChan = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'channel' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 12. 当前窗口的横坐标x */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "distX", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stWinParam[i][j].stDispRegion.x = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'distX' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 13. 当前窗口的纵坐标y */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "distY", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stWinParam[i][j].stDispRegion.y = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'distY' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 14. 当前窗口的宽度w */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "distW", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stWinParam[i][j].stDispRegion.w = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'distW' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 15. 当前窗口的高度h */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "distH", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stWinParam[i][j].stDispRegion.h = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get_object 'distH' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                return SAL_FAIL;
            }

            /* 16. 当前窗口的输入源模式(VIDEOIN|VIDEOOUT|DECODE|NULL) */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "srcMode", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stInSrcParam[i][j].enInSrcMode = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get object 'srcMode' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                s32Num = 0; //重置为0；
            }

            /* 17. 当前窗口的输入源通道 */
            s32JsonType = dspttk_cjson_get_object(pstJsonWinNum, "srcChan", &s32Num);
            if (s32JsonType == CJSON_LONG)
            {
                gstDevCfg.stSettingParam.stDisp.stInSrcParam[i][j].u32SrcChn = s32Num;
                s32Num = 0; //重置为0；
            }
            else
            {
                PRT_INFO("dspttk_cjson_get object 'srcChannel' failed, vodev is %s, window is %s, s32JsonType: %d, s32Num: %d\n", szVodev, szWin, s32JsonType, s32Num);
                s32Num = 0; //重置为0；
            }
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_enc
 * @brief   获取配置文件Setting中enc的参数
 *
 * @param   [IN] pstJsonSetting 配置文件Setting栏enc参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_setting_enc(CJSON *pstJsonSetting)
{

    INT32 s32JsonType = 0, i = 0;
    CJSON *pstJsonEnc = NULL;
    CJSON *pstJsonStreamLevel = NULL; // 码流等级(主码流|子码流)

    INT32 s32Num = 0;
    CHAR szStreamLevel[64] = {0}; // 码流等级(streamtype-0表示主码流|streamtype-1表示子码流)

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    /* 1. json中enc参数本地化*/
    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "enc", &pstJsonEnc);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonEnc)
    {
        PRT_INFO("dspttk_cjson_get_object 'enc' failed, s32JsonType: %d, pstJsonEnc: %p\n", s32JsonType, pstJsonEnc);
        return SAL_FAIL;
    }

    for (i = 0; i < DSPTTK_ENC_STREAMTYPE_NUM; i++)
    {
        /* 2. 主码流和子码流选择 streamtype-0表示主码流, streamtype-1表示子码流 */
        snprintf(szStreamLevel, sizeof(szStreamLevel), (i == DSPTTK_ENC_MAIN_STREAM) ? "mainStream" : "subStream");
        s32JsonType = dspttk_cjson_get_object(pstJsonEnc, szStreamLevel, &pstJsonStreamLevel);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonStreamLevel)
        {
            PRT_INFO("dspttk_cjson_get_object 'streamLevel' failed, streamtype is %s, StreamLevel: %p, s32JsonType: %d\n", szStreamLevel, pstJsonStreamLevel, s32JsonType);
            return SAL_FAIL;
        }

        /* 3. 编码功能使能 */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "enEnc", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].bStartEn = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enEnc' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 4. 保存功能使能 */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "enSave", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].bSaveEn = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enSave' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 5. 码流类型 (视频流|混合流)*/
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "streamType", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.streamType = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'streamType' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 6. 视频格式(H264|H265) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "videoType", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.videoType = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'videoType' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 7. 分辨率(1080p|720p) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "resolution", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.resolution = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'resolution' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 8. 码率控制(变码率|定码率) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "bpsType", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.bpsType = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'bpsType' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 9. 码率大小(单位kbps) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "bps", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.bps = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'bps' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 10. 量化系数 (1-31) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "quant", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.quant = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'quant' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 11. 帧率 (1-60) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "fps", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.fps = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'fps' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 12. I帧间隔 */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "iFrameInterval", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.IFrameInterval = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'iFrameInterval' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 13. 封装格式(PS|RTP|TS|HIK|ES) */
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "muxType", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.muxType = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'muxType' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 14. 音频格式 (AAC|G711μ|G711A)*/
        s32JsonType = dspttk_cjson_get_object(pstJsonStreamLevel, "audioType", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stEnc.stEncMultiParam[i].stEncParam.audioType = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'audioType' failed, streamtype is %s, s32JsonType: %d, s32Num: %d\n", szStreamLevel, s32JsonType, s32Num);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting_dec
 * @brief   获取配置文件Setting中dec的参数
 *
 * @param   [IN] pstJsonSetting 配置文件Setting栏dec参数
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS devcfg_load_setting_dec(CJSON *pstJsonSetting)
{

    INT32 s32JsonType = 0, i = 0;
    CJSON *pstJsonDec = NULL;
    CJSON *pstJsonChan = NULL;
    INT32 s32Num = 0;
    CHAR szChannel[64] = {0};

    if (NULL == pstJsonSetting)
    {
        PRT_INFO("the pstJsonSetting is NULL\n");
        return SAL_FAIL;
    }

    /* 1. json中dec参数本地化*/
    s32JsonType = dspttk_cjson_get_object(pstJsonSetting, "dec", &pstJsonDec);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonDec)
    {
        PRT_INFO("dspttk_cjson_get_object 'dec' failed, s32JsonType: %d, pstJsonDec: %p\n", s32JsonType, pstJsonDec);
        return SAL_FAIL;
    }

    for (i = 0; i < MAX_VDEC_CHAN; i++)
    {
        /* 2. 解码通道号[0-15] */
        snprintf(szChannel, sizeof(szChannel), "channel-%d", i);
        s32JsonType = dspttk_cjson_get_object(pstJsonDec, szChannel, &pstJsonChan);
        if (s32JsonType != CJSON_OBJECT || NULL == pstJsonChan)
        {
            PRT_INFO("dspttk_cjson_get_object 'channel' failed, channel is %s, pstJsonChan: %p, s32JsonType: %d\n", szChannel, pstJsonChan, s32JsonType);
            return SAL_FAIL;
        }

        /* 3. 使能解码功能 */
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "enDecode", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].bDecode = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enDecode' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 4. 使能转封装功能 */
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "enRecode", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].bRecode = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'enRecode' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 5. 码流混合格式(PS|RTP-H264|RTP-H265) */
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "streamType", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].enMixStreamType = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'streamType' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 6. 速率(MAX|X256|X128|X64|X32|X16|X8|X4|X2|X1|X1/2|X1/4|X1/8|X1/16)*/
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "speed", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].stDecAttrPrm.speed = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'speed' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 7. 模式(正常|倍速|仅I帧|倒放|拖放|I帧转BMP) */
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "mode", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].stDecAttrPrm.vdecMode = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'mode' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 8. 封面功能是否开启 */
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "encover", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].stCoverPrm.cover_sign = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'encover' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 9. 封面索引 */
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "cover-index", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].stCoverPrm.logo_pic_indx = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'cover-index' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }

        /* 10. 重复播放模式选择 (手动|自动)*/
        s32JsonType = dspttk_cjson_get_object(pstJsonChan, "repeat", &s32Num);
        if (s32JsonType == CJSON_LONG)
        {
            gstDevCfg.stSettingParam.stDec.stDecMultiParam[i].enRepeat = s32Num;
            s32Num = 0; //重置为0；
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object 'repeat' failed, channel is %s, s32JsonType: %d, s32Num: %d\n", szChannel, s32JsonType, s32Num);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @fn      devcfg_load_setting
 * @brief   加载DSP TTK的配置文件setting的参数
 *
 * @param   pstJsonSetting[IN] 配置文件Setting栏参数
 *
 * @return  SAL_STATUS SAL_SOK：加载成功，SAL_FAIL：创建失败
 */
static SAL_STATUS devcfg_load_setting(CJSON *pstJsonSetting)
{
    SAL_STATUS sRet = SAL_SOK;

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    sRet = devcfg_load_setting_env(pstJsonSetting);
    sRet |= devcfg_load_setting_xray(pstJsonSetting);
    sRet |= devcfg_load_setting_osd(pstJsonSetting);
    sRet |= devcfg_load_setting_sva(pstJsonSetting);
    sRet |= devcfg_load_setting_xpack(pstJsonSetting);
    sRet |= devcfg_load_setting_disp(pstJsonSetting);
    sRet |= devcfg_load_setting_enc(pstJsonSetting);
    sRet |= devcfg_load_setting_dec(pstJsonSetting);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    return sRet;
}

/**
 * @fn      dspttk_devcfg_load
 * @brief   加载DSP TTK的配置文件
 *
 * @param   [IN] pJsonFile Json类型默认参数配置文件
 *
 * @return  SAL_STATUS SAL_SOK：加载成功，SAL_FAIL：创建失败
 */
SAL_STATUS dspttk_devcfg_load(CHAR *pJsonFile)
{
    SAL_STATUS sRet = SAL_FAIL;
    INT32 s32JsonFileSize = 0, s32JsonType = 0;
    CHAR *pJsonString = NULL;
    CJSON *pstJsonGlobal = NULL, *pstJsonInit = NULL, *pstJsonSetting = NULL;

    if (NULL == pJsonFile)
    {
        PRT_INFO("the 'pJsonFile' is NULL\n");
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memset(&gstDevCfg, 0, sizeof(DSPTTK_DEVCFG));
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    s32JsonFileSize = (INT32)dspttk_get_file_size(pJsonFile);
    if (s32JsonFileSize < 0)
    {
        PRT_INFO("dspttk_get_file_size failed, pJsonFile: %s\n", pJsonFile);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pJsonString = (CHAR *)malloc(s32JsonFileSize);
    if (NULL == pJsonString)
    {
        PRT_INFO("malloc for 'pJsonString' failed, size: %d\n", s32JsonFileSize);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = dspttk_read_file(pJsonFile, 0, pJsonString, (UINT32 *)&s32JsonFileSize);
    if (SAL_FAIL == sRet)
    {
        PRT_INFO("dspttk_read_file failed, name: %s, buf: %p, size: %u\n", pJsonFile, pJsonString, s32JsonFileSize);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pstJsonGlobal = dspttk_cjson_start(pJsonString);
    if (NULL == pstJsonGlobal)
    {
        PRT_INFO("dspttk_cjson_start failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* 读取init参数 */
    s32JsonType = dspttk_cjson_get_object(pstJsonGlobal, "initParam", &pstJsonInit);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonInit)
    {
        PRT_INFO("dspttk_cjson_get_object 'initParam' failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = devcfg_load_init(pstJsonInit);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("devcfg_load_init failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* 读取setting默认参数 */
    s32JsonType = dspttk_cjson_get_object(pstJsonGlobal, "setting", &pstJsonSetting);
    if (s32JsonType != CJSON_OBJECT || NULL == pstJsonSetting)
    {
        PRT_INFO("dspttk_cjson_get_object 'setting' failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = devcfg_load_setting(pstJsonSetting);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("devcfg_load_setting failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

EXIT:
    if (NULL != pJsonString)
    {
        free(pJsonString);
    }
    if (NULL == pstJsonGlobal)
    {
        dspttk_cjson_end(pstJsonGlobal);
    }

    return sRet;
}

/**
 * @fn      dspttk_operate_output_dir
 * @brief   更新输出数据目录
 */
static void dspttk_operate_output_dir(void)
{
    UINT32 i = 0, j = 0;
    CHAR sTaskName[64] = {0};
    CHAR pOutDir[64] = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV_OUTPUT *pstOutput = &gstDevCfg.stSettingParam.stEnv.stOutputDir;
    CHAR sOutputDir[128] = {0};
    struct {
        CHAR *pOutputDir;
        UINT32 u32ChanCnt;
    } astDir[] = {
        {pstOutput->sliceNraw, MAX_XRAY_CHAN},     // 回调的条带RAW数据
        {pstOutput->packageImg, MAX_XRAY_CHAN},    // 回调的包裹图像，暂仅有JPG
        {pstOutput->saveScreen, MAX_XRAY_CHAN},    // Save的当前屏幕图像
        {pstOutput->packageSeg, MAX_XRAY_CHAN},    // 回调的包裹分片
        {pstOutput->encStream, MAX_XRAY_CHAN},     // 实时预览（模拟通道）编码，包括RTP和PS流
        {pstOutput->packageTrans, MAX_XRAY_CHAN},  // 包裹转存文件夹
        {pstOutput->streamTrans, MAX_VIPC_CHAN},   // 转封装流
        {pstOutput->xrawStore, MAX_XRAY_CHAN},     // xraw回调存储路径
        {"./dump/b"},
        {"./dump/c"},
        {"./dump/d"},
        {"./dump/e"},
        {"./dump/s"},
        {"./dump/bi"},
        {"./dump/courtous"},
        {"./dumpCorr/trunk/after"},
        {"./dumpCorr/trunk/before"},
        {"./dumpCorr/trunk/current"}
    };

    snprintf(sTaskName, sizeof(sTaskName), "operate-output-dir");
    if (SAL_FAIL ==  dspttk_pthread_set_name(sTaskName))
    {
        PRT_INFO("dspttk_pthread %s set_name failed.\n",sTaskName);
    }

    strcpy(pOutDir ,astDir[0].pOutputDir);
    (void)strtok(pOutDir, "/");
    if (access(pOutDir, 06) != 0)
    {
        if (mkdir(pOutDir, 0777) != 0)
        {
            PRT_INFO("error outPutDir %s\n", pOutDir);
        }
    }

    for (i = 0; i < SAL_arraySize(astDir); i++)
    {
        if (pstDevCfg->stSettingParam.stEnv.bOutputDirAutoClear)
        {
            //删除之前目录文件
            if (SAL_FAIL == dspttk_remove_dir_or_file(astDir[i].pOutputDir, RM_FILE_ONLY))
            {
                PRT_INFO("error: Remove [%s] failed\n", astDir[i].pOutputDir);
            }
        }

        // 输出目录不存在则创建
        if (strlen(astDir[i].pOutputDir) > 0 && access(astDir[i].pOutputDir, 06) != 0)
        {
            if (mkdir(astDir[i].pOutputDir, 0777) == 0)
            {
                for (j = 0; j < astDir[i].u32ChanCnt; j++)
                {
                    snprintf(sOutputDir, sizeof(sOutputDir), "%s/%u", astDir[i].pOutputDir, j);
                    if (mkdir(sOutputDir, 0777) < 0)
                    {
                        PRT_INFO("mkdir %s failed, errno: %d, %s\n", sOutputDir, errno, strerror(errno));
                    }
                }
            }
            else
            {
                PRT_INFO("mkdir %s failed, errno: %d, %s\n", astDir[i].pOutputDir, errno, strerror(errno));
            }
        }
        /* pOutputDir 目录下 0/ 1/ 文件夹不存在则创建 */
        else if (strlen(astDir[i].pOutputDir) > 0 && access(astDir[i].pOutputDir, 06) == 0) 
        {
            for (j = 0; j < astDir[i].u32ChanCnt; j++)
            {
                snprintf(sOutputDir, sizeof(sOutputDir), "%s/%u", astDir[i].pOutputDir, j);
                if(access(sOutputDir, 06) != 0)
                {
                    if (mkdir(sOutputDir, 0777) < 0)
                    {
                        PRT_INFO("mkdir %s failed, errno: %d, %s\n", sOutputDir, errno, strerror(errno));
                    }
                }
            }
        }
    }

    return;
}

/**
 * @fn      dspttk_devcfg_init
 * @brief   从指定路径读取DSP TTK的配置文件，若不存在则创建 
 * @warning 每次运行，该接口只允许调用一次 
 *
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS dspttk_devcfg_init(void)
{
    SAL_STATUS sRet = SAL_FAIL;
    CHAR sJsonPath[64] = DSPTTK_DEFAULT_DEV_JSON;
    INT32 s32FileSize = 0;
    UINT32 u32ReadSize = 0;

    sRet = dspttk_mutex_init(&gMutexDevcfg);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_mutex_init 'gMutexDevcfg' failed\n");
        return SAL_FAIL;
    }

    /* 先判断Bin配置文件大小是否与结构体DSPTTK_DEVCFG大小一致，不一致则重新创建 */
    s32FileSize = (INT32)dspttk_get_file_size(DSPTTK_DEVCFG_PATH);
    if (sizeof(DSPTTK_DEVCFG) == s32FileSize)
    {
        sRet = dspttk_devcfg_read();
    }
    else
    {
        if (s32FileSize >= SAL_offsetof(DSPTTK_DEVCFG, stSettingParam))
        {
            // 读取现有配置文件中的设备型号
            u32ReadSize = s32FileSize;
            sRet = dspttk_read_file(DSPTTK_DEVCFG_PATH, 0, &gstDevCfg, &u32ReadSize);
            if (SAL_SOK == sRet && s32FileSize == u32ReadSize && 
                strlen(gstDevCfg.stInitParam.stCommon.boardType) > 0)
            {
                snprintf(sJsonPath, sizeof(sJsonPath), "devcfg/%s.json", gstDevCfg.stInitParam.stCommon.boardType);
                PRT_INFO("device config size(%d) unmatched with struct DSPTTK_DEVCFG(%zu), rewrite it\n", 
                         s32FileSize, sizeof(DSPTTK_DEVCFG));
            }
        }
        sRet = SAL_FAIL;
    }

    if (sRet != SAL_SOK)
    {
        sRet = dspttk_devcfg_load(sJsonPath);
        if (SAL_SOK == sRet)
        {
            dspttk_devcfg_save();
            PRT_INFO("create '%s' successfully, base '%s'\n", DSPTTK_DEVCFG_PATH, sJsonPath);
        }
        else
        {
            PRT_INFO("oops, dspttk_devcfg_load failed\n");
        }
    }

    if (gOprDirId ==0)
    {
        /* 删除输出目录线程 */
        if (SAL_SOK != dspttk_pthread_spawn(NULL, DSPTTK_PTHREAD_SCHED_RR, 90, 0x100000, dspttk_operate_output_dir, 0))
        {
            PRT_INFO("dspttk_pthread_spawn failed, hook function: dspttk_operate_output_dir\n");
        }
    }

    return sRet;
}