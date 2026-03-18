/**
 * @file:   sal_rtld.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  运行时动态库加载模块(源文件)
 * @author: sunzelin
 * @date    2021/4/20
 * @note:
 * @note \n History:
   1.日    期: 2021/4/20
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define SAL_CHECK_RET(val, str, ret) \
    { \
        if (val) \
        { \
            SAL_ERROR("%s! \n", str); \
            return ret; \
        } \
    }

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Sal_PutLibHandle
 * @brief:      释放动态库加载句柄
 * @param[in]:  VOID *pHandle
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sal_PutLibHandle(VOID *pHandle)
{
    SAL_CHECK_RET(pHandle == NULL, "ptr null!", SAL_FAIL);

    (VOID)dlclose(pHandle);
    return SAL_SOK;
}

/**
 * @function:   Sal_GetLibSymbol
 * @brief:      获取动态库符号
 * @param[in]:  VOID *pHandle
 * @param[in]:  CHAR *pcSymName
 * @param[in]:  VOID **ppSym
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sal_GetLibSymbol(VOID *pHandle, CHAR *pcSymName, VOID **ppSym)
{
    VOID *p = NULL;

    SAL_CHECK_RET(pHandle == NULL, "ptr null!", SAL_FAIL);
    SAL_CHECK_RET(pcSymName == NULL, "ptr null!", SAL_FAIL);

    p = dlsym(pHandle, pcSymName);
    SAL_CHECK_RET(p == NULL, dlerror(), SAL_FAIL);

    *ppSym = p;
    return SAL_SOK;
}

/**
 * @function:   Sal_GetLibHandle
 * @brief:      获取动态库加载句柄
 * @param[in]:  CHAR *pcPath
 * @param[in]:  VOID **ppHandle
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sal_GetLibHandle(CHAR *pcPath, VOID **ppHandle)
{
    VOID *handle = NULL;

    SAL_CHECK_RET(pcPath == NULL, "ptr null!", SAL_FAIL);

    handle = dlopen(pcPath, RTLD_NOW);
    SAL_CHECK_RET(handle == NULL, dlerror(), SAL_FAIL);

    *ppHandle = handle;
    return SAL_SOK;
}

