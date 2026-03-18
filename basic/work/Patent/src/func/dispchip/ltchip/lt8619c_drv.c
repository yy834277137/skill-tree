/*******************************************************************************
 * lt8619C.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : wangzhenya5
 * Version: V1.0.0  2021年05月10日 Create
 *
 * Description : lt8619C配置,HDMI转LVDS
 * Modification: 
 *******************************************************************************/


#include "platform_hal.h"
#include "lt8619c_drv.h"
#include "sal.h"
#line __LINE__ "lt8619c_drv.c"


typedef struct
{
    UINT8    u8IICBusIdx;        //iic总线号
    UINT8    u16ChipAddr;        //LT9211芯片设备地址

} LT8619C_BOARD_INFO_S;

static LT8619C_BOARD_INFO_S  g_stLT8619cBoardInfo = {0};

/* ========================================================================== */
/*                             函数定义区                                          */
/* ========================================================================== */


/*******************************************************************************
* 函数名  : LT8619C_GetBoardInfo
* 描  述  : 获取lt8619c的iic总线号和设备地址
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT8619C_GetBoardInfo(UINT8 u8BusIdx, UINT8 u8ChipAddr)
{
    g_stLT8619cBoardInfo.u8IICBusIdx = u8BusIdx;
    g_stLT8619cBoardInfo.u16ChipAddr = u8ChipAddr;

}


/*******************************************************************************
* 函数名  : LT8619C_WriteI2C_Byte
* 描  述  : 向制定寄存器中写入参数
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/

static INT32 LT8619C_WriteI2C_Byte(UINT32 u32Reg, UINT8 u8Data)
{
    INT32 s32Ret;
    UINT8  u8IICID;
    UINT16 u16ChipAddr;
    
    u8IICID     = g_stLT8619cBoardInfo.u8IICBusIdx;
    u16ChipAddr = g_stLT8619cBoardInfo.u16ChipAddr;
    
    s32Ret = IIC_Write(u8IICID, u16ChipAddr, u32Reg, u8Data);
    
    if(s32Ret != SAL_SOK )
    {
        DISP_LOGE("LT8619C_WriteI2C_Byte fail reg[0x%x], data[0x%x] \n", u32Reg, u8Data);
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : LT8619C_ReadI2C_Byte
* 描  述  : 向制定寄存器中写入参数
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/

static UINT8 LT8619C_ReadI2C_Byte(UINT32 u32Reg)
{
    INT32  ret;
    UINT8  u8Data = 0;
    UINT8  u8IICID;
    UINT16 u16ChipAddr;
    
    u8IICID     = g_stLT8619cBoardInfo.u8IICBusIdx;
    u16ChipAddr = g_stLT8619cBoardInfo.u16ChipAddr;
    
    ret = IIC_Read(u8IICID, u16ChipAddr, u32Reg, &u8Data);
    if(ret != SAL_SOK)
    {
        DISP_LOGE("ReadI2Cfail reg[0x%x]\n", u32Reg);
        return SAL_FAIL;
    }
    return u8Data;
}


/*******************************************************************************
* 函数名  : LT8619C_HardReset
* 描  述  : LT8619C硬复位
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8619C_HardReset(VOID)
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

    //LT8619C
    memset((char *)&stResetPrm, 0, sizeof(DDM_resetSet));
    stResetPrm.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    stResetPrm.subtype = DDM_DP_SUBTYPE_LT8619;   //二级子类型为LT8619显示模块
    stResetPrm.num = 0;                            

    s32status = ioctl(s32Fd, DDM_IOC_RESET_OFFON, &stResetPrm);  //使用ioctl接口复位
    close(s32Fd);
    if (s32status < 0)
    {
        DISP_LOGE("RESET ioctl reset LT8619C failed fd %d %d!\n",s32Fd, s32status);
    }
    else
    {
        DISP_LOGI("LT8619C reset ok!\n");
    }

    return SAL_SOK;

}


/*******************************************************************************
* 函数名  : LT8619C_SyncPolConfigPositive
* 描  述  : 设置LT8619C输出极性为正
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT8619C_SyncPolConfigPositive(VOID)
{
    LT8619C_WriteI2C_Byte( 0xFF, 0x80 );

    if( ( LT8619C_ReadI2C_Byte( 0x17 ) & 0x40 ) == 0x00 )
    {
        if( ( LT8619C_ReadI2C_Byte( 0x05 ) & 0x10 ) == 0x10 )
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) & 0xef );   // close V sync polarity swap
        }
        else 
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) | 0x10 );   // enable V sync polarity swap
        }
    }

    if( ( LT8619C_ReadI2C_Byte( 0x17 ) & 0x80 ) == 0x00 )
    {
        if( ( LT8619C_ReadI2C_Byte( 0x05 ) & 0x20 ) == 0x20 )
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) & 0xdf );   // close H sync polarity swap
        }
        else 
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) | 0x20 );   // enable H sync polarity swap
        }
    }
}

/*******************************************************************************
* 函数名  : LT8619C_SyncPolConfigNegative
* 描  述  : 设置LT8619C输出极性为负
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT8619C_SyncPolConfigNegative(VOID)
{
    LT8619C_WriteI2C_Byte( 0xFF, 0x80 );

    if( ( LT8619C_ReadI2C_Byte( 0x17 ) & 0x40 ) == 0x40 )
    {
        if( ( LT8619C_ReadI2C_Byte( 0x05 ) & 0x10 ) == 0x10 )
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) & 0xef );   // close V sync polarity swap
        }
        else 
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) | 0x10 );   // enable V sync polarity swap
        }
    }

    if( ( LT8619C_ReadI2C_Byte( 0x17 ) & 0x80 ) == 0x80 )
    {
        if( ( LT8619C_ReadI2C_Byte( 0x05 ) & 0x20 ) == 0x20 )
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) & 0xdf );   // close H sync polarity swap
        }
        else 
        {
            LT8619C_WriteI2C_Byte( 0x05, LT8619C_ReadI2C_Byte( 0x05 ) | 0x20 );   // enable H sync polarity swap
        }
    }
}


/*******************************************************************************
* 函数名  : LT8619C_SyncSetPol
* 描  述  : 设置LT8619C输出极性
* 输  入  : UINT8  u8Status
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT8619C_SyncSetPol(UINT8  u8Status)
{
    if(1 == u8Status)
    {
        LT8619C_SyncPolConfigPositive();
    }
    else if(0 == u8Status)
    {
        LT8619C_SyncPolConfigNegative();
    }
}


/*******************************************************************************
* 函数名  : LT8619C_OutPutSet2Lvds
* 描  述  : 配置LT8619C为双端口lvds输出
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT8619C_OutPutSet2Lvds(VOID)
{
    /* LT8619C 设置为双端口lvds的输出 */
    LT8619C_WriteI2C_Byte(0xFF,0x60);
    /*
    bit7 : LVDS TX controller module clock enable.
    bit6 : BT TX controller module clock enable.
    bit5 : EDID shadow module sys_clk clock enable.
    bit2 : Video check module sys_clk clock enable.
    bit1 ：Video check module pix_clk clock enable.
    bit0 ：Interrupt process module sys_clk clock enable. 
    */
    LT8619C_WriteI2C_Byte(0x06,0xe7);
 
    /* bit7 for VESA/JEIDA mode Uuse VESA mode; 
       bit6 Control lvds channel output data;
       bit5 for DE/SYNC mode use Sync mode; 
       bit4 for 6/8bit use 8bit; 
       bit1 for port swap
    */
    LT8619C_WriteI2C_Byte(0x59,0x42);
    /* bit7 LVDS pll Normal work; 
       bit4 This bit controls the operation of PLL locking block;
       bit3 LVDS pll reference select use Pixel clock as reference;
    */
    LT8619C_WriteI2C_Byte(0xa0,0x58);
    
    LT8619C_WriteI2C_Byte(0xa4,0x01);
    LT8619C_WriteI2C_Byte(0xa8,0x00);
    LT8619C_WriteI2C_Byte(0xba,0x18);   //LVDS C channel control
    LT8619C_WriteI2C_Byte(0xc0,0x18);   //LVDS D channel control
    
    LT8619C_WriteI2C_Byte(0xb0,0x66);
    LT8619C_WriteI2C_Byte(0xb1,0x66);
    LT8619C_WriteI2C_Byte(0xb2,0x66);
    LT8619C_WriteI2C_Byte(0xb3,0x66);
    LT8619C_WriteI2C_Byte(0xb4,0x66);
    LT8619C_WriteI2C_Byte(0xb5,0x41);
    LT8619C_WriteI2C_Byte(0xb6,0x41);
    LT8619C_WriteI2C_Byte(0xb7,0x41);
    LT8619C_WriteI2C_Byte(0xb8,0x4c);
    LT8619C_WriteI2C_Byte(0xb9,0x41);
    LT8619C_WriteI2C_Byte(0xbb,0x41);
    LT8619C_WriteI2C_Byte(0xbc,0x41);
    LT8619C_WriteI2C_Byte(0xbd,0x41);
    LT8619C_WriteI2C_Byte(0xbe,0x4c);
    LT8619C_WriteI2C_Byte(0xbf,0x41);
    LT8619C_WriteI2C_Byte(0xa0,0x18);
    LT8619C_WriteI2C_Byte(0xa1,0xb0);
    LT8619C_WriteI2C_Byte(0xa2,0x10);
    /* bit0=0:single port  bit0=1:dual port */
    LT8619C_WriteI2C_Byte(0x5c,0x01);
}

