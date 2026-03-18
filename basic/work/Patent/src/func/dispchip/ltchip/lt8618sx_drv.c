
/*******************************************************************************
 * lt8618sx_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : 
 * Version: V1.0.0  2021年11月25日 Create
 *
 * Description : lt8618sx配置
 * Modification: 
 *******************************************************************************/

#include "platform_hal.h"
#include "sal.h"

typedef enum {
    DVI_OUT=0x00,
    HDMI_OUT=0x80
}TX_OUT_MODE;

typedef enum {
    Input_RGB888,//yes
    Input_RGB_12BIT,
    Input_RGB565,
    Input_YCbCr444,//yes
    Input_YCbCr422_16BIT,//yes
    Input_YCbCr422_20BIT,//no
    Input_YCbCr422_24BIT,//no
    Input_BT1120_16BIT,//ok
    Input_BT1120_20BIT,//no
    Input_BT1120_24BIT,//no
    Input_BT656_8BIT,//OK
    Input_BT656_10BIT,//no use
    Input_BT656_12BIT,//no use
    Input_BT601_8BIT//OK
}LT8618SX_INPUTMODE_ENUM;

typedef enum {
    I2S_2CH,
    I2S_8CH,
    SPDIF
}AUDIO_INPUTMODE_ENUM;

typedef enum VIDEO_OUTPUTMODE_ENUM
{
    Output_RGB888,
    Output_YCbCr444,
    Output_YCbCr422,
    Output_YCbCr422_16BIT,
    Output_YCbCr422_20BIT,
    Output_YCbCr422_24BIT
}_Video_Output_Mode;

struct video_timing{
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
    UINT8  vic;
    UINT8  vpol;
    UINT8  hpol;
};

enum VideoFormat
{
    video_none,
    video_720x480i_60Hz_43=6,     
    video_720x480i_60Hz_169=7,    
    video_1920x1080i_60Hz_169=5,  

    video_640x480_60Hz_vic1,      
    video_720x480_60Hz_vic3,     

    video_1280x720_24Hz_vic60,
    video_1280x720_25Hz_vic61,   
    video_1280x720_30Hz_vic62,
    video_1280x720_50Hz_vic19,
    video_1280x720_60Hz_vic4,   

    video_720x240P_60Hz_43=8,    
    video_720x240P_60Hz_169=9 ,   

    video_1920x1080_30Hz_vic34,
    video_1920x1080_50Hz_vic31,
    video_1920x1080_60Hz_vic16,    

    video_3840x2160_24Hz_vic207,
    video_3840x2160_25Hz_vic206,
    video_3840x2160_30Hz_vic205,
    video_1024x768_60Hz_vic0,
    video_1280x1024_60Hz_vic0,
    video_1600x1200_60Hz_vic0
};

#define Tx_Out_Mode             HDMI_OUT
#define Video_Input_Mode        Input_BT1120_16BIT
#define Audio_Input_Mode        I2S_2CH
#define Video_Output_Mode       Output_RGB888
#define USE_DDRCLK              0

int Tx_HPD=0;
int vid_chk_flag=1;
int intb_flag=0;
UINT8 irq_flag3=0;
UINT8 irq_flag1=0;
UINT8 irq_flag0=0;
    
enum VideoFormat Video_Format;
UINT8 HDMI_VIC=0x00;
UINT8 HDMI_Y=0x00;
struct video_timing video_bt={0};

UINT16 hfp,hs_width,hbp,h_act,h_tal,vfp,vs_width,vbp,v_act,v_tal,v_pol,h_pol;
const BT_TIMING_S *g_pstTiming = NULL;

UINT8 phease_offset=0;
/*note phease_offset should be setted according to TTL timing.*/

/*************************************
 Resolution			HDMI_VIC
--------------------------------------
640x480		1
720x480P 60Hz		2
720x480i 60Hz		6

720x576P 50Hz		17
720x576i 50Hz		21

1280x720P 24Hz		60
1280x720P 25Hz		61
1280x720P 30Hz		62
1280x720P 50Hz		19
1280x720P 60Hz		4

1920x1080P 24Hz	32
1920x1080P 25Hz	33
1920x1080P 30Hz	34

1920x1080i 50Hz		20
1920x1080i 60Hz		5

1920x1080P 50Hz	31
1920x1080P 60Hz	16

3840x2160P 24Hz	207
3840x2160P 25Hz	206
3840x2160P 30Hz	205

Other resolution	0(default)

**************************************/

enum {
    _32KHz = 0,
    _44d1KHz,
    _48KHz,

    _88d2KHz,
    _96KHz,
    _176Khz,
    _196KHz
};

UINT16 IIS_N[] =
{
    4096,   // 32K
    6272,   // 44.1K
    6144,   // 48K
    12544,  // 88.2K
    12288,  // 96K
    25088,  // 176K
    24576   // 196K
};

UINT16 Sample_Freq[] =
{
    0x30,   // 32K
    0x00,   // 44.1K
    0x20,   // 48K
    0x80,   // 88.2K
    0xa0,   // 96K
    0xc0,   // 176K
    0xe0    //  196K
};

enum {
    _16bit = 1,
    _20bit,
    _24_32bit
};

#define Sampling_rate	_48KHz
#define Sampling_Size	_24_32bit


// hfp, hs, hbp,hact,htotal,vfp, vs, vbp,vact,vtotal,vic,vpol,hpol,
const struct video_timing video_640x480_60Hz     = { 16, 96,  48, 640,   800, 10,  2,  33, 480,   525,};
const struct video_timing video_720x480_60Hz     = {16, 62,  60, 720,   858,  9,  6,  30, 480,   525};
const struct video_timing video_800x600_60Hz     = {16, 62,  60, 800,   1056,  9,  6,  30, 600,   628};
const struct video_timing video_1280x720_60Hz    = {110,40, 220,1280,  1650,  6,  5,  19, 720,   750};
const struct video_timing video_1280x720_30Hz   = {736, 40, 1244,1280,  3300, 5,  5,  20, 720,   750};
const struct video_timing video_1920x1080_60Hz   = {88,44, 148,1920,  2200,  5,  5, 37, 1080,1125};
const struct video_timing video_1920x1080_50Hz   = {528,44, 148,1920,  2640,  4,  5, 36, 1080,1125};
const struct video_timing video_1920x1080_30Hz   = {528,44, 148,1920,  2640,  4,  5, 36, 1080,1125};
const struct video_timing video_1280x1024_60Hz   = {48,112, 248,1280,  1688,  1,  3, 38, 1024, 1066};
const struct video_timing video_1024x768_60Hz   = {24,136, 160,1024, 1344,  3,  6, 29, 768, 806};
//const struct video_timing video_3840x2160_24Hz   = {1276,88, 296,3840,  5500,  8,  10, 72, 2160, 2250};
//const struct video_timing video_3840x2160_25Hz   = {1056,88, 296,3840,  5280,  8,  10, 72, 2160, 2250};
const struct video_timing video_3840x2160_30Hz   = {176,88, 296,3840,  4400,  8,  10, 72, 2160, 2250};
const struct video_timing video_1600x1200_60Hz   = {64,192, 304,1600, 2160,  1,  3, 46, 1200, 1250};

static UINT32 g_u32IIC = 0;
static UINT32 g_u32Dev = 0;


UINT8 HDMI_ReadI2C_Byte(UINT8 RegAddr)
{
    UINT8 data = 0;
    IIC_Read(g_u32IIC, g_u32Dev, RegAddr, &data);

    return data;
}


void HDMI_WriteI2C_Byte(UINT8 RegAddr, UINT8 d)
{
    IIC_Write(g_u32IIC, g_u32Dev, RegAddr, d);
}


INT32 RESET_Lt8618SX(void)
{
    int fd = -1;
    int ret = -1;
    DDM_resetSet stResetPrm;
    
    fd = open(DDM_DEV_RESET_NAME, O_RDWR);
    if(fd < 0)
    {
        DISP_LOGE("open %s failed\n", DDM_DEV_RESET_NAME);
        return SAL_FAIL;
    }

    memset((char *)&stResetPrm, 0, sizeof(DDM_resetSet));
    stResetPrm.type = DDM_RESET_TYPE_DISPLAY;
    stResetPrm.subtype = DDM_DP_SUBTYPE_LT8618SX;
    stResetPrm.num = 0;                            

    ret = ioctl(fd, DDM_IOC_RESET_OFFON, &stResetPrm);
    close(fd);
    if (ret < 0)
    {
        DISP_LOGE("8618 reset fail!\n");
        return SAL_FAIL;
    }

    DISP_LOGI("8618 reset success!\n");
    return SAL_SOK;
}


void LT8618SX_Chip_ID(void)
{
    HDMI_WriteI2C_Byte(0xFF,0x80);
    HDMI_WriteI2C_Byte(0xee,0x01);
    DISP_LOGI("LT8618SX Chip ID = 0x%x 0x%x 0x%x\n",HDMI_ReadI2C_Byte(0x00), HDMI_ReadI2C_Byte(0x01), HDMI_ReadI2C_Byte(0x02));

}

void LT8618SX_RST_PD_Init(void)
{
    HDMI_WriteI2C_Byte(0xff,0x80);
    HDMI_WriteI2C_Byte(0x11,0x00); //reset MIPI Rx logic
    HDMI_WriteI2C_Byte(0x13,0xf1);
    HDMI_WriteI2C_Byte(0x13,0xf9); //reset TTL video process
}

