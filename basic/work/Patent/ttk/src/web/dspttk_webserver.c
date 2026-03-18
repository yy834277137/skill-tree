
/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include "sal_type.h"
#include "sal_macro.h"
#include "prt_trace.h"
#include "appweb.h"
#include "dspttk_cjson.h"
#include "dspttk_devcfg.h"
#include "dspttk_util.h"
#include "dspttk_html.h"
#include "dspttk_panel.h"
#include "dspttk_init.h"
#include "dspttk_cmd_xray.h"
#include "dspttk_cmd_disp.h"
#include "dspttk_cmd_enc.h"
#include "dspttk_cmd_dec.h"
#include "dspttk_cmd.h"
#include "dspttk_cmd_sva.h"
#include "dspttk_cmd_xpack.h"
#include "dspttk_inet.h"
#include "dspttk_callback.h"
#include "dspttk_fop.h"


/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define APPWEB_CONFIG               "appweb.conf"

// Initialization栏中的动作URI路径
#define WEBAPI_ACTION_START_DEV     "/action/start-device"
#define WEBAPI_ACTION_CHANGE_DEV    "/action/change-devtype"

// Setting栏中的动作URI路径
#define WEBAPI_ACTION_SET_ENV       "/action/set-env"
#define WEBAPI_ACTION_SET_XRAY      "/action/set-xray"
#define WEBAPI_ACTION_SET_DISP      "/action/set-disp"
#define WEBAPI_ACTION_SET_OSD       "/action/set-osd"
#define WEBAPI_ACTION_SET_XPACK     "/action/set-xpack"
#define WEBAPI_ACTION_SET_ENC       "/action/set-enc"
#define WEBAPI_ACTION_SET_DEC       "/action/set-dec"
#define WEBAPI_ACTION_SET_SVA       "/action/set-sva"

// Setting栏Dec Tab页的底部控制按钮与进度条
#define WEBAPI_ACTION_CTRL_DEC      "/action/ctrl-dec"
#define WEBAPI_EVENT_DEC_PROGRESS   "/sse/dec-progress"
#define WEBAPI_EVENT_STREAM_PORT    8000

// Control栏中的动作URI路径
#define WEBAPI_ACTION_CTRL_XRAY     "/action/ctrl-xray"
#define WEBAPI_ACTION_CTRL_XSP      "/action/ctrl-xsp"

// Setting栏中Package Tab页获取包裹列表 
#define WEBAPI_ACTION_GET_PACK_CH0  "/action/get-package-ch0"
#define WEBAPI_ACTION_GET_PACK_CH1  "/action/get-package-ch1"
#define WEBAPI_ACTION_GET_SAVE      "/action/get-package-save"
#define WEBAPI_ACTION_GET_SEGMENT_CH0  "/action/get-package-segment-ch0"
#define WEBAPI_ACTION_GET_SEGMENT_CH1  "/action/get-package-segment-ch1"
#define WEBAPI_ACTION_CTRL_PB_PACK_CH0  "/action/ctrl-pb-package-ch0"
#define WEBAPI_ACTION_CTRL_PB_PACK_CH1  "/action/ctrl-pb-package-ch1"
#define WEBAPI_ACTION_CTRL_PB_SAVE  "/action/ctrl-pb-package-save"


/**
 * 该定义抽象了SendCmdToDsp的引用，每个HOST_CMD对应一个函数指针
 * 入参仅一个通道号
 * 返回值是64位整型，高32位表示命令号，低32位表示错误号
 */
typedef unsigned long long (*dspttk_send_cmd)(UINT32 u32Chan);

// 命令API的封装，每个参数都有一个，多个参数可对应同一个，同时设置时，该命令API只会执行一次
typedef struct
{
    dspttk_send_cmd pFunc;  // 命令执行函数
    UINT32 u32Chan;         // SendCmdToDsp()中的通道号，支持通配符“*”、“?”表示，该值需小于“*”的值（0x2A）
    UINT32 u32MapIdx;       // FORM_CFG_ADDR_MAP映射表中的索引值，用于反向快速定位
}CMD_API_POT;

typedef struct
{
    CHAR sFormName[64];     // Html中的表单名，关键词“name”对应的值，支持通配符“*”、“?”表示循环变量
    VOID *pCfgAddr;         // 参数在配置结构体DSPTTK_DEVCFG中的地址，若有通配符，仅填写首个参数地址
    UINT32 u32VarSize;      // 参数所占内存大小，单位：Byte
    UINT32 u32VarType;      // 参数数据类型，复用CJSON中的数据类型，仅使用CJSON_STRING、CJSON_LONG、CJSON_DOUBLE
    UINT32 u32OffsAsterisk; // 通配符“*”指代的相邻参数间的偏移，单位：Byte
    UINT32 u32OffsQuestion; // 通配符“?”指代的相邻参数间的偏移，单位：Byte
    CMD_API_POT stCmdApi;   // 命令API的封装
}FORM_CFG_ADDR_MAP;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
extern SAL_STATUS dspttk_data_storage_init(void);


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
extern DSPTTK_PROGRESS_CTL gstDecProgressCtl; // 解码进度信息


/**
 * @fn      dspttk_webact_resp
 * @brief   返回给浏览器客户端的响应，Json格式，如：
 *            {"errno":-1,"errmsg":"Command(HOST_CMD_DSP_INIT) processing failed!!"}
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * @param   [IN] s32Errno 错误号
 * @param   [IN] pResMsg 返回给浏览器的信息
 */
static void dspttk_webact_resp(HttpConn *pstHttpConn, INT32 s32Errno, CHAR *pResMsg)
{
    HttpQueue *pstHttpQueue = NULL;
    MprJson *pstMprJson = mprCreateJson(MPR_JSON_OBJ);
    CHAR *pJsonResp = NULL;
    char cmsg[16] ={0};

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the 'pstHttpConn' is NULL\n");
        return;
    }

    // Add desired headers. "Set" will overwrite, add will create if not already defined.
    httpSetHeaderString(pstHttpConn, "Content-Type", "application/json");
    httpSetHeaderString(pstHttpConn, "Cache-Control", "no-cache");

    mprWriteJson(pstMprJson, "errno", sfmt("%d", s32Errno), MPR_JSON_NUMBER);
    if (NULL != pResMsg && strlen(pResMsg) > 0)
    {
        snprintf(cmsg, sizeof(cmsg), "%s", (s32Errno == 0) ? "msg" : "errmsg");
        mprWriteJson(pstMprJson, cmsg, sfmt("%s", pResMsg), MPR_JSON_STRING);
    }

    pJsonResp = mprJsonToString(pstMprJson, MPR_JSON_QUOTES);

    pstHttpQueue = pstHttpConn->writeq;
    httpWrite(pstHttpQueue, "%s", pJsonResp);
    httpWrite(pstHttpQueue, "\r\n");

    if (0 == s32Errno)
    {
        httpSetStatus(pstHttpConn, HTTP_CODE_OK);
    }
    else
    {
        httpSetStatus(pstHttpConn, HTTP_CODE_BAD_REQUEST);
    }
    httpFinalize(pstHttpConn);

    return;
}


/**
 * @fn      dspttk_take_cfg_seat
 * @brief   字符串pStrValue以pstFormCfgMap中指定的数据类型存入相应内存（入位）
 *
 * @param   [IN] pstFormCfgMap 表单中<Input>标签与配置文件中的匹配位置
 * @param   [IN] pStrValue 设置的参数值，字符串格式
 *
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
static SAL_STATUS dspttk_take_cfg_seat(FORM_CFG_ADDR_MAP *pstFormCfgMap, CHAR *pStrValue)
{
    if (NULL == pstFormCfgMap || NULL == pStrValue)
    {
        PRT_INFO("pstFormCfgMap(%p) OR pStrValue(%p) is NULL\n", pstFormCfgMap, pStrValue);
        return SAL_FAIL;
    }
    if (NULL == pstFormCfgMap->pCfgAddr)
    {
        PRT_INFO("the pCfgAddr is NULL, FormName: %s\n", pstFormCfgMap->sFormName);
        return SAL_FAIL;
    }

    switch (pstFormCfgMap->u32VarType)
    {
    case CJSON_STRING:
        snprintf(pstFormCfgMap->pCfgAddr, pstFormCfgMap->u32VarSize, "%s", pStrValue);
        break;

    case CJSON_LONG:
        switch (pstFormCfgMap->u32VarSize)
        {
        case sizeof(INT8):
            *(INT8 *)pstFormCfgMap->pCfgAddr = (INT8)strtol(pStrValue, NULL, 0);
            break;
        case sizeof(INT16):
            *(INT16 *)pstFormCfgMap->pCfgAddr = (INT16)strtol(pStrValue, NULL, 0);
            break;
        case sizeof(INT32):
            *(INT32 *)pstFormCfgMap->pCfgAddr = (INT32)strtol(pStrValue, NULL, 0);
            break;
        case sizeof(UINT64):
            *(UINT64 *)pstFormCfgMap->pCfgAddr = (UINT64)strtol(pStrValue, NULL, 0);
            break;
        default:
            PRT_INFO("VarSize(%u) is invalid as long, FormName: %s\n", pstFormCfgMap->u32VarSize, pstFormCfgMap->sFormName);
            return SAL_FAIL;
        }
        break;

    case CJSON_DOUBLE:
        switch (pstFormCfgMap->u32VarSize)
        {
        case sizeof(FLOAT32):
            *(FLOAT32 *)pstFormCfgMap->pCfgAddr = strtof(pStrValue, NULL);
            break;
        case sizeof(FLOAT64):
            *(FLOAT64 *)pstFormCfgMap->pCfgAddr = strtod(pStrValue, NULL);
            break;
        default:
            PRT_INFO("VarSize(%u) is invalid as double, FormName: %s\n", pstFormCfgMap->u32VarSize, pstFormCfgMap->sFormName);
            return SAL_FAIL;
        }
        break;

    default:
        PRT_INFO("VarType(%u) is unrecognized, FormName: %s\n", pstFormCfgMap->u32VarType, pstFormCfgMap->sFormName);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_webact_var_import
 * @brief   浏览器客户端下发的配置参数（Json格式）导入到本地的配置文件中
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * @param   [IN] pstFormCfgMap 表单中<Input>标签与配置文件中的匹配位置，数组，长度由u32VarGrpCnt指定
 * @param   [IN] u32VarGrpCnt 数组pstFormCfgMap长度
 * @param   [IN] bReversedQuery 是否逆向查询：
 *                              TRUE表示以浏览器下发的参数为依据，从pstFormCfgMap中寻找匹配项
 *                              FALSE表示直接遍历pstFormCfgMap中每个元素，从浏览器下发的参数中寻找匹配项
 * @param   [OUT] aCmdApi 需要执行的命令API序列
 *
 * @return  UINT32 需要执行的命令API个数，即数组aCmdApi中元素个数
 */
static UINT32 dspttk_webact_var_import(HttpConn *pstHttpConn, FORM_CFG_ADDR_MAP *pstFormCfgMap, UINT32 u32VarGrpCnt, BOOL bReversedQuery, CMD_API_POT aCmdApi[])
{
    UINT32 i = 0, j = 0, k = 0, u32MatchedCnt = 0;
    MprJson *pstMprJson = NULL;

    if (NULL == pstHttpConn || NULL == pstFormCfgMap)
    {
        PRT_INFO("pstHttpConn(%p) OR pstFormCfgMap(%p) is NULL\n", pstHttpConn, pstFormCfgMap);
        return SAL_FAIL;
    }

    if (bReversedQuery)
    {
        for (ITERATE_JSON(pstHttpConn->rx->params, pstMprJson, j))
        {
            for (i = 0; i < u32VarGrpCnt; i++)
            {
                if (0 == strcmp(pstMprJson->name, pstFormCfgMap[i].sFormName)) /* json文件与map表匹配找到待执行CMD */
                {
                    if (SAL_SOK == dspttk_take_cfg_seat(pstFormCfgMap + i, (CHAR *)pstMprJson->value)) /* 设置web配置的参数到本地 e.g.:出图方向L2R*/
                    {
                         /* 此打印打开可以看到上层下发的json */
                       // PRT_INFO("name %s value %s\n", pstMprJson->name, (CHAR *)pstMprJson->value);
                        // 查找aCmdApi是否已有相同的API，若存在，则无需再添加
                        if (pstFormCfgMap[i].stCmdApi.pFunc != NULL)
                        {
                            for (k = 0; k < u32MatchedCnt; k++)
                            {
                                // 函数地址与入参均相同，则判定为重复API
                                if (aCmdApi[k].pFunc == pstFormCfgMap[i].stCmdApi.pFunc && 
                                    aCmdApi[k].u32Chan == pstFormCfgMap[i].stCmdApi.u32Chan)
                                {
                                    break;
                                }
                            }
                            if (k == u32MatchedCnt) // 无重复的API
                            {
                                memcpy(aCmdApi + u32MatchedCnt++, &pstFormCfgMap[i].stCmdApi, sizeof(CMD_API_POT));
                            }
                        }
                    }
                    else
                    {
                        PRT_INFO("dspttk_take_cfg_seat failed, idx: %d, FormName: %s\n", i, pstFormCfgMap[i].sFormName);
                    }
                    break;
                }
            }
            if (i == u32VarGrpCnt) // 未匹配成功
            {
                PRT_INFO("cannot find matched form, JsonName: %s\n", pstMprJson->name);
            }
        }
    }
    else
    {
        for (i = 0; i < u32VarGrpCnt; i++)
        {
            pstMprJson = mprReadJsonObj(pstHttpConn->rx->params, pstFormCfgMap[i].sFormName);
            if (NULL != pstMprJson)
            {
                if (SAL_SOK == dspttk_take_cfg_seat(pstFormCfgMap + i, (CHAR *)pstMprJson->value))
                {
                    // 查找aCmdApi是否已有相同的API，若存在，则无需再添加
                    if (pstFormCfgMap[i].stCmdApi.pFunc != NULL)
                    {
                        for (k = 0; k < u32MatchedCnt; k++)
                        {
                            // 函数地址与入参均相同，则判定为重复API
                            if (aCmdApi[k].pFunc == pstFormCfgMap[i].stCmdApi.pFunc && 
                                aCmdApi[k].u32Chan == pstFormCfgMap[i].stCmdApi.u32Chan)
                            {
                                break;
                            }
                        }
                        if (k == u32MatchedCnt) // 无重复的API
                        {
                            memcpy(aCmdApi + u32MatchedCnt++, &pstFormCfgMap[i].stCmdApi, sizeof(CMD_API_POT));
                        }
                    }
                }
                else
                {
                    PRT_INFO("dspttk_take_cfg_seat failed, idx: %d, FormName: %s\n", i, pstFormCfgMap[i].sFormName);
                }
            }
            else
            {
                PRT_INFO("mprReadJsonObj failed, idx: %d, FormName: %s\n", i, pstFormCfgMap[i].sFormName);
            }
        }
    }
    for (k = 0; k < u32MatchedCnt; k++)
    {
        PRT_INFO("%u, %p, %u, %u\n", k, aCmdApi[k].pFunc, aCmdApi[k].u32Chan, aCmdApi[k].u32MapIdx);
    }
    return u32MatchedCnt;
}


