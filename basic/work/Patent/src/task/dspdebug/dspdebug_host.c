/**
 * @file   dspdebug_host.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  dspdebug调试服务端
 * @author dsp
 * @date   2022/5/11
 * @note
 * @note \n History
   1.日    期: 2022/5/11
     作    者: dsp
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                                   头文件区                                     */
/* ========================================================================== */
#include <sys/ioctl.h>

#include "dspdebug.h"
#include "disp_tsk_api.h"
#include "task_ism.h"
#include "libdemux.h"
#include "vdec_tsk_api.h"
#include "dup_tsk_api.h"
#include "capt_chip_drv_api.h"


/* 智能相关头文件 */
#include "sva_out.h"
#include "ba_out.h"
#include "ppm_out.h"
#include "sts.h"

/* ========================================================================== */
/*                                宏和类型定义区                                     */
/* ========================================================================== */
#define DISP_DEV_CHN_CNT 16

#define DSP_DEBUG_ARG_NUM       (16)
#define DSP_DEBUG_ARG_LEN       (32)
#define DSP_DEBUG_CMD_ARG_NUM   (4)         /* 回调函数支持的最大指令的个数 */


/* ========================================================================== */
/*                                数据结构定义区                                     */
/* ========================================================================== */
static char IaMenu[] =
{
    "\r\n ========================"
    "\r\n         IA Menu"
    "\r\n ========================"
    "\r\n Usage:"
    "\r\n"
    "\r\n   hik_echo \"ia $MOD $TYPE\" > /tmp/dspDebug"
    "\r\n"
    "\r\n eg: "
    "\r\n"
    "\r\n   hik_echo \"ia sva comm\" > /tmp/dspDebug" /* 设置显示模块的打印等级为3 */
    "\r\n"
    "\r\n args description:"
    "\r\n"
    "\r\n   MOD : sva ba ppm dbg"
    "\r\n   TYPE: (1) comm: common status"
    "\r\n         (2) alg : algrithem status"
    "\r\n         (3) all : all status info"
    "\r\n"
};


/* 回调函数，对应的指令执行的函数 */
typedef INT32 (*DSP_DEBUG_FUNC)(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4);

/* 传入参数的类型 */
typedef enum
{
    DSP_DEBUG_ARG_TYPE_INT32,
    DSP_DEBUG_ARG_TYPE_STR,
    DSP_DEBUG_ARG_TYPE_BUTT,
} DSP_DEBUG_ARG_TYPE_E;


/* 保存回调函数的参数 */
typedef struct
{
    DSP_DEBUG_ARG_TYPE_E enType;
    union
    {
        INT32 s32Arg;
        const char *szArg;
        const VOID *pvArg;
    };
} DSP_DEBUG_ARG_S;

/* 指令将相关参数的定义 */
typedef struct
{
    char cCmd;                                          // 对应指令的符号，如帮助指令"-h"，该值为h
    UINT32 u32Args;                                     // 回调函数实际生效的个数
    DSP_DEBUG_ARG_S astArgs[DSP_DEBUG_CMD_ARG_NUM];     // 传入回调函数的参数
    DSP_DEBUG_FUNC pfuncCallBack;                       // 回调函数
    char *szUsage;                                      // 使用说明
} DSP_DEBUG_CMD_S;




/* ========================================================================== */
/*                                全局变量定义区                                     */
/* ========================================================================== */
static int fd_tty_out = -1;

/* 采集相关指令定义 */
static DSP_DEBUG_CMD_S g_astCaptCmds[] = {
    {
        'a', 1,
        {
            {DSP_DEBUG_ARG_TYPE_INT32,},
        },
        capt_func_chipDebugAutoAdjust,
        "-a [chn]:vga auto adjust position"
    },

    {
        'e', 3,
        {
            {DSP_DEBUG_ARG_TYPE_INT32,},
            {DSP_DEBUG_ARG_TYPE_INT32,},
            {DSP_DEBUG_ARG_TYPE_STR,},
        },
        capt_func_chipEdidToFile,
        "-e [chn] [cable] [file]:save edid to file"
    },

    {
        'r', 2,
        {
            {DSP_DEBUG_ARG_TYPE_INT32,},
            {DSP_DEBUG_ARG_TYPE_INT32,},
        },
        capt_func_chipReloadEdid,
        "-r [chn] [cable]:reload edid from file"
    },

    {
        'p', 2,
        {
            {DSP_DEBUG_ARG_TYPE_INT32,},
            {DSP_DEBUG_ARG_TYPE_INT32,},
        },
        capt_func_chipHotPlug,
        "-p [chn] [cable]:hot plug"
    },

    {
        'u', 2,
        {
            {DSP_DEBUG_ARG_TYPE_INT32,},
            {DSP_DEBUG_ARG_TYPE_STR,},
        },
        capt_func_chipUpdateByFile,
        "-u [chn] [file]:update firmware"
    },

    {
        'h', 0,
        {
        },
        NULL,
        "-h:help"
    },

    {
        '\0',
    },
};

static char g_aszArgs[DSP_DEBUG_ARG_NUM][DSP_DEBUG_ARG_LEN];


/* ========================================================================== */
/*                                  函数申明区                                  */
/* ========================================================================== */
extern void Xsp_ShowStatus(void);
extern void Xray_ShowStatus(void);
extern void Xray_ShowXsensor(UINT32 chan);
extern void Xray_ShowPbFrame(UINT32 chan);
extern void Xray_ShowPbSlice(UINT32 chan);
extern void Xray_ShowPbPack(UINT32 chan);
extern void Xray_SetXrawMark(UINT32 chan, BOOL bEnable);
extern void SAL_setAllModLogLevel(UINT32 level);
extern void SAL_setAllModLogSaveMode(LogLevel mode);
extern void fb_hal_dumpSet(UINT32 chan, CHAR *psDumpDir, UINT32 u32DumpDp, UINT32 u32DumpCnt);
extern int sts_demo(void);   /* 维护模块dbg使用 */
extern void mem_ShowMMZ_Alloc(void);
extern void SAL_showMem_Alloc(void);
extern VOID xpack_show_package_info(UINT32 chan);
extern VOID xpack_show_display_info(UINT32 chan);
extern SAL_STATUS xpack_set_dump_prm(UINT32 u32Chan, BOOL bEnable, CHAR *pchDumpDir, UINT32 u32DumpDp, UINT32 u32DumpCnt);
extern void xpack_show_dup_time(UINT32 chan, UINT32 u32PrintType);

/**
 * @function   dspdebug_log_redirect
 * @brief      串口重定向
 * @param[in]  CHAR chTtyName[]  终端名称
 * @param[out] None
 * @return     static void
 */
