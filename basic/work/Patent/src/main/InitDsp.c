/**
 * @file   InitDsp.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  DSP提供给应用API
 * @author dsp
 * @date   2022/5/7
 * @note
 * @note \n History
   1.日    期: 2022/5/7
     作    者: dsp
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <sal.h>
#include "system_prm_api.h"

#include "dspcommon.h"
#include "disp_tsk_api.h"
#include "capt_tsk_api.h"
#include "sva_out.h"
#include "ba_out.h"
#include "ppm_out.h"
#include "face_out.h"
#include "sys_hostcmd_proc_api.h"
#include "osd_hostcmd_proc_api.h"
#include "venc_hostcmd_proc_api.h"
#include "vdec_hostcmd_proc.h"
#include "audio_hostcmd_proc_api.h"
#include "dfr_hostcmd_proc.h"
#include "fld_hostcmd_proc.h"
#include "recode_hostcmd_proc.h"
/* #include "vda_hostcmd_proc.h" */
#include "task_ism.h"
#include "sal_trace.h"

#ifdef DSP_ISM
#include "xpack_hostcmd_proc.h"
#include "xray_hostcmd_proc.h"
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define CMD_GET_TYPE(a) (a & 0xffff0000)   /* 获取命令字类型 */


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
extern INT32 CmdProc_osdCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf);


/**
 * @function   SendCmdToDsp
 * @brief      向DSP模块发送主机命令
 * @param[in]  HOST_CMD cmd          命令字
 * @param[in]  unsigned int chan     通道
 * @param[in]  unsigned int *pParam  命令参数
 * @param[in]  void *pBuf            命令buf
 * @param[out] None
 * @return     int
 */
int SendCmdToDsp(HOST_CMD cmd, unsigned int chan, unsigned int *pParam, void *pBuf)
{
    int iRet = SAL_FAIL;

    switch (CMD_GET_TYPE(cmd))
    {
        case HOST_CMD_MODULE_SYS:
        {
            iRet = CmdProc_sysCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_VI:
        case HOST_CMD_MODULE_ISP:
        {
            iRet = CmdProc_captCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_VENC:
        {
            iRet = CmdProc_encCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_DEC:
        {
            iRet = CmdProc_vdecCmdProc(cmd, chan, pBuf);
            break;
        }
        //自动调试进入到这里面
        case HOST_CMD_MODULE_DISP:
        {
            iRet = CmdProc_dispCmdProc(cmd, chan,pBuf);
            break;
        }
        case HOST_CMD_MODULE_AUDIO:
        {
            iRet = CmdProc_audioCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_OSD:
        {
            iRet = CmdProc_osdCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_FR:
        {
            iRet = CmdProc_dfrCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_FACE:
        {
            iRet = CmdProc_faceCmdProc(cmd, chan, pBuf);
            break;
        }
        case HOST_CMD_MODULE_SVA:
        {
            iRet = CmdProc_svaCmdProc(cmd, chan, pBuf);
            break;
        }
#if 1
        case HOST_CMD_MODULE_BA:
        {
            iRet = CmdProc_baCmdProc(cmd, chan, pBuf);
            break;
        }
#endif
        case HOST_CMD_MODULE_RECODE:
        {
            iRet = CmdProc_recodeCmdProc(cmd, chan, pBuf);
            break;
        }
#if 0   /* 移动侦测未使用，暂时注释保留 */
        case HOST_CMD_MODULE_VDA:
        {
            iRet = CmdProc_vdaCmdProc(cmd, chan, pBuf);
            break;
        }
#endif

#ifdef DSP_ISM
        case HOST_CMD_MODULE_XPACK:
        {
            iRet = CmdProc_XpackCmdProc(cmd, chan, pParam, pBuf);
            break;
        }
        case HOST_CMD_MODULE_XRAY:
        {
            iRet = CmdProc_XrayCmdProc(cmd, chan, pParam, pBuf);
            break;
        }
        case HOST_CMD_MODULE_XSP:
        {
            iRet = CmdProc_xspCmdProc(cmd, chan, pBuf);
            break;
        }
#endif
        case HOST_CMD_MODULE_PPM:
        {
            iRet = CmdProc_ppmCmdProc(cmd, chan, pBuf);
            break;
        }
        default:
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);

            iRet = SAL_FAIL;
            break;
        }
    }

    return iRet;
}

/**
 * @function   ProgramVersionShow
 * @brief      打印 DSP 版本与编译信息
 * @param[in]  VOID
 * @param[out] None
 * @return     static VOID
 */
static VOID ProgramVersionShow(VOID)
{
    SAL_INFO(NONE"\n");
    SAL_INFO(PURPLE "*****************************************************\n");
    SAL_INFO(PURPLE "**                                                 **\n");
    SAL_INFO(PURPLE "** MODULE       : DSP %d.%d.%d\n", DSP_COMMON_MAIN_VER, DSP_COMMON_SUB_VER, SVN_NUM);
    SAL_INFO(PURPLE "** COMPILE USER : %s\n", SAL_COMPILE_USER);
    SAL_INFO(PURPLE "** BUILD TIME   : %s %s\n", __DATE__, __TIME__);
    SAL_INFO(PURPLE "** DSP_COMMON   :main %d sub %d\n", DSP_COMMON_MAIN_VERSION, DSP_COMMON_SUB_VERSION);
    SAL_INFO(PURPLE "**                                                 **\n");
    SAL_INFO(PURPLE "*****************************************************\n");
    SAL_INFO(NONE"\n");
}

/**
 * @function   BspLogRegister
 * @brief      BSP DSP_log函数注册

 * @param[out] None
 * @return     static void
 */
static void BspLogRegister()
{
    int logsize = 0;
    int sync_size = 0;
    int net_type = 0;
    char msg[128] = {0};

    logsize = 10 * 1024 * 1024;
    sync_size = 2 * 1024;

    snprintf(msg, sizeof(msg), LOG_MODULE_INIT_FORMAT_STRING, LOG_MSG_MODULE_INIT, logsize, sync_size, net_type);

    log_msg_write(LOG_MODULE_DSP_NAME, LOG_LEVEL_CMD, msg);
    log_msg_write(LOG_MODULE_DSP_NAME, LOG_LEVEL_SYNC, " ");
}

/**
 * @function   InitDspSys
 * @brief      初始化 DSP 模块资源
 * @param[in]  DSPINITPARA **ppDspInitPara  DSP初始化结构体
 * @param[in]  DATACALLBACK pFunc           回调函数
 * @param[out] None
 * @return     int
 */
int InitDspSys(DSPINITPARA **ppDspInitPara, DATACALLBACK pFunc)
{
    /* 注册日志模块 安检机中bsp修改方案，dsp的日志都写在app日志中*/
    //BspLogRegister();

    /* 显示 DSP 模块版本与编译信息 */
    ProgramVersionShow();

    /* 显示FPGA 版本信息*/
    /* Fpga_GetVersion(); */

    /* 初始化共享结构体 */
    InitDspPara(ppDspInitPara, pFunc);

    return SAL_SOK;
}

