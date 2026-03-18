
/*******************************************************************************
* edid.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : cuifeng5
* Version: V1.0.0  2020年11月30日 Create
*
* Description : EDID操作相关的函数
* Modification:
*******************************************************************************/

#include "sal.h"
#include "platform_hal.h"
#include "../inc/edid.h"
#include "mcu_drv.h"


#define EDID_FILE_NAME_VGA(chn, buff)           sprintf((buff), "/home/config/edid_vga%u.bin", (chn))
#define EDID_FILE_NAME_HDMI(chn, buff)          sprintf((buff), "/home/config/edid_hdmi%u.bin", (chn))
#define EDID_FILE_NAME_DVI(chn, buff)           sprintf((buff), "/home/config/edid_dvi%u.bin", (chn))

/* 对应功能对应的字节位置 */
#define EDID_POS_HEADER             (0x00)      // 标识头信息
#define EDID_POS_MANUF              (0x08)      // 厂商名称
#define EDID_POS_VER                (0x12)      // 版本号
#define EDID_POS_REV                (0x13)      // 修订号
#define EDID_POS_INPUT_DEF          (0x14)      // video input definition
#define EDID_POS_ESTA               (0x23)      // established timing
#define EDID_POS_STAND              (0x26)      // standard timing
#define EDID_POS_DETAIL             (0x36)      // detailed timing
#define EDID_POS_EXTENSION_FLAG     (0x7E)      // extension flag
#define EDID_POS_CHECKSUM           (0x7F)      // CheckSum

#define EDID_HEADER_SIZE            (8)         // 标识头的长度
#define EDID_DETAIL_SIZE            (18)        // 描述一个Detailed Timing需要的字节数
#define EDID_DETAIL_NUM             (4)
#define EDID_MONITOR_NAME_LEN_MAX   (13)        // 显示器名字的最大长度
#define EDID_BLOCK_BYTE             (128)       // 一个block的大小
#define EDID_TOTAL_BYTE_MAX         (256)       // EDID最大值

#define EDID_CHN_NUM_MAX            (2)
#define EDID_FUNC_NAME "edid_func"
#define EDID_MEM_NAME  "edid_mem"


static const EDID_DESC_FALG_MAP_S g_astEdidDescMap[] = 
    {
        {EDID_DESC_SN,          0xFF},
        {EDID_DESC_STRING,      0xFE},
        {EDID_DESC_LIMIT,       0xFD},
        {EDID_DESC_NAME,        0xFC},      /* 当前仅支持设置名字 */
        {EDID_DESC_COLOR,       0xFB},
        {EDID_DESC_STANDARD,    0xFA},
    };

static const UINT8 g_au8EdidVgaDefault[] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x21, 0x2B, 0x9E, 0x02, 0x11, 0x15, 0x03, 0x00,
    0x1B, 0x1D, 0x01, 0x03, 0x08, 0x30, 0x1B, 0x78,
    0x2E, 0xAD, 0x45, 0xA3, 0x57, 0x52, 0xA1, 0x27,
    0x0D, 0x50, 0x54, 0x21, 0x0B, 0x00, 0x81, 0xC0,
    0x95, 0x00, 0xD1, 0xC0, 0xA9, 0x40, 0x81, 0x80,
    0xB3, 0x00, 0x90, 0x40, 0x81, 0x40, 0x30, 0x2A,
    0x00, 0x98, 0x51, 0x00, 0x2A, 0x40, 0x30, 0x70,
    0x13, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,
    0x66, 0x21, 0x56, 0xAA, 0x51, 0x00, 0x1E, 0x30,
    0x46, 0x8F, 0x33, 0x00, 0xC4, 0x8E, 0x21, 0x00,
    0x00, 0x1E, 0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38,
    0x2D, 0x40, 0x10, 0x2C, 0x45, 0x80, 0xC4, 0x8E,
    0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC,
    0x00, 0x48, 0x49, 0x4B, 0x5F, 0x56, 0x47, 0x41,
    0x5F, 0x49, 0x4E, 0x0A, 0x20, 0x20, 0x00, 0x0A,  
};

static const UINT8 g_au8EdidHdmiDefault[] = 
{    
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x21, 0x2B, 0x9F, 0x02, 0x11, 0x15, 0x03, 0x00, 
    0x1B, 0x1D, 0x01, 0x03, 0x80, 0x78, 0x3C, 0x78,
    0x2E, 0x60, 0xE5, 0xA3, 0x57, 0x4B, 0x9C, 0x25, 
    0x11, 0x50, 0x54, 0x21, 0x09, 0x00, 0x81, 0xC0,
    0x95, 0x00, 0xD1, 0xC0, 0xA9, 0x40, 0x81, 0x80, 
    0xB3, 0x00, 0x90, 0x40, 0x81, 0x40, 0x30, 0x2A,
    0x00, 0x98, 0x51, 0x00, 0x2A, 0x40, 0x30, 0x70, 
    0x13, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,
    0x66, 0x21, 0x56, 0xAA, 0x51, 0x00, 0x1E, 0x30, 
    0x46, 0x8F, 0x33, 0x00, 0xC4, 0x8E, 0x21, 0x00,
    0x00, 0x1E, 0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38, 
    0x2D, 0x40, 0x10, 0x2C, 0x45, 0x80, 0xC4, 0x8E,
    0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 
    0x00, 0x48, 0x49, 0x4B, 0x5F, 0x48, 0x44, 0x4D,
    0x49, 0x5F, 0x49, 0x4E, 0x0A, 0x20, 0x01, 0xBC,  
    
    0x02, 0x03, 0x26, 0xF1, 0x4A, 0x90, 0x01, 0x02, 
    0x03, 0x04, 0x11, 0x12, 0x13, 0x1F, 0x4D, 0x23,
    0x09, 0x07, 0x07, 0x83, 0x01, 0x00, 0x00, 0x65, 
    0x03, 0x0C, 0x00, 0x10, 0x00, 0x68, 0x1A, 0x00,
    0x00, 0x01, 0x01, 0x30, 0x90, 0xED, 0x86, 0x6F, 
    0x80, 0xA0, 0x70, 0x38, 0x40, 0x40, 0x30, 0x20,
    0x35, 0x00, 0xBA, 0x89, 0x21, 0x00, 0x00, 0x1E, 
    0xFD, 0x81, 0x80, 0xA0, 0x70, 0x38, 0x1F, 0x40,
    0x30, 0x20, 0x35, 0x00, 0xBA, 0x89, 0x21, 0x00, 
    0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB,
};