/*******************************************************************************
* 函数名  : LT8619C_RXInit
* 描  述  : LT8619C输入初始化
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT8619C_RXInit(VOID)
{

    LT8619C_WriteI2C_Byte(0xFF,0x80);
    /* RGD_CLK_STABLE_OPT[1:0] */
    LT8619C_WriteI2C_Byte(0x2c,LT8619C_ReadI2C_Byte(0x2c)|0x30);
    
    LT8619C_WriteI2C_Byte(0xFF,0x60);
    /* LVDS PLL lock detect module clock enable */
    LT8619C_WriteI2C_Byte(0x04,0xF2);
    /* IO output drive strength */
    LT8619C_WriteI2C_Byte(0x83,0x3F);
    /* use xtal_clk as sys_clk  */
    LT8619C_WriteI2C_Byte(0x80,0x08);  
    /* use SDR clk */
    LT8619C_WriteI2C_Byte(0xa4,0x10);//0x10:SDR clk,0x14: DDR clk

    DISP_LOGI("LT8619C output MODE set to: OUTPUT_LVDS_2_PORT\n");
    LT8619C_OutPutSet2Lvds();

    LT8619C_WriteI2C_Byte(0xff,0x60);
    /* LVDSPLL soft reset */
    LT8619C_WriteI2C_Byte(0x0e,0xfd);
    LT8619C_WriteI2C_Byte(0x0e,0xff);
    /* BT TX controller module soft reset. 
       BT TX afifo soft reset.          */
    LT8619C_WriteI2C_Byte(0x0d,0xfc);
    LT8619C_WriteI2C_Byte(0x0d,0xff);
}

