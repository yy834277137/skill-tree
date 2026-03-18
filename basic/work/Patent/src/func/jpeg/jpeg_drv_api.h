/**
 * @file   jpeg_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  jpeg 模块 drv 层外部接口
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

#ifndef _JPEG_DRV_API_H_
#define _JPEG_DRV_API_H_

#include <sal.h>
#include <platform_hal.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************
                            结构定义
*****************************************************************************/
/* 编码抓图模式 */
typedef enum JpegCtrlModeE
{
    JPEG_ONCE = 0,            /* 单张抓拍，一次命令抓一张 */
    JPEG_MORE,                /* 连续抓拍, 按时间抓多张   */
    JPEG_BUTT
} JPEG_CTRL_MODE_E;


/* 编码抓图控制属性 */
typedef struct tagJpegCtrlPrmSt
{
    JPEG_CTRL_MODE_E eJpegMode;      /* 抓图模式 */
    UINT32 uiFinCnt;         /* 当前已抓完的张数 */
    UINT32 uiCaptCnt;        /* 抓拍张数，在连续抓拍模式下有效 */
    UINT32 uiCaptSec;        /* 在 uiCaptSec 秒内均匀抓拍 uiCaptCnt 张数 */
    UINT32 uiCaptInterval;   /* 抓图间隔 */
} JPEG_CTRL_PRM_S;


/* JPEG 编码设置属性 */
typedef struct tagJpegCommonEncPrmSt
{
    UINT32 quality;
    UINT32 OutWidth;
    UINT32 OutHeight;

    UINT32 outSize;
    INT8 *pOutJpeg;
    SYSTEM_FRAME_INFO stSysFrame;  /* 平台相关的输入图片信息指针 */
} JPEG_COMMON_ENC_PRM_S;


/*****************************************************************************
                            函数声明
*****************************************************************************/

/**
 * @function   jpeg_drv_createChn
 * @brief   创建一个抓图通道
 * @param[in]  SAL_VideoFrameParam *pstCreatePrm 抓图参数
 * @param[out] void **ppJpegHandle 抓图通道句柄指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_createChn(void **ppJpegHandle, SAL_VideoFrameParam *pstCreatePrm);

/**
 * @function   jpeg_drv_deleteChn
 * @brief   销毁抓图通道
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_deleteChn(void *pChnHandle);

/**
 * @function   jpeg_drv_enc
 * @brief   编码一张图片，调用前需要先使用Jpeg_drvCreate创建一个handle
            输出的jpeg内存由使用者分配
            非平台相关输入NV21 的YUV图片地址和宽高
            和平台相关则直接在输入参数内填入相关信息，默认平台相关的输入图像参数是合法的
            如果接口频繁调用，且编码参数会经常变化，推荐创建编码通道时直接创建最大分辨率
            每次编码不要重置编码器
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[in]  UINT32 isReset 是否需要重置编码器
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_enc(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm, UINT32 isReset);

/**
 * @function   jpeg_drv_cropEnc
 * @brief   编码一张图片，支持抠图
 * @param[in]  void *pChnHandle 抓图句柄
 * @param[in]  JPEG_COMMON_ENC_PRM_S *pstInPrm 抓图参数
 * @param[in]  CROP_S *pstCropInfo 抠图信息，可为NULL
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_cropEnc(void *pChnHandle, JPEG_COMMON_ENC_PRM_S *pstInPrm, CROP_S *pstCropInfo);

/**
 * @function   jpeg_drv_init
 * @brief    drv层编码器初始化
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 jpeg_drv_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _JPEG_DRV_H_ */


