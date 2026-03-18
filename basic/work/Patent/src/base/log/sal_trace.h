/***
 * @file   sal_trace.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  替换系统的打印函数，增加终端打印颜色，加上模块名称，进行统一的调
           试打印接口，内核驱动和系统程序都可以使用。如果模块名称涉及到保密
           命名可以置空格或者代号打印。 修改Makefile宏SAL_MODULE_NAME = XXX
           的 XXX 为需要的名称即可。
 * #!/bin/sh
            for attr in 0 1 4 5 7 8; do
                    echo "ESC[$attr;Foreground;Background:"
                    for fore in 30 31 32 33 34 35 36 37; do
                            for back in 40 41 42 43 44 45 46 47; do
                                    printf '\033[%s;%s;%sm' $attr $fore $back
                                    printf '\\033[%s;%02s;%02sm'  $attr $fore $back         # 这行的直观打印可以直接用在程序作为颜色码
                                    printf '\033[m'
                                    printf ' '
                            done
                            echo
                    done
                    echo
                    echo
            done
 * @author rayin_dsp
 * @date   2022-02-24
 * @note
 * @note History:
 *      2014-04-21 ------ 添加对终端颜色打印的详细说明。
        终端显示打印支持 6种显示方式，8种前景色，8种背景色；共支持 6*8*8 = 384 种色彩组合显示
        6种显示方式:   0(关闭属性) 1(高亮) 4(下划线) 5(闪烁) 7(反显) 8(消隐)
        8种前景色:     30 (黑色) 31(红色) 32(绿色) 33(黄色) 34(蓝色) 35(紫色) 36(深绿) 37(白色)
        8种前景色:     40 (黑色) 41(红色) 42(绿色) 43(黄色) 44(蓝色) 45(紫色) 46(深绿) 47(白色)
        格式设置为ESC[attr;Foreground;Background，这里提取了几种常见的颜色配置。
        运行下面的脚本文件，会得到384种直观的颜色。

        修改该模块，去除不常用的颜色，只保留常用三种颜色，正常信息(绿)、警告信息(黄)以及错误信息(红)
        以及增加共享内存日志打印功能 --by wangweiqin

        2020.07.14 --封装新的接口SAL_LOGE/SAL_LOGW/SAL_LOGI/SAL_LOGD，基于这些接口各个模块可单独
        控制输出哪些等级的日志，各个模块可再定义一层宏定义，可参考DISP_LOGE；--yeyanzhong
 */

#ifndef _SAL_TRACE_H_
#define _SAL_TRACE_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <stdarg.h>
#include "sal_type.h"
#include "log_client.h"

/* 不同平台打印函数不一样是修改或者增加这里 */
#ifdef _ANDROID_
/* Android 日志头文件  */
#include <android/log.h>
#include <utils/Log.h>
#define SAL_print __android_log_print
#else
/* 通用日志打印头文件 */
#include <stdio.h>
#define SAL_print printf
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
/* 定义终端支持打印字体显示的颜色，格式控制: ESC[ 显示方式m; 前景色m; 背景色m; */
#define NONE	"\033[m"
#define RED		"\033[0;31m"            /* 红色 */
#define GREEN	"\033[0;32m"            /* 绿色 */
#define BROWN	"\033[0;33m"            /* 黄色 */
#define BLUE	"\033[0;34m"            /* 蓝色 */
#define PURPLE	"\033[0;35m"            /* 紫色 */
#define CYAN	"\033[0;36m"            /* 深绿 */
#define GRAY	"\033[1;30m"            /* 灰色，即高亮黑色 */
#define YELLOW	"\033[1;33m"            /* 高亮黄色 */
#define WHITE	"\033[1;37m"            /* 高亮白色 */

/*
   模块的名字必须在Makefile或其他头文件中定义，名字不需要加上[]。
   在Makefile中添加的方法是：
   SAL_MODULE_NAME = XXX
   CFLAGS       += -DSAL_MODULE_NAME='"$(SAL_MODULE_NAME)"'
   EXTRA_CFLAGS += -D'SAL_MODULE_NAME=\"$(SAL_MODULE_NAME)\"'
   应用程序中使用CFLAGS，驱动程序中使用EXTRA_CFLAGS。
 */