/*******************************************************************************
* 函数名  : LT8619C_RXReset
* 描  述  : lt8619c接收模块软件复位
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT8619C_RxReset(VOID)
{
    LT8619C_WriteI2C_Byte(0xFF,0x60);
    LT8619C_WriteI2C_Byte(0x0E,0xBF);       /* HDMI RXPLL soft reset */
    LT8619C_WriteI2C_Byte(0x09,0xFD);       /* RXPLL lock detect control logic soft reset. */
    /* 延时等待复位完成 */
    SAL_msleep(5);
    LT8619C_WriteI2C_Byte(0x0E,0xFF);       /* release RXPLL */
    LT8619C_WriteI2C_Byte(0x09,0xFF);       /* release RXPLL Lock detect */

    LT8619C_WriteI2C_Byte(0xFF,0x60);
    LT8619C_WriteI2C_Byte(0x0e,0xC7);       /* HDMI RX PI0/1/2 soft reset. */
    LT8619C_WriteI2C_Byte(0x09,0x0F);       /* HDMI RX control logic soft reset;
                                               HDMI RX CDR0/1/2 control logic soft reset */
    /* 延时等待复位完成 */
    SAL_msleep(10);
    LT8619C_WriteI2C_Byte(0x0e,0xFF);       /* release PI0/1/2 */
    /* 延时等待复位完成 */
    SAL_msleep(10);
    LT8619C_WriteI2C_Byte(0x09,0x8F);       /* release RX */
    /* 延时等待复位完成 */
    SAL_msleep(10);
    LT8619C_WriteI2C_Byte(0x09,0xFF);       /* release CDR0/1/2 */
    /* 延时等待复位完成 */
    SAL_msleep(50);
}


