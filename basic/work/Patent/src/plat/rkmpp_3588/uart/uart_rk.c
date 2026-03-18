
/*******************************************************************************
 * usrt.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2021年01月05日 Create
 *
 * Description : uart操作函数
 * Modification: 
 *******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <sal.h>
#include "platform_hal.h"

static const char *g_aszUartDev[] = {
    [UART_ID_0] = "/dev/ttyS0",
    [UART_ID_1] = "/dev/ttyS1",
    [UART_ID_2] = "/dev/ttyS2",
    [UART_ID_3] = "/dev/ttyS3",
    [UART_ID_4] = "/dev/ttyS4",
    [UART_ID_5] = "/dev/ttyS5",
    [UART_ID_6] = "/dev/ttyS6",
    [UART_ID_7] = "/dev/ttyS7",
    [UART_ID_8] = "/dev/ttyS8",
    [UART_ID_9] = "/dev/ttyS9"
};

static int g_aiFds[UART_ID_BUTT] = {0};
static UART_ATTR_S g_astUartAttr[UART_ID_BUTT];


/* 波特率定义 */
static const UINT32 g_au32BaudMap[][2] = 
{
    {0,         B0},
    {50,        B50},
    {75,        B75},
    {110,       B110},
    {134,       B134},
    {150,       B150},
    {200,       B200},
    {300,       B300},
    {600,       B600},
    {1200,      B1200},
    {1800,      B1800},
    {2400,      B2400},
    {4800,      B4800},
    {9600,      B9600},
    {19200,     B19200},
    {38400,     B38400},
    {57600,     B57600},
    {115200,    B115200},
    {230400,    B230400},
    {460800,    B460800},
    {500000,    B500000},
    {576000,    B576000},
    {921600,    B921600},
    {1000000,   B1000000},
    {1152000,   B1152000},
    {1500000,   B1500000},
    {2000000,   B2000000},
    {2500000,   B2500000},
    {3000000,   B3000000},
    {3500000,   B3500000},
    {4000000,   B4000000},
};


