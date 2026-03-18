/**
* @file  adv7842_drv.c
* @note  2001-2012, 杭州海康威视数字技术股份有限公司
* @brief  fpga配置文件

* @author  wutao
* @date    2019/01/09

* @note :
* @note \n History:
  1.日    期: 2019/01/09
    作    者: wutao
    修改历史: 创建文件
*/

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/

//#include "fpga_drv.h"
#include "sal_macro.h"
#include <platform_hal.h>
#include "mcu_drv.h"



typedef struct
{
    INT32(*pFuncInit)(UINT32 u32Chn);
    INT32(*pFuncReset)(UINT32 u32Chn);
    INT32(*pFuncSetResolution)(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps);
} FPGA_FUNC_S;

typedef struct tagDDM_fpgaArg{
   UINT32 reg;    
   UINT32 value;
}DDM_fpgaArg;

#define DDM_DEV_FPGA_NAME           "/dev/DDM/fpga"             
#define DDM_IOC_FPGA_MNUM           'F'
#define DDM_IOC_FPGA_GET_REG        _IOW(DDM_IOC_FPGA_MNUM, 0, DDM_fpgaArg)
#define DDM_IOC_FPGA_SET_REG        _IOR(DDM_IOC_FPGA_MNUM, 1, DDM_fpgaArg)

#define FPGA_CHN_NUM                (2)

#define FPGA_REG_BUILD_TIME         (0x01)          /* [31:16]表示年份，[15:8]表示月份，[7:0]表示日期 */

/* 下列寄存器只能写，不能读 */
#define FPGA_REG_RESET              (0x02)          /* 复位流程：先写1，延时后写0 */
#define FPGA_REG_VIN0               (0x03)          /* [31:16]表示一行像素点个数，[15:0]表示行数 */
#define FPGA_REG_VIN1               (0x04)          /* 同上 */
#define FPGA_REG_ENABLE0            (0x05)          /* 使能寄存器，1为使能，0为失能 */
#define FPGA_REG_ENABLE1            (0x06)          /* 同上 */

/* 下列寄存器只能读，不能写 */
#define FPGA_REG_DEBUG_RES0         (0x100)         /* 应用配置通道0的分辨率，[31:16]表示一行像素点个数，[15:0]表示行数 */
#define FPGA_REG_DEBUG_RES1         (0x101)         /* 应用配置通道1的分辨率，[31:16]表示一行像素点个数，[15:0]表示行数 */
#define FPGA_REG_DEBUG_ERROR0       (0x102)         /* 通道0接收图像异常报错，0正常 */
#define FPGA_REG_DEBUG_ERROR0_RES   (0x103)         /* 通道0接收异常时统计的宽高 */
#define FPGA_REG_DEBUG_ERROR1       (0x104)         /* 通道1接收图像异常报错，0正常 */
#define FPGA_REG_DEBUG_ERROR1_RES   (0x105)         /* 通道1接收异常时统计的宽高 */
#define FPGA_REG_DEBUG_ENABLE       (0x106)         /* 应用配置的使能值，bit[0]表示通道0，bit[1]表示通道1 */

#define FPGA_WIDTH_ALIGN_NUM        (8)
#define FPGA_HEIGHT_ALIGN_NUM       (4)

static int g_FpgaFd = -1;
static FPGA_FUNC_S g_astFpgaFunc[FPGA_CHN_NUM] = {0};
static BOOL g_abFpgaFuncInited[FPGA_CHN_NUM] = {SAL_FALSE, SAL_FALSE};
static UINT8 g_au8BuildTime[FPGA_CHN_NUM][32] = {0};