/*******************************************************************************
* 函数名  : LT8619C_LvdsPllLockDet
* 描  述  : pll锁定检测
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT8619C_LvdsPllLockDet(VOID)
{  

    UINT8 u8ReadData;
    UINT8 u8CheckNum = 0;
    
    LT8619C_WriteI2C_Byte(0xFF,0x80);
    while((LT8619C_ReadI2C_Byte(0x87) & 0x20) == 0x00)//Output LVDSPLL unlock status
    {
        LT8619C_WriteI2C_Byte(0xFF,0x60);
        u8ReadData = LT8619C_ReadI2C_Byte(0x0e);
        LT8619C_WriteI2C_Byte(0x0e,u8ReadData & 0xFD);//LVDSPLL soft reset
        /* 延时等待复位完成 */
        SAL_msleep(5);
        LT8619C_WriteI2C_Byte(0x0e,0xFF);

        LT8619C_WriteI2C_Byte(0xFF,0x80);

        /* 循环检测多次 */
        u8CheckNum++;
        if(u8CheckNum > 3)
        {
            break;
        }
    }
	
}

/*******************************************************************************
* 函数名  : LT8619C_HsStable
* 描  述  : 检测HS状态
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL: 失败
*******************************************************************************/
static VOID LT8619C_HsStable(VOID)
{
    UINT8 i;
    UINT8 u8Cnt = 0;
    UINT8 u8LoopNum = 10;
    UINT8 u8ReadData = 0;
    UINT8 u8HsyncStableFlag = SAL_FALSE;

    LT8619C_WriteI2C_Byte(0xFF,0x80);
    /* Hsync状态：
       0 = No hsync detected or not stable
       1 = Hsync is detected, and is stable */
       
    //循环监测10次
    for(i = 0; i < u8LoopNum; i++)
    {
        if(LT8619C_ReadI2C_Byte(0x13) & 0x01)
        {
            u8Cnt++;
        }
        else
        {
            DISP_LOGI("LT8619C Hsync check %dth times  !!!\n", i);
        }
    }
    if (u8LoopNum == u8Cnt)
    {
        u8HsyncStableFlag = SAL_TRUE;
        DISP_LOGI("LT8619C Hsync stable success\n");
    }
    else
    {
        u8HsyncStableFlag = SAL_FALSE;
        DISP_LOGE("LT8619C Hsync not stable, check [%d]times, [%d] success\n", u8LoopNum, u8Cnt);
    }
    
    if( u8HsyncStableFlag)
    {
        LT8619C_WriteI2C_Byte(0xFF,0x60);
        u8ReadData = LT8619C_ReadI2C_Byte(0x0D);
        /* bit2 LVDS TX controller module soft reset
           bit1 BT TX controller module soft reset.
           bit0 BT TX afifo soft reset */
        LT8619C_WriteI2C_Byte(0x0D, (u8ReadData & 0xf8));
        SAL_msleep(10);
        LT8619C_WriteI2C_Byte(0x0D, (u8ReadData | 0x06));
        LT8619C_WriteI2C_Byte(0x0D, (u8ReadData | 0x01));

        DISP_LOGI("LT8619C Hsync stable!!!\n");

        u8ReadData = 0;
        u8ReadData = LT8619C_ReadI2C_Byte(0x97);
        LT8619C_WriteI2C_Byte(0x97,u8ReadData & 0x3f);
        LT8619C_WriteI2C_Byte(0xFF, 0x80);
        LT8619C_WriteI2C_Byte(0x1b, 0x00);

        /* CSC相关配置 */
        LT8619C_WriteI2C_Byte(0xFF,0x60);
        LT8619C_WriteI2C_Byte(0x07,0xfe);
        LT8619C_WriteI2C_Byte(0x52,0x00);
        LT8619C_WriteI2C_Byte(0x53,0x00);

        
        LT8619C_LvdsPllLockDet();
    }
}