/**
 * @fn      dspttk_expand_wildcard_in_name
 * @brief   展开字符串中的通配符，最多支持4个通配符
 * 
 * @param   [IN/OUT] pDstString IN：字符串地址数组，OUT：展开后的N个字符串
 * @param   [IN] pSrcString 带通配符的字符串
 * @param   [IN] cWildcard 通配符
 * @param   [IN] u32RangeLower 通配符取值范围小值
 * @param   [IN] u32RangeUpper 通配符取值范围大值
 * 
 * @return  UINT32 识别并替换的通配符个数，取值范围[0, 4]
 */
static UINT32 dspttk_expand_wildcard_in_name(CHAR *pDstString[], CHAR *pSrcString, CHAR cWildcard, UINT32 u32RangeLower, UINT32 u32RangeUpper)
{
    UINT32 i = 0, j = 0, k = 0, val = 0;
    CHAR *pWildcard[4] = {NULL}; // 最多仅支持有4个通配符
    CHAR *pChar = pSrcString;

    while (NULL != (pChar = strchr(pChar, cWildcard)))
    {
        pWildcard[k++] = pChar;
        pChar++;
        if (k >= SAL_arraySize(pWildcard))
        {
            PRT_INFO("just support %lu wildcard characters: %s\n", SAL_arraySize(pWildcard), pSrcString);
            break;
        }
    }

    if (k > 0)
    {
        for (i = 0, val = u32RangeLower; val <= u32RangeUpper; i++, val++)
        {
            // 以通配符为分割，拷贝上一个通配符（或开头）到下一个通配符之间字符串
            for (j = 0; j < k; j++)
            {
                if (0 == j) // 拷贝从开头到下一个通配符之间字符串
                {
                    strncpy(pDstString[i], pSrcString, pWildcard[j] - pSrcString);
                }
                else // 拷贝上一个通配符到下一个通配符之间字符串
                {
                    strncpy(pDstString[i] + strlen(pDstString[i]), pWildcard[j-1] + 1, pWildcard[j] - pWildcard[j-1] - 1);
                }
                sprintf(pDstString[i] + strlen(pDstString[i]), "%u", val);
            }
            strcpy(pDstString[i] + strlen(pDstString[i]), pWildcard[k-1] + 1); // 拷贝最后一个通配符到结束的字符串
        }
    }

    return k;
}


/**
 * @fn      dspttk_expand_form_cfg_map
 * @brief   展开有通配符的form-configure映射表，通配符支持“*”和“?”
 * @warning 返回的form-configure映射表是动态申请的，需要用free释放 
 *  
 * @param   [IN] pstFormCfgMap 需要展开的form-configure映射表
 * @param   [IN/OUT] pu32MapLen IN：展开前的form-configure映射表长度；OUT：展开后的form-configure映射表长度
 * @param   [IN] u32AsteriskValue 通配符“*”的元素个数
 * @param   [IN] u32QuestionValue 通配符“?”的元素个数
 * 
 * @return  FORM_CFG_ADDR_MAP* 将通配符展开后的form-configure映射表，使用完后需要free，当为NULL时展开失败
 */
static FORM_CFG_ADDR_MAP *dspttk_expand_form_cfg_map(FORM_CFG_ADDR_MAP *pstFormCfgMap, UINT32 *pu32MapLen, UINT32 u32AsteriskValue, UINT32 u32QuestionValue)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32ExpMapLen = 0, u32ExpMapIdx = 0;
    UINT32 u32WildcardTag = 0;
    CHAR *pNameAsterisk[u32AsteriskValue];
    CHAR *pNameQuestion[u32QuestionValue];
    FORM_CFG_ADDR_MAP *pstMapExp1 = NULL, *pstMapExp2 = NULL;
    FORM_CFG_ADDR_MAP *pstSrc = NULL, *pstDst = NULL;

    if (NULL == pstFormCfgMap || NULL == pu32MapLen)
    {
        PRT_INFO("pstFormCfgMap(%p) OR pu32MapLen(%p) is NULL\n", pstFormCfgMap, pu32MapLen);
        return NULL;
    }

    if(0 == u32AsteriskValue && 0 == u32QuestionValue)
    {
        PRT_INFO("u32AsteriskValue(%u) OR u32QuestionValue(%u) should be bigger than ZERO\n", u32AsteriskValue, u32QuestionValue);
        return NULL;
    }

    for (i = 0; i < *pu32MapLen; i++)
    {
        k = 1;
        if (NULL != strchr(pstFormCfgMap[i].sFormName, '*'))
        {
            k *= u32AsteriskValue;
            u32WildcardTag |= 0x1; // 1 << 0
        }
        if (NULL != strchr(pstFormCfgMap[i].sFormName, '?'))
        {
            k *= u32QuestionValue;
            u32WildcardTag |= 0x2; // 1 << 1
        }
        u32ExpMapLen += k;
    }

    // 先展开通配符“*”
    if (u32WildcardTag & 0x1)
    {
        pstMapExp1 = (FORM_CFG_ADDR_MAP *)malloc(sizeof(FORM_CFG_ADDR_MAP) * u32ExpMapLen);
        if (NULL != pstMapExp1)
        {
            memset(pstMapExp1, 0, sizeof(FORM_CFG_ADDR_MAP) * u32ExpMapLen);
        }
        else
        {
            PRT_INFO("malloc for 'pstMapExp1' failed, size: %zu\n", sizeof(FORM_CFG_ADDR_MAP) * u32ExpMapLen);
            return NULL;
        }

        for (i = 0; i < *pu32MapLen; i++)
        {
            pstSrc = pstFormCfgMap + i;
            if (NULL != strchr(pstSrc->sFormName, '*'))
            {
                for (j = 0; j < u32AsteriskValue; j++)
                {
                    pstDst = pstMapExp1 + u32ExpMapIdx;
                    pNameAsterisk[j] = pstDst->sFormName;
                    pstDst->pCfgAddr = (UINT8 *)pstSrc->pCfgAddr + pstSrc->u32OffsAsterisk * j;
                    pstDst->u32VarSize = pstSrc->u32VarSize;
                    pstDst->u32VarType = pstSrc->u32VarType;
                    pstDst->u32OffsQuestion = pstSrc->u32OffsQuestion;
                    pstDst->stCmdApi.pFunc = pstSrc->stCmdApi.pFunc;
                    if (pstSrc->stCmdApi.u32Chan == '*')
                    {
                        pstDst->stCmdApi.u32Chan = j;
                    }
                    else if ((pstSrc->stCmdApi.u32Chan == '^') && (u32WildcardTag & 0x2))
                    {
                        pstDst->stCmdApi.u32Chan = (j << 16) | '^';
                    }
                    pstDst->stCmdApi.u32MapIdx = u32ExpMapIdx++;
                }
                dspttk_expand_wildcard_in_name(pNameAsterisk, pstSrc->sFormName, '*', 0, u32AsteriskValue-1);
            }
            else
            {
                pstDst = pstMapExp1 + u32ExpMapIdx;
                memcpy(pstDst, pstSrc, sizeof(FORM_CFG_ADDR_MAP));
                pstDst->stCmdApi.u32MapIdx = u32ExpMapIdx++;
            }
        }
        *pu32MapLen = u32ExpMapIdx;
        u32ExpMapIdx = 0;
    }

    // 再展开通配符“?”
    if (u32WildcardTag & 0x2)
    {
        pstMapExp2 = (FORM_CFG_ADDR_MAP *)malloc(sizeof(FORM_CFG_ADDR_MAP) * u32ExpMapLen);
        if (NULL != pstMapExp2)
        {
            memset(pstMapExp2, 0, sizeof(FORM_CFG_ADDR_MAP) * u32ExpMapLen);
        }
        else
        {
            PRT_INFO("malloc for 'pstMapExp2' failed, size: %zu\n", sizeof(FORM_CFG_ADDR_MAP) * u32ExpMapLen);
            if (NULL != pstMapExp1)
            {
                free(pstMapExp1);
                return NULL;
            }
        }

        for (i = 0; i < *pu32MapLen; i++)
        {
            pstSrc = ((NULL != pstMapExp1) ? pstMapExp1 : pstFormCfgMap) + i; // 若之前已扩展了通配符“*”，则从扩展的结果继续
            if (NULL != strchr(pstSrc->sFormName, '?'))
            {
                for (j = 0; j < u32QuestionValue; j++)
                {
                    pstDst = pstMapExp2 + u32ExpMapIdx;
                    pNameQuestion[j] = pstDst->sFormName;
                    pstDst->pCfgAddr = (UINT8 *)pstSrc->pCfgAddr + pstSrc->u32OffsQuestion * j;
                    pstDst->u32VarSize = pstSrc->u32VarSize;
                    pstDst->u32VarType = pstSrc->u32VarType;
                    pstDst->stCmdApi.pFunc = pstSrc->stCmdApi.pFunc;
                    if (pstSrc->stCmdApi.u32Chan == '?')
                    {
                        pstDst->stCmdApi.u32Chan = j;
                    }
                    else if (((pstSrc->stCmdApi.u32Chan & 0xFFFF) == '^') && (u32WildcardTag & 0x1))
                    {
                        pstDst->stCmdApi.u32Chan = (pstSrc->stCmdApi.u32Chan & 0xFFFF0000) | j;
                    }
                    pstDst->stCmdApi.u32MapIdx = u32ExpMapIdx++;
                }
                dspttk_expand_wildcard_in_name(pNameQuestion, pstSrc->sFormName, '?', 0, u32QuestionValue-1);
            }
            else
            {
                pstDst = pstMapExp2 + u32ExpMapIdx;
                memcpy(pstDst, pstSrc, sizeof(FORM_CFG_ADDR_MAP));
                pstDst->stCmdApi.u32MapIdx = u32ExpMapIdx++;
            }
        }
        *pu32MapLen = u32ExpMapIdx;
        u32ExpMapIdx = 0;
        if (NULL != pstMapExp1)
        {
            free(pstMapExp1);
            pstMapExp1 = NULL;
        }
    }

    return (pstMapExp1 != NULL) ? pstMapExp1 : pstMapExp2; // 取非空的指针作为返回结果
}


