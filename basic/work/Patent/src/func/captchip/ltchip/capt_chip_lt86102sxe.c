/******************************************************************************
  * @project: LT9211
  * @file: lt9211.c
  * @author: zll
  * @company: LONTIUM COPYRIGHT and CONFIDENTIAL
  * @date: 2019.04.10_20220222 modify RXPLL to Adaptive BW tracking PLL. 8225 to 0x07 &8227 to 0x32
******************************************************************************/
#include "platform_hal.h"
#include "sal.h"
#include "capt_chip_lt86102sxe.h"


/*RX EQ */
 //HDCP
#define   HDCP_RX_ONLY
#define    RX_FIXED_EQ      0x11    /*??EQ */
#define    RX_EQ_VALUE      0x04
//#define   HDCP_RX_TX


/*CHIP */
//#define   LT86104SXE 
#define  LT86102SXE

#define CHIP_DETECT_TX_HPD
//#define TX_PLUG_INFLUENCE
#define TX_PLUG_NO_INFLUENCE

#define    TX_SWING_VALUE 0x60
#define    RX_CLK_STABLE_INT 0xdF
/******************* LVDS Input Config ********************/
#define  TX0_HPD_HIGH   0x01
#define  TX0_HPD_LOW    0x00
#define  TX1_HPD_HIGH   0x02
#define  TX1_HPD_LOW    0x00
#define  TX2_HPD_HIGH   0x04
#define  TX2_HPD_LOW    0x00
#define  TX3_HPD_HIGH   0x08
#define  TX3_HPD_LOW    0x00

/*****************************************************************************
                               全局变量
*****************************************************************************/
static UINT8 g_Old_Tx_Hpd = 0;
static UINT8 PresentBank  = 0x70;
static UINT32 g_UINT32IIC = 2;
static UINT32 g_UINT32Dev = 0x70;
static BOOL FlagExtendBlockExist[4]={1,1,1,1};
static BOOL Flag_clk_int;

static SAL_ThrHndl stCaptLtThrHandl;                      /* 通道线程 */




static UINT8 capt_chip_readI2cByte(UINT8 RegAddr)
{
    UINT8 data = 0;
    IIC_Read(g_UINT32IIC, g_UINT32Dev, RegAddr, &data);

    return data;
}


static void capt_chip_writeI2cByte(UINT8 RegAddr, UINT8 d)
{
    IIC_Write(g_UINT32IIC, g_UINT32Dev, RegAddr, d);
}

static void capt_chipLlt86102sxevdPowerDownChip( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x9A, 0x02 );
    capt_chip_writeI2cByte( 0xAA, 0x02 );
    capt_chip_writeI2cByte( 0xBA, 0x02 );
    capt_chip_writeI2cByte( 0xCA, 0x02 );
}


static void capt_chipvdTxSwingSet( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    /*tx0 */
    capt_chip_writeI2cByte( 0x9D, TX_SWING_VALUE );
    /*tx1 */
    capt_chip_writeI2cByte( 0xAD, TX_SWING_VALUE );
    /*tx2 */
    capt_chip_writeI2cByte( 0xBD, TX_SWING_VALUE );
    /*tx3 */
    capt_chip_writeI2cByte( 0xCD, TX_SWING_VALUE );
}


static void capt_chipvdRxClkGatingEnable( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x04, 0xFF );
    capt_chip_writeI2cByte( 0x05, 0xFF );
    capt_chip_writeI2cByte( 0x06, 0xFF );
    capt_chip_writeI2cByte( 0x07, 0xFF );
    capt_chip_writeI2cByte( 0x08, 0xFF );
}



static void capt_chipvdRxEqModeConfig( UINT8 rx_eq_mode, UINT8 rx_eq_val )
{
    capt_chip_writeI2cByte( 0xFF, 0x70 );
    PresentBank = 0x70;
    capt_chip_writeI2cByte( 0x23, rx_eq_mode );
    capt_chip_writeI2cByte( 0x2D, rx_eq_val );
    capt_chip_writeI2cByte( 0x2E, rx_eq_val );
    capt_chip_writeI2cByte( 0x2F, rx_eq_val );
}

