/**
 * @file   jpeg_tsk_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  jpeg 模块 tsk 层
 * @author liuyun10
 * @date   2018年12月16日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月16日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _JPEG_TSK_API_H_
#define _JPEG_TSK_API_H_

#include <sal.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/*****************************************************************************
                            函数声明
*****************************************************************************/

/**
 * @function   jpeg_tsk_init
 * @brief  Jpeg tsk层初始化资源
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_init(void);

/**
 * @function   jpeg_tsk_stop
 * @brief  销毁编码抓图命令
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_stop(UINT32 u32Dev, void *pInParam);

/**
 * @function   jpeg_tsk_start
 * @brief  开启编码抓图命令
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_start(UINT32 u32Dev, void *pInParam);
/**
 * @function   jpeg_tsk_setPrm
 * @brief   设置编码抓图命令
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_setPrm(UINT32 u32Dev, void *pInParam);

/**
 * @function   jpeg_tsk_setAiPrm
 * @brief   设置智能参数
 * @param[in]  UINT32 u32Dev 设备号
 * @param[in]  void *pInParam 输入参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_tsk_setAiPrm(UINT32 u32Dev, void *pInParam);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _JPEG_TSK_H_ */




