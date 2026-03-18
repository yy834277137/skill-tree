/*******************************************************************************
 * sal_dspdebug.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : heshengyuan <heshengyuan@hikvision.com>
 * Version: V1.0.0  2014年4月2日 Create
 *
 * Description : 实现进程调试，单元测试，通过接收命令的方式能够实时的监视DSP的各个子模块工作状态
 * Modification:
 *******************************************************************************/

#ifndef _DSP_DEBUG_H_
#define _DSP_DEBUG_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                                   头文件区                                  */
/* ========================================================================== */
#include "sal.h"


/* ========================================================================== */
/*                                宏和类型定义区                                */
/* ========================================================================== */

/* 进程通讯文件，放置设备tmp下，使用linux隐藏文件的方式 */
#define FILE_DSP_DEBUG         "/tmp/.dspd"
#define FILE_DSP_DEBUG_V2      "/tmp/dspDebug"


#define DBG_CMD_MAX_LEN         32          /* 单个命令和参数的字符最大长度 */
#define DBG_CMD_INPUT_LEN       32          /* 输入命令字符最大长度 */
#define DBG_CMD_NUM             8           /* 输入命令和参数的最多个数 */

#define IPC_FLAG_CREATE         0x01

/* 最大的参数结构体的长度 */
#define MAX_PARAM_LENGTH        256

#define CMD_TTY_CHANGE          0x1000
#define CMD_DSP_VDEC            0x1001
#define CMD_DSP_JDEC            0x1002
#define CMD_DSP_VINT            0x1002
#define CMD_DSP_VENC            0x1003
#define CMD_DSP_JENC            0x1004
#define CMD_DSP_VOUT            0x1005
#define CMD_DSP_VOFB            0x1006
#define CMD_DSP_AIAO            0x1007
#define CMD_DSP_SYS             0x1008
#define CMD_DSP_DFR             0x1009

#define CMD_DSP_SISP            0x100C
#define CMD_DSP_GISP            0x100D
#define CMD_DSP_WDR             0x100E



/* 方便调试用的自定义的命令，DSP模块调试从0x1000开始 */
#define CMD_DEBUG_DISP            0x1000
#define CMD_DEBUG_VDEC            0x1001
#define CMD_DEBUG_JPEG            0x1002
#define CMD_DEBUG_VI              0x1003
#define CMD_DEBUG_VENC            0x1004
#define CMD_DEBUG_DRAW            0x1005
#define CMD_DEBUG_AUDIO           0x1006
#define CMD_DEBUG_VOFB            0x1007
#define CMD_DEBUG_PACK            0x1008
#define CMD_DEBUG_UNPACK          0x1009
#define CMD_DEBUG_SVA             0x1010
#define CMD_DEBUG_BA              0x1011
#define CMD_DEBUG_DUP             0x1012
#define CMD_DEBUG_XPACK           0x1013
#define CMD_DEBUG_XSP             0x1014
#define CMD_DEBUG_PPM             0x1015
#define CMD_DEBUG_IA              0x1016          /* 智能维护子模块 */
#define CMD_DEBUG_MEM             0x1017          /* 内存统计模块 */

#define DSPDEBUG_MQ_NAME	            "/mq_dspdebug"  /* dspdebug命令消息队列名 */
#define DSPDEBUG_MQ_NUM_MAX	            17      /* dspdebug命令消息队列最大个数 */
#define DSPDEBUG_MODULE_NAME_LEN    	16      /* 模块名最大长度 */
#define DSPDEBUG_ARGS_CNT_MIN           3       /* dspdebug命令最少参数个数 */
#define DSPDEBUG_OPTIONS_NUM_MAX        8       /* dspdebug命令最多选项个数，不计主程序名、功能选项和模块名 */
#define DSPDEBUG_OPT_STRLEN_MAX         32      /* dspdebug命令参数字符串最大长度，不超过32个，包括“\0” */