/**
 * @fn      dspttk_webact_start_dev
 * @brief   Initialization栏中Start按钮的响应回调函数，初始化DSP组件
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_start_dev(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32MapLen = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_INIT_PARAM *pstCfgInit = &pstDevCfg->stInitParam;
    FORM_CFG_ADDR_MAP *pstFormCfgMap = NULL;
    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"xray-det-vendor", pstCfgInit->stXray.xrayDetVendor, sizeof(pstCfgInit->stXray.xrayDetVendor), CJSON_STRING}, // 探测板型号
        {"xray-chan-cnt", &pstCfgInit->stXray.xrayChanCnt, sizeof(pstCfgInit->stXray.xrayChanCnt), CJSON_LONG}, // 射线源数
        {"xray-width-max", &pstCfgInit->stXray.xrayWidthMax, sizeof(pstCfgInit->stXray.xrayWidthMax), CJSON_LONG}, // XRaw宽
        {"xray-height-max", &pstCfgInit->stXray.xrayHeightMax, sizeof(pstCfgInit->stXray.xrayHeightMax), CJSON_LONG}, // XRaw高
        {"xray-resize-factor", &pstCfgInit->stXray.resizeFactor, sizeof(pstCfgInit->stXray.resizeFactor), CJSON_DOUBLE}, // XSP超分系数
        {"xray-lib-src", &pstCfgInit->stXray.enXspLibSrc, sizeof(pstCfgInit->stXray.enXspLibSrc), CJSON_LONG}, // XSP算法库
        {"xray-source-type-0", pstCfgInit->stXray.xraySourceType[0], sizeof(pstCfgInit->stXray.xraySourceType[0]), CJSON_STRING}, // 主源型号
        {"xray-source-type-1", pstCfgInit->stXray.xraySourceType[1], sizeof(pstCfgInit->stXray.xraySourceType[1]), CJSON_STRING}, // 侧源型号
        {"xray-belt-speed-*-?", &pstCfgInit->stXray.stSpeed[0][0].beltSpeed, sizeof(pstCfgInit->stXray.stSpeed[0][0].beltSpeed), 
            CJSON_DOUBLE, sizeof(DSPTTK_INIT_XRAY_SPEED)*XRAY_SPEED_NUM, sizeof(DSPTTK_INIT_XRAY_SPEED)}, // 传送带速度
        {"xray-slice-height-*-?", &pstCfgInit->stXray.stSpeed[0][0].sliceHeight, sizeof(pstCfgInit->stXray.stSpeed[0][0].sliceHeight), 
            CJSON_LONG, sizeof(DSPTTK_INIT_XRAY_SPEED)*XRAY_SPEED_NUM, sizeof(DSPTTK_INIT_XRAY_SPEED)}, // 条带高度
        {"xray-integral-time-*-?", &pstCfgInit->stXray.stSpeed[0][0].integralTime, sizeof(pstCfgInit->stXray.stSpeed[0][0].integralTime), 
            CJSON_LONG, sizeof(DSPTTK_INIT_XRAY_SPEED)*XRAY_SPEED_NUM, sizeof(DSPTTK_INIT_XRAY_SPEED)}, // 积分时间
        {"xray-disp-fps-*-?", &pstCfgInit->stXray.stSpeed[0][0].dispfps, sizeof(pstCfgInit->stXray.stSpeed[0][0].dispfps), 
            CJSON_LONG, sizeof(DSPTTK_INIT_XRAY_SPEED)*XRAY_SPEED_NUM, sizeof(DSPTTK_INIT_XRAY_SPEED)}, // 显示帧率
        {"disp-vodev-cnt", &pstCfgInit->stDisplay.voDevCnt, sizeof(pstCfgInit->stDisplay.voDevCnt), CJSON_LONG}, // 输出设备数
        {"disp-padding-top", &pstCfgInit->stDisplay.paddingTop, sizeof(pstCfgInit->stDisplay.paddingTop), CJSON_LONG}, // Padding Top
        {"disp-padding-bottom", &pstCfgInit->stDisplay.paddingBottom, sizeof(pstCfgInit->stDisplay.paddingBottom), CJSON_LONG}, // Padding Botottom
        {"disp-blanking-top", &pstCfgInit->stDisplay.blankingTop, sizeof(pstCfgInit->stDisplay.blankingTop), CJSON_LONG}, // Blanking Top
        {"disp-blanking-bottom", &pstCfgInit->stDisplay.blankingBottom, sizeof(pstCfgInit->stDisplay.blankingBottom), CJSON_LONG}, // Blanking Botottom
        {"enc-chan-cnt", &pstCfgInit->stEncDec.encChanCnt, sizeof(pstCfgInit->stEncDec.encChanCnt), CJSON_LONG}, // 编码通道数
        {"dec-chan-cnt", &pstCfgInit->stEncDec.decChanCnt, sizeof(pstCfgInit->stEncDec.decChanCnt), CJSON_LONG}, // 解码通道数
        {"ipc-stream-pack-trans-chan-cnt", &pstCfgInit->stEncDec.ipcStreamPackTransChanCnt, sizeof(pstCfgInit->stEncDec.ipcStreamPackTransChanCnt), CJSON_LONG},// 转封装通道数
        {"sva-dev-chip-type", &pstCfgInit->stSva.enDevChipType, sizeof(pstCfgInit->stSva.enDevChipType), CJSON_LONG},// 设备种类，用于区分单双芯片
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 解析Web端下发的content（格式name1=value1&name2=value2或Json，均可解析），并保存到配置文件中
    if (DSPTTK_RUNSTAT_READY == gstGlobalStatus.enRunStatus)
    {
        u32MapLen = SAL_arraySize(astFormCfgMap);
        pstFormCfgMap = dspttk_expand_form_cfg_map(astFormCfgMap, &u32MapLen, XRAY_FORM_NUM, XRAY_SPEED_NUM);
        if (NULL == pstFormCfgMap)
        {
            PRT_INFO("dspttk_expand_form_cfg_map failed\n");
            dspttk_webact_resp(pstHttpConn, sRet, "Expand form-configure mapping table failed!!");
            return;
        }
        gstGlobalStatus.enRunStatus = DSPTTK_RUNSTAT_INITING;

        // 从浏览器客户端接收设置参数，并保存到配置文件中
        dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        dspttk_webact_var_import(pstHttpConn, pstFormCfgMap, u32MapLen, SAL_FALSE, NULL);
        dspttk_devcfg_save();
        dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
        free(pstFormCfgMap);

        // 初始化DSP
        sRet = dspttk_sys_init(pstDevCfg);
        if (0 == sRet)
        {
            dspttk_webact_resp(pstHttpConn, sRet, NULL);
            gstGlobalStatus.enRunStatus = DSPTTK_RUNSTAT_RUNNING; // 转入运行状态
            if (SAL_SOK != dspttk_data_storage_init())
            {
                PRT_INFO("oops, dspttk_data_storage_init failed\n");
            }
        }
        else
        {
            dspttk_webact_resp(pstHttpConn, sRet, "Command(HOST_CMD_DSP_INIT) processing failed!!");
            gstGlobalStatus.enRunStatus = DSPTTK_RUNSTAT_READY; // 重新回到就绪状态
            PRT_INFO("dspttk_sys_init error\n");
            return;
        }

        // 根据配置文件，更新Html页面
        dspttk_build_panel_init(gpHtmlHandle, &pstDevCfg->stInitParam);
        dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

        if (SINGLE_CHIP_TYPE == pstDevCfg->stInitParam.stSva.enDevChipType || 
            DOUBLE_CHIP_MASTER_TYPE == pstDevCfg->stInitParam.stSva.enDevChipType)
        {
            sRet = dspttk_osd_init(pstDevCfg);
            if(sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_osd_init failed.\n");
            }

            sRet = dspttk_xpack_init(pstDevCfg);
            if(sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_xpack_init failed.\n");
            }

            //初始化xray成像处理参数
            sRet = dspttk_xray_init(pstDevCfg);
            if(sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_xray_init failed.\n");
            }

            // 初始化Display参数
            sRet = dspttk_disp_set_init(pstDevCfg);
            if(sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_disp_set_init failed.\n");
            }

            //初始化encode参数
            sRet = dspttk_enc_set_init(pstDevCfg);
            if(sRet == SAL_FAIL)
            {
                PRT_INFO("dspttk_enc_set_init failed.\n");
            }
        }
    }
    else
    {
        dspttk_webact_resp(pstHttpConn, -1, sfmt("Device is already %s !!", (DSPTTK_RUNSTAT_INITING == gstGlobalStatus.enRunStatus) ? "initializing" : "running"));
    }

    return;
}


/**
 * @fn      dspttk_webact_change_devtype
 * @brief   
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_change_devtype(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    CHAR sDevJson[64] = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    FORM_CFG_ADDR_MAP stFormCfgMap = {
        "dev-board-type", &pstDevCfg->stInitParam.stCommon.boardType, sizeof(pstDevCfg->stInitParam.stCommon.boardType), CJSON_STRING};

    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_READY)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Stop device first!!");
        return;
    }

    strcpy(sDevJson, pstDevCfg->stInitParam.stCommon.boardType); // 备份当前的设备类型
    dspttk_webact_var_import(pstHttpConn, &stFormCfgMap, 1, SAL_FALSE, NULL);
    if (0 != strcmp(pstDevCfg->stInitParam.stCommon.boardType, sDevJson)) // 若与当前的不匹配，则重新加载参数并更新Html页面
    {
        snprintf(sDevJson, sizeof(sDevJson), "devcfg/%s.json", pstDevCfg->stInitParam.stCommon.boardType);
        sRet = dspttk_devcfg_load(sDevJson); // 重新加载指定机型对应的参数
        if (SAL_SOK == sRet)
        {
            dspttk_devcfg_save();
            PRT_INFO("create devcfg.bin successfully, base '%s'\n", sDevJson);

            // 更新Html页面
            sRet = dspttk_build_panels(gpHtmlHandle, pstDevCfg);
            if (SAL_SOK == sRet)
            {
                dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);
            }
            else
            {
                PRT_INFO("dspttk_build_panels failed, base '%s'\n", sDevJson);
            }
        }
    }

    if (SAL_SOK == sRet)
    {
        dspttk_webact_resp(pstHttpConn, SAL_SOK, NULL);
    }
    else
    {
        dspttk_webact_resp(pstHttpConn, sRet, sfmt("Change device to %s failed!!", pstDevCfg->stInitParam.stCommon.boardType));
    }

    return;
}


/**
 * @fn      dspttk_webact_set_env
 * @brief   Setting栏中Env标签页Apply按钮的响应回调函数，设置运行的环境变量
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_set_env(HttpConn *pstHttpConn)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV *pstCfgEnv = &pstDevCfg->stSettingParam.stEnv;
    DSPTTK_SETTING_ENV_INPUT *pstInput = &pstCfgEnv->stInputDir;
    DSPTTK_SETTING_ENV_OUTPUT *pstOutput = &pstCfgEnv->stOutputDir;

    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"env-runtime-dir", pstCfgEnv->runtimeDir, sizeof(pstCfgEnv->runtimeDir), CJSON_STRING},             // dspttk程序运行时的绝对路径，相对于PC端
        {"env-input-raw-path-loop", &pstCfgEnv->bInputDirRtLoop, sizeof(pstCfgEnv->bInputDirRtLoop), CJSON_LONG},// 输入路径下xraw数据否循环
        {"env-output-dir-auto-clear", &pstCfgEnv->bOutputDirAutoClear, sizeof(pstCfgEnv->bOutputDirAutoClear), CJSON_LONG}, // 输出路径下文件是否清空
        {"env-input-rtxraw", pstInput->rtXraw, sizeof(pstInput->rtXraw), CJSON_STRING},                      // 输入的XRAW源文件
        {"env-input-tipnraw", pstInput->tipNraw, sizeof(pstInput->tipNraw), CJSON_STRING},                   // 输入的TIP归一化RAW文件
        {"env-input-ipcstream", pstInput->ipcStream, sizeof(pstInput->ipcStream), CJSON_STRING},             // 输入的IPC视频流或混合流
        {"env-input-aimodel", pstInput->aiModel, sizeof(pstInput->aiModel), CJSON_STRING},                   // 输入的AI模型
        {"env-input-alarmaudio", pstInput->alramAudio, sizeof(pstInput->alramAudio), CJSON_STRING},          // 输入的报警音频 
        {"env-input-gui", pstInput->gui, sizeof(pstInput->gui), CJSON_STRING},                               // 输入的GUI图片文件
        {"env-output-rawslice", pstOutput->sliceNraw, sizeof(pstOutput->sliceNraw), CJSON_STRING},           // 输出的归一化RAW条带数据
        {"env-output-packageimg", pstOutput->packageImg, sizeof(pstOutput->packageImg), CJSON_STRING},       // 输出的包裹JPG图片
        {"env-output-savescreen", pstOutput->saveScreen, sizeof(pstOutput->saveScreen), CJSON_STRING},       // 输出的save图片
        {"env-output-packageseg", pstOutput->packageSeg, sizeof(pstOutput->packageSeg), CJSON_STRING},       // 输出的包裹分片
        {"env-output-encstream", pstOutput->encStream, sizeof(pstOutput->encStream), CJSON_STRING},          // 输出编码
        {"env-output-packagetrans", pstOutput->packageTrans, sizeof(pstOutput->packageTrans), CJSON_STRING}, // 输出包裹转存
        {"env-output-streamtrans", pstOutput->streamTrans, sizeof(pstOutput->streamTrans), CJSON_STRING},    // 输出转封装
        // {"env-output-xrawstore", pstOutput->xrawStore, sizeof(pstOutput->xrawStore), CJSON_STRING},          // 原始raw数据保存
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 从浏览器客户端接收设置参数，并保存到配置文件中
    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_webact_var_import(pstHttpConn, astFormCfgMap, SAL_arraySize(astFormCfgMap), SAL_TRUE, NULL);
    dspttk_devcfg_save();
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    // 根据配置文件，更新Html页面
    dspttk_build_panel_setting_env(gpHtmlHandle, pstCfgEnv);
    dspttk_webact_resp(pstHttpConn, 0, NULL);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}


/**
 * @fn      dspttk_webact_set_sva
 * @brief   Setting栏中sva标签页Apply按钮的响应回调函数，设置sva属性
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * 
 * @return   none
 */
static void dspttk_webact_set_sva(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, u32ApiGrpCnt = 0, u32MapLen = 0, u32MapIdx = 0;
    CMD_API_POT *pstCmdApiGrp = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_SVA stCfgSvaBak = {0}, *pstCfgSva = &pstDevCfg->stSettingParam.stSva;

    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"sva-init-switch", &pstCfgSva->enSvaInit, sizeof(pstCfgSva->enSvaInit), 
            CJSON_LONG, 0, 0, {dspttk_sva_set_init_switch, 0}},
        {"sva-pack-conse", &pstCfgSva->enSvaPackConse, sizeof(pstCfgSva->enSvaPackConse), 
            CJSON_LONG, 0, 0, {NULL, 0}}
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        return;
    }

    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgSvaBak, pstCfgSva, sizeof(DSPTTK_SETTING_SVA));
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, astFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);
        if (0 != CMD_RET_OF(u64CmdRet))
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = astFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgSva;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(astFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&stCfgSvaBak + s64AddrOffs, astFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save();
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);

    dspttk_build_panel_setting_sva(gpHtmlHandle, pstCfgSva);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}


/**
 * @fn      dspttk_webact_set_osd
 * @brief   Setting栏中osd标签页Apply按钮的响应回调函数，设置osd属性
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * 
 * @return  none
 */
static void dspttk_webact_set_osd(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, u32ApiGrpCnt = 0, u32MapLen = 0, u32MapIdx = 0;
    CMD_API_POT *pstCmdApiGrp = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD stCfgOsdBak = {0}, *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;

    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"osd-all-disp-enable", &pstCfgOsd->enOsdAllDisp, sizeof(pstCfgOsd->enOsdAllDisp), 
            CJSON_LONG, 0, 0, {dspttk_sva_dect_switch, 0}},
        {"osd-line-type", &pstCfgOsd->enBorderType, sizeof(pstCfgOsd->enBorderType), 
            CJSON_LONG, 0, 0, {dspttk_sva_set_osd_border_type, 0}},
        {"osd-disp-color", &pstCfgOsd->cDispColor, sizeof(pstCfgOsd->cDispColor), 
            CJSON_STRING, 0, 0, {dspttk_sva_set_alert_color, 0}},
        {"osd-line-attr", &pstCfgOsd->stDispLinePrm.frametype, sizeof(pstCfgOsd->stDispLinePrm.frametype), 
            CJSON_LONG, 0, 0, {dspttk_sva_set_disp_line_type, 0}},
        {"osd-scale-level", &pstCfgOsd->u32ScaleLevel, sizeof(pstCfgOsd->u32ScaleLevel), 
            CJSON_LONG, 0, 0, {dspttk_sva_set_scale_level, 0}},
        {"osd-combine-alert-name", &pstCfgOsd->stDispLinePrm.emDispAIDrawType, sizeof(pstCfgOsd->stDispLinePrm.emDispAIDrawType), 
            CJSON_LONG, 0, 0, {dspttk_sva_set_disp_line_type, 0}},
        {"osd-conf-disp-enable", &pstCfgOsd->enOsdConfDisp, sizeof(pstCfgOsd->enOsdConfDisp), 
            CJSON_LONG, 0, 0, {dspttk_sva_set_confidence_switch, 0}}
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        return;
    }

    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgOsdBak, pstCfgOsd, sizeof(DSPTTK_SETTING_OSD));
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, astFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);
        if (0 != CMD_RET_OF(u64CmdRet))
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = astFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgOsd;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(astFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&stCfgOsdBak + s64AddrOffs, astFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save();
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);

    dspttk_build_panel_setting_osd(gpHtmlHandle, pstCfgOsd);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}


