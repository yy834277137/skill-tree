
/*******************************************************************************
 * adv7842_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2020年09月07日 Create
 *
 * Description : ADV7842配置
 * Modification: 
 *******************************************************************************/

#include "platform_hal.h"
#include "../inc/adv7842_drv.h"


#include "sal.h"

/*******************************************************************************
 ***********************    ADV7842宏    ***************************************
 *******************************************************************************/
#define ADV_COLOK_PERI0D            (28636363)      /* 7842测量时钟 */
#define ADV_INPUT_PIXEL_CLOCK_MAX   (170000000)     /* 支持的最大像素点时钟数 */


/*******************************************************************************
 ***********************    用户配置宏    **************************************
 *******************************************************************************/
#define ADV_I2C_IO_MAP              (0x40)
#define ADV_I2C_SDP_MAP             (0x90)
#define ADV_I2C_SDPIO_MAP           (0x94)
#define ADV_I2C_AVLINKI_MAP         (0x84)
#define ADV_I2C_CEC_MAP             (0x30)
#define ADV_I2C_INFOFRAME_MAP       (0x3C)
#define ADV_I2C_AFE_MAP             (0x50)
#define ADV_I2C_REPEATER_MAP        (0x34)
#define ADV_I2C_EDID_MAP            (0x5C)
#define ADV_I2C_HDMI_MAP            (0x57)
#define ADV_I2C_CP_MAP              (0x54)
#define ADV_I2C_VDP_MAP             (0x48)
#define ADV_I2C_EEPROM_MAP          (0xA0)

#define ADV_NUM_MAX                 (2)             /* 接入的7842的数量 */
#define ADV_RESET_FILE              "/dev/DDM/reset"

/* 读寄存器，如果读取错误，返回失败 */
#define ADV_READ_AND_RETURN(iic, dev, addr, val) \
    do { \
        if (SAL_SOK != IIC_Read((iic), (dev), (addr), (val))) \
        { \
            ADV_LOGE("iic[%u] dev[%u] read[0x%x] error\n", (iic), (dev), (addr)); \
            return ADV_ERR_IIC_FAIL; \
        } \
    } while (0)

/* 读寄存器，忽略IIC失败 */
#define ADV_READ(iic, dev, addr, val) \
    do { \
        if (SAL_SOK != IIC_Read((iic), (dev), (addr), (val))) \
        { \
            ADV_LOGE("iic[%u] dev[%u] read[0x%x] error\n", (iic), (dev), (addr)); \
        } \
    } while (0)

/* 写寄存器，如果写错误，返回失败 */
#define ADV_WRITE_AND_RETURN(iic, dev, addr, val) \
    do { \
        if (SAL_SOK != IIC_Write((iic), (dev), (addr), (val))) \
        { \
            ADV_LOGE("iic[%u] dev[0x%x] write[0x%x] error\n", (iic), (dev), (addr)); \
            return ADV_ERR_IIC_FAIL; \
        } \
    } while (0)

/* 写寄存器，忽略IIC失败 */
#define ADV_WRITE(iic, dev, addr, val) \
    do { \
        if (SAL_SOK != IIC_Write((iic), (dev), (addr), (val))) \
        { \
            ADV_LOGE("iic[%u] dev[0x%x] write[0x%x] error\n", (iic), (dev), (addr)); \
        } \
    } while (0)

/* 置位寄存器的一位 */
#define ADV_SET_REG_BIT(iic, dev, addr, tmp, offset) \
    do { \
        ADV_READ_AND_RETURN((iic), (dev), (addr), &tmp); \
        tmp |= (0x01 << (offset)); \
        ADV_WRITE_AND_RETURN((iic), (dev), (addr), tmp); \
    } while (0)

/* 将寄存器中的对应位写成相应的值 */
#define ADV_SET_REG_BITS(iic, dev, addr, tmp, offset, bits, val) \
    do { \
        ADV_READ_AND_RETURN((iic), (dev), (addr), &tmp); \
        tmp |= ((0x01 << (bits)) - 1) << (offset); \
        tmp &= ((val) << (offset)); \
        ADV_WRITE_AND_RETURN((iic), (dev), (addr), tmp); \
    } while (0)

/* 将寄存器中的对应位置位 */
#define ADV_SET_REG_MASK(iic, dev, addr, tmp, mask) \
    do { \
        ADV_READ_AND_RETURN((iic), (dev), (addr), &tmp); \
        tmp |= mask; \
        ADV_WRITE_AND_RETURN((iic), (dev), (addr), tmp); \
    } while (0)

/* 清除寄存器的一位 */
#define ADV_CLEAR_REG_BIT(iic, dev, addr, tmp, offset) \
    { \
        ADV_READ_AND_RETURN((iic), (dev), (addr), &tmp); \
        tmp &= ~(0x01 << (offset)); \
        ADV_WRITE_AND_RETURN((iic), (dev), (addr), tmp); \
    }

/* 将寄存器中的对应位置0 */
#define ADV_CLEAR_REG_MASK(iic, dev, addr, tmp, mask) \
    do { \
        ADV_READ_AND_RETURN((iic), (dev), (addr), &tmp); \
        tmp &= ~mask; \
        ADV_WRITE_AND_RETURN((iic), (dev), (addr), tmp); \
    } while (0)



/*******************************************************************************
 ***********************    全局变量     ***************************************
 *******************************************************************************/
static CAPT_CHIP_FUNC_S g_advExtChipFunc = {0};
static const ADV_HDMI_PORT_E genAdvHdmiPort = ADV_HDMI_PORT_A;      /* 请参考硬件原理图 */
static const UINT32 gau32I2cMap[ADV_NUM_MAX] = {11, 5};             /* 请参考硬件原理图 */
static ADV_DETECT_STATUS_S gastCurrentStatus[ADV_NUM_MAX] = {
    {TIMING_BUTT,   CAPT_CABLE_BUTT},
    {TIMING_BUTT,   CAPT_CABLE_BUTT},
};
static VIDEO_EDID_BUFF_S *g_apstEdidBuff[ADV_NUM_MAX] = {0};              /* EDID缓存结构体 */

/* CSC参数，区间范围0--100 */
static VIDEO_CSC_S gastCsc[ADV_NUM_MAX] = {
    {50, 50, 50, 50},
    {50, 50, 50, 50},
};

/* 枚举对应的字符串名称 */
static const char *gaszCableName[CAPT_CABLE_BUTT + 1] = {
    [CAPT_CABLE_VGA]  = "VGA",
    [CAPT_CABLE_HDMI] = "HDMI",
    [CAPT_CABLE_DVI]  = "DVI",
    [CAPT_CABLE_BUTT] = "NO INPUT"
};

