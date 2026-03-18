/**
 * @file   osd_hal.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  hal层OSD处理接口封装
 * @author heshengyuan
 * @date   2014年12月31日 Create
 * @note
 * @note \n History
   1.Date        : 2014年12月31日 Create
     Author      : heshengyuan
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include "platform_hal.h"
#include "hal_inc_inter/osd_hal_inter.h"

/*****************************************************************************
                            全局结构体
*****************************************************************************/

OSD_PLAT_OPS_S g_stOsdHalOps = {0};

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   osd_hal_process
 * @brief      osd叠加处理
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]   void *pHandle hal层OSD句柄
 * @param[in]   PUINT8 pCharImgBuf 输入buf
 * @param[in]   UINT32 dstW, UINT32 dstH, UINT32 dstX, UINT32 dstY OSD区域信息
 * @param[in]   UINT32 color OSD颜色
 * @param[in]   UINT32 bTranslucent 是否半透明
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_process(UINT32 chan, void *pHandle, PUINT8 pCharImgBuf, UINT64 u64PhyAddr,
                      UINT32 dstW, UINT32 dstH, UINT32 dstX, UINT32 dstY, UINT32 color, UINT32 bTranslucent)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stOsdHalOps.Process)
    {
        s32Ret = g_stOsdHalOps.Process(chan, pHandle, pCharImgBuf, u64PhyAddr, dstW, dstH, dstX, dstY, color, bTranslucent);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        OSD_LOGE("fun not registed\n");
    }

    return SAL_SOK;
}

/**
 * @function   osd_hal_destroy
 * @brief      销毁OSD句柄
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]  void *pHandle OSD句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_destroy(UINT32 chan, void *pHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stOsdHalOps.Destroy)
    {
        s32Ret = g_stOsdHalOps.Destroy(chan, pHandle);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        OSD_LOGE("fun not registed\n");
    }

    return SAL_SOK;
}

/**
 * @function   osd_hal_getMemSize
 * @brief    获取osd hdl所需内存大小
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_getMemSize(void)
{
    if (NULL != g_stOsdHalOps.GetMemSize)
    {
        return g_stOsdHalOps.GetMemSize();
    }
    else
    {
        OSD_LOGE("fun not registed\n");
    }

    return 0;
}

/**
 * @function   osd_hal_create
 * @brief    创建OSD句柄
 * @param[in]  UINT32 u32Idx rgn id
 * @param[in]  unsigned char *pInBuf 输入buf，外部申请
 * @param[out] void **ppHandle hal层OSD句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_create(UINT32 u32Start, UINT32 u32Idx, void **pHandle, unsigned char *pInBuf)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL != g_stOsdHalOps.Create)
    {
        s32Ret = g_stOsdHalOps.Create(u32Start, u32Idx, pHandle, pInBuf);
        if (SAL_FAIL == s32Ret)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        OSD_LOGE("fun not registed\n");
    }

    return SAL_SOK;
}

#ifdef DSP_WEAK_FUNC
/**
 * @function   osd_hal_register
 * @brief      弱函数当有平台不支持osd时保证编译通过
 * @param[in]  OSD_PLAT_OPS_S *pstOsdHalOps  
 * @param[out] None
 * @return     __attribute__((weak) ) INT32
 */
__attribute__((weak) ) INT32 osd_hal_register(OSD_PLAT_OPS_S *pstOsdHalOps)
{
    return SAL_FAIL;
}
#endif

/**
 * @function   osd_hal_init
 * @brief    OSD模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_init(void)
{
    memset(&g_stOsdHalOps, 0x00, sizeof(OSD_PLAT_OPS_S));

    if (SAL_FAIL == osd_hal_register(&g_stOsdHalOps))
    {
        SAL_ERROR("plat reigister fail\n");
        return SAL_SOK;
    }

    return SAL_SOK;
}