static const UINT8 g_au8EdidDviDefault[] = 
{    
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
    0x21, 0x2B, 0x9F, 0x02, 0x11, 0x15, 0x03, 0x00, 
    0x1B, 0x1D, 0x01, 0x03, 0x80, 0x78, 0x3C, 0x78,
    0x2E, 0x60, 0xE5, 0xA3, 0x57, 0x4B, 0x9C, 0x25, 
    0x11, 0x50, 0x54, 0x21, 0x09, 0x00, 0x81, 0xC0,
    0x95, 0x00, 0xD1, 0xC0, 0xA9, 0x40, 0x81, 0x80, 
    0xB3, 0x00, 0x90, 0x40, 0x81, 0x40, 0x30, 0x2A,
    0x00, 0x98, 0x51, 0x00, 0x2A, 0x40, 0x30, 0x70, 
    0x13, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,
    0x66, 0x21, 0x56, 0xAA, 0x51, 0x00, 0x1E, 0x30, 
    0x46, 0x8F, 0x33, 0x00, 0xC4, 0x8E, 0x21, 0x00,
    0x00, 0x1E, 0x02, 0x3A, 0x80, 0xD0, 0x72, 0x38, 
    0x2D, 0x40, 0x10, 0x2C, 0x45, 0x80, 0xC4, 0x8E,
    0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 
    0x00, 0x48, 0x49, 0x4B, 0x5F, 0x44, 0x56, 0x49,
    0x5F, 0x49, 0x4E, 0x0A, 0x20, 0x20, 0x00, 0xDC, 
};

/* EDID缓存结构体 */
static VIDEO_EDID_BUFF_S g_astEdidBuff[EDID_CHN_NUM_MAX][CAPT_CABLE_BUTT] = {0};

static EDID_DISP_DEV_S g_astEdidDev104[EDID_CHN_NUM_MAX][CAPT_CABLE_BUTT] = 
{
    {
        /* 第0路VGA */
        [CAPT_CABLE_VGA] = {
            .enDispType = EDID_DISP_DEV_TYPE_BUTT,      /* 不支持VGA */
        },
    
        /* 第0路HDMI */
        [CAPT_CABLE_HDMI] = {
            .enDispType = EDID_DISP_DEV_TYPE_HISI_HDMI,
            .enHiHdmi   = SOC_HDMI_ID_0,
        },

        /* 第0路DVI */
        [CAPT_CABLE_DVI] = {
            .enDispType = EDID_DISP_DEV_TYPE_BUTT,      /* 输出无DVI信号 */
        },
    },

    {
        /* 第1路VGA */
        [CAPT_CABLE_VGA] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {2, 0xA0},
        },
    
        /* 第1路HDMI */
        [CAPT_CABLE_HDMI] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {6, 0xA0},
        },

        /* 第1路DVI */
        [CAPT_CABLE_DVI] = {
            .enDispType = EDID_DISP_DEV_TYPE_BUTT,      /* 输出无DVI信号 */
        },
    },
};

static EDID_DISP_DEV_S g_astEdidDev204[EDID_CHN_NUM_MAX][CAPT_CABLE_BUTT] = 
{
    {
        /* 第0路VGA */
        [CAPT_CABLE_VGA] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {1, 0xA0},
        },
    
        /* 第0路HDMI */
        [CAPT_CABLE_HDMI] = {
            .enDispType = EDID_DISP_DEV_TYPE_HISI_HDMI,
            .enHiHdmi   = SOC_HDMI_ID_0,
        },

        /* 第0路DVI */
        [CAPT_CABLE_DVI] = {
            .enDispType = EDID_DISP_DEV_TYPE_BUTT,      /* 输出无DVI信号 */
        },
    },

    {
        /* 第1路VGA */
        [CAPT_CABLE_VGA] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {2, 0xA0},
        },
    
        /* 第1路HDMI */
        [CAPT_CABLE_HDMI] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {6, 0xA0},
        },

        /* 第1路DVI */
        [CAPT_CABLE_DVI] = {
            .enDispType = EDID_DISP_DEV_TYPE_BUTT,      /* 输出无DVI信号 */
        },
    },
};


static EDID_DISP_DEV_S g_astEdidDevIsaBox[EDID_CHN_NUM_MAX][CAPT_CABLE_BUTT] = 
{

    {
        /* 第1路VGA */
        [CAPT_CABLE_VGA] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {1, 0xA0},
        },
    
        /* 第1路HDMI */
        [CAPT_CABLE_HDMI] = {
            .enDispType = EDID_DISP_DEV_TYPE_IIC,
            .stI2cDev = {0, 0xA0},
        },

        /* 第1路DVI */
        [CAPT_CABLE_DVI] = {
            .enDispType = EDID_DISP_DEV_TYPE_BUTT,      /* 输出无DVI信号 */
        },
    },
};


/* 设置芯片内的EDID */
EDID_SetChipEdid g_apfuncSetChipEdid[EDID_CHN_NUM_MAX] = {0};


/*******************************************************************************
* 函数名  : EDID_CheckSum
* 描  述  : 计算CheckSum
* 输  入  : UINT8 *pu8Buff : 输入缓存
          UINT32 u32Len : 输入缓存大小
* 输  出  : UINT8 *pu8Sum : checksum返回结果
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static UINT8 EDID_CheckSum(UINT8 *pu8Buff, UINT32 u32Len)
{
    UINT32 u32Sum = 0;
    UINT32 i = 0;

    while (i++ < u32Len)
    {
        u32Sum += *pu8Buff++;
    }

    u32Sum &= 0xFF;

    /* 所有字节相加等于0x00 */
    return (UINT8)((0x100 - u32Sum) & 0xFF);
}


