/*******************************************************************************
 * lt8612ex.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : Coordinates
 * Version: V1.0.0  2019年1月5日 Create
 *
 * Description : lt8612ex HDMI 转VGA和HDMI
 * Modification:
 *******************************************************************************/

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "platform_hal.h"
#include "lt8612ex_drv.h"

#include "sal.h"



/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


typedef struct
{
    UINT8    u8IICBusIdx;           //iic总线号
    UINT8    u8ChipAddr;            //LT8612EX芯片设备地址
} LT8612EX_BOARD_INFO_S;


static LT8612EX_BOARD_INFO_S  g_stLt8612exBoardInfo = {0};


static const IIC_ADDR_MAP_S g_astLt8612exVGARegs[] =
{  
    { 0x4C, 0x79 },
    { 0xBE, 0x01 },
    { 0xbb, 0x1A },
    { 0x89, 0x8c },
    { 0x84, 0x08 },
    { 0x83, 0x00 },
    { 0x8e, 0x00 },
    { 0x48, 0x10 },
    { 0x9a, 0x24 },
    { 0xa9, 0x00 },
    { 0x4D, 0x29 },
    { 0xBD, 0x00 },
    { 0xBC, 0x00 },
    { 0xB0, 0x1F },
    { 0x0C, 0x0f },
    { 0x4C, 0x79 },
    { 0x4D, 0x29 },
    { 0x06, 0x08 },
    { 0xAC, 0x20 },
    { 0xBE, 0x01 },
    { 0xAA, 0x20 },
    { 0xBB, 0x1A },
    { 0xA6, 0x00 },
    { 0xA9, 0x60 },
};


static const IIC_ADDR_MAP_S g_astLt8612exHDMIRegs[] =
{  
    { 0x4C, 0x79 },
    { 0xBE, 0x01 },
    { 0x06, 0x08 },
    { 0xbb, 0x1A },
    { 0x89, 0x8c },
    { 0x84, 0x08 },
    { 0x83, 0x00 },
    { 0x8e, 0x00 },
    { 0x48, 0x10 },
    { 0x9a, 0x24 },
    { 0xa9, 0x00 },
    { 0x4D, 0x29 },
    { 0x0C, 0x0f },
    { 0xBD, 0x00 },
    { 0xBC, 0x00 },
    { 0xB0, 0x1F },
};


static const IIC_ADDR_MAP_S g_astLt8612exAllRegs[] =
{  
    { 0x4C, 0x79 },
    { 0xBE, 0x01 },
    { 0x06, 0x08 },
    { 0xbb, 0x1A },
    { 0x89, 0x8c },
    { 0x84, 0x08 },
    { 0x83, 0x00 },
    { 0x8e, 0x00 },
    { 0x48, 0x10 },
    { 0x9a, 0x24 },
    { 0xa9, 0x60 },
    { 0x4D, 0x29 },
    { 0xBD, 0x00 },
    { 0xBC, 0x00 },
    { 0xB0, 0x1F },
    { 0x0C, 0x0f },
    { 0x4C, 0x79 },
    { 0x4D, 0x29 },
    { 0xAC, 0x20 },
    { 0xBE, 0x01 },
    { 0xAA, 0x60 },
    { 0xBB, 0x1A },
    { 0x05, 0x80 },
    { 0x08, 0x10 },
};

/*开启输出*/
static const IIC_ADDR_MAP_S  g_astLt8612exEnabledRegs[] =
{
    {0xAA, 0x20},
    {0xa9, 0x00},
};

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : LT8612EX_GetBoardInfo
* 描  述  : 获取lt8612ex的iic总线号和设备地址
* 输  入  : u32Reg : 寄存器地址
          u8Data ：写入的参数
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID LT8612EX_GetBoardInfo(UINT8 u8BusIdx, UINT8 u8ChipAddr)
{
    g_stLt8612exBoardInfo.u8IICBusIdx = u8BusIdx;
    g_stLt8612exBoardInfo.u8ChipAddr  = u8ChipAddr;

}