#define SAL_MODULE_NAME "DSP"  /* 打印在终端上用于显示标识的模块名 */

#ifndef SAL_MODULE_NAME
#error ##########################################################
#error Not define SAL_MODULE_NAME at Makefile !!!
#error ##########################################################
#endif

#ifndef SAL_COMPILE_USER
#define SAL_COMPILE_USER "unknown"
#endif

#ifndef LOG_TAG
#define LOG_TAG SAL_MODULE_NAME
#endif

/* 临时特殊定义*/
#ifdef PCIE_M2S_DEBUG
#define SZL_DBG_LOG SAL_WARN
#else
#define SZL_DBG_LOG SAL_LOGD
#endif

#ifdef PCIE_S2M_DEBUG
#define SZL_DBG2_LOG SAL_WARN
#else
#define SZL_DBG2_LOG SAL_LOGD
#endif

/* 打印等级定义*/
typedef enum
{
    DEBUG_LEVEL0 = 0,
    DEBUG_LEVEL1,
    DEBUG_LEVEL2,
    DEBUG_LEVEL3,
    DEBUG_LEVEL4,
    DEBUG_LEVEL5,
    DEBUG_LEVEL_BUll
} DEBUG_LEVEL_E;

/* 6个打印等级*/
#define DEBUG_LEVEL_SILENT	DEBUG_LEVEL0
#define DEBUG_LEVEL_ERROR	DEBUG_LEVEL1
#define DEBUG_LEVEL_WARNING DEBUG_LEVEL2
#define DEBUG_LEVEL_INFO	DEBUG_LEVEL3
#define DEBUG_LEVEL_DEBUG	DEBUG_LEVEL4
#define DEBUG_LEVEL_TRACE	DEBUG_LEVEL5
#define DEBUG_LEVEL_BUTT	DEBUG_LEVEL_TRACE

/**
 * 打印统一宏定义，不建议使用下面的宏，采用后面模块打印方式来输出日志(方便dspdebug控制)
 * 除了tools目录和demo目录，禁止直接使用printf打印，在一些状态获取中也需使用SAL_print
 * BSP_LOG_TRACE宏表示是否启动bsp_log，可通过Makeife传入，传入方法是-DBSP_LOG_TRACE
 */
#ifndef BSP_LOG_TRACE
#define SAL_DEBUG(fmt, args...) SAL_print("[" SAL_MODULE_NAME "] " fmt,##args)

