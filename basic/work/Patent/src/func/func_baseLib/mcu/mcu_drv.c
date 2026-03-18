
/*******************************************************************************
 * mcu_drv.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2021年06月07日 Create
 *
 * Description : mcu通信相关
 * Modification: 
 *******************************************************************************/

#include <time.h>
#include "sal.h"
#include "platform_hal.h"
#include "mcu_expand_gpio.h"
#include "mcu_drv.h"


#define MCU_MSTAR_WIDTH_MAX     (1920)
#define MCU_MSTAR_HEIGHT_MAX    (1080)
#define MCU_NUM_MAX                         2

/* 与MCU连接的外设定义 */
typedef enum {
    MCU_PERIPH_MOD_MASTER = 0x00,                                       /* 主控，当前为海思3559av100 */
    MCU_PERIPH_MOD_MCU = 0x10,                                          /* MCU */
    MCU_PERIPH_MOD_FPGA = 0x20,                                         /* FPGA */
    MCU_PRRIPH_MOD_MSTAR = 0x30,                                        /* MSTAR */
    MCU_PERIPH_MOD_CH7053  = 0x40,                                      /* ch7053 */
    MCU_PERIPH_MOD_MOINTOR = 0x50,                                      /* 显示器 */
    
    MCU_PERIPH_MOD_INVALID = 0xFF,
} MCU_PERIPH_MOD_E;

/* 心跳信息结构体 */
typedef struct {
    SAL_ThrHndl stThreadHndl;
    BOOL bInited;
    volatile BOOL abStarted[MCU_NUM_MAX];
    BOOL au32Level[MCU_NUM_MAX];
} MCU_HEART_BEAT_INFO_S;

/* mcu升级相关结构体 */
typedef struct
{
    UINT8 u8Version;
    UINT8 au8Cmd[16];
    UINT8 au8ChipID[8];
    INT32 s32ReadProtect;
    INT32 s32WriteProtect;
}MCU_UPDATE_BOARDINFO;

/* T1测试GPIO */
typedef struct
{
    UINT32 u32Chn;
    UINT32 u32Level;
} T1_GPIO_LEVEL_S;

/* T1测试相关的MCU GPIO通道号 */
typedef enum {
    T1_MCU_GPIO_SWITCH = 0,
    T1_MCU_GPIO_BOOT0,
    T1_MCU_GPIO_BOOT1,
    T1_MCU_GPIO_RES0,

    T1_MCU_GPIO_BUTT,
} T1_MCU_GPIO_E;

/* T1测试相关的FPGA GPIO通道号 */
typedef enum {
    T1_FPGA_GPIO_CFG_DONE = 0,
    T1_FPGA_GPIO_CFG_INIT_B,
    T1_FPGA_GPIO_CFG_PROG_B,
    T1_FPGA_GPIO_RES0,
    T1_FPGA_GPIO_RES1,

    T1_FPGA_GPIO_BUTT,
} T1_FPGA_GPIO_E;

/* T1测试相关的MSTAR GPIO通道号 */
typedef enum {
    T1_MSTAR_GPIO_55 = 0,
    T1_MSTAR_GPIO_56,

    T1_MSTAR_GPIO_BUTT,
} T1_MSTAR_GPIO_E;

/* 分辨率过大时需要缩放的比例 */
static const UINT32 g_au32McuRatio[][2] = {
    {4, 3},
    {5, 4},
    {16, 9},
};


/* 接收到数据后的回调 */
typedef INT32(*MCU_RECEIVE_CB_F)(MCU_PERIPH_MOD_E enPeriph, UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg);

#define MCU_SEND_START_CODE                 0x7F                                /* 发送的起始码 */
#define MCU_RESPONSE_START_CODE             0x7E                                /* 返回的起始码 */

#define MCU_START_CODE_POS                  (0)
#define MCU_START_CODE_LEN                  (1)
#define MCU_DATA_LEN_POS                    (MCU_START_CODE_POS + MCU_START_CODE_LEN)
#define MCU_DATA_LEN_LEN                    (2)
#define MCU_SRC_MOD_POS                     (MCU_DATA_LEN_POS + MCU_DATA_LEN_LEN)
#define MCU_SRC_MOD_LEN                     (1)
#define MCU_DST_MOD_POS                     (MCU_SRC_MOD_POS + MCU_SRC_MOD_LEN)
#define MCU_DST_MOD_LEN                     (1)
#define MCU_CMD_POS                         (MCU_DST_MOD_POS + MCU_DST_MOD_LEN)
#define MCU_CMD_LEN                         (1)
#define MCU_HEAD_LEN                        (MCU_CMD_POS + MCU_CMD_LEN)
#define MCU_DATA_POS                        (MCU_HEAD_LEN)
#define MCU_CRC_LEN                         (1)
#define MCU_MIN_LEN                         (MCU_HEAD_LEN + MCU_CRC_LEN)


/*********************************** 通用指令 *********************************/
/* 系统相关 */
#define MCU_CMD_SYSTEM_BASE                 0x00                                /* 系统相关指令基地址 */
#define MCU_CMD_RST                         MCU_CMD_SYSTEM_BASE + 0x00          /* 设备复位 */
#define MCU_CMD_GET_VERSION                 MCU_CMD_SYSTEM_BASE + 0x01          /* 获取软件版本 */
#define MCU_CMD_GET_BUILD_TIME              MCU_CMD_SYSTEM_BASE + 0x02          /* 获取编译时间 */
#define MCU_CMD_WRITE_REG                   MCU_CMD_SYSTEM_BASE + 0x03          /* 写寄存器 */
#define MCU_CMD_READ_REG                    MCU_CMD_SYSTEM_BASE + 0x04          /* 读寄存器 */
#define MCU_CMD_TIME_SYNC                   MCU_CMD_SYSTEM_BASE + 0x05          /* 时间同步*/

/* 输入输出相关 */
#define MCU_CMD_INOUT_BASE                  0x10                                /* 输入输出相关指令基地址 */
#define MCU_CMD_SET_IN_RES                  MCU_CMD_INOUT_BASE + 0x00           /* 设置输入分辨率 */
#define MCU_CMD_GET_IN_RES                  MCU_CMD_INOUT_BASE + 0x01           /* 获取输入分辨率 */
#define MCU_CMD_SET_OUT_RES                 MCU_CMD_INOUT_BASE + 0x02           /* 设置输出分辨率 */
#define MCU_CMD_GET_OUT_RES                 MCU_CMD_INOUT_BASE + 0x03           /* 获取输出分辨率 */
#define MCU_CMD_SET_IN_TIMING               MCU_CMD_INOUT_BASE + 0x04           /* 设置输入时序 */
#define MCU_CMD_GET_IN_TIMING               MCU_CMD_INOUT_BASE + 0x05           /* 获取输入时序 */
#define MCU_CMD_SET_OUT_TIMING              MCU_CMD_INOUT_BASE + 0x06           /* 设置输出时序 */
#define MCU_CMD_GET_OUT_TIMING              MCU_CMD_INOUT_BASE + 0x07           /* 获取输出时序 */
#define MCU_CMD_SET_BIND                    MCU_CMD_INOUT_BASE + 0x08           /* 设置绑定关系 */
#define MCU_CMD_GET_OUT_BIND                MCU_CMD_INOUT_BASE + 0x09           /* 获取与输出绑定的输入 */
#define MCU_CMD_GET_IN_BIND                 MCU_CMD_INOUT_BASE + 0x0A           /* 获取与输入绑定的输出 */
#define MCU_CMD_SET_IN_OFFSET               MCU_CMD_INOUT_BASE + 0x0B           /* 设置输入图像的偏移 */
#define MCU_CMD_SET_OUT_OFFSET              MCU_CMD_INOUT_BASE + 0x0C           /* 设置输出图像的偏移 */
#define MCU_CMD_HOTPLUG                     MCU_CMD_INOUT_BASE + 0x0D           /* 热插拔 */

/* 图像效果相关参数 */
#define MCU_CMD_CSC_BASE                    0x20
#define MCU_CMD_SET_CSC                     MCU_CMD_CSC_BASE + 0x00             /* 设置CSC参数 */
#define MCU_CMD_GET_CSC                     MCU_CMD_CSC_BASE + 0x01             /* 获取CSC参数 */

/* EDID相关参数 */
#define MCU_CMD_EDID_BASE                   0x30
#define MCU_CMD_SET_EDID                    MCU_CMD_EDID_BASE + 0x00            /* 设置CSC参数 */
#define MCU_CMD_GET_EDID                    MCU_CMD_EDID_BASE + 0x01            /* 获取CSC参数 */

/* 数据相关 */
#define MCU_CMD_DATA_BASE                   0x40                                /* 发送接收数据相关 */
#define MCU_CMD_SEND_DATA_START             MCU_CMD_DATA_BASE + 0x00            /* 开始发送数据 */
#define MCU_CMD_SEND_DATA                   MCU_CMD_DATA_BASE + 0x01            /* 发送数据 */
#define MCU_CMD_SEND_DATA_END               MCU_CMD_DATA_BASE + 0x02            /* 结束发送数据 */
#define MCU_CMD_GET_DATA_START              MCU_CMD_DATA_BASE + 0x03            /* 开始获取数据 */
#define MCU_CMD_GET_DATA                    MCU_CMD_DATA_BASE + 0x04            /* 获取数据 */
#define MCU_CMD_GET_DATA_END                MCU_CMD_DATA_BASE + 0x05            /* 获取数据结束 */
#define MCU_CMD_SAVE_DATA                   MCU_CMD_DATA_BASE + 0x06            /* 保存数据 */

/* T1测试 */
#define MCU_CMD_T1_BASE                     0x50
#define MCU_CMD_T1_RESET                    MCU_CMD_T1_BASE + 0x00
#define MCU_CMD_T1_GPIO_READ                MCU_CMD_T1_BASE + 0x01
#define MCU_CMD_T1_GPIO_WRITE               MCU_CMD_T1_BASE + 0x02
#define MCU_CMD_T1_GPIO                     MCU_CMD_T1_BASE + 0x03
#define MCU_CMD_T1_UART                     MCU_CMD_T1_BASE + 0x04
#define MCU_CMD_T1_IIC                      MCU_CMD_T1_BASE + 0x05
#define MCU_CMD_T1_SPI                      MCU_CMD_T1_BASE + 0x06

/* 私有指令 */
#define MCU_CMD_PRIV_BASE                   0xA0                                /* 私有指令 */

/* MCU相关的私有指令 */
#define MCU_CMD_MCU_SET_LOOP                0xA0                                /* 配置环通 */
#define MCU_CMD_MCU_HEART_BEAT_START        0xA1                                /* 开始发送心跳 */
#define MCU_CMD_MCU_HEART_BEAT_STOP         0xA2                                /* 结束发送心跳 */

#define MCU_CMD_SUCCESS                     0x00
#define MCU_CMD_FAIL                        0x01

#define MCU_UART_BAUD_DEFAULT               115200                              /* 默认波特率 */
#define MCU_UART_BUFF_SIZE                  1024                                /* 接收缓存 */

/*********************************** MCU升级相关宏 *********************************/

#define MCU_UPDATE_SYNC                     0x7F          /* mcu和主控芯片连接状态 */
#define MCU_UPDATE_ACK                      0x79          /* mcu应答 */
#define MCU_UPDATE_NACK                     0x1F          /* mcu不应答 */
#define MCU_UPDATE_FLASH_ADDR               0x08000000    /* mcu用户flash地址 */

#define MCU_UPDATE_MAXLEN                   256           /* 每次读写最大字节 */
#define MCU_UPDATE_MAXTRY                   4             /* 重试次数 */
#define MCU_UPDATE_FLASH_MAXLEN            (128 * 1024)   /* 每次读写最大字节 */


/*********************************** MCU boot状态相关指令 *********************************/
/* mcu boot状态下使用的命令 */
#define MCU_UPDATE_GET_SUPPORT_VER_CMD      0x00          /* 获取当前版本的bootloader程序支持的版本和允许的命令 */
#define MCU_UPDATE_GET_VER_PROT_STATUS      0x01          /* 获取bootloader程序版本和闪存;Flash 存储器的读保护状态 */
#define MCU_UPDATE_GET_CHIPID               0x02          /* 获取ChipID */
#define MCU_UPDATE_READ_MEMORY              0x11          /* 从应用程序指定的地址开始读取最多 256 字节的内存 */
#define MCU_UPDATE_GO                       0x21          /* 跳转到位于内部闪存或 SRAM 中的用户应用程序代码 */
#define MCU_UPDATE_WRITE_MEMORY             0x31          /* 从应用程序指定的地址开始，最多可将 256 字节写入 RAM 或闪存 */
/* 擦除(0x43)和扩展擦除(0x44)连个命令不能同时使用 */
#define MCU_UPDATE_ERASE                    0x43          /* 从一个到所有闪存页面擦除 */
#define MCU_UPDATE_EXTENDED_ERASE           0x44          /* 使用两字节寻址模式从一个到所有闪存页面擦除 */
#define MCU_UPDATE_WRITE_PROTECT            0x63          /* 启用某些扇区写保护 */
#define MCU_UPDATE_WRITE_UNPROTECT          0x73          /* 禁用所有闪存扇区的写保护 */
#define MCU_UPDATE_READOUT_PROTECT          0x82          /* 启用读保护 */
#define MCU_UPDATE_READOUT_UNPROTECT        0x92          /* 禁用读保护 */


static MCU_UPDATE_BOARDINFO g_stBoardInfo = {0};

static UART_ID_E g_aenUartMap[MCU_NUM_MAX] = {UART_ID_BUTT, UART_ID_BUTT};           /* 通信串口 */
static UART_ID_E g_aenLogUartMap[MCU_NUM_MAX] = {UART_ID_BUTT, UART_ID_BUTT};        /* 日志串口 */
static BOOL g_bMcuLogCreated = SAL_FALSE;


static BOOL g_abInited[MCU_NUM_MAX] = {SAL_FALSE, SAL_FALSE};
static Handle g_aMutexHandle[MCU_NUM_MAX] = {NULL, NULL};
static MCU_HEART_BEAT_INFO_S g_stHeartBeat = {0};
static int iDriverFd = -1;
static UINT8 g_au8McuTime[MCU_NUM_MAX][32] = {0};
static UINT8 g_au8MstarTime[MCU_NUM_MAX][32] = {0};
static UINT8 g_au8FpgaTime[MCU_NUM_MAX][32] = {0};


