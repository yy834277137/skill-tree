
#ifndef _DSPTTK_HTML_H_
#define _DSPTTK_HTML_H_

#include "lexbor/html/html.h"

#ifdef __cplusplus
extern "C"
{
#endif



/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define DSPTTK_INDEX_HTML_FILE      "index.html" // 网页总入口文件

#if 0
typedef enum
{
    DSPTTK_HTML_COLIDX_INIT = 0,
    DSPTTK_HTML_COLIDX_SETTING = 1,
    DSPTTK_HTML_COLIDX_CONTROL = 2,
}DSPTTK_HTML_COLIDX;
#endif



/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
VOID *dspttk_html_create_handle(CHAR *pHtmlPath);
void dspttk_html_destroy_handle(VOID *pHandle);

/**
 * @fn      dspttk_html_save_as
 * @brief   将html句柄保存为文本文件
 * 
 * @param   [IN] pHandle html句柄
 * @param   [IN] pHtmlPath html文件的保存路径
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS dspttk_html_save_as(VOID *pHandle, CHAR *pHtmlPath);

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
SAL_STATUS dspttk_html_set_attr_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pAttrKey, CHAR *pAttrValue);

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
SAL_STATUS dspttk_html_del_attr_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pAttrKey);

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
SAL_STATUS dspttk_html_set_text_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pTextValue);

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
SAL_STATUS dspttk_html_set_style_by_id(VOID *pHandle, CHAR *pIdValue, CHAR *pAttrKey, CHAR *pAttrValue);

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
SAL_STATUS dspttk_html_set_attr_by_name(VOID *pHandle, CHAR *pNameValue, CHAR *pAttrKey, CHAR *pAttrValue);

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
SAL_STATUS dspttk_html_set_select_range(VOID *pHandle, CHAR *pSelectName, CHAR pOptionValue[][16], UINT32 u32Lens);

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
SAL_STATUS dspttk_html_set_select_by_name(VOID *pHandle, CHAR *pSelectName, CHAR *pOptionValue);

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
SAL_STATUS dspttk_html_set_radio_by_name(VOID *pHandle, CHAR *pRadioName, CHAR *pCheckedValue);

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
SAL_STATUS dspttk_html_set_checkbox_by_name(VOID *pHandle, CHAR *pCheckboxName, BOOL bChecked);

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
SAL_STATUS dspttk_html_set_table_cell_inputs(VOID *pHandle, CHAR *pTableId, UINT32 u32RowIdx, UINT32 u32CellIdx, CHAR *pInputValue[], UINT32 u32InputCnt);

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
SAL_STATUS dspttk_html_set_table_input_by_name(VOID *pHandle, CHAR *pTableId, CHAR *pInputName, CHAR *pInputValue);

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
extern VOID *gpHtmlHandle; // Html文件句柄，由dspttk_html_create_handle()返回

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_HTML_H_ */

