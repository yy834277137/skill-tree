/**
 * @file   venc_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  venc 模块 fun 层外部接口
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

#ifndef _VENC_DRV_API_H_
#define _VENC_DRV_API_H_

#include <sal.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                         结构定义
*****************************************************************************/
typedef struct tagVencDrvInitPrmst
{
    UINT32 u32Dev;
    UINT32 u32StreamId;                /* 码流id, 0:main,1:sub,2:third */

    SAL_VideoFrameParam stVencPrm;
} VENC_DRV_INIT_PRM_S;

/*****************************************************************************
                            函数
*****************************************************************************/

/**
 * @function   venc_drv_init
 * @brief  drv层编码初始化
 * @param[in] None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_init(void);

/**
 * @function   venc_drv_getHalChan
 * @brief   获取hal通道ID
 * @param[in]  void *pChanHandle通道句柄
 * @param[out] UINT32 *pChn 通道指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_getHalChan(void *pChanHandle, UINT32 *pChn);

/**
 * @function   venc_drv_addChan
 * @brief   新增一个编码通道
 * @param[in]  VENC_DRV_INIT_PRM_S *pstInPrm 输入参数
 * @param[out]  void **pphandle 通道句柄指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_addChan(void **pphandle, VENC_DRV_INIT_PRM_S *pstInPrm);

/**
 * @function   venc_drv_setPrm
 * @brief   配置编码参数: 切换分辨率、码率、I帧间隔、编码质量、帧率等
            编码器如果创建好立即生效，没有创建则下次开启编码器时生效
 * @param[in]  void *pChanHandle 通道句柄
 * @param[in]  SAL_VideoFrameParam *pstVencChnAttr 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_setPrm(void *pChanHandle, SAL_VideoFrameParam *pstVencChnAttr);

/**
 * @function   venc_drv_requestIDR
 * @brief   编码模块强制I帧
 * @param[in]  void *pChanHandle 通道句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_requestIDR(void *pChanHandle);

/**
 * @function   venc_drv_stop
 * @brief  停止编码器
 * @param[in] void *pChanHandle 通道句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_stop(void *pChanHandle);

/**
 * @function   venc_drv_start
 * @brief  开启编码器
 * @param[in] void *pChanHandle 通道句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_start(void *pChanHandle);

/**
 * @function   venc_drv_create
 * @brief  创建编码通道，包括hal层的创建和线程的发布
 * @param[in] void *pChanHandle 通道句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_create(void *pChanHandle);

/**
 * @function   venc_drv_delete
 * @brief  编码通道的销毁，停止线程，销毁hal层编码器
 * @param[in] void *pChanHandle 通道句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_delete(void *pChanHandle);

/**
 * @function   venc_drv_staticParmCheck
 * @brief   编码器静态参数设置检查
 * @param[in]  void *pChanHandle 通道句柄
 * @param[in]  SAL_VideoFrameParam *pstVencChnParm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
UINT32 venc_drv_staticParmCheck(void *pChanHandle, SAL_VideoFrameParam *pstVencChnParm);

/**
 * @function   venc_drv_getBuff
 * @brief  获取一帧码流
 * @param[in]  void *pChanHandle 通道句柄
 * @param[out] void *pBuffer 含有码流数据地址的结构体指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_getBuff(void *pChanHandle, void *pBuffer);

/**
 * @function   venc_drv_putBuff
 * @brief  释放一帧数据
 * @param[in]  void *pChanHandle 通道句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_putBuff(void *pChanHandle);
/**
 * @function   venc_drv_sendVideoFrm
 * @brief      向编码器发送一帧视频数据
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_drv_sendVideoFrm(void *pChnHandle, void *pstInFrm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _VENC_DRV_H_ */



