/*******************************************************************************
* face_tsk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : zongkai5 <zongkai5@hikvision.com>
* Version: V2.0.0	2021年10月30日 Create
*
* Description :
* Modification:
*******************************************************************************/

/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
#include "face_tsk.h"
#include "face_drv.h"

/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构区									   */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */


/**
 * @function    CmdProc_faceCmdProc
 * @brief         人脸模块对外交互命令接口
 * @param[in]  cmd - 命令字
 * @param[in]  chan - 通道号
 * @param[in]  prm - 命令参数
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 CmdProc_faceCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    int iRet = SAL_FAIL;

    /* 0830 add: 提供人脸debug版本用于应用集成，当前人脸功能尚未支持，待后续功能调通后放开 */
    /* return SAL_SOK; */

    switch (cmd)
    {
        case HOST_CMD_FACE_INIT:
        {
            iRet = Face_DrvModuleInit(chan, prm);
            break;
        }
        case HOST_CMD_FACE_DEINIT:
        {
            iRet = Face_DrvModuleDeInit(chan);
            break;
        }
        case HOST_CMD_FACE_LOGIN:
        {
            iRet = Face_DrvUserLogin(chan, prm);
            break;
        }
        case HOST_CMD_FACE_REGISTER:
        {
            iRet = Face_DrvUserRegister(chan, prm);
            break;
        }
        case HOST_CMD_FACE_UPDATE_COMPARE_LIB:
        {
            iRet = Face_DrvUpdateCompLib(chan, prm);
            break;
        }
        case HOST_CMD_FACE_UPDATE_DATABASE:
        {
            iRet = Face_DrvUpdateDataBase(chan, prm);
            break;
        }
        case HOST_CMD_FACE_SET_CONFIG:
        {
            iRet = Face_DrvSetConfig(chan, prm);
            break;
        }
        case HOST_CMD_FACE_GET_VERSION:
        {
            iRet = Face_DrvGetVersion(prm);
            break;
        }
        case HOST_CMD_FACE_CMP_FACE:
        {
            iRet = Face_DrvCompareFeatureData(chan, prm);
            break;
        }
        case HOST_CMD_FACE_CAP_STATR:
        {
            iRet = Face_DrvStartCap(chan);
            break;
        }
        case HOST_CMD_FACE_CAP_STOP:
        {
            iRet = Face_DrvStopCap(chan);
            break;
        }
        case HOST_CMD_FACE_FEATURE_CMP:
        {
            iRet = Face_DrvGetFeatureVersionRst(prm);
            break;
        }
        default:
        {
            FACE_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            iRet = SAL_FAIL;
            break;
        }
    }

    return iRet;
}

/**
 * @function   Face_tskShowCompileVersion
 * @brief      打印智能编译时间
 * @param[in]  VOID  
 * @param[out] None
 * @return     static VOID
 */
static VOID Face_tskShowCompileVersion(VOID)
{
    FACE_LOGW("Module: face, Build Time: %s %s\n", __DATE__, __TIME__);
	return;
}

/**
 * @function    Face_tskInit
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Face_tskInit(void)
{
    INT32 s32Ret = SAL_FAIL;

    /* 通道相关资源初始化 */
    s32Ret = Face_DrvInit();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("error\n");
        return SAL_FAIL;
    }

	/* 打印人脸智能模块编译时间 */
	Face_tskShowCompileVersion();

    FACE_LOGI("Face Drv Init OK!\n");
    return SAL_SOK;
}