void LT8618SX_TTL_Input_Analog(void)
{
    //TTL mode
    HDMI_WriteI2C_Byte(0xff,0x81);
    HDMI_WriteI2C_Byte(0x02,0x66);
    HDMI_WriteI2C_Byte(0x0a,0x06);//0x06
    HDMI_WriteI2C_Byte(0x15,0x06);
    HDMI_WriteI2C_Byte(0x4e,0x00);//
    
    HDMI_WriteI2C_Byte( 0xff,0x82);// ring
    HDMI_WriteI2C_Byte( 0x1b,0x77);
    HDMI_WriteI2C_Byte( 0x1c,0xec); 
}

void LT8618SX_TTL_Input_Digital(void)
{
    //TTL mode
    if(Video_Input_Mode == Input_RGB888)
    {
        DISP_LOGI("Video_Input_Mode=Input_RGB888\n");
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x45,0x00); //RGB channel swap
        if(USE_DDRCLK == 1)
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x80;  0xc0: invert dclk 
        }
        else
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: invert dclk 
        }
        HDMI_WriteI2C_Byte(0x50,0x00);
        HDMI_WriteI2C_Byte(0x51,0x00);
    }
    else if(Video_Input_Mode == Input_RGB_12BIT)
    {
        DISP_LOGI("Video_Input_Mode=Input_RGB_12BIT\n");
        HDMI_WriteI2C_Byte(0xff,0x80);
        HDMI_WriteI2C_Byte(0x0a,0x80);

        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x45,0x70); //RGB channel swap

        HDMI_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: invert dclk 

        HDMI_WriteI2C_Byte(0x50,0x00);
        HDMI_WriteI2C_Byte(0x51,0x30);
        HDMI_WriteI2C_Byte(0x40,0x00);
        HDMI_WriteI2C_Byte(0x41,0xcd);
    }
    else if(Video_Input_Mode == Input_YCbCr444)
    {
        DISP_LOGI("Video_Input_Mode=Input_YCbCr444\n");
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x45,0x70); //RGB channel swap
        HDMI_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: dclk 
    }
    else if(Video_Input_Mode==Input_YCbCr422_16BIT)
    {
        DISP_LOGI("Video_Input_Mode=Input_YCbCr422_16BIT\n");
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x45,0x00); //RGB channel swap
        if(USE_DDRCLK == 1)
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x80;  0xc0: invert dclk 
        }
        else
        {
            HDMI_WriteI2C_Byte(0x4f,0x00); //0x00;  0x40: invert dclk 
        }
    }
    
    else if(Video_Input_Mode==Input_BT1120_16BIT)
    {
        DISP_LOGI("Video_Input_Mode=Input_BT1120_16BIT\n");
        HDMI_WriteI2C_Byte(0xff,0x82);
        //HDMI_WriteI2C_Byte(0x45,0x30); // D0 ~ D7 Y ; D8 ~ D15 C
        //HDMI_WriteI2C_Byte(0x45,0x70); // D8 ~ D15 Y ; D16 ~ D23 C
        //HDMI_WriteI2C_Byte(0x45,0x00); // D0 ~ D7 C ; D8 ~ D15 Y
        HDMI_WriteI2C_Byte(0x45,0x60); // D8 ~ D15 C ; D16 ~ D23 Y
        if(USE_DDRCLK == 1)
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x80;  0xc0: invert dclk 
        }
        else
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: invert dclk 
        }
        HDMI_WriteI2C_Byte(0x48,0x08); //0x1c  0x08 Embedded sync mode input enable.
        HDMI_WriteI2C_Byte(0x51,0x42); //0x02 0x43 0x34 embedded DE mode input edable.
        HDMI_WriteI2C_Byte(0x47,0x07);
    }
    else if(Video_Input_Mode==Input_BT656_8BIT)
    {
        DISP_LOGI("Video_Input_Mode=Input_BT656_8BIT\n");
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x45,0x00); //RGB channel swap
        if(USE_DDRCLK == 1)
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x80;  0xc0: dclk
            HDMI_WriteI2C_Byte(0x48,0x5c);//0x50 0x5c 0x40 0x48 
        }
        else
        {
            HDMI_WriteI2C_Byte(0x4f,0x40); //0x00;  0x40: dclk 
            HDMI_WriteI2C_Byte(0x48,0x48);
        }
        HDMI_WriteI2C_Byte(0x51,0x42); 
        HDMI_WriteI2C_Byte(0x47,0x87); 
    }		
    else if(Video_Input_Mode==Input_BT601_8BIT)
    {
        DISP_LOGI("Video_Input_Mode=Input_BT601_8BIT\n");
        HDMI_WriteI2C_Byte(0xff,0x81);
        HDMI_WriteI2C_Byte(0x0a,0x90);

        HDMI_WriteI2C_Byte(0xff,0x81);
        HDMI_WriteI2C_Byte(0x4e,0x02);

        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x45,0x00); //RGB channel swap
        if(USE_DDRCLK == 1)
        {
            HDMI_WriteI2C_Byte(0x4f,0xc0); //0x80;  0xc0: dclk
            HDMI_WriteI2C_Byte(0x48,0x5c);//0x50 0x5c 0x40 0x48 
        }
        else
        {
            HDMI_WriteI2C_Byte(0x4f,0xc0); //0x00;  0x40: dclk 
            HDMI_WriteI2C_Byte(0x48,0x40);
        }
        HDMI_WriteI2C_Byte(0x51,0x00); 
        HDMI_WriteI2C_Byte(0x47,0x87); 
    }
}


UINT32 LT8618SX_CLK_Det(void)
{
    UINT32 dclk_;
    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x80);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad ttl dclk = %d\n", dclk_);

    return dclk_;
}


