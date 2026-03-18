/***
 * @file   sal_log.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  日志模块，显示、dup、视频接入等各个模块可单独控制是否输出调试信息
 * @author yeyanzhong
 * @date   2022-03-02
 * @note
 * @note History:
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sal_type.h"
#include "sal_trace.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define MAX_MOD_NAME 16 /* 模块名的最大长度 */

#ifdef _DBG_MODE_ /*< 编译Debug版本，该宏定义在Makefile中定义 */
#define DEFAULT_SAVE_MODE	LOG_LEVEL_PRINT_SAVE
#define DEFAULT_DEBUG_LEVEL DEBUG_LEVEL_INFO
#else /* BUILD_TYPE_RELEASE */
#define DEFAULT_SAVE_MODE	LOG_LEVEL_SAVE_ONLY
#define DEFAULT_DEBUG_LEVEL DEBUG_LEVEL_INFO
#endif

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/** @struct	MOD_ATTRIBUTE
 *  @brief	模块属性结构体
 */
typedef struct _SAL_LOG_MOD_ATTR_
{
    UINT32 mod_idx;                /* 模块索引号 */
    INT8 mod_name[MAX_MOD_NAME];   /* 模块名 */
    UINT32 level;                  /* 调试等级 */
} SAL_LOG_MOD_ATTR;

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

/** @struct	SAL_LOG_MOD_ATTR
 *  @brief	各个模块的初始化默认日志等级；增加一个新的模块时，
 *  这里需要增加该模块的默认日志等级; 该结构体内模块排列顺序最后与模块枚举值顺序一致；
 */
static SAL_LOG_MOD_ATTR g_stLogModAttr[MOD_MAX_NUMS] =
{
	// 媒体类
	[MOD_MEDIA_SYS]         = {MOD_MEDIA_SYS,       "SYS",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_MEM]         = {MOD_MEDIA_MEM,       "MEM",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_SAL]         = {MOD_MEDIA_SAL,       "SAL",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_PLAT]        = {MOD_MEDIA_PLAT,      "PLAT",     DEFAULT_DEBUG_LEVEL},
    
	[MOD_MEDIA_EDID]        = {MOD_MEDIA_EDID,      "EDID",     DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_ADV]         = {MOD_MEDIA_ADV,       "ADV",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_MSTAR]       = {MOD_MEDIA_MSTAR,     "MSTAR",    DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_CH7053]      = {MOD_MEDIA_CH7053,    "CH7053",   DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_CAPT]        = {MOD_MEDIA_CAPT,      "CAPT",     DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_FPGA]        = {MOD_MEDIA_FPGA,      "FPGA",     DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_SVP]         = {MOD_MEDIA_SVP,       "SVP",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_DUP]         = {MOD_MEDIA_DUP,       "DUP",      DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_VENC]        = {MOD_MEDIA_VENC,      "VENC",     DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_JPEG]        = {MOD_MEDIA_JPEG,      "JPEG",     DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_DEC]         = {MOD_MEDIA_DEC,       "DEC",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_DISP]        = {MOD_MEDIA_DISP,      "DISP",     DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_OSD]         = {MOD_MEDIA_OSD,       "OSD",      DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_AUD]         = {MOD_MEDIA_AUD,       "AUD",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_MUX]         = {MOD_MEDIA_MUX,       "MUX",      DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_DEMUX]       = {MOD_MEDIA_DEMUX,     "DEMUX",    DEFAULT_DEBUG_LEVEL},
	[MOD_MEDIA_RECODE]      = {MOD_MEDIA_RECODE,    "RECODE",   DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_XPACK]       = {MOD_MEDIA_XPACK,     "XPACK",    DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_XRAY]        = {MOD_MEDIA_XRAY,      "XRAY",     DEFAULT_DEBUG_LEVEL},    
    [MOD_MEDIA_VCA]         = {MOD_MEDIA_VCA,       "VCA",      DEFAULT_DEBUG_LEVEL},
    [MOD_MEDIA_LINK]        = {MOD_MEDIA_LINK,      "LINK",     DEFAULT_DEBUG_LEVEL},

    // AI 类
    [MOD_AI_XSP]            = {MOD_AI_XSP,          "XSP",      DEFAULT_DEBUG_LEVEL},
	[MOD_AI_SVA]            = {MOD_AI_SVA,          "SVA",      DEFAULT_DEBUG_LEVEL},
	[MOD_AI_BA]             = {MOD_AI_BA,           "BA",       DEFAULT_DEBUG_LEVEL},
    [MOD_AI_MD]             = {MOD_AI_MD,           "MD",       DEFAULT_DEBUG_LEVEL},
	[MOD_AI_PPM]	        = {MOD_AI_PPM,		    "PPM",		DEFAULT_DEBUG_LEVEL},
	[MOD_AI_FACE]			= {MOD_AI_FACE,			"FACE",		DEFAULT_DEBUG_LEVEL},

    // 外设类
    [MOD_DEV_HARDWARE]      = {MOD_DEV_HARDWARE,    "HARDWARE", DEFAULT_DEBUG_LEVEL},
    [MOD_DEV_IIC]           = {MOD_DEV_IIC,         "IIC",      DEFAULT_DEBUG_LEVEL},
    [MOD_DEV_UART]          = {MOD_DEV_UART,        "UART",     DEFAULT_DEBUG_LEVEL},
    [MOD_DEV_MCU]           = {MOD_DEV_MCU,         "MCU",      DEFAULT_DEBUG_LEVEL},

    // MISC
    [MOD_MISC_ICONV]        = {MOD_MISC_ICONV,      "ICONV",    DEFAULT_DEBUG_LEVEL},    
    [MOD_MEDIA_FREETYPE]    = {MOD_MEDIA_FREETYPE,  "FREETYPE", DEFAULT_DEBUG_LEVEL},
};