/* 出错打印，输出字体为红色，打印出文件名、函数名和行号,表明程序不能继续运行。 */
#define SAL_ERROR(fmt, args...) \
    do \
    { \
        SAL_print(RED "[" SAL_MODULE_NAME "] " "ERROR|%s|%s|%d: " \
                  fmt NONE, __FILE__, __func__, __LINE__,##args); \
    } while (0)

/*
 * 警告打印，输出字体为黄色，打印出文件名、函数名和行号。表明程序仍可继续运行，但须警示。
 */
#define SAL_WARN(fmt, args...) \
    do \
    { \
        SAL_print(YELLOW "[" SAL_MODULE_NAME "] " "WARN|%s|%s|%d: "  \
                  fmt NONE, __FILE__, __func__, __LINE__,##args); \
    } while (0)

/* 信息通告打印，输出字体为绿色。 */
#define SAL_INFO(fmt, args...) \
    do \
    { \
        SAL_print(GREEN "[" SAL_MODULE_NAME "] " "INFO|%s|%s|%d: "  \
                  fmt NONE, __FILE__, __func__, __LINE__,##args); \
    } while (0)

#else
#define SAL_DEBUG(fmt, args...)

/* 出错打印，输出字体为红色，打印出文件名、函数名和行号,表明程序不能继续运行。 */
#define SAL_ERROR(fmt, args...) \
    do \
    { \
        log_msg_write(LOG_MODULE_APP_NAME, LOG_LEVEL_PRINT_SAVE, RED "[" SAL_MODULE_NAME "] " "ERROR|%s|%d: "  \
                      fmt NONE, __func__, __LINE__,##args); \
    } while (0)

/*
 * 警告打印，输出字体为黄色，打印出文件名、函数名和行号。表明程序仍可继续运行，但须警示。
 */
#define SAL_WARN(fmt, args...) \
    do \
    { \
        log_msg_write(LOG_MODULE_APP_NAME, LOG_LEVEL_PRINT_SAVE, YELLOW "[" SAL_MODULE_NAME "] " "WARN|%s|%d: "  \
                      fmt NONE, __func__, __LINE__,##args); \
    } while (0)

/* 信息通告打印，输出字体为绿色。 */
#define SAL_INFO(fmt, args...) \
    do \
    { \
        log_msg_write(LOG_MODULE_APP_NAME, LOG_LEVEL_PRINT_SAVE, GREEN "[" SAL_MODULE_NAME "] " "INFO|%s|%d: "  \
                      fmt NONE, __func__, __LINE__,##args); \
    } while (0)
#endif


/* BSP_LOG_TRACE宏表示是否启动bsp_log，可通过Makeife传入，传入方法是-DBSP_LOG_TRACE。 */

/*
    当前设置模块默认日志打印等级为info，默认info、debug、trace等级写文件，warning、error、silent按照模块的日志保存方式处理
    add by sunzelin@2022/9/6
 */
#ifdef BSP_LOG_TRACE
#define SAL_modDebug(mod_id, level, fmt, args...) \
    do { \
        if (SAL_getModLogLevel(mod_id) < level) { break; } \
        log_msg_write(LOG_MODULE_APP_NAME, SAL_getLogSaveMode(mod_id), fmt NONE,##args); \
    } while (0)

#else
#define SAL_modDebug(mod_id, level, fmt, args...) \
    do { \
        if (SAL_getModLogLevel(mod_id) < level) { break; } \
        else \
        { \
            SAL_print(fmt NONE,##args); \
        } \
    } while (0)
#endif

/*======================================模块打印=======================================*/

/** @enum	MOD_INDEX
 *  @brief	所有模块的索引值
 */
typedef enum
{
    /* 媒体类-通用 */
    MOD_MEDIA_SYS = 0,
    MOD_MEDIA_MEM,
    MOD_MEDIA_SAL,
    MOD_MEDIA_PLAT, /* 平台层打印，用于不归属于功能的模块打印*/

    /* 媒体类-输入输出 */
    MOD_MEDIA_EDID,
    MOD_MEDIA_ADV,
    MOD_MEDIA_MSTAR,
    MOD_MEDIA_CH7053,
    MOD_MEDIA_CAPT,
    MOD_MEDIA_DISP,

    /* 媒体类-数据处理 */
    MOD_MEDIA_FPGA,
    MOD_MEDIA_SVP,
    MOD_MEDIA_DUP,
    MOD_MEDIA_VENC,
    MOD_MEDIA_JPEG,
    MOD_MEDIA_DEC,
    MOD_MEDIA_OSD,
    MOD_MEDIA_AUD,
    MOD_MEDIA_MUX,
    MOD_MEDIA_DEMUX,
    MOD_MEDIA_RECODE,
    MOD_MEDIA_XPACK,
    MOD_MEDIA_XRAY,
    MOD_MEDIA_VCA,

    /* 媒体类-数据流 */
    MOD_MEDIA_LINK,

    /* AI类 */
    MOD_AI_XSP, /* X-RAY 图像处理 */
    MOD_AI_SVA, /* X-RAY智能模块 */
    MOD_AI_BA,  /* 行为分析 */
    MOD_AI_MD,  /* 移动侦测 */
    MOD_AI_PPM, /* 人包智能模块 */
    MOD_AI_FACE, /* 人脸智能模块 */

    /* 外设类 */
    MOD_DEV_HARDWARE,
    MOD_DEV_IIC,
    MOD_DEV_UART,
    MOD_DEV_MCU,

    /* MISC */
    MOD_MISC_ICONV,
    MOD_MEDIA_FREETYPE,

    MOD_MAX_NUMS
} MOD_ID;

#define SAL_MOD_LOGE(MOD_ID, fmt, args...)	SAL_modDebug(MOD_ID, DEBUG_LEVEL_ERROR, RED "[" SAL_MODULE_NAME "] " "ERROR|%s|%s|%d: " fmt NONE, __FILE__, __func__, __LINE__,##args)
#define SAL_MOD_LOGW(MOD_ID, fmt, args...)	SAL_modDebug(MOD_ID, DEBUG_LEVEL_WARNING, YELLOW "[" SAL_MODULE_NAME "] " "WARNING|%s|%s|%d: " fmt NONE, __FILE__, __func__, __LINE__,##args)
#define SAL_MOD_LOGI(MOD_ID, fmt, args...)	SAL_modDebug(MOD_ID, DEBUG_LEVEL_INFO, GREEN "[" SAL_MODULE_NAME "] " "INFO|%s|%s|%d: " fmt NONE, __FILE__, __func__, __LINE__,##args)
#define SAL_MOD_LOGD(MOD_ID, fmt, args...)	SAL_modDebug(MOD_ID, DEBUG_LEVEL_DEBUG, "[" SAL_MODULE_NAME "] " "DEBUG|%s|%s|%d: " fmt, __FILE__, __func__, __LINE__,##args)
#define SAL_MOD_LOGT(MOD_ID, fmt, args...)	SAL_modDebug(MOD_ID, DEBUG_LEVEL_TRACE, "[" SAL_MODULE_NAME "] " "DEBUG|%s|%s|%d: " fmt, __FILE__, __func__, __LINE__,##args)

/* media module 的调试接口*/
#define SYS_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_SYS, fmt,##args)
#define SYS_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_SYS, fmt,##args)
#define SYS_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_SYS, fmt,##args)
#define SYS_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_SYS, fmt,##args)
#define SYS_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_SYS, fmt,##args)

#define MEM_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_MEM, fmt,##args)
#define MEM_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_MEM, fmt,##args)
#define MEM_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_MEM, fmt,##args)
#define MEM_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_MEM, fmt,##args)
#define MEM_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_MEM, fmt,##args)

#define SAL_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_SAL, fmt,##args)
#define SAL_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_SAL, fmt,##args)
#define SAL_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_SAL, fmt,##args)
#define SAL_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_SAL, fmt,##args)
#define SAL_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_SAL, fmt,##args)

#define PLAT_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_PLAT, fmt,##args)
#define PLAT_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_PLAT, fmt,##args)
#define PLAT_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_PLAT, fmt,##args)
#define PLAT_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_PLAT, fmt,##args)
#define PLAT_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_PLAT, fmt,##args)

#define EDID_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_EDID, fmt,##args)
#define EDID_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_EDID, fmt,##args)
#define EDID_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_EDID, fmt,##args)
#define EDID_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_EDID, fmt,##args)
#define EDID_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_EDID, fmt,##args)

#define ADV_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_ADV, fmt,##args)
#define ADV_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_ADV, fmt,##args)
#define ADV_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_ADV, fmt,##args)
#define ADV_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_ADV, fmt,##args)
#define ADV_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_ADV, fmt,##args)

#define MSTAR_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_MSTAR, fmt,##args)
#define MSTAR_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_MSTAR, fmt,##args)
#define MSTAR_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_MSTAR, fmt,##args)
#define MSTAR_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_MSTAR, fmt,##args)
#define MSTAR_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_MSTAR, fmt,##args)

#define CAPT_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_CAPT, fmt,##args)
#define CAPT_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_CAPT, fmt,##args)
#define CAPT_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_CAPT, fmt,##args)
#define CAPT_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_CAPT, fmt,##args)
#define CAPT_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_CAPT, fmt,##args)

#define MCU_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_DEV_MCU, fmt,##args)
#define MCU_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_DEV_MCU, fmt,##args)
#define MCU_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_DEV_MCU, fmt,##args)
#define MCU_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_DEV_MCU, fmt,##args)
#define MCU_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_DEV_MCU, fmt,##args)

#define FPGA_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_FPGA, fmt,##args)
#define FPGA_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_FPGA, fmt,##args)
#define FPGA_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_FPGA, fmt,##args)
#define FPGA_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_FPGA, fmt,##args)
#define FPGA_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_FPGA, fmt,##args)

#define SVP_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_SVP, fmt,##args)
#define SVP_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_SVP, fmt,##args)
#define SVP_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_SVP, fmt,##args)
#define SVP_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_SVP, fmt,##args)
#define SVP_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_SVP, fmt,##args)

#define DISP_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_DISP, fmt,##args)
#define DISP_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_DISP, fmt,##args)
#define DISP_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_DISP, fmt,##args)
#define DISP_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_DISP, fmt,##args)
#define DISP_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_DISP, fmt,##args)

#define DUP_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_DUP, fmt,##args)
#define DUP_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_DUP, fmt,##args)
#define DUP_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_DUP, fmt,##args)
#define DUP_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_DUP, fmt,##args)
#define DUP_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_DUP, fmt,##args)

#define VCA_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_VCA, fmt,##args)
#define VCA_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_VCA, fmt,##args)
#define VCA_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_VCA, fmt,##args)
#define VCA_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_VCA, fmt,##args)
#define VCA_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_VCA, fmt,##args)

#define XPACK_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_XPACK, fmt,##args)
#define XPACK_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_XPACK, fmt,##args)
#define XPACK_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_XPACK, fmt,##args)
#define XPACK_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_XPACK, fmt,##args)
#define XPACK_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_XPACK, fmt,##args)

#define VENC_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_VENC, fmt,##args)
#define VENC_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_VENC, fmt,##args)
#define VENC_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_VENC, fmt,##args)
#define VENC_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_VENC, fmt,##args)
#define VENC_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_VENC, fmt,##args)

#define VDEC_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_DEC, fmt,##args)
#define VDEC_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_DEC, fmt,##args)
#define VDEC_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_DEC, fmt,##args)
#define VDEC_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_DEC, fmt,##args)
#define VDEC_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_DEC, fmt,##args)

#define RECODE_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_RECODE, fmt,##args)
#define RECODE_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_RECODE, fmt,##args)
#define RECODE_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_RECODE, fmt,##args)
#define RECODE_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_RECODE, fmt,##args)
#define RECODE_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_RECODE, fmt,##args)

#define HARDWARE_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_DEV_HARDWARE, fmt,##args)
#define HARDWARE_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_DEV_HARDWARE, fmt,##args)
#define HARDWARE_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_DEV_HARDWARE, fmt,##args)
#define HARDWARE_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_DEV_HARDWARE, fmt,##args)
#define HARDWARE_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_DEV_HARDWARE, fmt,##args)

#define IIC_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_DEV_IIC, fmt,##args)
#define IIC_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_DEV_IIC, fmt,##args)
#define IIC_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_DEV_IIC, fmt,##args)
#define IIC_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_DEV_IIC, fmt,##args)
#define IIC_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_DEV_IIC, fmt,##args)

#define UART_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_DEV_UART, fmt,##args)
#define UART_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_DEV_UART, fmt,##args)
#define UART_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_DEV_UART, fmt,##args)
#define UART_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_DEV_UART, fmt,##args)
#define UART_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_DEV_UART, fmt,##args)

#define FREETYPE_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_FREETYPE, fmt,##args)
#define FREETYPE_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_FREETYPE, fmt,##args)
#define FREETYPE_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_FREETYPE, fmt,##args)
#define FREETYPE_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_FREETYPE, fmt,##args)
#define FREETYPE_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_FREETYPE, fmt,##args)

#define ICONV_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MISC_ICONV, fmt,##args)
#define ICONV_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MISC_ICONV, fmt,##args)
#define ICONV_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MISC_ICONV, fmt,##args)
#define ICONV_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MISC_ICONV, fmt,##args)
#define ICONV_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MISC_ICONV, fmt,##args)

#define OSD_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_OSD, fmt,##args)
#define OSD_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_OSD, fmt,##args)
#define OSD_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_OSD, fmt,##args)
#define OSD_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_OSD, fmt,##args)
#define OSD_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_OSD, fmt,##args)

#define AUD_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_MEDIA_AUD, fmt,##args)
#define AUD_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_MEDIA_AUD, fmt,##args)
#define AUD_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_MEDIA_AUD, fmt,##args)
#define AUD_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_MEDIA_AUD, fmt,##args)
#define AUD_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_MEDIA_AUD, fmt,##args)

#define LINK_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_LINK, fmt,##args)
#define LINK_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_LINK, fmt,##args)
#define LINK_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_LINK, fmt,##args)
#define LINK_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_LINK, fmt,##args)
#define LINK_LOGT(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_LINK, fmt,##args)

#define JPEG_LOGE(fmt, args...) SAL_MOD_LOGE(MOD_MEDIA_JPEG, fmt,##args)
#define JPEG_LOGW(fmt, args...) SAL_MOD_LOGW(MOD_MEDIA_JPEG, fmt,##args)
#define JPEG_LOGI(fmt, args...) SAL_MOD_LOGI(MOD_MEDIA_JPEG, fmt,##args)
#define JPEG_LOGD(fmt, args...) SAL_MOD_LOGD(MOD_MEDIA_JPEG, fmt,##args)
#define JPEG_LOGT(fmt, args...) SAL_MOD_LOGT(MOD_MEDIA_JPEG, fmt,##args)

/* AI module 的调试接口*/
#define SVA_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_AI_SVA, fmt,##args)
#define SVA_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_AI_SVA, fmt,##args)
#define SVA_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_AI_SVA, fmt,##args)
#define SVA_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_AI_SVA, fmt,##args)
#define SVA_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_AI_SVA, fmt,##args)

#define BA_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_AI_BA, fmt,##args)
#define BA_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_AI_BA, fmt,##args)
#define BA_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_AI_BA, fmt,##args)
#define BA_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_AI_BA, fmt,##args)
#define BA_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_AI_BA, fmt,##args)

#define XSP_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_AI_XSP, fmt,##args)
#define XSP_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_AI_XSP, fmt,##args)
#define XSP_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_AI_XSP, fmt,##args)
#define XSP_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_AI_XSP, fmt,##args)
#define XSP_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_AI_XSP, fmt,##args)

#define PPM_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_AI_PPM, fmt,##args)
#define PPM_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_AI_PPM, fmt,##args)
#define PPM_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_AI_PPM, fmt,##args)
#define PPM_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_AI_PPM, fmt,##args)
#define PPM_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_AI_PPM, fmt,##args)

#define FACE_LOGE(fmt, args...)	SAL_MOD_LOGE(MOD_AI_FACE, fmt,##args)
#define FACE_LOGW(fmt, args...)	SAL_MOD_LOGW(MOD_AI_FACE, fmt,##args)
#define FACE_LOGI(fmt, args...)	SAL_MOD_LOGI(MOD_AI_FACE, fmt,##args)
#define FACE_LOGD(fmt, args...)	SAL_MOD_LOGD(MOD_AI_FACE, fmt,##args)
#define FACE_LOGT(fmt, args...)	SAL_MOD_LOGT(MOD_AI_FACE, fmt,##args)

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/***
 * @description 日志输出接口函数，可控制各个模块不同的日志等级的输出
 * @param [UINT32] debugflags 模块的当前日志等级，
 * @param [UINT32] level      level 为调用的地方的日志等级，包括ERROR/WARNING/INFO/DEBUG
 * @param [char] *fmt         打印信息格式
 * @return [*]
 */
void SAL_dbPrintf(UINT32 debugflags, UINT32 level, const char *fmt, ...);

/***
 * @brief  设置模块的日志等级
 * @param  [UINT32] mod_id          模块ID
 * @param  [UINT32] u32ModLogLevel  日志等级
 * @return [*]
 */
void SAL_setModLogLevel(UINT32 mod_id, UINT32 u32ModLogLevel);

/***
 * @description 设置模块日志保存模式
 * @param [UINT32] mod_id            模块ID
 * @param [LogLevel] u32LogSaveMode  模式
 * @return [*]
 */
void SAL_setModLogSaveMode(UINT32 mod_id, LogLevel u32LogSaveMode);

/***
 * @description 获取模块的日志打印等级
 * @param [UINT32] mod_id 模块ID
 * @return [*]
 */
UINT32 SAL_getModLogLevel(UINT32 mod_id);

/***
 * @description 获取模块日志保存模式
 * @param [UINT32] mod_id 模块ID
 * @return [*]
 */
UINT32 SAL_getLogSaveMode(UINT32 mod_id);


#endif /* _SAL_TRACE_H_ */