/*******************************************************************************
* 函数名  : LT8619C_ClkCheck
* 描  述  : LT8619C时钟检测
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL: 失败
*******************************************************************************/
static BOOL LT8619C_ClkCheck(VOID)
{
    UINT8  u8ReadData;
    
    BOOL   bClkStableStatus = SAL_FALSE;
    BOOL   bRXClkStableFlag = SAL_FALSE;
    BOOL   bRXPLLLockedFlag = SAL_FALSE;
    UINT32 u32MaxCheckCnt = 100;

    while((u32MaxCheckCnt-- > 0) && (SAL_FALSE == bRXPLLLockedFlag))
    {
        
        LT8619C_WriteI2C_Byte(0xFF,0x80);
        /* Clock stable status indicator */
        bClkStableStatus = LT8619C_ReadI2C_Byte(0x44) & 0x08;
        
        DISP_LOGI("LT8619C bClkStableStatus[%d],cnt [%d]\n", bClkStableStatus, u32MaxCheckCnt);
        
        if(!bClkStableStatus)
        {
            SAL_msleep(10); /* 休眠一定时间之后再检测clock是否稳定 */
            continue;
        }

        if(!bRXClkStableFlag)
        {
            bRXClkStableFlag = !bRXClkStableFlag;
            LT8619C_WriteI2C_Byte(0xFF,0x60);
            u8ReadData = LT8619C_ReadI2C_Byte(0x97);
            LT8619C_WriteI2C_Byte(0x97,(u8ReadData & 0x3f));
            LT8619C_WriteI2C_Byte(0xFF,0x80);
            LT8619C_WriteI2C_Byte(0x1b,0x00);
            LT8619C_RxReset();
            
            LT8619C_WriteI2C_Byte(0xFF,0x60);                  
            LT8619C_WriteI2C_Byte(0x0e,0xfd);          /* LVDSPLL soft reset */
            LT8619C_WriteI2C_Byte(0x0e,0xff);          /* release LVDSPLL */       
            /* 延时等待复位完成 */
            SAL_msleep(5);
            LT8619C_WriteI2C_Byte(0xFF,0x80);
            u8ReadData = LT8619C_ReadI2C_Byte(0x87) & 0x10; /* Output RXPLL lock status */
            if( u8ReadData )
            {
                bRXPLLLockedFlag = SAL_TRUE;
                DISP_LOGI("LT8619C clk detected!!!\n");
                DISP_LOGI("LT8619C pll lock!!!\n");
                break;
            }
            else
            {
                bRXPLLLockedFlag = SAL_FALSE;
                bRXClkStableFlag = SAL_FALSE;
                DISP_LOGI("LT8619C clk detected!!!\n");
                DISP_LOGI("LT8619C pll unlock!!!\n");
                DISP_LOGI("CheckHdmiRxClk maxCheckCnt %d \n", u32MaxCheckCnt);
                continue;
            }
        }
    }
    
    if((!bClkStableStatus) || (u32MaxCheckCnt == 0))
    {
        DISP_LOGE("LT8619C clk disappear!!!,CheckCnt[%d]\n", u32MaxCheckCnt);
    }

    if(bRXClkStableFlag && bRXPLLLockedFlag)
    {
        LT8619C_HsStable();
    }
    return SAL_TRUE;
}


/*******************************************************************************
* 函数名  : LT8619C_LvdsTxDeSigCheck
* 描  述  : LVDS输出DE信号检查
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_TRUE : 信号检查通过
*         SAL_FALSE: 信号检查失败
*******************************************************************************/
static BOOL LT8619C_LvdsTxDeSigCheck(VOID)
{
    UINT8  i;
    UINT8  u8TxDeSigStatus = 0;
    UINT8  u8DataCheckCnt = 0;

    for ( i = 0; i < 200; i++ )
    {
        LT8619C_WriteI2C_Byte(0xFF,0x60);
        
        /* 6092寄存器 bit0：LVDS TX channel D output de signal status */
        u8TxDeSigStatus = LT8619C_ReadI2C_Byte(0x92) & 0x01;
        if(u8TxDeSigStatus == 0)
        {
            u8DataCheckCnt++;
        }
    }
    DISP_LOGI("Data Check [%d] times, [%d] times Tx De Sig is 0 \n", i, u8DataCheckCnt);
    
    /* LVDS TX de signal; de信号为视频是否有效的依据，
       正常情况下de信号是在0或1跳变的，一直为0或者一直为1是异常的 */
    if((u8DataCheckCnt == 200) || (u8DataCheckCnt == 0))
    {
        DISP_LOGI("LVDS TX de signal unnormal \n");
        return SAL_FALSE;
    }
    else
    {
        return SAL_TRUE;
    }
}


