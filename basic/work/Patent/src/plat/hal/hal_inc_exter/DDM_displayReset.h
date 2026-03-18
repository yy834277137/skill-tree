#ifndef __DISPLAY_RESET_H_
#define __DISPLAY_RESET_H_

#ifdef __cplusplus
	#if __cplusplus
		extern "C"{
	#endif
#endif   /*__cplusplus*/

//#include "DDM_base.h"

/*设备节点名称*/
#define DDM_DEV_RESET_NAME        "/dev/DDM/reset" 
 
/* reset 字符驱动魔数 (magic number) */
#define DDM_IOC_RESET_MNUM        'R'

/* 外设复位的ioctl命令 */
#define DDM_IOC_RESET_OFFON       _IOR(DDM_IOC_RESET_MNUM, 0, DDM_resetSet)

/* 外设复位住或不使能 */
#define DDM_IOC_RESET_START       _IOW(DDM_IOC_RESET_MNUM, 1, DDM_resetSet)

/* 外设停止复位或使能 */
#define DDM_IOC_RESET_END         _IOW(DDM_IOC_RESET_MNUM, 2, DDM_resetSet)

/*********************************************************************
 ******************************* 枚举定义  *****************************
 *********************************************************************/

/**
 * reset 外设子类选择   (2)
 * DDM_RESET_TYPE_DISPLAY    显示屏外设二级分类
 */
typedef enum tagDDM_resetDPSubType
{
    DDM_DP_SUBTYPE_LT9611UXC,     //!< DDM_DP_SUBTYPE_LT9611UXC, LT9611UXC芯片: MIPI to HDMI(MIPI转HDMI输出)
    DDM_DP_SUBTYPE_LT8612SX,      //!< DDM_VIO_SUBTYPE_LT8612SX, LT8612sx芯片:HDMI to HDMI&VGA(HDMI转HDMI/VGA输出)
    DDM_DP_SUBTYPE_ICN6211,       //!< DDM_VIO_SUBTYPE_ICN6211,  ICN6211芯片: MIPI to RGB(MIPI转RGB输出)
    DDM_DP_SUBTYPE_CH7053,        //!< DDM_VIO_SUBTYPE_CH7053,   CH7053芯片: RGB to HDMI&VGA(RGB转HDMI输出)
    DDM_DP_SUBTYPE_LT9211,        //!< DDM_VIO_SUBTYPE_LT9211,   LT9211芯片: MIPI to LVDS(MIPI转LVDS输出)
    DDM_DP_SUBTYPE_LT8619,        //!< DDM_VIO_SUBTYPE_LT8619,   LT8619芯片: HDMI to LVDS(HDMI转LVDS输出)
    DDM_DP_SUBTYPE_LT9611,        //!< DDM_DP_SUBTYPE_LT9611,    LT9611芯片： MIPI to HDMI(MIPI转HDMI输出)
    DDM_DP_SUBTYPE_LT8912B,       //!< DDM_DP_SUBTYPE_LT8912B,   LT8912b芯片：MIPI to LVDS/HDMI(MIPI转LVDS/HDMI输出)
    DDM_DP_SUBTYPE_LT8612EX,      //!< DDM_DP_SUBTYPE_LT8612EX,  LT8612ex芯片：HDMI to HDMI/VGA(HDMI转HDMI/VGA输出)
    DDM_DP_SUBTYPE_LCD,           //!< DDM_DP_SUBTYPE_LCD, 显示屏复位
    DDM_DP_SUBTYPE_LT8618SX,      //!< DDM_DP_SUBTYPE_LT8618SX, LT8618芯片 :TTL转HDMI
    DDM_DP_SUBTYPE_LT86102,      //!< DDM_DP_SUBTYPE_LT86102, LT86102芯片 :HDMI一分为二

    DDM_DP_SUBTYPE_MAX,           //!< 最大值 必须为最后一个
}DDM_resetDPSubType;
    
/**
 * reset 外设子类选择   (3)
 * DDM_RESET_TYPE_VIDEO    视频模组外设二级分类
 */
typedef enum tagDDM_resetVIOSubType
{
    DDM_VIO_SUBTYPE_CAMMODULE,      //!< DDM_VIO_SUBTYPE_CAMMODULE   摄像头模组，如室内机使用的
    DDM_VIO_SUBTYPE_ISP,            //!< DDM_VIO_SUBTYPE_ISP         ISP芯片
    DDM_VIO_SUBTYPE_AD,             //!< DDM_VIO_SUBTYPE_AD          视频输入输出AD转换芯片
    DDM_VIO_SUBTYPE_DA,             //!< DDM_VIO_SUBTYPE_DA          视频输入输出DA转换芯片
    DDM_VIO_SUBTYPE_AD7842,        //!< DDM_VIO_SUBTYPE_AD7842   视频输入输出AD转换芯片AD7842
    DDM_VIO_SUBTYPE_LT8522EX,     //!< DDM_VIO_SUBTYPE_LT8522EX   视频输入输出AD转换芯片LT8522EX
    DDM_VIO_SUBTYPE_LT6911C,      //!< DDM_VIO_SUBTYPE_LT6911C   视频输入输出AD转换芯片LT6911C    
    
    DDM_VIO_SUBTYPE_MAX,            //!< 最大值 必须为最后一个
}DDM_resetVIOSubType;

/**
 * reset 外设选择
 * 部分设备难以确认属于哪种类型时需要讨论确定，保持统一
 */
typedef enum tagDDM_resetDeviceType
{
    DDM_RESET_TYPE_KEYBOARD,     //!<0>  DDM_RESET_TYPE_KEYBOARD     按键芯片,包括触摸按键,不排除以后物理按键和矩阵键盘会使用芯片
    DDM_RESET_TYPE_TOUCHPAD,     //!<1>  DDM_RESET_TYPE_TOUCHPAD     触摸屏
    DDM_RESET_TYPE_DISPLAY,      //!<2>  DDM_RESET_TYPE_DISPLAY      显示屏,包括LCD屏等
    DDM_RESET_TYPE_VIDEO,        //!<3>  DDM_RESET_TYPE_VIDEO        视频模组,包括摄像头模组/ISP芯片
    DDM_RESET_TYPE_AUDIO,        //!<4>  DDM_RESET_TYPE_AUDIO        音频模组,包括codec/回声消除等
    DDM_RESET_TYPE_RF,           //!<5>  DDM_RESET_TYPE_RF           近场射频模组,13.56MHz/125KHz等
    DDM_RESET_TYPE_WIRELESS,     //!<6>  DDM_RESET_TYPE_WIRELESS     无线模块,包括WIFI/蓝牙/sub1g
    DDM_RESET_TYPE_MOBILECOMMU,  //!<7>  DDM_RESET_TYPE_MOBILECOMMU  移动通信,2G/3G/4G等
    DDM_RESET_TYPE_GPS,          //!<8>  DDM_RESET_TYPE_GPS          定位模块,包括北斗/GPS
    DDM_RESET_TYPE_FINGER,       //!<9>  DDM_RESET_TYPE_FINGER       指纹模组
    DDM_RESET_TYPE_SAFE,         //!<10> DDM_RESET_TYPE_SAFE         安全模块,包括加密解密认证模块
    DDM_RESET_TYPE_ETH,          //!<11> DDM_RESET_TYPE_ETH          有线网模块,包括PHY/转换芯片等
    DDM_RESET_TYPE_PM,           //!<12> DDM_RESET_TYPE_PM           电源管理模块power manager,涉及POE/充电管理等
    DDM_RESET_TYPE_TM,           //!<13> DDM_RESET_TYPE_TM           时间管理模块,包括RTC
    DDM_RESET_TYPE_ADC,          //!<14> DDM_RESET_TYPE_ADC          A/D转换器
    DDM_RESET_TYPE_STORAGE,      //!<15> DDM_RESET_TYPE_STORAGE      存储器,包括EEPROM
    DDM_RESET_TYPE_SCHIP,        //!<16> DDM_RESET_TYPE_SCHIP        特殊用途芯片,包括外置MCU/DSP/FPGA/GPU/MA2x50等
    DDM_RESET_TYPE_SENSOR,       //!<17> DDM_RESET_TYPE_SENSOR       传感器,包括光敏/距离传感器等
	DDM_RESET_TYPE_HUB,			 //!<18> DDM_RESET_TYPE_HUB			 集线器, 当前有USB

	DDM_RESET_TYPE_MAX,          //!<?>  最大值 必须为最后一个
}DDM_resetDeviceType;

/*********************************************************************
 ******************************* 结构体  *****************************
 *********************************************************************/
/**
 * reset 输入参数集
 */
typedef struct tagDDM_resetSet
{
    UINT8  type;         //!< type 对应的设备类型，参考DDM_resetDeviceType枚举
    UINT8  subtype;      //!< subtype 对应的设备子类型，参考DDM_resetDPSubType枚举
    UINT32 num;          //!< num 同类外设编号,从0开始计数（和通道号的意思差不多）
    UINT8  res[2];       //!< reserved
}DDM_resetSet;

 
/**
 *显示模块复位使用示例, 仅供参考：
 #include <stdio.h>
 #include <sys/ioctl.h>
 #include <fcntl.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <string.h>
 
 int main()
 {
    int fd =  -1;
	int type = 0;
	int status = 0;
    DDM_resetSet  reset;
	
    fd = open(DDM_DEV_RESET_NAME, O_RDWR);
    if(fd < 0)
	{
		printf("open %s failed\n", DDM_DEV_RESET_NAME);
        return -1;
	}
	
    //LT9211复位示例
	memset((char *)&reset, 0, sizeof(DDM_resetSet));
	reset.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    reset.subtype = DDM_VIO_SUBTYPE_LT9211;   //二级子类型为LT9211显示模块
    reset.num = 0;                            //LT9211复位的通道号，从0开始
    
#if 0  //例如：其他视频芯片的复位示例
    //LT8619复位示例
	reset.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    reset.subtype = DDM_VIO_SUBTYPE_LT8619;   //二级子类型为LT8619显示模块
    reset.num = 0;                            //LT8619复位的通道号，从0开始
    
    //7053复位示例
	reset.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    reset.subtype = DDM_VIO_SUBTYPE_CH7053;   //二级子类型为CH7053显示模块
    reset.num = 0;                            //CH7053复位的通道号，从0开始
    
    //icn6211复位示例
	reset.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    reset.subtype = DDM_VIO_SUBTYPE_ICN6211;   //二级子类型为icn6211显示模块
    reset.num = 0;                             //icn6211复位的通道号，从0开始

    //LT8612复位示例
	reset.type = DDM_RESET_TYPE_DISPLAY;      //显示模块
    reset.subtype = DDM_VIO_SUBTYPE_LT8612;    //二级子类型为LT8612显示模块
    reset.num = 0;                            //LT8612复位的通道号，从0开始
#endif
	
    status = ioctl(fd, DDM_IOC_RESET_OFFON, &reset);  //使用ioctl接口复位
    if (status < 0)
        printf ("RESET ioctl reset failed fd %d %d!\n",fd, status);
    else
        printf("reset ok!\n");

    close(fd);
	return 0;
 }
*/
 
#ifdef __cplusplus
	#if __cplusplus
		}
	#endif
#endif   /*__cplusplus*/

#endif