/* VGA默认配置 */
static const ADV_REG_MAP_S gastVgaDefault[] = {
    {ADV_I2C_SDPIO_MAP, 0x29, 0x10}, /* memory pins should be tristated*/
    {ADV_I2C_SDP_MAP,   0x12, 0x00}, /* Disable 3D comb and Frame TBC.*/
    {ADV_I2C_SDPIO_MAP, 0x39, 0x40}, /* Disable clock to external memory. This is recommended to give lower power consumption.*/
    {ADV_I2C_SDP_MAP,   0x0E, 0x21}, /* Disable Y_2D_PK_EN.*/
    {ADV_I2C_IO_MAP,    0x00, 0x07}, /* autographics */
    {ADV_I2C_IO_MAP,    0x01, 0x82}, /* Prim_Mode to graphics input */
    {ADV_I2C_IO_MAP,    0x02, 0xF4}, /* Auto input color space, Limited Range RGB Output */
    {ADV_I2C_IO_MAP,    0x03, 0x80}, /* 16 bit SDR 422 */
    {ADV_I2C_IO_MAP,    0x05, 0x2C}, /* Enable AV Codes */
    {ADV_I2C_IO_MAP,    0x06, 0xA1}, /* Invert VS,HS pins */
    {ADV_I2C_IO_MAP,    0x0C, 0x40}, /* Power up Part */
    {ADV_I2C_IO_MAP,    0x15, 0xB0}, /* Disable Tristate of Pins except for Audio pins */
    {ADV_I2C_IO_MAP,    0x19, 0x80}, /* LLC DLL Phase */
    {ADV_I2C_IO_MAP,    0x33, 0x40}, /* LLC DLL Enable */
    {ADV_I2C_CP_MAP,    0x73, 0xEA}, /* Set manual gain of 0x2A8 */
    {ADV_I2C_CP_MAP,    0x74, 0x8A}, /* Set manual gain of 0x2A8 */
    {ADV_I2C_CP_MAP,    0x75, 0xA2}, /* Set manual gain of 0x2A8 */
    {ADV_I2C_CP_MAP,    0x76, 0xA8}, /* Set manual gain of 0x2A8 */
    {ADV_I2C_CP_MAP,    0x81, 0xD0}, /* GR_AV_BL_EN */
    {ADV_I2C_CP_MAP,    0x85, 0x0B}, /* Disable Autodetectmode for Sync_Source for CH1. Force CH1 to use HS&VS */
    {ADV_I2C_CP_MAP,    0x91, 0x00}, /* Set to progressive mode */
    {ADV_I2C_CP_MAP,    0xC0, 0x40}, /* Default value for channel A*/
    {ADV_I2C_CP_MAP,    0xC1, 0x70}, /* Default value for channel B*/
    {ADV_I2C_CP_MAP,    0xC2, 0xC0}, /* Default value for channel C*/
    {ADV_I2C_CP_MAP,    0xBF, 0x16}, /* Force FreeRun */
    {ADV_I2C_CP_MAP,    0xC3, 0x39}, /* ADI recommended write */
    {ADV_I2C_AFE_MAP,   0x0C, 0x1F}, /* ADI recommended write */
    {ADV_I2C_AFE_MAP,   0x12, 0x63}, /* ADI recommended write */
    {ADV_I2C_AFE_MAP,   0x00, 0x80}, /* ADC power Up */
    {ADV_I2C_AFE_MAP,   0x02, 0x00}, /* Ain_Sel to 000. (Ain 1,2,3) */
    {ADV_I2C_AFE_MAP,   0xC8, 0x33}  /* DLL Phase, 110011b */
};

/* HDMI默认配置 */
static const ADV_REG_MAP_S gastHdmiDefault[] = {
    {ADV_I2C_IO_MAP,    0x00, 0x13}, /* VID_STD =10011b 720P60 */
    {ADV_I2C_IO_MAP,    0x01, 0x05}, /* Prim_Mode =101b HDMI-Comp */
    {ADV_I2C_IO_MAP,    0x02, 0xF4}, /* Auto input color space, Limited Range YPbPr Output */
    {ADV_I2C_IO_MAP,    0x03, 0x80}, /* 16 bit SDR 422 */
    {ADV_I2C_IO_MAP,    0x05, 0x2C}, /* Enable AV Codes */
    {ADV_I2C_IO_MAP,    0x06, 0xA1}, /* Invert VS,HS pins */
    {ADV_I2C_HDMI_MAP,  0xC1, 0xFF}, /* HDMI power control (power saving) */
    {ADV_I2C_HDMI_MAP,  0xC2, 0xFF}, /* HDMI power control (power saving) */
    {ADV_I2C_HDMI_MAP,  0xC3, 0xFF}, /* HDMI power control (power saving) */
    {ADV_I2C_HDMI_MAP,  0xC4, 0xFF}, /* HDMI power control (power saving) */
    {ADV_I2C_HDMI_MAP,  0xC5, 0x00}, /* HDMI power control (power saving) */
    {ADV_I2C_HDMI_MAP,  0xC6, 0x00}, /* HDMI power control (power saving) */
    {ADV_I2C_HDMI_MAP,  0xC0, 0xFF}, /* HDMI power control (power saving) */
    {ADV_I2C_IO_MAP,    0x0C, 0x40}, /* Power up Part */
    {ADV_I2C_IO_MAP,    0x15, 0x80}, /* Disable Tristate of Pins except for Audio pins */
    {ADV_I2C_IO_MAP,    0x19, 0x83}, /* LLC DLL Phase */
    {ADV_I2C_IO_MAP,    0x33, 0x40}, /* LLC DLL Enable */
    {ADV_I2C_CP_MAP,    0xBA, 0x01}, /* Set HDMI FreeRun */
    {ADV_I2C_CP_MAP,    0x3E, 0x00}, /* Disable CP Pregain Block */
    {ADV_I2C_CP_MAP,    0x6C, 0x00}, /* Use fixed clamp values */
    {ADV_I2C_CP_MAP,    0x81, 0xD0}, /* GR_AV_BL_EN */
    {ADV_I2C_CP_MAP,    0xC0, 0x40}, /* Default value for channel A*/
    {ADV_I2C_CP_MAP,    0xC1, 0x70}, /* Default value for channel B*/
    {ADV_I2C_CP_MAP,    0xC2, 0xC0}, /* Default value for channel C*/
    {ADV_I2C_CP_MAP,    0xBF, 0x16}, /* Force FreeRun */
    {ADV_I2C_CP_MAP,    0xC9, 0x2D}, /* Disable the buffering of measured param in HDMI mode, free run standard determined by PRIM_MODE,VID_STD*/
    {ADV_I2C_AFE_MAP,   0x00, 0xFF}, /* Power Down ADC's and there associated clocks */
    {ADV_I2C_AFE_MAP,   0x01, 0xFE}, /* Power down ref buffer_bandgap_clamps_sync strippers_input mux_output buffer */
    {ADV_I2C_AFE_MAP,   0xB5, 0x01}, /* Setting MCLK to 256Fs */
    {ADV_I2C_HDMI_MAP,  0x00, 0x32}, /* Set HDMI Input Port A (Enable BG monitoring) */
    {ADV_I2C_HDMI_MAP,  0x01, 0x18}, /* Enable clock terminators */
    {ADV_I2C_HDMI_MAP,  0x0D, 0x34}, /* ADI recommended write */        
    {ADV_I2C_HDMI_MAP,  0x1A, 0x8A}, /* Unmute audio */
    {ADV_I2C_HDMI_MAP,  0x3D, 0x10}, /* HDMI ADI recommended write */
    {ADV_I2C_HDMI_MAP,  0x44, 0x85}, /* TMDS PLL Optimization */
    {ADV_I2C_HDMI_MAP,  0x46, 0x1F}, /* ADI Recommended Write ES3/Final silicon */
    {ADV_I2C_HDMI_MAP,  0x60, 0x88}, /* TMDS PLL Optimization */
    {ADV_I2C_HDMI_MAP,  0x61, 0x88}, /* TMDS PLL Optimization */
    {ADV_I2C_HDMI_MAP,  0x6C, 0x18}, /* Disable ISRC clearing bit, Improve robustness */
    {ADV_I2C_HDMI_MAP,  0x57, 0xB6}, /* TMDS PLL Optimization  */
    {ADV_I2C_HDMI_MAP,  0x58, 0x03}, /* TMDS PLL Setting */
    {ADV_I2C_HDMI_MAP,  0x67, 0x20}, /* TMDS PLL Setting */
    {ADV_I2C_HDMI_MAP,  0x75, 0x10}, /* DDC drive strength  */
    {ADV_I2C_HDMI_MAP,  0x85, 0x1F}, /* ADI Equaliser Setting */
    {ADV_I2C_HDMI_MAP,  0x87, 0x70}, /* HDMI Recommended write */
    {ADV_I2C_HDMI_MAP,  0x89, 0x04}, /* ADI Equaliser Setting */
    {ADV_I2C_HDMI_MAP,  0x8A, 0x1E}, /* ADI Equaliser Setting */
    {ADV_I2C_HDMI_MAP,  0x93, 0x04}, /* ADI Equaliser Setting */
    {ADV_I2C_HDMI_MAP,  0x94, 0x1E}, /* ADI Equaliser Setting */
    {ADV_I2C_HDMI_MAP,  0x9D, 0x02}, /* ADI Equaliser Setting */
    {ADV_I2C_HDMI_MAP,  0x99, 0xA1}, /* HDMI ADI recommended write */
    {ADV_I2C_HDMI_MAP,  0x9B, 0x09}, /* HDMI ADI recommended write */
    {ADV_I2C_HDMI_MAP,  0xC9, 0x01}  /* HDMI free Run based on PRIM_MODE, VID _STD */
};