/**
 * @fn      dspttk_webact_set_xpack
 * @brief   Setting栏中xpack标签页Apply按钮的响应回调函数，设置xpack属性
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * 
 * @return   none
 */
static void dspttk_webact_set_xpack(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, u32ApiGrpCnt = 0, u32MapLen = 0, u32MapIdx = 0;
    CMD_API_POT *pstCmdApiGrp = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XPACK stCfgXpackBak = {0}, *pstCfgXpack = &pstDevCfg->stSettingParam.stXpack;

    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"xpack-jpg-back-width", &pstCfgXpack->stXpackJpgSet.u32JpgBackWidth, sizeof(pstCfgXpack->stXpackJpgSet.u32JpgBackWidth), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_jpg, 0}},
        {"xpack-jpg-back-height", &pstCfgXpack->stXpackJpgSet.u32JpgBackHeight, sizeof(pstCfgXpack->stXpackJpgSet.u32JpgBackHeight), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_jpg, 0}},
        {"xpack-jpg-draw-switch", &pstCfgXpack->stXpackJpgSet.bJpgDrawSwitch, sizeof(pstCfgXpack->stXpackJpgSet.bJpgDrawSwitch), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_jpg, 0}},
        {"xpack-jpg-width-ratio", &pstCfgXpack->stXpackJpgSet.f32WidthResizeRatio, sizeof(pstCfgXpack->stXpackJpgSet.f32WidthResizeRatio), 
            CJSON_DOUBLE, 0, 0, {dspttk_xpack_set_jpg, 0}},
        {"xpack-jpg-height-ratio", &pstCfgXpack->stXpackJpgSet.f32HeightResizeRatio, sizeof(pstCfgXpack->stXpackJpgSet.f32HeightResizeRatio), 
            CJSON_DOUBLE, 0, 0, {dspttk_xpack_set_jpg, 0}},
        {"xpack-jpg-crop-switch", &pstCfgXpack->stXpackJpgSet.bJpgCropSwitch, sizeof(pstCfgXpack->stXpackJpgSet.bJpgCropSwitch), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_jpg, 0}},
        {"xpack-yuv-offset", &pstCfgXpack->stXpackYuvOffset.offset, sizeof(pstCfgXpack->stXpackYuvOffset.offset), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_yuv_offset, 0}},
        {"xpack-yuv-reset", &pstCfgXpack->stXpackYuvOffset.reset, sizeof(pstCfgXpack->stXpackYuvOffset.reset), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_yuv_offset, 0}},
        {"xpack-segment-enable", &pstCfgXpack->stSegmentSet.bSegFlag, sizeof(pstCfgXpack->stSegmentSet.bSegFlag), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_segment_attr, 0}},
        {"xpack-segment-width", &pstCfgXpack->stSegmentSet.SegmentWidth, sizeof(pstCfgXpack->stSegmentSet.SegmentWidth), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_segment_attr, 0}},
        {"xpack-segment-type", &pstCfgXpack->stSegmentSet.SegmentDataTpyeTpye, sizeof(pstCfgXpack->stSegmentSet.SegmentDataTpyeTpye), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_segment_attr, 0}},
        {"xpack-disp-sync-timeout", &pstCfgXpack->u32DispSyncTimeSet, sizeof(pstCfgXpack->u32DispSyncTimeSet), 
            CJSON_LONG, 0, 0, {dspttk_xpack_set_disp_sync_time, 0}},
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        return;
    }

    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgXpackBak, pstCfgXpack, sizeof(DSPTTK_SETTING_XPACK));
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, astFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);
        if (0 != CMD_RET_OF(u64CmdRet))
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = astFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgXpack;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(astFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&stCfgXpackBak + s64AddrOffs, astFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save();
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);

    dspttk_build_panel_setting_xpack(gpHtmlHandle, pstCfgXpack);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}

/**
 * @fn      dspttk_webact_set_xray
 * @brief   Setting栏中XRay标签页Apply按钮的响应回调函数，设置XRay参数
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_set_xray(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, u32ApiGrpCnt = 0, u32MapLen = 0, u32MapIdx = 0;
    CMD_API_POT *pstCmdApiGrp = NULL; // 该内存用于存储变化参数对应的函数指针，且已去重
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY stCfgXraybk = {0}, *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"xray-speed", &pstCfgXray->stSpeedParam.enSpeedUsr, sizeof(pstCfgXray->stSpeedParam.enSpeedUsr), 
            CJSON_LONG, 0, 0, {dspttk_xray_rtpreview_change_speed, 0}}, // 过包速度
        {"xray-format", &pstCfgXray->stSpeedParam.enFormUsr, sizeof(pstCfgXray->stSpeedParam.enFormUsr), 
            CJSON_LONG, 0, 0, {dspttk_xray_rtpreview_change_speed, 0}}, // 过包形态
        {"xray-direction", &pstCfgXray->enDispDir, sizeof(pstCfgXray->enDispDir), 
            CJSON_LONG, 0, 0, {NULL, 0}}, // 过包显示方向
        {"xray-enhanced-scan", &pstCfgXray->stEnhancedScan.bEnable, sizeof(pstCfgXray->stEnhancedScan.bEnable), 
            CJSON_LONG, 0, 0, {dspttk_rtpreview_enhanced_scan, 0}}, // 设置强扫
        {"xray-vertical-mirror-ch0", &pstCfgXray->stVMirror[0].bEnable, sizeof(pstCfgXray->stVMirror[0].bEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_vertical_mirror, 0}}, // 主视角垂直镜像
        {"xray-vertical-mirror-ch1", &pstCfgXray->stVMirror[1].bEnable, sizeof(pstCfgXray->stVMirror[1].bEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_vertical_mirror, 1}}, // 辅视角垂直镜像
        {"xray-fillin-blank-left-ch0", &pstCfgXray->stFillinBlank[0].marginTop, sizeof(pstCfgXray->stFillinBlank[0].marginTop), 
            CJSON_LONG, 0, 0, {dspttk_xray_rtpreview_fillin_blank, 0}}, // 主视角XRaw左置白
        {"xray-fillin-blank-right-ch0", &pstCfgXray->stFillinBlank[0].marginBottom, sizeof(pstCfgXray->stFillinBlank[0].marginBottom), 
            CJSON_LONG, 0, 0, {dspttk_xray_rtpreview_fillin_blank, 0}}, // 主视角XRaw右置白
        {"xray-fillin-blank-left-ch1", &pstCfgXray->stFillinBlank[1].marginTop, sizeof(pstCfgXray->stFillinBlank[1].marginTop), 
            CJSON_LONG, 0, 0, {dspttk_xray_rtpreview_fillin_blank, 1}}, // 辅视角XRaw左置白
        {"xray-fillin-blank-right-ch1", &pstCfgXray->stFillinBlank[1].marginBottom, sizeof(pstCfgXray->stFillinBlank[1].marginBottom), 
            CJSON_LONG, 0, 0, {dspttk_xray_rtpreview_fillin_blank, 1}}, // 辅视角XRaw右置白
        {"xray-color-table", &pstCfgXray->stColorTable.enConfigueTable, sizeof(pstCfgXray->stColorTable.enConfigueTable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_color_table, 0}},
        {"xray-pseudo-color", &pstCfgXray->stPseudoColor.pseudo_color, sizeof(pstCfgXray->stPseudoColor.pseudo_color), 
            CJSON_LONG, 0, 0, {dspttk_xsp_pseudo_color, 0}},
        {"xray-default-style", &pstCfgXray->stXspStyle.xsp_style, sizeof(pstCfgXray->stXspStyle.xsp_style), 
            CJSON_LONG, 0, 0, {dspttk_xsp_default_style, 0}},
        {"xray-unpen-enable", &pstCfgXray->stXspUnpen.uiEnable, sizeof(pstCfgXray->stXspUnpen.uiEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_alert_unpen, 0}},
        {"xray-unpen-sensitivity", &pstCfgXray->stXspUnpen.uiSensitivity, sizeof(pstCfgXray->stXspUnpen.uiSensitivity), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_alert_unpen, 0}},
        {"xray-susorg-enable", &pstCfgXray->stSusAlert.uiEnable, sizeof(pstCfgXray->stSusAlert.uiEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_alert_sus, 0}},
        {"xray-susorg-sensitivity", &pstCfgXray->stSusAlert.uiSensitivity, sizeof(pstCfgXray->stSusAlert.uiSensitivity), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_alert_sus, 0}},
        {"xray-tip-enable", &pstCfgXray->stTipSet.stTipData.uiEnable, sizeof(pstCfgXray->stTipSet.stTipData.uiEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_tip, 0}},
        {"xray-tip-timeout", &pstCfgXray->stTipSet.stTipData.uiTime, sizeof(pstCfgXray->stTipSet.stTipData.uiTime), 
            CJSON_LONG, 0, 0, {NULL, 0}},
        {"xray-bkg-det-th", &pstCfgXray->stBkg.uiBkgDetTh, sizeof(pstCfgXray->stBkg.uiBkgDetTh), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_bkg , 0}},
        {"xray-bkg-update-ratio", &pstCfgXray->stBkg.uiFullUpdateRatio, sizeof(pstCfgXray->stBkg.uiFullUpdateRatio), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_bkg, 0}},
        {"xray-deformity-corr", &pstCfgXray->stDeformityCorrection.bEnable, sizeof(pstCfgXray->stDeformityCorrection.bEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_deformity_correction, 0}},
        {"xray-color-adjust", &pstCfgXray->stColorAdjust.uiLevel, sizeof(pstCfgXray->stColorAdjust.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_color_adjust, 0}},
        {"xray-package-divide-method", &pstCfgXray->stPackagePrm.enDivMethod, sizeof(pstCfgXray->stPackagePrm.enDivMethod), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_package_divide_method, 0}},
        {"xray-divide-seg-master", &pstCfgXray->stPackagePrm.u32segMaster, sizeof(pstCfgXray->stPackagePrm.u32segMaster), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_package_divide_method, 0}},
        {"xray-rm-blank-slice-enable", &pstCfgXray->stRmBlankSlice.bEnable, sizeof(pstCfgXray->stRmBlankSlice.bEnable), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_rm_blank_slice, 0}},
        {"xray-rm-blank-slice-ratio", &pstCfgXray->stRmBlankSlice.uiLevel, sizeof(pstCfgXray->stRmBlankSlice.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_rm_blank_slice, 0}},
        {"xray-stretch-param-ratio", &pstCfgXray->stretchParam.uiLevel, sizeof(pstCfgXray->stretchParam.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_stretch, 0}},
        {"xray-gamma-ratio", &pstCfgXray->stXspGamma.uiLevel, sizeof(pstCfgXray->stXspGamma.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_gamma, 0}},
        {"xray-default-enhance-ratio", &pstCfgXray->stDefaultEnhance.uiLevel, sizeof(pstCfgXray->stDefaultEnhance.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_default_enhance, 0}},
        {"xray-contrast-ratio", &pstCfgXray->stContrast.uiLevel, sizeof(pstCfgXray->stContrast.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_contrast, 0}},
        {"xray-aixsp-rate-ratio", &pstCfgXray->stXspAixsprate.uiLevel, sizeof(pstCfgXray->stXspAixsprate.uiLevel), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_aixsp_rate, 0}},
        {"xray-bkg-color-format", &pstCfgXray->u32BkgColorFormat, sizeof(pstCfgXray->u32BkgColorFormat), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_bkg_color, 0}},
        {"xray-rt-input-delay-time", &pstCfgXray->u32InputDelayTime, sizeof(pstCfgXray->u32InputDelayTime), 
            CJSON_LONG, 0, 0, {NULL, 0}},
        {"xray-color-imaging", &pstCfgXray->stColorImage.enColorsImaging, sizeof(pstCfgXray->stColorImage.enColorsImaging), 
            CJSON_LONG, 0, 0, {dspttk_xsp_set_imaging_param, 0}},
        {"xray-bkg-color-value", &pstCfgXray->cBkgColorValue, sizeof(pstCfgXray->cBkgColorValue), 
            CJSON_STRING, 0, 0, {dspttk_xsp_set_bkg_color, 0}},
        {"xray-trans-type-bmp", &pstCfgXray->bTransTypeEn[0], sizeof(pstCfgXray->bTransTypeEn[0]), CJSON_LONG, 0, 0, {NULL, 0}},
        {"xray-trans-type-jpg", &pstCfgXray->bTransTypeEn[1], sizeof(pstCfgXray->bTransTypeEn[1]), CJSON_LONG, 0, 0, {NULL, 0}},
        {"xray-trans-type-gif", &pstCfgXray->bTransTypeEn[2], sizeof(pstCfgXray->bTransTypeEn[2]), CJSON_LONG, 0, 0, {NULL, 0}},
        {"xray-trans-type-png", &pstCfgXray->bTransTypeEn[3], sizeof(pstCfgXray->bTransTypeEn[3]), CJSON_LONG, 0, 0, {NULL, 0}},
        {"xray-trans-type-tiff", &pstCfgXray->bTransTypeEn[4], sizeof(pstCfgXray->bTransTypeEn[4]), CJSON_LONG, 0, 0, {NULL, 0}},
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        return;
    }

    // 遍历映射表astFormCfgMap，建立索引，为后续设置失败恢复原有参数做准备
    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    // 从浏览器客户端接收设置参数，并保存到配置文件中
    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgXraybk, pstCfgXray, sizeof(DSPTTK_SETTING_XRAY)); // 先备份参数，若有参数设置失败，需要恢复
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, astFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    /* 变化参数对应执行函数 */
    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);
        if (0 != CMD_RET_OF(u64CmdRet)) // 若命令执行失败，则从备份中恢复参数，并返回错误信息给web端
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = astFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgXray;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(astFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&stCfgXraybk + s64AddrOffs, astFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s0x%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save(); // 保存配置文件
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);

    // 根据配置文件，更新Html页面
    dspttk_build_panel_setting_xray(gpHtmlHandle, pstCfgXray);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}


