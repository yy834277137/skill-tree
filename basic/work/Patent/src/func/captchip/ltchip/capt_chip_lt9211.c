/******************************************************************************
  * @project: LT9211
  * @file: lt9211.c
  * @author: zll
  * @company: LONTIUM COPYRIGHT and CONFIDENTIAL
  * @date: 2019.04.10_20220222 modify RXPLL to Adaptive BW tracking PLL. 8225 to 0x07 &8227 to 0x32
******************************************************************************/
#include "platform_hal.h"
#include "sal.h"
#include "capt_chip_lt9211.h"


/*****************************************************************************
                                LVDS Input Config 
*****************************************************************************/

/*****************************************************************************
                                宏定义                                       
*****************************************************************************/
#define CAPT_LT9211_MAX_NUM 2           /* LT9211芯片数量 */

#define INPUT_PORTA
#define INPUT_PORTB

#define INPUT_PORT_NUM 2

#define MIPI_LANENUM 4 
#define OFFSET 5

/*****************************************************************************
                                结构体                                       
*****************************************************************************/

typedef enum LVDS_FORMAT_ENUM{
    VESA_FORMAT = 0,
    JEDIA_FORMAT
}LVDS_FORMAT;

#define LVDS_FORMAT VESA_FORMAT

typedef enum LVDS_MODE_ENUM{
    DE_MODE = 0,
    SYNC_MODE
}LVDS_MODE;

#define LVDS_MODE SYNC_MODE


typedef struct video_timing{
    UINT16 hfp;
    UINT16 hs;
    UINT16 hbp;
    UINT16 hact;
    UINT16 htotal;
    UINT16 vfp;
    UINT16 vs;
    UINT16 vbp;
    UINT16 vact;
    UINT16 vtotal;
    UINT32 pclk_khz;
}VIDEO_TIMING;

typedef enum 
{
    CAPT_CHIP_LT9211_OUTPUT_YUV422     =0,
    CAPT_CHIP_LT9211_OUTPUT_RGB888     =1,
}CAPT_CHIP_LT9211_OUTPUTFORMAT_ENUM;

typedef struct
{
    UINT8    u8IICBusIdx;        //iic总线号
    UINT8    u8IICAddr;          //iic设备地址
} CAPT_LT9211_IIC_INFO_S;

/*****************************************************************************
                               全局变量                                      
*****************************************************************************/
static UINT16 hact, vact;
static UINT16 hs, vs;
static UINT16 hbp, vbp;
static UINT16 htotal, vtotal;
static UINT16 hfp, vfp;
//static UINT8 VideoFormat=0;
//static UINT32 lvds_clk_in = 0;


static struct video_timing *pVideo_Format;

//hfp, hs, hbp, hact, htotal, vfp, vs, vbp, vact, vtotal, pixclk

/* mstar小于75不降帧时使用下面时序 */
static struct video_timing video_1920x1080_60Hz   ={88, 44, 148,1920,  2200,  4,  5,  36, 1080, 1125, 148500};

/* mstar大于75降帧时使用下面时序，实际帧率60帧率，像素时钟148500 */
static struct video_timing video_1920x1080_120Hz   ={88, 44, 148,1920,  2200,  1,  5,  39, 1080, 1125, 148500};


static CAPT_LT9211_IIC_INFO_S stCaptLt9211IicInfo[CAPT_LT9211_MAX_NUM] = {0};

UINT32 static g_u32CurrentChn = 0;


static CAPT_CHIP_LT9211_OUTPUTFORMAT_ENUM g_enOutputFormat =  CAPT_CHIP_LT9211_OUTPUT_YUV422;

static BOOL g_bEnableOutputClockBurstMode =  SAL_TRUE;

Handle g_capt_chip_lt9211MutexHdl = NULL;   /* 多通道配置时互斥锁 */


/*****************************************************************************
                                函数
*****************************************************************************/