static void dspdebug_log_redirect(CHAR chTtyName[])
{
    int tty_out = -1;

    /* 关闭已经打开的句柄 */
    if (fd_tty_out > 0)
    {
        close(fd_tty_out);
        fd_tty_out = -1;
    }

    /* 打开重定向设备 */
    fd_tty_out = open(chTtyName, O_RDONLY | O_WRONLY);
    if (fd_tty_out < 0)
    {
        printf("dspdebug_log_redirect failed, tty name:%s.\n", chTtyName);
        return;
    }

    /* 重定向*/
    tty_out = dup2(fd_tty_out, STDOUT_FILENO);
    if (tty_out < 0)
    {
        printf("dspdebug_log_redirect, err in dup STDOUT.\n");
        close(fd_tty_out);
        fd_tty_out = -1;
    }

    return;
}

/**
 * @function   dspdebug_dump_disp
 * @brief      dump显示数据
 * @param[in]  UINT32 u32Chan     通道
 * @param[in]  CHAR chDumpType[]  子类型
 * @param[in]  UINT32 u32DumpCnt  帧
 * @param[in]  CHAR chDumpDir[]   目的路径
 * @param[out] None
 * @return     static void
 */
static void dspdebug_dump_disp(UINT32 u32Chan, CHAR chDumpType[], UINT32 u32DumpCnt, CHAR chDumpDir[])
{
    INT32 vodev = 0;
    INT32 vochn = 0;
    INT32 dumpcnt = 0;
    INT32 arg0 = 0;

    if (NULL == chDumpType)
    {
        SAL_ERROR("the 'chDumpType' is NULL\n");
        return;
    }

    if (0 == strcmp(chDumpType, "nosignal"))
    {
        arg0 = 0;
    }
    else if (0 == strcmp(chDumpType, "vo"))
    {
        arg0 = 1;
    }
    else if (0 == strcmp(chDumpType, "nosignal_before"))
    {
        arg0 = 2;
    }
    else if (0 == strcmp(chDumpType, "fb"))
    {
        fb_hal_dumpSet(u32Chan, chDumpDir, 0, u32DumpCnt);
        return;
    }
    else
    {
        SAL_ERROR("unsupport this dump type: %s\n", chDumpType);
        return;
    }

    vodev = u32Chan / DISP_DEV_CHN_CNT;
    vochn = u32Chan % DISP_DEV_CHN_CNT;
    dumpcnt = SAL_MAX(u32DumpCnt, 1); /* 至少需要一次 */
    SAL_INFO("vodev %d vochn %d arg0 %d dumpcnt %d\n", vodev, vochn, arg0, u32DumpCnt);

    disp_tsk_dump(vodev, vochn, arg0, dumpcnt, chDumpDir);

    return;
}

#ifdef DSP_ISM

/**
 * @function   dspdebug_dump_xpack
 * @brief      dump xpack数据
 *             eg:dsp_debug -d vpssdup 0 1
 * @param[in]  UINT32 u32Chan     通道
 * @param[in]  CHAR chDumpType[]  子类型
 * @param[in]  UINT32 u32DumpCnt  帧数
 * @param[out] None
 * @return     static void
 */
static void dspdebug_dump_xpack(UINT32 u32Chan, CHAR chDumpType[], UINT32 u32DumpCnt,CHAR chDumpDir[])
{
    UINT32 u32DumpDp = 0;

    if (0 == strlen(chDumpType) || 0 == strlen(chDumpDir))
    {
        SAL_ERROR("the chDumpType is NULL\n");
        return;
    }

    if (0 == strncmp(chDumpType, "0x", 2) || 0 == strncmp(chDumpType, "0X", 2))
    {
        u32DumpDp = (UINT32)strtoul(chDumpType, NULL, 0);
    }
    else if (0 == strncmp(chDumpType, "0b", 2) || 0 == strncmp(chDumpType, "0B", 2))
    {
        u32DumpDp = (UINT32)strtoul(chDumpType + 2, NULL, 2);   /* '+2'是为了跳过开头的'0b'字符 */
    }
    else
    {
        SAL_ERROR("command: unsupport this dump type: %s\n", chDumpType);
    }

    xpack_set_dump_prm(u32Chan, (u32DumpCnt > 0 ? SAL_TRUE : SAL_FALSE), chDumpDir, u32DumpDp, u32DumpCnt);

    return;
}

/**
 * @function   dspdebug_dump_xsp
 * @brief      dump xsp数据
 * @param[in]  UINT32 u32Chan     通道
 * @param[in]  CHAR chDumpType[]  子类型
 * @param[in]  UINT32 u32DumpCnt  数量
 * @param[in]  CHAR chDumpDir[]   目的路径
 * @param[out] None
 * @return     static void
 */
static void dspdebug_dump_xsp(UINT32 u32Chan, CHAR chDumpType[], UINT32 u32DumpCnt, CHAR chDumpDir[])
{
    UINT32 u32DumpDp = 0;

    if (0 == strlen(chDumpType) || 0 == strlen(chDumpDir))
    {
        SAL_ERROR("the chDumpType is NULL\n");
        return;
    }

    if (0 == strcmp(chDumpType, "xrawin"))
    {
        u32DumpDp = XSP_DDP_XRAW_IN;
    }
    else if (0 == strcmp(chDumpType, "dispyuv-wp"))
    {
        u32DumpDp = XSP_DDP_DISP_OUT;
    }
    else if (0 == strcmp(chDumpType, "aiyuv"))
    {
        u32DumpDp = XSP_DDP_AI_YUV;
    }
    else if (0 == strcmp(chDumpType, "xrawout"))
    {
        u32DumpDp = XSP_DDP_XRAW_OUT;
    }
    else if (0 == strcmp(chDumpType, "xrawtip"))
    {
        u32DumpDp = XSP_DDP_XRAW_TIP;
    }
    else if (0 == strcmp(chDumpType, "grayfuse"))
    {
        u32DumpDp = XSP_DDP_XRAW_BLEND;
    }
    else if (0 == strcmp(chDumpType, "fullin"))
    {
        u32DumpDp = XSP_DDP_FULL_IN;
    }
    else if (0 == strcmp(chDumpType, "zeroin"))
    {
        u32DumpDp = XSP_DDP_ZERO_IN;
    }
    else if (0 == strcmp(chDumpType, "trans"))
    {
        u32DumpDp = XSP_DDP_TRANS;
    }
    else if (0 == strncmp(chDumpType, "0x", 2) || 0 == strncmp(chDumpType, "0X", 2))
    {
        u32DumpDp = (UINT32)strtoul(chDumpType, NULL, 0);
    }
    else if (0 == strncmp(chDumpType, "0b", 2) || 0 == strncmp(chDumpType, "0B", 2))
    {
        u32DumpDp = (UINT32)strtoul(chDumpType + 2, NULL, 2);   /* '+2'是为了跳过开头的'0b'字符 */
    }
    else if (0 == strcmp(chDumpType, "rtshbuf"))
    {
        u32DumpDp = XSP_DDP_RT_SHBUF;
    }
    else if (0 == strcmp(chDumpType, "pbshbuf"))
    {
        u32DumpDp = XSP_DDP_PB_SHBUF;
    }
    else
    {
        SAL_ERROR("command: unsupport this dump type: %s\n", chDumpType);
    }

    Xsp_DumpSet(u32Chan, (u32DumpCnt > 0 ? SAL_TRUE : SAL_FALSE), chDumpDir, u32DumpDp, u32DumpCnt);

    return;
}

