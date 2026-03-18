/*******************************************************************************
 * iic.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2020年08月11日 Create
 *
 * Description : iic读写操作
 * Modification: 
 *******************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#include <linux/fb.h>
#include <sys/mman.h>
#include <memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sal.h"
#include "platform_hal.h"
#include "platform_sdk.h"


/* IIC总数只计算挂载在A53和A73上的，sensor hub上的IIC暂时未使用 */
#define IIC_NUM             (12)

/* 所有已打开的句柄保存在该数组中 */
static int fds[IIC_NUM] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

/* IIC读写加锁 */
static Handle g_astI2cMutexHandle[IIC_NUM] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


/*******************************************************************************
* 函数名  : IIC_Open
* 描  述  : 打开iic设备节点，非线程安全
* 输  入  : UINT32 u32IIC : iic号
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_Open(UINT32 u32IIC)
{
    int fd = -1;
    char file_name[20];

    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "/dev/i2c-%u", u32IIC);
    fd = open(file_name, O_RDWR);
    if (fd < 0)
    {
        IIC_LOGE("open %s error:%s\n", file_name, strerror(errno));
        return SAL_FAIL;
    }
    fds[u32IIC] = fd;

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_astI2cMutexHandle[u32IIC]);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : IIC_Close
* 描  述  : 关闭iic设备节点，非线程安全
* 输  入  : UINT32 u32IIC : iic号
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_Close(UINT32 u32IIC)
{
    if (fds[u32IIC] < 0)
    {
        return SAL_SOK;
    }

    close(fds[u32IIC]);
    fds[u32IIC] = -1;
    SAL_mutexDelete(g_astI2cMutexHandle[u32IIC]);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IIC_Read
* 描  述  : 读一个设备寄存器
* 输  入  : UINT32 u32IIC : IIC号
          UINT32 u32Dev : 设备地址
          UINT32 u32Reg : 寄存器地址
          UINT8 *pu8Data : 读到的数据
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_Read(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT8 *pu8Data)
{
    int ret = 0;
    int fd  = fds[u32IIC];
    unsigned char buff[4];
    struct i2c_msg msg[2];
    struct i2c_rdwr_ioctl_data rdwr;
    
    if (NULL == pu8Data)
    {
        IIC_LOGE("invalid data pointer\n");
        return SAL_FAIL;
    }

    if (fd < 0)
    {
        if (SAL_SOK != IIC_Open(u32IIC))
        {
            IIC_LOGE("iic[%u] open fail\n", u32IIC);
            return SAL_FAIL;
        }

        fd = fds[u32IIC];
    }

    u32Dev >>= 1;
    
    /* i2c读过程中加锁 */
    SAL_mutexLock(g_astI2cMutexHandle[u32IIC]);
    
    ret = ioctl(fd, I2C_SLAVE_FORCE, u32Dev);
    if (ret < 0)
    {
        IIC_LOGE("iic set slave[0x%x] error:%s\n", u32Dev, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    buff[0] = u32Reg & 0xFF;

    msg[0].addr   = u32Dev;
    msg[0].flags  = 0;
    msg[0].len    = 1;
    msg[0].buf    = buff;

    msg[1].addr   = u32Dev;
    msg[1].flags  = 0;
    msg[1].flags |= I2C_M_RD;
    msg[1].len    = 1;
    msg[1].buf    = buff;

    rdwr.msgs  = &msg[0];
    rdwr.nmsgs = (__u32)2;

    ret = ioctl(fd, I2C_RDWR, &rdwr);
    if (ret != 2)
    {
        IIC_LOGE("iic read dev[0x%x] reg[0x%x] error:%s\n", u32Dev, u32Reg, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    *pu8Data = buff[0];
    
    SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
    
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IIC_ReadArray
* 描  述  : 连续读多个设备寄存器
* 输  入  : UINT32 u32IIC : IIC号
          UINT32 u32Dev : 设备地址
          UINT32 u32RegNum : 读的寄存器个数
          UINT8 *pu8Reg : 寄存器地址
          UINT8 *pu8Data : 读到的数据
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_ReadArray(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32RegNum, const UINT8 *pu8Reg, UINT8 *pu8Data)
{
    int ret = 0;
    int fd  = fds[u32IIC];
    unsigned char buff[4];
    struct i2c_msg msg[2];
    struct i2c_rdwr_ioctl_data rdwr;
    UINT32 i = 0;
    
    if (NULL == pu8Data)
    {
        IIC_LOGE("invalid buff pointer\n");
        return SAL_FAIL;
    }

    if (fd < 0)
    {
        if (SAL_SOK != IIC_Open(u32IIC))
        {
            IIC_LOGE("iic[%u] open fail\n", u32IIC);
            return SAL_FAIL;
        }

        fd = fds[u32IIC];
    }

    u32Dev >>= 1;
    
    /* i2c读数组过程中加锁 */
    SAL_mutexLock(g_astI2cMutexHandle[u32IIC]);
    
    ret = ioctl(fd, I2C_SLAVE_FORCE, u32Dev);
    if (ret < 0)
    {
        IIC_LOGE("iic set slave[0x%x] error:%s\n", u32Dev, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    msg[0].addr  = u32Dev;
    msg[0].flags = 0;
    msg[0].len   = 1;
    msg[0].buf   = buff;

    msg[1].addr  = u32Dev;
    msg[1].flags = 0;
    msg[1].flags |= I2C_M_RD;
    msg[1].len   = 1;
    msg[1].buf   = buff;

    rdwr.msgs  = &msg[0];
    rdwr.nmsgs = (__u32)2;

    for (i = 0; i < u32RegNum; i++)
    {
        buff[0] = pu8Reg[i] & 0xFF;
    
        ret = ioctl(fd, I2C_RDWR, &rdwr);
        if (ret != 2)
        {
            IIC_LOGE("iic read dev[0x%x] reg[0x%x] error:%s\n", u32Dev, pu8Reg[i], strerror(errno));
            SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
            return SAL_FAIL;
        }

        pu8Data[i] = buff[0];
    }
    
    SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
    
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : IIC_ReadBytes
* 描  述  : 连续读多个设备寄存器
* 输  入  : u32IIC : iic号
* 输  出  : UINT32 u32IIC : IIC号
          UINT32 u32Dev : 设备地址
          UINT32 u32Reg : 寄存器起始地址
          UINT32 u32RegNum : 寄存器个数
          UINT32 u32Step : 寄存器地址的步长
          UINT8 *pu8Data : 读到的数据
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_ReadBytes(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT32 u32RegNum, UINT32 u32Step, UINT8 *pu8Data)
{
    int ret = 0;
    int fd  = fds[u32IIC];
    unsigned int cur_addr = u32Reg;
    unsigned char buff[4];
    struct i2c_msg msg[2];
    struct i2c_rdwr_ioctl_data rdwr;
    UINT32 i = 0;
    
    if (NULL == pu8Data)
    {
        IIC_LOGE("invalid buff pointer\n");
        return SAL_FAIL;
    }

    if (fd < 0)
    {
        if (SAL_SOK != IIC_Open(u32IIC))
        {
            IIC_LOGE("iic[%u] open fail\n", u32IIC);
            return SAL_FAIL;
        }

        fd = fds[u32IIC];
    }

    u32Dev >>= 1;
    
    /* i2c读过程中加锁 */
    SAL_mutexLock(g_astI2cMutexHandle[u32IIC]);
    
    ret = ioctl(fd, I2C_SLAVE_FORCE, u32Dev);
    if (ret < 0)
    {
        IIC_LOGE("iic set slave[0x%x] error:%s\n", u32Dev, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    msg[0].addr  = u32Dev;
    msg[0].flags = 0;
    msg[0].len   = 1;
    msg[0].buf   = buff;

    msg[1].addr  = u32Dev;
    msg[1].flags = 0;
    msg[1].flags |= I2C_M_RD;
    msg[1].len   = 1;
    msg[1].buf   = buff;

    rdwr.msgs  = &msg[0];
    rdwr.nmsgs = (__u32)2;

    for (i = 0; i < u32RegNum; i++)
    {
        buff[0] = cur_addr & 0xFF;
    
        ret = ioctl(fd, I2C_RDWR, &rdwr);
        if (ret != 2)
        {
            IIC_LOGE("iic read dev[0x%x] reg[0x%x] error:%s\n", u32Dev, cur_addr, strerror(errno));
            SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
            return SAL_FAIL;
        }

        cur_addr += u32Step;

        pu8Data[i] = buff[0];
    }
    
    SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
    
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : IIC_Write
* 描  述  : 写一个设备寄存器
* 输  入  : UINT32 u32IIC : IIC号
          UINT32 u32Dev : 设备地址
          UINT32 u32Reg : 寄存器地址
          UINT8 u8Data : 写的数据
* 输  出  : 
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_Write(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT8 u8Data)
{
    int ret = 0;
    int fd  = fds[u32IIC];
    unsigned char buff[4];
    
    u32Dev >>= 1;

    if (fd < 0)
    {
        if (SAL_SOK != IIC_Open(u32IIC))
        {
            IIC_LOGE("iic[%u] open fail\n", u32IIC);
            return SAL_FAIL;
        }

        fd = fds[u32IIC];
    }
    
    /* i2c写过程中加锁 */
    SAL_mutexLock(g_astI2cMutexHandle[u32IIC]);

    ret = ioctl(fd, I2C_SLAVE_FORCE, u32Dev);
    if (ret < 0)
    {
        IIC_LOGE("iic set slave[0x%x] error:%s\n", u32Dev, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    buff[0] = u32Reg & 0xFF;
    buff[1] = u8Data;

    ret = write(fd, buff, 2);
    if (ret < 0)
    {
        IIC_LOGE("iic write dev[0x%x] u32Reg[0x%x] fail:%s", u32Dev, u32Reg, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }
    
    SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
    
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : IIC_WriteBytes
* 描  述  : 连续写多个设备寄存器
* 输  入  : u32IIC : iic号
* 输  出  : UINT32 u32IIC : IIC号
          UINT32 u32Dev : 设备地址
          UINT32 u32Reg : 寄存器起始地址
          UINT32 u32RegNum : 寄存器个数
          UINT32 u32Step : 寄存器地址的步长
          UINT8 *pu8Data : 写的数据
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_WriteBytes(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32Reg, UINT32 u32RegNum, UINT32 u32Step, UINT8 *pu8Data)
{
    int ret = 0;
    int fd  = fds[u32IIC];
    unsigned int cur_addr = u32Reg;
    unsigned char buff[4];
    UINT32 i = 0;
    
    u32Dev >>= 1;

    if (NULL == pu8Data)
    {
        IIC_LOGE("invalid buff pointer\n");
        return SAL_FAIL;
    }

    if (fd < 0)
    {
        if (SAL_SOK != IIC_Open(u32IIC))
        {
            IIC_LOGE("iic[%u] open fail\n", u32IIC);
            return SAL_FAIL;
        }

        fd = fds[u32IIC];
    }
    
    /* i2c写过程中加锁 */
    SAL_mutexLock(g_astI2cMutexHandle[u32IIC]);
    
    ret = ioctl(fd, I2C_SLAVE_FORCE, u32Dev);
    if (ret < 0)
    {
        IIC_LOGE("iic set slave[0x%x] error:%s\n", u32Dev, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    for (i = 0; i < u32RegNum; i++)
    {
        buff[0] = cur_addr & 0xFF;
        buff[1] = pu8Data[i];

        ret = write(fd, buff, 2);
        if (ret < 0)
        {
            IIC_LOGE("iic write dev[0x%x] reg[0x%x] fail:%s", u32Dev, cur_addr, strerror(errno));
            SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
            return SAL_FAIL;
        }

        cur_addr += u32Step;
    }

    SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : IIC_WriteByteArray
* 描  述  : 连续写多个设备寄存器
* 输  入  : u32IIC : iic号
* 输  出  : UINT32 u32IIC : IIC号
          UINT32 u32Dev : 设备地址
          UINT32 u32RegNum : 读的寄存器个数
          const IIC_ADDR_MAP_S *pstMap : 寄存器数值
* 返回值  : SAL_SOK   : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 IIC_WriteArray(UINT32 u32IIC, UINT32 u32Dev, UINT32 u32RegNum, const IIC_ADDR_MAP_S *pstMap)
{
    int ret = 0;
    int fd  = fds[u32IIC];
    int i   = 0;
    unsigned char buff[4];

    u32Dev >>= 1;

    if (NULL == pstMap)
    {
        IIC_LOGE("invalid buff pointer\n");
        return SAL_FAIL;
    }

    if (fd < 0)
    {
        if (SAL_SOK != IIC_Open(u32IIC))
        {
            IIC_LOGE("iic[%u] open fail\n", u32IIC);
            return SAL_FAIL;
        }

        fd = fds[u32IIC];
    }
    
    /* i2c写数组过程中加锁 */
    SAL_mutexLock(g_astI2cMutexHandle[u32IIC]);

    ret = ioctl(fd, I2C_SLAVE_FORCE, u32Dev);
    if (ret < 0)
    {
        IIC_LOGE("iic set slave[0x%x] error:%s\n", u32Dev, strerror(errno));
        SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
        return SAL_FAIL;
    }

    for (i = 0; i < u32RegNum; i++, pstMap++)
    {
        buff[0] = ((pstMap->u8Addr) & 0xFF);
        buff[1] = (pstMap->u8Value);

        ret = write(fd, buff, 2);
        if (ret < 0)
        {
            IIC_LOGE("iic write dev[0x%x] reg[0x%x] fail:%s\n", u32Dev, pstMap->u8Addr, strerror(errno));
            SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
            return SAL_FAIL;
        }
    }
    
    SAL_mutexUnlock(g_astI2cMutexHandle[u32IIC]);
    
    return SAL_SOK;
}


