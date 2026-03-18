
#ifndef __MCU_EXPAND_GPIO_H_
#define __MCU_EXPAND_GPIO_H_

/**
**说明：MCU1 和 MCU2 一共有8个gpio，
**      其中： RST、BOOT0、BOOT1 均为输出引脚（两片MCU 加起来有6个输出引脚），电平状态可以设置，但不能读
**             预留的GPIO为输入引脚（两片MCU 加起来有2个输入引脚），电平状态只能读，不能写                 
**
**/

/**
 * misc  (magic number)
 */
#define DDM_IOC_MISC_MNUM        'M'

#define DDM_MISC_DEVICE  "/dev/DDM/misc"

/**
 *  misc:MCU的RSTN 复位控制(PCA9534转gpio   ) DDM_miscIOCArgs.val:正常模式  (参数为1), 关闭电源(参数为0)
 */
#define DDM_IOC_MCU_RST   _IOW(DDM_IOC_MISC_MNUM, 34, DDM_miscIOCArgs)

/**
 *  misc:MCU的BOOT0控制(PCA9534转gpio   ) DDM_miscIOCArgs.val:正常模式  (参数为1), 关闭电源(参数为0)
 */
#define DDM_IOC_MCU_BOOT0  _IOW(DDM_IOC_MISC_MNUM, 35, DDM_miscIOCArgs)

/**
 *  misc:MCU的BOOT1控制(PCA9534转gpio   ) DDM_miscIOCArgs.val:正常模式  (参数为1), 关闭电源(参数为0)
 */
#define DDM_IOC_MCU_BOOT1  _IOW(DDM_IOC_MISC_MNUM, 36, DDM_miscIOCArgs)

/**
 *  misc:MCU的GPIO 电平状态获取(PCA9534转gpio   ) DDM_miscIOCArgs.val
 */
#define DDM_IOC_MCU_GPIO  _IOW(DDM_IOC_MISC_MNUM, 37, DDM_miscIOCArgs)

/********************************************************************************
 ********************************* 数据结构定义 *********************************
 ********************************************************************************/
typedef unsigned char Uint8;
 
/**
 * misc 通用控制 参数
 */
typedef struct tagDDM_miscIOCArgs
{
    Uint8 ch;  //!< ch 通道号
    Uint8 val; //!< val 值

    Uint8 rfu[6];           //!< rfu reserved for future use
}DDM_miscIOCArgs;

#endif