static void capt_chipvdRxAutoEQSetting( void )
{
    capt_chip_writeI2cByte( 0xff, 0x70 );
    PresentBank = 0x70;
    capt_chip_writeI2cByte( 0x28, 0x11 );       /* [7:4]=RGD_SW_CDR_FILT_SET=0x1,[3:0]=RGD_EQ3_SET_LOAD=0x4 */
    capt_chip_writeI2cByte( 0x29, 0x42 );       /*RGD_EQ1_SET_LOAD=0xf4 */
    capt_chip_writeI2cByte( 0x2b, 0x41 );       /*[7:6]=RGD_CLK_DET_CRIT=0x1,[4:0]=RGD_EQ_INDEX_LOAD=0x01 */
    capt_chip_writeI2cByte( 0x29, 0x7c );
    capt_chip_writeI2cByte( 0x2b, 0x42 );
    capt_chip_writeI2cByte( 0x29, 0xd4 );
    capt_chip_writeI2cByte( 0x2b, 0x43 );
    capt_chip_writeI2cByte( 0x29, 0x84 );
    capt_chip_writeI2cByte( 0x2b, 0x44 );
    capt_chip_writeI2cByte( 0x29, 0x18 );
    capt_chip_writeI2cByte( 0x2b, 0x45 );
    capt_chip_writeI2cByte( 0x29, 0x48 );
    capt_chip_writeI2cByte( 0x2b, 0x46 );
    capt_chip_writeI2cByte( 0x29, 0xb0 );
    capt_chip_writeI2cByte( 0x2b, 0x47 );
    capt_chip_writeI2cByte( 0x29, 0xd0 );
    capt_chip_writeI2cByte( 0x2b, 0x48 );
    capt_chip_writeI2cByte( 0x29, 0x90 );
    capt_chip_writeI2cByte( 0x2b, 0x49 );
    capt_chip_writeI2cByte( 0x29, 0xe0 );
    capt_chip_writeI2cByte( 0x2b, 0x4a );
    capt_chip_writeI2cByte( 0x29, 0x60 );
    capt_chip_writeI2cByte( 0x2b, 0x4b );
    capt_chip_writeI2cByte( 0x29, 0x20 );
    capt_chip_writeI2cByte( 0x2b, 0x4c );
    capt_chip_writeI2cByte( 0x28, 0xf1 );/* [7:4]=RGD_SW_CDR_FILT_SET=0xf */
    capt_chip_writeI2cByte( 0x29, 0xc0 );
    capt_chip_writeI2cByte( 0x2b, 0x4d );
    capt_chip_writeI2cByte( 0x29, 0x40 );
    capt_chip_writeI2cByte( 0x2b, 0x4e );
    capt_chip_writeI2cByte( 0x29, 0x80 );
    capt_chip_writeI2cByte( 0x2b, 0x4f );
    capt_chip_writeI2cByte( 0x28, 0xf1 );
    capt_chip_writeI2cByte( 0x29, 0x3e );
    capt_chip_writeI2cByte( 0x2b, 0x40 );
    /*capt_chip_writeI2cByte( 0x2C, 0x25 );       / * [7:6]=RGD_FREQ_CHG_CRIT=0x0 * / */
    capt_chip_writeI2cByte( 0x1E, 0x1F );       /* [5]=RGD_CDR_GEAR_EN=0x0 */
    capt_chip_writeI2cByte( 0x1F, 0x20 );       /* [7:4]=RGD_CDR_LOCK_CRIT=0x2 */
    capt_chip_writeI2cByte( 0x2A, 0x0B );
    capt_chip_writeI2cByte( 0x24, 0xc0 );
    capt_chip_writeI2cByte( 0x25, 0xb0 );
}

/**************************************
*  TX Function
**************************************/
static void capt_chipvdTxDriveOffConfig( void )
{
    /*SAL_INFO( "\r\ntx drive off"); */
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x97, 0x1B );
    capt_chip_writeI2cByte( 0xA7, 0x1B );
    capt_chip_writeI2cByte( 0xB7, 0x1B );
    capt_chip_writeI2cByte( 0xC7, 0x1B );
}

