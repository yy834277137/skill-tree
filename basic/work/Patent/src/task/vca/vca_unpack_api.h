/**
 * @file   vca_unpack_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  vca解封装函数接口
 * @author huangshuxin
 * @date   2019年5月24日 Create
 *
 * @note \n History
   1.Date        : 2019年5月24日 Create
     Author      : huangshuxin
     Modification: 新建文件
   2.Date        : 2021/08/19
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _VCA_UNPACK_API_H_
#define _VCA_UNPACK_API_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/**
 * @function   vca_unpack_init
 * @brief      vca 解析初始化
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_init(UINT32 u32Chan);

/**
 * @function   vca_unpack_process
 * @brief      vca信息解析处理
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] void *pstBuff 目标数据buf
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_process(UINT32 u32Chan, void *pstBuff);

/**
 * @function   vca_unpack_clearResult
 * @brief      清除vca解析结果
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] NONE
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_clearResult(UINT32 u32Chan);

/**
 * @function   vca_unpack_getResult
 * @brief      获取vca解析结果
 * @param[in]  UINT32 u32Chan 通道号 
 * @param[out] SVA_PROCESS_OUT *pstGetOut 输出结果buf
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 vca_unpack_getResult(UINT32 u32Chan, void *pstGetOut);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _VCA_UNPACK_H_ */