/*******************************************************************************
* 函数名  : EDID_CheckHeader
* 描  述  : 当前EDID头是否有效
* 输  入  : UINT8 *pu8Buff : EDID缓存
          UINT32 u32Len : EDID的长度
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_CheckHeader(const UINT8 *pu8Buff)
{
    static const UINT8 au8Header[EDID_HEADER_SIZE] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
    UINT32 i = 0;

    if (NULL == pu8Buff)
    {
        EDID_LOGE("invalid parameter, buff[%p]\n", pu8Buff);
        return SAL_FAIL;
    }

    /* EDID前8个字节头信息，内容固定 */
    for (i = 0; i < EDID_HEADER_SIZE; i++)
    {
        if (pu8Buff[i + EDID_POS_HEADER] != au8Header[i])
        {
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

#ifdef CAPT_DEBUG

/*******************************************************************************
* 函数名  : EDID_FillDetailTiming
* 描  述  : 填充Detailed Timing缓存
* 输  入  : BT_TIMING_S *pstTiming : 支持的时序
          UINT8 *pu8Buff : 填充时序的缓存
          UINT32 u32BuffLen : 缓存的长度
* 输  出  : 无
* 返回值  : 0        : 成功s
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_FillDetailTiming(const BT_TIMING_S *pstTiming, UINT8 *pu8Buff, UINT32 u32BuffLen)
{
    UINT32 u32Tmp = 0;

    if ((NULL == pstTiming) || (NULL == pu8Buff) || (u32BuffLen < EDID_DETAIL_SIZE))
    {
        EDID_LOGE("invalid parameter, timing[%p], buff[%p], len[%u]\n", pstTiming, pu8Buff, u32BuffLen);
        return SAL_FAIL;
    }

    /* 下面操作参考EDID标准Detailed Timing章节，共18个字节 */

    /* Pixel clock */
    u32Tmp = pstTiming->u32PixelClock / 10000;
    pu8Buff[0] = u32Tmp & 0xFF;
    pu8Buff[1] = (u32Tmp >> 8) & 0xFF;

    /* Horizontal Active */
    u32Tmp = pstTiming->u32Width;
    pu8Buff[2] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[4], u32Tmp >> 8, 4, 0, 4);

    /* Horizontol Blanking */
    u32Tmp = pstTiming->u32HFrontPorch + pstTiming->u32HSync + pstTiming->u32HBackPorch;
    pu8Buff[3] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[4], u32Tmp >> 8, 0, 0, 4);

    /* Vertical Active */
    u32Tmp = pstTiming->u32Height;
    pu8Buff[5] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[7], u32Tmp >> 8, 4, 0, 4);

    /* Vertical Blanking */
    u32Tmp = pstTiming->u32VFrontPorch + pstTiming->u32VSync + pstTiming->u32VBackPorch;
    pu8Buff[6] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[7], u32Tmp >> 8, 0, 0, 4);

    /* Horizontal Sync Offset */
    u32Tmp = pstTiming->u32HFrontPorch;
    pu8Buff[8] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[11], u32Tmp >> 8, 6, 0, 2);

    /* Horizontal Sync Pulse Width */
    u32Tmp = pstTiming->u32HSync;
    pu8Buff[9] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[11], u32Tmp >> 8, 4, 0, 2);

    /* Vertical Sync Offset */
    u32Tmp = pstTiming->u32VFrontPorch;
    BITS_SET(pu8Buff[10], u32Tmp & 0x0F, 4, 0, 4);
    BITS_SET(pu8Buff[11], u32Tmp >> 4, 2, 0, 2);

    /* Vertical Sync Pulse Width */
    u32Tmp = pstTiming->u32VSync;
    BITS_SET(pu8Buff[10], u32Tmp & 0x0F, 0, 0, 4);
    BITS_SET(pu8Buff[11], u32Tmp >> 4, 0, 0, 2);
    
    /* Horizontal Image Size, mm */
    u32Tmp = 708;
    pu8Buff[12] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[14], u32Tmp >> 8, 4, 0, 4);

    /* Vertical Image Size, mm */
    u32Tmp = 398;
    pu8Buff[13] = u32Tmp & 0xFF;
    BITS_SET(pu8Buff[14], u32Tmp >> 8, 0, 0, 4);

    /* Horizontal Border */
    pu8Buff[15] = 0;

    /* Vertical Border */
    pu8Buff[16] = 0;

    /* Interlace */
    BIT_CLEAR(pu8Buff[17], 7, UINT8);

    /* Stereo */
    BITS_SET(pu8Buff[17], 0x00, 5, 0, 2);
    BIT_CLEAR(pu8Buff[17], 0, UINT8);

    /* Sync Configuration */
    BITS_SET(pu8Buff[17], 0x03, 3, 0, 2);

    /* Horizontal polarity */
    if (pstTiming->enHSPol == POLARITY_POS)
    {
        BIT_SET(pu8Buff[17], 1, UINT8);
    }
    else
    {
        BIT_CLEAR(pu8Buff[17], 1, UINT8);
    }

    /* Vertical polarity */
    if (pstTiming->enHSPol == POLARITY_POS)
    {
        BIT_SET(pu8Buff[17], 2, UINT8);
    }
    else
    {
        BIT_CLEAR(pu8Buff[17], 2, UINT8);
    }

    return SAL_SOK;
}
#endif

/*******************************************************************************
* 函数名  : EDID_GetDetailTiming
* 描  述  : 获取Detailed Timing缓存
* 输  入  : BT_TIMING_S *pstTiming : 时序
          UINT8 *pu8Buff : 存放时序的缓存
          UINT32 u32BuffLen : 缓存的长度
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetDetailTiming(BT_TIMING_S *pstTiming, UINT8 *pu8Buff, UINT32 u32BuffLen)
{
    UINT32 u32HBlank = 0;
    UINT32 u32VBlank = 0;

    if ((NULL == pstTiming) || (NULL == pu8Buff) || (u32BuffLen < EDID_DETAIL_SIZE))
    {
        EDID_LOGE("invalid parameter, timing[%p], buff[%p], len[%u]\n", pstTiming, pu8Buff, u32BuffLen);
        return SAL_FAIL;
    }

    /* 下面操作参考EDID标准Detailed Timing章节，共18个字节 */

    /* Pixel clock */
    BITS_CAT(pstTiming->u32PixelClock, pu8Buff[1], 0, 8, pu8Buff[0], 0, 8);
    pstTiming->u32PixelClock *= 10000;

    /* Horizontal Active */
    BITS_CAT(pstTiming->u32Width, pu8Buff[4], 4, 4, pu8Buff[2], 0, 8);

    /* Horizontol Blanking */
    BITS_CAT(u32HBlank, pu8Buff[4], 0, 4, pu8Buff[3], 0, 8);

    /* Vertical Active */
    BITS_CAT(pstTiming->u32Height, pu8Buff[7], 4, 4, pu8Buff[5], 0, 8);

    /* Vertical Blanking */
    BITS_CAT(u32VBlank, pu8Buff[7], 0, 4, pu8Buff[6], 0, 8);

    /* Horizontal Sync Offset */
    BITS_CAT(pstTiming->u32HFrontPorch, pu8Buff[11], 6, 2 , pu8Buff[8], 0, 8);

    /* Horizontal Sync Pulse Width */
    BITS_CAT(pstTiming->u32HSync, pu8Buff[11], 4, 2, pu8Buff[9], 0, 8);

    /* Vertical Sync Offset */
    BITS_CAT(pstTiming->u32VFrontPorch, pu8Buff[11], 2, 2, pu8Buff[10], 4, 4);

    /* Vertical Sync Pulse Width */
    BITS_CAT(pstTiming->u32VSync, pu8Buff[11], 2, 2, pu8Buff[10], 0, 4);

    /* Horizontal Backporch */
    pstTiming->u32HBackPorch = u32HBlank - pstTiming->u32HFrontPorch - pstTiming->u32HSync;

    /* Vertical Backporch */
    pstTiming->u32VBackPorch = u32VBlank - pstTiming->u32VFrontPorch - pstTiming->u32VSync;

    pstTiming->f32VFreq = pstTiming->u32PixelClock / (FLOAT32)((u32HBlank + pstTiming->u32Width) * (u32VBlank + pstTiming->u32Height));
    pstTiming->u32Fps = (pstTiming->f32VFreq + 0.5);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : EDID_GetResByTiming