int LT8618SX_PLL_Version_U3(void)
{
    UINT8 read_val;
    UINT8 j;
    UINT8 cali_done;
    UINT8 cali_val;
    UINT8 lock;
    UINT32 dclk;

    dclk=LT8618SX_CLK_Det();
    
    if(USE_DDRCLK == 1)
    {
        dclk=dclk*2;
    }
    
    if(Video_Input_Mode==Input_RGB_12BIT||Video_Input_Mode==Input_RGB888 || Video_Input_Mode==Input_YCbCr444||Video_Input_Mode==Input_YCbCr422_16BIT||Video_Input_Mode==Input_BT1120_16BIT)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x23, 0x40 );
        HDMI_WriteI2C_Byte( 0x24, 0x62 );         //0x62(u3) ,0x64 icp set
        HDMI_WriteI2C_Byte( 0x25, 0x00 );         //prediv=div(n+1)
        HDMI_WriteI2C_Byte( 0x2c, 0x9e );
        //if(INPUT_IDCK_CLK==_Less_than_50M)
        if(dclk<50000)
        {
            HDMI_WriteI2C_Byte( 0x2d, 0xaa );       //[5:4]divx_set //[1:0]freq set
            DISP_LOGI("PLL LOW\n");
        } 
        //else if(INPUT_IDCK_CLK==_Bound_50_100M)
        else if((dclk>50000)&&(dclk<100000))
        {
            HDMI_WriteI2C_Byte( 0x2d, 0x99 );       //[5:4] divx_set //[1:0]freq set
            DISP_LOGI("PLL MID\n");
        }
        //else if(INPUT_IDCK_CLK==_Greater_than_100M)
        else
        {
            HDMI_WriteI2C_Byte( 0x2d, 0x88 );       //[5:4] divx_set //[1:0]freq set
            DISP_LOGI("PLL HIGH\n");
        }
        HDMI_WriteI2C_Byte( 0x26, 0x55 );
        HDMI_WriteI2C_Byte( 0x27, 0x66 );   //phase selection for d_clk
        HDMI_WriteI2C_Byte( 0x28, 0x88 );

        HDMI_WriteI2C_Byte( 0x29, 0x04 );   //for U3 for U3 SDR/DDR fixed phase
            
    }
    else if(Video_Input_Mode==Input_BT656_8BIT)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x23, 0x40 );
        HDMI_WriteI2C_Byte( 0x24, 0x61 );         //icp set
        HDMI_WriteI2C_Byte( 0x25, 0x00 );         //prediv=div(n+1)
        HDMI_WriteI2C_Byte( 0x2c, 0x9e );
        //if(INPUT_IDCK_CLK==_Less_than_50M)
        if(dclk<50000)
        {
            HDMI_WriteI2C_Byte( 0x2d, 0xaa );       //[5:4]divx_set //[1:0]freq set
            DISP_LOGI("PLL LOW\n");
        } 
        //else if(INPUT_IDCK_CLK==_Bound_50_100M)
        else if((dclk>50000)&&(dclk<100000))
        {
            HDMI_WriteI2C_Byte( 0x2d, 0x99 );       //[5:4] divx_set //[1:0]freq set
            DISP_LOGI("PLL MID\n");
        }
        //else if(INPUT_IDCK_CLK==_Greater_than_100M)
        else
        {
            HDMI_WriteI2C_Byte( 0x2d, 0x88 );       //[5:4] divx_set //[1:0]freq set
            DISP_LOGI("PLL HIGH\n");
        }
        HDMI_WriteI2C_Byte( 0x26, 0x55 );
        HDMI_WriteI2C_Byte( 0x27, 0x66 );   //phase selection for d_clk
        HDMI_WriteI2C_Byte( 0x28, 0xa9 );

        HDMI_WriteI2C_Byte( 0x29, 0x04 );   //for U3 for U3 SDR/DDR fixed phase
    }
        
        
    HDMI_WriteI2C_Byte( 0xff, 0x81 );
    read_val=HDMI_ReadI2C_Byte(0x2b);
    HDMI_WriteI2C_Byte( 0x2b, read_val&0xfd );// sw_en_txpll_cal_en
    read_val=HDMI_ReadI2C_Byte(0x2e);
    HDMI_WriteI2C_Byte( 0x2e, read_val&0xfe );//sw_en_txpll_iband_set

    HDMI_WriteI2C_Byte( 0xff, 0x82 );
    HDMI_WriteI2C_Byte( 0xde, 0x00 );
    HDMI_WriteI2C_Byte( 0xde, 0xc0 );

    HDMI_WriteI2C_Byte( 0xff, 0x80 );
    HDMI_WriteI2C_Byte( 0x16, 0xf1 );
    HDMI_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
    HDMI_WriteI2C_Byte( 0x18, 0xfc );
    HDMI_WriteI2C_Byte( 0x16, 0xf3 );	

    if(USE_DDRCLK == 1)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x27, 0x60 );   //phase selection for d_clk
        HDMI_WriteI2C_Byte( 0x4d, 0x05 );//
        HDMI_WriteI2C_Byte( 0x2a, 0x10 );//
        HDMI_WriteI2C_Byte( 0x2a, 0x30 );//sync rest
    }
    else
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x27, 0x66 );   //phase selection for d_clk
        HDMI_WriteI2C_Byte( 0x2a, 0x00 );//
        HDMI_WriteI2C_Byte( 0x2a, 0x20 );//sync rest
    }
    for(j=0;j<0x05;j++)
    {
        SAL_msleep(10);
        HDMI_WriteI2C_Byte(0xff,0x80);	
        HDMI_WriteI2C_Byte(0x16,0xe3); /* pll lock logic reset */
        HDMI_WriteI2C_Byte(0x16,0xf3);

        HDMI_WriteI2C_Byte( 0xff, 0x82 );
        lock=0x80&HDMI_ReadI2C_Byte(0x15);	
        cali_val=HDMI_ReadI2C_Byte(0xea);	
        cali_done=0x80&HDMI_ReadI2C_Byte(0xeb);
        if(lock&&cali_done&&(cali_val!=0xff))
        {	
#ifdef _DEBUG_MODE
            HDMI_WriteI2C_Byte( 0xff, 0x82 );
            DISP_LOGI("0x8215=0x%x\n",HDMI_ReadI2C_Byte(0x15));
            DISP_LOGI("0x82e6=0x%x\n",HDMI_ReadI2C_Byte(0xe6));	
            DISP_LOGI("0x82e7=0x%x\n",HDMI_ReadI2C_Byte(0xe7));	
            DISP_LOGI("0x82e8=0x%x\n",HDMI_ReadI2C_Byte(0xe8));	
            DISP_LOGI("0x82e9=0x%x\n",HDMI_ReadI2C_Byte(0xe9));	
            DISP_LOGI("0x82ea=0x%x\n",HDMI_ReadI2C_Byte(0xea));	
            DISP_LOGI("0x82eb=0x%x\n",HDMI_ReadI2C_Byte(0xeb));	
            DISP_LOGI("0x82ec=0x%x\n",HDMI_ReadI2C_Byte(0xec));	
            DISP_LOGI("0x82ed=0x%x\n",HDMI_ReadI2C_Byte(0xed));	
            DISP_LOGI("0x82ee=0x%x\n",HDMI_ReadI2C_Byte(0xee));	
            DISP_LOGI("0x82ef=0x%x\n",HDMI_ReadI2C_Byte(0xef));
#endif										
            DISP_LOGI("TXPLL Lock\n");

            if(USE_DDRCLK == 1)
            {
                HDMI_WriteI2C_Byte( 0xff, 0x81 );
                HDMI_WriteI2C_Byte( 0x4d, 0x05 );//
                HDMI_WriteI2C_Byte( 0x2a, 0x10 );//
                HDMI_WriteI2C_Byte( 0x2a, 0x30 );//sync rest
            }
            else
            {
                HDMI_WriteI2C_Byte( 0xff, 0x81 );
                HDMI_WriteI2C_Byte( 0x2a, 0x00 );//
                HDMI_WriteI2C_Byte( 0x2a, 0x20 );//sync rest
            }
            return 1;
        }
        else
        {
            HDMI_WriteI2C_Byte( 0xff, 0x80 );
            HDMI_WriteI2C_Byte( 0x16, 0xf1 );
            HDMI_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
            HDMI_WriteI2C_Byte( 0x18, 0xfc );
            HDMI_WriteI2C_Byte( 0x16, 0xf3 );	
            DISP_LOGI("TXPLL Reset\n");
        }
    }
    DISP_LOGI("TXPLL Unlock\n");
    return 0;
}

#if 1
int LT8618SX_PLL_Version_U2(void)
{
    UINT8 read_val;
    UINT8 j;
    UINT8 cali_done;
    UINT8 cali_val;
    UINT8 lock;
    UINT32 dclk;

    dclk=LT8618SX_CLK_Det();
    
    if(Video_Input_Mode==Input_RGB888 || Video_Input_Mode==Input_YCbCr444||Video_Input_Mode==Input_YCbCr422_16BIT||Video_Input_Mode==Input_BT1120_16BIT)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x23, 0x40 );
        HDMI_WriteI2C_Byte( 0x24, 0x62 );               //icp set
        HDMI_WriteI2C_Byte( 0x26, 0x55 );
        
        if(dclk<=25000)
        {
            HDMI_WriteI2C_Byte( 0x25, 0x00 );
            //HDMI_WriteI2C_Byte( 0x2c, 0xA8 );
            HDMI_WriteI2C_Byte( 0x2c, 0x94 );
            HDMI_WriteI2C_Byte( 0x2d, 0xaa );
        }
        else if((dclk>25000)&&(dclk<=50000))
        {
            HDMI_WriteI2C_Byte( 0x25, 0x00 );
            HDMI_WriteI2C_Byte( 0x2d, 0xaa );
            HDMI_WriteI2C_Byte( 0x2c, 0x94 );
        }
        else if((dclk>50000)&&(dclk<=100000))
        {
            HDMI_WriteI2C_Byte( 0x25, 0x01 );
            HDMI_WriteI2C_Byte( 0x2d, 0x99 );
            HDMI_WriteI2C_Byte( 0x2c, 0x94 );

            DISP_LOGI("50~100m\n");
        }

        else //if(dclk>100000)
        {
            HDMI_WriteI2C_Byte( 0x25, 0x03 );
            HDMI_WriteI2C_Byte( 0x2d, 0x88 );
            HDMI_WriteI2C_Byte( 0x2c, 0x94 );
        }

        if( USE_DDRCLK )
        {
            read_val=HDMI_ReadI2C_Byte(0x2c) &0x7f;
            read_val=read_val*2|0x80;
            HDMI_WriteI2C_Byte( 0x2c, read_val );
            DISP_LOGI("0x812c=%x\n",(UINT16)read_val);
            HDMI_WriteI2C_Byte( 0x4d, 0x05 );
            HDMI_WriteI2C_Byte( 0x27, 0x66 );                                               //0x60 //ddr 0x66
            HDMI_WriteI2C_Byte( 0x28, 0x88 ); 
            DISP_LOGI("PLL DDR\n");				 
        }
        else
        {
            HDMI_WriteI2C_Byte( 0x4d, 0x01 );
            HDMI_WriteI2C_Byte( 0x27, 0x60 ); //0x06                                              //0x60 //ddr 0x66
            HDMI_WriteI2C_Byte( 0x28, 0x88 );                                               // 0x88
            DISP_LOGI("PLL SDR\n");
        }
    }
    
    else if(Video_Input_Mode==Input_BT656_8BIT||Video_Input_Mode==Input_BT656_10BIT||Video_Input_Mode==Input_BT656_12BIT)
    {
        ;
    }
            
    // as long as changing the resolution or changing the input clock,	You need to configure the following registers.
    HDMI_WriteI2C_Byte( 0xff, 0x81 );
    read_val=HDMI_ReadI2C_Byte(0x2b);
    HDMI_WriteI2C_Byte( 0x2b, read_val&0xfd );// sw_en_txpll_cal_en
    read_val=HDMI_ReadI2C_Byte(0x2e);
    HDMI_WriteI2C_Byte( 0x2e, read_val&0xfe );//sw_en_txpll_iband_set

    HDMI_WriteI2C_Byte( 0xff, 0x82 );
    HDMI_WriteI2C_Byte( 0xde, 0x00 );
    HDMI_WriteI2C_Byte( 0xde, 0xc0 );

    HDMI_WriteI2C_Byte( 0xff, 0x80 );
    HDMI_WriteI2C_Byte( 0x16, 0xf1 );
    HDMI_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
    HDMI_WriteI2C_Byte( 0x18, 0xfc );
    HDMI_WriteI2C_Byte( 0x16, 0xf3 );	
    for(j=0;j<0x05;j++)
    {
        SAL_msleep(10);
        HDMI_WriteI2C_Byte(0xff,0x80);	
        HDMI_WriteI2C_Byte(0x16,0xe3); /* pll lock logic reset */
        HDMI_WriteI2C_Byte(0x16,0xf3);

        HDMI_WriteI2C_Byte( 0xff, 0x82 );
        lock=0x80&HDMI_ReadI2C_Byte(0x15);	
        cali_val=HDMI_ReadI2C_Byte(0xea);	
        cali_done=0x80&HDMI_ReadI2C_Byte(0xeb);
        if(lock&&cali_done&&(cali_val!=0xff))
        {	
#ifdef _DEBUG_MODE
            HDMI_WriteI2C_Byte( 0xff, 0x82 );
            DISP_LOGI("0x82ea=0x%x\n",HDMI_ReadI2C_Byte(0xea));	
            DISP_LOGI("0x82eb=0x%x\n",HDMI_ReadI2C_Byte(0xeb));	
            DISP_LOGI("0x82ec=0x%x\n",HDMI_ReadI2C_Byte(0xec));	
            DISP_LOGI("0x82ed=0x%x\n",HDMI_ReadI2C_Byte(0xed));	
            DISP_LOGI("0x82ee=0x%x\n",HDMI_ReadI2C_Byte(0xee));	
            DISP_LOGI("0x82ef=0x%x\n",HDMI_ReadI2C_Byte(0xef));	
#endif										
            DISP_LOGI("TXPLL Lock\n");
        return 1;
        }
        else
        {
            HDMI_WriteI2C_Byte( 0xff, 0x80 );
            HDMI_WriteI2C_Byte( 0x16, 0xf1 );
            HDMI_WriteI2C_Byte( 0x18, 0xdc );//txpll _sw_rst_n
            HDMI_WriteI2C_Byte( 0x18, 0xfc );
            HDMI_WriteI2C_Byte( 0x16, 0xf3 );	
            DISP_LOGI("TXPLL Reset\n");
        }
    }
    DISP_LOGI("TXPLL Unlock\n");
    return 0;
}
#endif


