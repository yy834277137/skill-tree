/*******************************************************************************
* disp_hal_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : liuxianying <liuxianying@hikvision.com>
* Version: V1.0.0  2021年07月22日 Create
*
* Description :
* Modification:
*******************************************************************************/



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "platform_hal.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#line __LINE__ "platform_hal.c"



/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


/*******************************************************************************
* 函数名  : plat_hal_init
* 描  述  : palt芯片平台层初始化
* 输  入  : - imgW              :
*         : - imgH              :
*         : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 plat_hal_init(void)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = sys_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("sys Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret =  mem_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("mem Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = disp_hal_Init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("disp Init Failed !!!\n");
        return SAL_FAIL;
    }

        /* fb初始化 */
    s32Ret = fb_hal_Init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("fb init failed\n");
    }
#ifdef DSP_ISA
    s32Ret = capt_hal_Init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("capt Init Failed !!!\n");
        return SAL_FAIL;
    }
#endif

    s32Ret = vgs_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("vgs Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("venc Init Failed !!!\n");
        return SAL_FAIL;
    }

#if 0
    s32Ret = svpdsp_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("svpdsp Init Failed !!!\n");
        return SAL_FAIL;
    }
#endif
    /* 公共资源在 sys 中初始化 */
    if (SAL_SOK != tde_hal_Init( ))
    {
        SYS_LOGE("tde Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = dup_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("dup Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = audio_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("audio Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = ive_hal_init( );
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("cipher Init Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = osd_hal_init();
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("osd Init Failed !!!\n");
        return SAL_FAIL;
    }

     SYS_LOGI("hal Init ok !!!\n");

    return SAL_SOK;
}

