
/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <string.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_cjson.h"
#include "dspttk_cjson_wrap.h"
/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/**
 * @fn      dspttk_cjson_start
 * @brief   提供一个 CJSON块，然后返回一个可以查询的CJSON对象
 *
 * @param   pJsonStr[IN] 提供的 CJONS块
 *
 * @return  CJSON 成功：返回 CJSON对象 失败：返回 NULL
 */
CJSON *dspttk_cjson_start(char *pJsonStr)
{
    if (NULL != pJsonStr && strlen(pJsonStr) > 0)
    {
        return cjson_parse(pJsonStr);
    }
    else
    {
        return NULL;
    }
}

/**
 * @fn      dspttk_cjson_end
 * @brief   删除 CJSON 实体和所有子实体。
 *
 * @param   pJson[IN] 要删除的 CJONS块
 */
void dspttk_cjson_end(CJSON *pJson)
{
    if (NULL != pJson)
    {
        cjson_delete(pJson);
    }

    return;
}

/**
 * @fn      dspttk_cjson_get_object
 * @brief   从 CJSON对象指定项目中获取数据,区分项目大小写
 *
 * @param   pJson[IN]  项目所在的 CJONS块
 * @param   key[IN]    项目名
 * @param   value[OUT] 获得数据所存地址
 *
 * @return  -1: 获取失败
 */
INT32 dspttk_cjson_get_object(CJSON *pJson, char *key, void *value)
{
    int ret_val = 0;
    CJSON *pObj = NULL;

    if (NULL == pJson || NULL == key || NULL == value)
    {
        PRT_INFO("pJson(%p) OR key(%p) OR value(%p) is NULL\n", pJson, key, value);
        return -1;
    }

    pObj = cjson_getObjectItemCaseSensitive(pJson, key);

    if (NULL != pObj)
    {
        switch (pObj->type)
        {
        case CJSON_LONG:
        {
            *(long *)value = pObj->valuelong;
        }
        break;

        case CJSON_DOUBLE:
        {
            *(double *)value = pObj->valuedouble;
        }
        break;

        case CJSON_STRING:
        {
            char **pTmp = (char **)value;
            *pTmp = pObj->valuestring;
        }
        break;

        case CJSON_ARRAY:
        case CJSON_OBJECT:
        {
            CJSON **pTmp = (CJSON **)value;
            *pTmp = pObj;
        }
        break;

        case CJSON_TRUE:
        {
            *(BOOL *)value = SAL_TRUE;
            pObj->type = CJSON_BOOLEAN;
        }
        break;

        case CJSON_FALSE:
        {
            *(BOOL *)value = SAL_FALSE;
            pObj->type = CJSON_BOOLEAN;
        }
        break;

        default:
            break;
        }
        ret_val = pObj->type;
    }
    else
    {
        PRT_INFO("cjson_getObjectItem '%s' failed\n", key);
        ret_val = -1;
    }

    return ret_val;
}

/**
 * @fn      dspttk_cjson_get_array_size
 * @brief   获取 CJSON数组大小
 *
 * @param   pJson[IN]  要获取的 CJONS数组
 *
 * @return  -1: 获取失败
 */
INT32 dspttk_cjson_get_array_size(CJSON *pJson)
{
    if (NULL == pJson)
    {
        PRT_INFO("pJson is NULL\n");
        return -1;
    }

    return cjson_getArraySize(pJson);
}

/**
 * @fn      dspttk_cjson_get_array_item
 * @brief   获取 从CJSON数组中检索指定数组下标的项目。
 *
 * @param   pJson[IN]  要检索的 CJONS数组
 *
 * @return  NULL: 获取失败
 */
CJSON *dspttk_cjson_get_array_item(CJSON *pJson, UINT32 idx)
{
    INT32 arraySize = 0;

    if (NULL == pJson)
    {
        PRT_INFO("pJson is NULL\n");
        return NULL;
    }

    arraySize = dspttk_cjson_get_array_size(pJson);
    if (arraySize > 0 && idx < arraySize)
    {
        return cjson_getArrayItem(pJson, idx);
    }
    else
    {
        PRT_INFO("idx(%u) is out of bounds, arraySize: %d\n", idx, arraySize);
        return NULL;
    }
}


SAL_STATUS dspttk_cjson_get_value(CJSON *pJson, CHAR *jsonKey, CHAR *valKey, INT32 valType, VOID *pBuf, UINT32 bufLen)
{
    INT32 s32JsonType = 0;
    CJSON *pJsonGet = pJson;
    CHAR *pString = NULL;
    FLOAT32 f32Tmp = 0;

    if (NULL == pJson || NULL == valKey || NULL == pBuf)
    {
        PRT_INFO("pJson(%p) OR valKey(%p) OR pBuf(%p)\n", pJson, valKey, pBuf);
        return SAL_FAIL;
    }

    if (NULL != jsonKey && strlen(jsonKey) > 0)
    {
        s32JsonType = dspttk_cjson_get_object(pJson, jsonKey, &pJsonGet);
        if (s32JsonType != CJSON_OBJECT || NULL == pJsonGet)
        {
            PRT_INFO("dspttk_cjson_get_object '%s' failed, s32JsonType: %d, pJsonGet: %p\n", jsonKey, s32JsonType, pJsonGet);
            return SAL_FAIL;
        }
    }

    if (CJSON_STRING == valType)
    {
        s32JsonType = dspttk_cjson_get_object(pJsonGet, valKey, &pString);
        if (s32JsonType == valType && NULL != pString)
        {
            snprintf((CHAR *)pBuf, bufLen, "%s", pString);
        }
        else
        {
            PRT_INFO("dspttk_cjson_get_object '%s' failed, JsonType: %d, pString: %p\n", valKey, s32JsonType, pString);
            return SAL_FAIL;
        }
    }
    else if (CJSON_LONG == valType || CJSON_DOUBLE == valType || CJSON_BOOLEAN == valType)
    {
        s32JsonType = dspttk_cjson_get_object(pJsonGet, valKey, pBuf);
        if (CJSON_DOUBLE == valType && CJSON_LONG == s32JsonType) // 期望是double，但输出是long，做强制类型转换
        {
            f32Tmp = (FLOAT32)(*(long *)pBuf);
            *(FLOAT32 *)pBuf = f32Tmp;
        }
    }
    else
    {
        PRT_INFO("unsupport this valType: %d\n", valType);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

