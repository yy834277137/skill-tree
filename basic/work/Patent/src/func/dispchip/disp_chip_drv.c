/*******************************************************************************
 * disp_chip.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangzhenya5
 * Version: V1.0.0  2021年06月12日 Create
 *
 * Description : 显示芯片对外接口
 * Modification: 
 *******************************************************************************/
#include "platform_hal.h"
#include "disp_chip_drv_api.h"
#include "../func_baseLib/mcu/mcu_drv.h"
#include "./ltchip/icn6211_drv.h"
#include "./ltchip/lt8612ex_drv.h"
#include "./ltchip/lt9211_drv.h"
#include "./ltchip/lt8619c_drv.h"
#include "./ltchip/ch7053_drv.h"
#include "./ltchip/lt8618sx_drv.h"
#include "sal.h"

#line __LINE__ "disp_chip_drv.c"



#define      DISP_CHIP_TOTAL_MAX               10
#define      DISP_CHIP_SINGLE_CHANNEL_MAX      5     /* 最大为5，要增加请修改宏            */

typedef enum
{
    DISP_CHIP_ICN6211  = 0,
    DISP_CHIP_CH7053   = 1,
    DISP_CHIP_LT8612EX = 2,
    DISP_CHIP_LT8619C  = 3,
    DISP_CHIP_LT9211   = 4,
    DISP_CHIP_FPGA     = 5,
    DISP_CHIP_LT8618SX = 6,
    DISP_CHIP_TYPE_BUTT= 0xff,
}DISP_CHIP_TYPE_E;


/* 因为RS20016单板有两块ch7053和FPGA芯片，所以需要通道号来作区分, */
/* 其他型号单板暂时不需要用到 */
typedef enum
{
    DISP_CHIP_CHANNEL_0 = 0,
    DISP_CHIP_CHANNEL_1 = 1,
    DISP_CHIP_CHANNEL_BUTT,
}DISP_CHIP_CHANNEL_E;


/**
* dp 配置集(set)
*/
typedef struct tagDispChannel
{
    DISP_VO_MODE_E     enVoMode;                        //通道,代表是HDMI或MIPI输出
    UINT8              u8ChipNum;                       //该输出通路显示芯片个数
}DISP_VO_CHANNEL_S;

typedef struct tagDispChip
{
    DISP_CHIP_TYPE_E    enDispChip;                     //芯片名称
    DISP_CHIP_CHANNEL_E enChannel;                      //因为RS20016单板有两块ch7053和FPGA芯片，所以需要通道号来作区分
    UINT8               u8BusIdx;                       //i2c总线号,mcu编号
    UINT8               u8ChipAddr;                     //芯片设备地址
}DISP_CHIP_INFO_S;


/**
* dp 配置集(set)
*/
typedef struct tagDispChipCfg
{
    DISP_VO_CHANNEL_S      stVoChannel;                                 //通道号
    DISP_CHIP_INFO_S       astChipInfo[DISP_CHIP_SINGLE_CHANNEL_MAX];   //对应通道下的几个芯片参数
}DISP_CHIP_CFG_S;


/* 单板和对应的显示芯片信息 */
typedef struct 
{
    UINT32                 u32BoardType;
    const DISP_CHIP_CFG_S *pstDispChipCfg;
} DISP_CHIP_BOARD_MATCH_S;



typedef struct {
    DISP_CHIP_TYPE_E    enDispChip;
    INT32(*SetResolution)(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn);
} DISP_CHIP_FUNC_S;

/* 关联分辨率和在用的时序 */
typedef struct tagDispTimingMap
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
    BT_TIMING_E enUserTiming;
} DISP_CHIP_MAP_S;