static void capt_chipLt86102sexState( void )
{
    capt_chip_writeI2cByte( 0xff, 0x60 );

    capt_chip_writeI2cByte( 0x09, 0x00 );
    capt_chip_writeI2cByte( 0x09, 0xFC );
    capt_chip_writeI2cByte( 0x0b, 0xdf );
    /*capt_chip_writeI2cByte(0x8B,0x30); */
    
    capt_chip_writeI2cByte( 0x8B, 0x30 );       /* Second order passive LPF PLL+1.8GVCO */
    capt_chip_writeI2cByte( 0x8D, 0x11 );       /*two order lpf */
    capt_chip_writeI2cByte( 0x84, 0xe8 );       /*afe_bw_setting */
    
    
    capt_chip_writeI2cByte( 0x88, 0xff );//rx term
    capt_chip_writeI2cByte( 0x48, 0x00 );
    capt_chip_writeI2cByte( 0x49, 0xFF );
    capt_chip_writeI2cByte( 0x8F, 0x24 );
    capt_chip_writeI2cByte( 0x41, 0xF1 );
    capt_chip_writeI2cByte( 0x86, 0x61 );
    capt_chip_writeI2cByte( 0x87, 0x51 );
    capt_chip_writeI2cByte( 0x85, 0x92 );
    capt_chip_writeI2cByte( 0x8C, 0x88 );
    capt_chip_writeI2cByte( 0x10, 0xFF );
    capt_chip_writeI2cByte( 0x44, 0x00 );
    capt_chip_writeI2cByte( 0x47, 0x30 );


    /*
     * capt_chip_writeI2cByte( 0x9f, 0x00 );/ *tx dc mode * /
     * capt_chip_writeI2cByte( 0xaf, 0x00 );
     * capt_chip_writeI2cByte( 0xbf, 0x00 );
     * capt_chip_writeI2cByte( 0xcf, 0x00 );
     */
    capt_chip_writeI2cByte( 0xff, 0x70 );

    /*capt_chip_writeI2cByte( 0x2b, 0x00 ); */
    capt_chip_writeI2cByte( 0x2c, 0x07 );
    capt_chipvdTxDriveOffConfig();
    capt_chipvdRxAutoEQSetting();
    capt_chipvdRxEqModeConfig( RX_FIXED_EQ, RX_EQ_VALUE );
    capt_chipvdRxClkGatingEnable();
    capt_chipvdTxSwingSet();
    capt_chipLlt86102sxevdPowerDownChip();
}


static INT32 capt_chipLt86102sxeReset(void)
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
    stResetPrm.subtype = DDM_DP_SUBTYPE_LT86102;   //二级子类型为LT9211显示模块
    stResetPrm.num = 0;                            

    s32status = ioctl(s32Fd, DDM_IOC_RESET_OFFON, &stResetPrm);  //使用ioctl接口复位
    close(s32Fd);
    if (s32status < 0)
    {
      DISP_LOGE("RESET ioctl reset capt_chipLt86102sxeReset failed fd %d %d!\n",s32Fd, s32status);
    }
    else
    {
      DISP_LOGI("LT9211 hard reset ok!\n");
    }


    return SAL_SOK;
    
}



#if 0
static void capt_chipvdSetRxHpdLow( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x47, 0x30 );
    SAL_INFO( "\r\nRX HPD LOW" );
}
#endif

static void capt_chipvdSetRxHpdHigh( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x47, 0x70 );
    SAL_INFO( "\r\nRX HPD HIGH" );
}


/*HDCPg功能，先不开启*/
//#define   HDCP_RX_ONLY

#if 0  //#ifdef HDCP_RX_ONLY
void vdHdcp_Mode_Config( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x70 );
    PresentBank=0x70;
#ifdef HDCP_RX_ONLY
    capt_chip_writeI2cByte( 0x95, 0xAC );
#endif
#ifdef HDCP_RX_TX
    capt_chip_writeI2cByte( 0x95, 0xAA );
    //capt_chip_writeI2cByte( 0x96, 0x13 );
#endif
#ifdef HDCP_RPT
    capt_chip_writeI2cByte( 0x95, 0xA9 );