static void dspttk_webact_set_disp(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, j = 0, u32ApiGrpCnt = 0;
    UINT32 u32MapLen = 0, u32MapIdx = 0;
    UINT32 u32CurrentWinNum = 8;    // 表示当前每个vodev下支持8个可配置窗口
    CMD_API_POT *pstCmdApiGrp = NULL; // 该内存用于存储变化参数对应的函数指针，且已去重
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DISP stCfgDispbk = {0}, *pstCfgDisp = &pstDevCfg->stSettingParam.stDisp;
    FORM_CFG_ADDR_MAP *pstFormCfgMap = NULL;
    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"disp-vodev*-enfb", &pstCfgDisp->stFBParam[0].bFBEn, sizeof(pstCfgDisp->stFBParam[0].bFBEn), 
            CJSON_LONG, sizeof(DSPTTK_FB_PARAM), 0, {dspttk_disp_fb_set, '*'}}, // GUI是否使能
        {"disp-vodev*-enmouse", &pstCfgDisp->stFBParam[0].bMouseEn, sizeof(pstCfgDisp->stFBParam[0].bMouseEn), 
            CJSON_LONG, sizeof(DSPTTK_FB_PARAM), 0, {dspttk_disp_fb_mouse_bind, '*'}}, // 鼠标是否使能
        {"disp-vodev*-mouse-x", &pstCfgDisp->stFBParam[0].stMousePos.x, sizeof(pstCfgDisp->stFBParam[0].stMousePos.x), 
            CJSON_LONG, sizeof(DSPTTK_FB_PARAM), 0, {NULL, '*'}}, // 鼠标的坐标，基于屏幕显示分辨率
        {"disp-vodev*-mouse-y", &pstCfgDisp->stFBParam[0].stMousePos.y, sizeof(pstCfgDisp->stFBParam[0].stMousePos.y), 
            CJSON_LONG, sizeof(DSPTTK_FB_PARAM), 0, {NULL, '*'}}, // 鼠标的坐标，基于屏幕显示分辨率
        {"disp-vodev*-win?-en", &pstCfgDisp->stWinParam[0][0].enable, sizeof(pstCfgDisp->stWinParam[0][0].enable), 
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, // 设置窗口是否使能
        {"disp-vodev*-win?-chan", &pstCfgDisp->stWinParam[0][0].stDispRegion.uiChan, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.uiChan), 
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, // 设置窗口的channel号
        {"disp-vodev*-win?-dist-x", &pstCfgDisp->stWinParam[0][0].stDispRegion.x, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.x), 
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, // 窗口坐标x，基于屏幕显示分辨率
        {"disp-vodev*-win?-dist-y", &pstCfgDisp->stWinParam[0][0].stDispRegion.y, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.y), 
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, // 窗口坐标y，基于屏幕显示分辨率
        {"disp-vodev*-win?-dist-w", &pstCfgDisp->stWinParam[0][0].stDispRegion.w, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.w), 
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, // 窗口区域宽度
        {"disp-vodev*-win?-dist-h", &pstCfgDisp->stWinParam[0][0].stDispRegion.h, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.h), 
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, // 窗口区域高度
        {"disp-vodev*-win?-border-en", &pstCfgDisp->stWinParam[0][0].stDispRegion.bDispBorder, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.bDispBorder),
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, //设置边框使能
        {"disp-vodev*-win?-border-width", &pstCfgDisp->stWinParam[0][0].stDispRegion.BorDerLineW, sizeof(pstCfgDisp->stWinParam[0][0].stDispRegion.BorDerLineW),
            CJSON_LONG, sizeof(DISP_POS_CTRL) * MAX_DISP_CHAN, sizeof(DISP_POS_CTRL), {dspttk_disp_vodev_win_set, '^'}}, //设置边框宽度
        {"disp-vodev*-win?-fill-color", &pstCfgDisp->stWinColorPrm[0][0].cFillWinColor, sizeof(pstCfgDisp->stWinColorPrm[0][0].cFillWinColor), 
            CJSON_STRING, sizeof(DSPTTK_WIN_COLOR_PARAM) * MAX_DISP_CHAN, sizeof(DSPTTK_WIN_COLOR_PARAM), {dspttk_disp_vodev_win_set, '^'}}, //设置窗口背景填充颜色
        {"disp-vodev*-win?-border-color", &pstCfgDisp->stWinColorPrm[0][0].cBorderLineColor, sizeof(pstCfgDisp->stWinColorPrm[0][0].cBorderLineColor), 
            CJSON_STRING, sizeof(DSPTTK_WIN_COLOR_PARAM) * MAX_DISP_CHAN, sizeof(DSPTTK_WIN_COLOR_PARAM), {dspttk_disp_vodev_win_set, '^'}}, //设置窗口边框颜色
        {"disp-vodev*-win?-input-src", &pstCfgDisp->stInSrcParam[0][0].enInSrcMode, sizeof(pstCfgDisp->stInSrcParam[0][0].enInSrcMode), 
            CJSON_LONG, sizeof(DSPTTK_INSOURCE_PARAM) * MAX_DISP_CHAN, sizeof(DSPTTK_INSOURCE_PARAM), {dspttk_disp_input_src_set, '^'}}, // 窗口输入源设置
        {"disp-vodev*-win?-input-chan", &pstCfgDisp->stInSrcParam[0][0].u32SrcChn, sizeof(pstCfgDisp->stInSrcParam[0][0].u32SrcChn), 
            CJSON_LONG, sizeof(DSPTTK_INSOURCE_PARAM) * MAX_DISP_CHAN, sizeof(DSPTTK_INSOURCE_PARAM), {dspttk_disp_input_src_set, '^'}}, // 窗口输入源通道号
        {"disp-vodev*-win?-enlarge-global-x", &pstCfgDisp->stGlobalEnlargePrm[0][0].x, sizeof(pstCfgDisp->stGlobalEnlargePrm[0][0].x), 
            CJSON_LONG, sizeof(DISP_GLOBALENLARGE_PRM) * MAX_DISP_CHAN, sizeof(DISP_GLOBALENLARGE_PRM), {dspttk_disp_enlarge_global, '^'}}, // 全局放大中心点x坐标
        {"disp-vodev*-win?-enlarge-global-y", &pstCfgDisp->stGlobalEnlargePrm[0][0].y, sizeof(pstCfgDisp->stGlobalEnlargePrm[0][0].y), 
            CJSON_LONG, sizeof(DISP_GLOBALENLARGE_PRM) * MAX_DISP_CHAN, sizeof(DISP_GLOBALENLARGE_PRM), {dspttk_disp_enlarge_global, '^'}}, // 全局放大中心点y坐标
        {"disp-vodev*-win?-enlarge-global-ratio", &pstCfgDisp->stGlobalEnlargePrm[0][0].ratio, sizeof(pstCfgDisp->stGlobalEnlargePrm[0][0].ratio), 
            CJSON_DOUBLE, sizeof(DISP_GLOBALENLARGE_PRM) * MAX_DISP_CHAN, sizeof(DISP_GLOBALENLARGE_PRM), {dspttk_disp_enlarge_global, '^'}}, // 全局放大比例
        {"disp-vodev*-win?-enlarge-local-x", &pstCfgDisp->stLocalEnlargePrm[0][0].x, sizeof(pstCfgDisp->stLocalEnlargePrm[0][0].x), 
            CJSON_LONG, sizeof(DISP_GLOBALENLARGE_PRM) * MAX_DISP_CHAN, sizeof(DISP_GLOBALENLARGE_PRM), {dspttk_disp_enlarge_local, '^'}}, // 局部放大中心点x坐标
        {"disp-vodev*-win?-enlarge-local-y", &pstCfgDisp->stLocalEnlargePrm[0][0].y, sizeof(pstCfgDisp->stLocalEnlargePrm[0][0].y), 
            CJSON_LONG, sizeof(DISP_GLOBALENLARGE_PRM) * MAX_DISP_CHAN, sizeof(DISP_GLOBALENLARGE_PRM), {dspttk_disp_enlarge_local, '^'}}, // 局部放大中心点y坐标
        {"disp-vodev*-win?-enlarge-local-ratio", &pstCfgDisp->stLocalEnlargePrm[0][0].ratio, sizeof(pstCfgDisp->stLocalEnlargePrm[0][0].ratio), 
            CJSON_DOUBLE, sizeof(DISP_GLOBALENLARGE_PRM) * MAX_DISP_CHAN, sizeof(DISP_GLOBALENLARGE_PRM), {dspttk_disp_enlarge_local, '^'}}, // 局部放大比例
    };

    // 初始化stWinParam结构体中voDev属性
    for (i = 0; i < pstDevCfg->stInitParam.stDisplay.voDevCnt; i++)
    {
        for (j = 0; j < u32CurrentWinNum; j++)
        {
            pstCfgDisp->stWinParam[i][j].voDev = i;
        }
    }

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Server malloc for 'pstCmdApiGrp' failed!!");
        return;
    }

    // 遍历映射表astFormCfgMap，建立索引，为后续设置失败恢复原有参数做准备
    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    pstFormCfgMap = dspttk_expand_form_cfg_map(astFormCfgMap, &u32MapLen, pstDevCfg->stInitParam.stDisplay.voDevCnt, u32CurrentWinNum);
    if (NULL == pstFormCfgMap)
    {
        PRT_INFO("dspttk_expand_form_cfg_map failed\n");
        dspttk_webact_resp(pstHttpConn, sRet, "Expand form-configure mapping table failed!!");
        return;
    }

    for (i = 0; i < pstDevCfg->stInitParam.stDisplay.voDevCnt; i++)
    {
        for (j = 0; j < u32CurrentWinNum; j++)
        {
            pstCfgDisp->stWinParam[i][j].voDev = i;
        }
    }
    // 从浏览器客户端接收设置参数，并保存到配置文件中
    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgDispbk, pstCfgDisp, sizeof(stCfgDispbk)); // 先备份参数，若有参数设置失败，需要恢复
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, pstFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    // 为变化的参数执行对应的命令
    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);
        if (0 != CMD_RET_OF(u64CmdRet)) // 若命令执行失败，则从备份中恢复参数，并返回错误信息给web端
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = pstFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgDisp;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(pstFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&stCfgDispbk + s64AddrOffs, pstFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }

    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save(); // 保存配置文件
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);
    free(pstFormCfgMap);

    // 根据配置文件，更新Html页面
    dspttk_build_panel_setting_disp(gpHtmlHandle, pstCfgDisp);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}