#endif

/**
 * @function   dspdebug_log_save_mode
 * @brief      获取日志保存方式
 * @param[in]  CHAR cmd_str[]       输入保存方式字符命令
 * @param[in]  LogLevel *save_mode  输出保存方式定义
 * @param[out] None
 * @return     static BOOL
 */
static BOOL dspdebug_log_save_mode(CHAR cmd_str[], LogLevel *save_mode)
{
    if (NULL == cmd_str || NULL == save_mode)
    {
        SAL_ERROR("cmd_str %p OR save_mode %p is NULL\n", cmd_str, save_mode);
        return SAL_FALSE;
    }

    if (0 == strlen(cmd_str))
    {
        return SAL_FALSE;
    }
    else
    {
        if (0 == strcmp(cmd_str, "p"))
        {
            *save_mode = LOG_LEVEL_PRINT_ONLY;
            return SAL_TRUE;
        }
        else if (0 == strcmp(cmd_str, "ps"))
        {
            *save_mode = LOG_LEVEL_PRINT_SAVE;
            return SAL_TRUE;
        }
        else if (0 == strcmp(cmd_str, "s"))
        {
            *save_mode = LOG_LEVEL_SAVE_ONLY;
            return SAL_TRUE;
        }
        else
        {
            SAL_ERROR("command: unsupport this save mode: %s\n", cmd_str);
            return SAL_FALSE;
        }
    }
}

/**
 * @function   module_process_log
 * @brief      日志处理
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志参数
 * @param[out] None
 * @return     static void
 */
static void module_process_log(DSPDEBUG_PARAM *pstDspdebugParam)
{
    UINT32 u32LogLevel = 0;
    LogLevel enLogSaveMode = 0;
    MOD_ID enModId = MOD_MAX_NUMS;
    BOOL b_set_save_mode = SAL_FALSE;

    u32LogLevel = (UINT32)strtoul(pstDspdebugParam->chOpt[0], NULL, 0);
    b_set_save_mode = dspdebug_log_save_mode(pstDspdebugParam->chOpt[1], &enLogSaveMode); /* 获取命令指定的日志保存方式 */

    switch (pstDspdebugParam->enModule)
    {
    #ifdef DSP_ISM
    case DSPDEBUG_MODULE_XPACK:
        enModId = MOD_MEDIA_XPACK;
        break;
    case DSPDEBUG_MODULE_XSP:
        enModId = MOD_AI_XSP;
        break;
    #endif
    case DSPDEBUG_MODULE_DISP:
    case DSPDEBUG_MODULE_VI:
    case DSPDEBUG_MODULE_VOFB:
        enModId = MOD_MEDIA_DISP;
        break;
    case DSPDEBUG_MODULE_VENC:
        enModId = MOD_MEDIA_VENC;
        break;
    case DSPDEBUG_MODULE_VDEC:
        enModId = MOD_MEDIA_DEC;
        break;
    case DSPDEBUG_MODULE_AUDIO:
        enModId = MOD_MEDIA_AUD;
        break;
    case DSPDEBUG_MODULE_SVA:
        enModId = MOD_AI_SVA;
        break;
    case DSPDEBUG_MODULE_BA:
        enModId = MOD_AI_BA;
        break;
    case DSPDEBUG_MODULE_DUP:
        enModId = MOD_MEDIA_DUP;
        break;
    case DSPDEBUG_MODULE_JPEG:
        enModId = MOD_MEDIA_JPEG;
        break;
    case DSPDEBUG_MODULE_DRAW:
        enModId = MOD_MEDIA_OSD;
        break;
    case DSPDEBUG_MODULE_PACK:
    case DSPDEBUG_MODULE_UNPACK:
        enModId = MOD_MEDIA_VCA;
        break;
    case DSPDEBUG_MODULE_PPM:
        enModId = MOD_AI_PPM;
        break;
    case DSPDEBUG_MODULE_MEM:
        enModId = MOD_MEDIA_MEM;
        break;
    case DSPDEBUG_MODULE_ALL:
        SAL_setAllModLogLevel(u32LogLevel);
        if (b_set_save_mode)
        {
            SAL_setAllModLogSaveMode(enLogSaveMode);
        }
        break;
    case DSPDEBUG_MODULE_UNDEF:
        /* 重定向 */
        dspdebug_log_redirect(pstDspdebugParam->chOpt[0]);
        break;
    default:
        SAL_ERROR("unsupport the module: %d\n", pstDspdebugParam->enModule);
        break;
    }

    if (MOD_MAX_NUMS != enModId)
    {
        SAL_setModLogLevel(enModId, u32LogLevel);
        if (b_set_save_mode)
        {
            SAL_setModLogSaveMode(enModId, enLogSaveMode);
        }
    }

    return;
}

/**
 * @fn      module_process_dump
 * @brief   dspdebug之dump处理
 *
 * @param   [IN] pstDspdebugParam dump参数
 * @note    pstDspdebugParam中第1个参数为：通道号
 * @note    pstDspdebugParam中第2个参数为：DUMP类型，各模块有区别
 * @note    pstDspdebugParam中从第3个参数开始后面的参数各模块自定义
 */