/*******************************************************************************
* 函数名  : mcu_drvSalCalCrc
* 描  述  : 计算CRC参数
* 输  入  : UINT32 u32Base :
          UINT8 *pu8Data :
          UINT32 u32Len :
* 输  出  : 
* 返回值  : 计算后的CRC参数
*******************************************************************************/
static UINT32 mcu_drvSalCalCrc(UINT32 u32Base, const UINT8 *pu8Data, UINT32 u32Len)
{
    UINT32 u32Crc = u32Base;

    while (u32Len--)
    {
        u32Crc += *pu8Data++;
    }

    return u32Crc;
}


/*******************************************************************************
* 函数名  : mcu_drvReadPin
* 描  述  : 读GPIO引脚电平
* 输  入  : UINT32 u32Chn
            S32 s32Cmd
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvReadPin(UINT32 u32Chn, S32 s32Cmd, UINT32 *pu32Level)
{
    DDM_miscIOCArgs stArg;

    stArg.ch  = u32Chn;
    if (ioctl(iDriverFd, s32Cmd, &stArg) < 0)
    {
        return SAL_FAIL;
    }

    *pu32Level = stArg.val;
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvSetPin
* 描  述  : 配置引脚
* 输  入  : UINT32 u32Chn
            S32 s32Cmd
            UINT32 u32Level
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvSetPin(UINT32 u32Chn, S32 s32Cmd, UINT32 u32Level)
{
    DDM_miscIOCArgs stArg;

    stArg.ch  = u32Chn;
    stArg.val = (u32Level > 0) ? 1 : 0;
    if (ioctl(iDriverFd, s32Cmd, &stArg) < 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvSetRstPin
* 描  述  : 配置引脚
* 输  入  : UINT32 u32Chn
            UINT32 u32Level
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvSetRstPin(UINT32 u32Chn, UINT32 u32Level)
{
    if (SAL_SOK != mcu_drvSetPin(u32Chn, DDM_IOC_MCU_RST, u32Level))
    {
        MCU_LOGE("mcu[%u] set reset pin[%u] fail\n", u32Chn, u32Level);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvSetBoot0Pin
* 描  述  : 配置引脚
* 输  入  : UINT32 u32Chn
            UINT32 u32Level
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvSetBoot0Pin(UINT32 u32Chn, UINT32 u32Level)
{
    if (SAL_SOK != mcu_drvSetPin(u32Chn, DDM_IOC_MCU_BOOT0, u32Level))
    {
        MCU_LOGE("mcu[%u] set boot0 pin[%u] fail\n", u32Chn, u32Level);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvSetBoot1Pin
* 描  述  : 配置引脚
* 输  入  : UINT32 u32Chn
            UINT32 u32Level
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvSetBoot1Pin(UINT32 u32Chn, UINT32 u32Level)
{
    if (SAL_SOK != mcu_drvSetPin(u32Chn, DDM_IOC_MCU_BOOT1, u32Level))
    {
        MCU_LOGE("mcu[%u] set boot1 pin[%u] fail\n", u32Chn, u32Level);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvDriverOpen
* 描  述  : 打开DDM句柄
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvDriverOpen(VOID)
{
    UINT32 i = 0;

    if (iDriverFd < 0)
    {
        iDriverFd = open(DDM_MISC_DEVICE, O_RDWR);
        if(iDriverFd < 0)
        {
            MCU_LOGE("mcu open driver:%s fail\n", DDM_MISC_DEVICE);
            return SAL_FAIL;
        }

        for (; i < MCU_NUM_MAX; i++)
        {
            if(mcu_drvSetRstPin(i, 1) !=  SAL_SOK)
            {
                MCU_LOGE("mcu_drvSetRstPin fail\n");
            }
            if(mcu_drvSetBoot0Pin(i, 0) != SAL_SOK)
            {
                MCU_LOGE("mcu_drvSetBoot0Pin fail\n");
            }
            if( mcu_drvSetBoot1Pin(i, 0) != SAL_SOK)
            {
                MCU_LOGE("mcu_drvSetBoot1Pin fail\n");
            }
        }

        MCU_LOGI("mcu open driver:%s success:%d\n", DDM_MISC_DEVICE, iDriverFd);
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvHeartBeatThread
* 描  述  : 心跳线程
* 输  入  : VOID *pvArg
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID *mcu_drvHeartBeatThread(VOID *pvArg)
{
    UINT32 i = 0;
    UINT32 u32CurTime = 0;
    UINT32 u32LastTime = 0;
    UINT32 u32SetMcuPinTime = 0;
        
    prctl(PR_SET_NAME, (unsigned long)"mcu_drvHeartBeatThread");
    SAL_thrChangePri(&g_stHeartBeat.stThreadHndl, SAL_THR_PRI_MAX - 5);

    while (1)
    {
        u32SetMcuPinTime = SAL_getCurMs();
        for (i = 0; i < MCU_NUM_MAX; i++)
        {
            if (SAL_TRUE == g_stHeartBeat.abStarted[i])
            {
                g_stHeartBeat.au32Level[i] = 1 - g_stHeartBeat.au32Level[i];
                if( mcu_drvSetBoot1Pin(i, g_stHeartBeat.au32Level[i]) != SAL_SOK)
                {
                    MCU_LOGI("mcu_drvSetBoot1Pin failed:i=%u\n", i);
                }
            }
        }
        u32CurTime = SAL_getCurMs();
        if((u32CurTime - u32LastTime) > 200 && u32LastTime != 0)
        {
            MCU_LOGE("mcu heart beat time out %u ,set pin time %u\n", (u32CurTime - u32LastTime),(u32CurTime - u32SetMcuPinTime));
            /* debug */
            if((u32CurTime - u32LastTime) > 300)
            {
                system("cat /proc/loadavg");                                            //查看cpu负载
                system("cat /proc/meminfo");                                            //查看内存使用
            }
        }
        u32LastTime = u32CurTime;
        /*和mcu保活心跳延时*/
        SAL_msleep(100);
    }

    return NULL;
}

/*******************************************************************************
* 函数名  : mcu_drvHeartBeatThreadCreate
* 描  述  : 创建心跳线程
* 输  入  : 
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static VOID mcu_drvHeartBeatThreadCreate(VOID)
{
    UINT32 i = 0;

    if (SAL_TRUE == g_stHeartBeat.bInited)
    {
        MCU_LOGI("mcu beat heart has been inited\n");
        return;
    }

    g_stHeartBeat.bInited = SAL_TRUE;
    SAL_SET_THR_NAME();

    for (; i < MCU_NUM_MAX; i++)
    {
        g_stHeartBeat.abStarted[i] = SAL_FALSE;
        g_stHeartBeat.au32Level[i] = 0;
        if(mcu_drvSetBoot1Pin(i, g_stHeartBeat.au32Level[i]) != SAL_SOK)
        {
            MCU_LOGI("mcu_drvSetBoot1Pin failed i=%u\n", i);
        }
    }
    SAL_thrCreate(&g_stHeartBeat.stThreadHndl, mcu_drvHeartBeatThread, 60,  0,  NULL);
    MCU_LOGI("create mcu beat heart thread success\n");

    return;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateUartRead
* 描  述  : mcu升级时使用的串口读写函数
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateUartRead(UART_ID_E enId, UINT8 *pu8Data, UINT32 u32Len)
{
    INT32     s32ret = SAL_FAIL;
    UINT32    i = 0;
    UINT32    u32Times = 5;
    UINT32    u32ReadSize;
    UINT32    u32ReadLen = 0;
    UINT32    u32ReadBytes = 0;
    UINT8    *pu8Databuff = NULL;

    u32ReadSize = u32Len;
    pu8Databuff = pu8Data;
    
    while (u32ReadLen < u32Len)
    {
        for(i = 0; i < u32Times; i++)
        {
            u32ReadBytes = 0;
            s32ret = UART_Read(enId, pu8Databuff, u32ReadSize, &u32ReadBytes, 1000);
            if ( SAL_SOK != s32ret) 
            {
                SAL_msleep(20);
                continue;
            }
            else
            {
                break;
            }
        }
        if (u32ReadBytes > 0)
        {
            u32ReadLen  += u32ReadBytes;
            pu8Databuff += u32ReadBytes;
            u32ReadSize -= u32ReadBytes;
        }
        else
        {
            MCU_LOGE("MCU_UpdateUart[%d] read %u times,have read %u bytes,but need %u bytes\n", enId, i, u32ReadLen, u32Len);
            return SAL_FAIL;
        }
    }
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateWaitAck
* 描  述  : 等待mcu应答
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateWaitAck(UINT32 u32Chn)
{
    INT32     s32ret = SAL_FAIL;
    UINT8     u8ReadData = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    if( SAL_SOK != mcu_drvUpdateUartRead(enId, &u8ReadData, sizeof(u8ReadData)))
    {
        MCU_LOGE("uart[%d] read fail, need read %u bytes\n", enId, (UINT32)sizeof(u8ReadData));
        return SAL_FAIL;
    }

    if(MCU_UPDATE_ACK == u8ReadData)
    {
        s32ret = SAL_SOK;
    }

    return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateSendCommand
* 描  述  : 在MCU boot状态下，向MCU发送命令
* 输  入  : UINT32 u32Chn
*          UINT8 u8Byte
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateSendCommand(UINT32 u32Chn, UINT8 u8Byte)
{
    UINT8   u8XorByte;
    INT32   s32ret = SAL_FAIL;
    UINT32  u32WriteBytes;
    
    UART_ID_E enId = g_aenUartMap[u32Chn];

    
    u8XorByte = u8Byte ^ 0xFF;
    
    //UART_Flush(enId);
    /* mcu boot状态下，一个命令都是一个字节 */

    s32ret = UART_Write(enId, &u8Byte, 1, &u32WriteBytes);
    if((SAL_SOK != s32ret) || (1 != u32WriteBytes))
    {
        MCU_LOGE("uart[%d] send cmd [0x%2x] fail, %u bytes send success\n", enId, u8Byte, u32WriteBytes);
        return SAL_FAIL;
    }
    
    s32ret = UART_Write(enId, &u8XorByte, 1, &u32WriteBytes);
    if((SAL_SOK != s32ret) || (1 != u32WriteBytes))
    {
        MCU_LOGE("uart[%d] send cmd xor [0x%2x] fail, %u bytes send success\n", enId, u8XorByte, u32WriteBytes);
        return SAL_FAIL;
    }
    s32ret = mcu_drvUpdateWaitAck(u32Chn);
    return s32ret;
}



/*******************************************************************************
* 函数名  : mcu_drvUpdateCmdGet
* 描  述  : 获取mcu可用的命令
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateCmdGet(UINT32 u32Chn)
{
    INT32     i;
    INT32     s32ret = SAL_FAIL;
    UINT8     u8CmdNum = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    if ( mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_GET_SUPPORT_VER_CMD) == SAL_SOK )
    {
        if( SAL_SOK != mcu_drvUpdateUartRead(enId, &u8CmdNum, sizeof(u8CmdNum))) 
        {
            MCU_LOGE("uart[%d] read fail, need read %u bytes\n", enId, (UINT32)sizeof(u8CmdNum));
            return SAL_FAIL;
        }
        
        if( SAL_SOK != mcu_drvUpdateUartRead(enId, &(g_stBoardInfo.u8Version), sizeof(g_stBoardInfo.u8Version))) 
        {
            MCU_LOGE("uart[%d] read fail, need read %u bytes\n", enId, (UINT32)sizeof(g_stBoardInfo.u8Version));
            return SAL_FAIL;
        }
        // 延时等待串口接收完数据
        //SAL_msleep(500);

        if( SAL_SOK != mcu_drvUpdateUartRead(enId, g_stBoardInfo.au8Cmd, u8CmdNum)) 
        {
            MCU_LOGE("uart[%d] read fail, need read %u bytes\n", enId, (UINT32)sizeof(u8CmdNum));
            return SAL_FAIL;
        }
        
        if ( mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK ) 
        {
            MCU_LOGI("Bootloader version: %x.%x\n", (g_stBoardInfo.u8Version)>> 4, (g_stBoardInfo.u8Version) & 0x0F);
            
            for ( i=0; i < u8CmdNum; i++ )
            {
                MCU_LOGI(" Command get-[%02X]\n", g_stBoardInfo.au8Cmd[i]);
            }
            s32ret = SAL_SOK;
        }
    }

    if ( s32ret == SAL_FAIL ) 
    {
        MCU_LOGE("Command-Get-Cmd: command error!\n");
    }
    return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateGetChipID
* 描  述  : 获取MCU的ChipID
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateGetChipID(UINT32 u32Chn)
{
    INT32     i;
    INT32     s32ret = SAL_FAIL;
    UINT8     u8CmdNum;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    if (mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_GET_CHIPID) == SAL_SOK ) 
    {
        if( SAL_SOK != mcu_drvUpdateUartRead(enId, &u8CmdNum, sizeof(u8CmdNum))) 
        {
            MCU_LOGE("uart[%d] read fail, need read %u bytes\n", enId, (UINT32)sizeof(u8CmdNum));
            return SAL_FAIL;
        }
        if( SAL_SOK != mcu_drvUpdateUartRead(enId, g_stBoardInfo.au8ChipID, u8CmdNum + 1)) 
        {
            MCU_LOGE("uart[%d] read fail, need read %u bytes\n", enId, (UINT32)sizeof(u8CmdNum));
            return SAL_FAIL;
        }

        printf("Product ID=");
        for ( i = 0; i < u8CmdNum+1; i++ ) 
        {
            printf("%02X", g_stBoardInfo.au8ChipID[i]);
        }
        printf("\n");

        if ( mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK )
        {
        	s32ret = SAL_SOK;
        }
    }

    if ( SAL_SOK != s32ret ) 
    {
        MCU_LOGE("Command-get-ChipID: command error!\n");
    }

    return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateErase
* 描  述  : 整片擦除MCU flash
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateErase(UINT32 u32Chn)
{
    INT32     s32ret = SAL_FAIL;
    UINT32    u32WriteBytes;
    UINT8     au8Buffer[] = {0xFF, 0xFF, 0x00};
    UART_ID_E enId = g_aenUartMap[u32Chn];
    

    MCU_LOGI("MCU Erase-Erase: [0x%X]\n", g_stBoardInfo.au8Cmd[6]);
    /* 擦除命令有两个：擦除和扩展擦除 */
    if ( g_stBoardInfo.au8Cmd[6] == MCU_UPDATE_ERASE ) 
    {
        if ( mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_ERASE) == SAL_SOK )
        {
            /* 发送擦除命令后，发送0xFF，表示全部清除 */
            if ( mcu_drvUpdateSendCommand(u32Chn, 0xFF) == SAL_SOK )
            {
                s32ret = SAL_SOK;
            } 
            else
            {
                MCU_LOGE("MCU-GlobalErase: fail\n");
                
            }
        } 
        else
        {
            MCU_LOGE("MCU-Erase: fail\n");
        }
    }
    else
    {
        if ( mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_EXTENDED_ERASE) == SAL_SOK )
        {
            s32ret = UART_Write(enId, au8Buffer, sizeof(au8Buffer), &u32WriteBytes);
            if((SAL_SOK != s32ret) || (sizeof(au8Buffer) != u32WriteBytes))
            {
                MCU_LOGE("uart[%d] send cmd fail, %u bytes need send, %u bytes send success\n", enId, (UINT32)sizeof(au8Buffer),u32WriteBytes);
                return SAL_FAIL;
            }
            if ( mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK )
            {
                s32ret = SAL_SOK;
            } 
            else
            {
                MCU_LOGE("MCU-GlobalExErase: fail\n");
            }
        }
        else
        {
            MCU_LOGE("MCU-ExErase: fail\n");
        }
    }

    if ( SAL_SOK == s32ret ) 
    {
        MCU_LOGI("MCU-Erase: OK\n");
    }

    return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateAddrPacket
* 描  述  : 向mcu发送打包 + 异或校验后的地址数据
* 输  入  : UINT32 u32Addr
*          UINT8 * pu8Buffer
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static UINT8 * mcu_drvUpdateAddrPacket(UINT32 u32Addr, UINT8 * pu8Buffer)
{
    pu8Buffer[0] = (u32Addr >> 24) & 0xFF;
    pu8Buffer[1] = (u32Addr >> 16) & 0xFF;
    pu8Buffer[2] = (u32Addr >> 8)  & 0xFF;
    pu8Buffer[3] = (u32Addr >> 0)  & 0xFF;
    pu8Buffer[4] = pu8Buffer[0] ^ pu8Buffer[1] ^ pu8Buffer[2] ^ pu8Buffer[3];

    return pu8Buffer;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateCheckDateXor
* 描  述  : 异或校验向mcu发送的数据
* 输  入  : const UINT8 * pu8data
*               UINT32 u32len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static UINT8 mcu_drvUpdateCheckDateXor(const UINT8 * pu8data, UINT32 u32len)
{
    UINT32  i;
    UINT8  u8CheckDateXor = (UINT8)(u32len-1);
    for ( i = 0; i < u32len; i++ )
    {
        u8CheckDateXor ^= pu8data[i];
    }
	return u8CheckDateXor;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateWriteBlock
* 描  述  : 向mcu写入整块数据，每次最大256字节
* 输  入  : UINT32 u32Chn
*          const UINT8 *pu8Data
*         UINT32 u32Len
*         UINT32 u32Addr
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateWriteBlock(UINT32 u32Chn, const UINT8 *pu8Data, UINT32 u32Len, UINT32 u32Addr)
{
    INT32     s32ret = SAL_FAIL;
    
    UINT8     au8AddrBuffer[5] = {0};
    UINT8     u8SendLen;
    UINT8     u8CheckDateXor;
    UINT32    u32WriteBytes;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    if(u32Len % 4 != 0)
    {
        MCU_LOGE(" Not a multiple of 4 byte \n");
        return SAL_FAIL;
    }
    
    if(u32Len > MCU_UPDATE_MAXLEN)
    {
        MCU_LOGE("write date length [%d],More than [%d] bytes\n",u32Len, MCU_UPDATE_MAXLEN);
        return SAL_FAIL;
    }

    if(NULL == pu8Data)
    {
        MCU_LOGE(" date is null\n");
        return SAL_FAIL;
    }

    u8SendLen = (UINT8)(u32Len - 1);
    
    u8CheckDateXor = mcu_drvUpdateCheckDateXor(pu8Data, u32Len);
    
    if ( mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_WRITE_MEMORY) == SAL_SOK ) 
    {

        s32ret = UART_Write(enId, mcu_drvUpdateAddrPacket(u32Addr, au8AddrBuffer), 5 , &u32WriteBytes);
        if ((SAL_SOK != s32ret) || (5 != u32WriteBytes))
        {
            MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, 5, u32WriteBytes);
            return SAL_FAIL;
        }
        
		if ( mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK )
        {
            if ((SAL_SOK != UART_Write(enId, &u8SendLen, 1, &u32WriteBytes)) || (1 != u32WriteBytes))
            {
                MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, 1, u32WriteBytes);
                return SAL_FAIL;
            }
            if ((SAL_SOK != UART_Write(enId, pu8Data, u32Len, &u32WriteBytes)) || (u32Len != u32WriteBytes))
            {
                MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, u32Len, u32WriteBytes);
                return SAL_FAIL;
            }
            if ((SAL_SOK != UART_Write(enId, &u8CheckDateXor, 1, &u32WriteBytes)) || (1 != u32WriteBytes))
            {
                MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, 1, u32WriteBytes);
                return SAL_FAIL;
            }
            
			if ( mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK ) 
            {
				s32ret = SAL_SOK;
			} 
            else 
            {
				MCU_LOGE("MCU-Write: data error!\n");
			}
		} 
        else
        {
			MCU_LOGE("MCU-Write: address error!\n");
		}
	} 
    else
    {
		MCU_LOGE("MCU-Write: command error!\n");
	}

	return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateWriteBin
* 描  述  : 向mcu写入bin文件
* 输  入  : UINT32 u32Chn
*         const UINT8 *pu8Buff
*         UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateWriteBin(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len)
{
    INT32     s32ret = SAL_FAIL;
    UINT32    u32Size = 0;
    UINT32    u32LocAddr = 0;
    UINT32    u32FlashAddr;
    UINT32    u32WrittenLen = 0;
    const UINT8    *pu8SendBuff = NULL;
    //UART_ID_E enId = g_aenUartMap[u32Chn];

    pu8SendBuff  = pu8Buff;
    u32FlashAddr = MCU_UPDATE_FLASH_ADDR;
    u32LocAddr   = u32FlashAddr;

    if(NULL == pu8Buff )
    {
        MCU_LOGE("Buff is null \n");
        return SAL_FAIL;
    }
    if(u32Len > MCU_UPDATE_FLASH_MAXLEN)
    {
        MCU_LOGE("Buff too long: flash length[%d],Buff length[%d]\n", MCU_UPDATE_FLASH_MAXLEN, u32Len);
        return SAL_FAIL;
    }
    
    printf("MCU Wirte Bin start addr: 0x%08X", u32LocAddr);
    while (u32WrittenLen < u32Len) 
    {
        u32Size = u32Len - u32WrittenLen;
        
    	if ( u32Size > MCU_UPDATE_MAXLEN) 
        {
    		u32Size = MCU_UPDATE_MAXLEN;
    	}
    	if ( u32Size % 4 ) 
        {
    		u32Size += 4 - u32Size % 4;
    	}
    	printf(".");
    	//UART_Flush(enId);
        
    	s32ret = mcu_drvUpdateWriteBlock(u32Chn, pu8SendBuff, u32Size, u32LocAddr);
        if (SAL_SOK != s32ret)
        {
            MCU_LOGE("WriteBlock fail:Size[%d],LocAddr[0x%08x]\n", u32Size, u32LocAddr);
            return SAL_FAIL;
        }

        u32WrittenLen += u32Size;
        u32LocAddr    += u32Size;
        pu8SendBuff   += u32Size;
    }
    printf("...Done.\n");

	printf("MCU-Write: success.\n");
	return s32ret;
}


/* mcu升级校验,代码保留,后续debug调试时使用 */

#if 0

/*******************************************************************************
* 函数名  : mcu_drvUpdateReadBlock
* 描  述  : 从mcu读出整块数据，每次最大256字节
* 输  入  :  UINT32 u32Chn
*          UINT8 *pu8Data
*          UINT32 u32Len
*          UINT32 u32Addr
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateReadBlock(UINT32 u32Chn, UINT8 *pu8Data, UINT32 u32Len, UINT32 u32Addr)
{
    INT32     s32ret = SAL_FAIL;
    UINT8     au8AddrBuffer[5] = {0};
    UINT32    u32WriteBytes = 0;
    UART_ID_E enId = g_aenUartMap[u32Chn];

    
    if(u32Len > MCU_UPDATE_MAXLEN)
    {
        MCU_LOGE("write date length [%d],More than [%d] bytes\n",u32Len, MCU_UPDATE_MAXLEN);
        return SAL_FAIL;
    }

    if(NULL == pu8Data)
    {
        MCU_LOGE("Buff is null \n");
        return SAL_FAIL;
    }

    if ( mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_READ_MEMORY) == SAL_SOK )
    {
        s32ret = UART_Write(enId, mcu_drvUpdateAddrPacket(u32Addr, au8AddrBuffer), 5, &u32WriteBytes);
        if ((SAL_SOK != s32ret) || (5 != u32WriteBytes))
        {
            MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, 5, u32WriteBytes);
            return SAL_FAIL;
        }
        if ( mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK )
        {
            if ( mcu_drvUpdateSendCommand(u32Chn, (u32Len - 1)) == SAL_SOK ) 
            {
                if ( mcu_drvUpdateUartRead(enId, pu8Data, u32Len) == SAL_SOK) 
                {
                    s32ret = SAL_SOK;
                }
                else
                {
                    s32ret = SAL_FAIL;
                    MCU_LOGE("MCU-Read: data error!\n");
                }
            }
            else
            {
                s32ret = SAL_FAIL;
                MCU_LOGE("MCU-Read: length error!\n");
            }
        } 
        else
        {
            s32ret = SAL_FAIL;
            MCU_LOGE("MCU-Read: address error!\n");
        }
    } 
    else
    {
        s32ret = SAL_FAIL;
        MCU_LOGE("MCU-Read: command error!\n");
        
    }
    return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateVerify
* 描  述  : 验证写入的bin文件
* 输  入  : UINT32 u32Chn
*         const UINT8 *pu8Buff
*         UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateVerify(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len)
{
    INT32     i;
    INT32     s32ret = SAL_FAIL;
    UINT32    u32ReadLen = 0;
    UINT32    u32Size = 0;
    UINT32    u32LocAddr = 0;
    UINT32    u32FlashAddr;
    UINT8     au8ReadBuffer[MCU_UPDATE_MAXLEN] ={0};
    const UINT8    *pu8DateBuff = NULL;

    UART_ID_E enId = g_aenUartMap[u32Chn];

    if(NULL == pu8Buff )
    {
        MCU_LOGE("Buff is null \n");
        return SAL_FAIL;
    }
    if(u32Len > MCU_UPDATE_FLASH_MAXLEN)
    {
        MCU_LOGE("Buff too long: flash length[%d],Buff length[%d]\n", MCU_UPDATE_FLASH_MAXLEN, u32Len);
        return SAL_FAIL;
    }
    
    pu8DateBuff  = pu8Buff;
    u32FlashAddr = MCU_UPDATE_FLASH_ADDR;
    u32LocAddr   = u32FlashAddr;
  
    MCU_LOGI("MCU Verifying\n");
    
    UART_Flush(enId);
	printf("%08X:", u32FlashAddr);
    while (u32ReadLen < u32Len) 
    {
        u32Size = u32Len - u32ReadLen;
    	if ( u32Size > MCU_UPDATE_MAXLEN) 
        {
    		u32Size = MCU_UPDATE_MAXLEN;
    	}
        
        s32ret = mcu_drvUpdateReadBlock(u32Chn, au8ReadBuffer, u32Size, u32LocAddr);
		if ( s32ret == SAL_SOK )
        {
			for ( i = 0; i < u32Size; i++ )
            {
				if ( pu8DateBuff[u32ReadLen] != au8ReadBuffer[i] )
                {
					MCU_LOGE("not match at Addr[0x%08X]:data[%d]=%02X, chip[%d]=%02X\n", \
                        u32LocAddr, u32ReadLen, pu8DateBuff[u32ReadLen], i, au8ReadBuffer[i]);
                    
					MCU_LOGE("MCU-Verify:not match at [0x%08X]:data=%02X, chip=%02X\n", u32LocAddr, pu8DateBuff[u32ReadLen], au8ReadBuffer[i]);
					return SAL_FAIL;
				}
				u32ReadLen ++;
                
			}
			printf(".");
            u32LocAddr += u32Size;
		} 
        else 
        {
			MCU_LOGE("MCU-Verify: read fail !!!\n");
            return SAL_FAIL;
		}
	}
	printf("\n");

	MCU_LOGI("MCU Verity success.\n");

	return s32ret;
}
#endif

/* gd32f30x mcu使用跳转指令有问题, 改为拉引脚重启, 跳转代码保留, 后续调试时使用 */
#if 0
/*******************************************************************************
* 函数名  : mcu_drvUpdateGo
* 描  述  : MCU跳转到指定地址执行
* 输  入  : UINT32 u32Chn
*         UINT32 u32Addr
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateGo(UINT32 u32Chn, UINT32 u32Addr)
{
    INT32     s32ret = SAL_FAIL;
    UINT8     au8AddrBuffer[5] = {0};
    UINT32    u32WriteBytes;
    UART_ID_E enId = g_aenUartMap[u32Chn];

	MCU_LOGI("MCU-Go: %08X\n", u32Addr);
	if ( mcu_drvUpdateSendCommand(u32Chn, MCU_UPDATE_GO) == SAL_SOK )
    {
        if ((SAL_SOK != UART_Write(enId, mcu_drvUpdateAddrPacket(u32Addr, au8AddrBuffer), 5, &u32WriteBytes)) || (5 != u32WriteBytes))
        {
            MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, 5, u32WriteBytes);
            return SAL_FAIL;
        }
		if (mcu_drvUpdateWaitAck(u32Chn) == SAL_SOK ) 
        {
			MCU_LOGI("MCU-Go: success\n");
			s32ret = SAL_SOK;
		}
        else 
        {
			MCU_LOGE("MCU-Go: address error!\n");
		}

	} 
    else 
	{
		MCU_LOGE("MCU-Go: command error!\n");
	}

	return s32ret;
}
#endif

/*******************************************************************************
* 函数名  : mcu_drvUpdateSync
* 描  述  : 和mcu同步，看是否连接成功
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateSync(UINT32 u32Chn)
{
    INT32   i;
    INT32   s32ret = SAL_FAIL;
    UINT8   u8WriteData;
    UINT32  u32WriteBytes;
    UINT8   u8ReadData;
    UART_ID_E enId = g_aenUartMap[u32Chn];


    u8WriteData = MCU_UPDATE_SYNC;

    printf("Sync\n");

    for( i = 0; i < MCU_UPDATE_MAXTRY; i++ ) 
    {
        printf(".\n");
        s32ret = UART_Write(enId, &u8WriteData, sizeof(u8WriteData), &u32WriteBytes);
        if ((SAL_SOK != s32ret) || (1 != u32WriteBytes))
        {
            MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, (UINT32)sizeof(u8WriteData), u32WriteBytes);
            return SAL_FAIL;
        }
        
        s32ret = mcu_drvUpdateUartRead(enId, &u8ReadData, sizeof(u8ReadData));
        if( SAL_SOK == s32ret) 
        {
            MCU_LOGI("uart[%d] read date 0x[%2x]\n", enId, u8ReadData);
            if ( u8ReadData == MCU_UPDATE_ACK || u8ReadData == MCU_UPDATE_NACK) 
            {
                s32ret = SAL_SOK;
                break;
        	}
        }
    }

	if ( s32ret == SAL_SOK ) 
    {
		MCU_LOGI("Connected to board.\n");
	}
    else
    {
        MCU_LOGE("Can not connect to board\n");
    }

	return s32ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateReboot2Isp
* 描  述  : MCU重启进入boot状态
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateReboot2Isp(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_FAIL;
    s32Ret = mcu_drvSetBoot0Pin(u32Chn, 1);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] Reboot2Isp boot0 set fail\n", u32Chn);
        return SAL_FAIL;
    }
    s32Ret = mcu_drvSetBoot1Pin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] Reboot2Isp boot1 set fail\n", u32Chn);
        return SAL_FAIL;
    }
    /* delay after change bootp */
    SAL_msleep(5);
    MCU_LOGI("Reboot board to isp. \n");
    
    s32Ret = mcu_drvSetRstPin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] Reboot2Isp Rst set 0 fail\n", u32Chn);
        return SAL_FAIL;
    }
    SAL_msleep(200);
    s32Ret = mcu_drvSetRstPin(u32Chn, 1);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] Reboot2Isp Rst set 1 fail\n", u32Chn);
        return SAL_FAIL;
    }
    SAL_msleep(200);

    s32Ret = mcu_drvUpdateSync(u32Chn);
    
    return s32Ret;
}