/*******************************************************************************
* 函数名  : ADV_GetI2c
* 描  述  : 获取使用哪个IIC
* 输  入  : UINT32 u32Chn
* 输  出  : IIC
* 返回值  : IIC
*******************************************************************************/
static inline UINT32 ADV_GetI2c(UINT32 u32Chn)
{
    return gau32I2cMap[u32Chn];
}


/*******************************************************************************
* 函数名  : ADV_WriteRegArray
* 描  述  : IIC配置多个寄存器
* 输  入  : UINT32 u32Chn
* 输  出  : IIC
* 返回值  : IIC
*******************************************************************************/
static INT32 ADV_WriteRegArray(UINT32 u32Chn, const ADV_REG_MAP_S *pstRegMap, UINT32 u32RegNum)
{
    UINT32 i = 0;
    UINT32 u32I2c = ADV_GetI2c(u32Chn);
    const ADV_REG_MAP_S *pstMap = pstRegMap;

    if (NULL == pstMap)
    {
        SAL_ERROR("adv chn[%u] null pointer\n", u32Chn);
        return SAL_FAIL;
    }

    
    for (i = 0; i < u32RegNum; i++, pstMap++)
    {
        ADV_WRITE_AND_RETURN(u32I2c, pstMap->u8DevAddr, pstMap->u8RegAddr, pstMap->u8Value);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_HDMI_GetInputRes
* 描  述  : 获取HDMI接口输入的分辨率
* 输  入  : UINT32 u32Chn
* 输  出  : UINT32 *pu32Width : 输入图像宽
          UINT32 *pu32Height : 输入图像高
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_HDMI_GetRes(UINT32 u32Chn, UINT32 *pu32Width, UINT32 *pu32Height)
{
    UINT32 u32I2c   = ADV_GetI2c(u32Chn);
    UINT8 u8RegVal0 = 0;
    UINT8 u8RegVal1 = 0;

    ADV_READ_AND_RETURN(u32I2c, ADV_I2C_HDMI_MAP, 0x07, &u8RegVal0);
    ADV_READ_AND_RETURN(u32I2c, ADV_I2C_HDMI_MAP, 0x08, &u8RegVal1);
    *pu32Width = ((UINT32)(u8RegVal0 & 0x0F) << 8) | u8RegVal1;

    ADV_READ_AND_RETURN(u32I2c, ADV_I2C_HDMI_MAP, 0x09, &u8RegVal0);
    ADV_READ_AND_RETURN(u32I2c, ADV_I2C_HDMI_MAP, 0x0A, &u8RegVal1);
    *pu32Height = ((UINT32)(u8RegVal0 & 0x0F) << 8) | u8RegVal1;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_ReadSTDI
* 描  述  : 读STDI寄存器中的数值
* 输  入  : UINT32 u32Chn
* 输  出  : ADV_STDI_S *pstSTDI
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_ReadSTDI(UINT32 u32Chn, ADV_STDI_S *pstSTDI)
{
    INT32 ret          = SAL_SOK;
    UINT32 u32I2c      = ADV_GetI2c(u32Chn);
    UINT8 au8RegVal[7] = {0};
    static const UINT8 au8RegAddr[7] = {
        0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB8, 0xB9,
    };

    ret = IIC_ReadArray(u32I2c, ADV_I2C_CP_MAP, sizeof(au8RegAddr), au8RegAddr, au8RegVal);
    if (SAL_SOK != ret)
    {
        ADV_LOGE("read stdi reg error\n");
        return ADV_ERR_IIC_FAIL;
    }

    BITS_CAT(pstSTDI->u16BL, au8RegVal[0], 0, 6, au8RegVal[1], 0, 8);
    BITS_SET(pstSTDI->u16LCVS, au8RegVal[2], 0, 3, 5);
    BITS_CAT(pstSTDI->u16LCF, au8RegVal[2], 0, 3, au8RegVal[3], 0, 8);
    BITS_CAT(pstSTDI->u16FCL, au8RegVal[5], 0, 5, au8RegVal[6], 0, 8);
    pstSTDI->enHSPol = (au8RegVal[4] & 0x10) ? ((au8RegVal[4] & 0x08) ? CAPT_POL_POS : CAPT_POL_NEG) : CAPT_POL_BUTT;
    pstSTDI->enVSPol = (au8RegVal[4] & 0x40) ? ((au8RegVal[4] & 0x20) ? CAPT_POL_POS : CAPT_POL_NEG) : CAPT_POL_BUTT;
    pstSTDI->bInterlaced = (au8RegVal[0] & 0x40) ? SAL_TRUE : SAL_FALSE;

    ADV_LOGD("adv chn[%u] unsupport timing: bl[%u], lcf[%u] fcl[%u] lcvs[%u]\n",
              u32Chn, pstSTDI->u16BL, pstSTDI->u16LCF, pstSTDI->u16FCL, pstSTDI->u16LCVS);

    /* 目前仅支持逐行扫描，不支持隔行扫描 */
    if (SAL_TRUE == pstSTDI->bInterlaced)
    {
        ADV_LOGW("unsupport interlaced mod\n");
        return ADV_ERR_UNSUPPORT_TIMING;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_DetectCable
* 描  述  : 检测外部接入的接口是VGA还是HDMI
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_DetectCable(UINT32 u32Chn, CAPT_CABLE_E *penCable, ADV_HDMI_PORT_E *penPort, BOOL *pbVideoDected, ADV_STDI_S *pstSTDI)
{
    UINT32 u32I2c   = ADV_GetI2c(u32Chn);
    UINT8 u8RegVal  = 0;
    UINT8 u8RegMask = 0;
    INT32 s32Cnt    = 0;
    INT32 s32Ret    = SAL_SOK;

    *penCable = CAPT_CABLE_BUTT;

    /* 5V cable只有HDMI有 */
    ADV_READ_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x6F, &u8RegVal);
    if (u8RegVal & (0x01 << 0))
    {
        *penCable = CAPT_CABLE_HDMI;
        *penPort  = ADV_HDMI_PORT_B;

        u8RegMask |= 0x01 << 0;
        u8RegMask |= 0x01 << 4;
    }
    else if (u8RegVal & (0x01 << 1))
    {
        *penCable = CAPT_CABLE_HDMI;
        *penPort  = ADV_HDMI_PORT_A;

        u8RegMask |= 0x01 << 1;
        u8RegMask |= 0x01 << 5;
    }

    if (CAPT_CABLE_HDMI == *penCable)
    {
        ADV_READ_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x6A, &u8RegVal);

        /* TDMS 时钟不稳定 */
        s32Cnt = 10;
        if (u8RegMask != (u8RegVal & u8RegMask))
        {
            while (s32Cnt-- > 0)
            {
                ADV_READ_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x6A, &u8RegVal);
                if (u8RegMask == (u8RegVal & u8RegMask))
                {
                    break;
                }

                SAL_msleep(10);
            }

            if (u8RegMask != (u8RegVal & u8RegMask))
            {
                ADV_LOGW("TDMS clock not detected\n");
                return ADV_ERR_NO_TDMS;
            }
        }
    }

    /* 检测STDI测量结果是否锁定 */
    ADV_READ_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0xB1, &u8RegVal);
    if (u8RegVal & (0x01 << 7))
    {
        s32Ret = ADV_ReadSTDI(u32Chn, pstSTDI);
        if (SAL_SOK != s32Ret)
        {
            ADV_LOGE("adv chn[%u] read stdi fail:0x%x\n", u32Chn, s32Ret);
            return s32Ret;
        }

        if ((pstSTDI->u16BL > 0) && (pstSTDI->u16LCF > 0) && (pstSTDI->u16LCVS > 0))
        {
            *pbVideoDected = SAL_TRUE;
            if (CAPT_CABLE_BUTT == *penCable)
            {
                *penCable = CAPT_CABLE_VGA;
            }
        }
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_STDI2Timing
* 描  述  : 将STDI检测结果转化为时序
* 输  入  : UINT32 u32Width
          UINT32 u32Height
          ADV_STDI_S *pstSTDI
* 输  出  : BT_TIMING_MAP_S **ppstTiming
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_STDI2Timing(UINT32 u32Width, UINT32 u32Height, ADV_STDI_S *pstSTDI, const BT_TIMING_MAP_S **ppstTiming)
{
    UINT32 u32HFreq      = 0;
    UINT32 u32PixClock   = 0;                 /* 像素时钟 */
    UINT32 u32TimingNum  = BT_GetTimingNum();
    
    const BT_TIMING_S *pstTiming = NULL;
    UINT32 i = 0;

    if (0 == pstSTDI->u16BL)
    {
        ADV_LOGE("adv read invalid bl\n");
        return SAL_FAIL;
    }

    u32HFreq = ADV_COLOK_PERI0D * 8 / pstSTDI->u16BL;

    for (i = 0; i < u32TimingNum; i++)
    {
        pstTiming = &gastTimingMap[i].stTiming;

        if ((0 != u32Width) && (0 != u32Height))
        {
            if ((pstTiming->u32Width != u32Width) || (pstTiming->u32Height != u32Height))
            {
                continue;
            }
        }
        
        if ((pstSTDI->u16LCF + 1) != BT_GetVTotal(pstTiming))
        {
            continue;
        }

        if (pstSTDI->u16LCVS != pstTiming->u32VSync)
        {
            continue;
        }

        if ((u32HFreq < pstTiming->u32HFreq - 1000) ||
            (u32HFreq > pstTiming->u32HFreq + 1000))
        {
            continue;
        }

        u32PixClock = u32HFreq * BT_GetHTotal(pstTiming);
        if ((u32PixClock > pstTiming->u32PixelClock - 1000000) &&
            (u32PixClock < pstTiming->u32PixelClock + 1000000))
        {
            *ppstTiming = &gastTimingMap[i];
            return SAL_SOK;
        }
    }

    return ADV_ERR_UNSUPPORT_TIMING;
}


/*******************************************************************************
* 函数名  : ADV_HardReset
* 描  述  : 7842硬复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_HardReset(UINT32 u32Chn)
{
    INT32 s32Ret   = SAL_SOK;
    INT32 s32Fd    = -1;
    UINT32 u32Gpio = (u32Chn == 0) ? 1 : 2;
	DDM_resetSet stResetPrm = {0};
	
    s32Fd = open(ADV_RESET_FILE, O_RDWR);
	if (s32Fd < 0)
	{
	   	ADV_LOGE("open %s fail\n", ADV_RESET_FILE);
        return SAL_FAIL;
	}

	stResetPrm.type    = DDM_RESET_TYPE_VIDEO;
	stResetPrm.subtype =  DDM_VIO_SUBTYPE_AD7842;
	stResetPrm.num     = u32Gpio;

    /* BSP在驱动中有添加延时，所以，此处无需延时，至少延时5ms */
	s32Ret = ioctl(s32Fd, DDM_IOC_RESET_OFFON, &stResetPrm);
    close(s32Fd);
	if(s32Ret < 0)
	{	
		ADV_LOGE("reset adv7842[%u] fail\n", u32Chn);
        return SAL_FAIL;
	}

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SoftReset
* 描  述  : 7842软复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SoftReset(UINT32 u32Chn)
{
    ADV_WRITE_AND_RETURN(ADV_GetI2c(u32Chn), ADV_I2C_IO_MAP, 0xFF, 0x80);      /* MAIN_RESET */
    SAL_msleep(20);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SetI2cMap
* 描  述  : 配置7842除IO模块其他模块的IIC设备地址
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetI2cMap(UINT32 u32Chn)
{
    INT32 s32Ret  = SAL_SOK;
    UINT32 u32I2c = ADV_GetI2c(u32Chn);
    UINT8 u8Val   = 0;
    static const IIC_ADDR_MAP_S astMap[] = {
        {0xF1, ADV_I2C_SDP_MAP,},
        {0xF2, ADV_I2C_SDPIO_MAP,},
        {0xF3, ADV_I2C_AVLINKI_MAP,},
        {0xF4, ADV_I2C_CEC_MAP,},
        {0xF5, ADV_I2C_INFOFRAME_MAP,},
        {0xF8, ADV_I2C_AFE_MAP,},
        {0xF9, ADV_I2C_REPEATER_MAP,},
        {0xFA, ADV_I2C_EDID_MAP,},
        {0xFB, ADV_I2C_HDMI_MAP,},
        {0xFD, ADV_I2C_CP_MAP,},
        {0xFE, ADV_I2C_VDP_MAP,},
    };

    s32Ret = IIC_WriteArray(u32I2c, ADV_I2C_IO_MAP, sizeof(astMap)/sizeof(astMap[0]), astMap);
    if (SAL_SOK != s32Ret)
    {
        ADV_LOGE("set i2c map fail\n");
        return ADV_ERR_IIC_FAIL;
    }


    ADV_SET_REG_BIT(u32I2c, ADV_I2C_CP_MAP, 0xF5, u8Val, 1);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_PowerUp
* 描  述  : 7842内部模块上电
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_PowerUp(UINT32 u32Chn)
{
    UINT32 u32I2c = ADV_GetI2c(u32Chn);
    UINT8 u8Val = 0;

    ADV_CLEAR_REG_MASK(u32I2c, ADV_I2C_IO_MAP, 0x0B, u8Val, 0x27);
    ADV_CLEAR_REG_MASK(u32I2c, ADV_I2C_IO_MAP, 0x0B, u8Val, 0x03);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SetMemory
* 描  述  : 配置存储相关
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetMemory(     UINT32 u32Chn)
{
    UINT32 u32I2c = ADV_GetI2c(u32Chn);
    UINT8 u8RegValue = 0;

    ADV_SET_REG_BIT(u32I2c, ADV_I2C_SDPIO_MAP, 0x29, u8RegValue, 4);
    ADV_CLEAR_REG_MASK(u32I2c, ADV_I2C_SDP_MAP, 0x12, u8RegValue, 0x05);
    ADV_SET_REG_BIT(u32I2c, ADV_I2C_SDPIO_MAP, 0x39, u8RegValue, 6);
    ADV_CLEAR_REG_BIT(u32I2c, ADV_I2C_SDPIO_MAP, 0x0E, u8RegValue, 4);
    
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_HDMI_SetHPA
* 描  述  : 配置HDMI hot plug assert
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_HDMI_SetHPA(UINT32 u32Chn, ADV_HDMI_PORT_E enPort)
{
    UINT32 u32I2c  = ADV_GetI2c(u32Chn);
    UINT8 u8RegVal = 0;

    /* 检测到线缆接入后，延时200ms后HPA引脚输出检测信号 */
    ADV_SET_REG_MASK(u32I2c, ADV_I2C_HDMI_MAP, 0x69, u8RegVal, 0x21);

    ADV_SET_REG_BIT(u32I2c, ADV_I2C_IO_MAP, 0x20, u8RegVal, 5);
    SAL_msleep(1);
    ADV_CLEAR_REG_BIT(u32I2c, ADV_I2C_IO_MAP, 0x20, u8RegVal, 5);
    SAL_msleep(2000);
    ADV_SET_REG_BIT(u32I2c, ADV_I2C_IO_MAP, 0x20, u8RegVal, 5);
    SAL_msleep(1);
    ADV_CLEAR_REG_BIT(u32I2c, ADV_I2C_HDMI_MAP, 0x69, u8RegVal, 0);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_HDMI_SetEdid
* 描  述  : 配置HDMI的EDID信息
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_HDMI_SetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff)
{
    UINT32 u32I2c = ADV_GetI2c(u32Chn);
    UINT32 i      = 0;

    if (NULL == pstEdidBuff)
    {
        ADV_LOGE("adv chn[%u] edid buff is null\n", u32Chn);
        return SAL_FAIL;
    }

    /* edid一个block为128字节 */
    if ((0 == pstEdidBuff->u32Len) || (0 != (pstEdidBuff->u32Len % 128)))
    {
        ADV_LOGE("chn[%u] edid value num[%u] is invalid\n", u32Chn, pstEdidBuff->u32Len);
        return SAL_FAIL;
    }

    for (i = 0; i < pstEdidBuff->u32Len; i++)
    {
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_EDID_MAP, i, pstEdidBuff->au8Buff[i]);
    }

    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x77, 0x00);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x70, 0x20);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x71, 0x00);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x72, 0x30);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x73, 0x00);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x74, 0x40);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x75, 0x00);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x76, 0x9E);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_REPEATER_MAP, 0x77, 0x0F);

    SAL_msleep(100);

    (VOID)ADV_HDMI_SetHPA(u32Chn, genAdvHdmiPort);
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : ADV_VGA_SetEdid
* 描  述  : 配置VGA的EDID信息
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_VGA_SetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff)
{
    UINT32 i      = 0;
    INT32 s32Ret  = SAL_SOK;
    IIC_DEV_S stEdidIIC;

    if (NULL == pstEdidBuff)
    {
        ADV_LOGE("adv chn[%u] edid buff is null\n", u32Chn);
        return SAL_FAIL;
    }

    /* edid一个block为128字节 */
    if ((0 == pstEdidBuff->u32Len) || (0 != (pstEdidBuff->u32Len % 128)))
    {
        ADV_LOGE("chn[%u] edid value num[%u] is invalid\n", u32Chn, pstEdidBuff->u32Len);
        return SAL_FAIL;
    }

    if (SAL_SOK != HARDWARE_GetInputEdidIIC(u32Chn, &stEdidIIC))
    {
        ADV_LOGE("adv chn[%u] get edid eeprom fail\n", u32Chn);
        return SAL_FAIL;
    }

    for (i = 0; i < pstEdidBuff->u32Len; i++)
    {
        s32Ret = IIC_Write(stEdidIIC.u32IIC, stEdidIIC.u32DevAddr, i, pstEdidBuff->au8Buff[i]);
        if (SAL_SOK != s32Ret)
        {
            ADV_LOGE("chn[%u] IIC[%u] write 0x%x to 0x%x error\n", u32Chn, stEdidIIC.u32IIC, pstEdidBuff->au8Buff[i], i);
            return SAL_FAIL;
        }

        /* EEPROM写的时间在3-4ms之间 */
        SAL_msleep(5);
    }

    ADV_LOGI("chn[%u] write to EEPROM[%u] success\n", u32Chn, stEdidIIC.u32IIC);
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : ADV_SetEdid
* 描  述  : 配置EDID信息
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff, CAPT_CABLE_E enCable)
{
    if ((NULL == pstEdidBuff) || (enCable >= CAPT_CABLE_BUTT) || (CAPT_CABLE_DVI == enCable))
    {
        ADV_LOGW("adv chn[%u] null\n", u32Chn);
        return SAL_FAIL;
    }

    if (CAPT_CABLE_HDMI == enCable)
    {
        (VOID)ADV_HDMI_SetEdid(u32Chn, pstEdidBuff);
    }
    else
    {
        (VOID)ADV_VGA_SetEdid(u32Chn, pstEdidBuff);
    }

    ADV_LOGI("adv chn[%u] set edid success\n", u32Chn);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_HDMI_SetPort
* 描  述  : 配置7842 hdmi输入的port口
* 输  入  : UINT32 u32Chn
*       : ADV_HDMI_PORT_E enPort
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_HDMI_SetPort(UINT32 u32Chn, ADV_HDMI_PORT_E enPort)
{
    UINT32 u32I2c  = ADV_GetI2c(u32Chn);
    UINT8 u8RegVal = 0;

    u8RegVal = (ADV_HDMI_PORT_A == enPort) ? 0x32 : 0x13;
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_HDMI_MAP, 0x00, u8RegVal);
    ADV_SET_REG_BIT(u32I2c, ADV_I2C_HDMI_MAP, 0x01, u8RegVal, 7);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_EnableInterrupt
* 描  述  : 使能中断
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_EnableInterrupt(UINT32 u32Chn)
{
    UINT32 u32I2c  = ADV_GetI2c(u32Chn);

    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x44, 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x6C, 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x40, 0x51);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x46, 0x10);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x6E, 0x33);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SetCsc