/*******************************************************************************
* 函数名  : LT8619C_LvdsDataDet
* 描  述  : LT8619C输出LVDS数据检测
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL: 失败
*******************************************************************************/
static INT32 LT8619C_LvdsDataDet(VOID)
{
    UINT8  u8Times = 0;
    UINT8  u8ReadData = 0;

    do
    {
        if(!LT8619C_LvdsTxDeSigCheck())
        {
            /* 检测到输出的DE信号异常，重新复位后再一次检测DE信号 */
            LT8619C_WriteI2C_Byte(0xFF,0x60);
            u8ReadData = LT8619C_ReadI2C_Byte(0x0D);
            /* reset LVDS/BT fifo;
               bit2: LVDS TX controller module soft reset.
               bit1: BT TX controller module soft reset. 
               bit0: BT TX afifo soft reset.*/
            LT8619C_WriteI2C_Byte(0x0D, u8ReadData & 0xf8);
            /* 延时等待生效 */
            SAL_msleep(10);
            LT8619C_WriteI2C_Byte(0x0D, u8ReadData | 0x06);
            LT8619C_WriteI2C_Byte(0x0D, u8ReadData | 0x01);
            
            u8Times++;
        }

        if(u8Times > 4)
        {
            return SAL_FAIL;
        }

    }
    while(!LT8619C_LvdsTxDeSigCheck());
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : LT8619C_CheckChipID
* 描  述  : 读取chipID
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL: 失败
*******************************************************************************/

static INT32 LT8619C_CheckChipID(VOID)
{
    LT8619C_WriteI2C_Byte(0xFF, 0x60);

    if( (LT8619C_ReadI2C_Byte(0x00) == 0x16) && (LT8619C_ReadI2C_Byte(0x01) == 0x04) )
    {
        DISP_LOGI("ChipID is OK:0x1604\n");
        return SAL_SOK;
    }
    else
    {
        DISP_LOGE("ChipID Fail!!!!\n");
        return SAL_FAIL;
    }
}

/*******************************************************************************
* 函数名  : LT8619C_Init
* 描  述  : LT8619C初始化, 设置分辨率
* 输  入  : const BT_TIMING_S *pstTiming
          UINT8 u8BusIdx
          UINT8 u8ChipAddr
* 输  出  : 
* 返回值  : 
*******************************************************************************/
INT32 LT8619C_Init(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn)
{
    UINT8 u8ReInitCnt = 0;
    INT32 s32Ret = SAL_FAIL;
    BOOL  bReInitFlg = SAL_FALSE;
    
    DISP_LOGI("LT8619C Set Output Resolution [%d*%dP%d]\n", pstTiming->u32Width , pstTiming->u32Height, pstTiming->u32Fps);

    LT8619C_GetBoardInfo(u8BusIdx, u8ChipAddr);
    
    /* lt8619c hardware reset */
    s32Ret = LT8619C_HardReset();
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT8619C Hard Reset failed\n");
        return SAL_FAIL;
    }
    
    s32Ret =  LT8619C_CheckChipID();
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT8619C Check ChipID failed, Hard Reset failed\n");
        return SAL_FAIL;
    }
    
    /* 延时等待海思输出时钟稳定 */
    SAL_msleep(500);
    
    do{
        LT8619C_RXInit();
        LT8619C_ClkCheck();
         
        /* 检测到输出的DE信号异常, 复位TX模块无效后, 重新进行初始化 */
        s32Ret = LT8619C_LvdsDataDet();
        if(s32Ret != SAL_SOK)
        {
            bReInitFlg = SAL_TRUE;
            
            DISP_LOGE("LT8619C LVDS Data Det failed, ReInit, ReInit times [%d] \n", u8ReInitCnt);
            
            if(u8ReInitCnt < 5)
            {
                u8ReInitCnt ++;
            }
            else
            {
                DISP_LOGI("LT8619C Init failed \n");
                return SAL_FAIL;
            }
        }
        else
        {
            bReInitFlg = SAL_FALSE;
        }
    }
    while(bReInitFlg);
    
    /* 设置Lt8619C输出极性为正 */
    LT8619C_SyncSetPol(1);
    
    DISP_LOGI("LT8619C_Init end!!!!\n");
    return SAL_SOK;
}