/*******************************************************************************
* 函数名  : SpiFpga_Open
* 描  述  : 打开fpga节点句柄
* 输  入  : UINT32 u32Reg : 寄存器地址
* 输  出  : UINT32 *pu32Val : 返回值
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 SpiFpga_Open(VOID)
{
    if (g_FpgaFd >= 0)
    {
        return SAL_SOK;
    }

    g_FpgaFd = open(DDM_DEV_FPGA_NAME, O_RDWR);	
	if(g_FpgaFd < 0)	
	{	
	    FPGA_LOGE("fpga open %s error:%s\n", DDM_DEV_FPGA_NAME, strerror(errno));
	  	return SAL_FAIL;	
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : SpiFpga_Read
* 描  述  : 读fpga寄存器的值
* 输  入  : UINT32 u32Reg : 寄存器地址
* 输  出  : UINT32 *pu32Val : 返回值
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 SpiFpga_Read(UINT32 u32Reg, UINT32 *pu32Val)
{
    DDM_fpgaArg stFpga;
    
    if (g_FpgaFd < 0)
    {
        if (SAL_SOK != SpiFpga_Open())
        {
            FPGA_LOGE("open fpga fail");
            return SAL_FAIL;
        }
    }

    stFpga.reg = u32Reg;
	if(ioctl(g_FpgaFd, DDM_IOC_FPGA_GET_REG, &stFpga) < 0)    
    {       
  	    FPGA_LOGE("fpga read 0x%x fail:%s\n", u32Reg, strerror(errno));		
	    return SAL_FAIL;	
	}

    *pu32Val = stFpga.value;
    FPGA_LOGD("fpga read addr[0x%x] value[0x%x]\n", u32Reg, stFpga.value);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : SpiFpga_Write
* 描  述  : 写fpga寄存器的值
* 输  入  : UINT32 u32Reg : 寄存器地址
* 输  出  : UINT32 u32Val : 值
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 SpiFpga_Write(UINT32 u32Reg, UINT32 u32Val)
{
    DDM_fpgaArg stFpga;

    if (g_FpgaFd < 0)
    {
        if (SAL_SOK != SpiFpga_Open())
        {
            FPGA_LOGE("open fpga fail");
            return SAL_FAIL;
        }
    }

    stFpga.reg   = u32Reg;
    stFpga.value = u32Val;
 
	if(ioctl(g_FpgaFd, DDM_IOC_FPGA_SET_REG, &stFpga) < 0)    
    {       
  	    FPGA_LOGE("fpga write 0x%x to 0x%x fail:%s\n", u32Val, u32Reg, strerror(errno));		
	    return SAL_FAIL;	
	}

    FPGA_LOGD("fpga write 0x%x to 0x%x\n", u32Val, u32Reg);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : SpiFpga_Reset
* 描  述  : 复位FPGA
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 SpiFpga_Reset(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;

    SAL_UNUSED(u32Chn);

    s32Ret = SpiFpga_Write(FPGA_REG_RESET, 1);
    if (SAL_SOK != s32Ret)
    {
        FPGA_LOGE("fpga set reset reg to 1 fail\n");
        return SAL_FAIL;
    }

    SAL_msleep(100);

    s32Ret = SpiFpga_Write(FPGA_REG_RESET, 0);
    if (SAL_SOK != s32Ret)
    {
        FPGA_LOGE("fpga set reset reg to 0 fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : SpiFpga_IsRecSuccess
* 描  述  : FPGA接收的图像宽高是否成功
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static BOOL SpiFpga_IsRecSuccess(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Error = 0;
    UINT32 u32Res = 0;
    static UINT32 au32ErrorReg[FPGA_CHN_NUM] = {FPGA_REG_DEBUG_ERROR0, FPGA_REG_DEBUG_ERROR1};
    static UINT32 au32ResReg[FPGA_CHN_NUM] = {FPGA_REG_DEBUG_ERROR0_RES, FPGA_REG_DEBUG_ERROR1_RES};

    s32Ret |= SpiFpga_Read(au32ErrorReg[u32Chn], &u32Error);
    s32Ret |= SpiFpga_Read(au32ResReg[u32Chn], &u32Res);
    /* 当前FPGA只有在异常的时候会对这两个值赋值，所以，正常情况下，这两个值非0 */
    if ((SAL_SOK != s32Ret) || (0 != u32Error) || (0 != u32Res))
    {
        FPGA_LOGW("fpga chn[%u] rec input resolution[0x%x] error:0x%x\n", u32Chn, u32Res, u32Error);
        return SAL_FALSE;
    }

    return SAL_TRUE;
}