* 描  述  : 配置颜色空间转换参数
* 输  入  : UINT32 u32Chn
*         VIDEO_CSC_S *pstCsc
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetCscReg(UINT32 u32Chn, VIDEO_CSC_S *pstCsc)
{
    UINT32 u32I2c  = ADV_GetI2c(u32Chn);
    UINT8 u8Tmp = 0;
    UINT32 u32Tmp = 0;

    ADV_SET_REG_BIT(u32I2c, ADV_I2C_CP_MAP, 0x69, u8Tmp, 4);
    ADV_SET_REG_BIT(u32I2c, ADV_I2C_CP_MAP, 0x3E, u8Tmp, 7);
    
    /* 对比度 */
    u32Tmp = pstCsc->uiContrast * 255 / 100;
    u8Tmp = u32Tmp > 255 ? 255 : u32Tmp;
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x3A, u8Tmp);

    /* 饱和度 */
    u32Tmp = pstCsc->uiSatuature * 255 / 100;
    u8Tmp = u32Tmp > 255 ? 255 : u32Tmp;
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x3B, u8Tmp);

    /* 亮度 */
    /* 避免全黑，亮度最小值为10 */
    if (pstCsc->uiLuma < 10)
    {
        pstCsc->uiLuma = 10;
    }
    
    if (pstCsc->uiLuma < 50)
    {
        u32Tmp = pstCsc->uiLuma * 127 / 50;
        u8Tmp  = 0x80 | (UINT8)u32Tmp;
    }
    else if (pstCsc->uiLuma > 50)
    {
        if (pstCsc->uiLuma > 100)
        {
            pstCsc->uiLuma = 100;
        }

        u32Tmp = 127 - (100 - pstCsc->uiLuma) * 127 / 50;
        u8Tmp = u32Tmp;
    }
    else
    {
        u8Tmp = 0;
    }
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x3C, (UINT8)u8Tmp);

    /* 色度 */
    if (pstCsc->uiHue < 50)
    {
        u32Tmp = pstCsc->uiHue * 127 / 50;
        u8Tmp  = 0x80 | (UINT8)u32Tmp;
    }
    else if (pstCsc->uiHue > 50)
    {
        if (pstCsc->uiHue > 100)
        {
            pstCsc->uiHue = 100;
        }

        u32Tmp = 127 - (100 - pstCsc->uiHue) * 127 / 50;
        u8Tmp = u32Tmp;
    }
    else
    {
        u8Tmp = 0;
    }
    //u32Tmp = pstCsc->uiHue * 255 / 100;
    //u8Tmp = u32Tmp > 255 ? 255 : u32Tmp;
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x3D, u8Tmp);

    ADV_LOGI("adv[%u] iic[%u] set luma[%u] hue[%u] contrast[%u] satuature[%u] success\n", 
             u32Chn, u32I2c, pstCsc->uiLuma, pstCsc->uiHue, pstCsc->uiContrast, pstCsc->uiSatuature);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : ADV_SetCsc