static void module_process_dump(DSPDEBUG_PARAM *pstDspdebugParam)
{
    UINT32 u32Chan = 0;
    CHAR *pchDumpType = NULL;
    UINT32 u32DumpCnt = 1;
    CHAR chDumpDir[DSPDEBUG_OPT_STRLEN_MAX] = "/tmp";

    u32Chan = (UINT32)strtoul(pstDspdebugParam->chOpt[0], NULL, 0); /* 通道号 */
    pchDumpType = pstDspdebugParam->chOpt[1]; /* dump类型 */
    if (pstDspdebugParam->u32OptNum >= 3)
    {
        u32DumpCnt = (UINT32)strtoul(pstDspdebugParam->chOpt[2], NULL, 0); /* dump次数 */
    }

    switch (pstDspdebugParam->enModule)
    {
        case DSPDEBUG_MODULE_DISP:
            if (pstDspdebugParam->u32OptNum >= 4)
            {
                strcpy(chDumpDir, pstDspdebugParam->chOpt[3]); /* dump路径 */
            }

            dspdebug_dump_disp(u32Chan, pchDumpType, u32DumpCnt, chDumpDir);
            break;

        case DSPDEBUG_MODULE_VENC:
            break;

        case DSPDEBUG_MODULE_VDEC:
            break;

        case DSPDEBUG_MODULE_SVA:
            break;

#ifdef DSP_ISM
        case DSPDEBUG_MODULE_XPACK:
            if (pstDspdebugParam->u32OptNum >= 4)
            {
                strcpy(chDumpDir, pstDspdebugParam->chOpt[3]); /* dump路径 */
            }
            dspdebug_dump_xpack(u32Chan, pchDumpType, u32DumpCnt,chDumpDir);
            break;

        case DSPDEBUG_MODULE_XSP:
            if (pstDspdebugParam->u32OptNum >= 4)
            {
                strcpy(chDumpDir, pstDspdebugParam->chOpt[3]); /* dump路径 */
            }

            dspdebug_dump_xsp(u32Chan, pchDumpType, u32DumpCnt, chDumpDir);
            break;
#endif

        default:
            SAL_ERROR("unsupport the module: %d\n", pstDspdebugParam->enModule);
            break;
    }

    return;
}

/**
 * @function   ia_sva_process_status
 * @brief      sva状态信息
 * @param[in]  int start_idx                     选项起始
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  参数
 * @param[out] None
 * @return     static void
 */
static void ia_sva_process_status(int start_idx, DSPDEBUG_PARAM *pstDspdebugParam)
{
    char *arg0 = pstDspdebugParam->chOpt[start_idx++];
    char *arg1 = pstDspdebugParam->chOpt[start_idx++];

    if (0 == strcmp(arg0, "comm"))
    {
        sts_pr_node_info_by_name("sva", "comm");
    }
    else if (0 == strcmp(arg0, "alg"))
    {
        sts_pr_node_info_by_name("sva", "alg");
    }
    else if (0 == strcmp(arg0, "dump"))
    {
        int flag = strtoul(arg1, NULL, 0);
        printf("get flag %d \n", flag);

        Sva_HalSetDumpYuvFlag(!!flag);
    }
    else  /* 默认打印所有维护信息 */
    {
        sts_pr_node_info_by_name("sva", NULL);
    }
}

/**
 * @function   ia_ba_process_status
 * @brief      ba状态处理
 * @param[in]  int start_idx                     选项起始
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  参数
 * @param[out] None
 * @return     static void
 */
static void ia_ba_process_status(int start_idx, DSPDEBUG_PARAM *pstDspdebugParam)
{
    /* todo: 待行为分析适配调试命令，当前未实现 */

    return;
}

/**
 * @function   ia_ppm_process_status
 * @brief      ppm状态处理
 * @param[in]  int start_idx                     选项起始
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  参数
 * @param[out] None
 * @return     static void
 */
static void ia_ppm_process_status(int start_idx, DSPDEBUG_PARAM *pstDspdebugParam)
{
    /* todo: 待人包关联适配调试命令，当前未实现 */

    return;
}

/**
 * @function   ia_process_status
 * @brief      ia（sva，ba，ppm，等）状态处理
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam
 * @param[out] None
 * @return     static void
 */
static void ia_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    int start_idx = 0;

    char *arg0 = (CHAR *)pstDspdebugParam->chOpt[start_idx++];

    if (pstDspdebugParam->u32OptNum < 1)
    {
        printf("%s", IaMenu);
        return;
    }

    if (0 == strcmp(arg0, "sva"))
    {
        ia_sva_process_status(start_idx, pstDspdebugParam);
    }
    else if (0 == strcmp(arg0, "ba"))
    {
        ia_ba_process_status(start_idx, pstDspdebugParam);
    }
    else if (0 == strcmp(arg0, "ppm"))
    {
        ia_ppm_process_status(start_idx, pstDspdebugParam);
    }

    /* 支持智能模块拓展... */


    return;
}

/**
 * @function   ppm_process_status
 * @brief      设置sva模块的debug
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志选项
 * @param[out] None
 * @return     static void
 */
static void ppm_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    char arg0 = *(char *)(pstDspdebugParam->chOpt[0]);
    INT32 arg1 = strtoul(pstDspdebugParam->chOpt[1], NULL, 0);
    INT32 arg2 = strtoul(pstDspdebugParam->chOpt[2], NULL, 0);
    INT32 arg3 = strtoul(pstDspdebugParam->chOpt[3], NULL, 0);
    INT32 arg4 = strtoul(pstDspdebugParam->chOpt[4], NULL, 0);

    /* 设置debug printf等级参数 */
    if (arg0 == 'l')
    {
        SAL_INFO("args %c ,level:%d \n", arg0, arg1);
        (VOID)SAL_setModLogLevel(MOD_AI_PPM, arg1);
    }
    else if (arg0 == 's')
    {
        SAL_INFO("args %c, sync gap: %d \n", arg0, arg1);
        set_sync_gap(SAL_TRUE, arg1);
    }
    else if (arg0 == 'd')
    {
        SAL_INFO("args %c, debug switch %d \n", arg0, arg1);
        (VOID)Ppm_DrvSetDbgSwitch(arg1);
    }
    else if (arg0 == 'p')
    {
        SAL_INFO("args %c, start print debug prm \n", arg0);
        Ppm_DrvPrintDbgPrm();
    }
    else if (arg0 == 'o')
    {
        SAL_INFO("args %c, pos print flag %d \n", arg0, arg1);
        (VOID)Ppm_DrvSetPosInfoFlag(arg1);
    }
    else if (arg0 == 'n')
    {
        SAL_INFO("args %c, sync cnt %d \n", arg0, arg1);
        (VOID)Ppm_DrvSetSyncGapCnt(arg1);
    }
    else if (arg0 == 'a')   /* 设置可见光包裹的检测灵敏度 */
    {
        SAL_INFO("args %c, sync cnt %d \n", arg0, arg1);
        (VOID)Ppm_DrvSetPackSensity(arg1);
    }
	else if (arg0 == 'b')   /* 开关metadata解析的json字段信息 */
	{
		SAL_INFO("args %c, sync cnt %d \n", arg0, arg1);
		(VOID)Ppm_DrvSetMetaCtlFlag(arg1);
	}
	else if (arg0 == 'c')
	{
		SAL_INFO("args %c \n", arg0);	
		ppm_debug_dump_depth();
	}
	else if (arg0 == 'f')
	{
		SAL_INFO("args %c \n", arg0);	
		Ppm_DrvParsePos();
	}
	else if (arg0 == 'g')
	{
		SAL_INFO("args %c, depth sync gap %d \n", arg0, arg1);	
		Ppm_DrvSetDepthSyncGap(arg1);
	}
    else
    {
        SAL_INFO("args %c ,%d ,%d ,%d ,%d\n", arg0, arg1, arg2, arg3, arg4);
    }
}

