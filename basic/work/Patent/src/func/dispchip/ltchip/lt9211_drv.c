/*******************************************************************************
* lt9211.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangzhenya5
* Version: V1.0.0  2021年05月10日 Create
*
* Description : lt9211配置,MIPI转LVDS
* Modification:
*******************************************************************************/

/* ========================================================================== */
/*                              头文件区                                          */
/* ========================================================================== */
#include "platform_hal.h"
#include "sal.h"
#include "lt9211_drv.h"

#line __LINE__ "lt9211_drv.c"


/* ========================================================================== */
/*                           宏和类型定义区                                          */
/* ========================================================================== */

#define  MIPI_LANE_CNT       MIPI_4_LANE /* 0: 4lane */
#define  MIPI_SETTLE_VALUE   0x0D        /* 0x05  0x0a */
#define  _Mipi_PortA_
#define  LT9211_OutPutModde  LT9211_OUTPUT_LVDS_2_PORT //OUTPUT_BT656_8BIT
#define  Video_Input_Mode    INPUT_RGB888


/* ========================================================================== */
/*                           数据结构定义区                                          */
/* ========================================================================== */

typedef enum 
{
    LT9211_OUTPUT_RGB888         =0,
    LT9211_OUTPUT_BT656_8BIT     =1,
    LT9211_OUTPUT_BT1120_16BIT   =2,
    LT9211_OUTPUT_LVDS_2_PORT    =3,
	LT9211_OUTPUT_LVDS_1_PORT    =4,
    LT9211_OUTPUT_YCbCr444       =5,
    LT9211_OUTPUT_YCbCr422_16BIT =6
}LT9211_OUTPUTMODE_ENUM;

typedef enum VIDEO_INPUTMODE_ENUM
{
    INPUT_RGB888          =1,
    INPUT_YCbCr444        =2,
    INPUT_YCbCr422_16BIT  =3
}Video_Input_Mode_TypeDef;

typedef enum  _MIPI_LANE_NUMBER
{
    MIPI_1_LANE = 1,
    MIPI_2_LANE = 2,
    MIPI_3_LANE = 3,
    MIPI_4_LANE = 0   //default 4Lane
} MIPI_LANE_NUMBER__TypeDef;


typedef struct
{
    UINT8    u8IICBusIdx;        //iic总线号
    UINT8    u16ChipAddr;        //LT9211芯片设备地址

} LT9211_BOARD_INFO_S;


static LT9211_BOARD_INFO_S  g_stLT9211BoardInfo = {0};


/* ========================================================================== */
/*                             函数声明区                                          */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : LT9211_GetBoardInfo
* 描  述  : 获取lt9211的iic总线号和设备地址
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT9211_GetBoardInfo(UINT8 u8BusIdx, UINT8 u8ChipAddr)
{
    g_stLT9211BoardInfo.u8IICBusIdx = u8BusIdx;
    g_stLT9211BoardInfo.u16ChipAddr = u8ChipAddr;
}


/*******************************************************************************
* 函数名  : LT9211_WriteI2C_Byte
* 描  述  : 向制定寄存器中写入参数
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/

static INT32 LT9211_WriteI2C_Byte(UINT32 u32Reg, UINT8 u8Data)
{
    INT32  s32ret = SAL_FAIL;
    UINT8  u8IICID;
    UINT16 u16ChipAddr;
    
    u8IICID     = g_stLT9211BoardInfo.u8IICBusIdx;
    u16ChipAddr = g_stLT9211BoardInfo.u16ChipAddr;
    s32ret = IIC_Write(u8IICID, u16ChipAddr, u32Reg, u8Data);
    return s32ret;
}

/*******************************************************************************
* 函数名  : LT9211_ReadI2C_Byte
* 描  述  : 向制定寄存器中写入参数
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/

static UINT8 LT9211_ReadI2C_Byte(UINT32 u32Reg)
{
    INT32  s32ret = SAL_FAIL;
    UINT8  u8Data = 0;
    UINT8  u8IICID;
    UINT16 u16ChipAddr;
    
    u8IICID     = g_stLT9211BoardInfo.u8IICBusIdx;
    u16ChipAddr = g_stLT9211BoardInfo.u16ChipAddr;
    
    s32ret = IIC_Read(u8IICID, u16ChipAddr, u32Reg, &u8Data);
    if(s32ret != SAL_SOK)
    {
        DISP_LOGE("ReadI2Cfail reg[0x%x]\n", u32Reg);
        return SAL_FAIL;
    }
    return u8Data;
}

/* lt9211 debug调试代码,先保留 */
#if 0
static UINT16 hact = 0, vact = 0;
static UINT16 hs = 0, vs = 0;
static UINT16 hbp = 0, vbp = 0;
static UINT16 htotal = 0, vtotal = 0;
static UINT16 hfp = 0, vfp = 0;