/*******************************************************************************
* 函数名  : capt_chip_readI2cByte
* 描  述  : IIC读取
* 输  入  : - RegAddr 地址
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static UINT8 capt_chip_readI2cByte(UINT8 RegAddr)
{
    UINT8 data = 0;
    IIC_Read(stCaptLt9211IicInfo[g_u32CurrentChn].u8IICBusIdx, stCaptLt9211IicInfo[g_u32CurrentChn].u8IICAddr, RegAddr, &data);

    return data;
}

/*******************************************************************************
* 函数名  : capt_chip_writeI2cByte
* 描  述  : IIC写
* 输  入  : - RegAddr 地址
          - data    数据
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_writeI2cByte(UINT8 RegAddr, UINT8 data)
{
    IIC_Write(stCaptLt9211IicInfo[g_u32CurrentChn].u8IICBusIdx, stCaptLt9211IicInfo[g_u32CurrentChn].u8IICAddr, RegAddr, data);
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211ChipID
* 描  述  : ChipID打印
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211ChipID(void)
{
    capt_chip_writeI2cByte(0xff,0x81);//register bank
    SAL_LOGI("LT9211 Chip ID:0x%x, 0x%x, 0x%x\n",capt_chip_readI2cByte(0x00), capt_chip_readI2cByte(0x01), capt_chip_readI2cByte(0x02));
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211ChipID
* 描  述  : video chk soft rst
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211vid_chk_rst(void)       
{
    capt_chip_writeI2cByte(0xff,0x81);    
    capt_chip_writeI2cByte(0x10,0xbe); 
    SAL_msleep(10);
    capt_chip_writeI2cByte(0x10,0xfe); 
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211ChipID
* 描  述  : lvds rx logic rst
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211lvdsrx_logic_rst(void)       
{
    capt_chip_writeI2cByte(0xff,0x81);     
    capt_chip_writeI2cByte(0x0c,0xeb);
    SAL_msleep(10);
    capt_chip_writeI2cByte(0x0c,0xfb);
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211SystemInt
* 描  述  : system clock init
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211SystemInt(void)
{
    capt_chip_writeI2cByte(0xff,0x82);
    capt_chip_writeI2cByte(0x01,0x18);
    
    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x06,0x61);  
    capt_chip_writeI2cByte(0x07,0xa8); //fm for sys_clk
    
    capt_chip_writeI2cByte(0xff,0x87); //初始化 txpll 寄存器列表默认值给错了
    capt_chip_writeI2cByte(0x14,0x08); //default value
    capt_chip_writeI2cByte(0x15,0x00); //default value
    capt_chip_writeI2cByte(0x18,0x0f);
    capt_chip_writeI2cByte(0x22,0x08); //default value
    capt_chip_writeI2cByte(0x23,0x00); //default value
    capt_chip_writeI2cByte(0x26,0x0f); 
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211LvdsRxPhy
* 描  述  : Rx Lvds Phy配置
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211LvdsRxPhy(void)
{
    #ifdef INPUT_PORTA
    SAL_LOGI("Port A PHY Config\n");
    capt_chip_writeI2cByte(0xff,0x82);
    capt_chip_writeI2cByte(0x02,0x8B);   //Port A LVDS mode enable
    capt_chip_writeI2cByte(0x05,0x21);   //port A CLK lane swap
    capt_chip_writeI2cByte(0x07,0x1f);   //port A clk enable
    capt_chip_writeI2cByte(0x04,0xa0);   //select port A clk as byteclk
    //capt_chip_writeI2cByte(0x09,0xFC); //port A P/N swap
        
    capt_chip_writeI2cByte(0xff,0x86);      
    capt_chip_writeI2cByte(0x33,0xe4);   //Port A Lane swap
    #endif
        
    #ifdef INPUT_PORTB
    SAL_LOGI("Port B PHY Config\n");
    capt_chip_writeI2cByte(0xff,0x82);
    capt_chip_writeI2cByte(0x02,0x88);   //Port A/B LVDS mode enable
    capt_chip_writeI2cByte(0x05,0x21);   //port A CLK lane swap and rterm turn-off
    capt_chip_writeI2cByte(0x0d,0x21);   //port B CLK lane swap
    capt_chip_writeI2cByte(0x07,0x1f);   //port A clk enable  (只开Portb时,porta的lane0 clk要打开)
    capt_chip_writeI2cByte(0x0f,0x1f);   //port B clk enable
    //capt_chip_writeI2cByte(0x10,0x00);   //select port B clk as byteclk
    capt_chip_writeI2cByte(0x04,0xa1);   //reserve
    //capt_chip_writeI2cByte(0x11,0x01);   //port B P/N swap
    capt_chip_writeI2cByte(0x10,0xfc);
    
    capt_chip_writeI2cByte(0xff,0x86);      
    capt_chip_writeI2cByte(0x34,0xe4);   //Port B Lane swap
    
    capt_chip_writeI2cByte(0xff,0xd8);      
    capt_chip_writeI2cByte(0x16,0x80);
    #endif
    
    capt_chip_writeI2cByte(0xff,0x81);
    capt_chip_writeI2cByte(0x20,0x7f);  
    capt_chip_writeI2cByte(0x20,0xff);  //mlrx calib reset
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211LvdsRxDigital
* 描  述  : Rx Lvds Digital配置
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211LvdsRxDigital(void)
{     
    capt_chip_writeI2cByte(0xff,0x85);
    capt_chip_writeI2cByte(0x88,0x10);      //LVDS input, MIPI output
    
    capt_chip_writeI2cByte(0xff,0xd8);

    if(INPUT_PORT_NUM == 1)             //1Port LVDS Input
    {
        capt_chip_writeI2cByte(0x10,0x80);
    }
    else if(INPUT_PORT_NUM == 2)        //2Port LVDS Input
    {
        capt_chip_writeI2cByte(0x10,0x00);
    }
    else
    {
        SAL_LOGI("Port Num Set Error\n");
    }   

    capt_chip_lt9211vid_chk_rst();              //video chk soft rst
    capt_chip_lt9211lvdsrx_logic_rst();         //lvds rx logic rst     
    
    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x30,0x45);     //port AB input port sel  
    
    if(LVDS_FORMAT == JEDIA_FORMAT)
    {
        capt_chip_writeI2cByte(0xff,0x85);
        capt_chip_writeI2cByte(0x59,0xd0);  
        capt_chip_writeI2cByte(0xff,0xd8);
        capt_chip_writeI2cByte(0x11,0x40);
    }
}

#ifdef LT9211_CPAT
/*******************************************************************************
* 函数名  : capt_chip_lt9211lvds_clkstb_check
* 描  述  : lvds_clkstb检查
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static INT32 capt_chip_lt9211lvds_clkstb_check(void)
{
    UINT8 porta_clk_state = 0;
    UINT8 portb_clk_state = 0;
    
    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x00,0x01);
    SAL_msleep(300);
    porta_clk_state = (capt_chip_readI2cByte(0x08) & (0x20));
    
    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x00,0x02);
    SAL_msleep(300);
    portb_clk_state = (capt_chip_readI2cByte(0x08) & (0x20));
    
    if(INPUT_PORT_NUM == 1)
    {
        #ifdef INPUT_PORTA
        if( porta_clk_state )
        {
            return 1;
        }
        else
        {
            return 0;
        }
        #endif
        #ifdef INPUT_PORTB
        if( portb_clk_state )
        {
            return 1;
        }
        else
        {
            return 0;
        }
        #endif
    }
    else if(INPUT_PORT_NUM == 2)
    {
        if(porta_clk_state && portb_clk_state)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}
#endif

/*******************************************************************************
* 函数名  : capt_chip_lt9211ClockCheckDebug
* 描  述  : 读取lvds时钟
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211ClockCheckDebug(void)
{
    UINT32 fm_value;
    UINT32 lvds_clk_in;

    lvds_clk_in = 0;
    #ifdef INPUT_PORTA
    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x00,0x01);
    SAL_msleep(50);
    fm_value = 0;
    fm_value = (capt_chip_readI2cByte(0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + capt_chip_readI2cByte(0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + capt_chip_readI2cByte(0x0a);
    SAL_LOGI("Port A lvds clock: %u\n",fm_value);
    lvds_clk_in = fm_value;
    #endif
    
    #ifdef INPUT_PORTB
    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x00,0x02);
    SAL_msleep(50);
    fm_value = 0;
    fm_value = (capt_chip_readI2cByte(0x08) &(0x0f));
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + capt_chip_readI2cByte(0x09);
    fm_value = (fm_value<<8) ;
    fm_value = fm_value + capt_chip_readI2cByte(0x0a);
    SAL_LOGI("Port B lvds clock: %u\n",fm_value);
    lvds_clk_in += fm_value;
    #endif
    pVideo_Format->pclk_khz = lvds_clk_in;

}

/*******************************************************************************
* 函数名  : capt_chip_lt9211LvdsRxPll
* 描  述  : LvdsRxPll config
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211LvdsRxPll(void)
{
    UINT8 loopx = 0;
    
    capt_chip_writeI2cByte(0xff,0x82);
    capt_chip_writeI2cByte(0x25,0x07);
    capt_chip_writeI2cByte(0x27,0x32);  
    
    if(INPUT_PORT_NUM == 1)             //1Port LVDS Input
    {
        capt_chip_writeI2cByte(0x24,0x24); //RXPLL_LVDSCLK_MUXSEL,PIXCLK_MUXSEL 0x2c.
        capt_chip_writeI2cByte(0x28,0x44); //0x64
    }
    else if(INPUT_PORT_NUM == 2)        //2Port LVDS Input
    {
        capt_chip_writeI2cByte(0x24,0x2c); //RXPLL_LVDSCLK_MUXSEL,PIXCLK_MUXSEL 0x2c.
        capt_chip_writeI2cByte(0x28,0x64); //0x64
    }
    else
    {
        SAL_LOGI("LvdsRxPll: lvds port count error");
    }
    SAL_msleep(10);
    capt_chip_writeI2cByte(0xff,0x81);
    capt_chip_writeI2cByte(0x20,0xdf);
    capt_chip_writeI2cByte(0x20,0xff);
    SAL_msleep(100);
    for(loopx = 0; loopx < 10; loopx++) //Check Rx PLL cal
    {
        capt_chip_writeI2cByte(0xff,0x87);          
        if(capt_chip_readI2cByte(0x12)& 0x80)
        {
            SAL_LOGI("LT9211 rx pll lock");
            break;
        }
        else
        {
            SAL_LOGI("LT9211 rx pll unlocked\n");
        }
    }
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211VideoCheck
* 描  述  : 读取输入端lvds信号
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
void capt_chip_lt9211VideoCheck(void)
{
    UINT8 sync_polarity;

    capt_chip_writeI2cByte(0xff,0x86);
    capt_chip_writeI2cByte(0x20,0x00);
    
    sync_polarity = capt_chip_readI2cByte(0x70);
    vs = capt_chip_readI2cByte(0x71);

    hs = capt_chip_readI2cByte(0x72);
    hs = (hs<<8) + capt_chip_readI2cByte(0x73);
    
    vbp = capt_chip_readI2cByte(0x74);
    vfp = capt_chip_readI2cByte(0x75);

    hbp = capt_chip_readI2cByte(0x76);
    hbp = (hbp<<8) + capt_chip_readI2cByte(0x77);

    hfp = capt_chip_readI2cByte(0x78);
    hfp = (hfp<<8) + capt_chip_readI2cByte(0x79);

    vtotal = capt_chip_readI2cByte(0x7A);
    vtotal = (vtotal<<8) + capt_chip_readI2cByte(0x7B);

    htotal = capt_chip_readI2cByte(0x7C);
    htotal = (htotal<<8) + capt_chip_readI2cByte(0x7D);

    vact = capt_chip_readI2cByte(0x7E);
    vact = (vact<<8)+ capt_chip_readI2cByte(0x7F);

    hact = capt_chip_readI2cByte(0x80);
    hact = (hact<<8) + capt_chip_readI2cByte(0x81);

    SAL_LOGI("sync_polarity = %x\n", sync_polarity);
    if(!(sync_polarity & 0x01)) //hsync
    {
        capt_chip_writeI2cByte(0xff,0xd8);
        capt_chip_writeI2cByte(0x10, (capt_chip_readI2cByte(0x10)| 0x10));
    }
    if(!(sync_polarity & 0x02)) //vsync
    {
        capt_chip_writeI2cByte(0xff,0xd8);
        capt_chip_writeI2cByte(0x10, (capt_chip_readI2cByte(0x10)| 0x20));
    }

    SAL_LOGI("hfp %d, hs %d , hbp %d, hact %d, htotal = %d\n", hfp, hs, hbp, hact, htotal);
    SAL_LOGI("vfp %d, vs %d, vbp %d, vact%d , vtotal = %d\n", vfp, vs, vbp, vact, vtotal);

}

/*******************************************************************************
* 函数名  : capt_chip_lt9211MipiTxByteClk
* 描  述  : 配置输出端Mipi时钟
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211MipiTxByteClk(UINT16 byteclk)
{
    UINT16 bitrate = 0;
    UINT8 seri_div = 0;
    UINT8 post_div = 2;
    UINT8 pre_div = 1;

    UINT8 seri_div_reg = 0;
    UINT8 post_div_reg = 0;
    UINT8 pre_div_reg = 0;
    UINT8 loop_div_reg = 0;
    
    bitrate = byteclk * 8;
    if(((bitrate > 40) && (bitrate < 80)) || (bitrate == 40))
    {
        seri_div = 16;
        seri_div_reg = 3;
    }
    else if(((bitrate > 80) && (bitrate < 160)) || (bitrate == 80))
    {
        seri_div = 8;
        seri_div_reg = 2;
    }
    else if(((bitrate > 160) && (bitrate < 320)) || (bitrate == 160))
    {
        seri_div = 4;
        seri_div_reg = 1;
    }
    else if(((bitrate > 320) && (bitrate < 640)) || (bitrate == 320))
    {
        seri_div = 2;
        seri_div_reg = 0;
    }
    else if(((bitrate > 640) && (bitrate < 1280)) || (bitrate == 640))
    {
        seri_div = 1;
        seri_div_reg = 4;
    }
    
    if(post_div == 1)
    {
        post_div_reg = 0;
    }
    else if(post_div == 2)
    {
        post_div_reg = 1;
    }
    else if(post_div == 4)
    {
        post_div_reg = 2;
    }
    else if(post_div == 8)
    {
        post_div_reg = 3;
    }
    
    if(pre_div == 1)
    {
        pre_div_reg = 0;
    }
    else if(pre_div == 2)
    {
        pre_div_reg = 1;
    }
    else if(pre_div == 4)
    {
        pre_div_reg = 2;
    }
    else if(pre_div == 8)
    {
        pre_div_reg = 3;
    }
    else if(pre_div == 16)
    {
        pre_div_reg = 7;
    }
    
    loop_div_reg = (UINT16)(bitrate * seri_div) / post_div / 25;

    capt_chip_writeI2cByte(0xff,0x82);
    capt_chip_writeI2cByte(0x36,0x03); //b7:txpll_pd
    capt_chip_writeI2cByte(0x37, (0x28 + pre_div_reg));
    capt_chip_writeI2cByte(0x38, (seri_div_reg << 4) | (post_div_reg << 2));
    capt_chip_writeI2cByte(0x3a, (0x80 + loop_div_reg));
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211MipiTxpll
* 描  述  : 配置输出端Mipi pll
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211MipiTxpll(void)
{
    UINT8 loopx;
    UINT8 byteclk = 0;
	
    if( CAPT_CHIP_LT9211_OUTPUT_RGB888 == g_enOutputFormat )
    {
        byteclk = (((pVideo_Format->pclk_khz / 1000) * 3) / MIPI_LANENUM) + OFFSET;//dsi rgb888选择这个
    }
    else
    {
        byteclk = (((pVideo_Format->pclk_khz / 1000) * 2) / MIPI_LANENUM) + OFFSET;//csi yuv422选择这个
    }
    
    //SAL_LOGI("pixclk: %u ,byteclk: %u \n",(pVideo_Format->pclk_khz / 1000),byteclk);

    capt_chip_lt9211MipiTxByteClk(byteclk);
	
    capt_chip_writeI2cByte(0xff,0x87);
    capt_chip_writeI2cByte(0x13,0x00);
    capt_chip_writeI2cByte(0x13,0x80);
    SAL_msleep(100);
    for(loopx = 0; loopx < 10; loopx++) //Check Tx PLL cal done
    {
			
        capt_chip_writeI2cByte(0xff,0x87);			
        if(capt_chip_readI2cByte(0x1f)& 0x80)
        {
            if(capt_chip_readI2cByte(0x20)& 0x80)
            {
                SAL_LOGI("LT9211 tx pll lock\n");
            }
            else
            {
                SAL_LOGI("LT9211 tx pll unlocked\n");
            }					
            SAL_LOGI("LT9211 tx pll cal done\n");
            break;
        }
        else
        {
            SAL_LOGI("LT9211 tx pll unlocked\n");
        }
    }	 		
}


/*******************************************************************************
* 函数名  : capt_chip_lt9211MipiTxPhy
* 描  述  : 配置输出端Mipi phy
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211MipiTxPhy(void)
{       
    capt_chip_writeI2cByte(0xff,0x82);
    capt_chip_writeI2cByte(0x62,0x00); //ttl output disable
    capt_chip_writeI2cByte(0x23,0x02);
    capt_chip_writeI2cByte(0x3b,0x32); //mipi en    
    
    capt_chip_writeI2cByte(0xff,0x81);
    capt_chip_writeI2cByte(0x20,0xfb);
    SAL_msleep(10);
    capt_chip_writeI2cByte(0x20,0xff);   //tx rterm calibration
    
    //capt_chip_writeI2cByte(0x48,0x5f); //Port A Lane P/N Swap
    //capt_chip_writeI2cByte(0x49,0x92); 
    //capt_chip_writeI2cByte(0x52,0x5f); //Port B Lane P/N Swap
    //capt_chip_writeI2cByte(0x53,0x92); 
    
    capt_chip_writeI2cByte(0xff,0x86);  
    capt_chip_writeI2cByte(0x40,0x80); //tx_src_sel
    /*port src sel*/
    capt_chip_writeI2cByte(0x41,0x01);  
    capt_chip_writeI2cByte(0x42,0x23);
    capt_chip_writeI2cByte(0x43,0x40); //Port A MIPI Lane Swap
    capt_chip_writeI2cByte(0x44,0x12);
    capt_chip_writeI2cByte(0x45,0x34); //Port B MIPI Lane Swap
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211MipiTxDigital
* 描  述  : 配置输出端Mipi信号
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211MipiTxDigital(void)
{   
    capt_chip_writeI2cByte(0xff,0xd4);
    capt_chip_writeI2cByte(0x1c,0x30);  //hs_rqst_pre
    capt_chip_writeI2cByte(0x1d,0x0a);  //lpx
    capt_chip_writeI2cByte(0x1e,0x06);  //prpr
    capt_chip_writeI2cByte(0x1f,0x0a);  //trail
    capt_chip_writeI2cByte(0x21,0x00);  //[5]byte_swap,[0]burst_clk

    capt_chip_writeI2cByte(0xff,0xd4); 

    capt_chip_writeI2cByte(0x15,0x01);
    capt_chip_writeI2cByte(0x16,0x55);

    
    if(CAPT_CHIP_LT9211_OUTPUT_RGB888 == g_enOutputFormat)
    {
        capt_chip_writeI2cByte(0x10,0x01);
        capt_chip_writeI2cByte(0x11,0x20);//read byteclk
        capt_chip_writeI2cByte(0x12,0x24);      //rgb888
        capt_chip_writeI2cByte(0x13,0x0f);  //bit[5:4]:lane num, bit[2]:bllp,bit[1:0]:vid_mode
	    capt_chip_writeI2cByte(0x14,0x20);
    }
    else
    {
        capt_chip_writeI2cByte(0x10,0x01);
        capt_chip_writeI2cByte(0x11,0x50); //read byteclk
        capt_chip_writeI2cByte(0x12,0x1E);    //yuv422
        capt_chip_writeI2cByte(0x13,0x0f);  //bit[5:4]:lane num, bit[2]:bllp,bit[1:0]:vid_mode
        capt_chip_writeI2cByte(0x14,0x04); //bit[5:4]:data typ,bit[2:0]:fmt sel 000:rgb888
        
    }

    if(SAL_TRUE == g_bEnableOutputClockBurstMode)
    {
        capt_chip_writeI2cByte(0x21,0x02);      //时钟非连续模式
    }
    else
    {
        capt_chip_writeI2cByte(0x21,0x01);      //时钟连续模式
    }
    
    capt_chip_writeI2cByte(0xff,0xd0);
    capt_chip_writeI2cByte(0x04,0x10);
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211SetTxTiming
* 描  述  : 配置输出端Mipi信号
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
void capt_chip_lt9211SetTxTiming(void)
{
    UINT16 hact, vact;
    UINT16 vbp, vs;
#if 0
    UINT16 hbp,hs;
    UINT16 htotal, vtotal;
    UINT16 hfp, vfp;  

    htotal = pVideo_Format->htotal;
    vtotal = pVideo_Format->vtotal;
    hs = pVideo_Format->hs;

    hfp = pVideo_Format->hfp;
    vfp = pVideo_Format->vfp; 
    hbp = pVideo_Format->hbp;
#endif
    hact = pVideo_Format->hact;
    vact = pVideo_Format->vact;
    vs   = pVideo_Format->vs;
    vbp  = pVideo_Format->vbp;

    capt_chip_writeI2cByte(0xff,0xd4);
    capt_chip_writeI2cByte(0x04,0x01); //hs[7:0] not care
    capt_chip_writeI2cByte(0x05,0x01); //hbp[7:0] not care
    capt_chip_writeI2cByte(0x06,0x01); //hfp[7:0] not care
    capt_chip_writeI2cByte(0x07,(UINT8)(hact>>8)); //hactive[15:8]
    capt_chip_writeI2cByte(0x08,(UINT8)(hact)); //hactive[7:0]
    
    capt_chip_writeI2cByte(0x09,(UINT8)(vs)+(UINT8)(vbp) - 1); //vfp[7:0]
    capt_chip_writeI2cByte(0x0a,0x00); //bit[3:0]:vbp[11:8]
    capt_chip_writeI2cByte(0x0b,0x01); //vbp[7:0]
    capt_chip_writeI2cByte(0x0c,(UINT8)(vact>>8)); //vcat[15:8]
    capt_chip_writeI2cByte(0x0d,(UINT8)(vact)); //vcat[7:0]
    capt_chip_writeI2cByte(0x0e,0x00); //vfp[11:8]
    capt_chip_writeI2cByte(0x0f,0x00); //vfp[7:0]


}

/*******************************************************************************
* 函数名  : capt_chip_lt9211CSC
* 描  述  : 配置输出端Mipi色彩
* 输  入  : - 无
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
static void capt_chip_lt9211CSC(void)
{
    capt_chip_writeI2cByte(0xff,0xf9);
    capt_chip_writeI2cByte(0x86,0x0f);   //03/00
    capt_chip_writeI2cByte(0x87,0x30);   //4f/4c
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211Reset
* 描  述  : 复位Lt9211
* 输  入  : - uiChn       通道
* 输  出  : - 无
* 返回值  : - SAL_SOK        : 成功
*           SAL_FAIL      : 失败
*******************************************************************************/
static INT32 capt_chip_lt9211Reset(UINT32 uiChn)
{

    INT32 s32Fd =  -1;
    INT32 s32status = 0;
    DDM_resetSet  stResetPrm;
    UINT32  u32BoardType = HARDWARE_GetBoardType();

    s32Fd = open(DDM_DEV_RESET_NAME, O_RDWR);
    if(s32Fd < 0)
    {
      DISP_LOGE("open %s failed\n", DDM_DEV_RESET_NAME);
      return SAL_FAIL;
    }

    //LT9211
    memset((char *)&stResetPrm, 0, sizeof(DDM_resetSet));
    if(DB_TS3719_V1_0 == u32BoardType || DB_RS20046_V1_0 == u32BoardType)
    {
        stResetPrm.type = DDM_RESET_TYPE_VIDEO;
        DISP_LOGI("RESET TYPEmcu VIDEO!\n");
    }
    else
    {
        /* nt用的显示模块宏定义的类型，后续统一改为输入DDM_RESET_TYPE_VIDEO */
        stResetPrm.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
        DISP_LOGI("RESET TYPE DISPLAY!\n");
    }
    stResetPrm.subtype = DDM_DP_SUBTYPE_LT9211;   //二级子类型为LT9211显示模块
    stResetPrm.num = uiChn;


    s32status = ioctl(s32Fd, DDM_IOC_RESET_OFFON, &stResetPrm);  //使用ioctl接口复位
    close(s32Fd);
    if (s32status < 0)
    {
      DISP_LOGE("RESET ioctl reset DDM_DP_SUBTYPE_LT9211 failed fd %d %d!\n",s32Fd, s32status);
    }
    else
    {
      DISP_LOGI("LT9211 hard reset ok!\n");
    }


    return SAL_SOK;
    
}

/*******************************************************************************
* 函数名  : capt_chip_lt9211Config
* 描  述  : 配置Lt9211 (目前MSTAR固定输出1080P分辨率，不考虑w h)
* 输  入  : - uiChn       通道
          - u32Width    宽
          - u32height   高
          - fps         帧率
          - bIsCheckPixClk 是否检查时钟
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
void capt_chip_lt9211Config(UINT32 uiChn, UINT32 u32Width, UINT32 u32height, UINT32 fps, BOOL bIsCheckPixClk)
{
    SAL_mutexLock(g_capt_chip_lt9211MutexHdl);

    UINT32  u32CheckPixClk = 0;
    UINT32 u32CheckTimes = 15;

    if(uiChn >= CAPT_LT9211_MAX_NUM)
    {
        SAL_LOGE("Chn is err %u,mxa num is %u",uiChn, CAPT_LT9211_MAX_NUM);
        SAL_mutexUnlock(g_capt_chip_lt9211MutexHdl);
        return;
    }

    g_u32CurrentChn = uiChn;
    SAL_LOGI("Chn:%u IIC:%u %u\n",g_u32CurrentChn,stCaptLt9211IicInfo[g_u32CurrentChn].u8IICBusIdx,stCaptLt9211IicInfo[g_u32CurrentChn].u8IICAddr);

    /* 验证LT9211读出的输入是否和计算的匹配 */
    while(1)
    {
        capt_chip_lt9211Reset(uiChn);
        capt_chip_lt9211ChipID();
        capt_chip_lt9211SystemInt();
        capt_chip_lt9211LvdsRxPhy();
        capt_chip_lt9211LvdsRxDigital();
        capt_chip_lt9211LvdsRxPll();
        
        capt_chip_lt9211VideoCheck();

        /* mstar降帧时序和不降帧时序不同 */
        if(fps <= 75)
        {
            /* mstar出来固定1080P，只是帧率不同，将需更改pclk_khz */
            pVideo_Format = &video_1920x1080_60Hz;
        }
        else
        {
            pVideo_Format = &video_1920x1080_120Hz;
        }
        
        capt_chip_lt9211ClockCheckDebug();

        if(fps <= 75)
        {
            u32CheckPixClk = 2200 * 1125 * fps / 1000;
        }
        else
        {
            u32CheckPixClk = 2200 * 1125 * 60 / 1000;
        }

        if((u32CheckPixClk / 1000  == pVideo_Format->pclk_khz / 1000 && pVideo_Format->hact == hact && pVideo_Format->vact == vact && pVideo_Format->htotal == htotal && pVideo_Format->vtotal == vtotal) 
            || SAL_FALSE == bIsCheckPixClk)
        {
            SAL_LOGI("LT92211 timing sequence right %u\n",u32CheckPixClk / 1000);
            break;
        }
        u32CheckTimes--;
        if(0 == u32CheckTimes)
        {
            SAL_LOGE("LT92211 timing sequence err %u %u\n",u32CheckPixClk / 1000,pVideo_Format->pclk_khz / 1000);
            break;
        }
        SAL_msleep(200);
    }

    if( pVideo_Format != NULL )
    {   
        capt_chip_lt9211LvdsRxPll();
        /********MIPI OUTPUT CONFIG********/
        capt_chip_lt9211MipiTxPhy();
        capt_chip_lt9211MipiTxpll();
        capt_chip_lt9211SetTxTiming();
        if(CAPT_CHIP_LT9211_OUTPUT_RGB888 != g_enOutputFormat)
        {
            capt_chip_lt9211CSC();
        }
        capt_chip_lt9211MipiTxDigital();
    }

    SAL_mutexUnlock(g_capt_chip_lt9211MutexHdl);

}