/*******************************************************************************
* 函数名  : UART_SetBaud
* 描  述  : 设置uart的波特率
* 输  入  : UART_ID_E enId
          UINT32 u32Baud
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetBaud(UART_ID_E enId, UINT32 u32Baud)
{
    UINT32 i = 0;
    UINT32 u32BaudNum = sizeof(g_au32BaudMap)/sizeof(g_au32BaudMap[0]);
    UINT32 u32TermBaud = 0;
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    for (i = 0; i < u32BaudNum; i++)
    {
        if (u32Baud == g_au32BaudMap[i][0])
        {
            u32TermBaud = g_au32BaudMap[i][1];
            break;
        }
    }

    if (i >= u32BaudNum)
    {
        UART_LOGE("uart[%d] invalid baud[%u]\n", enId, u32Baud);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    cfsetispeed(&stTermios, u32TermBaud);
    cfsetospeed(&stTermios, u32TermBaud);

    stTermios.c_cflag |= (CLOCAL | CREAD);

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    g_astUartAttr[enId].u32Baud = u32Baud;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_SetDataBits
* 描  述  : 设置uart的数据位数
* 输  入  : UART_ID_E enId
          UINT32 u32DataBits
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetDataBits(UART_ID_E enId, UINT32 u32DataBits)
{
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    stTermios.c_cflag &= ~CSIZE; 
    switch (u32DataBits)
    {   
        case 5:		
            stTermios.c_cflag |= CS5; 
            break;

        case 6:
            stTermios.c_cflag |= CS6;
            break;
        
        case 7:		
            stTermios.c_cflag |= CS7; 
            break;

        case 8:
            stTermios.c_cflag |= CS8;
            break;

        default:
            UART_LOGE("uart[%d] invalid data bits[%u]\n", enId, u32DataBits);
            return SAL_FAIL;
	}

    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    g_astUartAttr[enId].u32DataBits = u32DataBits;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_SetStopBits
* 描  述  : 设置uart的数据位数
* 输  入  : UART_ID_E enId
          UART_STOP_BITS_E enStopBits
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetStopBits(UART_ID_E enId, UART_STOP_BITS_E enStopBits)
{
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    switch (enStopBits)
    {   
        case UART_STOP_BITS_1:		  
            stTermios.c_cflag &= ~CSTOPB;  
            break;

        case UART_STOP_BITS_2:		  
            stTermios.c_cflag |= CSTOPB;  
            break;

        default:
            UART_LOGE("uart[%d] invalid stop bits[%d]\n", enId, enStopBits);
            return SAL_FAIL;
	} 

    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    g_astUartAttr[enId].enStopBits = enStopBits;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_SetStopBits
* 描  述  : 设置uart的校验位
* 输  入  : UART_ID_E enId
          UART_POLARITY_E enPolarity
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetPolarity(UART_ID_E enId, UART_POLARITY_E enPolarity)
{
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    switch (enPolarity)
    {   
        case UART_POLARITY_ODD:		  
            stTermios.c_cflag |= PARENB;
            stTermios.c_cflag |= PARODD;
            stTermios.c_iflag |= INPCK; 
            break;

        case UART_POLARITY_EVEN:		  
            stTermios.c_cflag |= PARENB;
            stTermios.c_cflag &= ~PARODD;
            stTermios.c_iflag |= INPCK; 
            break;

        case UART_POLARITY_NONE:
            stTermios.c_cflag &= ~PARENB;
            stTermios.c_iflag &= ~INPCK; 
            break;

        default:
            UART_LOGE("uart[%d] invalid stop bits[%d]\n", enId, enPolarity);
            return SAL_FAIL;
	} 

    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    g_astUartAttr[enId].enPolarity = enPolarity;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_SetFlowControl
* 描  述  : 设置uart的流控
* 输  入  : UART_ID_E enId
          UART_FLOWCONTROL_E enFlowControl
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetFlowControl(UART_ID_E enId, UART_FLOWCONTROL_E enFlowControl)
{
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    switch (enFlowControl)
    {   
        case UART_FLOWCONTROL_NONE:		  
            stTermios.c_iflag &= ~(IXON | IXOFF | IXANY);
            stTermios.c_cflag &= ~CRTSCTS;
            break;

        case UART_FLOWCONTROL_XON_XOFF:		  
            stTermios.c_iflag |= (IXON | IXOFF | IXANY);
            stTermios.c_cflag &= ~CRTSCTS;
            break;

        case UART_FLOWCONTROL_RTS_CTS:
            stTermios.c_iflag &= ~(IXON | IXOFF | IXANY);
            stTermios.c_cflag |= CRTSCTS;
            break;

        default:
            UART_LOGE("uart[%d] invalid flow control[%d]\n", enId, enFlowControl);
            return SAL_FAIL;
	}

    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    g_astUartAttr[enId].enFlowControl = enFlowControl;

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_SetRawMode
* 描  述  : uart设置为raw mode
* 输  入  : UART_ID_E enId
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetRawMode(UART_ID_E enId)
{
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s, fd:%d\n", enId, strerror(errno), fd);
        return SAL_FAIL;
    }

    
    stTermios.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR);
    stTermios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    stTermios.c_oflag &= ~OPOST;

    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s, fd:%d\n", enId, strerror(errno), fd);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_SetOverTime
* 描  述  : uart设置超时时间
* 输  入  : UART_ID_E enId
          UINT32 u32MinBytes : 接收的最小字节数
          UINT32 u32OverTime : 超时时间，ms
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_SetOverTime(UART_ID_E enId, UINT32 u32MinBytes, UINT32 u32OverTime)
{
    int fd = 0;
    struct termios stTermios;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    stTermios.c_cc[VMIN]  = u32MinBytes;
    stTermios.c_cc[VTIME] = (u32OverTime + 99) / 100;

    if (tcsetattr(fd, TCSANOW, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] set attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    if (tcgetattr(fd, &stTermios) < 0)
    {
        UART_LOGE("uart[%d] get termios attr fail:%s\n", enId, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_Init
* 描  述  : uart初始化
* 输  入  : UART_ID_E enId
          UART_ATTR_S *pstAttr
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_Init(UART_ID_E enId, UART_ATTR_S *pstAttr)
{
    INT32 s32Ret = SAL_SOK;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    if (g_aiFds[enId] > 0)
    {
        UART_LOGW("uart[%d] has already initialized\n", enId);
        return SAL_SOK;
    }

    if (NULL == g_aszUartDev[enId])
    {
        UART_LOGE("uart[%d] dev is null\n", enId);
        return SAL_FAIL;
    }

    g_aiFds[enId] = open(g_aszUartDev[enId], O_RDWR);
    if (g_aiFds[enId] <= 0)
    {
        UART_LOGE("uart[%d] open:%s fail, fd:%d\n", enId, g_aszUartDev[enId], g_aiFds[enId]);
        return SAL_FAIL;
    }
    
    /* DEBUG */
    UART_LOGI("uart[%d] open:%s success, fd:%d\n", enId, g_aszUartDev[enId], g_aiFds[enId]);

    s32Ret |= UART_SetRawMode(enId);
    s32Ret |= UART_SetBaud(enId, pstAttr->u32Baud);
    s32Ret |= UART_SetDataBits(enId, pstAttr->u32DataBits);
    s32Ret |= UART_SetStopBits(enId, pstAttr->enStopBits);
    s32Ret |= UART_SetPolarity(enId, pstAttr->enPolarity);
    s32Ret |= UART_SetFlowControl(enId, pstAttr->enFlowControl);
    s32Ret |= UART_SetOverTime(enId, pstAttr->u32MinBytes, pstAttr->u32TimeOut);

    if (SAL_SOK != s32Ret)
    {
        UART_LOGE("uart[%d] init fail\n", enId);
        return SAL_FAIL;
    }

    UART_LOGI("uart[%d] init success, baud:%u, data bits:%u, stop bits:%d, polarity:%d, flow control:%d\n",
              enId, pstAttr->u32Baud, pstAttr->u32DataBits, pstAttr->enStopBits, pstAttr->enPolarity, pstAttr->enFlowControl);
    
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_Flush
* 描  述  : 清空缓冲区
* 输  入  : UART_ID_E enId
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_Flush(UART_ID_E enId)
{
    int fd = 0;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    tcflush(fd, TCIOFLUSH);

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : UART_Read
* 描  述  : uart读数据
* 输  入  : UART_ID_E enId
          UINT8 *pu8Buff
          UINT32 u32BufLen
          UINT32 *pu32ReadBytes
          UINT32 u32TimeOut
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_Read(UART_ID_E enId, UINT8 *pu8Buff, UINT32 u32BufLen, UINT32 *pu32ReadBytes, UINT32 u32TimeOut)
{
    struct timeval stTimeOut;
    fd_set stFds;
    ssize_t readSize = 0;
    int fd = 0;
    int ret = -1;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    stTimeOut.tv_sec  = u32TimeOut / 1000;
    stTimeOut.tv_usec = (u32TimeOut % 1000) * 1000;

    FD_ZERO(&stFds);
    FD_SET(fd, &stFds);

    if ((ret = select(fd + 1, &stFds, NULL, NULL, &stTimeOut) <= 0))
    {
        UART_LOGE("uart[%d] fd[%d] read fail:%s\n", enId, fd, (ret < 0) ? strerror(errno) : "TIMEOUT");
        return SAL_FAIL;
    }

    if (FD_ISSET(fd, &stFds))
    {
        readSize = read(fd, (void *)pu8Buff, u32BufLen);
        *pu32ReadBytes = (UINT32)readSize;
    }
    else
    {
        UART_LOGE("uart[%d] fd[%d] not set\n", enId, fd);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : UART_Write
* 描  述  : uart写数据
* 输  入  : UART_ID_E enId
          UINT8 *pu8Buff
          UINT32 u32BufLen
          UINT32 *pu32WriteBytes
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 UART_Write(UART_ID_E enId, const UINT8 *pu8Buff, UINT32 u32BufLen, UINT32 *pu32WriteBytes)
{
    ssize_t writeSize = 0;
    int fd = 0;

    if (enId >= UART_ID_BUTT)
    {
        UART_LOGE("invalid uart id[%d]\n", enId);
        return SAL_FAIL;
    }

    fd = g_aiFds[enId];
    if (fd <= 0)
    {
        UART_LOGE("uart[%d] fd[%d] is invalid\n", enId, fd);
        return SAL_FAIL;
    }

    writeSize = write(fd, (const void *)pu8Buff, (size_t)u32BufLen);
    *pu32WriteBytes = (UINT32)writeSize;

    return SAL_SOK;
}