/** @struct	SAL_LOG_MOD_ATTR
 *  @brief	各个模块的初始化默认日志保存方式；增加一个新的模块时，
 *  这里需要增加该模块的默认日志保存方式; 该结构体内模块排列顺序最后与模块枚举值顺序一致；
 */
static SAL_LOG_MOD_ATTR g_stLogSaveMode[MOD_MAX_NUMS] =
{
	// 媒体类
	[MOD_MEDIA_SYS]    = {MOD_MEDIA_SYS,    "SYS",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_MEM]    = {MOD_MEDIA_MEM,    "MEM",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_SAL]    = {MOD_MEDIA_SAL,    "SAL",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_PLAT]   = {MOD_MEDIA_PLAT,   "PLAT",     DEFAULT_SAVE_MODE},
	
	[MOD_MEDIA_EDID]   = {MOD_MEDIA_EDID,   "EDID",     DEFAULT_SAVE_MODE},
	[MOD_MEDIA_ADV]    = {MOD_MEDIA_ADV,    "ADV",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_MSTAR]  = {MOD_MEDIA_MSTAR,  "MSTAR",    DEFAULT_SAVE_MODE},
    [MOD_MEDIA_CH7053] = {MOD_MEDIA_CH7053, "CH7053",   DEFAULT_SAVE_MODE},
	[MOD_MEDIA_CAPT]   = {MOD_MEDIA_CAPT,   "CAPT",     DEFAULT_SAVE_MODE},
	[MOD_MEDIA_FPGA]   = {MOD_MEDIA_FPGA,   "FPGA",     DEFAULT_SAVE_MODE},
	[MOD_MEDIA_SVP]    = {MOD_MEDIA_SVP,    "SVP",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_DUP]    = {MOD_MEDIA_DUP,    "DUP",      DEFAULT_SAVE_MODE},
    [MOD_MEDIA_VENC]   = {MOD_MEDIA_VENC,   "VENC",     DEFAULT_SAVE_MODE},
	[MOD_MEDIA_JPEG]   = {MOD_MEDIA_JPEG,   "JPEG",     DEFAULT_SAVE_MODE},
	[MOD_MEDIA_DEC]    = {MOD_MEDIA_DEC,    "DEC",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_DISP]   = {MOD_MEDIA_DISP,   "DISP",     DEFAULT_SAVE_MODE},
    [MOD_MEDIA_OSD]    = {MOD_MEDIA_OSD,    "OSD",      DEFAULT_SAVE_MODE},
    [MOD_MEDIA_AUD]    = {MOD_MEDIA_AUD,    "AUD",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_MUX]    = {MOD_MEDIA_MUX,    "MUX",      DEFAULT_SAVE_MODE},
	[MOD_MEDIA_DEMUX]  = {MOD_MEDIA_DEMUX,  "DEMUX",    DEFAULT_SAVE_MODE},
	[MOD_MEDIA_RECODE] = {MOD_MEDIA_RECODE, "RECODE",   DEFAULT_SAVE_MODE},
    [MOD_MEDIA_XPACK]  = {MOD_MEDIA_XPACK,  "XPACK",    DEFAULT_SAVE_MODE},
    [MOD_MEDIA_XRAY]   = {MOD_MEDIA_XRAY,   "XRAY",     DEFAULT_SAVE_MODE},
    [MOD_MEDIA_VCA]    = {MOD_MEDIA_VCA,    "VCA",      DEFAULT_SAVE_MODE},
    [MOD_MEDIA_LINK]   = {MOD_MEDIA_LINK,   "LINK",     DEFAULT_SAVE_MODE},

    // AI 类
    [MOD_AI_XSP]       = {MOD_AI_XSP,       "XSP",      DEFAULT_SAVE_MODE},
	[MOD_AI_SVA]       = {MOD_AI_SVA,       "SVA",      DEFAULT_SAVE_MODE},
	[MOD_AI_BA]        = {MOD_AI_BA,        "BA",       DEFAULT_SAVE_MODE},
    [MOD_AI_MD]        = {MOD_AI_MD,        "MD",       DEFAULT_SAVE_MODE},
	[MOD_AI_PPM] 	   = {MOD_AI_PPM,		"PPM",		DEFAULT_SAVE_MODE},
	[MOD_AI_FACE]	   = {MOD_AI_FACE, 		"FACE", 	DEFAULT_SAVE_MODE},

    // 外设类
    [MOD_DEV_HARDWARE] = {MOD_DEV_HARDWARE, "HARDWARE", DEFAULT_SAVE_MODE},
    [MOD_DEV_IIC]      = {MOD_DEV_IIC,      "IIC",      DEFAULT_SAVE_MODE},
    [MOD_DEV_UART]     = {MOD_DEV_UART,     "UART",     DEFAULT_SAVE_MODE},
    [MOD_DEV_MCU]      = {MOD_DEV_MCU,      "MCU",      DEFAULT_SAVE_MODE},

    // MISC
    [MOD_MISC_ICONV]   = {MOD_MISC_ICONV,   "ICONV",    DEFAULT_SAVE_MODE},
    [MOD_MEDIA_FREETYPE] = {MOD_MEDIA_FREETYPE,  "FREETYPE", DEFAULT_SAVE_MODE},
};


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
void SAL_dbPrintf(UINT32 debugflags, UINT32 level, const char *fmt, ...)
{
    if (debugflags < level)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    return;
}