#endif
}


void  vdTx_Hdcp_RdR0_Time_Config( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x70 );
    PresentBank=0x70;
    capt_chip_writeI2cByte( 0x90, 0x39 );/* 25M 0x2B:100ms; 33M 0x39:115ms */
}


void  vdRx_Hdcp_Load_Key( void )
{
    capt_chip_writeI2cByte( 0xff, 0x70 );
    PresentBank=0x70;
    capt_chip_writeI2cByte( 0x84, 0x85 );
    capt_chip_writeI2cByte( 0x83, 0x03 );
}


void vdRx_Hdcp_key_Load_Det( void )
{
    UINT8   load_done = 0;
    UINT8   load_key_cnt_done = 0;
    UINT8   load_key_cnt_low = 0, load_key_cnt_high = 0;
        capt_chip_writeI2cByte( 0xff, 0x70 );
        PresentBank=0x70;
        load_done       = ( (capt_chip_readI2cByte( 0xBF ) & 0x04) >> 2);
        load_key_cnt_low    = capt_chip_readI2cByte( 0xBE );
        load_key_cnt_high   = capt_chip_readI2cByte( 0xBF ) & 0x03;
        if ( (load_key_cnt_low==0x3b)&&(load_key_cnt_high==0x02) )
        {
            load_key_cnt_done = 1;
        }else {
            load_key_cnt_done = 0;
        }
        if ( load_done & load_key_cnt_done )
        {
            capt_chip_writeI2cByte( 0x83, 0x0a );
            SAL_INFO( "\r\nLOAD HDCP DONE" );
        }
}

#endif


static BOOL capt_chipTxHpdChangeDetect( void )
{
    UINT8    New_Tx1_Hpd = 0  , New_Tx3_Hpd = 0, New_Tx_Hpd;
    UINT8   i       = 0;
    UINT8   HDMI_MODE   = 0;
    



    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    if ( capt_chip_readI2cByte( 0x4A ) & 0x40 )
    {
        New_Tx1_Hpd = TX1_HPD_HIGH;
    }
    else
    {
        New_Tx1_Hpd = TX1_HPD_LOW;
    }
    if ( capt_chip_readI2cByte( 0x4A ) & 0x10 )
    {
        New_Tx3_Hpd = TX3_HPD_HIGH;
    }
    else
    {
        New_Tx3_Hpd = TX3_HPD_LOW;
    }
    New_Tx_Hpd = New_Tx1_Hpd | New_Tx3_Hpd;



    if ( g_Old_Tx_Hpd == New_Tx_Hpd )
    {
        return(SAL_FALSE);
    }
    #ifdef TX_PLUG_NO_INFLUENCE

    for ( i = 0; i < 4; i++ )
    {
        // getHdmiTx1Edid(edid);           /*获取tx输出设备edid*/
    }
    g_Old_Tx_Hpd       = New_Tx_Hpd;
    #endif

    if ( FlagExtendBlockExist[0x00] == SAL_FALSE )
    {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) | 0x01;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX0 DVI MODE" );
    }else   {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) & 0xfe;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_INFO( "\r\nTX0 HDMI MODE" );
    }
    if ( FlagExtendBlockExist[0x01] == SAL_FALSE )
    {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) | 0x02;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX1 DVI MODE" );
    }else   {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) & 0xfd;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX1 HDMI MODE" );
    }
    if ( FlagExtendBlockExist[0x02] == SAL_FALSE )
    {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) | 0x04;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX2 DVI MODE" );
    }else   {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) & 0xfb;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX2 HDMI MODE" );
    }
    if ( FlagExtendBlockExist[0x03] == SAL_FALSE )
    {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) | 0x08;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX3 DVI MODE" );
    }else   {
        capt_chip_writeI2cByte( 0xff, 0x70 );
        HDMI_MODE = capt_chip_readI2cByte( 0xaa ) & 0xf7;
        capt_chip_writeI2cByte( 0xaa, HDMI_MODE );
        SAL_DEBUG( "\r\nTX3 HDMI MODE" );
    }
    
    if ( New_Tx_Hpd == 0 )
    {
       // capt_chipvdSetRxHpdLow();  /*硬件检测线未接默认设置为HIGH*/
        return(SAL_FALSE);
    }else {
        //g_Flag_Tx_HPDChange   = SAL_TRUE;
        //g_FlagTxExist     = SAL_TRUE;
        return(SAL_TRUE);
    }
}