int LT8618SX_PLL(void)
{
    UINT8 read_val=0;
    HDMI_WriteI2C_Byte(0xff,0x80);
    read_val=HDMI_ReadI2C_Byte(0x02);//get IC Version
    if(read_val==0xe1)
    {
        LT8618SX_PLL_Version_U2();
        DISP_LOGI("chip u2c\n");
    }
    else if(read_val==0xe2)
    {
        LT8618SX_PLL_Version_U3();
        DISP_LOGI("chip u3c\n");
    }
    else
    {
        DISP_LOGI("fail version=%x\n",read_val);
    }
    return 0;
}


void LT8618SX_Audio_Init(void)
{
    if(Audio_Input_Mode==I2S_2CH)
    {
        // IIS Input
        HDMI_WriteI2C_Byte( 0xff, 0x82 );   // register bank
        HDMI_WriteI2C_Byte( 0xd6, Tx_Out_Mode|0x0e );   // bit7 = 0 : DVI output; bit7 = 1: HDMI output
        HDMI_WriteI2C_Byte( 0xd7, 0x04 );   

        HDMI_WriteI2C_Byte( 0xff, 0x84 );   // register bank
        HDMI_WriteI2C_Byte( 0x06, 0x08 );
        HDMI_WriteI2C_Byte( 0x07, 0x10 );	
        HDMI_WriteI2C_Byte( 0x10, 0x15 );
        HDMI_WriteI2C_Byte( 0x12, 0x60 );

        HDMI_WriteI2C_Byte( 0x0f, 0x0b + Sample_Freq[_32KHz] );
        HDMI_WriteI2C_Byte( 0x34, 0xd4 );   //CTS_N / 2; 32bit(24bit)
        //	HDMI_WriteI2C_Byte( 0x34, 0xd5 );	//CTS_N / 4; 16bit
        HDMI_WriteI2C_Byte( 0x35, (UINT8)( IIS_N[_32KHz] / 0x10000 ) );
        HDMI_WriteI2C_Byte( 0x36, (UINT8)( ( IIS_N[_32KHz] & 0x00FFFF ) / 0x100 ) );
        HDMI_WriteI2C_Byte( 0x37, (UINT8)( IIS_N[_32KHz] & 0x0000FF ) );
        HDMI_WriteI2C_Byte( 0x3c, 0x21 );   // Null packet enable
    }
    else if(Audio_Input_Mode==SPDIF)///
    {
        DISP_LOGI("Audio inut = SPDIF\n");
        HDMI_WriteI2C_Byte(0xff,0x84);
        HDMI_WriteI2C_Byte(0x06,0x0c);
        HDMI_WriteI2C_Byte(0x07,0x10);	
        HDMI_WriteI2C_Byte(0x34,0xd4); //CTS_N
    } 
}

void LT8618SX_HDCP_Init( void )         //luodexing
{
    UINT8 read_val=0;

    HDMI_WriteI2C_Byte( 0xff, 0x80 );
    HDMI_WriteI2C_Byte( 0x13, 0xf8 );
    HDMI_WriteI2C_Byte( 0x13, 0xf9 );

    HDMI_WriteI2C_Byte( 0xff, 0x85 );
    HDMI_WriteI2C_Byte( 0x15, 0x15 );//bit3
    HDMI_WriteI2C_Byte( 0x15, 0x05 );//0x45

    HDMI_WriteI2C_Byte( 0xff, 0x85 );
    HDMI_WriteI2C_Byte( 0x17, 0x0f );
    HDMI_WriteI2C_Byte( 0x0c, 0x30 );

    HDMI_WriteI2C_Byte( 0xff, 0x82 );
    read_val=HDMI_ReadI2C_Byte( 0xd6 );
    DISP_LOGI(" hdcp 0x82d6=0x%x\n",read_val);
    if(read_val&0x80)//hdmi mode
    {
        HDMI_WriteI2C_Byte( 0xff, 0x85 );
        HDMI_WriteI2C_Byte( 0x13, 0x3c );//bit3
    }
    else//dvi mode
    {
        HDMI_WriteI2C_Byte( 0xff, 0x85 );
        HDMI_WriteI2C_Byte( 0x13, 0x34 );//bit3
    }

    HDMI_WriteI2C_Byte(0xff,0x84);
    HDMI_WriteI2C_Byte(0x10,0x2c);
    HDMI_WriteI2C_Byte(0x12,0x64);    

}

void LT8618SX_HDCP_Enable( int HDCP_EN )       //luodexing
{
    
    if(HDCP_EN)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x85 );
        HDMI_WriteI2C_Byte( 0x15, 0x05 );//bit3
        HDMI_WriteI2C_Byte( 0x15, 0x65 );
        DISP_LOGI(" hdcp en\n");

    }
    else
    {
        HDMI_WriteI2C_Byte( 0xff, 0x85 );
        HDMI_WriteI2C_Byte( 0x15, 0x15 );//bit3
        HDMI_WriteI2C_Byte( 0x15, 0x05 );//0x45
        HDMI_WriteI2C_Byte( 0xff, 0x80 );
        HDMI_WriteI2C_Byte( 0x13,0xf8 );
        HDMI_WriteI2C_Byte( 0x13,0xf9 );
        DISP_LOGI(" hdcp dis\n");
    }
}

/***********************************************************/

void LT8618SX_HDCP_Disable( void )      //luodexing
{
    HDMI_WriteI2C_Byte( 0xff, 0x85 );
    HDMI_WriteI2C_Byte( 0x15, 0x45 );   //enable HDCP
}

void LT8618SX_IRQ_Init(void)
{
    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x10,0x00); //Output low level active;
    HDMI_WriteI2C_Byte(0x58,0x02); //Det HPD

    //HDMI_WriteI2C_Byte(0x9e,0xff); //vid chk clk
    HDMI_WriteI2C_Byte(0x9e,0xf7);

    HDMI_WriteI2C_Byte(0x00,0xfe);   //mask0 vid_change
#ifdef _HDCP
    HDMI_WriteI2C_Byte(0x01,0xed);   //mask1 bit1:~tx_auth_pass bit4:tx_auth_done
#endif
    HDMI_WriteI2C_Byte(0x03,0x3f); //mask3  //tx_det

    HDMI_WriteI2C_Byte(0x02,0xff); //mask2


    HDMI_WriteI2C_Byte(0x04,0xff); //clear0
    HDMI_WriteI2C_Byte(0x04,0xfe); //clear0
#ifdef _HDCP
    HDMI_WriteI2C_Byte(0x05,0xff); //clear1
    HDMI_WriteI2C_Byte(0x05,0xed); //clear1
#endif		
    HDMI_WriteI2C_Byte(0x07,0xff); //clear3
    HDMI_WriteI2C_Byte(0x07,0x3f); //clear3
}

int LT8618SX_HPD_Status(void)
{
    HDMI_WriteI2C_Byte(0xFF,0x80);
    HDMI_WriteI2C_Byte(0xee,0x01);
    HDMI_WriteI2C_Byte(0xFF,0x81);
    HDMI_WriteI2C_Byte(0x49,HDMI_ReadI2C_Byte(0x49)&0xfb);
    HDMI_WriteI2C_Byte(0xff,0x82);
    if(HDMI_ReadI2C_Byte(0x5e)&0x04)
    {
        Tx_HPD=1;
        DISP_LOGI("hpd:high\n");
    }
    else
    {
        Tx_HPD=0;
        DISP_LOGI("hpd:low\n");
    }
    return Tx_HPD;
}

void LT8618SX_HDMI_Out_Enable(int en)
{
    HDMI_WriteI2C_Byte(0xff,0x81);
    if(en)
    {
        HDMI_WriteI2C_Byte(0x30,0xea);
        DISP_LOGI("HDMI output Enable\n");
    }
    else
    {
        HDMI_WriteI2C_Byte(0x30,0x00);
        DISP_LOGI("HDMI output Disable\n");
    }
}