/***
 * @brief  设置模块的日志等级
 * @param  [UINT32] mod_id          模块ID
 * @param  [UINT32] u32ModLogLevel  日志等级
 * @return [*]
 */
void SAL_setModLogLevel(UINT32 mod_id, UINT32 u32ModLogLevel)
{
    if (mod_id >= MOD_MAX_NUMS)
    {
        SAL_ERROR(" mod_id %d exceed the MAX %d \n", mod_id, MOD_MAX_NUMS);
        return;
    }

    if (u32ModLogLevel > DEBUG_LEVEL_BUTT)
    {
        SAL_ERROR(" u32ModLogLevel %d exceed the MAX %d \n", mod_id, DEBUG_LEVEL_BUTT);
        return;
    }

    g_stLogModAttr[mod_id].level = u32ModLogLevel;

    return;
}

/***
 * @description 获取模块的日志打印等级
 * @param [UINT32] mod_id 模块ID
 * @return [*]
 */
inline UINT32 SAL_getModLogLevel(UINT32 mod_id)
{
    if (mod_id >= MOD_MAX_NUMS)
    {
        SAL_ERROR(" mod_id %d exceed the MAX %d \n", mod_id, MOD_MAX_NUMS);
        return DEFAULT_DEBUG_LEVEL;
    }

    return g_stLogModAttr[mod_id].level;
}

/***
 * @description 设置模块日志保存模式
 * @param [UINT32] mod_id            模块ID
 * @param [LogLevel] u32LogSaveMode  模式
 * @return [*]
 */
void SAL_setModLogSaveMode(UINT32 mod_id, LogLevel u32LogSaveMode)
{
    if (mod_id >= MOD_MAX_NUMS)
    {
        SAL_ERROR(" mod_id %d exceed the MAX %d \n", mod_id, MOD_MAX_NUMS);
        return;
    }

    if (u32LogSaveMode >= LOG_LEVEL_RES)
    {
        SAL_ERROR(" u32LogSaveMode %d exceed the MAX %d \n", mod_id, LOG_LEVEL_RES);
        return;
    }

    g_stLogSaveMode[mod_id].level = u32LogSaveMode;

    return;
}

/***
 * @description 获取模块日志保存模式
 * @param [UINT32] mod_id 模块ID
 * @return [*]
 */
inline UINT32 SAL_getLogSaveMode(UINT32 mod_id)
{
    if (mod_id >= MOD_MAX_NUMS)
    {
        SAL_ERROR(" mod_id %d exceed the MAX %d \n", mod_id, MOD_MAX_NUMS);
        return DEFAULT_SAVE_MODE;
    }

    return g_stLogSaveMode[mod_id].level;
}

/***
 * @description 设置所有模块日志等级设置
 * @param [UINT32] level 日志等级
 * @return [*]
 */
void SAL_setAllModLogLevel(UINT32 level)
{
    UINT32 mod_num = 0;

    SAL_INFO("set all module log level:%d \n", level);
    for (mod_num = 0; mod_num < MOD_MAX_NUMS; mod_num++)
    {
        SAL_setModLogLevel(mod_num, level);
    }

    return;
}

/***
 * @description 获取所有模块日志保存方式设置
 * @param [LogLevel] mode 保存方式，见枚举LogLevel
 * @return [*]
 */
void SAL_setAllModLogSaveMode(LogLevel mode)
{
    UINT32 mod_num = 0;

    SAL_INFO("set all module log save mode:%d \n", mode);
    for (mod_num = 0; mod_num < MOD_MAX_NUMS; mod_num++)
    {
        SAL_setModLogSaveMode(mod_num, mode);
    }

    return;
}