/*******************************************************************************
* 函数名  : capt_chip_lt9211ModuleInit
* 描  述  : 配置Lt9211
* 输  入  : - uiChn       通道
          - u32I2cBusID    IIC总线号
          - u32I2cAddr     IIC地址
* 输  出  : - 无
* 返回值  : - 无
*******************************************************************************/
void capt_chip_lt9211ModuleInit(UINT32 uiChn,UINT32 u32I2cBusID, UINT32 u32I2cAddr)
{
    if(uiChn >= CAPT_LT9211_MAX_NUM)
    {
        SAL_LOGE("Chn is err,mxa num is %u", CAPT_LT9211_MAX_NUM);
        return;
    }
    
    if(g_capt_chip_lt9211MutexHdl == NULL)
    {
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_capt_chip_lt9211MutexHdl);
    }

    /* IIC赋值 */
    stCaptLt9211IicInfo[uiChn].u8IICBusIdx = u32I2cBusID;
    stCaptLt9211IicInfo[uiChn].u8IICAddr = u32I2cAddr;

    SAL_LOGI("Chn: %u ,IIC: %u %u \n",uiChn,stCaptLt9211IicInfo[uiChn].u8IICBusIdx,stCaptLt9211IicInfo[uiChn].u8IICAddr);

    /* 产品配置 */
    UINT32  u32BoardType = HARDWARE_GetBoardType();
    if(DB_TS3719_V1_0 == u32BoardType || DB_RS20046_V1_0 == u32BoardType)
    {
        g_enOutputFormat =  CAPT_CHIP_LT9211_OUTPUT_RGB888;
        g_bEnableOutputClockBurstMode = SAL_FALSE;
    }
    else
    {
        g_enOutputFormat =  CAPT_CHIP_LT9211_OUTPUT_YUV422;
        g_bEnableOutputClockBurstMode = SAL_FALSE;
    }
    
    /* 初始化默认不验证pix_clk */
    capt_chip_lt9211Config(uiChn,1920,1080,60,SAL_FALSE);
    
    return;
}