static void LT9211_VideoCheck(void)
{
    UINT8 sync_polarity;

    LT9211_WriteI2C_Byte(0xff,0x86);
    LT9211_WriteI2C_Byte(0x20,0x00);
    
    sync_polarity = LT9211_ReadI2C_Byte(0x70);
    vs = LT9211_ReadI2C_Byte(0x71);

    hs = LT9211_ReadI2C_Byte(0x72);
    hs = (hs<<8) + LT9211_ReadI2C_Byte(0x73);
    
    vbp = LT9211_ReadI2C_Byte(0x74);
    vfp = LT9211_ReadI2C_Byte(0x75);

    hbp = LT9211_ReadI2C_Byte(0x76);
    hbp = (hbp<<8) + LT9211_ReadI2C_Byte(0x77);

    hfp = LT9211_ReadI2C_Byte(0x78);
    hfp = (hfp<<8) + LT9211_ReadI2C_Byte(0x79);

    vtotal = LT9211_ReadI2C_Byte(0x7A);
    vtotal = (vtotal<<8) + LT9211_ReadI2C_Byte(0x7B);

    htotal = LT9211_ReadI2C_Byte(0x7C);
    htotal = (htotal<<8) + LT9211_ReadI2C_Byte(0x7D);

    vact = LT9211_ReadI2C_Byte(0x7E);
    vact = (vact<<8)+ LT9211_ReadI2C_Byte(0x7F);

    hact = LT9211_ReadI2C_Byte(0x80);
    hact = (hact<<8) + LT9211_ReadI2C_Byte(0x81);

    DISP_LOGE("sync_polarity = %x\n", sync_polarity);
    if(!(sync_polarity & 0x01)) //hsync
    {
        LT9211_WriteI2C_Byte(0xff,0xd8);
        LT9211_WriteI2C_Byte(0x10, (LT9211_ReadI2C_Byte(0x10)| 0x10));
    }
    if(!(sync_polarity & 0x02)) //vsync
    {
        LT9211_WriteI2C_Byte(0xff,0xd8);
        LT9211_WriteI2C_Byte(0x10, (LT9211_ReadI2C_Byte(0x10)| 0x20));
    }

    DISP_LOGE(" hfp %d, hs %d , hbp %d, hact %d, htotal = %d\n", hfp, hs, hbp, hact, htotal);
    DISP_LOGE(" vfp %d, vs %d, vbp %d, vact%d , vtotal = %d\n", vfp, vs, vbp, vact, vtotal);
}

