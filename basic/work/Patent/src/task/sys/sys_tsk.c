#include "sys_tsk.h"
#include "platform_hal.h"

#ifdef DMA_TEST
#include "dma.h"
#endif

/* Tsk ID 幻数，暂无具体意义 */
#define SYS_OBJ_COMMON (0xAB)

/*******************************************************************************
* 函数名  : sys_task_makeTskID
* 描  述  : system 通用模块 创建 ID
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
UINT32 sys_task_makeTskID(SYSTEM_MOD_ID_E modID, UINT32 uiModChn)
{
    /* tskID 由一个幻数 + 模块标号 + 模块通道号 + tskObj标号组成 */
    UINT32 uiTskID = ((SYS_OBJ_COMMON << 24)
                      | (modID << 16)
                      | (uiModChn << 8));

    return uiTskID;
}

/*******************************************************************************
* 函数名  : sys_task_makeTskID
* 描  述  : system 通用模块 创建 ID
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
UINT32 sys_task_makeTskOutID(UINT32 uiTskID, UINT32 uiChn)
{
    UINT32 uiTskOutID = ((uiTskID)
                         | (uiChn));

    return uiTskOutID;
}

/*******************************************************************************
* 函数名  : System_commonGetModID
* 描  述  : system 通用模块 获取模块类型 ID
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
UINT32 sys_task_getModID(UINT32 uiTskID)
{
    UINT32 uiModID = ((uiTskID & 0xFFFFFF) >> 16);

    return uiModID;
}

/*******************************************************************************
* 函数名  : sys_task_getModChn
* 描  述  : system 通用模块 获取模块通道
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
UINT32 sys_task_getModChn(UINT32 uiTskID)
{
    UINT32 uiModChn = ((uiTskID & 0xFFFF) >> 8);

    return uiModChn;
}

/*******************************************************************************
* 函数名  : System_commonGetModSubChn
* 描  述  : system 通用模块 获取模块的子通道
* 输  入  :
*         :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
UINT32 sys_task_getModSubChn(UINT32 uiTskID)
{
    UINT32 uiModSubChn = (uiTskID & 0xFF);

    return uiModSubChn;
}


INT32 sys_task_deInit( )
{
    if (SAL_SOK != sys_hal_deInit())
    {
        SYS_LOGE("Platform System Init Failed !!!\n");
        return SAL_FAIL;
    }
        
#ifdef DMA_TEST
        (VOID)Dma_CloseDev();
#endif

    return SAL_SOK;
}

INT32 sys_task_init(UINT32 u32ViChnNum )
{
    if (SAL_SOK != sys_hal_initMpp(u32ViChnNum))
    {
        SYS_LOGE("Platform System Init Failed !!!\n");
        return SAL_FAIL;
    }

    /* 公共资源在 sys 中初始化 */
    if (SAL_SOK != tde_hal_Init( ))
    {
        SYS_LOGE("Platform System Init Failed !!!\n");
        return SAL_FAIL;
    }

#ifdef DMA_TEST
    /* 打开DMA设备用于后续拷贝使用 */
    if (SAL_SOK != Dma_OpenDev())
    {
        SAL_ERROR("open dev failed! \n");
        return SAL_FAIL;
    }
#endif



    return SAL_SOK;
}


