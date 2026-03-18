/**
 * @file   mstar_mcu.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @author liuxianying
 * @date   2021/9/6
 * @note
 * @note \n History
   1.日    期: 2021/9/6
     作    者: liuxianying
     修改历史: 创建文件
 */

#include "sal.h"
#include "platform_hal.h"
#include "mcu_drv.h"
#include "../inc/mstar_drv.h"

static CAPT_CHIP_FUNC_S g_MstarMcuFunc = {0};


/**
 * @function   mcu_func_mstarRegister
 * @brief      mcu控制mstar芯片注册函数
 * @param[in]  void  
 * @param[out] None
 * @return     CAPT_CHIP_FUNC_S *
 */
CAPT_CHIP_FUNC_S *mcu_func_mstarRegister(void)
{

    memset(&g_MstarMcuFunc, 0, sizeof(CAPT_CHIP_FUNC_S));
    g_MstarMcuFunc.pfuncInit       = mcu_func_mstarInit;
    g_MstarMcuFunc.pfuncHotPlug    = mcu_func_mstarHotplug;
    g_MstarMcuFunc.pfuncReset      = mcu_func_mstarReset;
    g_MstarMcuFunc.pfuncDetect     = mcu_func_mstarGetRes;
    g_MstarMcuFunc.pfuncSetRes     = NULL;
    g_MstarMcuFunc.pfuncSetCsc     = mcu_func_mstarSetCsc;
    g_MstarMcuFunc.pfuncAutoAdjust = mcu_func_mstarAutoAdjust;
    g_MstarMcuFunc.pfuncGetEdid    = mcu_func_mucGetEdid;
    g_MstarMcuFunc.pfuncSetEdid    = mcu_func_mcuSetEdid;
    g_MstarMcuFunc.pfuncScale      = mcu_func_mstarScale;
    g_MstarMcuFunc.pfuncUpdate     = NULL;

    return &g_MstarMcuFunc;
}