typedef enum
{
    DSPDEBUG_MODULE_UNDEF,      /* 未定义 */
    DSPDEBUG_MODULE_DISP        = CMD_DEBUG_DISP,
    DSPDEBUG_MODULE_VI          = CMD_DEBUG_VI,
    DSPDEBUG_MODULE_VENC        = CMD_DEBUG_VENC,
    DSPDEBUG_MODULE_VDEC        = CMD_DEBUG_VDEC,
    DSPDEBUG_MODULE_AUDIO       = CMD_DEBUG_AUDIO,
    DSPDEBUG_MODULE_SVA         = CMD_DEBUG_SVA,
    DSPDEBUG_MODULE_BA          = CMD_DEBUG_BA,
    DSPDEBUG_MODULE_DUP         = CMD_DEBUG_DUP,
    DSPDEBUG_MODULE_XPACK       = CMD_DEBUG_XPACK,
    DSPDEBUG_MODULE_XSP         = CMD_DEBUG_XSP,
    DSPDEBUG_MODULE_JPEG        = CMD_DEBUG_JPEG,
    DSPDEBUG_MODULE_DRAW        = CMD_DEBUG_DRAW,
    DSPDEBUG_MODULE_VOFB        = CMD_DEBUG_VOFB,
    DSPDEBUG_MODULE_PACK        = CMD_DEBUG_PACK,
    DSPDEBUG_MODULE_UNPACK      = CMD_DEBUG_UNPACK,
    DSPDEBUG_MODULE_PPM         = CMD_DEBUG_PPM,
    DSPDEBUG_MODULE_IA          = CMD_DEBUG_IA,
    DSPDEBUG_MODULE_MEM         = CMD_DEBUG_MEM,

    DSPDEBUG_MODULE_ALL         = 0xFFFFFFFF,
}DSPDEBUG_MODULE;

typedef enum
{
    DSPDEBUG_FUNC_UNDEF,        /* 未定义 */
    DSPDEBUG_FUNC_LOG,          /* -l，Log功能 */
    DSPDEBUG_FUNC_DUMP,         /* -d，Dump功能 */
    DSPDEBUG_FUNC_STATUS,       /* -s，Status功能 */
    DSPDEBUG_FUNC_CMD_EXECUTE,  /* dspdebug程序暂不用，其他调试程序用，如：edid*/
} DSPDEBUG_FUNCOPT;


/* ========================================================================== */
/*                                数据结构定义区                                */
/* ========================================================================== */
typedef struct
{
    DSPDEBUG_FUNCOPT enFuncOpt; /* 功能选项 */
    DSPDEBUG_MODULE enModule; /* 模块名 */
    UINT32 u32OptNum; /* dspdebug命令个数，不超过DSPDEBUG_OPTIONS_NUM_MAX */
    CHAR chOpt[DSPDEBUG_OPTIONS_NUM_MAX][DSPDEBUG_OPT_STRLEN_MAX]; /* dspdebug命令选项，通过字符串传入DSP主程序，主程序内各模块分别解析 */
} DSPDEBUG_PARAM;

typedef struct
{
    pthread_mutex_t  accesslock;            /* 互斥保护主要是用于程序异常退出而没有释放锁的情况 */
    INT32            semid;

    /* 模块ID，主要使用与多个库之间命令识别 */
    INT32            currentUserID;

    INT32           cmdId;
    INT32           inArg0;
    INT32           inArg1;
    INT32           inArg2;
    INT32           inArg3;
    INT32           inArg4;
    INT32           inArg5;
    INT32           inArg6;
    CHAR            inPara[MAX_PARAM_LENGTH];       /* 结构体类型的参数传递输出参数 */

    INT32           outCmdid;                       /* 表示命令的完成情况 */
    INT32           outArg0;                        /* 设置最大支持8个输出参数 */
    INT32           outArg1;
    INT32           outArg2;
    INT32           outArg3;
    INT32           outArg4;
    INT32           outArg5;
    INT32           outArg6;
    INT32           outArg7;
    INT8            outPara[MAX_PARAM_LENGTH];      /* 结构体类型的参数传递 */
} DSP_DEBUG_CMD_ATTR;

/* 进程通讯结构体 */
typedef struct
{
    DSP_DEBUG_CMD_ATTR       cmd;                /* DSP模块的调试命令 */
}DSP_DEBUG_CMD;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

