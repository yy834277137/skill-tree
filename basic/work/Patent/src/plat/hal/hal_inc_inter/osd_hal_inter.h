/**
 * @file   osd_hal_inter.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  hal层OSD处理接口封装
 * @author wangweiqin
 * @date   2017年09月08日 Create
 * @note
 * @note \n History
   1.Date        : 2017年09月08日 Create
     Author      : wangweiqin
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _OSD_HAL_INTER_H_
#define _OSD_HAL_INTER_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef struct tagOsdPlatOpsInfo
{
    UINT32 (*GetMemSize)(void);
    INT32 (*Create)(UINT32, UINT32, void **, unsigned char *);
    INT32 (*Destroy)(UINT32, void *);
    INT32 (*Process)(UINT32 chan, void *pHandle, PUINT8 pCharImgBuf, UINT64 u64PhyAddr,
                      UINT32 dstW, UINT32 dstH, UINT32 dstX, UINT32 dstY, UINT32 color, UINT32 bTranslucent);
} OSD_PLAT_OPS_S;

/**
 * @function   osd_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] OSD_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_register(OSD_PLAT_OPS_S *pstOsdPlatOps);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_OSD_HAL_INTER_H_*/


