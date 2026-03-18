/**
 * @file   venc_drv.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  venc 模块 fun 层
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

#ifndef _VENC_DRV_H_
#define _VENC_DRV_H_

#include <sal.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************
                            宏定义
*****************************************************************************/

#define VENC_MAX_CHAN_NUM (6)


/*****************************************************************************
                         结构定义
*****************************************************************************/

/* Venc Chn 信息 */
typedef struct tagVencDrvChnInfoSt
{
    UINT32 u32Chn;               /* 当前通道号drv      */
    UINT32 u32Dev;               /* vi通道      */
    UINT32 u32StreamId;          /* 码流id, 0:main,1:sub,2:third */
    UINT32 isCreate;

    void *pVencHandle;           /* 对应hal层编码器handle */
    SAL_VideoFrameParam stVencChnPrm;             /* 当前通道的编码器属性 */
} VENC_DRV_CHN_INFO_S;

/* 采集模块状态结构体 */
typedef struct tagVencDrvMdStatusSt
{
    UINT32 u32VencNum;              /* 编码通道个数 */
    UINT32 u32VencChnUsedFlg;       /*通道占用标记*/
    void *pMutexHandle;
    VENC_DRV_CHN_INFO_S stVencChnInfo[VENC_MAX_CHAN_NUM];      /* 数组下标表示编码通道 */
} VENC_DRV_CTRL_S;

/*****************************************************************************
                            函数
*****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _VENC_DRV_H_ */