/* 分辨率和在用的时序对照表 自定义需要支持的时序，每新增一种分辨率时序都需要增加到该数组中 */
static const DISP_CHIP_MAP_S gastDispChipTimingMap[] = {

     {1920,  1080,   72,   TIMING_CVT_1920X1080P72_R2},
     {1920,  1080,   60,   TIMING_VESA_1920X1080P60},
     {1920,  1080,   50,   TIMING_CVT_1920X1080P50},
                          
     {1680,  1050,   60,   TIMING_CVT_1680X1050P60},
                          
     {1280,  1024,   85,   TIMING_VESA_1280X1024P85},
     {1280,  1024,   75,   TIMING_VESA_1280X1024P75},
     {1280,  1024,   72,   TIMING_GTF_1280X1024P72},
     {1280,  1024,   70,   TIMING_GTF_1280X1024P70},
     {1280,  1024,   60,   TIMING_VESA_1280X1024P60},
                          
     {1280,  800,    85,   TIMING_VESA_1280X800P85},
     {1280,  800,    75,   TIMING_CVT_1280X800P75},
     {1280,  800,    72,   TIMING_GTF_1280X800P72},
     {1280,  800,    60,   TIMING_CVT_1280X800P60},
                          
     {1280,  768,    85,   TIMING_VESA_1280X768P85},
     {1280,  768,    75,   TIMING_VESA_1280X768P75},
     {1280,  768,    72,   TIMING_GTF_1280X768P72},
     {1280,  768,    60,   TIMING_CVT_1280X768P60},
                          
     {1280,  720,    72,   TIMING_GTF_1280X720P72},
        
     /* RK芯片特殊，要求输出时序的水平参数:htotal、hactive、hbp、hsync、hfp 都4对齐做约束 
         VESA标准的1280X720P60,hfp为110没有4对齐,所以RK平台改用CVT标准的; 非环通项目转换芯片时序由
         芯片固定因此无法统一使用CVT时序，此处暂不修改*/
#if defined (RK3588) && defined (DSP_ISA)
     {1280,  720,    60,   TIMING_CVT_1280X720P60},
#else
     {1280,  720,    60,   TIMING_VESA_1280X720P60},
#endif
     {1280,  720,    50,   TIMING_CVT_1280X720P50},
                          
     {1024,  768,    85,   TIMING_VESA_1024X768P85},
     {1024,  768,    75,   TIMING_VESA_1024X768P75},
     {1024,  768,    70,   TIMING_VESA_1024X768P70},
     {1024,  768,    72,   TIMING_CVT_1024X768P72},
     {1024,  768,    60,   TIMING_VESA_1024X768P60},

};



/* RS20001单板, 只有MIPI输出通路上有显示芯片 */
const DISP_CHIP_CFG_S  RS20001V1_1BoardDispChipCfg[] = 
{
    [DISP_CHIP_CHANNEL_1] = 
    {
        .stVoChannel =
        {
            .enVoMode  = DISP_VO_MIPI,
            .u8ChipNum = 2,           //MIPI通路上显示芯片个数
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_ICN6211,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 11,
                .u8ChipAddr = 0x58,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 11,
                .u8ChipAddr = 0xEC,
            },
        },
    },
};




const DISP_CHIP_CFG_S  RS20007V1_0BoardDispChipCfg[] = 
{
    [DISP_CHIP_CHANNEL_0] = 
    {
        .stVoChannel =  
        {
            .enVoMode  = DISP_VO_HDMI,
            .u8ChipNum = 1,           //HDMI通路上显示芯片个数
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT8612EX,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 3,
                .u8ChipAddr = 0x78,
            },
        },
    },
    [DISP_CHIP_CHANNEL_1] = 
    {
        .stVoChannel =
        {
            .enVoMode  = DISP_VO_MIPI,
            .u8ChipNum = 2,           //MIPI通路上显示芯片个数
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_ICN6211,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 11,
                .u8ChipAddr = 0x58,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 11,
                .u8ChipAddr = 0xEC,
            },
        },
    },
};


/* RS20016单板上有HDMI和MIPI两路输出,每路输出通道上都有两个显示芯片,和一块FPGA，
   每条通路都有CH7053芯片,RS20016由mcu去配置初始化 */
