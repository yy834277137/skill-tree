/*******************************************************************************
 * hal_light.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : heshengyuan <heshengyuan@hikvision.com>
 * Version: V1.0.0  2018年11月24日 Create
 *
 * Description : 和硬件相关的闪光灯、补光灯、背光灯控制；允许项目存在差异，但是接口不变
 * Modification: 
 *******************************************************************************/

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include <sys/ioctl.h>

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define DDM_IOC_PWMLIGHT_MNUM           'G'
#define DDM_IOC_PWMLIGHT_SET_VAL        _IOW(DDM_IOC_PWMLIGHT_MNUM, 0, unsigned char)

/* 设置补光灯和红外灯亮度 */
#define DDM_IOC_PWMIR_MNUM              'N'
#define DDM_IOC_PWMIR_SET_VAL           _IOW(DDM_IOC_PWMIR_MNUM, 0, unsigned char)

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
static INT32 irFd  = -1;
static INT32 wlFd  = -1;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : HAL_flashlight
* 描  述  : 控制摄像头的闪光灯亮度
* 输  入  : - chan : 0 白光 1 红光
*         : - value: 亮度值
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 HAL_flashlight(UINT32 chan, UINT32 value)
{
    int ret   = 0;

    if(chan == 0)
    {
        /* 可见光测试设置 */
        if(wlFd < 0)
        {
            wlFd = open("/dev/DDM/pwmLight", O_RDWR);
            if(wlFd < 0)
            {
                return -1;
            }
                
        }

        ret = ioctl(wlFd, DDM_IOC_PWMLIGHT_SET_VAL, &value);
        if(ret < 0)
        {
            close(wlFd);
            return -1;
        }
        
        close(wlFd);
    }
    else
    {
        /* 红外光测试设置 */
        if(irFd < 0)
        {
            irFd = open("/dev/DDM/pwmIR", O_RDWR);            
            if(irFd < 0)
            {
                return -1;
            }
        }

        ret = ioctl(irFd, DDM_IOC_PWMIR_SET_VAL, &value);
        if(ret < 0)
        {
            close(irFd);
            return -1;
        }
        close(irFd);
    }
    
    return 0;
}