/**
 * @function   ba_process_status
 * @brief      设置ba模块的debug
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志选项
 * @param[out] None
 * @return     static void
 */
static void ba_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    char arg0 = *(char *)(pstDspdebugParam->chOpt[0]);
    INT32 arg1 = strtoul(pstDspdebugParam->chOpt[1], NULL, 0);
    INT32 arg2 = strtoul(pstDspdebugParam->chOpt[2], NULL, 0);
    INT32 arg3 = strtoul(pstDspdebugParam->chOpt[3], NULL, 0);
    INT32 arg4 = strtoul(pstDspdebugParam->chOpt[4], NULL, 0);

    /* 设置debug printf等级参数 */
    if (arg0 == 'l')
    {
        SAL_INFO("args %c ,level:%d \n", arg0, arg1);
        SAL_setModLogLevel(MOD_AI_BA, arg1);
    }
    else
    {
        SAL_INFO("args %c ,%d ,%d ,%d ,%d\n", arg0, arg1, arg2, arg3, arg4);
    }
}

/**
 * @function   sva_process_status
 * @brief      设置sva模块的debug
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志选项
 * @param[out] None
 * @return     static void
 */
static void sva_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
#ifndef SVA_NO_USE
    char arg0 = 0;    /* (char *)(pacOpt[0]); */
    INT32 arg1 = strtoul(pstDspdebugParam->chOpt[1], NULL, 0);
    INT32 arg2 = strtoul(pstDspdebugParam->chOpt[2], NULL, 0);
    INT32 arg3 = strtoul(pstDspdebugParam->chOpt[3], NULL, 0);
    INT32 arg4 = strtoul(pstDspdebugParam->chOpt[4], NULL, 0);

    memcpy(&arg0, (char *)(pstDspdebugParam->chOpt[0]), sizeof(char));

    /* 设置debug printf等级参数 */
    if (arg0 == 'l')
    {
        SAL_INFO("args %c ,level:%d \n", arg0, arg1);
        (VOID)Sva_DrvSetDbLevel(arg1);
        (VOID)SAL_setModLogLevel(MOD_AI_SVA, arg1);
    }
    else if (arg0 == 'j')
    {
        SAL_INFO("args %c, level:%d \n", arg0, arg1);
        Sva_DrvDbgSaveJpg(arg1);
    }
    else if (arg0 == 'a')
    {
        SAL_INFO("arg0:%c, arg1:%d, arg2:%d \n", arg0, arg1, arg2);
        (VOID)Sva_HalSetAlgDbgFlag(arg1);
        (VOID)Sva_HalSetAlgDbgLevel(arg2);  /* 耗时阈值设定最大100ms */
    }
    else if (arg0 == 't')
    {
        SAL_INFO("arg0:%c, arg1:%d \n", arg0, arg1);
        (VOID)Sva_DrvTransSetPrtFlag(!!arg1);
    }
    else if (arg0 == 's')
    {
        SAL_INFO("arg0:%c, arg1:%d \n", arg0, arg1);
        Sva_DrvSetSplitDbgSwitch(arg1);
    }
    else if (arg0 == 'd')
    {
        SAL_INFO("arg0:%c, arg1:%d, arg2:%d arg3:0x%x \n", arg0, arg1, arg2, arg3);
        (VOID)Sva_HalSetDumpYuvCnt(arg1, arg2, arg3);/*arg1:通道号 arg2:下载图片个数, arg3:下载模式(选择不同位置的图片，可选多个图片)*/
    }
    else if (arg0 == 'e')
    {
        SAL_INFO("arg0:%c, arg1:%d \n", arg0, arg1);
        Sva_DrvGetQueDbgInfo();
    }
    else if (arg0 == 'p')
    {
        SAL_INFO("arg0:%c, arg1:%d \n", arg0, arg1);
        Sva_DrvSetPrOutInfoFlag(arg1); /*arg1:0表示关闭，1表示只打印引擎传出的智能结果，2表示只打印SVA模块传出的智能结果，3表示两者都打印,>3两者都打印，且保存附带智能结果框的yuv数据*/
    }
    else if (arg0 == 'b')
    {
        SAL_INFO("arg0:%c \n", arg0);
        Sva_DrvChgPosWriteFlag(arg1);
    }
    else if (arg0 == 'z')
    {
        SAL_INFO("arg0:%c \n", arg0);
        (VOID)sts_demo();    /* 维护子模块demo */
    }
    else
    {
        SAL_INFO("args %c ,%d ,%d ,%d ,%d\n", arg0, arg1, arg2, arg3, arg4);
    }

#endif
}

/**
 * @function   dup_process_status
 * @brief      设置显示dup模块的debug
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志选项
 * @param[out] None
 * @return     static void
 */
static void dup_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    char arg0 = *(char *)(pstDspdebugParam->chOpt[0]);
    INT32 arg1 = strtoul(pstDspdebugParam->chOpt[1], NULL, 0);
    INT32 arg2 = strtoul(pstDspdebugParam->chOpt[2], NULL, 0);
    INT32 arg3 = strtoul(pstDspdebugParam->chOpt[3], NULL, 0);
    INT32 arg4 = strtoul(pstDspdebugParam->chOpt[4], NULL, 0);

    /* 设置debug printf等级参数 */
    if (arg0 == 'l')
    {
        SAL_INFO("args %c ,level:%d \n", arg0, arg1);
        dup_task_setDbLeave(arg1);
        SAL_setModLogLevel(MOD_MEDIA_DUP, arg1);
    }
    else
    {
        SAL_INFO("args %c ,%d ,%d ,%d ,%d\n", arg0, arg1, arg2, arg3, arg4);
    }
}