* 描  述  : 通过时序获取分辨率
* 输  入  : BT_TIMING_S *pstTiming : 时序
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static BT_TIMING_E EDID_GetResByTiming(BT_TIMING_S *pstTimging)
{
    UINT32 u32ResNum = BT_GetTimingNum();
    UINT32 i = 0;
    UINT32 u32SrcHBlank = 0;
    UINT32 u32SrcVBlank = 0;
    UINT32 u32DstHBlank = 0;
    UINT32 u32DstVBlank = 0;
    const BT_TIMING_S *pstTmp = NULL;

    for (i = 0; i < u32ResNum; i++)
    {
        pstTmp = &BT_GetTiming((BT_TIMING_E)i)->stTiming;
    
        u32SrcHBlank = pstTimging->u32HFrontPorch + pstTimging->u32HSync + pstTimging->u32HBackPorch;
        u32SrcVBlank = pstTimging->u32VFrontPorch + pstTimging->u32VSync + pstTimging->u32VBackPorch;
        u32DstHBlank = pstTmp->u32HFrontPorch + pstTmp->u32HSync + pstTmp->u32HBackPorch;
        u32DstVBlank = pstTmp->u32VFrontPorch + pstTmp->u32VSync + pstTmp->u32VBackPorch;

        /* 对比有效宽高、消隐区宽高、像素时钟 */
        if (  (pstTimging->u32Width == pstTmp->u32Width)
           && (pstTimging->u32Height == pstTmp->u32Height)
           && (pstTimging->u32PixelClock == pstTmp->u32PixelClock)
           && (u32SrcHBlank == u32DstHBlank)
           && (u32SrcVBlank == u32DstVBlank))
        {
            return (BT_TIMING_E)i;
        }
    }

    return TIMING_BUTT;
}


/*******************************************************************************
* 函数名  : EDID_GetDetailedInfo
* 描  述  : 获取时序或者设备的详细描述
* 输  入  : UINT8 *pu8Buff : EDID详细时序的缓存
          UINT32 u32BuffLen : 缓存大小
* 输  出  : EDID_DETAILED_INFO_S *pstDetailedInfo : 计算所得的详细时序
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetDetailedInfo(EDID_DETAILED_INFO_S *pstDetailedInfo, UINT8 *pu8Buff, UINT32 u32BuffLen)
{
    UINT32 u32DescNum = sizeof(g_astEdidDescMap)/sizeof(g_astEdidDescMap[0]);
    UINT32 i = 0, j = 0;
    BT_TIMING_S stTiming;
    BT_TIMING_E enTiming = TIMING_BUTT;
    INT32 s32Ret = 0;

    if ((NULL == pstDetailedInfo) || (NULL == pu8Buff) || (u32BuffLen < EDID_DETAIL_SIZE))
    {
        EDID_LOGE("invalid parameter, detailed info[%p], buff[%p], len[%u]\n", 
                  pstDetailedInfo, pu8Buff, u32BuffLen);
        return SAL_FAIL;
    }

    /* 存放Detailed Timing的18个细节，也可以存放一些设备描述信息，此时下标为0,1,2,4的数据均为0 */
    if ((0 == pu8Buff[0]) && (0 == pu8Buff[1]) && (0 == pu8Buff[2]) && (0 == pu8Buff[4]))
    {
        for (i = 0; i < u32DescNum; i++)
        {
            /* 下标为3的数据标志EDID标准已定义的描述信息 */
            if (pu8Buff[3] == g_astEdidDescMap[i].u8Flag)
            {
                pstDetailedInfo->enType            = EDID_DETAILED_DESC;
                pstDetailedInfo->stDescInfo.enDes  = g_astEdidDescMap[i].enType;
                if (EDID_DESC_NAME == pstDetailedInfo->stDescInfo.enDes)
                {
                    memset(pstDetailedInfo->stDescInfo.szName, 0, sizeof(pstDetailedInfo->stDescInfo.szName));
                    for (j = 0; j < EDID_MONITOR_NAME_LEN_MAX; j++)
                    {
                        /* 遇到不可见字符认为结束 */
                        if (pu8Buff[5 + j] < 0x20)
                        {
                            break;
                        }
                        else
                        {
                            pstDetailedInfo->stDescInfo.szName[j] = pu8Buff[5 + j];
                        }
                    }
                }
                
                return SAL_SOK;
            }
        }

        pstDetailedInfo->enType = EDID_DETAILED_BUTT;
        return SAL_SOK;
    }

    s32Ret = EDID_GetDetailTiming(&stTiming, pu8Buff, u32BuffLen);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("get detailed timing fail\n");
        return SAL_FAIL;
    }

    enTiming = EDID_GetResByTiming(&stTiming);
    if (TIMING_BUTT != enTiming)
    {
        pstDetailedInfo->enType   = EDID_DETAILED_TIMING;
        pstDetailedInfo->enTiming = enTiming;
    }
    else
    {
        pstDetailedInfo->enType = EDID_DETAILED_BUTT;
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : EDID_GetDefaultRes
* 描  述  : 获取时序或者设备的详细描述
* 输  入  : UINT8 *pu8Buff : EDID详细时序的缓存
          UINT32 u32BuffLen : 缓存大小
* 输  出  : EDID_DETAILED_INFO_S *pstDetailedInfo : 计算所得的详细时序
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetDefaultRes(VIDEO_RESOLUTION_S *pstRes, UINT8 *pu8Buff)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT8 *pu8Tmp = pu8Buff + EDID_POS_DETAIL;
    BT_TIMING_S stTiming;

    for (i = 0; i < 4; i++)
    {
        if ((0 == pu8Tmp[0]) && (0 == pu8Tmp[1]) && (0 == pu8Tmp[2]) && (0 == pu8Tmp[4]))
        {
            pu8Tmp += EDID_DETAIL_SIZE;
            continue;
        }

        s32Ret = EDID_GetDetailTiming(&stTiming, pu8Tmp, EDID_DETAIL_SIZE);
        if (SAL_SOK != s32Ret)
        {
            EDID_LOGE("get detailed timing fail\n");
            return SAL_FAIL;
        }

        pstRes->uiWidth  = stTiming.u32Width;
        pstRes->uiHeight = stTiming.u32Height;
        pstRes->uiFps    = stTiming.u32Fps;

        return SAL_SOK;
    }

    return SAL_FAIL;
}


