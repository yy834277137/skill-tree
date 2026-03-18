/**
 * @file:   sal_secure.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  libc函数进行封装
 * @author: sunzelin
 * @date    2021/7/19
 * @note:
 * @note \n History:
   1.日    期: 2021/7/19
     作    者: sunzelin
     修改历史: 创建文件
 */
	 
#ifndef _SAL_SECURE_H_
#define _SAL_SECURE_H_

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

/**
 * @function:   sal_memcpy_s
 * @brief:      拷贝
 * @param[in]:  VOID *pDst            
 * @param[in]:  UINT32 u32DstMaxSize  
 * @param[in]:  const VOID *pSrc      
 * @param[in]:  UINT32 u32SrcMaxSize  
 * @param[out]: None
 * @return:     VOID *
 */
VOID *sal_memcpy_s(VOID *pDst, UINT32 u32DstMaxSize, const VOID *pSrc, UINT32 u32SrcMaxSize);

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
VOID *sal_memset_s(VOID *pDst, UINT32 u32DstMaxSize, UINT8 u8Val, UINT32 u32SetMaxSize);

#endif