/**
 * @function   unpack_process_status
 * @brief      设置显示解包模块的debug
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志选项
 * @param[out] None
 * @return     static void
 */
static void unpack_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    char arg0 = *(char *)(pstDspdebugParam->chOpt[0]);
    INT32 arg1 = strtoul(pstDspdebugParam->chOpt[1], NULL, 0);
    INT32 arg2 = strtoul(pstDspdebugParam->chOpt[2], NULL, 0);
    INT32 arg3 = -1;
    INT32 arg4 = -1;

    /* 设置debug printf等级参数 */
    if (arg0 == 'l')
    {
        SAL_INFO("args %c ,level:%d ,unLevel %d\n", arg0, arg1, arg2);
        DemuxSetDbLeave(arg1, arg2);
    }
    else
    {
        SAL_INFO("args %c ,%d ,%d ,%d ,%d\n", arg0, arg1, arg2, arg3, arg4);
    }
}

/**
 * @function   vdec_process_status
 * @brief      设置显示解码模块的debug
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  日志选项
 * @param[out] None
 * @return     static void
 */
static void vdec_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    char arg0 = *(char *)(pstDspdebugParam->chOpt[0]);
    UINT32 arg1 = strtoul(pstDspdebugParam->chOpt[1], NULL, 0);
    INT32 arg2 = -1;
    INT32 arg3 = -1;
    INT32 arg4 = -1;

    /* 设置debug printf等级参数 */
    if (arg0 == 'l')
    {
        SAL_INFO("args %c ,level:%d \n", arg0, arg1);
        vdec_tsk_SetDbLeave(arg1);
        SAL_setModLogLevel(MOD_MEDIA_DEC, arg1);
    }
    else
    {
        SAL_INFO("args %c ,%d ,%d ,%d ,%d\n", arg0, arg1, arg2, arg3, arg4);
    }
}
/**
 * @function   mem_process_status
 * @brief      mem申请内存统计
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  调试选项
 * @param[out] None
 * @return     static void
 */
static void mem_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{

    CHAR *pStatusType = pstDspdebugParam->chOpt[0]; /* Status类型 */

    if (0 == strcmp(pStatusType, "mmzsize"))
    {
        mem_ShowMMZ_Alloc();
    }
    else if (0 == strcmp(pStatusType, "memsize"))
    {
        SAL_showMem_Alloc();
    }
    else
    {
        SAL_print("\nWrong Prm!!!\n");
    }
}

#ifdef DSP_ISM

/**
 * @function   xsp_process_status
 * @brief      xsp状态处理
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  调试选项
 * @param[out] None
 * @return     static void
 */
static void xsp_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    INT32 s32Val1 = 0, s32Val2 = 0;
    INT32 s32KeyIdx = 0, s32KeyVal = -1;

    UINT32 u32Chan = (UINT32)strtoul(pstDspdebugParam->chOpt[0], NULL, 0);
    CHAR *pStatusType = pstDspdebugParam->chOpt[1]; /* Status类型 */
    UINT32 u32TimePrintType = (UINT32)strtoul(pstDspdebugParam->chOpt[2], NULL, 0); // 时间打印格式, 

    if (0 == strcmp(pStatusType, "xsensor"))
    {
        Xray_ShowXsensor(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "time"))
    {
        if (0 == (u32TimePrintType & (0x4 | 0x2 | 0x1)))
        {
            // 设定默认打印类型为时间点打印
            u32TimePrintType = 0x1;
        }
        Xsp_ShowTime(u32Chan, u32TimePrintType);
    }
    else if (0 == strcmp(pStatusType, "xrawmark"))
    {
        Xray_SetXrawMark(u32Chan, (INT32)strtol(pstDspdebugParam->chOpt[2], NULL, 0));
    }
    else if (0 == strcmp(pStatusType, "idt"))
    {
        Xsp_ShowIdentify(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "slicb"))
    {
        Xsp_ShowSliceCb(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "pbframe"))
    {
        Xray_ShowPbFrame(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "pbslice"))
    {
        Xray_ShowPbSlice(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "pbpack"))
    {
        Xray_ShowPbPack(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "pbout"))
    {
        Xsp_ShowPb2Xpack(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "pbin"))
    {
        Xray_ShowPbFrame(u32Chan);
        Xray_ShowPbSlice(u32Chan);
        Xray_ShowPbPack(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "akey"))
    {
        if (pstDspdebugParam->u32OptNum >= 3)
        {
            s32KeyIdx = (INT32)strtol(pstDspdebugParam->chOpt[2], NULL, 0);
        }

        if (pstDspdebugParam->u32OptNum >= 4)
        {
            s32Val1 = (INT32)strtol(pstDspdebugParam->chOpt[3], NULL, 0);
        }

        if (pstDspdebugParam->u32OptNum >= 5)
        {
            s32Val2 = (INT32)strtol(pstDspdebugParam->chOpt[4], NULL, 0);
        }

        if (pstDspdebugParam->u32OptNum == 2)       /* 命令为：dspdebug -s xsp chan akey */
        {
            if (SAL_SOK != Xsp_DrvShowAllAlgKeyAndValue(u32Chan))
            {
                SAL_print("chan %u, Xsp_DrvShowAllAlgKeyAndValue failed\n", u32Chan);
            }
        }
        else if (pstDspdebugParam->u32OptNum == 3)  /* 命令格式为：dspdebug -s xsp chan akey 0x00000001 */
        {
            if (SAL_SOK == Xsp_DrvGetAlgKey(u32Chan, s32KeyIdx, &s32KeyVal, &s32Val1, &s32Val2))
            {
                SAL_print("type \t key       \t value1 \t value2 \t\n");
                if (-1 != s32KeyVal)  /* s32KeyVal默认为-1，若s32KeyVal变化了表示读取了图像处理参数 */
                {
                    SAL_print("0 \t 0x%08x \t %-8d \t -\n", s32KeyIdx, s32KeyVal); /* 打印图像处理相关的参数值 */
                }

                SAL_print("1 \t 0x%08x \t %-8d \t %-8d\n", s32KeyIdx, s32Val1, s32Val2); /* 打印参数设置相关的参数值 */
            }
        }
        else        /* 命令格式为：dspdebug -s xsp chan akey 0x00000001 val1 [val2] */
        {
            if (SAL_SOK != Xsp_DrvSetAlgKey(u32Chan, pstDspdebugParam->u32OptNum, s32KeyIdx, s32Val1, s32Val2))
            {
                SAL_print("chan %u, Xsp_DrvSetAlgKey failed, KeyIdx: 0x%x, Val1: %d, Val2: %d\n", u32Chan, s32KeyIdx, s32Val1, s32Val2);
            }
        }
    }
    else
    {
        SAL_print("\n================ XSP Status @ %llu ms ================\n", sal_get_tickcnt());
        Xray_ShowStatus();
        Xsp_ShowStatus();
    }

    return;
}