/*******************************************************************************
* 函数名  : EDID_GetMointerName
* 描  述  : 获取显示器名称
* 输  入  : UINT8 *pu8Buff : EDID详细时序的缓存
          UINT32 u32BuffLen : 缓存大小
* 输  出  : EDID_DETAILED_INFO_S *pstDetailedInfo : 计算所得的详细时序
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetMointerName(char *szName, UINT8 *pu8Buff)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;    
    EDID_DETAILED_INFO_S stDetailInfo;
    UINT8 *pu8Tmp = pu8Buff + EDID_POS_DETAIL;

    for (i = 0; i < 4; i++)
    {
        s32Ret = EDID_GetDetailedInfo(&stDetailInfo, pu8Tmp + i * EDID_DETAIL_SIZE, EDID_DETAIL_SIZE);
        if (SAL_SOK != s32Ret)
        {
            EDID_LOGW("edid get detailed info[%u] fail\n", i);
            continue;
        }

        if ((EDID_DETAILED_DESC == stDetailInfo.enType) && (EDID_DESC_NAME == stDetailInfo.stDescInfo.enDes))
        {
            memcpy(szName, stDetailInfo.stDescInfo.szName, 16);
            return SAL_SOK;
        }
    }

    EDID_LOGE("edid get mointer name fail\n");
    return SAL_FAIL;
}



/*******************************************************************************
* 函数名  : EDID_GetManufactureName
* 描  述  : 获取厂商名字
* 输  入  : UINT8 *pu8Buff : EDID详细时序的缓存
          UINT32 u32BuffLen : 缓存大小
* 输  出  : EDID_DETAILED_INFO_S *pstDetailedInfo : 计算所得的详细时序
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetManufactureName(char *szName, const UINT8 *pu8Buff)
{
    UINT8 u8Tmp = 0;;

    u8Tmp = pu8Buff[EDID_POS_MANUF] >> 2;
    u8Tmp &= 0x1F;
    szName[0] = u8Tmp + 'A' - 1;

    u8Tmp = pu8Buff[EDID_POS_MANUF] << 3;
    u8Tmp |= pu8Buff[EDID_POS_MANUF + 1] >> 5;
    u8Tmp &= 0x1F;
    szName[1] = u8Tmp + 'A' - 1;

    u8Tmp = pu8Buff[EDID_POS_MANUF + 1];
    u8Tmp &= 0x1F;
    szName[2] = u8Tmp + 'A' - 1;

    szName[3] = '\0';
    
    return SAL_SOK;
}

#ifdef CAPT_DEBUG


/*******************************************************************************
* 函数名  : EDID_SwapMem
* 描  述  : 内存空间交换
* 输  入  : UINT8 *pucMem1
          UINT8 *pucMem2
          INT32 uiLen
* 输  出  : 
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_SwapMem(UINT8 *pu8Mem1, UINT8 *pu8Mem2, UINT32 u32Len)
{
    UINT8 u8Tmp = 0;

    if ((NULL == pu8Mem1) || (NULL == pu8Mem2))
    {
        EDID_LOGE("null pointer[%p, %p]\n", pu8Mem1, pu8Mem2);
        return SAL_FAIL;
    }

    while (u32Len--)
    {
        u8Tmp = pu8Mem1[u32Len];
        pu8Mem1[u32Len] = pu8Mem2[u32Len];
        pu8Mem2[u32Len] = u8Tmp;
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : EDID_ModifyRes
* 描  述  : 修改默认分辨率
* 输  入  : BT_TIMING_E enTiming : 要修改的目标时序
*       : UINT8 *pu8Buff : 详细时序的缓存
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_ModifyRes(BT_TIMING_E enTiming, UINT8 *pu8Buff)
{
    INT32 s32Ret = 0;
    UINT32 i = 0;
    EDID_DETAILED_INFO_S astDetailInfo[EDID_DETAIL_NUM];
    const BT_TIMING_S *pstTiming = NULL;

    if (NULL == pu8Buff)
    {
        EDID_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    if (EDID_CheckHeader(pu8Buff) < 0)
    {
        EDID_LOGE("edid in device invalid\n");
        return SAL_FAIL;
    }

    /* 共4块可以描述detailed   timging信息，72个字节 */
    for (i = 0; i < EDID_DETAIL_NUM; i++)
    {
        s32Ret = EDID_GetDetailedInfo(&astDetailInfo[i], 
                                    pu8Buff + EDID_POS_DETAIL + i * EDID_DETAIL_SIZE,
                                    EDID_DETAIL_SIZE);
        if (s32Ret < 0)
        {
            EDID_LOGE("get detail info[%u] fail\n", i);
            return SAL_FAIL;
        }

        if ((EDID_DETAILED_TIMING == astDetailInfo[i].enType) && (enTiming == astDetailInfo[i].enTiming))
        {
            break;
        }
    }

    /* 第一个detailed timing默认为主画面，无需修改               */
    if (0 == i)
    {
        SAL_DEBUG("not need to modify the best resolution\n");
        return SAL_SOK;
    }

    if (i < EDID_DETAIL_NUM)
    {   
        /* 设备中存在该detailed timing，移动到第一个              */
        s32Ret = EDID_SwapMem(pu8Buff + EDID_POS_DETAIL,
                              pu8Buff + EDID_POS_DETAIL + i * EDID_DETAIL_SIZE,
                              EDID_DETAIL_SIZE);
    }
    else
    {
        /* 设备中无该detailed timing，修改第一个            */
        s32Ret = EDID_FillDetailTiming(pstTiming, pu8Buff + EDID_POS_DETAIL, EDID_DETAIL_SIZE);
    }

    if (s32Ret < 0)
    {
        EDID_LOGE("fill datailed timing fail\n");
        return SAL_SOK;
    }

    /* 计算CheckSum */
    pu8Buff[EDID_POS_CHECKSUM] = EDID_CheckSum(pu8Buff, EDID_BLOCK_BYTE - 1);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : EDID_SetMonitorName