/*******************************************************************************
* 函数名  : mcu_drvUpdateResetRboot
* 描  述  : 拉复位引脚重启MCU
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvUpdateResetRboot(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_FAIL;
    s32Ret = mcu_drvSetBoot0Pin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_updateReset boot0 set fail\n", u32Chn);
        return SAL_FAIL;
    }
    s32Ret = mcu_drvSetBoot1Pin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_updateReset boot1 set fail\n", u32Chn);
        return SAL_FAIL;
    }
    /* delay after change bootp */
    SAL_msleep(5);
    MCU_LOGI("mcu Update Reset Rboot. \n");
    
    s32Ret = mcu_drvSetRstPin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_updateReset Rst set 0 fail\n", u32Chn);
        return SAL_FAIL;
    }
    SAL_msleep(200);
    s32Ret = mcu_drvSetRstPin(u32Chn, 1);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_updateReset Rst set 1 fail\n", u32Chn);
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvCmdHeaderPacket
* 描  述  : 命令头打包
* 输  入  : MCU_PERIPH_MOD_E enPeriph
            UINT8 u8Cmd
            UINT32 u32TotalLen
            UINT8 *pu8Buff
* 输  出  : 
* 返回值  : 
*******************************************************************************/
static INT32 mcu_drvCmdHeaderPacket(MCU_PERIPH_MOD_E enPeriph, UINT8 u8Cmd, UINT32 u32TotalLen, UINT8 *pu8Buff)
{
    INT32 i = 0;

    pu8Buff[i++] = MCU_SEND_START_CODE;
    pu8Buff[i++] = (u32TotalLen >> 8) & 0xFF;
    pu8Buff[i++] = u32TotalLen & 0xFF;
    pu8Buff[i++] = MCU_PERIPH_MOD_MASTER;
    pu8Buff[i++] = (UINT8)enPeriph;
    pu8Buff[i++] = u8Cmd;
    
    return i;
}


/*******************************************************************************
* 函数名  : mcu_drvMcuPrivCmdPacket
* 描  述  : 封装mcu相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : 封装的字节数
*******************************************************************************/
static INT32 mcu_drvMcuPrivCmdPacket(UINT8 u8Cmd, UINT8 *pu8Data, VOID *pvArg, UINT32 u32Len)
{
    INT32 i = 0;

    switch (u8Cmd)
    {
        case MCU_CMD_MCU_SET_LOOP:
        {
            MCU_MODE_E *penMode = (MCU_MODE_E *)pvArg;
            pu8Data[i++] = *penMode;
            break;
        }
        case MCU_CMD_MCU_HEART_BEAT_START:
        case MCU_CMD_MCU_HEART_BEAT_STOP:
            break;
        default:
            MCU_LOGE("mcu unsupport priv cmd[0x%x]\n", u8Cmd);
            return 0;
    }

    return i;
}


/*******************************************************************************
* 函数名  : mcu_drvMstarPrivCmdPacket
* 描  述  : 封装mstar相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : 封装的字节数
*******************************************************************************/
static INT32 mcu_drvMstarPrivCmdPacket(UINT8 u8Cmd, UINT8 *pu8Data, VOID *pvArg, UINT32 u32Len)
{
    return 0;
}


/*******************************************************************************
* 函数名  : mcu_drvFpgaPrivCmdPacket
* 描  述  : 封装fpga相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : 封装的字节数
*******************************************************************************/
static INT32 mcu_drvFpgaPrivCmdPacket(UINT8 u8Cmd, UINT8 *pu8Data, VOID *pvArg, UINT32 u32Len)
{
    return 0;
}


/*******************************************************************************
* 函数名  : mcu_drvCh7053PrivCmdPacket
* 描  述  : 封装ch7053相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : 封装的字节数
*******************************************************************************/
static INT32 mcu_drvCh7053PrivCmdPacket(UINT8 u8Cmd, UINT8 *pu8Data, VOID *pvArg, UINT32 u32Len)
{
    return 0;
}


/*******************************************************************************
* 函数名  : mcu_drvMointorPrivCmdPacket
* 描  述  : 封装显示器相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : 封装的字节数
*******************************************************************************/
static INT32 mcu_drvMointorPrivCmdPacket(UINT8 u8Cmd, UINT8 *pu8Data, VOID *pvArg, UINT32 u32Len)
{
    return 0;
}


/*******************************************************************************
* 函数名  : mcu_drvCmdPacket
* 描  述  : 将对应的数据打包到缓存中
* 输  入  : MCU_PERIPH_MOD_E enPeriph
            UINT8 u8Cmd
            UINT8 *pu8Data
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : 封装的字节数
*******************************************************************************/
static INT32 mcu_drvCmdPacket(MCU_PERIPH_MOD_E enPeriph, UINT8 u8Cmd, UINT8 *pu8Data, VOID *pvArg, UINT32 u32Len)
{
    INT32 i = 0;

    if (u8Cmd >= MCU_CMD_PRIV_BASE)
    {
        switch (enPeriph)
        {
            case MCU_PERIPH_MOD_MCU:
                return mcu_drvMcuPrivCmdPacket(u8Cmd, pu8Data, pvArg, u32Len);
            case MCU_PERIPH_MOD_FPGA:
                return mcu_drvFpgaPrivCmdPacket(u8Cmd, pu8Data, pvArg, u32Len);
            case MCU_PRRIPH_MOD_MSTAR:
                return mcu_drvMstarPrivCmdPacket(u8Cmd, pu8Data, pvArg, u32Len);
            case MCU_PERIPH_MOD_CH7053:
                return mcu_drvCh7053PrivCmdPacket(u8Cmd, pu8Data, pvArg, u32Len);
            case MCU_PERIPH_MOD_MOINTOR:
                return mcu_drvMointorPrivCmdPacket(u8Cmd, pu8Data, pvArg, u32Len);
            default:
                MCU_LOGE("mcu unsupport mod[0x%x] cmd[0x%x]\n", enPeriph, u8Cmd);
                return 0;
        }
    }

    switch (u8Cmd)
    {
        case MCU_CMD_T1_RESET:
            break;
        case MCU_CMD_T1_GPIO_READ:
        case MCU_CMD_T1_GPIO_WRITE:
        {
            T1_GPIO_LEVEL_S *pstLevel = (T1_GPIO_LEVEL_S *)pvArg;
            pu8Data[i++] = pstLevel->u32Chn;
            pu8Data[i++] = pstLevel->u32Level;
            break;
        }
        case MCU_CMD_T1_GPIO:
        case MCU_CMD_T1_UART:
        case MCU_CMD_T1_IIC:
        case MCU_CMD_T1_SPI:
        {
            UINT32 *pu32Chn = (UINT32 *)pvArg;
            pu8Data[i++] = *pu32Chn;
            break;
        }
        case MCU_CMD_TIME_SYNC:
        {
            struct tm *timeinfo = (struct tm *)pvArg;
            pu8Data[i++] = ((timeinfo->tm_year + 1900) >> 8) & 0xFF;
            pu8Data[i++] = (timeinfo->tm_year + 1900) & 0xFF;
            pu8Data[i++] = (timeinfo->tm_mon + 1) & 0xFF;
            pu8Data[i++] = timeinfo->tm_mday & 0xFF;
            pu8Data[i++] = timeinfo->tm_hour & 0xFF;
            pu8Data[i++] = timeinfo->tm_min & 0xFF;
            pu8Data[i++] = timeinfo->tm_sec & 0xFF;
            pu8Data[i++] = 0;
            pu8Data[i++] = 0;
            break;
        }
        case MCU_CMD_SET_IN_RES:
        case MCU_CMD_SET_OUT_RES:
        {
            VIDEO_IO_STATUS_S *pstVideo = (VIDEO_IO_STATUS_S *)pvArg;
            pu8Data[i++] = pstVideo->enCable;
            pu8Data[i++] = ((pstVideo->stRes.u32Width) >> 8) & 0xFF;
            pu8Data[i++] = (pstVideo->stRes.u32Width) & 0xFF;
            pu8Data[i++] = ((pstVideo->stRes.u32Height) >> 8) & 0xFF;
            pu8Data[i++] = (pstVideo->stRes.u32Height) & 0xFF;
            pu8Data[i++] = (pstVideo->stRes.u32Fps) & 0xFF;
            break;
        }
        case MCU_CMD_GET_IN_RES:
        case MCU_CMD_GET_IN_TIMING:
        case MCU_CMD_GET_OUT_TIMING:
        case MCU_CMD_GET_OUT_BIND:
        case MCU_CMD_GET_IN_BIND:
        case MCU_CMD_GET_EDID:
        case MCU_CMD_HOTPLUG:
        {
            VIDEO_CABLE_E *penCable = (VIDEO_CABLE_E *)pvArg;
            pu8Data[i++] = *penCable;
            break;
        }
        case MCU_CMD_SET_IN_OFFSET:
        case MCU_CMD_SET_OUT_OFFSET:
        {
            VIDEO_CABLE_PARAM_S *pstParam = (VIDEO_CABLE_PARAM_S *)pvArg;
            pu8Data[i++] = pstParam->enCable;
            u32Len = pstParam->u32Len;
            if (sizeof(VIDEO_OFFSET_E) == u32Len)
            {
                VIDEO_OFFSET_E *penOffset = (VIDEO_OFFSET_E *)pstParam->pvArg;
                pu8Data[i++] = *penOffset;
            }
            else if ((u32Len > 0) && (0 == (u32Len % sizeof(VIDEO_OFFSET_S))))
            {
                VIDEO_OFFSET_S *pstOffset = (VIDEO_OFFSET_S *)pstParam->pvArg;
                for (UINT32 j = 0; j < (u32Len % sizeof(VIDEO_OFFSET_S)); j++)
                {
                    pu8Data[i++] = pstOffset->enDir;
                    pu8Data[i++] = (pstOffset->u32Pixel >> 8) & 0xFF;
                    pu8Data[i++] = pstOffset->u32Pixel & 0xFF;
                    pstOffset++;
                }
            }
            else
            {
                MCU_LOGE("mcu cmd[0x%x] invalid len[%u]\n", u8Cmd, u32Len);
                return 0;
            }
            break;
        }
        case MCU_CMD_SET_CSC:
        {
            VIDEO_CSC_PARAM_S *pstCsc = (VIDEO_CSC_PARAM_S *)pvArg;
            if ((u32Len > 0) && (0 == (u32Len % sizeof(VIDEO_CSC_PARAM_S))))
            {
                for (UINT32 j = 0; j < u32Len/sizeof(VIDEO_CSC_PARAM_S); j++)
                {
                    pu8Data[i++] = pstCsc->enCsc;
                    pu8Data[i++] = pstCsc->u32Value;
                    pstCsc++;
                }
            }
            else
            {
                MCU_LOGE("mcu cmd[0x%x] invalid len[%u]\n", u8Cmd, u32Len);
                return 0;
            }
            break;
        }
        case MCU_CMD_GET_CSC:
        {
            VIDEO_CSC_E *pstCsc = (VIDEO_CSC_E *)pvArg;
            if ((u32Len > 0) && (0 == (u32Len % sizeof(VIDEO_CSC_E))))
            {
                for (UINT32 j = 0; j < u32Len/sizeof(VIDEO_CSC_E); j++)
                {
                    pu8Data[i++] = *pstCsc++;
                }
            }
            else
            {
                MCU_LOGE("mcu cmd[0x%x] invalid len[%u]\n", u8Cmd, u32Len);
                return 0;
            }
            break;
        }
        case MCU_CMD_SET_IN_TIMING:
        case MCU_CMD_SET_OUT_TIMING:
        {
            VIDEO_TIMING_S *pstTiming = (VIDEO_TIMING_S *)pvArg;
            pu8Data[i++] = pstTiming->enCable;
            pu8Data[i++] = ((pstTiming->stTiming.u32Width) >> 8) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32Width) & 0xFF;
            pu8Data[i++] = ((pstTiming->stTiming.u32HFrontPorch) >> 8) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32HFrontPorch) & 0xFF;
            pu8Data[i++] = ((pstTiming->stTiming.u32HSync) >> 8) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32HSync) & 0xFF;
            pu8Data[i++] = ((pstTiming->stTiming.u32HBackPorch) >> 8) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32HBackPorch) & 0xFF;
            pu8Data[i++] = pstTiming->stTiming.enHSPol;
            pu8Data[i++] = ((pstTiming->stTiming.u32Height) >> 8) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32Height) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32VFrontPorch) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32VSync) & 0xFF;
            pu8Data[i++] = (pstTiming->stTiming.u32VBackPorch) & 0xFF;
            pu8Data[i++] = pstTiming->stTiming.enVSPol;
            break;
        }
        case MCU_CMD_SET_BIND:
        {
            VIDEO_CABLE_E *pstCable = (VIDEO_CABLE_E *)pvArg;
            pu8Data[i++] = *pstCable++;
            pu8Data[i++] = *pstCable++;
            break;
        }
        case MCU_CMD_SET_EDID:
        {
            VIDEO_EDID_BUFF_S *pstEdid = (VIDEO_EDID_BUFF_S *)pvArg;
            UINT8 *pu8EdidBuff = pstEdid->au8Buff;
            pu8Data[i++] = pstEdid->enCable;
            for (UINT32 j = 0; j < pstEdid->u32Len; j++)
            {
                pu8Data[i++] = *pu8EdidBuff++;
            }
            break;
        }
        case MCU_CMD_RST:
        case MCU_CMD_GET_VERSION:
        case MCU_CMD_GET_BUILD_TIME:
        case MCU_CMD_GET_OUT_RES:
            break;
        default:
            MCU_LOGE("mcu unsupport cmd[0x%x]\n", u8Cmd);
            return 0;
    }

    return i;
}


