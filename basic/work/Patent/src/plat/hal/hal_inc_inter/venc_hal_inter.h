/**
 * @file   vecn_hal_inter.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---hal层接口封装
 * @author 
 * @date   
 * @note
 * @note \n History
   1.Date        : xx年xx月xx日 Create
     Author      : unknow
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

 #ifndef _VENC_HAL_INTER_H_
#define _VENC_HAL_INTER_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "sal.h"
//#include <sal_video.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef struct tagVencPlatOpsInfo
{
    INT32 (*Init)(VOID);
    INT32 (*Create)(VOID **, SAL_VideoFrameParam *);
    INT32 (*Delete)(VOID *);
    INT32 (*Start)(VOID *);
    INT32 (*Stop)(VOID *);
    INT32 (*SetParam)(VOID *, SAL_VideoFrameParam *);
    INT32 (*ForceIFrame)(VOID *);
    INT32 (*GetFrame)(VOID *, void *);
    INT32 (*PutFrame)(VOID *);
    INT32 (*GetHalChn)(VOID *, UINT32 *);
    INT32 (*SetVencStatues)(VOID *, VOID *);
    INT32 (*EncJpegProcess)(void *handle, SYSTEM_FRAME_INFO *pFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm);
    INT32 (*EncSendFrm)(void *pHandle, void *pStr);
} VENC_PLAT_OPS_S;

/**
 * @function   venc_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] VENC_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_register(VENC_PLAT_OPS_S *pstVencPlatOps);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_VENC_HAL_INTER_H_*/