* 描  述  : 配置CSC参数
* 输  入  : UINT32 u32Chn
          VIDEO_CSC_S *pstCsc
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 ADV_SetCsc(UINT32 u32Chn, VIDEO_CSC_S *pstCsc)
{
    if (NULL == pstCsc)
    {
        ADV_LOGE("chn[%u] csc is null\n", u32Chn);
		return SAL_FAIL;
    }

    memcpy(gastCsc + u32Chn, pstCsc, sizeof(VIDEO_CSC_S));
    return ADV_SetCscReg(u32Chn, gastCsc + u32Chn);
}

/*******************************************************************************
* 函数名  : ADV_SetInputStandard
* 描  述  : 配置输入
* 输  入  : UINT32 u32Chn
*         CAPT_CABLE_E enCableType : hdmi接入还是vga接入
*         BT_TIMING_MAP_S *pstTiming : 输入时序
* 输  出  : BOOL *pbAutoGraph : auto graphics mode
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetInputStandard(UINT32 u32Chn, CAPT_CABLE_E enCableType, const BT_TIMING_MAP_S *pstTiming, BOOL *pbAutoGraph)
{
    /* VGA从autographics切换到7842默认支持的分辨率时需要复位，为了减少复位，将默认支持的分辨率按照autographics模式来配置 */

    /* VGA对应时序的寄存器配置，参考7842数据手册 */
    static const ADV_REG_TIMING_MAP_S aenVgaTimings[] = {
        //{TIMING_VESA_800X600P60,          0x01},
        //{TIMING_VESA_1280X1024P60,        0x05},
        //{TIMING_VESA_1280X1024P75,        0x06},
        //{TIMING_VESA_640X480P60,          0x08},
        //{TIMING_VESA_1024X768P60,         0x0C},
        //{TIMING_VESA_1024X768P75,         0x0E},
        //{TIMING_VESA_1366X768P60,         0x13},
        //{TIMING_VESA_1600X1200P60,        0x16},
        //{TIMING_CVT_1600X1200P60_RB,      0x17},
    };
    
    /* HDMI对应时序的寄存器配置，参考7842数据手册 */
    static const ADV_REG_TIMING_MAP_S aenHdmiTimings[] = {
        //{TIMING_VESA_800X600P60,          0x01},
        //{TIMING_VESA_1280X1024P60,        0x05},
        //{TIMING_VESA_1280X1024P75,        0x06},
        //{TIMING_VESA_640X480P60,          0x08},
        //{TIMING_VESA_1024X768P60,         0x0C},
        //{TIMING_VESA_1024X768P75,         0x0E},
    };

    UINT32 u32I2c                = ADV_GetI2c(u32Chn);
    UINT32 u32StdNum             = 0;
    UINT32 i                     = 0;
    UINT8 u8RegVal               = 0;
    const ADV_REG_TIMING_MAP_S *pstMap = NULL;

    /* 无视频接入的时候，无需配置 */
    if (enCableType >= CAPT_CABLE_BUTT)
    {
        ADV_LOGE("no cable access\n");
        return SAL_FAIL;
    }

    if (NULL == pstTiming)
    {
        ADV_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    pstMap    = (CAPT_CABLE_HDMI == enCableType) ? aenHdmiTimings : aenVgaTimings;
    u32StdNum = (CAPT_CABLE_HDMI == enCableType) ? sizeof(aenHdmiTimings)/sizeof(aenHdmiTimings[0])
                                                     : sizeof(aenVgaTimings)/sizeof(aenVgaTimings[0]);

    for (i = 0; i < u32StdNum; i++, pstMap++)
    {
        if (pstTiming->enTiming == pstMap->enTiming)
        {
            u8RegVal     = pstMap->u8RegVal;
            *pbAutoGraph = SAL_FALSE;
            break;
        }
    }

    /* 不支持当前时序，需要手动配置 */
    if (i == u32StdNum)
    {
        /* auto graphics mode */
        u8RegVal     = (CAPT_CABLE_HDMI == enCableType) ? 0x10 : 0x07;
        *pbAutoGraph = SAL_TRUE;
    }

    /* 设置VID_STD */
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x00, u8RegVal);

    /* 设置PRIM_MODE */
    u8RegVal = (CAPT_CABLE_HDMI == enCableType) ? 0x06 : 0x82;
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x01, u8RegVal);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SetInputTiming
* 描  述  : 配置输入
* 输  入  : UINT32 u32Chn
*         CAPT_CABLE_E enCableType : hdmi接入还是vga接入
*         BT_TIMING_MAP_S *pstTiming : 输入时序
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetInputTiming(UINT32 u32Chn, CAPT_CABLE_E enCableType, const BT_TIMING_MAP_S *pstTiming)
{
    UINT32 u32I2c  = ADV_GetI2c(u32Chn);
    UINT32 u32Tmp  = 0;
    UINT8 u8RegVal = 0;

    SAL_UNUSED(enCableType);

    /* PLL */
    u32Tmp = BT_GetHTotal(&pstTiming->stTiming);
    u8RegVal = 0xC0 | (u32Tmp >> 8);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x16, u8RegVal);
    u8RegVal = (UINT8)(u32Tmp & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_IO_MAP, 0x17, u8RegVal);

    /* 一行的时钟数 */
    u32Tmp = ADV_COLOK_PERI0D / pstTiming->stTiming.u32HFreq;
    ADV_SET_REG_BITS(u32I2c, ADV_I2C_CP_MAP, 0x8F, u8RegVal, 0, 3, (UINT8)((u32Tmp >> 8) & 0x07));
    u8RegVal = (UINT8)(u32Tmp & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x90, u8RegVal);

    /* SAV EAV */
    u32Tmp = pstTiming->stTiming.u32HBackPorch +  pstTiming->stTiming.u32HSync - 4;     /* 同步码4个字节 */
    u8RegVal = (UINT8)((u32Tmp >> 8) & 0x01F);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x26, u8RegVal);
    u8RegVal = (UINT8)(u32Tmp & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x27, u8RegVal);

    u32Tmp += (pstTiming->stTiming.u32Width + 4);
    u8RegVal = (UINT8)((u32Tmp >> 8) & 0x01F);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x28, u8RegVal);
    u8RegVal = (UINT8)(u32Tmp & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x29, u8RegVal);
    
    /* 逐行 */
    ADV_CLEAR_REG_BIT(u32I2c, ADV_I2C_CP_MAP, 0x91, u8RegVal, 6);

    /* VBI */
    u32Tmp = pstTiming->stTiming.u32VBackPorch + pstTiming->stTiming.u32VSync;
    u32Tmp = SAL_align(u32Tmp, 2);
    ADV_SET_REG_BITS(u32I2c, ADV_I2C_CP_MAP, 0xA6, u8RegVal, 0, 4, (UINT8)((u32Tmp >> 8) & 0x0F));
    u8RegVal = (UINT8)(u32Tmp & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0xA7, u8RegVal);

    u32Tmp += pstTiming->stTiming.u32Height;
    u8RegVal = (UINT8)((u32Tmp >> 4) & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0xA5, u8RegVal);
    ADV_SET_REG_BITS(u32I2c, ADV_I2C_CP_MAP, 0xA6, u8RegVal, 4, 4, (UINT8)((u32Tmp & 0x0F)));

    /* CP_LCOUNT_MAX */
    u32Tmp = BT_GetVTotal(&pstTiming->stTiming);
    u8RegVal = (UINT8)((u32Tmp >> 4) & 0xFF);
    ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0xAB, u8RegVal);
    ADV_SET_REG_BITS(u32I2c, ADV_I2C_CP_MAP, 0xAC, u8RegVal, 4, 4, (UINT8)((u32Tmp & 0x0F)));

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SetOutputTiming
* 描  述  : 配置输出时序相关
* 输  入  : UINT32 u32Chn
*         CAPT_CABLE_E enCableType : hdmi接入还是vga接入
*         BT_TIMING_MAP_S *pstTiming : 输入时序
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetOutputTiming(UINT32 u32Chn, CAPT_CABLE_E enCableType, const BT_TIMING_MAP_S *pstTiming)
{
    UINT32 u32I2c  = ADV_GetI2c(u32Chn);

    SAL_UNUSED(pstTiming);

    if (CAPT_CABLE_HDMI == enCableType)
    {
        
    }
    else if (CAPT_CABLE_VGA == enCableType)
    {
        /* HS */
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x7C, 0xC3);
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x7D, 0xFF);
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x7E, 0x00);

        /* VS */
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x7F, 0x00);

        /* DE */
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x8B, 0x00);
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x8C, 0x00);
        ADV_WRITE_AND_RETURN(u32I2c, ADV_I2C_CP_MAP, 0x8D, 0x00);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_SetConfig