/*******************************************************************************
* 函数名  : mcu_drvSendCmd
* 描  述  : 将指令信息通过串口发送给mcu
* 输  入  : UART_ID_E enId
            MCU_PERIPH_MOD_E enPeriph
            UINT8 u8Cmd
            UINT8 *pu8Buff
            VOID *pvArg
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvSendCmd(UART_ID_E enId, MCU_PERIPH_MOD_E enPeriph, UINT8 u8Cmd, UINT8 *pu8Buff, VOID *pvArg, UINT32 u32Len)
{
    INT32 s32Len = 0;
    UINT32 u32Total = 0;
    UINT32 u32Crc = 0;
    UINT32 u32SendBytes = 0;

    s32Len = mcu_drvCmdPacket(enPeriph, u8Cmd, pu8Buff + MCU_DATA_POS, pvArg, u32Len);
    if (s32Len < 0)
    {
        MCU_LOGE("mcu packet periph[0x%x] cmd[0x%x] fail\n", enPeriph, u8Cmd);
        return SAL_FAIL;
    }
    u32Total += s32Len;

    s32Len = mcu_drvCmdHeaderPacket(enPeriph, u8Cmd, MCU_HEAD_LEN + u32Total + MCU_CRC_LEN, pu8Buff);
    u32Total += s32Len;
    
    u32Crc = mcu_drvSalCalCrc(0, pu8Buff, u32Total);
    pu8Buff[u32Total++] = (UINT8)u32Crc;
    
    if ((SAL_SOK != UART_Write(enId, pu8Buff, u32Total, &u32SendBytes)) || (u32Total != u32SendBytes))
    {
        MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, u32Total, u32SendBytes);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvWaitResponse
* 描  述  : 接收串口数据
* 输  入  : UART_ID_E enId
            UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32BuffLen
            UINT32 u32TimeOut
            UINT32 *pu32Len
* 输  出  : 
* 返回值  : 数据位置指针
*******************************************************************************/
static UINT8 *mcu_drvWaitResponse(UART_ID_E enId, UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32BuffLen, UINT32 u32TimeOut, UINT32 *pu32Len)
{
    UINT32 u32RecLen = 0;
    UINT32 u32ReadByte = 0;
    UINT32 u32DataLen = 0;
    UINT32 u32Times = 3;
    UINT8 u8Crc = 0;
    UINT32 i = 0;

    do {
        for (i = 0; i < 3; i++)
        {
            u32ReadByte = 0;
            if (SAL_SOK != UART_Read(enId, pu8Data + u32RecLen, u32BuffLen - u32RecLen, &u32ReadByte, u32TimeOut))
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if (u32ReadByte > 0)
        {
            u32RecLen += u32ReadByte;
        }
        else
        {
            MCU_LOGE("mcu uart[%d] read %u times, %u bytes, but need %u bytes\n", enId, i, u32RecLen, u32DataLen);
            return NULL;
        }

        if (u32RecLen < MCU_MIN_LEN)
        {
            u32Times--;
            if (0 == u32Times)
            {
                MCU_LOGE("mcu uart[%d] read %u bytes, at least %u bytes\n", enId, u32RecLen, MCU_MIN_LEN);
                return NULL;
            }
            else
            {
                continue;
            }
        }

        if ((0 == u32DataLen) && (u32RecLen >= MCU_DATA_LEN_POS + MCU_DATA_LEN_LEN))
        {
            if ((MCU_SEND_START_CODE != pu8Data[MCU_START_CODE_POS]) && (MCU_RESPONSE_START_CODE != pu8Data[MCU_START_CODE_POS]))
            {
                MCU_LOGE("mcu uart[%d] receive lead code[0x%x] error\n", enId, pu8Data[MCU_START_CODE_POS]);
                return NULL;
            }

            u32DataLen = (((UINT32)pu8Data[MCU_DATA_LEN_POS]) << 8) | ((UINT32)pu8Data[MCU_DATA_LEN_POS + 1]);
        }
    } while (u32RecLen < u32DataLen);

#if 0
    printf("uart[%d] read %u bytes:", enId, u32RecLen);
    for (i = 0; i < u32RecLen; i++)
    {
        printf("0x%x ", pu8Data[i]);
    }
    printf("\n");
#endif
    
    if ((u32DataLen <= MCU_MIN_LEN) || (u32DataLen > u32RecLen))
    {
        MCU_LOGE("mcu uart[%d] receive %u bytes, buff len[%u] error\n", enId, u32RecLen, u32DataLen);
        return NULL;
    }

    if (u8Cmd != pu8Data[MCU_CMD_POS])
    {
        MCU_LOGE("mcu uart[%d] receive cmd[0x%x] fail\n", enId, u8Cmd);
        return NULL;
    }

    /* 校验CRC参数 */
    u8Crc = (UINT8)mcu_drvSalCalCrc(0, pu8Data, u32DataLen - 1);
    if (u8Crc != pu8Data[u32DataLen - 1])
    {
        MCU_LOGE("mcu uart[%d] check crc[0x%x 0x%x] fail\n", enId, u8Crc, pu8Data[u32DataLen - 1]);
        return NULL;
    }

    *pu32Len = u32DataLen - MCU_MIN_LEN;
    return pu8Data + MCU_HEAD_LEN;
}


/*******************************************************************************
* 函数名  : mcu_drvMcuPrivHandler
* 描  述  : 处理mcu相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32Len
            VOID *pvArg
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMcuPrivHandler(UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg)
{
    INT32 s32Ret = SAL_SOK;

    switch (u8Cmd)
    {
        case MCU_CMD_MCU_SET_LOOP:
        case MCU_CMD_MCU_HEART_BEAT_START:
        case MCU_CMD_MCU_HEART_BEAT_STOP:
            s32Ret = ((1 == u32Len) && (pu8Data[0] == MCU_CMD_SUCCESS)) ? SAL_SOK : SAL_FAIL;
            break;
        default:
            MCU_LOGE("mcu unsupport priv cmd[0x%x]\n", u8Cmd);
            return SAL_FAIL;
    }

    return s32Ret;
}


/*******************************************************************************
* 函数名  : mcu_drvMstarPrivHandler
* 描  述  : 处理mstar相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32Len
            VOID *pvArg
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMstarPrivHandler(UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg)
{
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvFpgaPrivHandler
* 描  述  : 处理fpga相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32Len
            VOID *pvArg
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvFpgaPrivHandler(UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg)
{
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvCh7053PrivHandler
* 描  述  : 处理ch7053相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32Len
            VOID *pvArg
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvCh7053PrivHandler(UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg)
{
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvMointorPrivHandler
* 描  述  : 处理显示器相关的私有指令
* 输  入  : UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32Len
            VOID *pvArg
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMointorPrivHandler(UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg)
{
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvReceiveHandler
* 描  述  : 处理串口接收到的数据
* 输  入  : MCU_PERIPH_MOD_E enPeriph
            UINT8 u8Cmd
            UINT8 *pu8Data
            UINT32 u32Len
            VOID *pvArg
            UINT32 u32ArgLen
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*          SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvReceiveHandler(MCU_PERIPH_MOD_E enPeriph, UINT8 u8Cmd, UINT8 *pu8Data, UINT32 u32Len, VOID *pvArg, UINT32 u32ArgLen)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;

    if (u8Cmd >= MCU_CMD_PRIV_BASE)
    {
        switch (enPeriph)
        {
            case MCU_PERIPH_MOD_MCU:
                return mcu_drvMcuPrivHandler(u8Cmd, pu8Data, u32Len, pvArg);
            case MCU_PERIPH_MOD_FPGA:
                return mcu_drvFpgaPrivHandler(u8Cmd, pu8Data, u32Len, pvArg);
            case MCU_PRRIPH_MOD_MSTAR:
                return mcu_drvMstarPrivHandler(u8Cmd, pu8Data, u32Len, pvArg);
            case MCU_PERIPH_MOD_CH7053:
                return mcu_drvCh7053PrivHandler(u8Cmd, pu8Data, u32Len, pvArg);
            case MCU_PERIPH_MOD_MOINTOR:
                return mcu_drvMointorPrivHandler(u8Cmd, pu8Data, u32Len, pvArg);
            default:
                MCU_LOGE("mcu unsupport mod[0x%x] cmd[0x%x]\n", enPeriph, u8Cmd);
                return SAL_FAIL;
        }
    }

    switch (u8Cmd)
    {
        case MCU_CMD_T1_GPIO_WRITE:
        case MCU_CMD_T1_GPIO_READ:
        case MCU_CMD_T1_RESET:
        case MCU_CMD_T1_GPIO:
        case MCU_CMD_T1_UART:
        case MCU_CMD_T1_IIC:
        case MCU_CMD_T1_SPI:
        {
            s32Ret = ((1 == u32Len) && (pu8Data[0] == MCU_CMD_SUCCESS)) ? SAL_SOK : SAL_FAIL;
            break;
        }
        case MCU_CMD_GET_VERSION:
        {
            CHAR *szVersion = pvArg;
            if ((0 != u32Len) && (u32ArgLen > u32Len))
            {
                //strncpy(szVersion, (const CHAR *)pu8Data, u32Len);
                memcpy(szVersion, (const CHAR *)pu8Data, u32Len);
                szVersion[u32Len] = '\0';
            }
            else
            {
                s32Ret = SAL_FAIL;
            }
            break;
        }
        case MCU_CMD_GET_BUILD_TIME:
        {
            CHAR *szBuildTime = pvArg;
            if ((0 != u32Len) && (u32ArgLen > u32Len))
            {
                //strncpy(szBuildTime, (const CHAR *)pu8Data, u32Len);
                memcpy(szBuildTime, (const CHAR *)pu8Data, u32Len);
                szBuildTime[u32Len] = '\0';
            }
            else
            {
                s32Ret = SAL_FAIL;
            }
            break;
        }
        case MCU_CMD_GET_IN_RES:
        case MCU_CMD_GET_OUT_RES:
        {
            VIDEO_IO_STATUS_S *pstVideo = (VIDEO_IO_STATUS_S *)pvArg;
            memset(pstVideo, 0, sizeof(*pstVideo));
            pstVideo->enCable = pu8Data[i++];
            pstVideo->stRes.u32Width  |= (((UINT32)pu8Data[i++]) << 8);
            pstVideo->stRes.u32Width  |= ((UINT32)pu8Data[i++]);
            pstVideo->stRes.u32Height |= (((UINT32)pu8Data[i++]) << 8);
            pstVideo->stRes.u32Height |= ((UINT32)pu8Data[i++]);
            pstVideo->stRes.u32Fps     = pu8Data[i++];
            s32Ret = (i == u32Len) ? SAL_SOK : SAL_FAIL;
            break;
        }
        case MCU_CMD_GET_IN_TIMING:
        case MCU_CMD_GET_OUT_TIMING:
        {
            VIDEO_TIMING_S *pstTiming = (VIDEO_TIMING_S *)pvArg;
            memset(pstTiming, 0, sizeof(*pstTiming));
            pstTiming->enCable = pu8Data[i++];
            pstTiming->stTiming.u32Width |= (((UINT32)pu8Data[i++]) << 8);
            pstTiming->stTiming.u32Width |= ((UINT32)pu8Data[i++]);
            pstTiming->stTiming.u32HFrontPorch |= (((UINT32)pu8Data[i++]) << 8);
            pstTiming->stTiming.u32HFrontPorch |= ((UINT32)pu8Data[i++]);
            pstTiming->stTiming.u32HSync |= (((UINT32)pu8Data[i++]) << 8);
            pstTiming->stTiming.u32HSync |= ((UINT32)pu8Data[i++]);
            pstTiming->stTiming.u32HBackPorch |= (((UINT32)pu8Data[i++]) << 8);
            pstTiming->stTiming.u32HBackPorch |= ((UINT32)pu8Data[i++]);
            pstTiming->stTiming.enHSPol = (0 == pu8Data[i++]) ? POLARITY_NEG : POLARITY_POS;
            pstTiming->stTiming.u32Height |= (((UINT32)pu8Data[i++]) << 8);
            pstTiming->stTiming.u32Height |= ((UINT32)pu8Data[i++]);
            pstTiming->stTiming.u32VFrontPorch |= pu8Data[i++];
            pstTiming->stTiming.u32VSync |= pu8Data[i++];
            pstTiming->stTiming.u32VBackPorch |= pu8Data[i++];
            pstTiming->stTiming.enVSPol = (0 == pu8Data[i++]) ? POLARITY_NEG : POLARITY_POS;
            pstTiming->stTiming.u32Fps = pu8Data[i++];
            s32Ret = (i == u32Len) ? SAL_SOK : SAL_FAIL;
            break;
        }
        case MCU_CMD_GET_OUT_BIND:
            break;
        case MCU_CMD_GET_IN_BIND:
            break;
        case MCU_CMD_GET_CSC:
            break;
        case MCU_CMD_GET_EDID:
        {
            if ((u32Len > 1) && (0 == (u32Len -1) % 128) && ((u32Len - 1) <= 256))
            {
                VIDEO_EDID_BUFF_S *pstEdid = (VIDEO_EDID_BUFF_S *)pvArg;
                UINT8 *pu8Buff = pstEdid->au8Buff;
                pstEdid->enCable = pu8Data[i++];
                for (; i < u32Len; i++)
                {
                    *pu8Buff++ = pu8Data[i];
                }
                pstEdid->u32Len = u32Len - 1;
            }
            else
            {
                s32Ret = SAL_FAIL;
            }
            break;
        }
        case MCU_CMD_RST:
        case MCU_CMD_TIME_SYNC:
        case MCU_CMD_SET_IN_RES:
        case MCU_CMD_SET_OUT_RES:
        case MCU_CMD_SET_IN_TIMING:
        case MCU_CMD_SET_OUT_TIMING:
        case MCU_CMD_SET_BIND:
        case MCU_CMD_SET_IN_OFFSET:
        case MCU_CMD_SET_OUT_OFFSET:
        case MCU_CMD_HOTPLUG:
        case MCU_CMD_SET_CSC:
        case MCU_CMD_SET_EDID:
            s32Ret = ((1 == u32Len) && (pu8Data[0] == MCU_CMD_SUCCESS)) ? SAL_SOK : SAL_FAIL;
            break;
        default:
            MCU_LOGE("mcu unsupport cmd[0x%x]\n", u8Cmd);
            return SAL_FAIL;
    }

    return s32Ret;
}


/*******************************************************************************
* 函数名  : mcu_drvSendAndReceive
* 描  述  : 接收串口数据
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            UINT8 u8Cmd
            VOID *pvSendArg
            UINT32 u32SendLen
            VOID *pvRecArg
            UINT32 u32RecLen
            UINT32 u32Timeout
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvSendAndReceive(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, UINT8 u8Cmd, VOID *pvSendArg, UINT32 u32SendLen, VOID *pvRecArg, UINT32 u32RecLen, UINT32 u32Timeout)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    UINT8 au8Buff[MCU_UART_BUFF_SIZE];
    UINT8 *pu8Data = NULL;
    UINT32 u32Len = 0;

    SAL_mutexLock(g_aMutexHandle[u32Chn]);

    s32Ret = mcu_drvSendCmd(enId, enPeriph, u8Cmd, au8Buff, pvSendArg, u32SendLen);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%d] send cmd[0x%x] fail\n", u32Chn, u8Cmd);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }

    if (NULL == (pu8Data = mcu_drvWaitResponse(enId, u8Cmd, au8Buff, sizeof(au8Buff), u32Timeout, &u32Len)))
    {
        MCU_LOGE("mcu[%d] wait cmd[0x%x] response fail\n", u32Chn, u8Cmd);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }

    s32Ret = mcu_drvReceiveHandler(enPeriph, u8Cmd, pu8Data, u32Len, pvRecArg, u32RecLen);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%d] receive cmd[0x%x] fail\n", u32Chn, u8Cmd);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvSendData
* 描  述  : 发送数据
* 输  入  : UART_ID_E enId
            MCU_PERIPH_MOD_E enPeriph
            UINT8 u8Type
            UINT32 u32Offset
            const UINT8 *pu8Data
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_drvSendData(UART_ID_E enId, MCU_PERIPH_MOD_E enPeriph, UINT8 u8Type, UINT32 u32Offset, const UINT8 *pu8Data, UINT32 u32Len)
{
    UINT8 au8Head[16];
    UINT8 *pu8Buff = au8Head;
    UINT32 u32WriteBytes = 0;
    UINT32 u32TotalLen = u32Len + MCU_HEAD_LEN + MCU_CRC_LEN + 5;       /* 5=1个字节的数据类型+4个字节偏移 */
    UINT32 u32Tmp = 0;
    UINT32 u32Crc = 0;
    UINT8 u8Crc = 0;

    if (u32TotalLen > MCU_UART_BUFF_SIZE)
    {
        MCU_LOGE("mcu send data len[%u] is bigger than buff[%u]\n", u32TotalLen, MCU_UART_BUFF_SIZE);
        return SAL_FAIL;
    }

    *pu8Buff++ = MCU_SEND_START_CODE;
    *pu8Buff++ = (u32TotalLen >> 8) & 0xFF;
    *pu8Buff++ = u32TotalLen & 0xFF;
    *pu8Buff++ = MCU_PERIPH_MOD_MASTER;
    *pu8Buff++ = (UINT8)enPeriph;
    *pu8Buff++ = MCU_CMD_SEND_DATA;
    *pu8Buff++ = u8Type;
    *pu8Buff++ = (u32Offset >> 24) & 0xFF;
    *pu8Buff++ = (u32Offset >> 16) & 0xFF;
    *pu8Buff++ = (u32Offset >> 8) & 0xFF;
    *pu8Buff++ = u32Offset & 0xFF;

    u32Tmp = (UINT32)(pu8Buff - au8Head);
    u32Crc = mcu_drvSalCalCrc(u32Crc, au8Head, u32Tmp);
    if ((SAL_SOK != UART_Write(enId, au8Head, u32Tmp, &u32WriteBytes)) || (u32Tmp != u32WriteBytes))
    {
        MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, u32Tmp, u32WriteBytes);
        return SAL_FAIL;
    }

    /* 发送数据 */
    u32Crc = mcu_drvSalCalCrc(u32Crc, pu8Data, u32Len);
    if ((SAL_SOK != UART_Write(enId, pu8Data, u32Len, &u32WriteBytes)) || (u32Len != u32WriteBytes))
    {
        MCU_LOGE("uart[%d] send %u bytes fail, %u bytes send success\n", enId, u32Len, u32WriteBytes);
        return SAL_FAIL;
    }

    /* 发送校验码 */
    u8Crc = (UINT32)u32Crc;
    if ((SAL_SOK != UART_Write(enId, &u8Crc, 1, &u32WriteBytes)) || (1 != u32WriteBytes))
    {
        MCU_LOGE("uart[%d] send 1 bytes fail, %u bytes send success\n", enId, u32WriteBytes);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphReset
* 描  述  : MCU外设复位
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphReset(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_RST, NULL, 0, NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] reset fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvMcuHeartBeatStart
* 描  述  : 开始发送心跳
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMcuHeartBeatStart(UINT32 u32Chn)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_MCU_HEART_BEAT_START, NULL, 0, NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] set start beat heart fail\n", u32Chn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvMcuHeartBeatStop
* 描  述  : 停止发送心跳
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMcuHeartBeatStop(UINT32 u32Chn)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_MCU_HEART_BEAT_STOP, NULL, 0, NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] set stop beat heart fail\n", u32Chn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvMcuTimeSync
* 描  述  : mcu时间同步
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMcuTimeSync(UINT32 u32Chn, struct tm *time)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_TIME_SYNC, time, sizeof(*time), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] time sync fail\n", u32Chn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphGetVersion
* 描  述  : 获取MCU外设版本号
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            UINT8 *pu8Buff
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphGetVersion(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, UINT8 *pu8Buff, UINT32 u32Len)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_GET_VERSION, NULL, 0, (VOID *)pu8Buff, u32Len, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] get version fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphGetBuildTime
* 描  述  : 获取MCU外设软件编译时间
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            UINT8 *pu8Buff
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphGetBuildTime(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, UINT8 *pu8Buff, UINT32 u32Len)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_GET_BUILD_TIME, NULL, 0, (VOID *)pu8Buff, u32Len, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] get build time fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphSetInputRes
* 描  述  : 获取MCU外设的输入分辨率
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_IO_STATUS_S *pstStatus
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphSetInputRes(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_IO_STATUS_S *pstStatus)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_SET_IN_RES, (VOID *)pstStatus, sizeof(*pstStatus), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] set input resolution fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphSetOutputRes
* 描  述  : 获取MCU外设的输出分辨率
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_IO_STATUS_S *pstStatus
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphSetOutputRes(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_IO_STATUS_S *pstStatus)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_SET_OUT_RES, (VOID *)pstStatus, sizeof(*pstStatus), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] set output resolution fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphGetOutputRes
* 描  述  : 获取MCU外设输出分辨率
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_IO_STATUS_S *pstStatus
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphGetOutputRes(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_IO_STATUS_S *pstStatus)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_GET_OUT_RES, NULL, 0, (VOID *)pstStatus, sizeof(*pstStatus), 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] get output resolution fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphSetBind
* 描  述  : 设置输入和输出的绑定关系
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_CABLE_E enInCable
            VIDEO_CABLE_E enOutCable
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_drvPheriphSetBind(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_CABLE_E enInCable, VIDEO_CABLE_E enOutCable)
{
    VIDEO_CABLE_E aenCable[] = {enOutCable, enInCable};
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_SET_BIND, aenCable, sizeof(aenCable), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] get output resolution fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphVgaInputAutoAdjust
* 描  述  : VGA输入自动调整
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphVgaInputAutoAdjust(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph)
{
    VIDEO_OFFSET_E enOffset = VIDEO_OFFSET_AUTO;
    VIDEO_CABLE_PARAM_S stParam = {VIDEO_CABLE_VGA, (VOID *)&enOffset, sizeof(enOffset)};
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_SET_IN_OFFSET, &stParam, sizeof(stParam), NULL, 0, 20000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] vga auto adjust fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphHotPlug
* 描  述  : 拉hot plug引脚
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_CABLE_E enCable
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphHotPlug(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_CABLE_E enCable)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_HOTPLUG, &enCable, sizeof(enCable), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] hot plug fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphSetCsc
* 描  述  : 设置CSC参数
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_CSC_PARAM_S *pstCsc
            UINT32 u32Num
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphSetCsc(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_CSC_PARAM_S *pstCsc, UINT32 u32Num)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_SET_CSC, pstCsc, sizeof(VIDEO_CSC_PARAM_S) * u32Num, NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] set csc fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphSetEdid
* 描  述  : 设置EDID
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphSetEdid(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_EDID_BUFF_S *pstEdid)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_SET_EDID, pstEdid, sizeof(*pstEdid), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] set edid fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_drvPheriphGetEdid
* 描  述  : 获取EDID
* 输  入  : UINT32 u32Chn
            MCU_PERIPH_MOD_E enPeriph
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvPheriphGetEdid(UINT32 u32Chn, MCU_PERIPH_MOD_E enPeriph, VIDEO_EDID_BUFF_S *pstEdid)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, enPeriph, MCU_CMD_GET_EDID, &pstEdid->enCable, sizeof(pstEdid->enCable), pstEdid, sizeof(*pstEdid), 5000))
    {
        MCU_LOGE("mcu[%u] periph[0x%x] get edid fail\n", u32Chn, enPeriph);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvT1Test
* 描  述  : T1测试
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void mcu_drvT1Test(unsigned int u32Chn)
{
    INT32 i = 0;
    T1_GPIO_LEVEL_S stGpio;
    UINT32 u32Level = 0;
    UINT32 u32HwChn = 0;
    
    mcu_func_heartBeatStop(u32Chn);
    SAL_msleep(20);

    /*******************************************************************************************/
    /********************************************* mcu *****************************************/
    /*******************************************************************************************/

    /* 模式切换按键 */
    MCU_LOGW("press switch key in 5 seconds\n");
    for (i = 5; i >= 0; i--)
    {
        MCU_LOGW("%d seconds...\n", i);
        SAL_msleep(1000);
    }

    stGpio.u32Chn = T1_MCU_GPIO_SWITCH;
    stGpio.u32Level = 0;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_T1_GPIO_READ, &stGpio, sizeof(stGpio), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] switch key test press fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mcu[%u] switch key test press success\n", u32Chn);
    }
    SAL_msleep(20);

    MCU_LOGW("relese switch key in 5 seconds\n");
    for (i = 5; i >= 0; i--)
    {
        MCU_LOGW("%d seconds...\n", i);
        SAL_msleep(1000);
    }

    stGpio.u32Chn = T1_MCU_GPIO_SWITCH;
    stGpio.u32Level = 1;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_T1_GPIO_READ, &stGpio, sizeof(stGpio), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] switch key test relese fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mcu[%u] switch key test relese success\n", u32Chn);
    }
    SAL_msleep(20);

    /* 预留GPIO */
    stGpio.u32Chn = T1_MCU_GPIO_RES0;
    stGpio.u32Level = 0;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_T1_GPIO_WRITE, &stGpio, sizeof(stGpio), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] res0 gpio test fail\n", u32Chn);
    }
    else
    {
        if (SAL_SOK != mcu_drvReadPin(u32Chn, DDM_IOC_MCU_GPIO, &u32Level))
        {
            MCU_LOGE("mcu[%u] res0 gpio test fail\n", u32Chn);
        }
        else
        {
            if (u32Level != stGpio.u32Level)
            {
                MCU_LOGE("mcu[%u] res0 gpio set[%u] read[%u] not match\n", u32Chn, stGpio.u32Level, u32Level);
            }
            else
            {
                MCU_LOGI("mcu[%u] res0 gpio test level:%u success\n", u32Chn, stGpio.u32Level);
            }
        }
    }
    SAL_msleep(20);

    stGpio.u32Chn = T1_MCU_GPIO_RES0;
    stGpio.u32Level = 1;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_T1_GPIO_WRITE, &stGpio, sizeof(stGpio), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] res0 gpio test fail\n", u32Chn);
    }
    else
    {
        if (SAL_SOK != mcu_drvReadPin(u32Chn, DDM_IOC_MCU_GPIO, &u32Level))
        {
            MCU_LOGE("mcu[%u] res0 gpio test fail\n", u32Chn);
        }
        else
        {
            if (u32Level != stGpio.u32Level)
            {
                MCU_LOGE("mcu[%u] res0 gpio set[%u] read[%u] not match\n", u32Chn, stGpio.u32Level, u32Level);
            }
            else
            {
                MCU_LOGI("mcu[%u] res0 gpio test level:%u success\n", u32Chn, stGpio.u32Level);
            }
        }
    }
    SAL_msleep(20);

    /*******************************************************************************************/
    /********************************************* fpga ****************************************/
    /*******************************************************************************************/

    /* cfg done gpio */
    u32HwChn = T1_FPGA_GPIO_CFG_DONE;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_FPGA, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("fpga[%u] cfg done gpio test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("fpga[%u] cfg done gpio success\n", u32Chn);
    }
    SAL_msleep(20);

    /* cfg init b */
    u32HwChn = T1_FPGA_GPIO_CFG_INIT_B;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_FPGA, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("fpga[%u] cfg init b gpio test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("fpga[%u] cfg init b gpio success\n", u32Chn);
    }
    SAL_msleep(20);

    /* cfg prog b */
    u32HwChn = T1_FPGA_GPIO_CFG_PROG_B;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_FPGA, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("fpga[%u] cfg prog b gpio test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("fpga[%u] cfg prog b gpio success\n", u32Chn);
    }
    SAL_msleep(20);

    /* res0 gpio */
    u32HwChn = T1_FPGA_GPIO_RES0;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_FPGA, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("fpga[%u] res0 gpio test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("fpga[%u] res0 gpio success\n", u32Chn);
    }
    SAL_msleep(20);

    /* res1 gpio */
    u32HwChn = T1_FPGA_GPIO_RES1;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_FPGA, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("fpga[%u] res1 gpio test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("fpga[%u] res1 gpio success\n", u32Chn);
    }
    SAL_msleep(20);

    /* reset */
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_FPGA, MCU_CMD_T1_RESET, NULL, 0, NULL, 0, 5000))
    {
        MCU_LOGE("fpga[%u] reset test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("fpga[%u] reset test success\n", u32Chn);
    }
    SAL_msleep(20);

    /*******************************************************************************************/
    /********************************************* mstar ***************************************/
    /*******************************************************************************************/

    /* gpio55 */
    u32HwChn = T1_MSTAR_GPIO_55;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PRRIPH_MOD_MSTAR, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("mstar[%u] gpio55 test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mstar[%u] gpio55 test success\n", u32Chn);
    }
    SAL_msleep(20);

    /* gpio56 */
    u32HwChn = T1_MSTAR_GPIO_56;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PRRIPH_MOD_MSTAR, MCU_CMD_T1_GPIO, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("mstar[%u] gpio56 test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mstar[%u] gpio56 test success\n", u32Chn);
    }
    SAL_msleep(20);

    /* reset */
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PRRIPH_MOD_MSTAR, MCU_CMD_T1_RESET, NULL, 0, NULL, 0, 5000))
    {
        MCU_LOGE("mstar[%u] reset test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mstar[%u] reset test success\n", u32Chn);
    }
    SAL_msleep(20);

    /*******************************************************************************************/
    /******************************************** ch7053 ***************************************/
    /*******************************************************************************************/

    /* reset */
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_CH7053, MCU_CMD_T1_RESET, NULL, 0, NULL, 0, 5000))
    {
        MCU_LOGE("ch7053[%u] reset test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("ch7053[%u] reset test success\n", u32Chn);
    }
    SAL_msleep(20);

    /*******************************************************************************************/
    /********************************************* 显示器 **************************************/
    /*******************************************************************************************/

    /* vga edid iic */
    u32HwChn = VIDEO_CABLE_VGA;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MOINTOR, MCU_CMD_T1_IIC, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("mointor[%u] vga edid iic test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mointor[%u] vga edid iic test success\n", u32Chn);
    }
    SAL_msleep(20);

    /* hdmi edid iic */
    u32HwChn = VIDEO_CABLE_HDMI;
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MOINTOR, MCU_CMD_T1_IIC, &u32HwChn, sizeof(u32HwChn), NULL, 0, 5000))
    {
        MCU_LOGE("mointor[%u] hdmi edid iic test fail\n", u32Chn);
    }
    else
    {
        MCU_LOGI("mointor[%u] hdmi edid iic test success\n", u32Chn);
    }
    
    return;
}

