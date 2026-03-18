/**
 * @file:   sal_secure.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  libc进行封装的安全函数
 * @author: sunzelin
 * @date    2021/7/19
 * @note:
 * @note \n History:
   1.日    期: 2021/7/19
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/

#include "sal.h"
#include "sal_secure.h"

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
 * @function:   sal_memcpy_s
 * @brief:      拷贝
 * @param[in]:  VOID *pDst            
 * @param[in]:  UINT32 u32DstMaxSize  
 * @param[in]:  VOID *pSrc      
 * @param[in]:  UINT32 u32SrcMaxSize  
 * @param[out]: None
 * @return:     VOID *
 */
VOID *sal_memcpy_s(VOID *pDst, UINT32 u32DstMaxSize, const VOID *pSrc, UINT32 u32SrcMaxSize)
{
	UINT32 u32CpySize = 0;

	u32CpySize = u32DstMaxSize < u32SrcMaxSize ? u32DstMaxSize : u32SrcMaxSize;
	memcpy(pDst, pSrc, u32CpySize);
	
	return pDst;
}

/**
 * @function:   sal_memset_s
 * @brief:      赋值
 * @param[in]:  VOID *pDst            
 * @param[in]:  UINT32 u32DstMaxSize  
 * @param[in]:  UINT8 u8Val           
 * @param[in]:  UINT32 u32SetMaxSize  
 * @param[out]: None
 * @return:     VOID *
 */
VOID *sal_memset_s(VOID *pDst, UINT32 u32DstMaxSize, UINT8 u8Val, UINT32 u32SetMaxSize)
{
	UINT32 u32SetSize = 0;

	u32SetSize = u32DstMaxSize < u32SetMaxSize ? u32DstMaxSize : u32SetMaxSize;
	memset(pDst, u8Val, u32SetSize);

	return pDst;
}