static void capt_chipvdTxDriveOnConfig( void )
{
    /*SAL_INFO( "\r\ntx drive on"); */
    if ( g_Old_Tx_Hpd & 0x01 )
    {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0x97, 0x7B );
        /*SAL_INFO( "\r\nTX0 ON" ); */
    }else {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0x97, 0x5B );
        /*SAL_INFO( "\r\nTX0 OFF" ); */
    }
    if ( g_Old_Tx_Hpd & 0x02 )
    {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0xA7, 0x3B );
        /*SAL_INFO( "\r\nTX1 ON" ); */
    }else {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0xA7, 0x1B );
        /*SAL_INFO( "\r\nTX1 OFF" ); */
    }
    if ( g_Old_Tx_Hpd & 0x04 )
    {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0xB7, 0x3B );
        /*SAL_INFO( "\r\nTX2 ON" ); */
    }else {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0xB7, 0x1B );
        /*SAL_INFO( "\r\nTX2 OFF" ); */
    }
    if ( g_Old_Tx_Hpd & 0x08 )
    {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0xC7, 0x3B );
        /*SAL_INFO( "\r\nTX3 ON" ); */
    }else {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        PresentBank = 0x60;
        capt_chip_writeI2cByte( 0xC7, 0x1B );
        /*SAL_INFO( "\r\nTX3 OFF" ); */
    }
}


static void capt_chipvdSyncDetect( void )
{
    UINT16  htotal1 = 0;
    UINT16  htotal2 = 0;
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x0A, 0xBF );
    capt_chip_writeI2cByte( 0x0A, 0xFF );
    SAL_msleep( 2 );
    htotal1 = (capt_chip_readI2cByte( 0x35 ) << 8) | capt_chip_readI2cByte( 0x36 );
    SAL_msleep( 2 );
    htotal2 = (capt_chip_readI2cByte( 0x35 ) << 8) | capt_chip_readI2cByte( 0x36 );
    if ( htotal1 == htotal2 )
    {
        if ( htotal1 > 500 )
        {
            capt_chipvdTxDriveOnConfig();
        }else  {
            capt_chipvdTxDriveOffConfig();
        }
    }else   {
        capt_chipvdTxDriveOffConfig();
    }
}


static void capt_chipvdSoftResetFun( UINT8 reset_status )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    /*capt_chip_writeI2cByte(0x09,reset_status); */
    capt_chip_writeI2cByte( 0x0A, reset_status );
    capt_chip_writeI2cByte( 0x0B, reset_status );
    capt_chip_writeI2cByte( 0x0C, reset_status );
    capt_chip_writeI2cByte( 0x0D, reset_status );
    capt_chip_writeI2cByte( 0x0E, reset_status );
    capt_chip_writeI2cByte( 0x0F, reset_status );
    capt_chip_writeI2cByte( 0x10, reset_status );
    capt_chip_writeI2cByte( 0x11, reset_status );
}

#if 0

void Write_Edid_To_Shadow( void )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank=0x60;
    capt_chip_writeI2cByte( 0x66, 0x07 );
    capt_chip_writeI2cByte( 0x65, 0x00 );
    capt_chip_writeI2cByteN( 0x60, edid_current, 0x80 );/*edid_current  Edid_Default */
    capt_chip_writeI2cByte( 0x65, 0x80 );
    capt_chip_writeI2cByteN( 0x60, &edid_current[0x80], 0x80 );
}

static void  vdRx_Ddc_Edid_Slave_Fun( UINT8 ddc_edid_slave )
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x66, ddc_edid_slave );
}
#endif