void LT8618SX_CSC(void)
{
    if(Video_Output_Mode == Output_RGB888)
    {
        HDMI_Y=0;
        HDMI_WriteI2C_Byte(0xff,0x82);
        if(Video_Input_Mode == Input_YCbCr444)
        {
            HDMI_WriteI2C_Byte(0xb9,0x08); //YCbCr to RGB
        }
        else if(Video_Input_Mode==Input_YCbCr422_16BIT||
        Video_Input_Mode==Input_BT1120_16BIT||
        Video_Input_Mode==Input_BT1120_20BIT||
        Video_Input_Mode==Input_BT1120_24BIT||
        Video_Input_Mode==Input_BT656_8BIT ||
        Video_Input_Mode==Input_BT656_10BIT||
        Video_Input_Mode==Input_BT656_12BIT||
        Video_Input_Mode==Input_BT601_8BIT )
        {
            HDMI_WriteI2C_Byte(0xb9,0x18); //YCbCr to RGB,YCbCr 422 convert to YCbCr 444
        }
        else
        {
            HDMI_WriteI2C_Byte(0xb9,0x00); //No csc
        }
    }
    else if(Video_Output_Mode == Output_YCbCr444)
    {
        HDMI_Y = 1;
    }
    else if(Video_Output_Mode == Output_YCbCr422)
    {
        HDMI_Y = 2;
    }
}

void LT8618SX_HDMI_TX_Digital( void )
{
    //AVI
    UINT8	AVI_PB0	   = 0x00;
    UINT8	AVI_PB1	   = 0x00;
    UINT8	AVI_PB2	   = 0x00;

    AVI_PB1 = ( HDMI_Y << 5 ) + 0x10;

    AVI_PB2 = 0x2A;  // 16:9
    //AVI_PB2 = 0x19;// 4:3

    AVI_PB0 = ( ( AVI_PB1 + AVI_PB2 + HDMI_VIC ) <= 0x6f ) ? ( 0x6f - AVI_PB1 - AVI_PB2 - HDMI_VIC ) : ( 0x16f - AVI_PB1 - AVI_PB2 - HDMI_VIC );

    HDMI_WriteI2C_Byte( 0xff, 0x84 );
    HDMI_WriteI2C_Byte( 0x43, AVI_PB0 );    //AVI_PB0
    HDMI_WriteI2C_Byte( 0x44, AVI_PB1 );    //AVI_PB1
    HDMI_WriteI2C_Byte( 0x45, AVI_PB2 );    //AVI_PB2
    HDMI_WriteI2C_Byte( 0x47, HDMI_VIC );   //AVI_PB4

    HDMI_WriteI2C_Byte( 0x10, 0x32 );       //data iland
    HDMI_WriteI2C_Byte( 0x12, 0x64 );       //act_h_blank

    //VS_IF, 4k 30hz need send VS_IF packet. Please refer to hdmi1.4 spec 8.2.3
    if( HDMI_VIC == 95 )
    {
//	   LT8618SXB_I2C_Write_Byte(0xff,0x84);
        HDMI_WriteI2C_Byte( 0x3d, 0x2a );   //UD1 infoframe enable

        HDMI_WriteI2C_Byte( 0x74, 0x81 );
        HDMI_WriteI2C_Byte( 0x75, 0x01 );
        HDMI_WriteI2C_Byte( 0x76, 0x05 );
        HDMI_WriteI2C_Byte( 0x77, 0x49 );
        HDMI_WriteI2C_Byte( 0x78, 0x03 );
        HDMI_WriteI2C_Byte( 0x79, 0x0c );
        HDMI_WriteI2C_Byte( 0x7a, 0x00 );
        HDMI_WriteI2C_Byte( 0x7b, 0x20 );
        HDMI_WriteI2C_Byte( 0x7c, 0x01 );
    }else
    {
//	   LT8618SXB_I2C_Write_Byte(0xff,0x84);
        HDMI_WriteI2C_Byte( 0x3d, 0x0a ); //UD1 infoframe disable
    }


    //AVI_audio
    HDMI_WriteI2C_Byte( 0xff, 0x84 );
    HDMI_WriteI2C_Byte( 0xb2, 0x84 );
    HDMI_WriteI2C_Byte( 0xb3, 0x01 );
    HDMI_WriteI2C_Byte( 0xb4, 0x0a );
    HDMI_WriteI2C_Byte( 0xb5, 0x60 - ( ( ( Sampling_rate + 1 ) << 2 ) + Sampling_Size ) );  //checksum
    HDMI_WriteI2C_Byte( 0xb6, 0x11 );                                                       //AVI_PB0//LPCM
    HDMI_WriteI2C_Byte( 0xb7, ( ( Sampling_rate + 1 ) << 2 ) + Sampling_Size );             //AVI_PB1//32KHz 24bit(32bit)
}
void LT8618SX_HDMI_TX_Phy(void) 
{
    HDMI_WriteI2C_Byte(0xff,0x81);
    HDMI_WriteI2C_Byte(0x30,0xea);//0xea
    HDMI_WriteI2C_Byte(0x31,0x44);//DC: 0x44, AC:0x73
    HDMI_WriteI2C_Byte(0x32,0x4a);
    HDMI_WriteI2C_Byte(0x33,0x0b);
    HDMI_WriteI2C_Byte(0x34,0x00);//d0 pre emphasis
    HDMI_WriteI2C_Byte(0x35,0x00);//d1 pre emphasis
    HDMI_WriteI2C_Byte(0x36,0x00);//d2 pre emphasis
    HDMI_WriteI2C_Byte(0x37,0x44);
    HDMI_WriteI2C_Byte(0x3f,0x0f);
    HDMI_WriteI2C_Byte(0x40,0xb0);//clk swing
    HDMI_WriteI2C_Byte(0x41,0xa0);//d0 swing
    HDMI_WriteI2C_Byte(0x42,0xa0);//d1 swing
    HDMI_WriteI2C_Byte(0x43,0xa0); //d2 swing
    HDMI_WriteI2C_Byte(0x44,0x0a);	
}

void LT8618SX_BT_Set(void)
{
    if(Video_Input_Mode == Input_BT1120_16BIT)
    {
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x20,((video_bt.hact)>>8)); 
        HDMI_WriteI2C_Byte(0x21,(video_bt.hact)); 
        HDMI_WriteI2C_Byte(0x22,((video_bt.hfp)>>8)); 
        HDMI_WriteI2C_Byte(0x23,(video_bt.hfp)); 
        HDMI_WriteI2C_Byte(0x24,((video_bt.hs)>>8)); 
        HDMI_WriteI2C_Byte(0x25,(video_bt.hs)); 
        HDMI_WriteI2C_Byte(0x26,0x00);
        HDMI_WriteI2C_Byte(0x27,0x00);
        HDMI_WriteI2C_Byte(0x36,(video_bt.vact>>8));
        HDMI_WriteI2C_Byte(0x37,video_bt.vact);
        HDMI_WriteI2C_Byte(0x38,(video_bt.vfp>>8));
        HDMI_WriteI2C_Byte(0x39,video_bt.vfp);
        HDMI_WriteI2C_Byte(0x3a,(video_bt.vbp>>8));
        HDMI_WriteI2C_Byte(0x3b,video_bt.vbp);
        HDMI_WriteI2C_Byte(0x3c,(video_bt.vs>>8));
        HDMI_WriteI2C_Byte(0x3d,video_bt.vs);
    }
    else if(Video_Input_Mode == Input_BT601_8BIT)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x82 );
        HDMI_WriteI2C_Byte( 0x2c,((2*video_bt.htotal)>>8));
        HDMI_WriteI2C_Byte( 0x2d,(2*video_bt.htotal));
        HDMI_WriteI2C_Byte( 0x2e,((2*video_bt.hact)>>8));
        HDMI_WriteI2C_Byte( 0x2f,(2*video_bt.hact));
        HDMI_WriteI2C_Byte( 0x30,((2*video_bt.hfp)>>8));
        HDMI_WriteI2C_Byte( 0x31,(2*video_bt.hfp));
        HDMI_WriteI2C_Byte( 0x32,((2*video_bt.hbp)>>8));
        HDMI_WriteI2C_Byte( 0x33,(2*video_bt.hbp));
        HDMI_WriteI2C_Byte( 0x34,((2*video_bt.hs)>>8));
        HDMI_WriteI2C_Byte( 0x35,(2*video_bt.hs));

        HDMI_WriteI2C_Byte( 0x36,((video_bt.vact)>>8));
        HDMI_WriteI2C_Byte( 0x37,(video_bt.vact));
        HDMI_WriteI2C_Byte( 0x38,((video_bt.vfp)>>8));
        HDMI_WriteI2C_Byte( 0x39,(video_bt.vfp));
        HDMI_WriteI2C_Byte( 0x3a,((video_bt.vbp)>>8));
        HDMI_WriteI2C_Byte( 0x3b,(video_bt.vbp));
        HDMI_WriteI2C_Byte( 0x3c,((video_bt.vs)>>8));
        HDMI_WriteI2C_Byte( 0x3d,(video_bt.vs));
    }
    else if(Video_Input_Mode == Input_BT656_8BIT)
    {
        hs_width *= 2;
        hbp *= 2;
        hfp *= 2;
        h_tal *= 2;
        h_act *= 2;
            
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte(0x20,((video_bt.hact)>>8)); 
        HDMI_WriteI2C_Byte(0x21,(video_bt.hact)); 
        HDMI_WriteI2C_Byte(0x22,((video_bt.hfp)>>8)); 
        HDMI_WriteI2C_Byte(0x23,(video_bt.hfp)); 
        HDMI_WriteI2C_Byte(0x24,((video_bt.hs)>>8)); 
        HDMI_WriteI2C_Byte(0x25,(video_bt.hs)); 
        HDMI_WriteI2C_Byte(0x26,0x00);
        HDMI_WriteI2C_Byte(0x27,0x00);
        HDMI_WriteI2C_Byte(0x36,(video_bt.vact>>8));
        HDMI_WriteI2C_Byte(0x37,video_bt.vact);
        HDMI_WriteI2C_Byte(0x38,(video_bt.vfp>>8));
        HDMI_WriteI2C_Byte(0x39,video_bt.vfp);
        HDMI_WriteI2C_Byte(0x3a,(video_bt.vbp>>8));
        HDMI_WriteI2C_Byte(0x3b,video_bt.vbp);
        HDMI_WriteI2C_Byte(0x3c,(video_bt.vs>>8));
        HDMI_WriteI2C_Byte(0x3d,video_bt.vs);
    }
}