/*******************************************************************************
* 函数名  : SpiFpga_SetResolution
* 描  述  : 配置输入分辨率
* 输  入  : UINT32 u32Chn
            UINT32 u32Width
            UINT32 u32Height
            UINT32 u32Fps
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 SpiFpga_SetResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Val = 0;
    static UINT32 au32ResReg[FPGA_CHN_NUM] = {FPGA_REG_VIN0, FPGA_REG_VIN1};
    static UINT32 au32EnableReg[FPGA_CHN_NUM] = {FPGA_REG_ENABLE0, FPGA_REG_ENABLE1};

    SAL_UNUSED(u32Fps);

    if (u32Chn >= FPGA_CHN_NUM)
    {
        FPGA_LOGE("invalid fpga chn[%u]\n", u32Chn);
        return SAL_FAIL;
    }

    u32Val = ((0 == u32Width) || (0 == u32Height)) ? 0 : ((u32Width << 16) | u32Height);
    s32Ret = SpiFpga_Write(au32ResReg[u32Chn], u32Val);
    if (SAL_SOK != s32Ret)
    {
        FPGA_LOGE("fpga chn[%u] set resolution[%uX%u] fail\n", u32Chn, u32Width, u32Height);
        return SAL_FAIL;
    }

    u32Val = ((0 == u32Width) || (0 == u32Height)) ? 0 : 1;
    s32Ret = SpiFpga_Write(au32EnableReg[u32Chn], u32Val);
    if (SAL_SOK != s32Ret)
    {
        FPGA_LOGE("fpga chn[%u] enable[%u] fail\n", u32Chn, u32Val);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : SpiFpga_DectedAndReset
* 描  述  : 检测FPGA统计的宽高并复位
* 输  入  : 
* 输  出  : UINT32 u32Chn
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 SpiFpga_DectedAndReset(UINT32 u32Chn)
{
    UINT32 u32ResetTimes = 0;

    if (u32Chn >= FPGA_CHN_NUM)
    {
        FPGA_LOGE("invalid fpga chn[%u]\n", u32Chn);
        return SAL_FAIL;
    }

    (VOID)SpiFpga_Reset(u32Chn);

    /* FPGA复位后会检测输入的时钟并锁定，如果刚开始时钟不稳定，锁定的时钟异常，会造成后面图像异常 */
    u32ResetTimes = 3;
    while (u32ResetTimes--)
    {
        SAL_msleep(5000);               /* 外部芯片给FPGA的时钟稳定时间较长 */

        (VOID)SpiFpga_Reset(u32Chn);
        SAL_msleep(100);

        /* FPGA调试寄存器在赋值后，必须读一次才会清掉之前的复位，读两次可以滤掉当前时间之前出现的错误 */
        SpiFpga_IsRecSuccess(u32Chn);
        if (SAL_TRUE == SpiFpga_IsRecSuccess(u32Chn))
        {
            return SAL_SOK;
        }
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : SpiFpga_Init
* 描  述  : 初始化FPGA
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 SpiFpga_Init(UINT32 u32Chn)
{
    UINT32 u32Val = 0;

    if (SAL_SOK == SpiFpga_Read(0x01, &u32Val))
    {
        FPGA_LOGI("fpga version:0x%x\n", u32Val);
    }
    sprintf((CHAR *)g_au8BuildTime[u32Chn], "%x", u32Val);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Fpga_Init
* 描  述  : 初始化FPGA
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Fpga_Init(UINT32 u32Chn)
{
    HARDWARE_INPUT_CHIP_E enChip = HARDWARE_GetInputChip();

    if (SAL_TRUE != g_abFpgaFuncInited[u32Chn])
    {
        g_abFpgaFuncInited[u32Chn] = SAL_TRUE;
        switch (enChip)
        {
            case HARDWARE_INPUT_CHIP_ADV7842:
                g_astFpgaFunc[u32Chn].pFuncInit = SpiFpga_Init;
                g_astFpgaFunc[u32Chn].pFuncReset = NULL;
                g_astFpgaFunc[u32Chn].pFuncSetResolution = SpiFpga_SetResolution;
                break;
            case HARDWARE_INPUT_CHIP_MSTAR:
                g_astFpgaFunc[u32Chn].pFuncInit = SpiFpga_Init;
                g_astFpgaFunc[u32Chn].pFuncReset = SpiFpga_DectedAndReset;
                g_astFpgaFunc[u32Chn].pFuncSetResolution = SpiFpga_SetResolution;
                break;
            case HARDWARE_INPUT_CHIP_MCU_MSTAR:
                g_astFpgaFunc[u32Chn].pFuncInit = mcu_func_fpgaInit;
                g_astFpgaFunc[u32Chn].pFuncReset = NULL;
                g_astFpgaFunc[u32Chn].pFuncSetResolution = mcu_func_fpgaSetInputResolution;
                break;
            case HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR:
                g_astFpgaFunc[u32Chn].pFuncInit = mcu_func_fpgaInit;
                g_astFpgaFunc[u32Chn].pFuncReset = NULL;
                g_astFpgaFunc[u32Chn].pFuncSetResolution = mcu_func_fpgaSetInputResolution;
                break;
            default:
                g_astFpgaFunc[u32Chn].pFuncInit = NULL;
                g_astFpgaFunc[u32Chn].pFuncReset = NULL;
                g_astFpgaFunc[u32Chn].pFuncSetResolution = NULL;
                FPGA_LOGE("unsupport capt chip[%d]\n", enChip);
                return SAL_FAIL;
        }
    }

    if (SAL_SOK != g_astFpgaFunc[u32Chn].pFuncInit(u32Chn))
    {
        FPGA_LOGE("fpga[%u] init fail\n", u32Chn);
        return SAL_FAIL;
    }

    if (HARDWARE_INPUT_CHIP_MCU_MSTAR == enChip || HARDWARE_INPUT_CHIP_LT9211_MCU_MSTAR == enChip)
    {
        mcu_func_fpgaGetBuildTime(u32Chn, g_au8BuildTime[u32Chn], sizeof(g_au8BuildTime[u32Chn]));
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : Fpga_Reset
* 描  述  : FPGA复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Fpga_Reset(UINT32 u32Chn)
{
    if (NULL != g_astFpgaFunc[u32Chn].pFuncReset)
    {
        return g_astFpgaFunc[u32Chn].pFuncReset(u32Chn);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Fpga_SetResolution
* 描  述  : 设置输入分辨率
* 输  入  : UINT32 u32Chn
            UINT32 u32Width
            UINT32 u32Height
            UINT32 u32Fps
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Fpga_SetResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps)
{
    if (NULL != g_astFpgaFunc[u32Chn].pFuncSetResolution)
    {
        return g_astFpgaFunc[u32Chn].pFuncSetResolution(u32Chn, u32Width, u32Height, u32Fps);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Fpga_GetBuildTime
* 描  述  : 获取固件编译时间
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Fpga_GetBuildTime(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32Len)
{
    strncpy((CHAR *)pu8Buff, (const CHAR *)g_au8BuildTime[u32Chn], u32Len);
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Fpga_Scale
* 描  述  : 对输入分辨率做相应缩放
* 输  入  : VIDEO_RES_S *pstDst
            VIDEO_RES_S *pstSrc
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Fpga_Scale(VIDEO_RES_S *pstDst, VIDEO_RES_S *pstSrc)
{
    UINT32 u32Width = pstSrc->u32Width & (~(FPGA_WIDTH_ALIGN_NUM - 1));
    UINT32 u32Height = pstSrc->u32Height & (~(FPGA_HEIGHT_ALIGN_NUM - 1));

    *pstDst = *pstSrc;
    if (SAL_TRUE == capb_get_capt()->bMcuEnable) {
        pstDst->u32Width = u32Width + ((pstDst->u32Width - u32Width > FPGA_WIDTH_ALIGN_NUM / 2) ? FPGA_WIDTH_ALIGN_NUM : 0);
        pstDst->u32Height = u32Height + ((pstDst->u32Height - u32Height > FPGA_HEIGHT_ALIGN_NUM / 2) ? FPGA_HEIGHT_ALIGN_NUM : 0);
    }

    return SAL_SOK;
}