/**
 * @fn      dspttk_webact_set_enc
 * @brief   Setting栏中Enc标签页Apply按钮的响应回调函数，设置Encode参数
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_set_enc(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, u32ApiGrpCnt = 0, u32MapLen = 0, u32MapIdx = 0;
    CMD_API_POT *pstCmdApiGrp = NULL; // 该内存用于存储变化参数对应的函数指针，且已去重
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENC stCfgEncbk = {0}, *pstCfgEnc = &pstDevCfg->stSettingParam.stEnc;
    FORM_CFG_ADDR_MAP *pstFormCfgMap = NULL;
    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"enc-en-*", &pstCfgEnc->stEncMultiParam[0].bStartEn, sizeof(pstCfgEnc->stEncMultiParam[0].bStartEn), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_enable_set, '*'}}, // 编码是否开启
        {"enc-save-*", &pstCfgEnc->stEncMultiParam[0].bSaveEn, sizeof(pstCfgEnc->stEncMultiParam[0].bSaveEn), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_save_set, '*'}}, // 编码是否保存到文件
        {"enc-streamtype-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.streamType, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.streamType), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 码流类型设置(视频流|混合流)
        {"enc-video-type-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.videoType, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.videoType), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 视频格式设置(H264|H265)
        {"enc-audio-type-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.audioType, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.audioType), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 音频格式设置(G711μ|G711A|AAC)
        {"enc-resolution-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.resolution, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.resolution), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 分辨率设置(720p|1080p)
        {"enc-bps-type-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.bpsType, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.bpsType), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 码率控制(变码率|定码率)
        {"enc-bps-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.bps, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.bps), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 码率设置(单位kbps)
        {"enc-quant-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.quant, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.quant), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 量化系数设置(1-31)
        {"enc-fps-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.fps, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.fps), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 帧率设置(1-60)
        {"enc-Iframeinterval-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.IFrameInterval, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.IFrameInterval), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // I帧间隔设置
        {"enc-muxtype-*", &pstCfgEnc->stEncMultiParam[0].stEncParam.muxType, sizeof(pstCfgEnc->stEncMultiParam[0].stEncParam.muxType), 
            CJSON_LONG, sizeof(DSPTTK_ENC_MULTI_PARAM), 0, {dspttk_enc_param_set, '*'}}, // 封装格式设置(PS|RTP|TS|HIK|ES), 目前仅支持PS和RTP格式
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Server malloc for 'pstCmdApiGrp' failed!!");
        return;
    }

    // 遍历映射表astFormCfgMap，建立索引，为后续设置失败恢复原有参数做准备
    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    pstFormCfgMap = dspttk_expand_form_cfg_map(astFormCfgMap, &u32MapLen, pstDevCfg->stInitParam.stEncDec.encChanCnt, 0);
    if(NULL == pstFormCfgMap)
    {
        PRT_INFO("dspttk_expand_form_cfg_map failed\n");
        dspttk_webact_resp(pstHttpConn, sRet, "Expand form-configure mapping table failed.\n");
        return;
    }

    // 从浏览器客户端接收设置参数，并保存到配置文件中
    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgEncbk, pstCfgEnc, sizeof(stCfgEncbk)); // 先备份参数，若有参数设置失败，需要恢复
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, pstFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    // 为变化的参数执行对应的命令
    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);
        if (0 != CMD_RET_OF(u64CmdRet)) // 若命令执行失败，则从备份中恢复参数，并返回错误信息给web端
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = astFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgEnc;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(astFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&stCfgEncbk + s64AddrOffs, astFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }

    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save(); // 保存配置文件
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);
    free(pstFormCfgMap);

    // 根据配置文件，更新Html页面
    dspttk_build_panel_setting_enc(gpHtmlHandle, pstCfgEnc);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}

/**
 * @fn      dspttk_webact_set_dec
 * @brief   Setting栏中Dec标签页Apply按钮的响应回调函数，设置Decode参数
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_set_dec(HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT64 u64CmdRet = 0;
    ptrdiff_t s64AddrOffs = 0;
    CHAR sRespStr[256] = "Command(";
    UINT32 i = 0, u32ApiGrpCnt = 0, u32MapLen = 0, u32MapIdx = 0;
    CMD_API_POT *pstCmdApiGrp = NULL; // 该内存用于存储变化参数对应的函数指针，且已去重
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_DEC stCfgDecbk = {0}, *pstCfgDec = &pstDevCfg->stSettingParam.stDec;
    FORM_CFG_ADDR_MAP *pstFormCfgMap = NULL;
    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"dec-en-chan*", &pstCfgDec->stDecMultiParam[0].bDecode, sizeof(pstCfgDec->stDecMultiParam[0].bDecode), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {dspttk_dec_chan_enable, '*'}}, // 解码功能是否使能
        {"dec-en-recode-chan*", &pstCfgDec->stDecMultiParam[0].bRecode, sizeof(pstCfgDec->stDecMultiParam[0].bRecode), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {dspttk_dec_recode_start, '*'}}, // 转封装是否使能
        {"dec-streamtype-chan*", &pstCfgDec->stDecMultiParam[0].enMixStreamType, sizeof(pstCfgDec->stDecMultiParam[0].enMixStreamType), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {dspttk_dec_set_muxtype, '*'}}, // 混合格式，获取包括视频格式和封装格式(PS|RTP-H264|RTP-H265)
        {"dec-speedtype-chan*", &pstCfgDec->stDecMultiParam[0].stDecAttrPrm.speed, sizeof(pstCfgDec->stDecMultiParam[0].stDecAttrPrm.speed), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {dspttk_dec_set_attr, '*'}}, // 速率(MAX|X256|X128|X64|X32|X16|X8|X4|X2|X1|X1/2|X1/4|X1/8|X1/16)
        {"dec-mode-chan*", &pstCfgDec->stDecMultiParam[0].stDecAttrPrm.vdecMode, sizeof(pstCfgDec->stDecMultiParam[0].stDecAttrPrm.vdecMode), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {dspttk_dec_set_attr, '*'}}, // 模式(正常|倍速|仅I帧|倒放|拖放|I帧转BMP)
        {"dec-en-coversign-chan*", &pstCfgDec->stDecMultiParam[0].stCoverPrm.cover_sign, sizeof(pstCfgDec->stDecMultiParam[0].stCoverPrm.cover_sign), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {NULL, '*'}}, // 封面功能是否开启
        {"dec-coversign-index-chan*", &pstCfgDec->stDecMultiParam[0].stCoverPrm.logo_pic_indx, sizeof(pstCfgDec->stDecMultiParam[0].stCoverPrm.logo_pic_indx), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {NULL, '*'}}, // 封面索引
        {"dec-replay-type-chan*", &pstCfgDec->stDecMultiParam[0].enRepeat, sizeof(pstCfgDec->stDecMultiParam[0].enRepeat), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {NULL, '*'}}, //循环模式设置
        {"dec-jump-percent-*", &pstCfgDec->stDecMultiParam[0].u32JumpPercent, sizeof(pstCfgDec->stDecMultiParam[0].u32JumpPercent), 
            CJSON_LONG, sizeof(DSPTTK_DEC_MULTI_PARAM), 0, {NULL, '*'}}, // 跳转进度
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    pstCmdApiGrp = (CMD_API_POT *)malloc(u32MapLen * sizeof(CMD_API_POT));
    if (NULL == pstCmdApiGrp)
    {
        PRT_INFO("malloc for 'pstCmdApiGrp' failed, unit size %zu, count %d\n", sizeof(CMD_API_POT), u32MapLen);
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Server malloc for 'pstCmdApiGrp' failed!!");
        return;
    }

    // 遍历映射表astFormCfgMap，建立索引，为后续设置失败恢复原有参数做准备
    for (i = 0; i < u32MapLen; i++)
    {
        astFormCfgMap[i].stCmdApi.u32MapIdx = i;
    }

    pstFormCfgMap = dspttk_expand_form_cfg_map(astFormCfgMap, &u32MapLen, pstDevCfg->stInitParam.stEncDec.decChanCnt, 0);
    if(NULL == pstFormCfgMap)
    {
        PRT_INFO("dspttk_expand_form_cfg_map failed\n");
        dspttk_webact_resp(pstHttpConn, sRet, "Expand form-configure mapping table failed.\n");
        return;
    }

    // 从浏览器客户端接收设置参数，并保存到配置文件中
    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&stCfgDecbk, pstCfgDec, sizeof(stCfgDecbk)); // 先备份参数，若有参数设置失败，需要恢复
    u32ApiGrpCnt = dspttk_webact_var_import(pstHttpConn, pstFormCfgMap, u32MapLen, SAL_TRUE, pstCmdApiGrp);
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);

    // 为变化的参数执行对应的命令
    for (i = 0; i < u32ApiGrpCnt; i++)
    {
        u64CmdRet = pstCmdApiGrp[i].pFunc(pstCmdApiGrp[i].u32Chan);

        if (0 != CMD_RET_OF(u64CmdRet)) // 若命令执行失败，则从备份中恢复参数，并返回错误信息给web端
        {
            u32MapIdx = pstCmdApiGrp[i].u32MapIdx;
            if (u32MapIdx < u32MapLen)
            {
                s64AddrOffs = astFormCfgMap[u32MapIdx].pCfgAddr - (VOID *)pstCfgDec;
                dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                memcpy(astFormCfgMap[u32MapIdx].pCfgAddr, (UINT8 *)&pstCfgDec + s64AddrOffs, astFormCfgMap[u32MapIdx].u32VarSize);
                dspttk_devcfg_save();
                dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
            }
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            sRet = SAL_FAIL;
        }
    }

    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_mutex_lock(&gMutexDevcfg, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    dspttk_devcfg_save(); // 保存配置文件
    dspttk_mutex_unlock(&gMutexDevcfg, __FUNCTION__, __LINE__);
    free(pstCmdApiGrp);
    free(pstFormCfgMap);
    // 根据配置文件，更新Html页面
    dspttk_build_panel_setting_dec(gpHtmlHandle, pstCfgDec);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
    dspttk_html_save_as(gpHtmlHandle, DSPTTK_INDEX_HTML_FILE);

    return;
}

/**
 * @fn      dspttk_webact_get_segment
 * @brief   获取节点中所有包裹分片信息并上传给web端用于拼图显示
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * @param   [IN] UINT32 chan 通道号
 * 
 * @return   none
 */
static void dspttk_webact_get_segment(UINT32 chan, HttpConn *pstHttpConn)
{
    UINT32 i = 0, j = 0, u32SegNum = 500, u32SegTotalW = 0, u32SegTotalH = 0; // 最多取500条记录
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_NODE *pNode = NULL;
    DSPTTK_SEG_DATA_NODE *pstNodeData = NULL;
    MprJson *pstMprObj = NULL, *pstMprArray = NULL;
    MprJson *pstMprImgArr = NULL, *pstMprImgObj = NULL;
    SEGMENT_CB_FOR_WEB *pstSeg = dspttk_get_seg_prm_for_web(chan);

    if (chan >= pstDevCfg->stInitParam.stXray.xrayChanCnt)
    {
        PRT_INFO("the chan(%u) is invalid, should be less than %u\n", chan, pstDevCfg->stInitParam.stXray.xrayChanCnt);
        dspttk_webact_resp(pstHttpConn, SAL_SOK, NULL);
        return;
    }

    if (pstSeg[chan].lstSeg == NULL || pstDevCfg->stSettingParam.stXpack.stSegmentSet.bSegFlag == 0)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "cannot open seg cb");
        return;
    }

    u32SegNum = dspttk_lst_get_count(pstSeg[chan].lstSeg);
    if (u32SegNum == 0)
    {
        PRT_INFO("u32SegNum %d\n", u32SegNum);
    }

    pstMprArray = mprCreateJson(MPR_JSON_ARRAY);
    if (pstMprArray)
    {
        for (j = 0; j < u32SegNum; j++)
        {
            pNode = dspttk_lst_locate(pstSeg[chan].lstSeg, j);
            pstNodeData =  (DSPTTK_SEG_DATA_NODE *)pNode->pAdData;
            pstMprObj = mprCreateJson(MPR_JSON_OBJ);
            if (NULL == pstMprObj)
            {
                PRT_INFO("chan %u, %u/%u, mprCreateJson MPR_JSON_OBJ failed\n", chan, j, u32SegNum);
                break;
            }
            pstMprImgArr = mprCreateJson(MPR_JSON_ARRAY);
            if (NULL != pstMprImgArr)
            {
                for(i = 0; i < SAL_arraySize(pstNodeData->stSegImgData); i++)
                {
                    pstMprImgObj = mprCreateJson(MPR_JSON_OBJ);
                    if (NULL == pstMprImgObj)
                    {
                        PRT_INFO("chan %u, %u/%u, mprCreateJson MPR_JSON_OBJ failed\n", chan, j, u32SegNum);
                        break;
                    }
                    if (strlen(pstNodeData->stSegImgData[i].sSegImgPath) != 0)
                    {
                        // mprWriteJson(pstMprImgObj, "idx", sfmt("%u",pstNodeData->stSegImgData[i].u32SegmentIndx), MPR_JSON_NUMBER);
                        mprWriteJson(pstMprImgObj, "Seg", pstNodeData->stSegImgData[i].sSegImgPath, MPR_JSON_STRING);
                        if (0 != mprSetJsonObj(pstMprImgArr, "[$]", pstMprImgObj))
                        {
                            PRT_INFO("chan %u, %u/%lu, mprSetJsonObj failed\n", chan, i, SAL_arraySize(pstNodeData->stSegImgData));
                        }
                        u32SegTotalW += pstNodeData->stSegImgData[i].u32SegmentWidth;
                        u32SegTotalH = pstNodeData->stSegImgData[i].u32SegmentHeight;
                    }
                }
            }
            mprWriteJson(pstMprObj, "No", sfmt("%u", j+1), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Width", sfmt("%u", u32SegTotalW), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Height", sfmt("%u", u32SegTotalH), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Time", sfmt("%llu", pstNodeData->u64SegPackTime), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "DIR", sfmt("%u", pstNodeData->Direction), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "JPG", pstNodeData->sSegDir, MPR_JSON_STRING);
            mprWriteJson(pstMprObj, "IMG", mprJsonToString(pstMprImgArr, MPR_JSON_QUOTES), MPR_JSON_STRING);
            if (0 != mprSetJsonObj(pstMprArray, "[$]", pstMprObj))
            {
                PRT_INFO("chan %u, %u/%u, mprSetJsonObj failed\n", chan, j, u32SegNum);
            }
            u32SegTotalW = u32SegTotalH =0;/* 下一个包裹总宽高复位累加 */
        }
    }
    else
    {
        PRT_INFO("chan %u, mprCreateJson MPR_JSON_ARRAY failed\n", chan);
    }

    // Add desired headers. "Set" will overwrite, add will create if not already defined.
    httpSetHeaderString(pstHttpConn, "Content-Type", "application/json");
    httpSetHeaderString(pstHttpConn, "Cache-Control", "no-cache");
    httpWrite(pstHttpConn->writeq, "%s", mprJsonToString(pstMprArray, MPR_JSON_QUOTES));
    httpWrite(pstHttpConn->writeq, "\r\n");

    httpSetStatus(pstHttpConn, HTTP_CODE_OK);
    httpFinalize(pstHttpConn);

    return;
}


