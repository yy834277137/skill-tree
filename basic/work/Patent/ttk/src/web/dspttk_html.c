
/**
 * 该文件内实现的功能包括： 
 * 1、修改Html对应属性的值 
 */

/**
 * Html需满足的设计规则： 
 * 1、Element的ID可不填，若填写，则必须全局唯一，重复ID会导致定位错误；
 * 2、单选<input type="radio">对象，同一功能的name属性必须全局唯一，否则选择会错误
 *    比如灵敏度sensitivity，低、中、高三选一，则三个radio的name属性需要相同；
 */
/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include "sal_type.h"
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_devcfg.h"
#include "dspttk_fop.h"
#include "dspttk_html.h"


/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define DSPTTK_DOM_COLLECTION_LENGTH            256

typedef struct
{
    UINT32 u32HtmlTxtLen;               // HTML文本长度
    lxb_html_document_t *pHtmlDoc;      // the whole of html
    lxb_dom_element_t *pBodyElem;       // same as (lxb_html_body_element_t *)
    lxb_dom_collection_t *pDomColl;     // collection for elements
    pthread_mutex_t mutexDom;           // mutex for document objects and collection
}LXB_HTML_HANDLE;


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
VOID *gpHtmlHandle = NULL; // Html文件句柄，由dspttk_html_create_handle()返回