/**
 * @function   xpack_process_status
 * @brief      xpack状态信息
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  状态选项
 * @param[out] None
 * @return     static void
 */
static void xpack_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    UINT32 u32Chan = (UINT32)strtoul(pstDspdebugParam->chOpt[0], NULL, 0);
    CHAR *pStatusType = pstDspdebugParam->chOpt[1]; /* Status类型 */
    UINT32 u32TimePrintType = (UINT32)strtoul(pstDspdebugParam->chOpt[2], NULL, 0); // 时间打印格式

    if (0 == strcmp(pStatusType, "package"))
    {
       xpack_show_package_info(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "display"))
    {
       xpack_show_display_info(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "sendtime"))
    {
        if (0 == (u32TimePrintType & (0x4 | 0x2 | 0x1)))
        {
            // 设定默认打印类型为时间点打印
            u32TimePrintType = 0x1;
        }
        xpack_show_dup_time(u32Chan, u32TimePrintType);
    }

    return;
}

#endif

/**
 * @function   disp_process_status
 * @brief      显示状态信息
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  状态选项
 * @param[out] None
 * @return     static void
 */
static void disp_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    UINT32 u32Chan = (UINT32)strtoul(pstDspdebugParam->chOpt[0], NULL, 0);
    CHAR *pStatusType = pstDspdebugParam->chOpt[1]; /* Status类型 */

    if (0 == strcmp(pStatusType, "win"))
    {
        disp_tskShowVoWin(u32Chan);
    }
    else if (0 == strcmp(pStatusType, "time"))
    {
        disp_tskShowVoTime(u32Chan);
    }
}

/**
 * @function   module_process_status
 * @brief      dspdebug状态信息获取处理
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  状态选项
 * @param[out] None
 * @return     static void
 */