* 描  述  : 设置监视器名称
* 输  入  : char *szStr : 监视器名称
          UINT8 *pu8Buff : 填充时序的缓存
          UINT32 u32BuffLen : 缓存的长度
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_SetMonitorName(char *szStr, UINT8 *pu8Buff, UINT32 u32BuffLen)
{
    UINT32 u32NameLen = 0;
    UINT32 u32Remain = 0;
    UINT32 u32Tmp = 0;

    if ((NULL == szStr) || (NULL == pu8Buff) || (u32BuffLen < EDID_DETAIL_SIZE))
    {
        EDID_LOGE("invalid parameter, str[%p], buff[%p], len[%u]\n",
                  szStr, pu8Buff, u32BuffLen);
        return SAL_FAIL;
    }

    u32NameLen = strlen(szStr) > EDID_MONITOR_NAME_LEN_MAX ? EDID_MONITOR_NAME_LEN_MAX : strlen(szStr);

    /* 标志位 */
    *pu8Buff++ = 0x00;
    *pu8Buff++ = 0x00;
    *pu8Buff++ = 0x00;
    *pu8Buff++ = 0xFC;
    *pu8Buff++ = 0x00;

    /* Monitor Name */
    u32Tmp = u32NameLen;
    while (u32Tmp--)
    {
        *pu8Buff++ = *szStr++;
    }

    /* 结束符 */
    *pu8Buff++ = 0x0A;

    /* 字节不足填充0x20 */
    u32Remain = EDID_MONITOR_NAME_LEN_MAX - u32NameLen;
    while (u32Remain--)
    {
        *pu8Buff++ = 0x20;
    }

    return SAL_SOK;
}
#endif