static void capt_chipLlt86102sxeVdStartSystem( void )
{
    /*capt_chipvdSoftResetFun(0x00); */
    capt_chipvdSoftResetFun( 0xff );
//  Write_Edid_To_Shadow();                         /*writeh edid into lt86102sxe edid shadow */
    capt_chipvdRxClkGatingEnable();
//  vdRx_Status_Interrupt_Fun( RX_CLK_STABLE_INT ); /*set lt86102sxe int mask  先不考虑使用中断，使用轮询方式*/
//  vdRx_Ddc_Edid_Slave_Fun( RX_EDID_DDC_SLAVE_EN );   /*输入edid从mstar中读*/

    capt_chipvdSetRxHpdHigh();                            /*set lt86102sxe hpd high */
    capt_chip_writeI2cByte( 0xff, 0x60 );
    capt_chip_writeI2cByte( 0x88, 0x5d );//rx term
}


static void capt_chipLlt86102sxeWork( void )
{
    if(capt_chipTxHpdChangeDetect())//DETECT TX hpd change  检测输出是否插入
    {
        capt_chipvdSyncDetect();     //检查输入是否插入一遍开启输出，否则会可能会花屏
        capt_chipLlt86102sxeVdStartSystem();          /*开启rx*/
    }

}


static INT32 capt_chipLlt86102sxeVdRxClkStableISR( void )
{
    INT32 s32Ret = SAL_SOK;
    UINT8 rdrxstatus = 0;

    capt_chip_writeI2cByte( 0xFF, 0x60 );
    rdrxstatus = capt_chip_readI2cByte( 0x1A );
    
    if ( rdrxstatus & 0x20 )/*clk rise */
    {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        capt_chip_writeI2cByte( 0x2A, 0x20 | RX_CLK_STABLE_INT );
        capt_chip_writeI2cByte( 0x2A, 0x00 | RX_CLK_STABLE_INT );
        capt_chipvdTxDriveOffConfig();
        SAL_INFO( "\r\nCLK RISE INT" );
        Flag_clk_int        = SAL_TRUE;
        s32Ret = SAL_FAIL;
        /*SAL_INFO( "\r\ng_Flag_CLK_STABLE = SAL_TRUE" ); */
    }
    
    if ( rdrxstatus & 0x80 )/*hdmi mode det */
    {
        capt_chip_writeI2cByte( 0xFF, 0x60 );
        capt_chip_writeI2cByte( 0x2A, 0x80 | RX_CLK_STABLE_INT );
        capt_chip_writeI2cByte( 0x2A, 0x00 | RX_CLK_STABLE_INT );
        SAL_INFO( "\r\nHDMI MODE" );
    }
    /*vdRx_Status_Interrupt_Fun( RX_CLK_STABLE_INT ); */
    rdrxstatus = capt_chip_readI2cByte( 0x1A );
    if ( rdrxstatus == 0x00 )
    {
       // SAL_INFO( "\r\nHDMI MODE rdrxstatus %d", rdrxstatus);
    }

    return s32Ret;
}

static BOOL capt_chipLlt86102sxeRxClockStable()
{
    UINT8 RGOD_CLK_STABLE = 0;
    capt_chip_writeI2cByte( 0xFF, 0x70 );
    PresentBank = 0x70;
    RGOD_CLK_STABLE = capt_chip_readI2cByte( 0x44 ) & 0x08;/*CLK_STABLE */
    if ( RGOD_CLK_STABLE )
    {
        RGOD_CLK_STABLE = 0;
        return(SAL_TRUE);
    }else {
        RGOD_CLK_STABLE = 0;
        return(SAL_FALSE);
    }
}

static void capt_chipLlt86102sxeVdRxPllReset( void )
{
    capt_chip_writeI2cByte( 0xff, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x10, 0xf0 );       /*reset pll PI */
    capt_chip_writeI2cByte( 0x0b, 0x98 );       /*disable PLL_lock to hardware reset CDR/PI; reset pll_lock and cdr*/
    /*reset CDR */
    SAL_msleep( 5 );
    capt_chip_writeI2cByte( 0x0b, 0xD8 );       /*release PLL_lock reset, keep cdr reset */
    capt_chip_writeI2cByte( 0x10, 0xf8 );       /*release PLL reset*/
}


