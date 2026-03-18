/*******************************************************************************
 * dfr_tsk.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangweiqin <wangweiqin@hikvision.com.cn>
 * Version: V1.0.0  2017年10月17日 Create
 *
 * Description : 智能模块硬件层
 * Modification: 
 *******************************************************************************/

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "dfr_tsk.h"
#include "sal.h"
#include "dspcommon.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 临时代码 */
/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

INT32 Dfr_tskCmdProc(UINT32 uiChn, UINT32 cmd, void *prm)
{
    INT32 status = SAL_SOK;
    
    switch(cmd)
    {
        case HOST_CMD_INIT_FR:  /* 初始化智能模块 */
        {
            break;
        }
        case HOST_CMD_FFD_CTRL: /* 人脸检测功能控制属性 */
        {
//            status = Dfr_drvFaceDetect(prm);
//            if (SAL_isFail(status))
//            {
//                SAL_ERROR(" Face Detected Failed !!!\n");
//            }
            break;
        }
        case HOST_CMD_FR_BM:    /* 人脸建模功能 */
        {
//            status = Dfr_drvBuildModel(prm);
//            if (SAL_isFail(status))
//            {
//                SAL_ERROR("Face Build Model Failed !!!\n");
//            }
            break;
        }
        case HOST_CMD_FR_CP:
        {
            break;
        }
        case HOST_CMD_FR_DET:  /* 活体检测  */
        {
//            status = Dfr_drvSetFaceLdPrm(prm);
//            if (SAL_isFail(status))
//            {
//                SAL_ERROR("Face Live Detected !!!\n");
//            }
            break;
        }
        case HOST_CMD_GET_FR_VER:
        {
//            status = Dfr_drvGetFrVersion(uiChn, prm);
//            if (SAL_isFail(status))
//            {
//                SAL_ERROR("Chn %x Host Cmd %x Failed !!!\n", uiChn, cmd);
//            }
            break;
        }
        case HOST_CMD_FR_PRESNAP:
        {
            break;
        }
        case HOST_CMD_FR_JPGSCALE:
        {
            break;
        }
        case HOST_CMD_FR_BMP2JPG:
        {
            break;
        }
        case HOST_CMD_FR_ROI:
        {
//            status = Dfr_drvSetVpssGrpCrop((FR_ROI_RECT *)prm);
//            if (SAL_isFail(status))
//            {
//                SAL_ERROR(" !!!\n");
//            }
            break;
        }
        default:
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            status = SAL_FAIL;
            break;
        }
    }

    return status;
}

void DBG_openSolidSaveLog(INT32 isOpen)
{
//    FR_SOLID_TRANS_PRM_ST solid;
//    solid.cmd   = 1;
//    solid.value = (!!isOpen);
//    Dfr_drvSolidSetPrm(0, solid.cmd, solid.value);
}