/*******************************************************************************
* 函数名  : EDID_GetSignalType
* 描  述  : 获取信号类型
* 输  入  : UINT8 *pu8Buff : EDID缓存
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static inline EDID_SIGNAL_TYPE_E EDID_GetSignalType(const UINT8 *pu8Buff)
{
    return (*(pu8Buff + EDID_POS_INPUT_DEF) & 0x80) ? EDID_SIGNAL_DIGTAL : EDID_SIGNAL_ANALOG;
}




/*******************************************************************************
* 函数名  : EDID_GetEdidFromHiEdid
* 描  述  : 获取显示模块的EDID
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetEdidFromHiEdid(EDID_DISP_DEV_S *pstDispDev, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = disp_hal_getHdmiEdid(pstDispDev->enHiHdmi, pu8Buff, pu32Len);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", pstDispDev->enHiHdmi, s32Ret);
        return SAL_FAIL;
    }

    EDID_LOGI("get edid from hisi[%d] success\n", pstDispDev->enHiHdmi);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : EDID_GetEdidFromIIC
* 描  述  : 获取显示模块的EDID
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetEdidFromIIC(EDID_DISP_DEV_S *pstDispDev, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32RegAddr = 0;
    UINT8 *pu8BuffTmp = pu8Buff;
    UINT32 i = 0;

    *pu32Len = 0;
    for (i = 0; i < EDID_BLOCK_BYTE; i++, u32RegAddr++, pu8BuffTmp++)
    {
        s32Ret = IIC_Read(pstDispDev->stI2cDev.u32IIC, pstDispDev->stI2cDev.u32DevAddr, u32RegAddr, pu8BuffTmp);
        if (SAL_SOK != s32Ret)
        {
            EDID_LOGE("iic[%u] dev[0x%x] read reg[0x%x] fail\n", pstDispDev->stI2cDev.u32IIC, pstDispDev->stI2cDev.u32DevAddr, u32RegAddr);
            return SAL_FAIL;
        }
    }
    *pu32Len += EDID_BLOCK_BYTE;

    if (0 != pu8Buff[EDID_POS_EXTENSION_FLAG])
    {
        if (pu8Buff[EDID_POS_EXTENSION_FLAG] > 1)
        {
            EDID_LOGW("set edid extension block num[%u] to 1\n", pu8Buff[EDID_POS_EXTENSION_FLAG]);
            pu8Buff[EDID_POS_EXTENSION_FLAG] = 1;
            pu8Buff[EDID_POS_CHECKSUM] = EDID_CheckSum(pu8Buff, EDID_BLOCK_BYTE - 1);
        }

        for (i = 0; i < EDID_BLOCK_BYTE; i++, u32RegAddr++, pu8BuffTmp++)
        {
            s32Ret = IIC_Read(pstDispDev->stI2cDev.u32IIC, pstDispDev->stI2cDev.u32DevAddr, u32RegAddr, pu8BuffTmp);
            if (SAL_SOK != s32Ret)
            {
                EDID_LOGE("iic[%u] dev[%u] read reg[%u] fail\n", pstDispDev->stI2cDev.u32IIC, pstDispDev->stI2cDev.u32DevAddr, u32RegAddr);
                return SAL_FAIL;
            }
        }

        *pu32Len += EDID_BLOCK_BYTE;
    }

    EDID_LOGI("get edid bytes[%u] from iic[%u] success\n", *pu32Len, pstDispDev->stI2cDev.u32IIC);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : EDID_RegisterSetFunc
* 描  述  : 注册回调函数
* 输  入  : UINT32 u32Chn : 对应通道
          EDID_SIGNAL_TYPE_E : edid类型
* 输  出  : UINT32 *pu32Len : 获取到的EDID长度
* 返回值  : const UINT8* : 获取到的EDID的地址
*******************************************************************************/
inline INT32 EDID_RegisterSetFunc(UINT32 u32Chn, EDID_SetChipEdid pfuncSetChipEdid)
{
    if (NULL == pfuncSetChipEdid)
    {
        EDID_LOGW("edid chn[%u] set chip func is null\n", u32Chn);
        return SAL_FAIL;
    }

    g_apfuncSetChipEdid[u32Chn] = pfuncSetChipEdid;
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : EDID_GetEdid
* 描  述  : 获取EDID
* 输  入  : UINT32 u32Chn : 对应通道
          EDID_SIGNAL_TYPE_E : edid类型
* 输  出  : UINT32 *pu32Len : 获取到的EDID长度
* 返回值  : const UINT8* : 获取到的EDID的地址
*******************************************************************************/
VIDEO_EDID_BUFF_S* EDID_GetEdid(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    char szFile[32] = {0};
    VIDEO_EDID_BUFF_S *pstBuff = &g_astEdidBuff[u32Chn][enCable];
    const UINT8 *pu8Default = NULL;
    UINT32 u32ReadSize = 0;

    switch (enCable)
    {
        case CAPT_CABLE_VGA:
            EDID_FILE_NAME_VGA(u32Chn, szFile);
            pu8Default = g_au8EdidVgaDefault;
            pstBuff->u32Len = sizeof(g_au8EdidVgaDefault);
            break;
        case CAPT_CABLE_HDMI:
            EDID_FILE_NAME_HDMI(u32Chn, szFile);
            pu8Default = g_au8EdidHdmiDefault;
            pstBuff->u32Len = sizeof(g_au8EdidHdmiDefault);
            break;
        case CAPT_CABLE_DVI:
            EDID_FILE_NAME_DVI(u32Chn, szFile);
            pu8Default = g_au8EdidDviDefault;
            pstBuff->u32Len = sizeof(g_au8EdidDviDefault);
            break;
        default:
            EDID_LOGE("chn[%u] get edid invalid cable[%d]\n", u32Chn, enCable);
            return NULL;
    }

    /* 先将默认EDID拷贝到缓存中，如果有设置过EDID，会覆盖掉原来的EDID */
    sal_memcpy_s(pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, pu8Default, pstBuff->u32Len);

    /* 无EDID文件返回默认EDID */
    if (SAL_SOK != SAL_ReadFromFile(szFile, 0, (CHAR *)pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, &u32ReadSize))
    {
        EDID_LOGW("chn[%u] get edid from file:%s fail\n", u32Chn, szFile);
        sal_memcpy_s(pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, pu8Default, pstBuff->u32Len);
    }
    else
    {
        pstBuff->u32Len = u32ReadSize;
    }

    
    return pstBuff;
}


/*******************************************************************************
* 函数名  : EDID_SetEdid
* 描  述  : 设置EDID
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 EDID_SetEdid(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len, EDID_SIGNAL_TYPE_E *penSignal)
{
    char szFile[32] = {0};
    VIDEO_EDID_BUFF_S *pstBuff = NULL;
    EDID_SetChipEdid pfuncSetEdid = g_apfuncSetChipEdid[u32Chn];

    if ((NULL == pu8Buff) || (NULL == penSignal) || (0 != (u32Len % EDID_BLOCK_BYTE)))
    {
        EDID_LOGE("invalid parameter:%p, %p, %u\n", pu8Buff, penSignal, u32Len);
        return SAL_FAIL;
    }

    *penSignal = EDID_SIGNAL_BUTT;

    if (SAL_SOK != EDID_CheckHeader(pu8Buff))
    {
        EDID_LOGE("edid check header fail:0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x,\n",
                  pu8Buff[0], pu8Buff[1], pu8Buff[2], pu8Buff[3],
                  pu8Buff[4], pu8Buff[5], pu8Buff[6], pu8Buff[7]);
        return SAL_FAIL;
    }

    *penSignal = EDID_GetSignalType(pu8Buff);

    if (EDID_SIGNAL_DIGTAL == *penSignal)
    {
        /* HDMI */
        pstBuff = &g_astEdidBuff[u32Chn][CAPT_CABLE_HDMI];
        
        EDID_FILE_NAME_HDMI(u32Chn, szFile);
		SAL_WriteToFile(szFile, 0, SEEK_SET, (CHAR *)pu8Buff, u32Len);

        sal_memcpy_s(pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, pu8Buff, u32Len);
        pstBuff->u32Len = EDID_TOTAL_BYTE_MAX > u32Len ? u32Len : EDID_TOTAL_BYTE_MAX;

        if (NULL != pfuncSetEdid)
        {
            (VOID)pfuncSetEdid(u32Chn, pstBuff, CAPT_CABLE_HDMI);
        }

        EDID_LOGI("chn[%u] write edid to[%s] success\n", u32Chn, szFile);

        /* DVI */
        pstBuff = &g_astEdidBuff[u32Chn][CAPT_CABLE_DVI];

        EDID_FILE_NAME_DVI(u32Chn, szFile);
		SAL_WriteToFile(szFile, 0, SEEK_SET, (CHAR *)pu8Buff, u32Len);

        sal_memcpy_s(pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, pu8Buff, u32Len);
        pstBuff->u32Len = EDID_TOTAL_BYTE_MAX > u32Len ? u32Len : EDID_TOTAL_BYTE_MAX;

        if (NULL != pfuncSetEdid)
        {
            (VOID)pfuncSetEdid(u32Chn, pstBuff, CAPT_CABLE_DVI);
        }

        EDID_LOGI("chn[%u] write edid to[%s] success\n", u32Chn, szFile);
    }
    else
    {
        /* VGA */
        pstBuff = &g_astEdidBuff[u32Chn][CAPT_CABLE_VGA];

        EDID_FILE_NAME_VGA(u32Chn, szFile);
		SAL_WriteToFile(szFile, 0, SEEK_SET, (CHAR *)pu8Buff, u32Len);

        sal_memcpy_s(pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, pu8Buff, u32Len);
        pstBuff->u32Len = EDID_TOTAL_BYTE_MAX > u32Len ? u32Len : EDID_TOTAL_BYTE_MAX;

        if (NULL != pfuncSetEdid)
        {
            (VOID)pfuncSetEdid(u32Chn, pstBuff, CAPT_CABLE_VGA);
        }

        EDID_LOGI("chn[%u] write edid to[%s] success\n", u32Chn, szFile);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : EDID_ReLoadEdid
* 描  述  : 从文件中重新加载EDID信息
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 EDID_ReLoadEdid(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    char szFile[32] = {0};
    VIDEO_EDID_BUFF_S *pstBuff = NULL;
    EDID_SetChipEdid pfuncSetEdid = g_apfuncSetChipEdid[u32Chn];

    if ((u32Chn >= EDID_CHN_NUM_MAX) || (enCable >= CAPT_CABLE_BUTT))
    {
        EDID_LOGE("invalid parameter:chn[%u] cable[%d]\n", u32Chn, enCable);
        return SAL_FAIL;
    }

    pstBuff = &g_astEdidBuff[u32Chn][enCable];

    switch (enCable)
    {
        case CAPT_CABLE_VGA:
            EDID_FILE_NAME_VGA(u32Chn, szFile);
            break;
        case CAPT_CABLE_HDMI:
            EDID_FILE_NAME_HDMI(u32Chn, szFile);
            break;
        case CAPT_CABLE_DVI:
            EDID_FILE_NAME_DVI(u32Chn, szFile);
            break;
        default:
            EDID_LOGE("invalid cable[%d]\n", enCable);
            return SAL_FAIL;
    }

    if (SAL_SOK != SAL_ReadFromFile(szFile, 0, (CHAR *)pstBuff->au8Buff, EDID_TOTAL_BYTE_MAX, &pstBuff->u32Len))
    {
        EDID_LOGE("get edid from file:%s fail\n", szFile);
        return SAL_FAIL;
    }

    if (NULL != pfuncSetEdid)
    {
        (VOID)pfuncSetEdid(u32Chn, pstBuff, enCable);
        EDID_LOGD("edid chn[%u] cable[%d] has changed\n", u32Chn, enCable);
    }

    return SAL_SOK;
}





/*******************************************************************************
* 函数名  : EDID_GetDispEdid
* 描  述  : 获取显示模块的EDID
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
static INT32 EDID_GetDispEdid(INT32 u32DispChn, UINT32 u32ViNum, EDID_SIGNAL_TYPE_E enSignal, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    INT32 s32Ret = SAL_SOK;
    EDID_DISP_DEV_S *pstDispDev = NULL;
    CAPT_CABLE_E enCable = (EDID_SIGNAL_DIGTAL == enSignal) ? CAPT_CABLE_HDMI : CAPT_CABLE_VGA;
    UINT32  u32BoardType = HARDWARE_GetBoardType();
    VIDEO_EDID_BUFF_S stEdid;
    CAPB_CAPT *pstCaptCap = NULL;

    if ((u32DispChn >= 2) || (enSignal >= EDID_SIGNAL_BUTT))
    {
        EDID_LOGE("disp[%u] or signal[%d] is invalid\n", u32DispChn, enSignal);
        return SAL_FAIL;
    }

    pstCaptCap = capb_get_capt();
    if(NULL == pstCaptCap)
    {
        CAPT_LOGE("get platform capbility fail\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstCaptCap->bMcuEnable)
    {
        stEdid.enCable = enCable;
        if (SAL_SOK != mcu_func_mointorGetEdid(u32DispChn, &stEdid))
        {
            EDID_LOGE("get edid fail\n");
            return SAL_FAIL;
        }

        sal_memcpy_s(pu8Buff, EDID_TOTAL_BYTE_MAX, stEdid.au8Buff, stEdid.u32Len);
        *pu32Len = stEdid.u32Len;
        return SAL_SOK;
    }

    if (1 == u32ViNum)
    {
        if(u32BoardType == DB_RS20032_V1_0)
        {
            pstDispDev = &g_astEdidDevIsaBox[u32DispChn][enCable];
        }
        else
        {
            pstDispDev = &g_astEdidDev104[u32DispChn][enCable];
        }
        
    }
    else if (2 == u32ViNum)
    {
        pstDispDev = &g_astEdidDev204[u32DispChn][enCable];
    }
    else
    {
        EDID_LOGE("invalid input chn num[%u]\n", u32ViNum);
        return SAL_FAIL;
    }

    switch (pstDispDev->enDispType)
    {
        case EDID_DISP_DEV_TYPE_HISI_HDMI:
            s32Ret = EDID_GetEdidFromHiEdid(pstDispDev, pu8Buff, pu32Len);
            break;
        case EDID_DISP_DEV_TYPE_IIC:
            s32Ret = EDID_GetEdidFromIIC(pstDispDev, pu8Buff, pu32Len);
            break;
        default:
            EDID_LOGE("invalid edid disp dev type[%d]\n", pstDispDev->enDispType);
            return SAL_FAIL;
    }

    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("get edid fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : EDID_DispLoopBack
* 描  述  : 将显示模块连接的EDID设置到输入
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 EDID_DispLoopBack(UINT32 u32Chn, UINT32 u32DispChn, UINT32 u32ViNum, EDID_SIGNAL_TYPE_E enSignal)
{
    INT32 s32Ret = SAL_SOK;
    EDID_SIGNAL_TYPE_E enRetSignal = EDID_SIGNAL_BUTT;
    UINT8 au8Buff[EDID_TOTAL_BYTE_MAX];
    UINT32 u32Len = 0;

    s32Ret = EDID_GetDispEdid(u32DispChn, u32ViNum, enSignal, au8Buff, &u32Len);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("get edid from disp[%u] signal[%d] fail\n", u32DispChn, enSignal);
        return SAL_FAIL;
    }
    
    s32Ret = EDID_SetEdid(u32Chn, au8Buff, u32Len, &enRetSignal);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("chn[%u] set edid fail\n", u32Chn);
        return SAL_FAIL;
    }

    if (enSignal != enRetSignal)
    {
        EDID_LOGW("chn[%u] edid signal is not equal[%d %d]\n",u32Chn, enSignal, enRetSignal);
    }

    EDID_LOGI("chn[%u] set edid from disp success\n", u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : EDID_GetEdidInfo
* 描  述  : 获取EDID信息
* 输  入  : 
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 EDID_GetEdidInfo(UINT32 u32Chn, CAPT_EDID_INFO_S *pstEdidInfo)
{
    INT32 s32Ret = SAL_SOK;
    VIDEO_EDID_BUFF_S *pu8Edid = NULL;

    if (NULL == pstEdidInfo)
    {
        EDID_LOGE("edid info pointer is null\n");
        return SAL_FAIL;
    }

    pu8Edid = EDID_GetEdid(u32Chn, (CAPT_CABLE_E)pstEdidInfo->enMode);
    if (NULL == pu8Edid)
    {
        EDID_LOGE("chn[%u] get edid fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = EDID_GetManufactureName(pstEdidInfo->szManufactureName, pu8Edid->au8Buff);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("get manufacture name fail\n");
        return SAL_FAIL;
    }

    s32Ret = EDID_GetMointerName(pstEdidInfo->szMonitorName, pu8Edid->au8Buff);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("get monitor name fail\n");
        return SAL_FAIL;
    }

    s32Ret = EDID_GetDefaultRes(&pstEdidInfo->stRes, pu8Edid->au8Buff);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("get res fail\n");
        return SAL_FAIL;
    }

    EDID_LOGI("get edid info[%uX%uP%u], manuf:%s name:%s\n", pstEdidInfo->stRes.uiWidth, pstEdidInfo->stRes.uiHeight, pstEdidInfo->stRes.uiFps,
                                                             pstEdidInfo->szManufactureName, pstEdidInfo->szMonitorName);

    return SAL_SOK;
}