/*******************************************************************************
* 函数名  : LT8612EX_Read
* 描  述  : LT8612EX Read
* 输  入  : UINT32 u32Reg     要读的寄存器地址
           UINT8 *pu8Value  要读的寄存器值
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_Read(UINT8 u8Reg, UINT8 *pu8Value)
{
    INT32 s32ret = SAL_FAIL;

    s32ret = IIC_Read(g_stLt8612exBoardInfo.u8IICBusIdx, g_stLt8612exBoardInfo.u8ChipAddr, u8Reg, pu8Value);
    return s32ret;

}

/*******************************************************************************
* 函数名  : LT8612EX_Write
* 描  述  : LT8612EX_Write 
* 输  入  : UINT32 u32Reg     要写入的寄存器地址
           UINT8 *pu8Value  要写入的寄存器值
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_Write(UINT8 u8Reg, UINT8 u8Data)
{  
    INT32 s32ret = SAL_FAIL;
    s32ret = IIC_Write(g_stLt8612exBoardInfo.u8IICBusIdx, g_stLt8612exBoardInfo.u8ChipAddr, u8Reg, u8Data);
    return s32ret;
}

/*******************************************************************************
* 函数名  : LT8612EX_Write
* 描  述  : 批量写入寄存器值
* 输  入  : UINT32 u32RegNum
          const IIC_ADDR_MAP_S *pstMap
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_WriteArray(UINT32 u32RegNum, const IIC_ADDR_MAP_S *pstMap)
{
    UINT8  u8IICID;
    UINT8  u8ChipAddr;
    
    u8IICID    = g_stLt8612exBoardInfo.u8IICBusIdx;
    u8ChipAddr = g_stLt8612exBoardInfo.u8ChipAddr;
    
    return IIC_WriteArray(u8IICID, u8ChipAddr, u32RegNum, pstMap);
}


/*******************************************************************************
* 函数名  : LT8612EX_Init
* 描  述  : lt8612ex初始化
* 输  入  : LT8612_MODE_E mode
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_Init(LT8612_MODE_E mode)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32RegNum = 0;
    const IIC_ADDR_MAP_S *pRegSet = NULL;
        
    /* Select register set according to video format */
    switch (mode)
    {
        case MODE_VGA:
        {
            pRegSet = g_astLt8612exVGARegs;
            s32RegNum = SAL_arraySize(g_astLt8612exVGARegs);
            DISP_LOGI("LT8612EX set video mode only HDMI\n");
            break;
        }
        case MODE_HDMI:
        {
            pRegSet = g_astLt8612exHDMIRegs;
            s32RegNum = SAL_arraySize(g_astLt8612exHDMIRegs);
            DISP_LOGI("LT8612EX set video mode only HDMI\n");
            break;
        }
        case MODE_ALL:
        {
            pRegSet = g_astLt8612exAllRegs;
            s32RegNum = SAL_arraySize(g_astLt8612exAllRegs);
            DISP_LOGI("LT8612EX set video mode VGA and HDMI\n");
            break;
        }
        default:
        {
            DISP_LOGI("FUNC %s line %d error\n", __FUNCTION__, __LINE__);
            return SAL_FAIL;
        }
    }

    /* Configure video mode specific registers */
    LT8612EX_WriteArray(s32RegNum, pRegSet);

    DISP_LOGI("NOTE:Init LT8612ex finish--------------------\n");
    return s32Ret;
}