UINT8 LT8618SX_Video_Check(void)
{

    UINT8 temp;

    hfp = 0;
    hs_width = 0;
    hbp = 0;
    h_act = 0;
    h_tal = 0;
    vfp = 0;
    vs_width = 0;
    vbp = 0;
    v_act = 0;
    v_tal = 0;
    
    HDMI_WriteI2C_Byte( 0xff, 0x80 );
    HDMI_WriteI2C_Byte( 0x13, 0xf1 );//ttl video process reset
    HDMI_WriteI2C_Byte( 0x12, 0xfb );//video check reset
    SAL_msleep(1);
    HDMI_WriteI2C_Byte( 0x12, 0xff );
    HDMI_WriteI2C_Byte( 0x13, 0xf9 );
    SAL_msleep(100);
    
    if((Video_Input_Mode==Input_BT601_8BIT)||(Video_Input_Mode==Input_RGB888)||(Video_Input_Mode==Input_YCbCr422_16BIT)||(Video_Input_Mode==Input_YCbCr444))/*extern sync*/
    {
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte( 0x51, 0x00 );
        
        HDMI_WriteI2C_Byte(0xff,0x82); //video check
        temp=HDMI_ReadI2C_Byte(0x70);  //hs vs polarity

        if(temp&0x02)
        {
            DISP_LOGI(" vs_pol is 1\n");
        }
        else
        {
            DISP_LOGI(" vs_pol is 0\n");
        }
        if( temp & 0x01 )
        {
            DISP_LOGI(" hs_pol is 1\n");
        }
        else
        {
            DISP_LOGI(" hs_pol is 0\n");
        } 
            
        vs_width = HDMI_ReadI2C_Byte(0x71);
        hs_width = HDMI_ReadI2C_Byte(0x72);
        hs_width = ( (hs_width & 0x0f) << 8 ) + HDMI_ReadI2C_Byte(0x73);
        vbp = HDMI_ReadI2C_Byte(0x74);
        vfp = HDMI_ReadI2C_Byte(0x75);
        hbp = HDMI_ReadI2C_Byte(0x76);
        hbp = ( (hbp & 0x0f) << 8 ) + HDMI_ReadI2C_Byte(0x77);
        hfp = HDMI_ReadI2C_Byte(0x78);
        hfp = ( (hfp & 0x0f) << 8 ) + HDMI_ReadI2C_Byte(0x79);
        v_tal = HDMI_ReadI2C_Byte(0x7a);
        v_tal = ( v_tal << 8 ) + HDMI_ReadI2C_Byte(0x7b);
        h_tal = HDMI_ReadI2C_Byte(0x7c);
        h_tal = ( h_tal << 8 ) + HDMI_ReadI2C_Byte(0x7d);
        v_act = HDMI_ReadI2C_Byte(0x7e);
        v_act = ( v_act << 8 ) + HDMI_ReadI2C_Byte(0x7f);
        h_act = HDMI_ReadI2C_Byte(0x80);
        h_act = ( h_act << 8 ) + HDMI_ReadI2C_Byte(0x81);
    }	  
    else if((Video_Input_Mode==Input_BT1120_16BIT)||(Video_Input_Mode==Input_BT656_8BIT))/*embbedded sync */
    {
        HDMI_WriteI2C_Byte(0xff,0x82);
        HDMI_WriteI2C_Byte( 0x51, 0x42 );
        v_act = HDMI_ReadI2C_Byte(0x8b);
        v_act = ( v_act << 8 ) + HDMI_ReadI2C_Byte(0x8c);

        h_act = HDMI_ReadI2C_Byte(0x8d);
        h_act = ( h_act << 8 ) + HDMI_ReadI2C_Byte(0x8e)-0x04;/*note -0x04*/

        h_tal= HDMI_ReadI2C_Byte(0x8f);
        h_tal = ( h_tal << 8 ) + HDMI_ReadI2C_Byte(0x90);
    }
    

    if(Video_Input_Mode==Input_BT601_8BIT||Video_Input_Mode==Input_BT656_8BIT)
    {
        hs_width/=2;
        hbp/=2;
        hfp/=2;
        h_tal/=2;
        h_act/=2;
    }
    
    if((h_act==video_640x480_60Hz.hact)&&
    (v_act==video_640x480_60Hz.vact)&&
    (h_tal==video_640x480_60Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_640x480_60Hz \n");
        Video_Format=video_640x480_60Hz_vic1;
        HDMI_VIC=1;
        video_bt= video_640x480_60Hz;
    }
    else if((h_act==video_720x480_60Hz.hact)&&
        (v_act==video_720x480_60Hz.vact)&&
        (h_tal==video_720x480_60Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_720x480_60Hz \n");
        Video_Format=video_720x480_60Hz_vic3;
        HDMI_VIC=3;
        video_bt=video_720x480_60Hz;
    }	
    else if((h_act==video_1280x720_30Hz.hact)&&
        (v_act==video_1280x720_30Hz.vact)&&
        (h_tal==video_1280x720_30Hz.htotal)
        )
    {
        DISP_LOGI("Video_Check = video_1280x720_30Hz \n");
        Video_Format=video_1280x720_30Hz_vic62;
        HDMI_VIC=0;
        video_bt=video_1280x720_30Hz;
    }
    else if((h_act==video_1280x720_60Hz.hact)&&
        (v_act==video_1280x720_60Hz.vact)&&
        (h_tal==video_1280x720_60Hz.htotal)
        )
    {
        DISP_LOGI("Video_Check = video_1280x720_60Hz \n");
        Video_Format=video_1280x720_60Hz_vic4;
        HDMI_VIC=4;
        video_bt=video_1280x720_60Hz;
    }
    else if((h_act==video_1920x1080_30Hz.hact)&&
        (v_act==video_1920x1080_30Hz.vact)&&
        (h_tal==video_1920x1080_30Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_1920x1080_30Hz \n");
        Video_Format=video_1920x1080_30Hz_vic34;
        HDMI_VIC=34;
        video_bt=video_1920x1080_30Hz;
    }
    else if((h_act==video_1920x1080_50Hz.hact)&&
    (v_act==video_1920x1080_50Hz.vact)&&
    (h_tal==video_1920x1080_50Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_1920x1080_50Hz \n");
        Video_Format=video_1920x1080_50Hz_vic31;
        HDMI_VIC=31;
        video_bt=video_1920x1080_50Hz;
    }
    else if((h_act==video_1920x1080_60Hz.hact)&&
        (v_act==video_1920x1080_60Hz.vact)&&
        (h_tal==video_1920x1080_60Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_1920x1080_60Hz \n");
        Video_Format=video_1920x1080_60Hz_vic16;
        HDMI_VIC=16;
        video_bt=video_1920x1080_60Hz;
    }
    else if((h_act==video_1024x768_60Hz.hact)&&
        (v_act==video_1024x768_60Hz.vact)&&
        (h_tal==video_1024x768_60Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_1024x768_60Hz \n");
        Video_Format=video_1024x768_60Hz_vic0;
        HDMI_VIC=0;
        video_bt=video_1024x768_60Hz;
    }
    else if((h_act==video_1280x1024_60Hz.hact)&&
        (v_act==video_1280x1024_60Hz.vact)&&
        (h_tal==video_1280x1024_60Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_1280x1024_60Hz \n");
        Video_Format=video_1280x1024_60Hz_vic0;
        HDMI_VIC=0;
        video_bt=video_1280x1024_60Hz;
    }
    else if((h_act==video_1600x1200_60Hz.hact)&&
        (v_act==video_1600x1200_60Hz.vact)&&
        (h_tal==video_1600x1200_60Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_1600x1200_60Hz \n");
        Video_Format=video_1600x1200_60Hz_vic0;
        HDMI_VIC=0;
        video_bt=video_1600x1200_60Hz;
    }
    else if((h_act==video_3840x2160_30Hz.hact)&&
        (v_act==video_3840x2160_30Hz.vact)&&
        (h_tal==video_3840x2160_30Hz.htotal))
    {
        DISP_LOGI("Video_Check = video_3840x2160_30Hz \n");
        Video_Format=video_3840x2160_30Hz_vic205;
        HDMI_VIC=16;
        video_bt=video_3840x2160_30Hz;
    }
    else
    {
        Video_Format=video_720x480_60Hz_vic3;
        HDMI_VIC=3;
        video_bt=video_720x480_60Hz;

        if (NULL != g_pstTiming)
        {
            if ((g_pstTiming->u32Width == 1920) &&
                (g_pstTiming->u32Height == 1080) &&
                (g_pstTiming->u32Fps == 60))
            {
                Video_Format=video_1920x1080_60Hz_vic16;
                HDMI_VIC=16;
                video_bt=video_1920x1080_60Hz;
            }
            else if ((g_pstTiming->u32Width == 1920) &&
                     (g_pstTiming->u32Height == 1080) &&
                     (g_pstTiming->u32Fps == 50))
            {
                Video_Format=video_1920x1080_50Hz_vic31;
                HDMI_VIC=31;
                video_bt=video_1920x1080_50Hz;
            }
        }
    }
    return Video_Format;
}


void LT8618SX_test_clk(void)
{
    UINT32 dclk_;

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x8d);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad hdmitx read clk = %d\n", dclk_);

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x8b);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad txpll pix dclk = %d\n", dclk_);

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x8c);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad hdmitx write clk = %d\n", dclk_);

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x86);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad txpll dec ddr d clk = %d\n", dclk_);

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x87);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad txpll drpt dclk = %d\n", dclk_);

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x8a);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad txpll ref dclk = %d\n", dclk_);

    HDMI_WriteI2C_Byte(0xff,0x82);
    HDMI_WriteI2C_Byte(0x17,0x8b);
    SAL_msleep(500);
    dclk_=(((HDMI_ReadI2C_Byte(0x1d))&0x0f)<<8)+HDMI_ReadI2C_Byte(0x1e);
    dclk_=(dclk_<<8)+HDMI_ReadI2C_Byte(0x1f);
    DISP_LOGI("ad txpll pix dclk = %d\n", dclk_);

}
int LT8618SX_Jug(void)
{
    UINT16		V_ACT	   = 0x0000;
    UINT16		H_ACT	   = 0x0000;
    UINT16		H_TOTAL	   = 0x0000;
    UINT16		V_TOTAL	   = 0x0000;
    
    UINT8 i=4;
    
    SAL_msleep(5);
    if(!LT8618SX_HPD_Status())
    {
    return 0;
    }
    
    while(i--)
    {
        SAL_msleep(5);
        if(Video_Input_Mode == Input_RGB888)
        {								
            HDMI_WriteI2C_Byte( 0xff, 0x82 );
            H_TOTAL	   = HDMI_ReadI2C_Byte( 0x7c );
            H_TOTAL	   = ( H_TOTAL << 8 ) + HDMI_ReadI2C_Byte( 0x7d );
            V_TOTAL	   = HDMI_ReadI2C_Byte( 0x7a );
            V_TOTAL	   = ( V_TOTAL << 8 ) + HDMI_ReadI2C_Byte( 0x7b );
            V_ACT  = HDMI_ReadI2C_Byte( 0x7e );
            V_ACT  = ( V_ACT << 8 ) + HDMI_ReadI2C_Byte( 0x7f );
            H_ACT  = HDMI_ReadI2C_Byte( 0x80 );
            H_ACT  = ( H_ACT << 8 ) + HDMI_ReadI2C_Byte( 0x81 );
        }
        else if((Video_Input_Mode == Input_BT1120_16BIT)||(Video_Input_Mode == Input_BT656_8BIT))
        {
            HDMI_WriteI2C_Byte( 0xff, 0x82 );
            H_TOTAL	   = HDMI_ReadI2C_Byte( 0x8f );
            H_TOTAL	   = ( H_TOTAL << 8 ) + HDMI_ReadI2C_Byte( 0x90 );
            V_ACT  = HDMI_ReadI2C_Byte( 0x8b );
            V_ACT  = ( V_ACT << 8 ) + HDMI_ReadI2C_Byte( 0x8c );
            H_ACT  = HDMI_ReadI2C_Byte( 0x8d );
            H_ACT  = ( H_ACT << 8 ) + HDMI_ReadI2C_Byte( 0x8e )-0x04;
        }

        if( ( V_ACT ==(video_bt.vact) ) &&(H_ACT==(video_bt.hact) )&&(H_TOTAL== (video_bt.htotal) ))
        {
            return 1;
        }
    }
    return 0;
}


UINT8 LT8618SX_Phase(void)  
{
    UINT8 temp=0;
    UINT8 read_value=0;
    UINT8 b_ok=0;
    UINT8 Temp_f=0;

    for(temp=0;temp<0x0a;temp++)
    {
        HDMI_WriteI2C_Byte(0xff,0x81); 
        HDMI_WriteI2C_Byte(0x27,(0x60+temp));
        if(USE_DDRCLK ==0 )
        {
            HDMI_WriteI2C_Byte(0x4d,0x01); //sdr=01,ddr=05
            HDMI_WriteI2C_Byte(0x4d,0x09); //sdr=09,ddr=0d;
        }
        else
        {
            HDMI_WriteI2C_Byte(0x4d,0x05); //sdr=01,ddr=05
            HDMI_WriteI2C_Byte(0x4d,0x0d); //sdr=09,ddr=0d;
        }
        read_value=HDMI_ReadI2C_Byte(0x50);//1->0 
#ifdef _DEBUG_MODE
        DISP_LOGI("temp= %d\n", temp);
        DISP_LOGI("read_value= %d\n",read_value);

#endif
        if(read_value==0x00)
        {
            if(b_ok==0)
            {
                Temp_f=temp;
            }
            b_ok=1;
        }
        else
        {
            b_ok=0;
        }
    }
#ifdef _DEBUG_MODE
    DISP_LOGI("Temp_f= %d\n", Temp_f);
#endif
    return Temp_f;
}


/**********************************************************/
//only use for embbedded_sync(bt1120,bt656...) or DDR
/************************************************************/
int LT8618SX_Phase_Config(void)
{
    UINT8		Temp       = 0x00;
    UINT8    Temp_f       =0x00;
    UINT8	    OK_CNT	   = 0x00;
    UINT8      OK_CNT_1   = 0x00;
    UINT8      OK_CNT_2   = 0x00;
    UINT8      OK_CNT_3   = 0x00;	
    UINT8		Jump_CNT   = 0x00;
    UINT8		Jump_Num   = 0x00;
    UINT8		Jump_Num_1 = 0x00;
    UINT8		Jump_Num_2 = 0x00;
    UINT8		Jump_Num_3 = 0x00;
    int    temp0_ok   =0;
    int    temp9_ok   =0;
    int	b_OK	   = 0;
    UINT16		V_ACT	   = 0x0000;
    UINT16		H_ACT	   = 0x0000;
    UINT16		H_TOTAL	   = 0x0000;

    Temp_f=LT8618SX_Phase() ;//it's setted before video check
        
    if(Video_Input_Mode == Input_RGB888)
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x27, ( 0x60 + (Temp_f+phease_offset)%0x0a ));
        DISP_LOGI("cail phase is 0x%x\n",(UINT16)HDMI_ReadI2C_Byte(0x27));
        return 1;
    }
        
    while( Temp <= 0x09 )
    {
        HDMI_WriteI2C_Byte( 0xff, 0x81 );
        HDMI_WriteI2C_Byte( 0x27, (0x60+Temp));
    
        HDMI_WriteI2C_Byte( 0xff, 0x80 );
        HDMI_WriteI2C_Byte( 0x13, 0xf1 );//ttl video process reset
        HDMI_WriteI2C_Byte( 0x12, 0xfb );//video check reset
        SAL_msleep(200);
        HDMI_WriteI2C_Byte( 0x12, 0xff );
        HDMI_WriteI2C_Byte( 0x13, 0xf9 );
        SAL_msleep(100);      
        if((Video_Input_Mode == Input_BT1120_16BIT)||(Video_Input_Mode == Input_BT656_8BIT))
        {
            HDMI_WriteI2C_Byte( 0xff, 0x82 );
            HDMI_WriteI2C_Byte( 0x51, 0x42 );
            H_TOTAL	   = HDMI_ReadI2C_Byte( 0x8f );
            H_TOTAL	   = ( H_TOTAL << 8 ) + HDMI_ReadI2C_Byte( 0x90 );
            V_ACT  = HDMI_ReadI2C_Byte( 0x8b );
            V_ACT  = ( V_ACT << 8 ) + HDMI_ReadI2C_Byte( 0x8c );
            H_ACT  = HDMI_ReadI2C_Byte( 0x8d );
            H_ACT  = ( H_ACT << 8 ) + HDMI_ReadI2C_Byte( 0x8e )-0x04;//note
        }
#ifdef _DEBUG_MODE
        DISP_LOGI(" h_total= %d\n", H_TOTAL);
        DISP_LOGI(" v_act= %d\n", V_ACT);
        DISP_LOGI(" h_act= %d\n", H_ACT);	
#endif
        if( ( V_ACT ==(video_bt.vact) ) &&(H_ACT==(video_bt.hact) )&&(H_TOTAL== (video_bt.htotal) ))
        {
            OK_CNT++;
            if( b_OK == 0 )
            {
                b_OK = 1;
                Jump_CNT++;
                if( Jump_CNT == 1 )
                {
                    Jump_Num_1 = Temp;
                }
                else if( Jump_CNT == 3 )
                {
                    Jump_Num_2 = Temp;
                }
                else if( Jump_CNT == 5 )
                {
                    Jump_Num_3 = Temp;
                }
            }

            if(Jump_CNT==1)
            {
                OK_CNT_1++;
            }
            else if(Jump_CNT==3)
            {
                OK_CNT_2++;
            }
            else if(Jump_CNT==5)
            {
                OK_CNT_3++;
            }

            if(Temp==0)
            {
                temp0_ok=1;
            }
            if(Temp==9)
            {
                Jump_CNT++;
                temp9_ok=1;
            }
#ifdef _DEBUG_MODE
            DISP_LOGI(" this phase is ok,temp= %d\n", Temp);
            DISP_LOGI(" Jump_CNT= %d \n", Jump_CNT);
#endif
        }
        else			
        {
            if( b_OK )
            {
                b_OK = 0;
                Jump_CNT++;
            }
#ifdef _DEBUG_MODE
            DISP_LOGI(" this phase is fail,temp= %d\n", Temp);
            DISP_LOGI(" Jump_CNT= %d \n", Jump_CNT);
#endif
        }
        Temp++;
    }
#ifdef _DEBUG_MODE
    DISP_LOGI(" OK_CNT_1= %d\n", OK_CNT_1);
    DISP_LOGI(" OK_CNT_2= %d\n", OK_CNT_2);
    DISP_LOGI(" OK_CNT_3= %d\n", OK_CNT_3);
#endif
    if((Jump_CNT==0)||(Jump_CNT>6))
    {
        DISP_LOGI("cali phase fail\n");
        return 0;
    }

    if((temp9_ok==1)&&(temp0_ok==1))
    {
        if(Jump_CNT==6)
        {
            OK_CNT_3=OK_CNT_3+OK_CNT_1;
            OK_CNT_1=0;
        }
        else if(Jump_CNT==4)
        {
            OK_CNT_2=OK_CNT_2+OK_CNT_1;
            OK_CNT_1=0;
        }
    }

    if(Jump_CNT>=2)
    {
        if(OK_CNT_1>=OK_CNT_2)
        {
            
            if(OK_CNT_1>=OK_CNT_3)
            {
                OK_CNT=OK_CNT_1;
                Jump_Num=Jump_Num_1;
            }
            else
            {
                OK_CNT=OK_CNT_3;
                Jump_Num=Jump_Num_3;
            }
        }
        else
        {
            if(OK_CNT_2>=OK_CNT_3)
            {
                OK_CNT=OK_CNT_2;
                Jump_Num=Jump_Num_2;
            }
            else
            {
                OK_CNT=OK_CNT_3;
                Jump_Num=Jump_Num_3;
            }
        }
    }	  
    HDMI_WriteI2C_Byte( 0xff, 0x81 );
            
    if( ( Jump_CNT == 2 ) || ( Jump_CNT == 4 ) || ( Jump_CNT == 6 ))
    {
        HDMI_WriteI2C_Byte( 0x27, ( 0x60 + ( Jump_Num  + ( OK_CNT / 2 ) ) % 0x0a ) );
    }
    if(OK_CNT==0x0a)
    {
        HDMI_WriteI2C_Byte( 0x27, ( 0x60 + (Temp_f+phease_offset)%0x0a ));
    
    }
    DISP_LOGI("cail phase is 0x%x\n",(UINT16)HDMI_ReadI2C_Byte(0x27));
    return 1;
}


