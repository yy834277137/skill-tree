/**
 * @file:   sal_rtld.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  运行时动态库加载模块(头文件)
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

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

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
INT32 Sal_PutLibHandle(VOID *pHandle);

/**
 * @function:   Sal_GetLibSymbol
 * @brief:      获取动态库符号
 * @param[in]:  VOID *pHandle
 * @param[in]:  CHAR *pcSymName
 * @param[in]:  VOID **ppSym
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sal_GetLibSymbol(VOID *pHandle, CHAR *pcSymName, VOID **ppSym);

/**
 * @function:   Sal_GetLibHandle
 * @brief:      获取动态库加载句柄
 * @param[in]:  CHAR *pcPath
 * @param[in]:  VOID **ppHandle
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sal_GetLibHandle(CHAR *pcPath, VOID **ppHandle);


