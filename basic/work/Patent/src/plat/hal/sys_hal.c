/**
 * @file   sys_hal.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  平台封装层，系统相关模块
 * @author yeyanzhong
 * @date   2021.6.22
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sys_hal_inter.h"
#include "hal_inc_exter/sys_hal_api.h"


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
/* 修改func宏，去掉路径*/
#line __LINE__ "sys_hal.c"


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
/* 保存平台操作符*/
static const SYS_PLAT_OPS *g_pstSysPlatOps = NULL;


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   sys_hal_allocVideoFrameInfoSt
 * @brief      分配存放帧信息的内存
 * @param[out] SYSTEM_FRAME_INFO *pstSystemFrameInfo  帧信息指针
 * @param[in]  None
 * @return     INT32
 */
INT32 sys_hal_allocVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    if (!pstSystemFrameInfo)
    {
        SYS_LOGE("input param is null \n");
        return SAL_FAIL;
    }

    return g_pstSysPlatOps->allocVideoFrameInfoSt(pstSystemFrameInfo);
}

/**
 * @function    sys_hal_rleaseVideoFrameInfoSt
 * @brief       释放存放帧信息的内存
 * @param[in]   pstSystemFrameInfo 帧信息指针
 * @param[out]
 * @return
 */
INT32 sys_hal_rleaseVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    if (!pstSystemFrameInfo)
    {
        SYS_LOGE("input param is null \n");
        return SAL_FAIL;
    }

    return g_pstSysPlatOps->releaseVideoFrameInfoSt(pstSystemFrameInfo);
}

/**
 * @function   sys_hal_buildVideoFrame
 * @brief      生成自定义帧信息
 * @param[in]  SAL_VideoFrameBuf *pVideoFrame         输入平台帧信息指针
 * @param[out]  SYSTEM_FRAME_INFO *pstSystemFrameInfo  输出自定义帧信息指针
 * @return     INT32
 */
INT32 sys_hal_buildVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    if (!pVideoFrame || !pstSystemFrameInfo)
    {
        SYS_LOGE("input param is null \n");
        return SAL_FAIL;
    }

    return g_pstSysPlatOps->buildVideoFrame(pVideoFrame, pstSystemFrameInfo);
}


/**
 * @function   sys_hal_getVideoFrameInfo
 * @brief      获取平台帧信息
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo  自定义帧信息指针
 * @param[out] SAL_VideoFrameBuf *pVideoFrame         平台帧信息指针
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_getVideoFrameInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame)
{
    if (!pVideoFrame || !pstSystemFrameInfo)
    {
        SYS_LOGE("input param is null \n");
        return SAL_FAIL;
    }

    return g_pstSysPlatOps->getVideoFrameInfo(pstSystemFrameInfo, pVideoFrame);
}

/**
 * @function   sys_hal_getMBbyVirAddr
 * @brief      获取虚拟地址的MB
 * @param[in]  void *viraddr  虚拟地址
 * @param[out] NANE
 * @param[out] None
 * @return     UINT64
 */
UINT64 sys_hal_getMBbyVirAddr(void *viraddr)
{

    return (UINT64)g_pstSysPlatOps->getMBbyVirAddr(viraddr);
}

/**
 * @function   sys_hal_SDKbind
 * @brief      SDK数据流绑定
 * @param[in]  SYSTEM_MOD_ID_E uiSrcModId  源模块ID
 * @param[in]  UINT32 s32SrcDevId          源设备ID
 * @param[in]  UINT32 s32SrcChn            源通道
 * @param[in]  SYSTEM_MOD_ID_E uiDstModId  目的模块ID
 * @param[in]  UINT32 s32DstDevId          目的设备ID
 * @param[in]  UINT32 s32DstChn            目的通道
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_SDKbind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 s32SrcDevId, UINT32 s32SrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 s32DstDevId, UINT32 s32DstChn)
{
    return g_pstSysPlatOps->bind(uiSrcModId, s32SrcDevId, s32SrcChn, uiDstModId, s32DstDevId, s32DstChn, 1);
}

/**
 * @function   sys_hal_SDKunbind
 * @brief      数据流SDK解绑
 * @param[in]  SYSTEM_MOD_ID_E uiSrcModId  源模块ID
 * @param[in]  UINT32 s32SrcDevId          源设备ID
 * @param[in]  UINT32 s32SrcChn            源通道
 * @param[in]  SYSTEM_MOD_ID_E uiDstModId  目的模块ID
 * @param[in]  UINT32 s32DstDevId          目的设备ID
 * @param[in]  UINT32 s32DstChn            目的通道
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_SDKunbind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 s32SrcDevId, UINT32 s32SrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 s32DstDevId, UINT32 s32DstChn)
{
    return g_pstSysPlatOps->bind(uiSrcModId, s32SrcDevId, s32SrcChn, uiDstModId, s32DstDevId, s32DstChn, 0);
}

/**
 * @function   sys_hal_GetSysCurPts
 * @brief      获取PTS
 * @param[out] UINT64 *pCurPts  获取的PTS
 * @param[in]  None
 * @return     INT32
 */
INT32 sys_hal_GetSysCurPts(UINT64 *pCurPts)
{
    return g_pstSysPlatOps->getPts(pCurPts);
}

/**
 * @function   sys_hal_setVideoTimeInfo
 * @brief      设置帧号和时间信息
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo  帧指针
 * @param[in]  UINT32 u32TimeRef                      帧号
 * @param[in]  UINT64 u64Pts                          帧PTS时间
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_setVideoTimeInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32TimeRef, UINT64 u64Pts)
{
    if (!pstSystemFrameInfo)
    {
        SYS_LOGE("input param is null \n");
        return SAL_FAIL;
    }

    return g_pstSysPlatOps->setVideoTimeInfo(pstSystemFrameInfo, u32TimeRef, u64Pts);
}

/**
 * @function   sys_hal_initMpp
 * @brief      初始化mpp
 * @param[in]  UINT32 u32ViChnNum  vi通道数
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_initMpp(UINT32 u32ViChnNum)
{
    if (NULL == g_pstSysPlatOps->init)
    {
        SYS_LOGW("g_pstSysPlatOps->init is null \n");
        return SAL_SOK;
    }

    return g_pstSysPlatOps->init(u32ViChnNum);
}

/**
 * @function   sys_hal_deInit
 * @brief      去初始化平台层
 * @param[in]  void
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_deInit(void)
{
    return g_pstSysPlatOps->deInit();
}

/**
 * @function   sys_hal_init
 * @brief      初始化平台层
 * @param[in]  void
 * @param[out] None
 * @return     INT32
 */
INT32 sys_hal_init(void)
{

    if (NULL == g_pstSysPlatOps)
    {
        g_pstSysPlatOps = sys_plat_register();
    }

    return SAL_SOK;
}