void LT8618SX_Init(void)
{
    RESET_Lt8618SX();
    LT8618SX_Chip_ID();	
    LT8618SX_RST_PD_Init();
    LT8618SX_TTL_Input_Analog();
    LT8618SX_TTL_Input_Digital();
    LT8618SX_PLL();
    LT8618SX_Audio_Init();
    SAL_msleep(1000);
#ifdef _HDCP
    LT8618SX_HDCP_Init();
    DISP_LOGI("HDCP Init\n");
#endif
    LT8618SX_IRQ_Init();//interrupt of hdp_change or video_input_change
    vid_chk_flag = 1;
}


void LT8618SX_INTB(void)
{

    //UINT8 read_val=0;
    //int stable_flag=0;
    
    if((irq_flag3==0)&&(irq_flag0==0)&&(irq_flag1==0))//no interrupt
    {
    }
    else//get interrupt
    {	 
        DISP_LOGI( "Enter IRQ Task:" );	
#ifdef _HDCP 
        if(irq_flag3 & 0x80)//~tx hpd:1->0
        {
            LT8618SX_HDCP_Enable(0);
            LT8618SX_HDMI_Out_Enable(0);
            //vid_chk_flag=1;
            irq_flag1=0;
            DISP_LOGI("~hpd\n");
        }
        if(irq_flag3 & 0x40)//tx hpd:0->1
        {
            LT8618SX_HDMI_Out_Enable(1);
            LT8618SX_HDCP_Enable(1);
            DISP_LOGI("hpd\n");
            //vid_chk_flag=1;
            irq_flag1=0;
        }
        HDMI_WriteI2C_Byte(0xff,0x82); 
        HDMI_WriteI2C_Byte(0x07,0xff); //clear3
        HDMI_WriteI2C_Byte(0x07,0x3f); //clear3 


        if(irq_flag1 & 0x02)//~tx auth pass
        {
            LT8618SX_HDCP_Enable(0);
            if(LT8618SX_Jug())
            {
                LT8618SX_HDCP_Enable(1);
                DISP_LOGI("~pass\n");
            }
        }
        else if((irq_flag1 & 0x10))//tx_auth_done
        {

            HDMI_WriteI2C_Byte( 0xff, 0x85 );
            read_val=HDMI_ReadI2C_Byte( 0x44 );
            if(!(read_val&0x02))//tx_auth_pass
            {
                LT8618SX_HDCP_Enable(0);
                if(LT8618SX_Jug())
                {
                LT8618SX_HDCP_Enable(1);
                DISP_LOGI("done\n");
                }
            }
        }
        HDMI_WriteI2C_Byte(0xff,0x82); 
        HDMI_WriteI2C_Byte(0x05,0xff); //clear1
        HDMI_WriteI2C_Byte(0x05,0xed); //clear1
            
#else
        if(irq_flag3 & 0xc0) //HPD_Detect
        {
            DISP_LOGI("HPD:\n");
            LT8618SX_HPD_Status();
            if(Tx_HPD)
            {
                DISP_LOGI("High\n");
                LT8618SX_HDMI_Out_Enable(1);
                vid_chk_flag=1;
            }
            else
            {
                DISP_LOGI("Low\n");
                LT8618SX_HDMI_Out_Enable(0);
            }
            
            HDMI_WriteI2C_Byte(0xff,0x82); 
            HDMI_WriteI2C_Byte(0x07,0xff); //clear3
            HDMI_WriteI2C_Byte(0x07,0x3f); //clear3
        } 
#endif
        if(irq_flag0 & 0x01) //vid_chk
        {
            vid_chk_flag=1;

            DISP_LOGI("video change\n");
            HDMI_WriteI2C_Byte(0xff,0x82); 
            HDMI_WriteI2C_Byte(0x9e,0xff); //clear vid chk irq
            HDMI_WriteI2C_Byte(0x9e,0xf7); 
                
            HDMI_WriteI2C_Byte(0x04,0xff); //clear0 irq
            HDMI_WriteI2C_Byte(0x04,0xfe); 
        }
        DISP_LOGI("flag3=0x%x\n",irq_flag3);
        DISP_LOGI("flag0=0x%x\n",irq_flag0);
        DISP_LOGI("flag1=0x%x\n",irq_flag1);

        irq_flag3=0;
        irq_flag1=0;
        irq_flag0=0;
    }

}