* 描  述  : 配置输入
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_SetConfig(UINT32 u32Chn, CAPT_CABLE_E enCableType, const BT_TIMING_MAP_S *pstTiming)
{
    INT32 s32Ret = SAL_SOK;
    BOOL bAutoGrap = SAL_FALSE;

    if (NULL == pstTiming)
    {
        ADV_LOGE("chn[%u] invalid timing\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = ADV_SetInputStandard(u32Chn, enCableType, pstTiming, &bAutoGrap);
    if (SAL_SOK != s32Ret)
    {
        ADV_LOGE("chn[%u] set input standard fail:0x%X\n", u32Chn, s32Ret);
        return s32Ret;
    }
    
    if ((SAL_TRUE == bAutoGrap) && (CAPT_CABLE_VGA == enCableType))
    {
        s32Ret = ADV_SetInputTiming(u32Chn, enCableType, pstTiming);
        if (SAL_SOK != s32Ret)
        {
            ADV_LOGE("chn[%u] set input timing fail:0x%X\n", u32Chn, s32Ret);
            return s32Ret;
        }
    }

    s32Ret = ADV_SetOutputTiming(u32Chn, enCableType, pstTiming);
    if (SAL_SOK != s32Ret)
    {
        ADV_LOGE("chn[%u] set output timing fail:0x%X\n", u32Chn, s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : ADV_Reset
* 描  述  : 复位7842
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 ADV_Reset(UINT32 u32Chn)
{
    if (NULL == g_apstEdidBuff[u32Chn])
    {
        if (NULL == (g_apstEdidBuff[u32Chn] = EDID_GetEdid(u32Chn, CAPT_CABLE_HDMI)))
        {
            CAPT_LOGW("chn[%u] get edid buff fail\n", u32Chn);
        }
    }

    (VOID)ADV_HardReset(u32Chn);
    (VOID)ADV_SoftReset(u32Chn);
    (VOID)ADV_SetI2cMap(u32Chn);
    (VOID)ADV_HDMI_SetPort(u32Chn, genAdvHdmiPort);    
    (VOID)ADV_HDMI_SetEdid(u32Chn, g_apstEdidBuff[u32Chn]);
    (VOID)ADV_PowerUp(u32Chn);
    (VOID)ADV_SetMemory(u32Chn);
    (VOID)ADV_SetCsc(u32Chn, gastCsc + u32Chn);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : ADV_Detect
* 描  述  : 检测输入并配置
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 ADV_Detect(UINT32 u32Chn, CAPT_STATUS_S *pstStatus)
{
    INT32 s32Ret = SAL_SOK;
    CAPT_CABLE_E enLastCable = CAPT_CABLE_BUTT;
    CAPT_CABLE_E enCable = CAPT_CABLE_BUTT;
    ADV_HDMI_PORT_E enHdmiPort = ADV_HDMI_PORT_BUTT;
    BOOL bVideoDetected = SAL_FALSE;
    ADV_STDI_S stStdiResult = {0};
    const BT_TIMING_MAP_S *pstBtTimging = NULL;
    ADV_DETECT_STATUS_S *pstCurrentStatus = gastCurrentStatus + u32Chn;
    BT_TIMING_E enDetectTiming    = TIMING_BUTT;
    UINT32 u32HdmiWidth  = 0;
    UINT32 u32HdmiHeight = 0;

    if (NULL == pstStatus)
    {
        ADV_LOGE("adv chn[%u] null pointer\n", u32Chn);
        return SAL_FAIL;
    }

    (VOID)ADV_EnableInterrupt(u32Chn);

    s32Ret = ADV_DetectCable(u32Chn, &enCable, &enHdmiPort, &bVideoDetected, &stStdiResult);
    if (NULL != pstStatus)
    {
        pstStatus->stRes.u32Width  = 0;
        pstStatus->stRes.u32Height = 0;
        pstStatus->stRes.u32Fps    = 0;
        pstStatus->bVideoDetected  = bVideoDetected;
        pstStatus->enCable         = enCable;
    }

    enLastCable = pstCurrentStatus->enCable;

    /* 在HDMI和VGA之间相互切换时，复位adv7842 */
    if (enLastCable != enCable)
    {
        ADV_LOGW("adv[%u] change cable[%s] to [%s]\n", u32Chn, gaszCableName[enLastCable], gaszCableName[enCable]);
        pstCurrentStatus->enCable  = enCable;
        pstCurrentStatus->enTiming = TIMING_BUTT;

        /* VGA和无输入之间切换不用复位 */
        if ((CAPT_CABLE_HDMI == enLastCable) || (CAPT_CABLE_HDMI == enCable))
        {
            (VOID)ADV_Reset(u32Chn);

            if (CAPT_CABLE_HDMI == enCable)
            {
                (VOID)ADV_WriteRegArray(u32Chn, gastHdmiDefault, sizeof(gastHdmiDefault)/sizeof(gastHdmiDefault[0]));
            }
            else
            {
                (VOID)ADV_WriteRegArray(u32Chn, gastVgaDefault, sizeof(gastVgaDefault)/sizeof(gastVgaDefault[0]));
            }
            return SAL_SOK;
        }
    }
    
    if (SAL_SOK != s32Ret)
    {
        ADV_LOGE("adv chn[%u] detect fail:0x%X\n", u32Chn, s32Ret);
        pstCurrentStatus->enTiming = TIMING_BUTT;
        return SAL_FAIL;
    }

    if (CAPT_CABLE_HDMI == enCable)
    {
        /* 使用HDMI宽高校验时序不是必选项，允许出错 */
        (VOID)ADV_HDMI_GetRes(u32Chn, &u32HdmiWidth, &u32HdmiHeight);
    }

    if ((SAL_TRUE == bVideoDetected) && (CAPT_CABLE_BUTT != enCable))
    {
        s32Ret = ADV_STDI2Timing(u32HdmiWidth, u32HdmiHeight, &stStdiResult, &pstBtTimging);
        if (SAL_SOK != s32Ret)
        {
            /* 不支持的时序不返回错误 */
            pstCurrentStatus->enTiming = TIMING_BUTT;
            ADV_LOGW("adv chn[%u] unsupport timing: bl[%u], lcf[%u] lcvs[%u]\n",
                     u32Chn, stStdiResult.u16BL, stStdiResult.u16LCF, stStdiResult.u16LCVS);
            return SAL_SOK;
        }

        enDetectTiming = pstBtTimging->enTiming;

        /* 大于支持的输入最大像素点时钟 */
        if (pstBtTimging->stTiming.u32PixelClock > ADV_INPUT_PIXEL_CLOCK_MAX)
        {
            enDetectTiming = TIMING_BUTT;
            ADV_LOGD("adv chn[%u] input video pixel clock is too big[%u]\n", u32Chn, pstBtTimging->stTiming.u32PixelClock);
        }

        if ((NULL != pstStatus) && (TIMING_BUTT != enDetectTiming))
        {
            pstStatus->stRes.u32Width  = pstBtTimging->stTiming.u32Width;
            pstStatus->stRes.u32Height = pstBtTimging->stTiming.u32Height;
            pstStatus->stRes.u32Fps    = pstBtTimging->stTiming.u32Fps;
        }

        /* 输入时序发生变化 */
        if (enDetectTiming != pstCurrentStatus->enTiming)
        {
            pstCurrentStatus->enTiming = enDetectTiming;

            /* VGA从autographics切换到7842默认支持的分辨率时需要复位，为了减少复位，将默认支持的分辨率按照autographics模式来配置 */
            /* 另参考ADV_SetInputStandard函数 */
            /*
            (VOID)ADV_Reset(u32Chn);

            if (CAPT_CABLE_HDMI == enCable)
            {
                (VOID)ADV_WriteRegArray(u32Chn, gastHdmiDefault, sizeof(gastHdmiDefault)/sizeof(gastHdmiDefault[0]));
            }
            else
            {
                (VOID)ADV_WriteRegArray(u32Chn, gastVgaDefault, sizeof(gastVgaDefault)/sizeof(gastVgaDefault[0]));
            }
            */

            if (TIMING_BUTT != enDetectTiming)
            {
                ADV_LOGW("adv[%u] change timing to:%s\n", u32Chn, pstBtTimging->szName);
                (VOID)ADV_SetConfig(u32Chn, enCable, pstBtTimging);
            }
            
            (VOID)ADV_EnableInterrupt(u32Chn);
        }
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : ADV_Init
* 描  述  : 配置输入
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 ADV_Init(UINT32 u32Chn)
{
    (VOID)ADV_Reset(u32Chn);

    if (CAPT_CABLE_HDMI == gastCurrentStatus[u32Chn].enCable)
    {
        (VOID)ADV_WriteRegArray(u32Chn, gastHdmiDefault, sizeof(gastHdmiDefault)/sizeof(gastHdmiDefault[0]));
    }
    else
    {
        (VOID)ADV_WriteRegArray(u32Chn, gastVgaDefault, sizeof(gastVgaDefault)/sizeof(gastVgaDefault[0]));
    }

    ADV_LOGI("adv chn[%u] init success\n", u32Chn);

    return SAL_SOK;
}

CAPT_CHIP_FUNC_S *ADV_chipRegister(void)
{

    memset(&g_advExtChipFunc, 0, sizeof(CAPT_CHIP_FUNC_S));
    g_advExtChipFunc.pfuncInit       = ADV_Init;
    g_advExtChipFunc.pfuncHotPlug    = NULL;
    g_advExtChipFunc.pfuncReset      = NULL;
    g_advExtChipFunc.pfuncDetect     = ADV_Detect;
    g_advExtChipFunc.pfuncSetRes     = NULL;
    g_advExtChipFunc.pfuncSetCsc     = ADV_SetCsc;
    g_advExtChipFunc.pfuncAutoAdjust = NULL;
    g_advExtChipFunc.pfuncGetEdid    = NULL;
    g_advExtChipFunc.pfuncSetEdid    = ADV_SetEdid;
    g_advExtChipFunc.pfuncScale      = NULL;
    g_advExtChipFunc.pfuncUpdate      = NULL;

    return &g_advExtChipFunc;
}