/**
 * @fn      dspttk_webact_get_packages
 * @brief   
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_get_packages(UINT32 chan, HttpConn *pstHttpConn)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 j = 0, u32BufSize = 0, u32PackageNum = 500; // 最多取500条记录
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DB_TABLE_PACKAGE *pstPackage = NULL;
    MprJson *pstMprObj = NULL;
    MprJson *pstMprArray = NULL;

    if (chan >= pstDevCfg->stInitParam.stXray.xrayChanCnt)
    {
        PRT_INFO("the chan(%u) is invalid, should be less than %u\n", chan, pstDevCfg->stInitParam.stXray.xrayChanCnt);
        dspttk_webact_resp(pstHttpConn, SAL_SOK, NULL);
        return;
    }

    u32BufSize = sizeof(DB_TABLE_PACKAGE) * u32PackageNum;
    pstPackage = (DB_TABLE_PACKAGE *)malloc(u32BufSize);
    if (NULL == pstPackage)
    {
        PRT_INFO("malloc for 'pstPackage' failed, buffer size: %u\n", u32BufSize);
        dspttk_webact_resp(pstHttpConn, SAL_SOK, NULL);
        return;
    }

    sRet = dspttk_query_package_lastest_n(chan, &pstPackage, &u32PackageNum);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("chan %u, dspttk_query_package_lastest_n failed\n", chan);
        u32PackageNum = 0; // 强制为输出0
    }

    pstMprArray = mprCreateJson(MPR_JSON_ARRAY);
    if (NULL != pstMprArray)
    {
        for (j = 0; j < u32PackageNum; j++)
        {
            pstMprObj = mprCreateJson(MPR_JSON_OBJ);
            if (NULL == pstMprObj)
            {
                PRT_INFO("chan %u, %u/%u, mprCreateJson MPR_JSON_OBJ failed\n", chan, j, u32PackageNum);
                break;
            }
            mprWriteJson(pstMprObj, "No", sfmt("%u", j+1), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Width", sfmt("%u", pstPackage[j].u32Width), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Height", sfmt("%u", pstPackage[j].u32Height), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Time", sfmt("%llu", pstPackage[j].u64TimeEnd), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "SVA", sfmt("%u", pstPackage[j].u32AiDgrNum), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "XRI", sfmt("%u", pstPackage[j].u32XrIdtNum), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "JPG", pstPackage[j].sJpgPath, MPR_JSON_STRING);
            if (0 != mprSetJsonObj(pstMprArray, "[$]", pstMprObj))
            {
                PRT_INFO("chan %u, %u/%u, mprSetJsonObj failed\n", chan, j, u32PackageNum);
            }
        }
    }
    else
    {
        PRT_INFO("chan %u, mprCreateJson MPR_JSON_ARRAY failed\n", chan);
    }
    free(pstPackage);

    // Add desired headers. "Set" will overwrite, add will create if not already defined.
    httpSetHeaderString(pstHttpConn, "Content-Type", "application/json");
    httpSetHeaderString(pstHttpConn, "Cache-Control", "no-cache");
    httpWrite(pstHttpConn->writeq, "%s", mprJsonToString(pstMprArray, MPR_JSON_QUOTES));
    httpWrite(pstHttpConn->writeq, "\r\n");

    httpSetStatus(pstHttpConn, HTTP_CODE_OK);
    httpFinalize(pstHttpConn);

    return;
}


/**
 * @fn      dspttk_webact_get_segment_chan0
 * @brief   web获取主通道分片信息
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_get_segment_chan0(HttpConn *pstHttpConn)
{
    dspttk_webact_get_segment(0, pstHttpConn);
}


/**
 * @fn      dspttk_webact_get_segment_chan1
 * @brief   web获取辅通道分片信息
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_get_segment_chan1(HttpConn *pstHttpConn)
{
    dspttk_webact_get_segment(1, pstHttpConn);
}


/**
 * @fn      dspttk_webact_get_packages
 * @brief   
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_get_packages_chan0(HttpConn *pstHttpConn)
{
    dspttk_webact_get_packages(0, pstHttpConn);
}


/**
 * @fn      dspttk_webact_get_packages_chan1
 * @brief   Setting栏中Dec组中底部除apply外按钮的响应回调函数 
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_get_packages_chan1(HttpConn *pstHttpConn)
{
    dspttk_webact_get_packages(1, pstHttpConn);
}


/**
 * @fn      dspttk_webact_get_save
 * @brief   Package栏目中获取save图片
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_get_save(HttpConn *pstHttpConn)
{
    UINT32 j = 0, u32SaveCount = 0;
    MprJson *pstMprObj = NULL;
    MprJson *pstMprArray = NULL;
    DSPTTK_XPACK_SAVE_PRM *pstXpackSaveInfo = NULL;

    pstXpackSaveInfo = dspttk_get_save_screen_list_info(0);
    u32SaveCount = pstXpackSaveInfo->u32SaveScreenCnt;
    if (u32SaveCount == 0)
    {
        dspttk_webact_resp(pstHttpConn, SAL_SOK, NULL);
        return;
    }

    pstMprArray = mprCreateJson(MPR_JSON_ARRAY);
    if (NULL != pstMprArray)
    {
        for (j = 0; j < u32SaveCount; j++)
        {
            pstMprObj = mprCreateJson(MPR_JSON_OBJ);
            if (NULL == pstMprObj)
            {
                PRT_INFO(" %u/%u, mprCreateJson MPR_JSON_OBJ failed\n", j, u32SaveCount);
                break;
            }
            mprWriteJson(pstMprObj, "No", sfmt("%u", j+1), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Width", sfmt("%u", pstXpackSaveInfo->stSavePicInfo[j].u32Width), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Height", sfmt("%u", pstXpackSaveInfo->stSavePicInfo[j].u32Height), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "Time", sfmt("%llu", pstXpackSaveInfo->stSavePicInfo[j].u64Time), MPR_JSON_NUMBER);
            mprWriteJson(pstMprObj, "JPG", pstXpackSaveInfo->stSavePicInfo[j].cJpgPath, MPR_JSON_STRING);
            if (0 != mprSetJsonObj(pstMprArray, "[$]", pstMprObj))
            {
                PRT_INFO("%u/%u, mprSetJsonObj failed\n", j, u32SaveCount);
            }
        }
    }
    else
    {
        PRT_INFO("mprCreateJson MPR_JSON_ARRAY failed\n");
    }

    // Add desired headers. "Set" will overwrite, add will create if not already defined.
    httpSetHeaderString(pstHttpConn, "Content-Type", "application/json");
    httpSetHeaderString(pstHttpConn, "Cache-Control", "no-cache");
    httpWrite(pstHttpConn->writeq, "%s", mprJsonToString(pstMprArray, MPR_JSON_QUOTES));
    httpWrite(pstHttpConn->writeq, "\r\n");

    httpSetStatus(pstHttpConn, HTTP_CODE_OK);
    httpFinalize(pstHttpConn);

    return;
}


/**
 * @fn      dspttk_webact_ctrl_pb_save_pic
 * @brief   web端获取当前所有save信息用于在web上显示
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * 
 * @return   none
 */
static void dspttk_webact_ctrl_pb_save_pic(HttpConn *pstHttpConn)
{
    MprJson *pstMprJson = NULL;
    SAL_STATUS sRet = SAL_SOK;
    CHAR sRespStr[256] = {0};
    CHAR *pJsonStr = NULL;
    CHAR *pStrChrName = NULL;
    UINT64 u64CmdRet = SAL_SOK;
    DSPTTK_XRAY_PB_SAVE *pstPbSave = &gstGlobalStatus.stPbInfo.stPbSave;
    if (NULL == pstHttpConn || pstHttpConn->rx->params == NULL)
    {
        PRT_INFO("save pic error: the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    pstMprJson = mprQueryJson(pstHttpConn->rx->params, "item.JPG", NULL, 0);
    pJsonStr = mprJsonToString(pstMprJson, MPR_JSON_QUOTES);
    pStrChrName = strrchr(pJsonStr, '/');
    /* 获取图像的宽高，raw域与argb域相反 */
    sscanf(pStrChrName, "/save_no%u_w%u_h%u.jpg", &pstPbSave->u32SaveNo, &pstPbSave->u32Height, &pstPbSave->u32Width);
    gstGlobalStatus.stPbInfo.enPbStatus = DSPTTK_STATUS_PB_SAVE;
    u64CmdRet =dspttk_xray_playback_right(0);
    if (0 != CMD_RET_OF(u64CmdRet)) // 命令执行失败时，返回给Web执行错误的命令号
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
        dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
        return;
    }

    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr, sizeof(sRespStr), "Playback package(%s) failed!!", pstPbSave->cNrawName);
    }

    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);

    return;
}


/**
 * @fn      dspttk_webact_ctrl_pb_package
 * @brief   Setting栏中Dec组中底部除apply外按钮的响应回调函数 
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_ctrl_pb_package(HttpConn *pstHttpConn)
{
    MprJson *pstMprJson = NULL;
    SAL_STATUS sRet = SAL_SOK;
    CHAR sRespStr[256] = {0};
    CHAR *pJsonStr = NULL;
    CHAR *pPackageId = NULL;
    UINT64 u64PackageId = 0;
    UINT64 u64CmdRet = SAL_SOK;

    if (NULL == pstHttpConn || pstHttpConn->rx->params == NULL)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    pstMprJson = mprQueryJson(pstHttpConn->rx->params, "item.JPG", NULL, 0);
    pJsonStr = mprJsonToString(pstMprJson, MPR_JSON_QUOTES);
    pPackageId = strrchr(pJsonStr, '/'); // 查找最后一级目录
    if (NULL != pPackageId)
    {
        if (NULL != strstr(pPackageId, ".jpg")) // 再次校验后缀名
        {
            u64PackageId = (UINT64)strtol(pPackageId+1, NULL, 0);
        }
    }

    if (u64PackageId > 0)
    {
        gstGlobalStatus.stPbInfo.enPbStatus = DSPTTK_STATUS_PB_PACKAGE;
        (void)dspttk_xray_playback_stop(0);
        gstGlobalStatus.stPbInfo.stPbPack.u64CurPbPackageId = u64PackageId;
        u64CmdRet = dspttk_xray_playback_right(0);
        if (0 != CMD_RET_OF(u64CmdRet)) // 命令执行失败时，返回给Web执行错误的命令号
        {
            snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
            dspttk_webact_resp(pstHttpConn, sRet, sRespStr);
            return;
        }
    }
    else
    {
        sRet = SAL_FAIL;
    }

    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr, sizeof(sRespStr), "Playback package(%llu) failed!!", u64PackageId);
    }

    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);

    return;
}


/**
 * @fn      dspttk_webact_ctrl_dec
 * @brief   Setting栏中Dec组中底部除apply外按钮的响应回调函数 
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_webact_ctrl_dec(HttpConn *pstHttpConn)
{
    UINT32 i = 0, j = 0, u32MapLen = 0;
    MprJson *pstMprJson = NULL;
    UINT64 u64CmdRet = 0;
    SAL_STATUS sRet = SAL_SOK;
    CHAR sRespStr[256] = "Command(";
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();

    /**
     * Dec栏中底部除apply外6个button无参数设置，只作button响应,因此pCfgAddr、u32VarSize和u32VarType均无效，填空即可 
     * Web下发的Json字符串格式为{"method": sFormName}，method是固定的关键字，sFormName在下面数组中定义
     */

    FORM_CFG_ADDR_MAP *pstFormCfgMap = NULL;
    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"start-*", NULL, 0, 0, 0, 0, {dspttk_dec_btn_start, '*'}},   // 开启解码
        {"pause-*", NULL, 0, 0, 0, 0, {dspttk_dec_btn_pause, '*'}},   // 暂停解码
        {"stop-*", NULL, 0, 0, 0, 0, {dspttk_dec_btn_stop, '*'}},     // 停止解码
        {"sync-*", NULL, 0, 0, 0, 0, {dspttk_dec_sync, '*'}},   //解码同步
        {"jump-*", NULL, 0, 0, 0, 0, {dspttk_dec_jump, '*'}},     // 跳转功能
    };

    if (NULL == pstHttpConn || pstHttpConn->rx->params == NULL)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);

    pstFormCfgMap = dspttk_expand_form_cfg_map(astFormCfgMap, &u32MapLen, pstDevCfg->stInitParam.stEncDec.decChanCnt, 0);
    if(NULL == pstFormCfgMap)
    {
        PRT_INFO("dspttk_expand_form_cfg_map failed.\n");
        dspttk_webact_resp(pstHttpConn, sRet, "Expand form-configure mapping table failed!\n");
        return;
    }

    for (ITERATE_JSON(pstHttpConn->rx->params, pstMprJson, j))
    {
        // 匹配Json字符串中的关键字“method”和值
        for (i = 0; i < u32MapLen; i++)
        {
            if (0 == strcmp(pstMprJson->name, "method") && 0 == strcmp(pstMprJson->value, pstFormCfgMap[i].sFormName))
            {
                u64CmdRet = pstFormCfgMap[i].stCmdApi.pFunc(pstFormCfgMap[i].stCmdApi.u32Chan);
                if (0 != CMD_RET_OF(u64CmdRet)) // 命令执行失败时，返回给Web执行错误的命令号
                {
                    // 若sRet首次出现SAL_FAIL，则直接加命令号；其他情况，先加“, ”分隔符，再加命令号
                    snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
                    sRet = SAL_FAIL;
                }
                break;
            }
            else
            {
                continue;
            }
        }
    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    free(pstFormCfgMap);
    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);

    return;
}


/**
 * @fn      dspttk_webact_ctrl_xray
 * @brief   Control栏中XRay组中按钮的响应回调函数 
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * 
 * @return   none
 */
static void dspttk_webact_ctrl_xray(HttpConn *pstHttpConn)
{
    UINT32 i = 0, j = 0;
    UINT32 u32MapLen = 0;
    MprJson *pstMprJson = NULL;
    UINT64 u64CmdRet = 0;
    SAL_STATUS sRet = SAL_SOK;
    CHAR sRespStr[256] = "Command(";

    /**
     * Control栏中只有Button响应，无参数设置，因此pCfgAddr、u32VarSize和u32VarType均无效，填空即可 
     * Web下发的Json字符串格式为{"method": sFormName}，method是固定的关键字，sFormName在下面数组中定义
     */
    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"rt-start", NULL, 0, 0, 0, 0, {dspttk_xray_rtpreview_start, 0}},
        {"stop", NULL, 0, 0, 0, 0, {dspttk_xray_all_status_stop, 0}},
        {"null", NULL, 0, 0, 0, 0, {dspttk_xray_rtpreview_stop, 0}},
        {"pb-left", NULL, 0, 0, 0, 0, {dspttk_xray_playback_left, 0}},
        {"pb-right", NULL, 0, 0, 0, 0, {dspttk_xray_playback_right, 0}},
        {"save", NULL, 0, 0, 0, 0, {dspttk_xpack_save_screen, 0}},
    };

    if (NULL == pstHttpConn || pstHttpConn->rx->params == NULL)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);
    for (ITERATE_JSON(pstHttpConn->rx->params, pstMprJson, j))
    {
        // 匹配Json字符串中的关键字“method”和值
        for (i = 0; i < u32MapLen; i++)
        {
            if (0 == strcmp(pstMprJson->name, "method") && 0 == strcmp(pstMprJson->value, astFormCfgMap[i].sFormName))
            {
                PRT_INFO("web set %s is [%s]\n", pstMprJson->name, pstMprJson->value);
                u64CmdRet = astFormCfgMap[i].stCmdApi.pFunc(astFormCfgMap[i].stCmdApi.u32Chan);
                if (0 != CMD_RET_OF(u64CmdRet)) // 命令执行失败时，返回给Web执行错误的命令号
                {
                    // 若sRet首次出现SAL_FAIL，则直接加命令号；其他情况，先加“, ”分隔符，再加命令号
                    snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
                    sRet = SAL_FAIL;
                }
                // break;
            }
            else
            {
                continue;
            }
        }
    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }
    else
    {
        sRespStr[0] = '\0';
    }

    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);

    return;
}


/**
 * @fn      dspttk_webact_ctrl_xsp
 * @brief   Xsp control栏中Xsp标签页过包按钮的响应回调函数
 *
 * @param   [IN] pstHttpConn Http连接句柄
 * 
 * @return   none
 */