/*******************************************************************************
* 函数名  : mcu_drvMstarGetEdid
* 描  述  : 获取EDID
* 输  入  : UINT32 u32Chn
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMstarGetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdid)
{
    return mcu_drvPheriphGetEdid(u32Chn, MCU_PRRIPH_MOD_MSTAR, pstEdid);
}


/*******************************************************************************
* 函数名  : mcu_drvMstarSetEdid
* 描  述  : 设置EDID
* 输  入  : UINT32 u32Chn
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvMstarSetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdid)
{
    return mcu_drvPheriphSetEdid(u32Chn, MCU_PRRIPH_MOD_MSTAR, pstEdid);
}


/*******************************************************************************
* 函数名  : mcu_drvMcuSetEdid
* 描  述  : 设置EDID
* 输  入  : UINT32 u32Chn
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mcuSetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdidBuff, CAPT_CABLE_E enCable)
{
    VIDEO_EDID_BUFF_S stEdid;
    memset(&stEdid, 0 ,sizeof(VIDEO_EDID_BUFF_S));

    sal_memcpy_s(stEdid.au8Buff, 256, pstEdidBuff->au8Buff, pstEdidBuff->u32Len);
    
    stEdid.enCable = enCable;
    stEdid.u32Len = pstEdidBuff->u32Len;
    return mcu_drvPheriphSetEdid(u32Chn, MCU_PRRIPH_MOD_MSTAR, &stEdid);
}


/*******************************************************************************
* 函数名  : mcu_drvMstarCheckEdid
* 描  述  : mstar升级的时候可能会把edid清掉，所以，初始化检查EDID
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
UINT32 mcu_func_mstarCheckEdid(UINT32 u32Chn, VIDEO_CABLE_E enCable, VIDEO_EDID_BUFF_S *pstEdidBuff)
{
    UINT32 i = 0;
    VIDEO_EDID_BUFF_S stEdid;
    VIDEO_EDID_BUFF_S stEdidInfo;
    UINT8 *pu8Buffer = NULL;

    stEdid.enCable = enCable;
    if ((SAL_SOK != mcu_drvMstarGetEdid(u32Chn, &stEdid)) || (0 == stEdid.u32Len))
    {
        MCU_LOGE("mstar chn[%u] get cable[%d] edid from rom fail\n", u32Chn, enCable);
        return SAL_FAIL;
    }

    if (NULL == pstEdidBuff)
    {
        MCU_LOGE("mstar chn[%u] get cable[%d] edid from file fail\n", u32Chn, enCable);
        return SAL_FAIL;
    }
    pu8Buffer = pstEdidBuff->au8Buff;
    /* 只校验一个block就可以 */
    for (i = 0; i < 128; i++)
    {
       
        if (stEdid.au8Buff[i] != *pu8Buffer)
        {
            sal_memcpy_s(stEdidInfo.au8Buff, 256, pstEdidBuff->au8Buff, pstEdidBuff->u32Len);
            stEdidInfo.enCable = enCable;
            stEdidInfo.u32Len = pstEdidBuff->u32Len;
            
            MCU_LOGW("mstar chn[%u] will update cable[%d] edid from file\n", u32Chn, enCable);
            if (SAL_SOK != mcu_drvMstarSetEdid(u32Chn, &stEdidInfo))
            {
                MCU_LOGE("mstar chn[%u] set cable[%d] edid fail\n", u32Chn, enCable);
                return SAL_FAIL;
            }
            return SAL_SOK;
        }
        pu8Buffer++;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvMstarGetBuildTime
* 描  述  : 获取mstar编译时间
* 输  入  : UINT32 u32Chn
            UINT8 *pu8Buff
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_drvMstarGetBuildTime(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32Len)
{
    strncpy((CHAR *)pu8Buff, (const CHAR *)g_au8MstarTime[u32Chn], u32Len);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_drvSetUart
* 描  述  : MCU设置串口
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static VOID mcu_drvSetUartId()
{
    UINT32 u32BoardType = HARDWARE_GetBoardType();
    UINT32 i = 0;
    
    if(UART_ID_BUTT != g_aenUartMap[0])
    {
        MCU_LOGI("mcu uart id has been seted\n");
        return;
    }
    
    for (; i < MCU_NUM_MAX; i++)
    {
        if(u32BoardType == DB_RS20016_V1_0 || u32BoardType == DB_RS20016_V1_1 || u32BoardType == DB_RS20016_V1_1_F303)
        {
            if(i == 0)
            {
                g_aenUartMap[i] = UART_ID_3; 
                g_aenLogUartMap[i] = UART_ID_5;
            }
            else
            {
                g_aenUartMap[i] = UART_ID_1;
                g_aenLogUartMap[i] = UART_ID_9;
            }
           
        }
        else if(u32BoardType == DB_TS3719_V1_0 || u32BoardType == DB_RS20046_V1_0)
        {
            if(i == 0)
            {
                g_aenUartMap[i] = UART_ID_0;
                g_aenLogUartMap[i] = UART_ID_4;
            }
            else
            {
                g_aenUartMap[i] = UART_ID_1;
                g_aenLogUartMap[i] = UART_ID_7;
            }
        }
        else
        {
            if(i == 0)
            {
                g_aenUartMap[i] = UART_ID_4; 
            }
            else
            {
                g_aenUartMap[i] = UART_ID_5;
            }
        }
    }
    
    return;
}

/*******************************************************************************
* 函数名  : mcu_drvCreatLog
* 描  述  : MCU创建日志
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static VOID mcu_drvCreatLog()
{
    CHAR comm[128] = {0};
    UART_ID_E aenLogUartId = 0;
    UINT32 i = 0;

    if(SAL_TRUE == g_bMcuLogCreated)
    {
        MCU_LOGI("mcu log has been created\n");
        return; 
    }

    g_bMcuLogCreated = SAL_TRUE;

    for (; i < MCU_NUM_MAX; i++)
    {
        aenLogUartId = g_aenLogUartMap[i];
        sprintf(comm,"/home/config/source/res/tools/log -d /dev/ttyS%u -b 115200 -f /home/config/log/mcu%u.log -t 1 &",aenLogUartId,i);
        system(comm);
        MCU_LOGI("comm:%s\n",comm);
    }

    return;
}

/*******************************************************************************
* 函数名  : mcu_drvInit
* 描  述  : MCU初始化
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 mcu_drvInit(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UART_ID_E enId = 0;
    UART_ATTR_S stUartAttr;
    time_t rawtime;
    struct tm *timeinfo;

    if (SAL_FALSE != g_abInited[u32Chn])
    {
        return SAL_SOK;
    }
    g_abInited[u32Chn] = SAL_TRUE;
    
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_aMutexHandle[u32Chn]);

    mcu_drvSetUartId();

    mcu_drvCreatLog();

    if (SAL_SOK != mcu_drvDriverOpen())
    {
        MCU_LOGE("mcu open DDM driver fail\n");
        return SAL_FAIL;
    }

    mcu_drvHeartBeatThreadCreate();

    stUartAttr.u32Baud       = MCU_UART_BAUD_DEFAULT;
    stUartAttr.u32DataBits   = 8;
    stUartAttr.enStopBits    = UART_STOP_BITS_1;
    stUartAttr.enPolarity    = UART_POLARITY_NONE;
    stUartAttr.enFlowControl = UART_FLOWCONTROL_NONE;
    stUartAttr.u32MinBytes   = MCU_UART_BUFF_SIZE;
    stUartAttr.u32TimeOut    = 500;

    enId = g_aenUartMap[u32Chn];
    s32Ret = UART_Init(enId, &stUartAttr);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu chn[%u] uart[%d] init fail\n", u32Chn, enId);
        return SAL_FAIL;
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (SAL_SOK != mcu_drvMcuTimeSync(u32Chn, timeinfo))
    {
        MCU_LOGE("mcu chn[%u] uart[%d] time sync fail\n", u32Chn, enId);
    }

    s32Ret = mcu_drvPheriphGetBuildTime(u32Chn, MCU_PERIPH_MOD_MCU, g_au8McuTime[u32Chn], sizeof(g_au8McuTime[u32Chn]));
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("mcu[%u] get build time fail\n", u32Chn);
        return SAL_FAIL;
    }
    MCU_LOGI("mcu[%u] get build time:%s\n", u32Chn, g_au8McuTime[u32Chn]);

    MCU_LOGI("mcu chn[%u] init success\n", u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_func_firmwareUpdate
* 描  述  : 固件升级
* 输  入  : UINT32 u32Chn
            const UINT8 *pu8Buff
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_firmwareUpdate(UINT32 u32Chn, const UINT8 *pu8Buff, UINT32 u32Len)
{
    UINT32 i;
    INT32  s32Ret = SAL_FAIL;
    UART_ID_E enId = g_aenUartMap[u32Chn];
    
    MCU_LOGI("mcu[%u] mcu_func_firmwareUpdate start\n", u32Chn);
    
    mcu_func_heartBeatStop(u32Chn);
    SAL_msleep(20);

    SAL_mutexLock(g_aMutexHandle[u32Chn]);
    
    memset(&g_stBoardInfo, 0, sizeof(g_stBoardInfo));
    
    s32Ret = UART_SetPolarity(enId, UART_POLARITY_EVEN);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] UART_SetPolarity fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }
    UART_Flush(enId);

    /* 防止进入boot模式失败, 尝试多次拉引脚进入boot模式 */
    for(i = 0; i < 5; i++)
    {
        s32Ret = mcu_drvUpdateReboot2Isp(u32Chn);
        if (SAL_SOK != s32Ret)
        {
            MCU_LOGI("mcu[%u] Times[%d] Boot fail\n", u32Chn, i);
        }
        else
        {
            break;
        }
    }
    
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] Reboot2Isp fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }
    
    s32Ret = mcu_drvUpdateCmdGet(u32Chn);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_drvUpdateCmdGet fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }
    
    s32Ret = mcu_drvUpdateGetChipID(u32Chn);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_drvUpdateGetChipID fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }
    
    s32Ret = mcu_drvUpdateErase(u32Chn);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_drvUpdateErase fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }
    
    s32Ret = mcu_drvUpdateWriteBin(u32Chn, pu8Buff, u32Len);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_drvUpdateWriteBin fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }

/* mcu升级校验,代码保留,后续debug调试时使用 */
#if 0
    // mcu升级bin文件校验
    s32Ret = mcu_drvUpdateVerify(u32Chn, pu8Buff, u32Len);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] MCU_UpdateVerify fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }
#endif

    s32Ret = mcu_drvUpdateResetRboot(u32Chn);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_updateReset fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }

    /* gd32f30x mcu 跳转指令使用有问题，规避为拉复位引脚重启mcu */
#if 0
    s32Ret = mcu_drvUpdateGo(u32Chn, MCU_UPDATE_FLASH_ADDR);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu[%u] mcu_drvUpdateGo fail\n", u32Chn);
        SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
        return SAL_FAIL;
    }

    s32Ret = mcu_drvSetBoot0Pin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGW("mcu[%u] set boot0 0 fail\n", u32Chn);
    }

    s32Ret = mcu_drvSetBoot1Pin(u32Chn, 0);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGW("mcu[%u] set boot1 0 fail\n", u32Chn);
    }
#endif

    s32Ret = UART_SetPolarity(enId, UART_POLARITY_NONE);
    if (SAL_SOK != s32Ret)
    {
        MCU_LOGW("mcu[%u] UART_SetPolarity fail\n", u32Chn);
    }

    SAL_mutexUnlock(g_aMutexHandle[u32Chn]);
    SAL_msleep(100);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_func_heartBeatStart