static LXB_HTML_HANDLE *dspttk_html_dom_create(const CHAR *pHtmlTxt, UINT32 u32HtmlLen, UINT32 u32DomCollLen)
{
    INT32 sRet = LXB_STATUS_OK;
    lxb_html_parser_t *pstParser = NULL;
    lxb_html_body_element_t *pHtmlBody = NULL;
    LXB_HTML_HANDLE *pHandle = NULL;

    if (NULL == pHtmlTxt || strlen(pHtmlTxt) < u32HtmlLen)
    {
        PRT_INFO("the 'pHtmlTxt' is invalid, size: %zu\n", (NULL != pHtmlTxt) ? strlen(pHtmlTxt) : 0);
        return NULL;
    }

    pHandle = (LXB_HTML_HANDLE *)malloc(sizeof(LXB_HTML_HANDLE));
    if (NULL == pHandle)
    {
        PRT_INFO("malloc for 'LXB_HTML_HANDLE' failed, size: %zu\n", sizeof(LXB_HTML_HANDLE));
        return NULL;
    }

    /* Initialization */
    pstParser = lxb_html_parser_create();
    if (NULL == pstParser)
    {
        PRT_INFO("lxb_html_parser_create failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = lxb_html_parser_init(pstParser);
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_html_parser_init failed, errno: %d\n", sRet);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pHandle->pHtmlDoc = lxb_html_parse(pstParser, (const lxb_char_t *)pHtmlTxt, u32HtmlLen);
    if (pHandle->pHtmlDoc != NULL)
    {
        pHandle->u32HtmlTxtLen = u32HtmlLen;
    }
    else
    {
        PRT_INFO("lxb_html_parse failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Get BODY elemenet (root for search) */
    pHtmlBody = lxb_html_document_body_element(pHandle->pHtmlDoc);
    if (NULL != pHtmlBody)
    {
        pHandle->pBodyElem = lxb_dom_interface_element(pHtmlBody);
    }
    else
    {
        PRT_INFO("lxb_html_document_body_element failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Create Collection for elements */
    pHandle->pDomColl = lxb_dom_collection_make(&pHandle->pHtmlDoc->dom_document, u32DomCollLen);
    if (pHandle->pDomColl == NULL)
    {
        PRT_INFO("lxb_dom_collection_make failed, size: %u\n", u32DomCollLen);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = dspttk_mutex_init(&pHandle->mutexDom);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_mutex_init for 'mutexDom' failed\n");
        goto EXIT;
    }

EXIT:
    /* Destroy parser */
    if (NULL != pstParser)
    {
        lxb_html_parser_destroy(pstParser);
    }
    if (sRet != SAL_SOK)
    {
        if (NULL != pHandle->pDomColl)
        {
            lxb_dom_collection_destroy(pHandle->pDomColl, SAL_TRUE);
        }
        if (NULL != pHandle->pHtmlDoc)
        {
            lxb_html_document_destroy(pHandle->pHtmlDoc);
        }
        dspttk_mutex_destroy(&pHandle->mutexDom);
        free(pHandle);
        pHandle = NULL;
    }

    return pHandle;
}


static void dspttk_html_dom_destroy(LXB_HTML_HANDLE *pHandle)
{
    if (NULL != pHandle)
    {
        if (NULL != pHandle->pDomColl)
        {
            lxb_dom_collection_destroy(pHandle->pDomColl, SAL_TRUE);
        }
        if (NULL != pHandle->pHtmlDoc)
        {
            lxb_html_document_destroy(pHandle->pHtmlDoc);
        }
        dspttk_mutex_destroy(&pHandle->mutexDom);
        free(pHandle);
    }

    return;
}


/**
 * @fn      dspttk_html_create_handle
 * @brief   创建html文件的句柄
 * 
 * @param   [IN] pHtmlPath html文件的路径
 * 
 * @return  VOID* html文件的句柄
 */
VOID *dspttk_html_create_handle(CHAR *pHtmlPath)
{
    long fileSize = 0;
    CHAR *pHtmlTxt = NULL;
    LXB_HTML_HANDLE *pHandle = NULL;

    if (NULL == pHtmlPath)
    {
        PRT_INFO("the 'pHtmlPath' is NULL\n");
        return NULL;
    }

    fileSize = dspttk_get_file_size(pHtmlPath);
    if (fileSize <= 0)
    {
        PRT_INFO("dspttk_get_file_size failed, fileSize: %ld\n", fileSize);
        return NULL;
    }

    pHtmlTxt = (CHAR *)malloc(fileSize);
    if (NULL == pHtmlTxt)
    {
        PRT_INFO("malloc for 'pHtmlTxt' failed, buffer size: %ld\n", fileSize);
        return NULL;
    }

    if (SAL_SOK != dspttk_read_file(pHtmlPath, 0, pHtmlTxt, (UINT32 *)&fileSize))
    {
        PRT_INFO("dspttk_read_file '%s' failed, fileSize: %ld\n", pHtmlPath, fileSize);
        free(pHtmlTxt);
        return NULL;
    }

    pHandle = dspttk_html_dom_create(pHtmlTxt, (UINT32)fileSize, DSPTTK_DOM_COLLECTION_LENGTH);
    free(pHtmlTxt);

    return (VOID *)pHandle;
}


/**
 * @fn      dspttk_html_destroy_handle
 * @brief   销毁html文件的句柄
 * 
 * @param   [IN] pHandle html文件的句柄
 */
void dspttk_html_destroy_handle(VOID *pHandle)
{
    dspttk_html_dom_destroy((LXB_HTML_HANDLE *)pHandle);

    return;
}


/**
 * @fn      dspttk_html_save_as
 * @brief   将html句柄保存为文本文件
 * 
 * @param   [IN] pHandle html句柄
 * @param   [IN] pHtmlPath html文件的保存路径
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS dspttk_html_save_as(VOID *pHandle, CHAR *pHtmlPath)
{
    INT32 sRet = 0;
    lxb_dom_node_t *pNode = NULL;
    lexbor_str_t stLexborStr = {0};
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHandle || NULL == pHtmlPath)
    {
        PRT_INFO("the pHandle(%p) OR pHtmlPath(%p) is NULL\n", pHandle, pHtmlPath);
        return SAL_FAIL;
    }

    stLexborStr.data = (lxb_char_t *)malloc(pHtmlHandle->u32HtmlTxtLen * 2); // 申请2倍的原HTML文本的内存
    if (NULL == stLexborStr.data)
    {
        PRT_INFO("malloc for 'stLexborStr' failed, buffer size: %u\n", pHtmlHandle->u32HtmlTxtLen * 2);
        return SAL_FAIL;
    }

    pNode = lxb_dom_interface_node(pHtmlHandle->pHtmlDoc);
    if (NULL != pNode)
    {
        sRet = lxb_html_serialize_tree_str(pNode, &stLexborStr);
        if (sRet != LXB_STATUS_OK)
        {
            PRT_INFO("lxb_html_serialize_tree_str failed, errno: %d\n", sRet);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }
    else
    {
        PRT_INFO("lxb_dom_interface_node failed\n");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = dspttk_write_file(pHtmlPath, 0, SEEK_SET, stLexborStr.data, stLexborStr.length);
    if (SAL_SOK != sRet)
    {
        PRT_INFO("dspttk_write_file '%s' failed\n", pHtmlPath);
        goto EXIT;
    }

EXIT:
    if (NULL != stLexborStr.data)
    {
        free(stLexborStr.data);
    }
    return sRet;
}


lxb_inline lxb_status_t serializer_callback(const lxb_char_t *data, size_t len, void *ctx)
{
    printf("%.*s", (int) len, (const char *) data);

    return LXB_STATUS_OK;
}


lxb_inline void serialize_node(lxb_dom_node_t *node)
{
    lxb_status_t status = LXB_STATUS_OK;

    status = lxb_html_serialize_pretty_cb(node, LXB_HTML_SERIALIZE_OPT_UNDEF, 0, serializer_callback, NULL);
    if (status != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_html_serialize_pretty_cb failed\n");
    }

    return;
}


static void print_collection_elements(lxb_dom_collection_t *pDomColl)
{
    size_t len = 0, i = 0;
    lxb_dom_element_t *pElemObj = NULL;

    printf(">>>>>>>>>>>>> current collection length: %zu\n", lxb_dom_collection_length(pDomColl));
    for (i = 0; i < lxb_dom_collection_length(pDomColl); i++)
    {
        pElemObj = lxb_dom_collection_element(pDomColl, i);
        printf("%4zu: %s, ", i, (CHAR *)lxb_dom_element_qualified_name(pElemObj, &len));
        serialize_node(lxb_dom_interface_node(pElemObj));
    }

    return;
}


static void print_element_attributes(lxb_dom_element_t *pElemObj)
{
    size_t len = 0, i = 0;
    CHAR printBuf[256] = {0};
    const lxb_char_t *pTmp = NULL;
    lxb_dom_attr_t *pDomAttr = NULL;

    printf(">>>>>>>>>>>>> qualified: %s, local: %s, prefix: %s, tag: %s\n", 
           (CHAR *)lxb_dom_element_qualified_name(pElemObj, &len), (CHAR *)lxb_dom_element_local_name(pElemObj, &len), 
           (CHAR *)lxb_dom_element_prefix(pElemObj, &len), (CHAR *)lxb_dom_element_tag_name(pElemObj, &len));
    printf("  Name                Value\n");
    pDomAttr = lxb_dom_element_first_attribute(pElemObj);
    while (pDomAttr != NULL)
    {
        pTmp = lxb_dom_attr_qualified_name(pDomAttr, &len);

        snprintf(printBuf, sizeof(printBuf), "  %s", pTmp);
        len = SAL_SUB_SAFE(20, len);
        for (i = 0; i < len; i++)
        {
            strcat(printBuf, " ");
        }

        pTmp = lxb_dom_attr_value(pDomAttr, &len);
        if (pTmp != NULL)
        {
            strcat(printBuf, (CHAR *)pTmp);
        }
        printf("%s\n", printBuf);
        pDomAttr = lxb_dom_element_next_attribute(pDomAttr);
    }

    return;
}


static BOOL dspttk_dom_element_has_attribute(lxb_dom_element_t *pElemObj, CHAR *pAttrName)
{
    size_t len = 0;
    const lxb_char_t *pTmp = NULL;
    lxb_dom_attr_t *pDomAttr = NULL;

    if (NULL == pElemObj || NULL == pAttrName || 0 == strlen(pAttrName))
    {
        return SAL_FALSE;
    }

    pDomAttr = lxb_dom_element_first_attribute(pElemObj);
    while (pDomAttr != NULL)
    {
        pTmp = lxb_dom_attr_qualified_name(pDomAttr, &len);
        if (NULL != pTmp && 0 == strcmp(pAttrName, (CHAR *)pTmp))
        {
            return SAL_TRUE;
        }
        pDomAttr = lxb_dom_element_next_attribute(pDomAttr);
    }

    return SAL_FALSE;
}


static void dspttk_dom_element_remove_attribute(lxb_dom_element_t *pElemObj, CHAR *pAttrName)
{
    size_t len = 0;
    const lxb_char_t *pTmp = NULL;
    lxb_dom_attr_t *pDomAttr = NULL;

    if (NULL == pElemObj || NULL == pAttrName || 0 == strlen(pAttrName))
    {
        return;
    }

    pDomAttr = lxb_dom_element_first_attribute(pElemObj);
    while (pDomAttr != NULL)
    {
        pTmp = lxb_dom_attr_qualified_name(pDomAttr, &len);
        if (NULL != pTmp && 0 == strcmp(pAttrName, (CHAR *)pTmp))
        {
            lxb_dom_element_attr_remove(pElemObj, pDomAttr);
        }
        pDomAttr = lxb_dom_element_next_attribute(pDomAttr);
    }

    return;
}


lxb_dom_element_t *dspttk_element_query_by_id(LXB_HTML_HANDLE *pHandle, CHAR *pIdValue)
{
    INT32 sRet = 0;
    CHAR *pIdKey = "id";

    if (NULL == pHandle || NULL == pIdValue)
    {
        PRT_INFO("the pHandle(%p) OR pIdValue(%p) is NULL\n", pHandle, pIdValue);
        return NULL;
    }
    if (0 == strlen(pIdValue))
    {
        PRT_INFO("the IdValue is Empty\n");
        return NULL;
    }

    lxb_dom_collection_clean(pHandle->pDomColl); // 先清空collection
    sRet = lxb_dom_elements_by_attr(pHandle->pBodyElem, pHandle->pDomColl, (const lxb_char_t *)pIdKey, strlen(pIdKey), 
                                    (const lxb_char_t *)pIdValue, strlen(pIdValue), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, IdValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHandle->pDomColl), pIdValue);
    }

    return lxb_dom_collection_element(pHandle->pDomColl, 0);
}


SAL_STATUS dspttk_element_set_attr(lxb_dom_element_t *pElemObj, CHAR *pAttrKey, CHAR *pAttrValue)
{
    INT32 sRet = 0;
    BOOL bExist = SAL_FALSE;
    lxb_dom_attr_t *pDomAttr = NULL;

    /* Check exist first */
    bExist = dspttk_dom_element_has_attribute(pElemObj, pAttrKey);
    if (bExist) // 存在则直接修改
    {
        pDomAttr = lxb_dom_element_attr_by_name(pElemObj, (const lxb_char_t *)pAttrKey, strlen(pAttrKey));
        if (NULL == pDomAttr)
        {
            PRT_INFO("lxb_dom_element_attr_by_name failed, AttrKey: %s\n", pAttrKey);
            return SAL_FAIL;
        }
        sRet = lxb_dom_attr_set_value(pDomAttr, (const lxb_char_t *)pAttrValue, strlen(pAttrValue));
        if (sRet != LXB_STATUS_OK)
        {
            PRT_INFO("lxb_dom_attr_set_value failed, AttrKey: %s, AttrValue: %s\n", pAttrKey, pAttrValue);
            return SAL_FAIL;
        }
    }
    else // 不存在则创建
    {
        /* Append new attrtitube */
        pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)pAttrKey, strlen(pAttrKey), 
                                                 (const lxb_char_t *)pAttrValue, strlen(pAttrValue));
        if (pDomAttr == NULL)
        {
            PRT_INFO("lxb_dom_element_set_attribute failed, AttrKey: %s, AttrValue: %s\n", pAttrKey, pAttrValue);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_html_set_attr_by_id
 * @brief   通过Element ID，设置该Element的属性值 
 * @note    常用于设置：<input type="number">中的value属性
 *                    <input type="checkbox">中的checked属性
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pIdValue Element ID号，必须全局唯一
 * @param   [IN] pAttrKey 属性关键字，比如“value”、“checked”等
 * @param   [IN] pAttrValue 属性值，字符串格式，比如value的"0.2"，checked的""
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_attr_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pAttrKey, CHAR *pAttrValue)
{
    INT32 sRet = 0;
    BOOL bExist = SAL_FALSE;
    lxb_dom_attr_t *pDomAttr = NULL;
    const CHAR *pIdKey = "id";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pIdValue)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pIdValue(%p) is NULL\n", pHtmlHandle, pIdValue);
        return SAL_FAIL;
    }
    if (NULL == pAttrKey || NULL == pAttrValue)
    {
        PRT_INFO("the pAttrKey(%p) OR pAttrValue(%p) is NULL\n", pAttrKey, pAttrValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pIdKey, strlen(pIdKey), 
                                    (const lxb_char_t *)pIdValue, strlen(pIdValue), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, IdValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pIdValue);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check exist first */
    bExist = dspttk_dom_element_has_attribute(pElemObj, pAttrKey);
    if (bExist) // 存在则直接修改
    {
        pDomAttr = lxb_dom_element_attr_by_name(pElemObj, (const lxb_char_t *)pAttrKey, strlen(pAttrKey));
        if (NULL == pDomAttr)
        {
            PRT_INFO("lxb_dom_element_attr_by_name failed, AttrKey: %s %s\n", pAttrKey, pIdValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
        sRet = lxb_dom_attr_set_value(pDomAttr, (const lxb_char_t *)pAttrValue, strlen(pAttrValue));
        if (sRet != LXB_STATUS_OK)
        {
            PRT_INFO("lxb_dom_attr_set_value failed, AttrKey: %s, AttrValue: %s\n", pAttrKey, pAttrValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }
    else // 不存在则创建
    {
        /* Append new attrtitube */
        pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)pAttrKey, strlen(pAttrKey), 
                                                 (const lxb_char_t *)pAttrValue, strlen(pAttrValue));
        if (pDomAttr == NULL)
        {
            PRT_INFO("lxb_dom_element_set_attribute failed, AttrKey: %s, AttrValue: %s\n", pAttrKey, pAttrValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_del_attr_by_id
 * @brief   通过Element ID，删除该Element的属性值 
 * @note    常用于删除：<input type="checkbox">中的checked属性
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pIdValue Element ID号，必须全局唯一
 * @param   [IN] pAttrKey 属性关键字，比如“checked”等
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_del_attr_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pAttrKey)
{
    INT32 sRet = 0;
    BOOL bExist = SAL_FALSE;
    const CHAR *pIdKey = "id";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pIdValue)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pIdValue(%p) is NULL\n", pHtmlHandle, pIdValue);
        return SAL_FAIL;
    }
    if (NULL == pAttrKey)
    {
        PRT_INFO("the pAttrKey is NULL\n");
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pIdKey, strlen(pIdKey), 
                                    (const lxb_char_t *)pIdValue, strlen(pIdValue), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, IdValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pIdValue);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check exist first */
    bExist = dspttk_dom_element_has_attribute(pElemObj, pAttrKey);
    if (bExist) // 存在则删除
    {
        dspttk_dom_element_remove_attribute(pElemObj, pAttrKey);
    }
    else
    {
        sRet = SAL_FAIL;
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_text_by_id
 * @brief   通过Element ID，设置该Element的文本 
 * @note    可用于设置：<button type="number">Text</button>中的Text内容
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pIdValue Element ID号，必须全局唯一
 * @param   [IN] pTextValue 文本内容，字符串格式
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_text_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pTextValue)
{
    INT32 sRet = 0;
    const CHAR *pIdKey = "id";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pIdValue)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pIdValue(%p) is NULL\n", pHtmlHandle, pIdValue);
        return SAL_FAIL;
    }
    if (NULL == pTextValue || 0 == strlen(pTextValue))
    {
        PRT_INFO("the pTextValue(%p) is invalid\n", pTextValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pIdKey, strlen(pIdKey), 
                                    (const lxb_char_t *)pIdValue, strlen(pIdValue), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, IdValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pIdValue);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    sRet = lxb_dom_node_text_content_set(&pElemObj->node, (const lxb_char_t *)pTextValue, strlen(pTextValue));
    if (0 != sRet)
    {
        PRT_INFO("lxb_dom_node_text_content_set failed, IdValue: %s, TextValue: %s\n", pIdValue, pTextValue);
        sRet = SAL_FAIL;
        goto EXIT;
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_style_by_id
 * @brief   通过Element ID，设置该Element的style属性值 
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pIdValue Element ID号，必须全局唯一
 * @param   [IN] pAttrKey 属性关键字，比如“opacity”、“blur”等
 * @param   [IN] pAttrValue 属性值，字符串格式
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_style_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pAttrKey, CHAR *pAttrValue)
{
    INT32 sRet = 0;
    size_t len = 0;
    BOOL bExist = SAL_FALSE;
    lxb_dom_attr_t *pDomAttr = NULL;
    CHAR sStyleValue[128] = {0}, sSubAttrKey[64] = {0};
    CHAR *pHitSub = NULL, *pSemicolon = NULL; // pHitSub：命中的子字符串，pSemicolon：分号
    const CHAR *pIdKey = "id";
    const CHAR *pStyleKey = "style";
    const lxb_char_t *pLxbCharTmp = NULL;
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pIdValue)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pIdValue(%p) is NULL\n", pHtmlHandle, pIdValue);
        return SAL_FAIL;
    }
    if (NULL == pAttrKey || NULL == pAttrValue)
    {
        PRT_INFO("the pAttrKey(%p) OR pAttrValue(%p) is NULL\n", pAttrKey, pAttrValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pIdKey, strlen(pIdKey), 
                                    (const lxb_char_t *)pIdValue, strlen(pIdValue), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, IdValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pIdValue);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check exist first */
    bExist = dspttk_dom_element_has_attribute(pElemObj, (CHAR *)pStyleKey);
    if (bExist) // 存在则直接修改
    {
        pDomAttr = lxb_dom_element_attr_by_name(pElemObj, (const lxb_char_t *)pStyleKey, strlen(pStyleKey));
        if (NULL == pDomAttr)
        {
            PRT_INFO("lxb_dom_element_attr_by_name failed, AttrKey: %s\n", pStyleKey);
            sRet = SAL_FAIL;
            goto EXIT;
        }
        pLxbCharTmp = lxb_dom_attr_value(pDomAttr, &len);
        if (pLxbCharTmp != NULL)
        {
            snprintf(sSubAttrKey, sizeof(sSubAttrKey), "%s:", pAttrKey);
            pHitSub = strcasestr((CHAR *)pLxbCharTmp, sSubAttrKey); // 先定位子属性的Key
            if (NULL != pHitSub)
            {
                pSemicolon = strchr(pHitSub, ';'); // 再定位子属性的结束位置
                if (NULL != pSemicolon)
                {
                    // 拼接整个style属性
                    strncpy(sStyleValue, (CHAR *)pLxbCharTmp, pHitSub - (CHAR *)pLxbCharTmp);
                    snprintf(sStyleValue+strlen(sStyleValue), sizeof(sStyleValue)-strlen(sStyleValue), "%s %s%s", sSubAttrKey, pAttrValue, pSemicolon);
                }
                else // 本身子属性已错误，不再处理
                {
                    PRT_INFO("sub attribute(%s) end without ';'\n", pHitSub);
                    sRet = SAL_FAIL;
                    goto EXIT;
                }
            }
            else // 无该子属性，在最后添加
            {
                snprintf(sStyleValue, sizeof(sStyleValue), "%s%s %s;", (CHAR *)pLxbCharTmp, sSubAttrKey, pAttrValue);
            }
        }
        else
        {
            PRT_INFO("lxb_dom_attr_value failed\n");
            sRet = SAL_FAIL;
            goto EXIT;
        }

        sRet = lxb_dom_attr_set_value(pDomAttr, (const lxb_char_t *)sStyleValue, strlen(sStyleValue));
        if (sRet != LXB_STATUS_OK)
        {
            PRT_INFO("lxb_dom_attr_set_value failed, AttrKey: %s, AttrValue: %s\n", pStyleKey, sStyleValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }
    else // 不存在则创建
    {
        /* Append new attrtitube */
        snprintf(sStyleValue, sizeof(sStyleValue), "%s: %s;", pAttrKey, pAttrValue);
        pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)pStyleKey, strlen(pStyleKey), 
                                                 (const lxb_char_t *)sStyleValue, strlen(sStyleValue));
        if (pDomAttr == NULL)
        {
            PRT_INFO("lxb_dom_element_set_attribute failed, AttrKey: %s, AttrValue: %s\n", sSubAttrKey, sStyleValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_attr_by_name
 * @brief   通过Element Name，设置该Element的属性值 
 * @note    常用于设置：<input type="number">中的value属性
 *                    <input type="checkbox">中的checked属性
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pNameValue Element Name，必须全局唯一
 * @param   [IN] pAttrKey 属性关键字，比如“value”、“checked”等
 * @param   [IN] pAttrValue 属性值，字符串格式，比如value的"0.2"，checked的""
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_attr_by_name(VOID *pHandle, CHAR *pNameValue, CHAR *pAttrKey, CHAR *pAttrValue)
{
    INT32 sRet = 0;
    BOOL bExist = SAL_FALSE;
    lxb_dom_attr_t *pDomAttr = NULL;
    const CHAR *pNameKey = "name";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pNameValue)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pNameValue(%p) is NULL\n", pHtmlHandle, pNameValue);
        return SAL_FAIL;
    }
    if (NULL == pAttrKey || NULL == pAttrValue)
    {
        PRT_INFO("the pAttrKey(%p) OR pAttrValue(%p) is NULL\n", pAttrKey, pAttrValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pNameKey, strlen(pNameKey), 
                                    (const lxb_char_t *)pNameValue, strlen(pNameValue), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, NameValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pNameValue);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check exist first */
    bExist = dspttk_dom_element_has_attribute(pElemObj, pAttrKey);
    if (bExist) // 存在则直接修改
    {
        pDomAttr = lxb_dom_element_attr_by_name(pElemObj, (const lxb_char_t *)pAttrKey, strlen(pAttrKey));
        if (NULL == pDomAttr)
        {
            PRT_INFO("lxb_dom_element_attr_by_name failed, AttrKey: %s\n", pAttrKey);
            sRet = SAL_FAIL;
            goto EXIT;
        }
        sRet = lxb_dom_attr_set_value(pDomAttr, (const lxb_char_t *)pAttrValue, strlen(pAttrValue));
        if (sRet != LXB_STATUS_OK)
        {
            PRT_INFO("lxb_dom_attr_set_value failed, AttrKey: %s, AttrValue: %s\n", pAttrKey, pAttrValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }
    else // 不存在则创建
    {
        /* Append new attrtitube */
        pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)pAttrKey, strlen(pAttrKey), 
                                                 (const lxb_char_t *)pAttrValue, strlen(pAttrValue));
        if (pDomAttr == NULL)
        {
            PRT_INFO("lxb_dom_element_set_attribute failed, AttrKey: %s, AttrValue: %s\n", pAttrKey, pAttrValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_select_range
 * @brief   通过DOM的name值，设置下拉菜单选项范围
 * @note    单选，从多个值中选择多值 
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pSelectName 下拉菜单选择框name值，必须全局唯一
 * @param   [IN] pOptionValue 期望选中的值
 * @param   [IN] u32Lens range长度
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_select_range(VOID *pHandle, CHAR *pSelectName, CHAR pOptionValue[][16], UINT32 u32Lens)
{
    INT32 sRet = 0;
    UINT32 u32CollOffset = 0, j = 0;
    CHAR *pStr = NULL;
    size_t strLen = 0, i = 0;
    const CHAR *pNameKey = "name";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle)
    {
        PRT_INFO("the pHtmlHandle is NULL\n");
        return SAL_FAIL;
    }
    if (NULL == pSelectName || NULL == pOptionValue)
    {
        PRT_INFO("the pSelectName(%p) OR pOptionValue(%p) is NULL\n", pSelectName, pOptionValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pNameKey, strlen(pNameKey), 
                                    (const lxb_char_t *)pSelectName, strlen(pSelectName), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, SelectName: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pSelectName);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check tag is <select> */
    pStr = (CHAR *)lxb_dom_element_qualified_name(pElemObj, &strLen);
    if (0 != strncmp(pStr, "select", strLen))
    {
        PRT_INFO("the element tag is '%s', should be 'select'\n", pStr);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect tag which is <option> */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_tag_name(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"option", strlen("option"));
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_tag_name 'option' failed, errno: %d\n", sRet);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    for (i = u32CollOffset; i < lxb_dom_collection_length(pHtmlHandle->pDomColl); i++)
    {
        pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, i);
        pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"value", strlen("value"), &strLen);
        if (pStr != NULL)
        {
            dspttk_dom_element_remove_attribute(pElemObj, "disabled");
        }
        else
        {
            PRT_INFO("dspttk_dom_element_remove_attribute 'value' failed\n");
        }
    }

    for (i = u32CollOffset; i < lxb_dom_collection_length(pHtmlHandle->pDomColl); i++)
    {
        pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, i);
        pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"value", strlen("value"), &strLen);
        if (pStr != NULL)
        {
                lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)"disabled", strlen("disabled"), 
                                (const lxb_char_t *)"", 0);
        }
        else
        {
            PRT_INFO("lxb_dom_element_get_attribute 'value' failed\n");
        }
    }

    for ( j = 0; j < u32Lens; j++)
    {
        if (pOptionValue[j] == NULL || 0 == strcmp(pOptionValue[j],""))
        {
            continue;;
        }

        for (i = u32CollOffset; i < lxb_dom_collection_length(pHtmlHandle->pDomColl); i++)
        {
            pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, i);
            pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"value", strlen("value"), &strLen);
            if (pStr != NULL)
            {
                // PRT_INFO("pStr %s pOptionValue[%d] [%s]\n", pStr, j , pOptionValue[j]);
                if (0 == strcmp(pStr, pOptionValue[j]))
                {
                    dspttk_dom_element_remove_attribute(pElemObj, "disabled");
                }
            }
            else
            {
                PRT_INFO("lxb_dom_element_get_attribute 'value' failed\n");
            }
        }
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}



/**
 * @fn      dspttk_html_set_select_by_name
 * @brief   通过DOM的name值，设置下拉菜单选择的值 
 * @note    单选，从多个值中选择一个值 
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pSelectName 下拉菜单选择框name值，必须全局唯一
 * @param   [IN] pOptionValue 期望选中的值
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_select_by_name(VOID *pHandle, CHAR *pSelectName, CHAR *pOptionValue)
{
    INT32 sRet = 0;
    BOOL bExist = SAL_FALSE, bMatched = SAL_FALSE;
    UINT32 u32CollOffset = 0;
    CHAR *pStr = NULL;
    size_t strLen = 0, i = 0;
    const CHAR *pNameKey = "name";
    CHAR sOptionVaues[128] = {0};
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle)
    {
        PRT_INFO("the pHtmlHandle is NULL\n");
        return SAL_FAIL;
    }
    if (NULL == pSelectName || NULL == pOptionValue)
    {
        PRT_INFO("the pSelectName(%p) OR pOptionValue(%p) is NULL\n", pSelectName, pOptionValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pNameKey, strlen(pNameKey), 
                                    (const lxb_char_t *)pSelectName, strlen(pSelectName), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, SelectName: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pSelectName);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check tag is <select> */
    pStr = (CHAR *)lxb_dom_element_qualified_name(pElemObj, &strLen);
    if (0 != strncmp(pStr, "select", strLen))
    {
        PRT_INFO("the element tag is '%s', should be 'select'\n", pStr);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect tag which is <option> */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_tag_name(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"option", strlen("option"));
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_tag_name 'option' failed, errno: %d\n", sRet);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* 遍历所有的<option>项，删除旧的‘selected’属性，并添加新的‘selected’属性 */
    for (i = u32CollOffset; i < lxb_dom_collection_length(pHtmlHandle->pDomColl); i++)
    {
        pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, i);
        bExist = dspttk_dom_element_has_attribute(pElemObj, "selected");
        if (bExist) // 存在则删除
        {
            dspttk_dom_element_remove_attribute(pElemObj, "selected");
        }
        // 获取value属性，判断是否与pOptionValue一致，一致则添加新的‘selected’属性
        if (!bMatched)
        {
            pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"value", strlen("value"), &strLen);
            if (pStr != NULL)
            {
                if (0 == strcmp(pStr, pOptionValue))
                {
                    lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)"selected", strlen("selected"), 
                                                  (const lxb_char_t *)"", 0);
                    bMatched = SAL_TRUE;
                    sRet = SAL_SOK;
                }
                else
                {
                    snprintf(sOptionVaues+strlen(sOptionVaues), sizeof(sOptionVaues)-strlen(sOptionVaues), "%s, ", pStr);
                }
            }
            else
            {
                PRT_INFO("lxb_dom_element_get_attribute 'value' failed\n");
            }
        }
    }
    if (!bMatched)
    {
        PRT_INFO("select name %s, cannot find matched option %s in {%s}\n", pSelectName, pOptionValue, sOptionVaues);
        sRet = SAL_FAIL;
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_radio_by_name
 * @brief   通过DOM的name值，设置单选框的选中项 
 * @note    单选，从多个值中选择一个值 
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pRadioName 单选框name值，必须全局唯一
 * @param   [IN] pCheckedValue 期望选中的项
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_radio_by_name(VOID *pHandle, CHAR *pRadioName, CHAR *pCheckedValue)
{
    INT32 sRet = 0;
    BOOL bExist = SAL_FALSE, bMatched = SAL_FALSE;
    UINT32 u32CollOffset = 0;
    CHAR *pStr = NULL;
    size_t strLen = 0, i = 0;
    const CHAR *pNameKey = "name";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle)
    {
        PRT_INFO("the pHtmlHandle is NULL\n");
        return SAL_FAIL;
    }
    if (NULL == pRadioName || NULL == pCheckedValue)
    {
        PRT_INFO("the pRadioName(%p) OR pCheckedValue(%p) is NULL\n", pRadioName, pCheckedValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pNameKey, strlen(pNameKey), 
                                    (const lxb_char_t *)pRadioName, strlen(pRadioName), SAL_TRUE);
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, RadioName: %s\n", sRet, pRadioName);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu %s\n", lxb_dom_collection_length(pHtmlHandle->pDomColl), pRadioName);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check tag is <input> */
    pStr = (CHAR *)lxb_dom_element_qualified_name(pElemObj, &strLen);
    if (0 != strncmp(pStr, "input", strLen))
    {
        PRT_INFO("the element tag is '%s', should be 'input'\n", pStr);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect tag which is <option> */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_tag_name(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"option", strlen("option"));
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_tag_name 'option' failed, errno: %d\n", sRet);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* 遍历所有的‘radio’项，删除旧的‘checked’属性，并添加新的‘checked’属性 */
    for (i = 0; i < u32CollOffset; i++)
    {
        pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, i);

        /* Check tag is 'input' and type is 'radio' */
        pStr = (CHAR *)lxb_dom_element_qualified_name(pElemObj, &strLen);
        if (0 != strncmp(pStr, "input", strLen))
        {
            PRT_INFO("the %s tag is '%s' value is %s, should be 'input'\n", pRadioName, pStr, pCheckedValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }

        pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"type", strlen("type"), &strLen);
        if (pStr != NULL && 0 == strcmp(pStr, "radio"))
        {
            bExist = dspttk_dom_element_has_attribute(pElemObj, "checked");
            if (bExist) // 存在则删除
            {
                dspttk_dom_element_remove_attribute(pElemObj, "checked");
            }
            // 获取value属性，判断是否与pCheckedValue一致，一致则添加新的‘checked’属性
            if (!bMatched)
            {
                pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"value", strlen("value"), &strLen);
                if (pStr != NULL)
                {
                    if (0 == strcmp(pStr, pCheckedValue))
                    {
                        lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)"checked", strlen("checked"), 
                                                      (const lxb_char_t *)"", 0);
                        bMatched = SAL_TRUE;
                        sRet = SAL_SOK;
                    }
                }
                else
                {
                    PRT_INFO("lxb_dom_element_get_attribute 'value' failed\n");
                }
            }
        }
        else
        {
            PRT_INFO("lxb_dom_element_get_attribute 'type' failed, pStr: %s\n", (pStr != NULL) ? pStr : "NULL");
        }
    }
    if (!bMatched)
    {
        PRT_INFO("error bMatched %s\n", pRadioName);
        sRet = SAL_FAIL;
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_checkbox_by_name
 * @brief   通过DOM的name值，设置复选框是否选中 
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pCheckboxName 复选框name值，必须全局唯一
 * @param   [IN] bChecked 是否选中
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_checkbox_by_name(VOID *pHandle, CHAR *pCheckboxName, BOOL bChecked)
{
    INT32 sRet = 0;
    UINT32 i = 0, u32CollCnt = 0;
    size_t len = 0;
    CHAR *pStr = NULL;
    size_t strLen = 0;
    lxb_dom_attr_t *pDomAttr = NULL;
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pCheckboxName)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pCheckboxName(%p) is NULL\n", pHtmlHandle, pCheckboxName);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)"name", strlen("name"), 
                                    (const lxb_char_t *)pCheckboxName, strlen(pCheckboxName), SAL_TRUE);
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, IdValue: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pCheckboxName);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    u32CollCnt = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    for (i = 0; i < u32CollCnt; i++)
    {
        pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, i);
        if (NULL == pElemObj)
        {
            PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
            sRet = SAL_FAIL;
            goto EXIT;
        }

        /* Check tag is <input> and type is 'checkbox' */
        pStr = (CHAR *)lxb_dom_element_qualified_name(pElemObj, &strLen);
        if (0 != strncmp(pStr, "input", strLen))
        {
            PRT_INFO("the element tag is '%s', should be 'input'\n", pStr);
            continue;
        }

        pStr = (CHAR *)lxb_dom_element_get_attribute(pElemObj, (const lxb_char_t *)"type", strlen("type"), &strLen);
        if (pStr != NULL && 0 == strcmp(pStr, "checkbox"))
        {
            /* 遍历所有的属性项，删除所有的‘checked’属性 */
            pDomAttr = lxb_dom_element_first_attribute(pElemObj);
            while (pDomAttr != NULL)
            {
                if (0 == strcmp("checked", (CHAR *)lxb_dom_attr_qualified_name(pDomAttr, &len)))
                {
                    lxb_dom_element_attr_remove(pElemObj, pDomAttr);
                }
                pDomAttr = lxb_dom_element_next_attribute(pDomAttr);
            }

            if (bChecked) // 选中则添加checked属性
            {
                pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)"checked", strlen("checked"),
                                                         (const lxb_char_t *)"", 0);
                if (pDomAttr == NULL)
                {
                    PRT_INFO("lxb_dom_element_set_attribute failed, Key: checked, Value: Null\n");
                    sRet = SAL_FAIL;
                }
                else
                {
                    sRet = SAL_SOK;
                }
            }
            break;
        }
    }
    if (i == u32CollCnt) // unmatched
    {
        // PRT_INFO("unmatch checkbox, Checkbox Name: %s\n", pCheckboxName);
        print_collection_elements(pHtmlHandle->pDomColl);
        sRet = SAL_FAIL;
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_table_cell_inputs
 * @brief   设置Table表中指定单元格中多个文本输入框的value属性
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pTableId Table ID号，必须全局唯一
 * @param   [IN] u32RowIdx 单元格横向索引，从1开始
 * @param   [IN] u32CellIdx 单元格纵向索引，从1开始
 * @param   [IN] pInputValue 输入框value属性值数组
 * @param   [IN] u32InputCnt 输入框value属性值数组个数
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_table_cell_inputs(VOID *pHandle, CHAR *pTableId, UINT32 u32RowIdx, UINT32 u32CellIdx, CHAR *pInputValue[], UINT32 u32InputCnt)
{
    INT32 sRet = 0;
    UINT32 u32RowCnt = 0, u32CellCnt = 0, u32CollOffset = 0;
    UINT32 i = 0, u32Len = 0;
    BOOL bExist = SAL_FALSE;
    lxb_dom_attr_t *pDomAttr = NULL;
    const CHAR *pIdKey = "id";
    const CHAR *pClassKey = "class";
    const CHAR *pInputKey = "value";
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pTableId)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pTableId(%p) is NULL\n", pHtmlHandle, pTableId);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)pIdKey, strlen(pIdKey), 
                                    (const lxb_char_t *)pTableId, strlen(pTableId), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, pTableId: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pTableId);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check class contain 'table' */
    bExist = lxb_dom_elements_by_attr_contain(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)pClassKey, strlen(pClassKey),
                                              (const lxb_char_t *)"table", sizeof("table"), SAL_TRUE);
    if (bExist != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_attr_contain 'table' failed, AttrKey: %s\n", pClassKey);
        print_element_attributes(pElemObj);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect table row: <tr> */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_tag_name(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"tr", strlen("tr"));
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_tag_name 'tr' failed, errno: %d\n", sRet);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    u32RowCnt = lxb_dom_collection_length(pHtmlHandle->pDomColl) - u32CollOffset;
    if (u32RowCnt == u32CollOffset || u32RowIdx >= u32RowCnt)
    {
        PRT_INFO("the length of 'tr' collection OR RowIdx(%u) is invalid, CollOffset: %u, RowCnt: %u\n", 
                 u32RowIdx, u32CollOffset, u32RowCnt);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, u32RowIdx + u32CollOffset);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, idx: %u, length: %u\n", u32RowIdx, u32RowCnt - u32CollOffset);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect table data: <td> */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_tag_name(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"td", strlen("td"));
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_tag_name 'td' failed, errno: %d\n", sRet);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    u32CellCnt = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    if (u32CellCnt == u32CollOffset || u32CellIdx >= u32CellCnt - u32CollOffset)
    {
        PRT_INFO("the length of 'td' collection OR CellIdx(%u) is invalid, CollOffset: %u, CellCnt: %u\n", 
                 u32CellIdx, u32CollOffset, u32CellCnt);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, u32CellIdx + u32CollOffset);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, idx: %u, length: %u\n", u32CellIdx, u32CellCnt - u32CollOffset);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect <input> by tag */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_tag_name(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"input", strlen("input"));
    if (sRet != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_tag_name 'input' failed, errno: %d\n", sRet);
        print_collection_elements(pHtmlHandle->pDomColl);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    u32Len = lxb_dom_collection_length(pHtmlHandle->pDomColl) - u32CollOffset;
    u32Len = SAL_MIN(u32Len, u32InputCnt);
    for (i = 0; i < u32Len; i++)
    {
        pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, u32CollOffset+i);
        if (NULL == pElemObj)
        {
            PRT_INFO("lxb_dom_collection_element failed, idx: %u, length: %zu\n", u32CollOffset+i, lxb_dom_collection_length(pHtmlHandle->pDomColl));
            sRet = SAL_FAIL;
            goto EXIT;
        }

        /* Modify <input> 'value', check exist first */
        bExist = dspttk_dom_element_has_attribute(pElemObj, (CHAR *)pInputKey);
        if (bExist) // 存在则直接修改
        {
            pDomAttr = lxb_dom_element_attr_by_name(pElemObj, (const lxb_char_t *)pInputKey, strlen(pInputKey));
            if (NULL == pDomAttr)
            {
                PRT_INFO("lxb_dom_element_attr_by_name failed, InputKey: %s\n", pInputKey);
                print_element_attributes(pElemObj);
                sRet = SAL_FAIL;
                goto EXIT;
            }

            sRet = lxb_dom_attr_set_value(pDomAttr, (const lxb_char_t *)pInputValue[i], strlen(pInputValue[i]));
            if (sRet != LXB_STATUS_OK)
            {
                PRT_INFO("lxb_dom_attr_set_value failed, InputKey: %s, InputValue: %s\n", pInputKey, pInputValue[i]);
                sRet = SAL_FAIL;
                goto EXIT;
            }
        }
        else // 不存在则创建
        {
            /* Append new attrtitube */
            pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)pInputKey, strlen(pInputKey), 
                                                     (const lxb_char_t *)pInputValue[i], strlen(pInputValue[i]));
            if (pDomAttr == NULL)
            {
                PRT_INFO("lxb_dom_element_set_attribute failed, InputKey: %s, AttrValue: %s\n", pInputKey, pInputValue[i]);
                sRet = SAL_FAIL;
                goto EXIT;
            }
        }
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      dspttk_html_set_table_input_by_name
 * @brief   设置Table表中指定名字文本输入框的value属性
 * 
 * @param   [IN] pHandle Html句柄
 * @param   [IN] pTableId Table ID号，必须全局唯一
 * @param   [IN] pInputName文本输入框的name属性值
 * @param   [IN] pInputValue 输入框value属性值
 * 
 * @return  SAL_STATUS SAL-SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_html_set_table_input_by_name(VOID *pHandle, CHAR *pTableId, CHAR *pInputName, CHAR *pInputValue)
{
    INT32 sRet = 0;
    UINT32 u32CollOffset = 0;
    BOOL bExist = SAL_FALSE;
    CHAR *pStr = NULL;
    size_t strLen = 0;
    const CHAR *pInputKey = "value";
    lxb_dom_attr_t *pDomAttr = NULL;
    lxb_dom_element_t *pElemObj = NULL;
    LXB_HTML_HANDLE *pHtmlHandle = (LXB_HTML_HANDLE *)pHandle;

    if (NULL == pHtmlHandle || NULL == pTableId)
    {
        PRT_INFO("the pHtmlHandle(%p) OR pTableId(%p) is NULL\n", pHtmlHandle, pTableId);
        return SAL_FAIL;
    }
    if (NULL == pInputName || NULL == pInputValue)
    {
        PRT_INFO("the pInputName(%p) OR pInputValue(%p) is NULL\n", pInputName, pInputValue);
        return SAL_FAIL;
    }

    dspttk_mutex_lock(&pHtmlHandle->mutexDom, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    lxb_dom_collection_clean(pHtmlHandle->pDomColl);

    /* collect <table> by id */
    sRet = lxb_dom_elements_by_attr(pHtmlHandle->pBodyElem, pHtmlHandle->pDomColl, (const lxb_char_t *)"id", strlen("id"), 
                                    (const lxb_char_t *)pTableId, strlen(pTableId), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, pTableId: %s\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pTableId);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* get <table> element */
    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, 0);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, length: %zu\n", lxb_dom_collection_length(pHtmlHandle->pDomColl));
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check class contain 'table' */
    bExist = lxb_dom_elements_by_attr_contain(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"class", strlen("class"),
                                              (const lxb_char_t *)"table", sizeof("table"), SAL_TRUE);
    if (bExist != LXB_STATUS_OK)
    {
        PRT_INFO("lxb_dom_elements_by_attr_contain 'table' failed, AttrKey: %s\n", "class");
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* collect <input> by name */
    u32CollOffset = lxb_dom_collection_length(pHtmlHandle->pDomColl);
    sRet = lxb_dom_elements_by_attr(pElemObj, pHtmlHandle->pDomColl, (const lxb_char_t *)"name", strlen("name"), 
                                    (const lxb_char_t *)pInputName, strlen(pInputName), SAL_TRUE);
    if (sRet != LXB_STATUS_OK || lxb_dom_collection_length(pHtmlHandle->pDomColl) != 1 + u32CollOffset)
    {
        PRT_INFO("lxb_dom_elements_by_attr failed, errno: %d, length: %zu, InputName: %s, CollOffset: %u\n", 
                 sRet, lxb_dom_collection_length(pHtmlHandle->pDomColl), pInputName, u32CollOffset);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* get <input> element */
    pElemObj = lxb_dom_collection_element(pHtmlHandle->pDomColl, u32CollOffset);
    if (NULL == pElemObj)
    {
        PRT_INFO("lxb_dom_collection_element failed, idx: %u\n", u32CollOffset);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Check tag is <input> */
    pStr = (CHAR *)lxb_dom_element_qualified_name(pElemObj, &strLen);
    if (0 != strncmp(pStr, "input", strLen))
    {
        PRT_INFO("the element tag is '%s', should be 'input'\n", pStr);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /* Modify <input> 'value', check exist first */
    bExist = dspttk_dom_element_has_attribute(pElemObj, (CHAR *)pInputKey);
    if (bExist) // 存在则直接修改
    {
        pDomAttr = lxb_dom_element_attr_by_name(pElemObj, (const lxb_char_t *)pInputKey, strlen(pInputKey));
        if (NULL == pDomAttr)
        {
            PRT_INFO("lxb_dom_element_attr_by_name failed, InputKey: %s\n", pInputKey);
            sRet = SAL_FAIL;
            goto EXIT;
        }
        sRet = lxb_dom_attr_set_value(pDomAttr, (const lxb_char_t *)pInputValue, strlen(pInputValue));
        if (sRet != LXB_STATUS_OK)
        {
            PRT_INFO("lxb_dom_attr_set_value failed, InputKey: %s, InputValue: %s\n", pInputKey, pInputValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }
    else // 不存在则创建
    {
        /* Append new attrtitube */
        pDomAttr = lxb_dom_element_set_attribute(pElemObj, (const lxb_char_t *)pInputKey, strlen(pInputKey), 
                                                 (const lxb_char_t *)pInputValue, strlen(pInputValue));
        if (pDomAttr == NULL)
        {
            PRT_INFO("lxb_dom_element_set_attribute failed, InputKey: %s, AttrValue: %s\n", pInputKey, pInputValue);
            sRet = SAL_FAIL;
            goto EXIT;
        }
    }

EXIT:
    dspttk_mutex_unlock(&pHtmlHandle->mutexDom, __FUNCTION__, __LINE__);

    return sRet;
}