/*******************************************************************************
* 函数名  : LT8612EX_CheckHdmiRxClk
* 描  述  : 延时到hdmi rx时钟有效
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_TRUE : 成功
*         SAL_FALSE : 失败
*******************************************************************************/
static BOOL LT8612EX_CheckHdmiRxClk(VOID)
{
    INT32  s32MaxCheckCnt = 100;
    INT32  s32Ret = SAL_FAIL;
    BOOL   bClkValid = SAL_FALSE;
    UINT8  u8ClkTmp = 0;
    UINT32 s32ClkFre = 0;


    while((s32MaxCheckCnt-- > 0) && (SAL_FALSE == bClkValid))
    {
        /* FREQ_CLK[18:16], 单位: KHZ/ */
        s32Ret = LT8612EX_Read(0xf4, &u8ClkTmp);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("LT8612EX_Read failed\n");
            return SAL_FALSE;
        }
        
        u8ClkTmp  &= 0x07;
        s32ClkFre |= u8ClkTmp;
        s32ClkFre <<= 8;
        
        /* FREQ_CLK[15:8], 单位: KHZ/ */
        s32Ret = LT8612EX_Read(0xf3, &u8ClkTmp);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("LT8612EX_Read failed\n");
            return SAL_FALSE;
        }
        s32ClkFre |= u8ClkTmp;
        s32ClkFre <<= 8;

        /* FREQ_CLK[7:0], 单位: KHZ/ */
        s32Ret = LT8612EX_Read(0xf2, &u8ClkTmp);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("LT8612EX_Read failed\n");
            return SAL_FALSE;
        }

        s32ClkFre |= u8ClkTmp;

        if(s32ClkFre > 0x4e20)           /*20MHZ*/
        {
            bClkValid = SAL_TRUE;
            DISP_LOGI("Lt8612maxCheckCnt %d\n", s32MaxCheckCnt);
        }

        if(SAL_FALSE == bClkValid)
        {
            SAL_msleep(10);
            DISP_LOGD("CheckHdmiRxClk maxCheckCnt %d \n", s32MaxCheckCnt);
        }
    }
	
    return bClkValid;
}


/*******************************************************************************
* 函数名  : LT8612EX_RxSoftReset
* 描  述  : 芯片输入模块软复位
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_RxSoftReset(VOID)
{
    INT32 s32Status = SAL_FAIL;
    
    if (SAL_FALSE == LT8612EX_CheckHdmiRxClk())
    {
        DISP_LOGI("hdmi rx clk check fail !\n");
    }
    /* 延时等待复位生效 */
    s32Status  = LT8612EX_Write(0x80,0x06);
    s32Status |= LT8612EX_Write(0x81,0x00);
    SAL_msleep(10);
    s32Status |= LT8612EX_Write(0x80,0x0e);
    SAL_msleep(10);
    s32Status |= LT8612EX_Write(0x81,0x08);
    SAL_msleep(10);
    s32Status |= LT8612EX_Write(0x81,0x0f);
    SAL_msleep(10);
    s32Status |= LT8612EX_Write(0x80,0x0f);
    SAL_msleep(10);
    s32Status |= LT8612EX_Write(0x03,0xdf);
    SAL_msleep(10);
    s32Status |= LT8612EX_Write(0x03,0xff);
    
    if ( SAL_SOK != s32Status)
    {
        DISP_LOGE("LT8612EX_RxSoftReset failed s32Status [%d]\n", s32Status);
        return SAL_FAIL;
    }

    DISP_LOGI("NOTE:lt8612ex_rx_reset end--------------------\n");
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : LT8612EX_HardReset
* 描  述  : 芯片硬件复位
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_HardReset(VOID)
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

    //lt8612ex
    memset((char *)&stResetPrm, 0, sizeof(DDM_resetSet));
    stResetPrm.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    stResetPrm.subtype = DDM_DP_SUBTYPE_LT8612EX;   //二级子类型为LT8612EX显示模块
    stResetPrm.num = 0;                            

    s32status = ioctl(s32Fd, DDM_IOC_RESET_OFFON, &stResetPrm);  //使用ioctl接口复位
    close(s32Fd);
    if (s32status < 0)
    {
        DISP_LOGE("RESET ioctl reset LT8612EX failed fd %d %d!\n",s32Fd, s32status);
        return SAL_FAIL;
    }
    else
    {
        DISP_LOGI("LT8612EX reset ok!\n");
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : LT8612EX_DisplayEnable
* 描  述  : 模块输出显示使能
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 LT8612EX_DisplayEnable( VOID )
{
    INT32  s32Ret = SAL_SOK;
    const IIC_ADDR_MAP_S *pstRegSet = NULL;
    
    pstRegSet = g_astLt8612exEnabledRegs;

    s32Ret = LT8612EX_WriteArray(SAL_arraySize(g_astLt8612exEnabledRegs), pstRegSet);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT8612EX_WriteArray failed\n");
        return SAL_FAIL;
    }
    SAL_msleep(500);
    
    s32Ret = LT8612EX_RxSoftReset();
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT8612EX_RxSoftReset failed\n");
        return SAL_FAIL;
    }

    DISP_LOGI("NOTE:LT8612EX_DisplayEnable---\n");
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : LT8612EX_ColorSpaceConver
* 描  述  : 颜色空间转换
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT8612EX_ColorSpaceConver(VOID)
{
    UINT8 u8Value = 0;
    INT32 s32Status = SAL_FAIL;
    
    s32Status = LT8612EX_Read(0x5b, &u8Value); 

    if ( SAL_SOK != s32Status)
    {
        DISP_LOGE("LT8612EX_ColorSpaceConver failed\n");
        return;
    }
    
    u8Value = u8Value & 0x30;
    {
        if(0x10 == u8Value)                 //input is ypbpr 422
        {
            s32Status = LT8612EX_Write(0x08,0x15);
            if ( SAL_SOK != s32Status)
            {
                DISP_LOGE("LT8612EX_ColorSpaceConver failed\n");
                return;
            }

        }
        else if(0x20 == u8Value)            //input is ypbpr 444
        {
            s32Status = LT8612EX_Write(0x08,0x14);
            if ( SAL_SOK != s32Status)
            {
                DISP_LOGE("LT8612EX_ColorSpaceConver failed\n");
                return;
            }

        }
        else                                //input is RGB
        {
            s32Status = LT8612EX_Write(0x08,0x10);
            if ( SAL_SOK != s32Status)
            {
                DISP_LOGE("LT8612EX_ColorSpaceConver failed\n");
                return;
            }

        }
    }
}