* 描  述  : 开始发送心跳信号
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_heartBeatStart(UINT32 u32Chn)
{
    if (u32Chn >= MCU_NUM_MAX)
    {
        MCU_LOGE("mcu invalid chn[%u]\n", u32Chn);
        return SAL_FAIL;
    }

    if (SAL_TRUE == g_stHeartBeat.abStarted[u32Chn])
    {
        MCU_LOGW("mcu[%u] beat heart has already started\n", u32Chn);
        return SAL_SOK;
    }

    g_stHeartBeat.abStarted[u32Chn] = SAL_TRUE;
    if (SAL_SOK != mcu_drvMcuHeartBeatStart(u32Chn))
    {
        MCU_LOGE("mcu[%u] send beat heart start cmd fail\n", u32Chn);
        g_stHeartBeat.abStarted[u32Chn] = SAL_FALSE;
        return SAL_FAIL;
    }

    MCU_LOGI("mcu[%u] beat heart start\n", u32Chn);
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_func_heartBeatStop
* 描  述  : 停止发送心跳信号
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_heartBeatStop(UINT32 u32Chn)
{
    if (u32Chn >= MCU_NUM_MAX)
    {
        MCU_LOGE("mcu invalid chn[%u]\n", u32Chn);
        return SAL_FAIL;
    }

    if (SAL_FALSE == g_stHeartBeat.abStarted[u32Chn])
    {
        MCU_LOGW("mcu[%u] beat heart has already stoped\n", u32Chn);
        return SAL_SOK;
    }

    g_stHeartBeat.abStarted[u32Chn] = SAL_FALSE;
    if (SAL_SOK != mcu_drvMcuHeartBeatStop(u32Chn))
    {
        MCU_LOGE("mcu[%u] send beat heart stop cmd fail\n", u32Chn);
        g_stHeartBeat.abStarted[u32Chn] = SAL_TRUE;
        return SAL_FAIL;
    }

    MCU_LOGI("mcu[%u] beat heart stop\n", u32Chn);
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_func_getBuildTime
* 描  述  : 获取MCU编译时间
* 输  入  : UINT32 u32Chn
            UINT8 *pu8Buff
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_getBuildTime(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32Len)
{
    strncpy((CHAR *)pu8Buff, (const CHAR *)g_au8McuTime[u32Chn], u32Len);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_func_mcuReset
* 描  述  : mcu复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mcuReset(UINT32 u32Chn)
{
    return mcu_drvPheriphReset(u32Chn, MCU_PERIPH_MOD_MCU);
}


/*******************************************************************************
* 函数名  : mcu_func_mcuSetMode
* 描  述  : 设置环通模式
* 输  入  : UINT32 u32Chn
            MCU_MODE_E enMode
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mcuSetMode(UINT32 u32Chn, MCU_MODE_E enMode)
{
    if (SAL_FAIL == mcu_drvSendAndReceive(u32Chn, MCU_PERIPH_MOD_MCU, MCU_CMD_MCU_SET_LOOP, &enMode, sizeof(enMode), NULL, 0, 5000))
    {
        MCU_LOGE("mcu[%u] set mode[%d] fail\n", u32Chn, enMode);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_func_fpgaInit
* 描  述  : fpga初始化
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_fpgaInit(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    
    s32Ret = mcu_drvInit(u32Chn);
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("mcu[%u] init fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mcu_drvPheriphGetBuildTime(u32Chn, MCU_PERIPH_MOD_FPGA, g_au8FpgaTime[u32Chn], sizeof(g_au8FpgaTime[u32Chn]));
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("fpga[%u] get build time fail\n", u32Chn);
        return SAL_FAIL;
    }
    MCU_LOGI("fpga[%u] get build time:%s\n", u32Chn, g_au8FpgaTime[u32Chn]);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_func_fpgaReset
* 描  述  : fpga复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_fpgaReset(UINT32 u32Chn)
{
    return mcu_drvPheriphReset(u32Chn, MCU_PERIPH_MOD_FPGA);
}


/*******************************************************************************
* 函数名  : mcu_func_fpgaSetInputResolution
* 描  述  : fpga设置输入分辨率
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_fpgaSetInputResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps)
{
    VIDEO_IO_STATUS_S stStatus = {VIDEO_CABLE_NONE, {u32Width, u32Height, u32Fps}};
    return mcu_drvPheriphSetInputRes(u32Chn, MCU_PERIPH_MOD_FPGA, &stStatus);
}


/*******************************************************************************
* 函数名  : mcu_func_fpgaSetOutputResolution
* 描  述  : fpga设置输出分辨率
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_fpgaSetOutputResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps)
{
    VIDEO_IO_STATUS_S stStatus = {VIDEO_CABLE_NONE, {u32Width, u32Height, u32Fps}};
    return mcu_drvPheriphSetOutputRes(u32Chn, MCU_PERIPH_MOD_FPGA, &stStatus);
}


/*******************************************************************************
* 函数名  : mcu_func_fpgaGetBuildTime
* 描  述  : 获取FPGA编译时间
* 输  入  : UINT32 u32Chn
            UINT8 *pu8Buff
            UINT32 u32Len
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_fpgaGetBuildTime(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32Len)
{
    strncpy((CHAR *)pu8Buff, (const CHAR *)g_au8FpgaTime[u32Chn], u32Len);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_func_ch7053Init
* 描  述  : CH7053初始化
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_ch7053Init(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    
    s32Ret = mcu_drvInit(u32Chn);
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("mcu[%u] init faill\n", u32Chn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_func_ch7053Reset
* 描  述  : ch7053复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_ch7053Reset(UINT32 u32Chn)
{
    return mcu_drvPheriphReset(u32Chn, MCU_PERIPH_MOD_CH7053);
}


/*******************************************************************************
* 函数名  : mcu_func_ch7053SetInputResolution
* 描  述  : Ch7053设置输入分辨率
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_ch7053SetInputResolution(UINT32 u32Chn, UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps)
{
    VIDEO_IO_STATUS_S stStatus = {VIDEO_CABLE_NONE, {u32Width, u32Height, u32Fps}};
    return mcu_drvPheriphSetInputRes(u32Chn, MCU_PERIPH_MOD_CH7053, &stStatus);
}


/*******************************************************************************
* 函数名  : mcu_func_mointorGetEdid
* 描  述  : 获取EDID
* 输  入  : UINT32 u32Chn
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mointorGetEdid(UINT32 u32Chn, VIDEO_EDID_BUFF_S *pstEdid)
{
    return mcu_drvPheriphGetEdid(u32Chn, MCU_PERIPH_MOD_MOINTOR, pstEdid);
}

/*******************************************************************************
* 函数名  : mcu_func_mstarReset
* 描  述  : 复位
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarReset(UINT32 u32Chn)
{
    return mcu_drvPheriphReset(u32Chn, MCU_PRRIPH_MOD_MSTAR);
}


/*******************************************************************************
* 函数名  : mcu_func_mstarHotplug
* 描  述  : 热插拔
* 输  入  : UINT32 u32Chn
            VIDEO_CABLE_E enCable
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarHotplug(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    VIDEO_CABLE_E enVdCable = enCable;

    return mcu_drvPheriphHotPlug(u32Chn, MCU_PRRIPH_MOD_MSTAR, enVdCable);
}


/*******************************************************************************
* 函数名  : mcu_func_mstarGetRes
* 描  述  : 获取mstar的分辨率
* 输  入  : UINT32 u32Chn
            VIDEO_IO_STATUS_S *pstStatus
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarGetRes(UINT32 u32Chn, CAPT_STATUS_S *pstStatus)
{
    INT32 s32Ret = SAL_SOK;
    VIDEO_IO_STATUS_S pstVdStatus;

    memset(&pstVdStatus, 0, sizeof(VIDEO_IO_STATUS_S));

    s32Ret = mcu_drvPheriphGetOutputRes(u32Chn, MCU_PRRIPH_MOD_MSTAR, &pstVdStatus);
    if(SAL_SOK != s32Ret)
    {
        MCU_LOGE("mcu_func_mstarGetRes chn %d \n", u32Chn);
        return SAL_FAIL;
    }

    pstStatus->enCable = pstVdStatus.enCable;
    if(pstVdStatus.stRes.u32Width > 0 && pstVdStatus.stRes.u32Height > 0)
    {
        pstStatus->bVideoDetected = SAL_TRUE;
        pstStatus->stRes.u32Fps = pstVdStatus.stRes.u32Fps;
        pstStatus->stRes.u32Width = pstVdStatus.stRes.u32Width;
        pstStatus->stRes.u32Height = pstVdStatus.stRes.u32Height;
    }
    else
    {
        pstStatus->bVideoDetected = SAL_FALSE;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : mcu_func_mstarSetCsc
* 描  述  : 配置CSC参数
* 输  入  : UINT32 u32Chn
            VIDEO_CSC_S *pstCsc
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarSetCsc(UINT32 u32Chn, VIDEO_CSC_S *pstCsc)
{
    VIDEO_CSC_PARAM_S astCscParam[] = {
        {VIDEO_CSC_LUMA,      pstCsc->uiLuma},
        {VIDEO_CSC_CONTRAST,  pstCsc->uiContrast},
        {VIDEO_CSC_HUE,       pstCsc->uiHue},
        {VIDEO_CSC_SATUATURE, pstCsc->uiSatuature},
    };

    return mcu_drvPheriphSetCsc(u32Chn, MCU_PRRIPH_MOD_MSTAR, astCscParam, sizeof(astCscParam)/sizeof(astCscParam[0]));
}


/*******************************************************************************
* 函数名  : mcu_func_mstarAutoAdjust
* 描  述  : 自动调整
* 输  入  : UINT32 u32Chn
            VIDEO_CABLE_E enCable
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarAutoAdjust(UINT32 u32Chn, CAPT_CABLE_E enCable)
{
    VIDEO_CABLE_E enVdCable = enCable;

    if (VIDEO_CABLE_VGA != enVdCable)
    {
        MCU_LOGE("mstar not support %d auto adjust except vga\n", enVdCable);
        return SAL_FAIL;
    }

    return mcu_drvPheriphVgaInputAutoAdjust(u32Chn, MCU_PRRIPH_MOD_MSTAR);
}


/*******************************************************************************
* 函数名  : mcu_func_mucGetEdid
* 描  述  : 获取EDID
* 输  入  : UINT32 u32Chn
            VIDEO_EDID_BUFF_S *pstEdid
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 mcu_func_mucGetEdid(UINT32 u32Chn, UINT8 *pu8Buff, UINT32 u32BuffLen, CAPT_CABLE_E enCable)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32Len = 0;
    VIDEO_EDID_BUFF_S pstEdid;
    
    s32Ret = mcu_drvPheriphGetEdid(u32Chn, MCU_PRRIPH_MOD_MSTAR, &pstEdid);
    if(SAL_SOK != s32Ret)
    {
        MCU_LOGE("mstar chn[%u] get cable[%d] edid from rom fail\n", u32Chn, enCable);
        return SAL_FAIL;
    }

    s32Len = u32BuffLen > pstEdid.u32Len ? pstEdid.u32Len:u32BuffLen;
    memcpy(pu8Buff, pstEdid.au8Buff, s32Len);

    return pstEdid.u32Len;
}





/*******************************************************************************
* 函数名  : mcu_func_mstarInit
* 描  述  : 初始化
* 输  入  : UINT32 u32Chn
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarInit(UINT32 u32Chn)
{
    INT32 s32Ret = SAL_SOK;
    UINT8 au8Buff[8];

    s32Ret = mcu_drvInit(u32Chn);
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("mcu[%u] init fail\n", u32Chn);
        return SAL_FAIL;
    }

    s32Ret = mcu_drvPheriphGetVersion(u32Chn, MCU_PRRIPH_MOD_MSTAR, au8Buff, sizeof(au8Buff));
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("mstar[%u] get version fail\n", u32Chn);
        return SAL_FAIL;
    }
    MCU_LOGI("mstar get version:%s\n", au8Buff);

    s32Ret = mcu_drvPheriphGetBuildTime(u32Chn, MCU_PRRIPH_MOD_MSTAR, g_au8MstarTime[u32Chn], sizeof(g_au8MstarTime[u32Chn]));
    if (SAL_SOK != s32Ret) {
        MCU_LOGE("mstar[%u] get build time fail\n", u32Chn);
        return SAL_FAIL;
    }



    MCU_LOGI("mstar get build time:%s\n", g_au8MstarTime[u32Chn]);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : mcu_func_mstarScale
* 描  述  : mstar输入分辨率大于面板分辨率时，会对输入分辨率进行缩放，计算缩放后的分辨率
* 输  入  : CAPT_RESOLUTION_S *pstSrc
* 输  出  : CAPT_RESOLUTION_S *pstDst
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 mcu_func_mstarScale(CAPT_RESOLUTION_S *pstDst, CAPT_RESOLUTION_S *pstSrc)
{
    UINT32 u32Num = sizeof(g_au32McuRatio)/sizeof(g_au32McuRatio[0]);
    UINT32 i = 0;

    if ((NULL == pstDst) || (NULL == pstSrc))
    {
        MSTAR_LOGE("invalid pointer:src[%p], dst[%p]\n", pstSrc, pstDst);
        return SAL_FAIL;
    }

    pstDst->u32Fps = pstSrc->u32Fps;

    if (pstSrc->u32Width > MCU_MSTAR_WIDTH_MAX || pstSrc->u32Height > MCU_MSTAR_HEIGHT_MAX)
    {
        for (i = 0; i < u32Num; i++)
        {
            /* 对比是否有支持的比例 */
            if (pstSrc->u32Width * g_au32McuRatio[i][1] == pstSrc->u32Height * g_au32McuRatio[i][0])
            {
                /* mstar输出的宽高比大于相应比例 */
                if (MCU_MSTAR_WIDTH_MAX * g_au32McuRatio[i][1] > MCU_MSTAR_HEIGHT_MAX * g_au32McuRatio[i][0])
                {
                    pstDst->u32Height = MCU_MSTAR_HEIGHT_MAX;
                    pstDst->u32Width  = pstDst->u32Height * g_au32McuRatio[i][0] / g_au32McuRatio[i][1];
                }
                else
                {
                    pstDst->u32Width  = MCU_MSTAR_WIDTH_MAX;
                    pstDst->u32Height = pstDst->u32Width * g_au32McuRatio[i][1] / g_au32McuRatio[i][0];
                }

                /* 四对齐，方便FPGA抠图 */
                pstDst->u32Width  &= 0xFFFFFFFC;
                pstDst->u32Height &= 0xFFFFFFFC;

                return SAL_SOK;
            }
        }

        pstDst->u32Width  = MCU_MSTAR_WIDTH_MAX;
        pstDst->u32Height = MCU_MSTAR_HEIGHT_MAX;

        return SAL_SOK;
    }

    pstDst->u32Width  = pstSrc->u32Width;
    pstDst->u32Height = pstSrc->u32Height;

    return SAL_SOK;
}