void LT8618SX_Task(void)
{
    
    if(vid_chk_flag)
    {     
        // timeout++;
        LT8618SX_PLL();
        //LT8618SX_test_clk();//only for debug
        LT8618SX_Video_Check();
        DISP_LOGI("Video_Format = %d\n", Video_Format);

        if(0)//((Video_Format == video_none))
        {
            LT8618SX_HDMI_Out_Enable(0);
#ifdef _HDCP
            LT8618SX_HDCP_Enable(0);
#endif	 
        }
        else
        {
            LT8618SX_CSC();
            LT8618SX_HDMI_TX_Digital();
            LT8618SX_HDMI_TX_Phy();
            LT8618SX_BT_Set();
            LT8618SX_HDMI_Out_Enable(1);
#ifdef _HDCP
            SAL_msleep(200);
            LT8618SX_HDCP_Enable(1);
#endif	
            LT8618SX_Phase_Config();
                        
            vid_chk_flag=0;
            intb_flag=0;
        }
    }
}

void LT8618sx_System_Init(void)  //dsren
{
    HDMI_WriteI2C_Byte(0xFF,0x82);
    HDMI_WriteI2C_Byte(0x51,0x11);
    //Timer for Frequency meter
    HDMI_WriteI2C_Byte(0xFF,0x82);
    HDMI_WriteI2C_Byte(0x1b,0x69); //Timer 2
    HDMI_WriteI2C_Byte(0x1c,0x78);
    HDMI_WriteI2C_Byte(0xcb,0x69); //Timer 1
    HDMI_WriteI2C_Byte(0xcc,0x78);
    
    /*power consumption for work*/
    HDMI_WriteI2C_Byte(0xff,0x80); 
    HDMI_WriteI2C_Byte(0x04,0xf0);
    HDMI_WriteI2C_Byte(0x06,0xf0);
    HDMI_WriteI2C_Byte(0x0a,0x80);
    HDMI_WriteI2C_Byte(0x0b,0x46); //csc clk
    HDMI_WriteI2C_Byte(0x0d,0xef);
    HDMI_WriteI2C_Byte(0x11,0xfa);
}

/*******************************************************************************
* 函数名  : LT8618sx_Set
* 描  述  : lt8618sx初始化, 设置分辨率
* 输  入  : const BT_TIMING_S *pstTiming
            UINT8 u8BusIdx
            UINT8 u8ChipAddr
* 输  出  : 
* 返回值  : 
*******************************************************************************/
INT32 LT8618sx_Set(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn)
{
    g_pstTiming = pstTiming;
    g_u32IIC = u8BusIdx;
    g_u32Dev = u8ChipAddr;

    LT8618SX_Init();
    LT8618SX_Task();

    DISP_LOGI("lt8618sx set %uX%uP%u\n", pstTiming->u32Width, pstTiming->u32Height, pstTiming->u32Fps);

    return SAL_SOK;
}