/*******************************************************************************
* 函数名  : LT8612EX_Dectect
* 描  述  : 输出检测
* 输  入  : LT8612EX_DP_MODE  mode -输出格式
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static VOID LT8612EX_Dectect(LT8612EX_DP_MODE_E mode)
{
    INT32 s32Status = 0;

    switch(mode)
    {
        case HDMI_TO_HDMI:
            DISP_LOGI("lt8612ex_dectect HDMI out\n");
            s32Status |= LT8612EX_Write(0x05,mode);
            break;
        case HDMI_TO_DVI:
            DISP_LOGI("lt8612ex_dectect DVI out\n");  
            s32Status |= LT8612EX_Write(0x05,mode);
            LT8612EX_ColorSpaceConver();
            break;
        case DVI_TO_DVI:
            DISP_LOGI("lt8612ex_dectect DVI out\n");
            s32Status |= LT8612EX_Write(0x05,mode);
            /* Color space conversion based on different standards 
                bit4: 1 based on BT709, 0 based on BT601 */
            s32Status |= LT8612EX_Write(0x08,0x10);
            break;
        default:           
            DISP_LOGE("lt8612ex Unsupported mode\n");
            break;
    }
    
}

/*******************************************************************************
* 函数名  : LT8612EX_SetResolution
* 描  述  : 模块输出格式配置接口
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 LT8612EX_SetResolution(const BT_TIMING_S *pstTiming, UINT8 u8BusIdx, UINT8 u8ChipAddr, UINT32 u32Chn)
{
    /* 函数形参是为了与其它接口保持一致 */
    INT32 s32Ret = SAL_FAIL;

    DISP_LOGI("LT8612EX Set Output Resolution [%d*%dP%d]\n", pstTiming->u32Width , pstTiming->u32Height, pstTiming->u32Fps);
    
    LT8612EX_GetBoardInfo(u8BusIdx, u8ChipAddr);
    
    s32Ret = LT8612EX_HardReset();
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("lt8612ex Hard Reset failed\n");
        return SAL_FAIL;
    }
    
    s32Ret = LT8612EX_Init(MODE_ALL);
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("lt8612ex init failed\n");
        return SAL_FAIL;
    }

    LT8612EX_Dectect(DVI_TO_DVI);
    /* 延时等待前级输出时钟稳定 */
    SAL_msleep(1000);
    
    s32Ret = LT8612EX_DisplayEnable();
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGE("LT8612EX Display Enable failed\n");
        return SAL_FAIL;
    }

    return s32Ret;
}