static void module_process_status(DSPDEBUG_PARAM *pstDspdebugParam)
{
    /* checker */
    if (pstDspdebugParam->u32OptNum > DSPDEBUG_OPTIONS_NUM_MAX)
    {
        SAL_LOGE("invalid opt num %d > max %d \n", pstDspdebugParam->u32OptNum, DSPDEBUG_OPTIONS_NUM_MAX);
        return;
    }

    switch (pstDspdebugParam->enModule)
    {
        case DSPDEBUG_MODULE_DISP:
        {
            disp_process_status(pstDspdebugParam);
            break;
        }
#ifdef DSP_ISM
        case DSPDEBUG_MODULE_XPACK:
        {
            xpack_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_MODULE_XSP:
        {
            xsp_process_status(pstDspdebugParam);
            break;
        }
#endif
        case DSPDEBUG_MODULE_VDEC:
        {
            vdec_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_MODULE_UNPACK:
        {
            unpack_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_MODULE_DUP:
        {
            dup_process_status(pstDspdebugParam);
            break;
        }
#if 1   /* 后续智能模块介由统一的维护模块进行处理，参考 DSPDEBUG_MODULE_IA */
        case DSPDEBUG_MODULE_SVA:
        {
            sva_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_MODULE_BA:
        {
            ba_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_MODULE_PPM:
        {
            ppm_process_status(pstDspdebugParam);
            break;
        }
#endif
        case DSPDEBUG_MODULE_IA:
        {
            ia_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_MODULE_MEM:
        {
             mem_process_status(pstDspdebugParam);
             break;
        }

        default:
        {
            SAL_WARN("DSP V%d.%d.%d @ %s %s # %s\n", DSP_COMMON_MAIN_VER, DSP_COMMON_SUB_VER, SVN_NUM, __DATE__, __TIME__, SAL_COMPILE_USER);
            SAL_ERROR("unsupport the module: %d\n", pstDspdebugParam->enModule);
            break;
        }
    }

    return;
}


/**
 * @function   vi_process_cmd_execute
 * @brief      vi指令执行
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  状态选项
 * @param[out] None
 * @return     static void
 */
static INT32 vi_process_cmd_execute(UINT32 u32Argc)
{
    DSP_DEBUG_CMD_S *pstCmd = g_astCaptCmds;
    DSP_DEBUG_ARG_S *pstArg = NULL;
    const char *szArg = NULL;
    UINT32 u32ArgIdx = 0;
    UINT32 i = 0;

    if (0 == u32Argc)
    {
        SAL_ERROR("no valid input\n");
        return SAL_FAIL;
    }

    /* 第一个参数为模块名 */
    szArg = g_aszArgs[u32ArgIdx++];
    if (0 != strcmp(szArg, "capt"))
    {
        SAL_ERROR("mod[%s]  error\n", g_aszArgs[0]);
        return SAL_FAIL;
    }
   
    szArg = g_aszArgs[u32ArgIdx++];

    /* 当前仅支持执行一条指令，后续支持同时执行多条指令，敬请期待！！！ */
    while ('\0' != pstCmd->cCmd)
    {
        if ((szArg[0] == pstCmd->cCmd) || ((szArg[0] == '-') && (szArg[1] == pstCmd->cCmd)))
        {
            if ('h' == pstCmd->cCmd)
            {
                SAL_ERROR("mod[%s] cmd[%c] arg[%u] error\n", "capt", pstCmd->cCmd, i);
                return SAL_SOK;
            }

            if (u32Argc > u32ArgIdx + pstCmd->u32Args)
            {
                SAL_ERROR("mod[%s] cmd[%c] need %u args but %u\n", "capt", pstCmd->cCmd, pstCmd->u32Args, u32Argc - u32ArgIdx);
                return SAL_FAIL;
            }

            /* 解析回调函数的入参 */
            pstArg = pstCmd->astArgs;
            for (i = 0; (i < pstCmd->u32Args) && (i < DSP_DEBUG_CMD_ARG_NUM); i++)
            {
                szArg = g_aszArgs[u32ArgIdx++];
                switch (pstArg->enType)
                {
                    case DSP_DEBUG_ARG_TYPE_INT32:
                        pstArg->s32Arg = atoi(szArg);
                        break;
                    case DSP_DEBUG_ARG_TYPE_STR:
                        pstArg->szArg = szArg;
                        break;
                    default:
                        SAL_ERROR("mod[%s] cmd[%c] arg[%u] type[%d] error\n", "capt", pstCmd->cCmd, i, pstArg->enType);
                        return SAL_FAIL;
                }

                pstArg++;
            }

            /* 执行对应指令 */
            if (NULL != pstCmd->pfuncCallBack)
            {
                pstCmd->pfuncCallBack(&pstCmd->astArgs[0].pvArg, &pstCmd->astArgs[1].pvArg, &pstCmd->astArgs[2].pvArg, &pstCmd->astArgs[3].pvArg);

            }

            return SAL_SOK;
        }

        pstCmd++;
    }

    SAL_ERROR("mod[%s] invalid cmd[%s]\n", "capt", szArg);

    return SAL_FAIL;
}


/*******************************************************************************
* 函数名  : module_process_cmd_parse
* 描  述  : 解析输入的字符串并分段保存
* 输  入  : const char *szStr : 输入字符串
* 输  出  : 
* 返回值  : 分段个数
*******************************************************************************/
static UINT32 module_process_cmd_parse(const char *szStr)
{
    UINT32 u32Argc = 0;
    UINT32 u32Len = 0;
    UINT32 u32Start = 0;
    const char *szTmp = szStr;

    if(NULL == szStr)
    {
        SAL_LOGE("szStr is NULL\n");
        return 0;
    }

	memset(&g_aszArgs, 0, sizeof(g_aszArgs));

    while (*szTmp)
    {
        /* 有效值后的第一个空格 */
        if ((' ' == *szTmp) && (u32Len > 0))
        {
            sal_memcpy_s(g_aszArgs[u32Argc], sizeof(g_aszArgs[u32Argc]), szStr + u32Start, u32Len);
            u32Argc++;
            u32Len = 0;
        }
        /* 不可见字符认为是结束符 */
        else if (*szTmp < 0x20)
        {
            if (u32Len > 0)
            {
                sal_memcpy_s(g_aszArgs[u32Argc], sizeof(g_aszArgs[u32Argc]), szStr + u32Start, u32Len);
                u32Argc++;
            }

            return u32Argc;
        }
        else if (' ' != *szTmp)
        {
            if (0 == u32Len++)
            {
                u32Start = szTmp - szStr;
            }
        }

        szTmp++;
    }

    if (u32Len > 0)
    {
        sal_memcpy_s(g_aszArgs[u32Argc], sizeof(g_aszArgs[u32Argc]), szStr + u32Start, u32Len);
        u32Argc++;
    }

    return u32Argc;
}


/**
 * @function   module_process_cmd_execute
 * @brief      指令执行
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  状态选项
 * @param[out] None
 * @return     static void
 */
static void module_process_cmd_execute(DSPDEBUG_PARAM *pstDspdebugParam)
{
    UINT32 i = 0;
    UINT32 u32Argc = 0;
    INT32 i32Ret = 0;
    /* checker */
    if (pstDspdebugParam->u32OptNum > DSPDEBUG_OPTIONS_NUM_MAX)
    {
        SAL_LOGE("invalid opt num %d > max %d \n", pstDspdebugParam->u32OptNum, DSPDEBUG_OPTIONS_NUM_MAX);
        return;
    }

    switch (pstDspdebugParam->enModule)
    {
        case DSPDEBUG_MODULE_VI:
        {
            for(i = 0;i < pstDspdebugParam->u32OptNum; i++)
            {
                u32Argc = module_process_cmd_parse(pstDspdebugParam->chOpt[i]);
                if(u32Argc == 0)
                {
                    SAL_LOGE("the %d cmd parse error %s\n",i+1,pstDspdebugParam->chOpt[i]);
                    continue;
                }
                i32Ret = vi_process_cmd_execute(u32Argc);
                if(SAL_SOK != i32Ret)
                {
                    SAL_LOGE("the %d cmd execute error %s\n",i+1,pstDspdebugParam->chOpt[i]);
                    continue;
                }
            }
            break;
        }
        default:
        {
            SAL_ERROR("unsupport the module: %d\n", pstDspdebugParam->enModule);
            break;
        }
    }

    return;
}


/**
 * @function   dspdebug_analysis
 * @brief      命令分析
 * @param[in]  DSPDEBUG_PARAM *pstDspdebugParam  调试选项
 * @param[out] None
 * @return     static void
 */
static void dspdebug_analysis(DSPDEBUG_PARAM *pstDspdebugParam)
{
    if (pstDspdebugParam == NULL)
    {
        SAL_ERROR("the 'pstDspdebugParam' is NULL\n");
        return;
    }

    switch (pstDspdebugParam->enFuncOpt)
    {
        case DSPDEBUG_FUNC_LOG:
        {
            module_process_log(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_FUNC_DUMP:
        {
            module_process_dump(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_FUNC_STATUS:
        {
            module_process_status(pstDspdebugParam);
            break;
        }
        case DSPDEBUG_FUNC_CMD_EXECUTE:
        {
            module_process_cmd_execute(pstDspdebugParam);
            break;
        }
        default:
        {
            return;
        }
    }

    return;
}

/**
 * @function   dspdebug_handle
 * @brief      dspdebug服务线程
 * @param[in]  void
 * @param[out] None
 * @return     static void *
 */
static void *dspdebug_handle(void)
{
    INT32 ret_val = 0;
    mqd_t mq_dspdebug;
    DSPDEBUG_PARAM stDspdebugParam = {0};

    if (SAL_SOK != SAL_MqInit(&mq_dspdebug, DSPDEBUG_MQ_NAME, DSPDEBUG_MQ_NUM_MAX, sizeof(DSPDEBUG_PARAM)))
    {
        SAL_ERROR("init '%s' failed\n", DSPDEBUG_MQ_NAME);
        return NULL;
    }

    SAL_SET_THR_NAME();

    while (1)
    {
        ret_val = SAL_MqRecv(mq_dspdebug, &stDspdebugParam, sizeof(DSPDEBUG_PARAM), SAL_TIMEOUT_FOREVER, NULL);

        if (sizeof(DSPDEBUG_PARAM) == ret_val)
        {
            dspdebug_analysis(&stDspdebugParam);
        }
        else
        {
            SAL_ERROR("SAL_MqRecv failed, ret_val: %d, sizeof: %zu\n", ret_val, sizeof(DSPDEBUG_PARAM));
        }

        SAL_msleep(500);
    }

    return NULL;
}

/**
 * @function   dspdebug_init
 * @brief      dspdebug初始化
 * @param[in]  void
 * @param[out] None
 * @return     void
 */
void dspdebug_init(void)
{
    SAL_ThrHndl dsp_mq_receive;

    SAL_thrCreate(&dsp_mq_receive, (void *)dspdebug_handle, SAL_THR_PRI_DEFAULT, 0, NULL);

    return;
}