static void dspttk_webact_ctrl_xsp(HttpConn *pstHttpConn)
{
    UINT32 i = 0 , j =0;
    UINT32 u32MapLen = 0;
    MprJson *pstMprJson = NULL;
    UINT64 u64CmdRet = 0;
    SAL_STATUS sRet = SAL_SOK;
    CHAR sRespStr[256] = "Command(";

    FORM_CFG_ADDR_MAP astFormCfgMap[] = {
        {"anti", NULL, 0, 0, 0, 0, {dspttk_xsp_set_anti, 0}},
        {"gray-or-color", NULL, 0, 0, 0, 0, {dspttk_xsp_set_gray_or_color, 0}},
        {"organic", NULL, 0, 0, 0, 0, {dspttk_xsp_set_organic, 0}},
        {"inorganic", NULL, 0, 0, 0, 0, {dspttk_xsp_set_inorganic, 0}},
        {"ue", NULL, 0, 0, 0, 0, {dspttk_xsp_set_ue, 0}},
        {"le", NULL, 0, 0, 0, 0, {dspttk_xsp_set_le, 0}},
        {"ee", NULL, 0, 0, 0, 0, {dspttk_xsp_set_ee, 0}},
        {"absorptance-add", NULL, 0, 0, 0, 0, {dspttk_xsp_set_absorptance_add, 0}},
        {"absorptance-sub", NULL, 0, 0, 0, 0, {dspttk_xsp_set_absorptance_sub, 0}},
        {"z789", NULL, 0, 0, 0, 0, {dspttk_xsp_set_z789, 0}},
        {"lp-or-hp", NULL, 0, 0, 0, 0, {dspttk_xsp_set_lp_or_hp, 0}},
        {"lum-add", NULL, 0, 0, 0, 0, {dspttk_xsp_set_lum_add, 0}},
        {"pseudo-color", NULL, 0, 0, 0, 0, {dspttk_xsp_pseudo_color, 0}},
        {"default-mode", NULL, 0, 0, 0, 0, {dspttk_xsp_default_style, 0}},
        // {"default-mode", NULL, 0, 0, 0, 0, {dspttk_xsp_set_default_mode, 0}},
        {"original-mode", NULL, 0, 0, 0, 0, {dspttk_xsp_set_original_mode, 0}},
        {"lum-sub", NULL, 0, 0, 0, 0, {dspttk_xsp_set_lum_sub, 0}},
    };

    if (NULL == pstHttpConn)
    {
        PRT_INFO("the pstHttpConn is NULL\n");
        return;
    }

    // 若当前设备不在运行中，直接返回失败，不设置任何参数
    if (gstGlobalStatus.enRunStatus != DSPTTK_RUNSTAT_RUNNING)
    {
        dspttk_webact_resp(pstHttpConn, SAL_FAIL, "Start device first!!");
        return;
    }

    //web参数判断
    if (pstHttpConn->rx->params == NULL)
    {
        PRT_INFO("the pstHttpConn param is NULL\n");
        return;
    }

    u32MapLen = SAL_arraySize(astFormCfgMap);

    for (ITERATE_JSON(pstHttpConn->rx->params, pstMprJson, j))
    {
        for (i = 0; i < u32MapLen; i++)
        {
            if (0 == strcmp(pstMprJson->name, "method") && 0 == strcmp(pstMprJson->value, astFormCfgMap[i].sFormName))
            {
                u64CmdRet = astFormCfgMap[i].stCmdApi.pFunc(astFormCfgMap[i].stCmdApi.u32Chan);
                if (0 != CMD_RET_OF(u64CmdRet)) // 若命令执行失败，则从备份中恢复参数，并返回错误信息给web端
                {
                    PRT_INFO("set %s error.\n", astFormCfgMap[i].sFormName);
                    sRet = SAL_FAIL;
                }
                snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), "%s%x", (SAL_SOK == sRet) ? "" : " ", (INT32)CMD_NO_OF(u64CmdRet));
                break;
            }
            else
            {
                continue;
            }
        }

    }
    if (SAL_FAIL == sRet)
    {
        snprintf(sRespStr + strlen(sRespStr), sizeof(sRespStr) - strlen(sRespStr), ") processing failed!!");
    }

    dspttk_webact_resp(pstHttpConn, sRet, sRespStr);

    return;
}


/**
 * @fn      dspttk_websse_dec_progress
 * @brief   dec_progress控制
 *
 * @param   [IN] pstHttpConn Http连接句柄
 */
static void dspttk_websse_dec_progress(INT32 s32ConnFd)
{
    UINT32 i = 0, cnt = 0;
    UINT32 au32UpFlagLocal[MAX_VDEC_CHAN] = {0}, au32UpFlagCur[MAX_VDEC_CHAN] = {0};
    SAL_STATUS sRet = SAL_FAIL;
    DSPTTK_COND_T condZero = {0};
    CHAR szHttpBuf[2048] = {0};
    CHAR *pSseConnReq = "GET /sse/dec-progress HTTP/1.1\r\n";
    CHAR *pSseConnResp = "HTTP/1.1 200 OK\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Content-Type: text/event-stream; charset=utf-8\r\n"
                         "X-Content-Type-Options: nosniff\r\n"
                         "Cache-Control: no-cache\r\n"
                         "X-XSS-Protection: 1; mode=block\r\n"
                         "Connection: Keep-Alive\r\n"
                         "Accept-Ranges: bytes\r\n"
                         "\r\n\r\n"
                         ": [server-sent event] dec progress start\n\n";

    if (0 == memcmp(&condZero, &gstDecProgressCtl.stCondDecProgress, sizeof(DSPTTK_COND_T)))
    {
        if (SAL_SOK != dspttk_cond_init(&gstDecProgressCtl.stCondDecProgress))
        {
            PRT_INFO("dspttk_cond_init 'gstDecProgressCtl.stCondDecProgress' failed\n");
            return;
        }
    }

    while (1)
    {
        sRet = dspttk_socket_recv(s32ConnFd, szHttpBuf, sizeof(szHttpBuf), SAL_TIMEOUT_FOREVER); // 接收Get请求：GET /sse/dec-progress HTTP/1.1
        if (sRet > 0 && 0 == strncmp(szHttpBuf, pSseConnReq, strlen(pSseConnReq)))
        {
            sRet = dspttk_socket_send(s32ConnFd, pSseConnResp, strlen(pSseConnResp), 3000); // 发送响应，开始server-sent event服务
            if (sRet != strlen(pSseConnResp))
            {
                PRT_INFO("dspttk_socket_send failed, ConnFd %d, Ret %d. Close it!\n", s32ConnFd, sRet);
                goto EXIT;
            }

            while (1)
            {
                dspttk_mutex_lock(&gstDecProgressCtl.stCondDecProgress.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                sRet = dspttk_cond_wait(&gstDecProgressCtl.stCondDecProgress, 100, __FUNCTION__, __LINE__);
                if (SAL_SOK == sRet)
                {
                    if (0 != memcmp(au32UpFlagLocal, gstDecProgressCtl.stDecProgress.au32UpdatedFlag, sizeof(au32UpFlagLocal))) // 有通道更新进度信息
                    {
                        memcpy(au32UpFlagCur, gstDecProgressCtl.stDecProgress.au32UpdatedFlag, sizeof(au32UpFlagCur)); // 全局参数本地化，然后释放锁
                        dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgress.mid, __FUNCTION__, __LINE__);
                        for (i = 0; i < MAX_VDEC_CHAN; i++)
                        {
                            if (au32UpFlagCur[i] != au32UpFlagLocal[i])
                            {
                                snprintf(szHttpBuf, sizeof(szHttpBuf), "event: dec-progress\ndata: {\"chan\": %u, \"totalTime\": %u, \"progress\": %u}\n\n", 
                                         i, gstDecProgressCtl.stDecProgress.u32TotalTime[i], gstDecProgressCtl.stDecProgress.au32Progress[i]);
                                sRet = dspttk_socket_send(s32ConnFd, szHttpBuf, strlen(szHttpBuf), 3000);
                                if (sRet == strlen(szHttpBuf))
                                {
                                    au32UpFlagCur[i] = au32UpFlagLocal[i];
                                }
                                else
                                {
                                    PRT_INFO("dspttk_socket_send failed, ConnFd %d, Ret %d. Close it!\n", s32ConnFd, sRet);
                                    goto EXIT;
                                }
                            }
                        }
                        cnt = 0;
                    }
                    else
                    {
                        dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgress.mid, __FUNCTION__, __LINE__);
                    }
                }
                else
                {
                    dspttk_mutex_unlock(&gstDecProgressCtl.stCondDecProgress.mid, __FUNCTION__, __LINE__);
                    if (++cnt % 30 == 0) // 3秒无数据则发一次心跳
                    {
                        snprintf(szHttpBuf, sizeof(szHttpBuf), ": [server-sent event] dec progress heartbeat %u\n\n", cnt);
                        sRet = dspttk_socket_send(s32ConnFd, szHttpBuf, strlen(szHttpBuf), 3000);
                        if (sRet != strlen(szHttpBuf))
                        {
                            PRT_INFO("dspttk_socket_send failed, ConnFd %d, Ret %d. Close it!\n", s32ConnFd, sRet);
                            goto EXIT;
                        }
                    }
                }
            }
        }
        else
        {
            if (sRet > 0) // 错误数据，重复尝试
            {
                PRT_INFO("dspttk_socket_recv failed, ConnFd %d, Ret %d, HttpRecv %s\n", s32ConnFd, sRet, szHttpBuf);
                dspttk_msleep(500);
            }
            else // 连接错误，直接跳出关闭连接
            {
                PRT_INFO("dspttk_socket_recv failed, ConnFd %d, Ret %d. Close it!\n", s32ConnFd, sRet);
                goto EXIT;
            }
        }
    }

EXIT:
    close(s32ConnFd);
    return;
}


/**
 * @fn      dspttk_websse_concentrate
 * @brief   web的server-send event请求监听线程，监听端口为8000，最多同时支持10路连接
 */
static VOID dspttk_websse_concentrate(VOID)
{
    SAL_STATUS sRet = SAL_FAIL;
    INT32 s32SockFd = 0, s32ConnFd = 0;
    SOCK_ADDR sock_addr = {0};

    sock_addr.sin4.sin_family = AF_INET;
    sock_addr.sin4.sin_port = htons(WEBAPI_EVENT_STREAM_PORT);
    sock_addr.sin4.sin_addr.s_addr = htonl(INADDR_ANY);

    s32SockFd = dspttk_socket_create(AF_INET, SOCK_STREAM);
    if (s32SockFd < 0)
    {
        PRT_INFO("dspttk_socket_create failed\n");
        return;
    }

    dspttk_socket_reuse(s32SockFd);
    dspttk_socket_ling(s32SockFd, SAL_TRUE, 3);

    sRet = dspttk_socket_bind(s32SockFd, &sock_addr);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_socket_bind failed, s32SockFd: %d\n", s32SockFd);
        goto ERR_EXIT;
    }

    sRet = dspttk_socket_listen(s32SockFd, 10);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_socket_listen failed, s32SockFd: %d\n", s32SockFd);
        goto ERR_EXIT;
    }

    while (1)
    {
        s32ConnFd = dspttk_socket_accept(s32SockFd, &sock_addr);
        if (s32ConnFd < 0)
        {
            PRT_INFO("dspttk_socket_accept faield, try again...\n");
            continue;
        }
        else
        {
            PRT_INFO("Access Point(%s @ %u) connect successfully, ConnFd: %d\n", 
                     inet_ntoa(sock_addr.sin4.sin_addr), ntohs(sock_addr.sin4.sin_port), s32ConnFd);
        }

        dspttk_pthread_spawn(NULL, DSPTTK_PTHREAD_SCHED_RR, 50, 0x100000, dspttk_websse_dec_progress, 1, s32ConnFd);
    }

ERR_EXIT:
    close(s32SockFd);
    return;
}


void dspttk_webserver(void)
{
    INT32 s32Ret = 0;
    Mpr *pstMpr = NULL;
    Http *pstHttp = NULL;

    pstMpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD);
    if (NULL == pstMpr)
    {
        PRT_INFO("oops, mprCreate failed\n");
        return;
    }

    pstHttp = httpCreate(HTTP_CLIENT_SIDE | HTTP_SERVER_SIDE);
    if (NULL == pstHttp)
    {
        PRT_INFO("oops, httpCreate failed\n");
        return;
    }

    s32Ret = mprStart();
    if (s32Ret != MPR_ERR_OK)
    {
        PRT_INFO("mprStart failed, errno: %d\n", s32Ret);
        return;
    }

    s32Ret = maParseConfig(APPWEB_CONFIG);
    if (s32Ret < 0)
    {
        PRT_INFO("maParseConfig '%s' failed, errno: %d\n", APPWEB_CONFIG, s32Ret);
        return;
    }

    httpDefineAction(WEBAPI_ACTION_START_DEV, dspttk_webact_start_dev);
    httpDefineAction(WEBAPI_ACTION_CHANGE_DEV, dspttk_webact_change_devtype);
    httpDefineAction(WEBAPI_ACTION_SET_ENV, dspttk_webact_set_env);
    httpDefineAction(WEBAPI_ACTION_SET_XRAY, dspttk_webact_set_xray);
    httpDefineAction(WEBAPI_ACTION_SET_DISP, dspttk_webact_set_disp);
    httpDefineAction(WEBAPI_ACTION_SET_OSD, dspttk_webact_set_osd);
    httpDefineAction(WEBAPI_ACTION_SET_SVA, dspttk_webact_set_sva);
    httpDefineAction(WEBAPI_ACTION_SET_XPACK, dspttk_webact_set_xpack);
    httpDefineAction(WEBAPI_ACTION_CTRL_XRAY, dspttk_webact_ctrl_xray);
    httpDefineAction(WEBAPI_ACTION_CTRL_DEC, dspttk_webact_ctrl_dec);
    httpDefineAction(WEBAPI_ACTION_CTRL_XSP, dspttk_webact_ctrl_xsp);
    httpDefineAction(WEBAPI_ACTION_SET_ENC, dspttk_webact_set_enc);
    httpDefineAction(WEBAPI_ACTION_SET_DEC, dspttk_webact_set_dec);
    httpDefineAction(WEBAPI_ACTION_GET_PACK_CH0, dspttk_webact_get_packages_chan0);
    httpDefineAction(WEBAPI_ACTION_GET_PACK_CH1, dspttk_webact_get_packages_chan1);
    httpDefineAction(WEBAPI_ACTION_GET_SAVE, dspttk_webact_get_save);
    httpDefineAction(WEBAPI_ACTION_GET_SEGMENT_CH0, dspttk_webact_get_segment_chan0);
    httpDefineAction(WEBAPI_ACTION_GET_SEGMENT_CH1, dspttk_webact_get_segment_chan1);
    httpDefineAction(WEBAPI_ACTION_CTRL_PB_PACK_CH0, dspttk_webact_ctrl_pb_package);
    httpDefineAction(WEBAPI_ACTION_CTRL_PB_PACK_CH1, dspttk_webact_ctrl_pb_package);
    httpDefineAction(WEBAPI_ACTION_CTRL_PB_SAVE, dspttk_webact_ctrl_pb_save_pic);

    dspttk_pthread_spawn(NULL, DSPTTK_PTHREAD_SCHED_RR, 50, 0x100000, dspttk_websse_concentrate, 0);
    s32Ret = httpStartEndpoints();
    if (s32Ret < 0)
    {
        PRT_INFO("httpStartEndpoints failed, errno: %d\n", s32Ret);
        return;
    }

    mprServiceEvents(-1, 0);
    mprDestroy();

    return;
}