static void capt_chipLlt86102sxeVdVcoCal( void )
{
    capt_chip_writeI2cByte( 0x56, 0x00 );       /*RXPLL vco cur_sel calibration enable signal */
    capt_chip_writeI2cByte( 0x56, 0x80 );
    SAL_msleep( 10 );
}

static BOOL capt_chipLlt86102sxeRxPllLockStable()
{
    UINT8 RGOD_PLL_STABLE = 0;
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    RGOD_PLL_STABLE = capt_chip_readI2cByte( 0x57 ) & 0x01;
    if ( RGOD_PLL_STABLE )
    {
        RGOD_PLL_STABLE = 0;
        return(SAL_TRUE);
    }else {
        RGOD_PLL_STABLE = 0;
        return(SAL_FALSE);
    }
}

static void capt_chipLlt86102sxeVdRxPllRelease( void )
{
    capt_chip_writeI2cByte( 0xff, 0x60 );
    PresentBank = 0x60;
    /*capt_chip_writeI2cByte( 0x0b, 0xd8 ); */
    capt_chip_writeI2cByte( 0x10, 0xff );       /*release  PI reset */
    capt_chip_writeI2cByte( 0x0b, 0xdf );       /*enable PLL_lock to hardware reset CDR/PI;release cdr reset */
}

static void capt_chipLlt86102sxeTxLogicRst(void)
{
    capt_chip_writeI2cByte( 0xFF, 0x60 );
    PresentBank = 0x60;
    capt_chip_writeI2cByte( 0x0A, 0xf0 );
    SAL_msleep(5);
    capt_chip_writeI2cByte( 0x0A, 0xff );
}

static void capt_chipLlt86102sxeVdRxPllStatus( void )
{

    if ( capt_chipLlt86102sxeRxClockStable() )       /*detect the clock stable */
    {
        /*可能需要一直读取或者读取几次*/
        capt_chipLlt86102sxeVdRxPllReset();
        capt_chipLlt86102sxeVdVcoCal();

        if ( capt_chipLlt86102sxeRxPllLockStable() )     /*wair pll  satble */
        {

            capt_chipLlt86102sxeVdRxPllRelease();
            capt_chipLlt86102sxeTxLogicRst();
            SAL_msleep(50);

            capt_chipvdTxDriveOnConfig();
        }
    }

}

static void *capt_chipLlt86102sxeThread()
{
    UINT8 StoreBank;
    INT32 s32Ret = SAL_FAIL;

    SAL_SET_THR_NAME();

    while(1)
    {
        
        capt_chipLlt86102sxeWork();   //检查输入输出，确认是否开启输入输出
        StoreBank = PresentBank;
        s32Ret = capt_chipLlt86102sxeVdRxClkStableISR();
    
        PresentBank = StoreBank;
        if ( PresentBank == 0x60 )
        {
            capt_chip_writeI2cByte( 0xFF, 0x60 );
        }else   {
            capt_chip_writeI2cByte( 0xFF, 0x70 );
        }
    
        if(SAL_SOK != s32Ret)
        {
            capt_chipLlt86102sxeVdRxPllStatus();
        }
        SAL_msleep(300);   /*降低检测时间，插拔时或者帧率改变才需要重新检测*/
    }

    return NULL;
}




void capt_chip_lt86102sxeInit(UINT32 u32devId, UINT32 u32Addr)
{
    SAL_LOGI("\r\n*************LT9211 LVDS2MIPICSI Config*************");

    
    //    g_UINT32IIC = u32devId;
    //    g_UINT32Dev = u32Addr;
    capt_chipLt86102sxeReset();
    capt_chipLt86102sexState();

#if 0    //#ifdef HDCP_RX_ONLY
    vdHdcp_Mode_Config();
    vdTx_Hdcp_RdR0_Time_Config();
    vdRx_Hdcp_Load_Key();
    
    SAL_msleep(100);
    vdRx_Hdcp_key_Load_Det();
#endif
    SAL_thrCreate(&stCaptLtThrHandl, capt_chipLlt86102sxeThread, 60, 0, NULL);
    SAL_LOGI("\r\n*************LT9211 LVDS2MIPICSI Config*************");

  //  capt_chipLlt86102sxeThread();  //创建线程
}