const DISP_CHIP_CFG_S  RS20016V1_0BoardDispChipCfg[] = 
{
    [DISP_CHIP_CHANNEL_0] = 
    {
        .stVoChannel =  
        {
            .enVoMode  = DISP_VO_HDMI,
            .u8ChipNum = 3,     //HDMI通路上有几个显示芯片
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT8619C,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 8,
                .u8ChipAddr = 0x64,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_FPGA,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 0,                    /* FPGA不需要这些总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
            [2] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 0,                  /* RS20016单板，ch7053和mcu直连，不需要海思芯片传递总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
        },
    },
    [DISP_CHIP_CHANNEL_1] = 
    {
        .stVoChannel =
        {
            .enVoMode  = DISP_VO_MIPI,
            .u8ChipNum = 3,
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT9211,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 7,
                .u8ChipAddr = 0x5A,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_FPGA,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 0,                    /* FPGA不需要这些总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
            [2] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 0,                  /* RS20016单板，ch7053和mcu直连，不需要海思芯片传递总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
        },
    },
};


/* TS3637和RS20016硬件类似，单板上有HDMI和MIPI两路输出,每路输出通道上都有两个显示芯片,和一块FPGA，
   每条通路都有CH7053芯片,RS20016由mcu去配置初始化 */
const DISP_CHIP_CFG_S  TS3637_0BoardDispChipCfg[] = 
{
    [DISP_CHIP_CHANNEL_0] = 
    {
        .stVoChannel =  
        {
            .enVoMode  = DISP_VO_HDMI,
            .u8ChipNum = 3,     //HDMI通路上有几个显示芯片
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT8619C,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 3,
                .u8ChipAddr = 0x64,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_FPGA,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 1,                    /* FPGA不需要这些总线号和设备地址参数，mcu通信用作mcu编号 */
                .u8ChipAddr = 0,
            },
            [2] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 1,                  /* TS3637单板，ch7053和mcu直连，不需要海思芯片传递总线号和设备地址参数，，mcu通信用作mcu编号 */
                .u8ChipAddr = 0,
            },
        },
    },
    [DISP_CHIP_CHANNEL_1] = 
    {
        .stVoChannel =
        {
            .enVoMode  = DISP_VO_MIPI,
            .u8ChipNum = 3,
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT9211,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 2,
                .u8ChipAddr = 0x5A,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_FPGA,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 0,                    /* FPGA不需要这些总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
            [2] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 0,                  /* TS3637单板，ch7053和mcu直连，不需要海思芯片传递总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
        },
    },
};

/* RS20025单板 */
const DISP_CHIP_CFG_S  RS20025V1_0BoardDispChipCfg[] = 
{
    [DISP_CHIP_CHANNEL_1] = 
    {
        .stVoChannel =
        {
            .enVoMode  = DISP_VO_MIPI,
            .u8ChipNum = 1,           //MIPI通路上显示芯片个数
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT8618SX,
                .enChannel  = DISP_CHIP_CHANNEL_BUTT,
                .u8BusIdx   = 0,
                .u8ChipAddr = 0x76,
            },
        },
    },
};

/* TS3719_V1_0单板上有两路MIPI输出,每路输出通道上都有两个显示芯片,和一块FPGA，
   每条通路都有CH7053芯片,RS20016由mcu去配置初始化 */
const DISP_CHIP_CFG_S  TS3719_V1_0BoardDispChipCfg[] = 
{
    [DISP_CHIP_CHANNEL_0] = 
    {
        .stVoChannel =  
        {
            .enVoMode  = DISP_VO_MIPI_1,
            .u8ChipNum = 3,     //HDMI通路上有几个显示芯片
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT9211,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 4,
                .u8ChipAddr = 0x5A,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_FPGA,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 0,                    /* FPGA不需要这些总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
            [2] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_0,
                .u8BusIdx   = 0,                  /* TS3719单板，ch7053和mcu直连，不需要海思芯片传递总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
        },
    },
    [DISP_CHIP_CHANNEL_1] = 
    {
        .stVoChannel =
        {
            .enVoMode  = DISP_VO_MIPI,
            .u8ChipNum = 3,
        },
        .astChipInfo = 
        {
            [0] =
            {
                .enDispChip = DISP_CHIP_LT9211,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 3,
                .u8ChipAddr = 0x5A,
            },
            [1] =
            {
                .enDispChip = DISP_CHIP_FPGA,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 0,                    /* FPGA不需要这些总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
            [2] =
            {
                .enDispChip = DISP_CHIP_CH7053,
                .enChannel  = DISP_CHIP_CHANNEL_1,
                .u8BusIdx   = 0,                  /* TS3719单板，ch7053和mcu直连，不需要海思芯片传递总线号和设备地址参数 */
                .u8ChipAddr = 0,
            },
        },
    },
};

/* 注: 因为有些板子显示芯片相关的硬件信息一致, 所以会有多个板子共用一套信息的情况 */
/* 新增设备类型，需要将设备上的硬件信息填写正确,  iic号之类的信息 */
static const DISP_CHIP_BOARD_MATCH_S g_astDispChipBoardMatch [] =
{
    {DB_DS50018_V1_0,       RS20007V1_0BoardDispChipCfg },
    {DB_RS20001_V1_0,       RS20001V1_1BoardDispChipCfg },
    {DB_RS20001_V1_1,       RS20001V1_1BoardDispChipCfg },
    {DB_RS20006_V1_0,       RS20001V1_1BoardDispChipCfg },
    {DB_RS20007_V1_0,       RS20007V1_0BoardDispChipCfg },
    {DB_RS20007_A_V1_0,     RS20007V1_0BoardDispChipCfg },
    {DB_RS20016_V1_0,       RS20016V1_0BoardDispChipCfg },
    {DB_TS3637_V1_0,        TS3637_0BoardDispChipCfg    },
    {DB_RS20025_V1_0,       RS20025V1_0BoardDispChipCfg },
    {DB_RS20033_V1_0,       RS20025V1_0BoardDispChipCfg },
    {DB_RS20016_V1_1,       RS20016V1_0BoardDispChipCfg },
    {DB_RS20016_V1_1_F303,  RS20016V1_0BoardDispChipCfg },
    {DB_TS3719_V1_0,        TS3719_V1_0BoardDispChipCfg },
    {DB_RS20046_V1_0,        TS3719_V1_0BoardDispChipCfg },
};

/*******************************************************************************
* 函数名  : FpgaSetOutputResolution
* 描  述  : fpga对显示芯片配置接口，保持芯片配置入参统一
* 输  入  : const BT_TIMING_S *pstTiming
            UINT8 u8BusIdx
            UINT8 u8ChipAddr
            UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 FpgaSetOutputResolution(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn)
{
    DISP_LOGI(" Fpga Set Output Resolution [%d*%dP%d]\n", pstTiming->u32Width, pstTiming->u32Height, pstTiming->u32Fps);
    return mcu_func_fpgaSetOutputResolution(u32Chn, pstTiming->u32Width, pstTiming->u32Height, pstTiming->u32Fps);
}


static  DISP_CHIP_FUNC_S    gstFunc[] =
{
    [0] =
    {
        .enDispChip    = DISP_CHIP_ICN6211,
        .SetResolution = ICN6211_Init,
    },
    [1] =
    {
        .enDispChip    = DISP_CHIP_CH7053,
        .SetResolution = CH7053_SetResolution,
    },
    [2] =
    {
        .enDispChip    = DISP_CHIP_LT8612EX,
        .SetResolution = LT8612EX_SetResolution,
    },
    [3] =
    {
        .enDispChip    = DISP_CHIP_LT8619C,
        .SetResolution = LT8619C_Init,
    },
    [4] =
    {
        .enDispChip    = DISP_CHIP_LT9211,
        .SetResolution = LT9211_Config,
    },
    [5] =
    {
        .enDispChip    = DISP_CHIP_FPGA,
        .SetResolution = FpgaSetOutputResolution,
    },
    [6] = 
    {
        .enDispChip    = DISP_CHIP_LT8618SX,
        .SetResolution = LT8618sx_Set,
    },
    
};

/*******************************************************************************
* 函数名  : DispChip_GetBoardInfo
* 描  述  : 获取显示芯片信息
* 输  入  : const DISP_CHIP_CFG_S **pstChipCfg
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32  DispChip_GetBoardInfo(const DISP_CHIP_CFG_S **pstChipCfg)
{
    UINT32  i = 0;
    UINT32  u32Num = 0;
    UINT32  u32BoardType;

    const DISP_CHIP_BOARD_MATCH_S  *pstMatch = g_astDispChipBoardMatch;

    u32Num = SAL_arraySize(g_astDispChipBoardMatch);
    
    u32BoardType = HARDWARE_GetBoardType();
    
    for(i = 0; i < u32Num; i ++, pstMatch ++)
    {
        if(u32BoardType == pstMatch->u32BoardType)
        {
            break;
        }
    }
    
    DISP_LOGI("Disp Chip BoardType[0x%x]\n", u32BoardType);
    if(i >= u32Num)
    {
        DISP_LOGI("Disp Chip unsupport BoardType[%d]\n", u32BoardType);
        return SAL_FAIL;
    }
    
    *pstChipCfg = pstMatch->pstDispChipCfg;
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : DispChip_SetRes
* 描  述  : 显示芯片信息对上层调用统一接口
* 输  入  : DISP_DEV_COMMON *pDispDev
            DISP_VO_MODE_E enVoMode
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_chipSetRes(DISP_DEV_COMMON *pDispDev, DISP_VO_MODE_E enVoMode)
{
    INT8   i = 0, j = 0;
    INT32  s32Ret     = SAL_FAIL;
    UINT8  u8Num      = 0;
    UINT32 u32Chn     = 0;
    UINT32 u32ResNum  = 0;
    UINT8  u8BusIdx   = 0;
    UINT8  u8ChipAddr = 0;
    UINT32 u32FunSize = 0;
    DISP_CHIP_TYPE_E   enChipName;
    const DISP_CHIP_CFG_S *pstChipCfg    = NULL;
    const DISP_CHIP_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S     *pstTiming     = NULL;
    UINT32  u32BoardType = 0;
  

    if(NULL == pDispDev)
    {
        DISP_LOGE("Disp Chip_SetRes failed, pDispDev is null\n");
        return SAL_FAIL;
    }
    
    if(enVoMode >= DISP_VO_BUTT)
    {
        DISP_LOGE("Disp Chip_SetRes failed, enVoMode :[%d] is error  \n", enVoMode);
        return SAL_FAIL;
    }

    s32Ret = DispChip_GetBoardInfo(&pstChipCfg);
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGW("Disp Chip Get Board Info failed\n");
        return SAL_FAIL;
    }

    u32BoardType = HARDWARE_GetBoardType();
    u32ResNum = sizeof(gastDispChipTimingMap)/sizeof(gastDispChipTimingMap[0]);

    for (i = 0; i < u32ResNum; i++)
     {
         pstDispTiming = gastDispChipTimingMap + i;
         if ((pstDispTiming->u32Width == pDispDev->uiDevWith) && 
             (pstDispTiming->u32Height == pDispDev->uiDevHeight) && 
             (pstDispTiming->u32Fps == pDispDev->uiDevFps))
         {
             break;
         }
     }
    
    if ((i >= u32ResNum) || (pstDispTiming->enUserTiming >= TIMING_BUTT))
    {
       DISP_LOGI("unsupport resolution[%uX%u@%u]\n", pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps);
       return SAL_FAIL;
    }

    /* 获取相应时序 */
    pstTiming = &(BT_GetTiming(pstDispTiming->enUserTiming)->stTiming);

    if(NULL == pstTiming)
    {
        DISP_LOGE("Disp Chip_SetRes failed, pstTiming is null\n");
        return SAL_FAIL;
    }

    for(i = 0; i < DISP_CHIP_CHANNEL_BUTT; i++)
    {
        if(enVoMode != pstChipCfg->stVoChannel.enVoMode)
        {
            pstChipCfg ++;
        }
        else
        {
            break;
        }
    }
    u32FunSize = SAL_arraySize(gstFunc);
    
    u8Num = pstChipCfg->stVoChannel.u8ChipNum;
    for(i = 0; i < u8Num; i++)
    {
        enChipName = pstChipCfg->astChipInfo[i].enDispChip;
        u8BusIdx   = pstChipCfg->astChipInfo[i].u8BusIdx;
        u8ChipAddr = pstChipCfg->astChipInfo[i].u8ChipAddr;
        u32Chn     = pstChipCfg->astChipInfo[i].enChannel;

/*DB_TS3637_V1_0 设备hdmi0硬件接口连接的是MIPI接口实际是VoDev1，连接的单片机实际设定为0所以通道1需要重新映射为单片机ID为0*/
        if ((SAL_TRUE == capb_get_capt()->bMcuEnable) && (u32BoardType == DB_TS3637_V1_0))
        {
            u32Chn = u8BusIdx;
        }

        for(j = 0; j < u32FunSize; j++)
        {
            if(enChipName == gstFunc[j].enDispChip)
            {
                if(NULL != gstFunc[j].SetResolution)
                {
                    s32Ret = gstFunc[j].SetResolution(pstTiming, u8BusIdx, u8ChipAddr, u32Chn);
                    if(SAL_SOK != s32Ret)
                    {
                        DISP_LOGE("gstFunc[%d].SetResolution failed\n", j);
                        return SAL_FAIL;
                    }
                    break;
                }
                else
                {
                    DISP_LOGE("gstFunc[%d].SetResolution is null\n", j);
                    return SAL_FAIL;
                }
            }
        }
    }
    return SAL_SOK;
}