static void LT9211_ClockCheckDebug(void)
{
    UINT32 fm_value = 0;

    LT9211_WriteI2C_Byte(0xff,0x86);
    LT9211_WriteI2C_Byte(0x00,0x01);
    SAL_msleep(250);
    fm_value = 0;
    fm_value = (LT9211_ReadI2C_Byte(0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + LT9211_ReadI2C_Byte(0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + LT9211_ReadI2C_Byte(0x0a);
    DISP_LOGE("Port A lvds clock: %u \n",fm_value);
    //SAL_LOGI(fm_value);
    
    LT9211_WriteI2C_Byte(0xff,0x86);
    LT9211_WriteI2C_Byte(0x00,0x02);
    SAL_msleep(250);
    fm_value = 0;
    fm_value = (LT9211_ReadI2C_Byte(0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + LT9211_ReadI2C_Byte(0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + LT9211_ReadI2C_Byte(0x0a);
    DISP_LOGE("Port B lvds clock: %u \n",fm_value);
    //SAL_LOGI(fm_value);

}
#endif

/*******************************************************************************
* 函数名  : LT9211_HardReset
* 描  述  : lt9211硬件复位
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT9211_HardReset(UINT32 u32Chan)
{
    INT32 s32Fd =  -1;
    INT32 s32status = 0;
    DDM_resetSet  stResetPrm;

    s32Fd = open(DDM_DEV_RESET_NAME, O_RDWR);
    if(s32Fd < 0)
    {
        DISP_LOGE("open %s failed\n", DDM_DEV_RESET_NAME);
        return SAL_FAIL;
    }

    //LT9211
    memset((char *)&stResetPrm, 0, sizeof(DDM_resetSet));
    stResetPrm.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    stResetPrm.subtype = DDM_DP_SUBTYPE_LT9211;   //二级子类型为LT9211显示模块
    stResetPrm.num = u32Chan;                            

    s32status = ioctl(s32Fd, DDM_IOC_RESET_OFFON, &stResetPrm);  //使用ioctl接口复位
    close(s32Fd);
    if (s32status < 0)
    {
        DISP_LOGE("RESET ioctl reset LT9211 failed fd %d %d!\n",s32Fd, s32status);
    }
    else
    {
        DISP_LOGI("LT9211 hard reset ok!\n");
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : LT9211_ChipID
* 描  述  : 检测Chip ID
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT9211_ChipID(VOID)
{
    UINT32 u32ChipId = 0;

    LT9211_WriteI2C_Byte(0xff, 0x81); /* register bank */
    u32ChipId |= LT9211_ReadI2C_Byte(0x00) << 16;
    u32ChipId |= LT9211_ReadI2C_Byte(0x01) << 8;
    u32ChipId |= LT9211_ReadI2C_Byte(0x02);

    //因为R7板子第一版为lt9211u4版本，ChipID为0x1801E3，后续板子会使用lt9211u5,ChipID为0x1801E4.
    if((u32ChipId == 0x1801E4) || (u32ChipId == 0x1801E3))
    {
        DISP_LOGI("ChipID check OK: 0x%x\n", u32ChipId);
        return SAL_SOK;
    }
    else
    {
        DISP_LOGE("ChipID Fail!!!!\n");
        return SAL_FAIL;
    }
}

/*******************************************************************************
* 函数名  : LT9211_SystemInt
* 描  述  : lt9211系统初始化
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT9211_SystemInt(VOID)
{
    /* system clock init */
    LT9211_WriteI2C_Byte(0xff, 0x82);
    LT9211_WriteI2C_Byte(0x01, 0x18);

    LT9211_WriteI2C_Byte(0xff, 0x86);
    LT9211_WriteI2C_Byte(0x06, 0x61);
    LT9211_WriteI2C_Byte(0x07, 0xa8); /* fm for sys_clk */

    LT9211_WriteI2C_Byte(0xff, 0x87); /* 初始化 txpll 寄存器列表默认值给错了 */
    LT9211_WriteI2C_Byte(0x14, 0x08); /* default value */
    LT9211_WriteI2C_Byte(0x15, 0x00); /* default value */
    LT9211_WriteI2C_Byte(0x18, 0x0f);
    LT9211_WriteI2C_Byte(0x22, 0x08); /* default value */
    LT9211_WriteI2C_Byte(0x23, 0x00); /* default value */
    LT9211_WriteI2C_Byte(0x26, 0x0f);
}

/*******************************************************************************
* 函数名  : LT9211_MipiRxPhy
* 描  述  : lt9211 mipi接收配置
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT9211_MipiRxPhy(VOID)
{
#if 1 /* yelsin for WYZN CUSTOMER 2Lane 9211 mipi2RGB */
    LT9211_WriteI2C_Byte(0xff, 0xd0);
    LT9211_WriteI2C_Byte(0x00, MIPI_LANE_CNT); /* 0: 4 Lane / 1: 1 Lane / 2 : 2 Lane / 3: 3 Lane */
#endif

    /* Mipi rx phy */
    LT9211_WriteI2C_Byte(0xff, 0x82);
    LT9211_WriteI2C_Byte(0x02, 0x44); /* port A mipi rx enable */
    /*port A/B input 8205/0a bit6_4:EQ current setting*/
    LT9211_WriteI2C_Byte(0x05, 0x36); /* port A CK lane swap  0x32--0x36 for WYZN Glassbit2- Port A mipi/lvds rx s2p input clk select: 1 = From outer path. */
    LT9211_WriteI2C_Byte(0x0d, 0x26); /* bit6_4:Port B Mipi/lvds rx abs refer current  0x26 0x76 */
    LT9211_WriteI2C_Byte(0x17, 0x0c);
    LT9211_WriteI2C_Byte(0x1d, 0x0c);

    LT9211_WriteI2C_Byte(0x0a, 0x81); /* eq control for LIEXIN  horizon line display issue 0xf7->0x80 */
    LT9211_WriteI2C_Byte(0x0b, 0x00); /* eq control  0x77->0x00 */
#ifdef _Mipi_PortA_
    /*port a*/
    LT9211_WriteI2C_Byte(0x07, 0x9f); /* port clk enable  （只开Portb时,porta的lane0 clk要打开） */
    LT9211_WriteI2C_Byte(0x08, 0xfc); /* port lprx enable */
#endif
#ifdef _Mipi_PortB_
    /*port a*/
    LT9211_WriteI2C_Byte(0x07, 0x9f); /* port clk enable  （只开Portb时,porta的lane0 clk要打开） */
    LT9211_WriteI2C_Byte(0x08, 0xfc); /* port lprx enable */
    /*port b*/
    LT9211_WriteI2C_Byte(0x0f, 0x9F); /* port clk enable */
    LT9211_WriteI2C_Byte(0x10, 0xfc); /* port lprx enable */
    LT9211_WriteI2C_Byte(0x04, 0xa1);
#endif
    /*port diff swap*/
    LT9211_WriteI2C_Byte(0x09, 0x01); /* port a diff swap */
    LT9211_WriteI2C_Byte(0x11, 0x01); /* port b diff swap */
    
    LT9211_WriteI2C_Byte(0xff, 0x82);
    LT9211_WriteI2C_Byte(0x23, 0x02);

    /*port lane swap*/
    LT9211_WriteI2C_Byte(0xff, 0x86);
    LT9211_WriteI2C_Byte(0x33, 0x1b); /* port a lane swap	1b:no swap */
    LT9211_WriteI2C_Byte(0x34, 0x1b); /* port b lane swap 1b:no swap */

#ifdef CSI_INPUTDEBUG
    LT9211_WriteI2C_Byte(0xff, 0xd0);
    LT9211_WriteI2C_Byte(0x04, 0x10);  /* bit4-enable CSI mode */
    LT9211_WriteI2C_Byte(0x21, 0xc6);
#endif
}

/*******************************************************************************
* 函数名  : LT9211_MipiRxDigital
* 描  述  : lt9211 mipi接收配置
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT9211_MipiRxDigital(VOID)
{
    LT9211_WriteI2C_Byte(0xff, 0x86);
#ifdef _Mipi_PortA_
    LT9211_WriteI2C_Byte(0x30, 0x85); /* mipirx HL swap */
#endif

#ifdef _Mipi_PortB_
    LT9211_WriteI2C_Byte(0x30, 0x8f); /* mipirx HL swap */
#endif
    LT9211_WriteI2C_Byte(0xff, 0xD8);
#ifdef _Mipi_PortA_
    LT9211_WriteI2C_Byte(0x16, 0x00); /* mipirx HL swap	  bit7- 0:portAinput */
#endif

#ifdef _Mipi_PortB_
    LT9211_WriteI2C_Byte(0x16, 0x80); /* mipirx HL swap bit7- portBinput */
#endif

    LT9211_WriteI2C_Byte(0xff, 0xd0);
    LT9211_WriteI2C_Byte(0x43, 0x12); /* rpta mode enable,ensure da_mlrx_lptx_en=0 */

    LT9211_WriteI2C_Byte(0x02, MIPI_SETTLE_VALUE); /* mipi rx controller	//settle值 */
#if 0 /* forWTWD yelsin0520 */
    LT9211_WriteI2C_Byte(0x0c, 0x40);     /* MIPI read delay setting */
    LT9211_WriteI2C_Byte(0x1B, 0x00);     /* PCR de mode delay.[15:8] */
    LT9211_WriteI2C_Byte(0x1c, 0x40);     /* PCR de mode delay.[7:0] */
    LT9211_WriteI2C_Byte(0x24, 0x70);
    LT9211_WriteI2C_Byte(0x25, 0x40);
 #endif
}


/*******************************************************************************
* 函数名: LT9211_SetVideoTiming
* 描  述  : lt9211 视频时序设置
* 输  入  : const BT_TIMING_S *pstTiming
* 输  出  : 
* 返回值: SAL_SOK  : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT9211_SetVideoTiming(const BT_TIMING_S *pstTiming)
{
    UINT32 u32Htotal;
    UINT32 u32Vtotal;
    
    u32Htotal = pstTiming->u32Width  + pstTiming->u32HBackPorch + pstTiming->u32HFrontPorch + pstTiming->u32HSync;
    u32Vtotal = pstTiming->u32Height + pstTiming->u32VBackPorch + pstTiming->u32VFrontPorch + pstTiming->u32VSync;
    
    LT9211_WriteI2C_Byte(0xff, 0xd0);
    LT9211_WriteI2C_Byte(0x0d, (UINT8)(u32Vtotal >> 8));                 /* vtotal[15:8]  */
    LT9211_WriteI2C_Byte(0x0e, (UINT8)(u32Vtotal));                      /* vtotal[7:0]   */
    LT9211_WriteI2C_Byte(0x0f, (UINT8)(pstTiming->u32Height >> 8));      /* vactive[15:8] */
    LT9211_WriteI2C_Byte(0x10, (UINT8)(pstTiming->u32Height));           /* vactive[7:0]  */
    LT9211_WriteI2C_Byte(0x15, (UINT8)(pstTiming->u32VSync));            /* vs[7:0]       */
    LT9211_WriteI2C_Byte(0x17, (UINT8)(pstTiming->u32VFrontPorch >> 8)); /* vfp[15:8]     */
    LT9211_WriteI2C_Byte(0x18, (UINT8)(pstTiming->u32VFrontPorch));      /* vfp[7:0]      */

    LT9211_WriteI2C_Byte(0x11, (UINT8)(u32Htotal >> 8));                 /* htotal[15:8]  */
    LT9211_WriteI2C_Byte(0x12, (UINT8)(u32Htotal));                      /* htotal[7:0]   */
    LT9211_WriteI2C_Byte(0x13, (UINT8)(pstTiming->u32Width >> 8));       /* hactive[15:8] */
    LT9211_WriteI2C_Byte(0x14, (UINT8)(pstTiming->u32Width));            /* hactive[7:0]  */
    LT9211_WriteI2C_Byte(0x16, (UINT8)(pstTiming->u32HSync));            /* hs[7:0]       */
    LT9211_WriteI2C_Byte(0x19, (UINT8)(pstTiming->u32HFrontPorch >> 8)); /* hfp[15:8]     */
    LT9211_WriteI2C_Byte(0x1a, (UINT8)(pstTiming->u32HFrontPorch));      /* hfp[7:0]      */

}

/*******************************************************************************
* 函数名: LT9211_TimingSet
* 描  述  : lt9211 视频时序设置
* 输  入  : const BT_TIMING_S *pstTiming
* 输  出  : 
* 返回值: 
*******************************************************************************/
static INT32 LT9211_TimingSet(const BT_TIMING_S *pstTiming)
{
    UINT16 u16Hact;
    UINT16 u16Vact;
    UINT8  u8Fmt;
    UINT8  u8PaLpn = 0;

    /* 延时等待前级输出时钟稳定 */
    SAL_msleep(500); /* 500-->100 */
    
    LT9211_WriteI2C_Byte(0xff, 0xd0);
    
    /* detect source hact */
    u16Hact = (LT9211_ReadI2C_Byte(0x82) << 8) + LT9211_ReadI2C_Byte(0x83);
    DISP_LOGI("source hact counter = %d \n", u16Hact);
    
    u16Hact = u16Hact / 3;
    /* MIPI video format. */
    u8Fmt     = (LT9211_ReadI2C_Byte(0x84) & 0x0f);
    /* Mipi detect vactive line number. */
    u16Vact    = (LT9211_ReadI2C_Byte(0x85) << 8) + LT9211_ReadI2C_Byte(0x86);
    /* MIPI lane 0-3 pn */
    u8PaLpn  = LT9211_ReadI2C_Byte(0x9c);
    
    DISP_LOGI("source timing : hact = %d, vact = %d, fmt = %x, PaLpn = %x \n", u16Hact, u16Vact, u8Fmt, u8PaLpn);
    DISP_LOGI("set timing : hact = %d, vact = %d, fps = %d\n", pstTiming->u32Width , pstTiming->u32Height, pstTiming->u32Fps);
    
    if ((0 == u16Hact) || (0 == u16Vact) || (0x55 != u8PaLpn))
    {
        DISP_LOGE("detect source timing:[%d*%d] error, set timing:[%d*%d]\n", u16Hact, u16Vact, pstTiming->u32Width , pstTiming->u32Height);
        return SAL_FAIL;
    }
    
    if ((u16Hact != pstTiming->u32Width) || (u16Vact != pstTiming->u32Height))
    {
        DISP_LOGE("detect source timing:[%d*%d], set timing:[%d*%d]\n", u16Hact, u16Vact, pstTiming->u32Width , pstTiming->u32Height);
        return SAL_FAIL;
    }
    
    LT9211_SetVideoTiming(pstTiming);
    return SAL_SOK;
    
}


/*******************************************************************************
* 函数名: LT9211_DesscPllMipi
* 描  述  : lt9211 时钟设置
* 输  入  : const BT_TIMING_S *pstTiming
* 输  出  : 
* 返回值: 
*******************************************************************************/
static VOID LT9211_DesscPllMipi(const BT_TIMING_S *pstTiming)
{
    UINT8  i;
    UINT32 u32PixelClk;
    UINT8  u8PllLockFlag;
    UINT8  u8PllPostDiv;
    UINT8  u8PcrM;
    
    u32PixelClk = pstTiming->u32PixelClock / 1000;
    
    DISP_LOGI(" LT9211_DesscPll: set rx pll = %u\n", u32PixelClk);

    LT9211_WriteI2C_Byte(0xff, 0x82);
    LT9211_WriteI2C_Byte(0x2d, 0x48);

    if (u32PixelClk > 80000)
    {
        LT9211_WriteI2C_Byte(0x35, 0x81);
        u8PllPostDiv = 0x01;
    }
    else if (u32PixelClk > 20000)
    {
        LT9211_WriteI2C_Byte(0x35, 0x82);
        u8PllPostDiv = 0x02;
    }
    else
    {
        LT9211_WriteI2C_Byte(0x35, 0x83);
        u8PllPostDiv = 0x04;
    }

    u8PcrM = (UINT8)((u32PixelClk * 4 * u8PllPostDiv) / 25000);
    DISP_LOGI(" LT9211_DesscPll: set rx pll pcr_m = 0x%x\n", u8PcrM);

    LT9211_WriteI2C_Byte(0xff, 0xd0);
    LT9211_WriteI2C_Byte(0x2d, 0x40);           /* M_up_limit */
    LT9211_WriteI2C_Byte(0x31, 0x10);           /* M_low_limit */
    LT9211_WriteI2C_Byte(0x26, u8PcrM | 0x80);

    LT9211_WriteI2C_Byte(0xff, 0x81);           /* dessc pll sw rst */
    LT9211_WriteI2C_Byte(0x20, 0xef);
    LT9211_WriteI2C_Byte(0x20, 0xff);

    /* pll lock status */
    for (i = 0; i < 6; i++)
    {
        LT9211_WriteI2C_Byte(0xff, 0x81);
        LT9211_WriteI2C_Byte(0x11, 0xfb);     /* pll lock logic reset */
        LT9211_WriteI2C_Byte(0x11, 0xff);

        LT9211_WriteI2C_Byte(0xff, 0x87);
        u8PllLockFlag = LT9211_ReadI2C_Byte(0x04);
        if (u8PllLockFlag & 0x01)
        {
            DISP_LOGI(" LT9211_DesscPll: dessc pll locked\n");
            break;
        }
        else
        {
            LT9211_WriteI2C_Byte(0xff, 0x81);    /* dessc pll sw rst */
            LT9211_WriteI2C_Byte(0x20, 0xef);
            LT9211_WriteI2C_Byte(0x20, 0xff);
            DISP_LOGI(" LT9211_DesscPll: dessc pll unlocked,sw reset\n");
        }
    }
}

/*******************************************************************************
* 函数名: LT9211_MipiPcr
* 描  述  : 检测 pcr是否稳定
* 输  入  : 
* 输  出  : 
* 返回值: 
*******************************************************************************/
static INT32 LT9211_MipiPcr(VOID)
{
    UINT8 u8Loop;
    UINT8 u8PcrM;

    LT9211_WriteI2C_Byte(0xff, 0xd0);
    LT9211_WriteI2C_Byte(0x0c, 0x60);  /* fifo position */
    LT9211_WriteI2C_Byte(0x1c, 0x60);  /* fifo position */
    LT9211_WriteI2C_Byte(0x24, 0x70);  /* pcr mode( de hs vs) */

    LT9211_WriteI2C_Byte(0x2d, 0x30); /* M up limit */
    LT9211_WriteI2C_Byte(0x31, 0x0a); /* M down limit */

    /*stage1 hs mode*/
    LT9211_WriteI2C_Byte(0x25, 0xf0);  /* line limit */
    LT9211_WriteI2C_Byte(0x2a, 0x30);  /* step in limit */
    LT9211_WriteI2C_Byte(0x21, 0x4f);  /* hs_step */
    LT9211_WriteI2C_Byte(0x22, 0x00);

    /*stage2 hs mode*/
    LT9211_WriteI2C_Byte(0x1e, 0x01);  /* RGD_DIFF_SND[7:4],RGD_DIFF_FST[3:0] */
    LT9211_WriteI2C_Byte(0x23, 0x80);  /* hs_step */
    /*stage2 de mode*/
    LT9211_WriteI2C_Byte(0x0a, 0x02); /* de adjust pre line */
    LT9211_WriteI2C_Byte(0x38, 0x02); /* de_threshold 1 */
    LT9211_WriteI2C_Byte(0x39, 0x04); /* de_threshold 2 */
    LT9211_WriteI2C_Byte(0x3a, 0x08); /* de_threshold 3 */
    LT9211_WriteI2C_Byte(0x3b, 0x10); /* de_threshold 4 */

    LT9211_WriteI2C_Byte(0x3f, 0x04); /* de_step 1 */
    LT9211_WriteI2C_Byte(0x40, 0x08); /* de_step 2 */
    LT9211_WriteI2C_Byte(0x41, 0x10); /* de_step 3 */
    LT9211_WriteI2C_Byte(0x42, 0x20); /* de_step 4 */

    LT9211_WriteI2C_Byte(0x2b, 0xa0); /* stable out */
    /* SAL_msleep(100); */
    LT9211_WriteI2C_Byte(0xff, 0xd0);   /* enable HW pcr_m */

    u8PcrM = LT9211_ReadI2C_Byte(0x26);
    u8PcrM &= 0x7f;
    LT9211_WriteI2C_Byte(0x26, u8PcrM);
    DISP_LOGI("LT9211 0xd026 u8PcrM %#x!!!!\n", u8PcrM);
    
    LT9211_WriteI2C_Byte(0x27, 0x0f);

    LT9211_WriteI2C_Byte(0xff, 0x81);  /* pcr reset */
    LT9211_WriteI2C_Byte(0x20, 0xbf); /* mipi portB div issue */
    LT9211_WriteI2C_Byte(0x20, 0xff);
    SAL_msleep(5);
    LT9211_WriteI2C_Byte(0x0B, 0x6F);
    LT9211_WriteI2C_Byte(0x0B, 0xFF);

    /* 延时等待复位生效 */
    SAL_msleep(800); /* 800->120 */
    for (u8Loop = 0; u8Loop < 10; u8Loop++) /* Check pcr_stable 10 */
    {
        /* 延时等待稳定再检测 */
        SAL_msleep(200);
        LT9211_WriteI2C_Byte(0xff, 0xd0);
        if (LT9211_ReadI2C_Byte(0x87) & 0x08)
        {
            DISP_LOGI("LT9211 pcr stable\n");
            break;
        }
        else
        {
            DISP_LOGW("LT9211 pcr unstable Loop %d times !!!!\n", u8Loop);
            //return SAL_FAIL;
        }
    }

    LT9211_WriteI2C_Byte(0xff, 0xd0);
    DISP_LOGI("LT9211 pcr_stable_M=%x\n", (LT9211_ReadI2C_Byte(0x94) & 0x7F)); /* 打印M值 */
    return SAL_SOK;
}

/*******************************************************************************
* 函数名: LT9211_RXCSC
* 描  述  : 色彩空间设置
* 输  入  : 
* 输  出  : 
* 返回值: 
*******************************************************************************/
static VOID LT9211_RXCSC(VOID)
{
    LT9211_WriteI2C_Byte(0xff, 0xf9);
    if (LT9211_OutPutModde == LT9211_OUTPUT_RGB888)
    {
        if (Video_Input_Mode == INPUT_RGB888)
        {
            LT9211_WriteI2C_Byte(0x86, 0x00);
            LT9211_WriteI2C_Byte(0x87, 0x00);
        }
        else if (Video_Input_Mode == INPUT_YCbCr444)
        {
            LT9211_WriteI2C_Byte(0x86, 0x0f);
            LT9211_WriteI2C_Byte(0x87, 0x00);
        }
        else if (Video_Input_Mode == INPUT_YCbCr422_16BIT)
        {
            LT9211_WriteI2C_Byte(0x86, 0x00);
            LT9211_WriteI2C_Byte(0x87, 0x03);
        }
    }
    else if ((LT9211_OutPutModde == LT9211_OUTPUT_BT656_8BIT) || (LT9211_OutPutModde == LT9211_OUTPUT_BT1120_16BIT) \
            || (LT9211_OutPutModde == LT9211_OUTPUT_YCbCr422_16BIT))
    {
        if (Video_Input_Mode == INPUT_RGB888)
        {
            LT9211_WriteI2C_Byte(0x86, 0x0f);
            LT9211_WriteI2C_Byte(0x87, 0x30);
        }
        else if (Video_Input_Mode == INPUT_YCbCr444)
        {
            LT9211_WriteI2C_Byte(0x86, 0x00);
            LT9211_WriteI2C_Byte(0x87, 0x30);
        }
        else if (Video_Input_Mode == INPUT_YCbCr422_16BIT)
        {
            LT9211_WriteI2C_Byte(0x86, 0x00);
            LT9211_WriteI2C_Byte(0x87, 0x00);
        }
    }
    else if (LT9211_OutPutModde == LT9211_OUTPUT_YCbCr444)
    {
        if (Video_Input_Mode == INPUT_RGB888)
        {
            LT9211_WriteI2C_Byte(0x86, 0x0f);
            LT9211_WriteI2C_Byte(0x87, 0x00);
        }
        else if (Video_Input_Mode == INPUT_YCbCr444)
        {
            LT9211_WriteI2C_Byte(0x86, 0x00);
            LT9211_WriteI2C_Byte(0x87, 0x00);
        }
        else if (Video_Input_Mode == INPUT_YCbCr422_16BIT)
        {
            LT9211_WriteI2C_Byte(0x86, 0x00);
            LT9211_WriteI2C_Byte(0x87, 0x03);
        }
    }
}

/*******************************************************************************
* 函数名: LT9211_Txpll
* 描  述  : 输出pll设置
* 输  入  : 
* 输  出  : 
* 返回值: 
*******************************************************************************/
static VOID LT9211_Txpll(VOID)
{
    UINT8  u8Loop;

    LT9211_WriteI2C_Byte(0xff, 0x82);
    LT9211_WriteI2C_Byte(0x36, 0x01); /* b7:txpll_pd */
    LT9211_WriteI2C_Byte(0x37, 0x2a);
    LT9211_WriteI2C_Byte(0x38, 0x06);
    LT9211_WriteI2C_Byte(0x39, 0x30);
    LT9211_WriteI2C_Byte(0x3a, 0x8e);
    LT9211_WriteI2C_Byte(0xff, 0x87);
    LT9211_WriteI2C_Byte(0x37, 0x14);
    LT9211_WriteI2C_Byte(0x13, 0x00);
    LT9211_WriteI2C_Byte(0x13, 0x80);
    /* 延时等待配置生效 */
    SAL_msleep(100);
    
    for (u8Loop = 0; u8Loop < 10; u8Loop++) /* Check Tx PLL cal */
    {

        LT9211_WriteI2C_Byte(0xff, 0x87);
        if (LT9211_ReadI2C_Byte(0x1f) & 0x80) /* Output TXPLL vco cur_sel calibration done status */
        {
            if (LT9211_ReadI2C_Byte(0x20) & 0x80) /* Output TXPLL lock status */
            {
                DISP_LOGI("LT9211 tx pll lock\n");
            }
            else
            {
                DISP_LOGI("LT9211 tx pll unlocked\n");
            }

            DISP_LOGI("LT9211 tx pll cal done\n");
            break;
        }
        else
        {
            DISP_LOGI("LT9211 tx pll unlocked\n");
        }
    }
}

/*******************************************************************************
* 函数名: LT9211_TxPhy
* 描  述  : 输出模式设置
* 输  入  : 
* 输  出  : 
* 返回值: 
*******************************************************************************/
static VOID LT9211_TxPhy(VOID)
{
    LT9211_WriteI2C_Byte(0xff, 0x82);
    if ((LT9211_OutPutModde == LT9211_OUTPUT_RGB888) || (LT9211_OutPutModde == LT9211_OUTPUT_BT656_8BIT) || (LT9211_OutPutModde == LT9211_OUTPUT_BT1120_16BIT))
    {
        LT9211_WriteI2C_Byte(0x62, 0x01); /* ttl output enable */
        LT9211_WriteI2C_Byte(0x63, 0x00); /* add for U5 */
        LT9211_WriteI2C_Byte(0x6b, 0xff);
    }
    else if ((LT9211_OutPutModde == LT9211_OUTPUT_LVDS_2_PORT) || (LT9211_OutPutModde == LT9211_OUTPUT_LVDS_1_PORT))
    {
        /* dual-port lvds tx phy */
        LT9211_WriteI2C_Byte(0x62, 0x00); /* ttl output disable */
        if (LT9211_OutPutModde == LT9211_OUTPUT_LVDS_2_PORT)
        {
            LT9211_WriteI2C_Byte(0x3b, 0xb8);
        }
        else
        {
            LT9211_WriteI2C_Byte(0x3b, 0x38);
        }

        /* LT9211_WriteI2C_Byte(0x3b,0xb8); //dual port lvds enable */
        LT9211_WriteI2C_Byte(0x3e, 0x92);
        LT9211_WriteI2C_Byte(0x3f, 0x48);
        LT9211_WriteI2C_Byte(0x40, 0x31);
        LT9211_WriteI2C_Byte(0x43, 0x80);
        LT9211_WriteI2C_Byte(0x44, 0x00);
        LT9211_WriteI2C_Byte(0x45, 0x00);
        LT9211_WriteI2C_Byte(0x49, 0x00);
        LT9211_WriteI2C_Byte(0x4a, 0x01);
        LT9211_WriteI2C_Byte(0x4e, 0x00);
        LT9211_WriteI2C_Byte(0x4f, 0x00);
        LT9211_WriteI2C_Byte(0x50, 0x00);
        LT9211_WriteI2C_Byte(0x53, 0x00);
        LT9211_WriteI2C_Byte(0x54, 0x01);
        LT9211_WriteI2C_Byte(0xff, 0x81);
        LT9211_WriteI2C_Byte(0x20, 0x7b);
        LT9211_WriteI2C_Byte(0x20, 0xff); /* mlrx mltx calib reset */
    }
}


/*******************************************************************************
* 函数名: LT9211_TxDigital
* 描  述  : 输出模式设置
* 输  入  : 
* 输  出  : 
* 返回值: 
*******************************************************************************/
static VOID LT9211_TxDigital(VOID)
{
    DISP_LOGI("LT9211 OUTPUT_MODE: \n");
    if (LT9211_OutPutModde == LT9211_OUTPUT_RGB888)
    {
        DISP_LOGI("LT9211 set to LT9211_OUTPUT_RGB888\n");
        LT9211_WriteI2C_Byte(0xff, 0x85);
        LT9211_WriteI2C_Byte(0x88, 0x50);
        LT9211_WriteI2C_Byte(0x60, 0x00);
        LT9211_WriteI2C_Byte(0x6d, 0x07); /* 0x07 */
        LT9211_WriteI2C_Byte(0x6E, 0x00);
        LT9211_WriteI2C_Byte(0xff, 0x81);
        LT9211_WriteI2C_Byte(0x36, 0xc0); /* bit7:ttltx_pixclk_en;bit6:ttltx_BT_clk_en */
    }
    else if (LT9211_OutPutModde == LT9211_OUTPUT_BT656_8BIT)
    {
        DISP_LOGI("LT9211 set to LT9211_OUTPUT_BT656_8BIT\n");
        LT9211_WriteI2C_Byte(0xff, 0x85);
        LT9211_WriteI2C_Byte(0x88, 0x40);
        LT9211_WriteI2C_Byte(0x60, 0x34); /* bit5_4 BT TX module hsync/vs polarity bit2-Output 8bit BT data enable */
        LT9211_WriteI2C_Byte(0x6d, 0x00); /* 0x08 YC SWAP BIT[2:0]=000-outputBGR */
        LT9211_WriteI2C_Byte(0x6e, 0x07); /* 0x07-low 16BIT ; 0x06-High 8 */

        LT9211_WriteI2C_Byte(0xff, 0x81);
        LT9211_WriteI2C_Byte(0x0d, 0xfd);
        LT9211_WriteI2C_Byte(0x0d, 0xff);
        LT9211_WriteI2C_Byte(0xff, 0x81);
        LT9211_WriteI2C_Byte(0x36, 0xc0); /* bit7:ttltx_pixclk_en;bit6:ttltx_BT_clk_en */
    }
    else if (LT9211_OutPutModde == LT9211_OUTPUT_BT1120_16BIT)
    {
        DISP_LOGI("LT9211 set to LT9211_OUTPUT_BT1120_16BIT\n");
        LT9211_WriteI2C_Byte(0xff, 0x85);
        LT9211_WriteI2C_Byte(0x88, 0x40);
        LT9211_WriteI2C_Byte(0x60, 0x33); /* output 16 bit BT1120 */
        LT9211_WriteI2C_Byte(0x6d, 0x08); /* 0x08 YC SWAP */
        LT9211_WriteI2C_Byte(0x6e, 0x06); /* HIGH 16BIT   0x07-BT low 16bit ;0x06-High 16bit BT */

        LT9211_WriteI2C_Byte(0xff, 0x81);
        LT9211_WriteI2C_Byte(0x0d, 0xfd);
        LT9211_WriteI2C_Byte(0x0d, 0xff);
    }
    else if ((LT9211_OutPutModde == LT9211_OUTPUT_LVDS_2_PORT) || (LT9211_OutPutModde == LT9211_OUTPUT_LVDS_1_PORT))
    {
        DISP_LOGI("LT9211 set to LT9211_OUTPUT_LVDS\n");
        LT9211_WriteI2C_Byte(0xff, 0x85); /* lvds tx controller */
        LT9211_WriteI2C_Byte(0x59, 0x50);  /* bit4-LVDSTX Display color depth set 1-8bit, 0-6bit; */
        LT9211_WriteI2C_Byte(0x5a, 0xaa); /* Port A 各参数output data set */
        LT9211_WriteI2C_Byte(0x5b, 0xaa); /* Port b 各参数output data set */
        if (LT9211_OutPutModde == LT9211_OUTPUT_LVDS_2_PORT)
        {
            LT9211_WriteI2C_Byte(0x5c, 0x03);  /* lvdstx port sel 01:dual;00:single */
        }
        else
        {
            LT9211_WriteI2C_Byte(0x5c, 0x00);
        }

        LT9211_WriteI2C_Byte(0x88, 0x50);
        LT9211_WriteI2C_Byte(0xa1, 0x77);
        LT9211_WriteI2C_Byte(0xff, 0x86);
        LT9211_WriteI2C_Byte(0x40, 0x40); /* tx_src_sel */
        /*port src sel*/
        LT9211_WriteI2C_Byte(0x41, 0x34);
        LT9211_WriteI2C_Byte(0x42, 0x10);
        LT9211_WriteI2C_Byte(0x43, 0x23); /* pt0_tx_src_sel */
        LT9211_WriteI2C_Byte(0x44, 0x41);
        LT9211_WriteI2C_Byte(0x45, 0x02); /* pt1_tx_src_scl */

#if 1 /* PORT_OUT_SWAP */
        DISP_LOGI("SAWP reg8646 = 0x%x,\n", LT9211_ReadI2C_Byte(0x46));
        /* LT9211_WriteI2C_Byte(0x46,0x40); //pt0/1_tx_src_scl yelsin */
#endif

#ifdef lvds_format_JEIDA
        LT9211_WriteI2C_Byte(0xff, 0x85);
        LT9211_WriteI2C_Byte(0x59, 0xd0);
        LT9211_WriteI2C_Byte(0xff, 0xd8);
        LT9211_WriteI2C_Byte(0x11, 0x40);
#endif
    }
}


/*******************************************************************************
* 函数名: LT9211_TxReset
* 描  述  : 输出模块复位
* 输  入  : 
* 输  出  : 
* 返回值: 
*******************************************************************************/
static VOID LT9211_TxReset(VOID)
{
    
    LT9211_WriteI2C_Byte(0xFF, 0x81);
    LT9211_WriteI2C_Byte(0x20, 0xFB);
    LT9211_WriteI2C_Byte(0x20, 0xFF);
    
    LT9211_WriteI2C_Byte(0xFF, 0x81);
    LT9211_WriteI2C_Byte(0x20, 0xF1);
    LT9211_WriteI2C_Byte(0x20, 0xFF);
    
    LT9211_WriteI2C_Byte(0xFF, 0x81);
    LT9211_WriteI2C_Byte(0x0D, 0xFB);
    SAL_msleep(5);
    LT9211_WriteI2C_Byte(0x0D, 0xFF);
}

/* 测试代码,需保留 */
#if 0
static VOID LT9211_ReadSettleCycleNumber(VOID)
{
    UINT32 SettleNumber = 0;
    
    LT9211_WriteI2C_Byte(0xFF, 0xD0);
    SettleNumber = LT9211_ReadI2C_Byte(0x88);
    DISP_LOGI(" LT9211_mipi 0x88 Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x89);
    DISP_LOGI(" LT9211_mipi 0x89 Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x8a);
    DISP_LOGI(" LT9211_mipi 0x8a Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x8b);
    DISP_LOGI(" LT9211_mipi 0x8b Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x8c);
    DISP_LOGI(" LT9211_mipi 0x8c Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x8d);
    DISP_LOGI(" LT9211_mipi 0x8d Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x8e);
    DISP_LOGI(" LT9211_mipi 0x8e Number %#x\n",SettleNumber);
    SettleNumber = LT9211_ReadI2C_Byte(0x8f);
    DISP_LOGI(" LT9211_mipi 0x8f Number %#x\n",SettleNumber);

}
#endif

/*******************************************************************************
* 函数名  : LT9211_Init
* 描  述  : lt9211初始化
* 输  入  : const BT_TIMING_S *pstTiming
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT9211_Init(const BT_TIMING_S *pstTiming)
{
    INT32 s32Ret = SAL_FAIL;
    
    DISP_LOGI(" LT9211_mipi to lvds start\n");
    
    s32Ret = LT9211_ChipID();
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT9211 Init failed\n");
        return SAL_FAIL;
    }
    
    LT9211_SystemInt();
    LT9211_MipiRxPhy();
    LT9211_MipiRxDigital();
    
    s32Ret = LT9211_TimingSet(pstTiming);
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT9211 Init detect Timing Set failed\n");
        return SAL_FAIL;
    }
    
    LT9211_DesscPllMipi(pstTiming);
    
    s32Ret = LT9211_MipiPcr();
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT9211 Init MipiPcr failed\n");
        return SAL_FAIL;
    }

    LT9211_TxDigital();
    LT9211_TxPhy();
    /* 延时等待配置生效 */
    SAL_msleep(10);
    LT9211_Txpll();
    LT9211_RXCSC();

    //LT9211_BT_Set();
    LT9211_TxReset();
    
    //LT9211_ReadSettleCycleNumber();
    
    DISP_LOGI(" LT9211_mipi to lvds end\n");
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : LT9211_Config
* 描  述  : lt9211配置
* 输  入  : const BT_TIMING_S *pstTiming : 视屏时序
          UINT8  u8BusIdx              : i2c总线号
          UINT8  u8ChipAddr            : 设备地址
          UINT32 u32Chn                : 统一函数参数，未使用
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 LT9211_Config(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn)
{
    INT32 s32Ret = SAL_FAIL;
    UINT8 u8ReInitCnt = 0;
    
    DISP_LOGI("LT9211 Chn %d BusIdx %d  Set Output Resolution [%d*%dP%d] PixelClock %d\n",u32Chn, u8BusIdx,
        pstTiming->u32Width , pstTiming->u32Height, pstTiming->u32Fps, pstTiming->u32PixelClock);

    LT9211_GetBoardInfo(u8BusIdx, u8ChipAddr);

    while(1)
    {
         
        /* 检测到输出的DE信号异常, 复位TX模块无效后, 重新进行初始化 */
        s32Ret = LT9211_HardReset(u32Chn);
        if(SAL_SOK != s32Ret)
        {
            DISP_LOGE("LT9211 Hard Reset failed\n");
            continue;
        }
        
        SAL_msleep(50);
        
        s32Ret = LT9211_Init(pstTiming);

        if(s32Ret == SAL_SOK)
        {
            DISP_LOGI("LT9211 LVDS init success, ReInit times [%d] \n", u8ReInitCnt);
            break;
        }
        else
        {
            u8ReInitCnt ++;
            if(u8ReInitCnt < 6)
            {
                DISP_LOGW("LT9211 LVDS detect source Data failed, ReInit, ReInit times [%d] \n", u8ReInitCnt);
                continue;
            }
            else
            {
                DISP_LOGE("LT9211 Init failed, ReInit times [%d] \n", u8ReInitCnt);
                return SAL_FAIL;
            }
        }
    }
    return SAL_SOK;
}

/**********************END OF FILE******************/



