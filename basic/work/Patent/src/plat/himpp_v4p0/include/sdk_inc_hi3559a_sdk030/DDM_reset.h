/**
 * @File: DDM_reset.h
 * @Module: DDM (Device Driver Model 设备驱动模型)
 * @Author: fushichen@hikvision.com.cn
 * @Created: 2016年12月12日
 *
 * @Description: 平台RESET模块(component)模型(内核层kernel) 与 应用层(user-land)接口
 *               功能包括 通过 DDM_IOC_RESET_TOTAL 对外设进行复位
 *
 * @Note : PCM 提供给内部模块、外部模块（如DDM）使用的公共头文件。修改需要评审
 * 
 * @Usage:操作步骤
 *
 *                  open /dev/DDMreset 打开驱动
 *                             |
 *               ioctl DDM_IOC_RESET_TOTAL 进行外设复位
 *                             |
 *                           close
 */

#ifndef INCLUDE_DDM_DDM_RESET_H_
#define INCLUDE_DDM_DDM_RESET_H_

/*********************************************************************
 ******************************* 头文件  *******************************
 *********************************************************************/
/**
 * 底层公共接口
 */
//#include "OSAL_api.h"
typedef signed char                           Int8;
typedef unsigned char                         Uint8;

typedef short int                             Int16;
typedef unsigned short int                    Uint16;

typedef int                                   Int32;
typedef unsigned int                          Uint32; 

/*********************************************************************
 ******************************* 结构体  *****************************
 *********************************************************************/
/**
 * reset 输入参数集
 */
typedef struct tagDDM_resetSet
{
    Uint8  type;         //!< type 对应的设备类型
    Uint8  subtype;      //!< subtype 对应的设备子类型
    Uint32 num;          //!< num 同类外设编号,从0开始计数 使用掩码标示通道
    Uint8  res[2];       //!< reserved
}DDM_resetSet;


/*********************************************************************
 ******************************* 宏枚举定义  ***************************
 *********************************************************************/
#define DDM_DEV_RESET_NAME        "reset"     //reset 设备名称
/**
 * reset 字符驱动魔数 (magic number)
 */
#define DDM_IOC_RESET_MNUM        'R'

/**
 *  外设复位
 */
#define DDM_IOC_RESET_OFFON       _IOR(DDM_IOC_RESET_MNUM, 0, DDM_resetSet)

/**
 *  外设复位住或不使能
 */
#define DDM_IOC_RESET_START       _IOW(DDM_IOC_RESET_MNUM, 1, DDM_resetSet)

/**
 *  外设停止复位或使能
 */
#define DDM_IOC_RESET_END         _IOW(DDM_IOC_RESET_MNUM, 2, DDM_resetSet)


/**
 * reset 外设子类选择   (0)
 * DDM_RESET_TYPE_KEYBOARD    keyboard外设二级分类
 */
typedef enum tagDDM_resetKBSubType
{
    DDM_KB_SUBTYPE_TOUCHKEY,       //!< DDM_KB_SUBTYPE_TOUCHKEY   按键芯片
    DDM_KB_SUBTYPE_BUTTON,         //!< DDM_KB_SUBTYPE_BUTTON     物理按键
    DDM_KB_SUBTYPE_MATRIXKEY,      //!< DDM_KB_SUBTYPE_MATRIXKEY  矩阵键盘
    DDM_KB_SUBTYPE_MAX,            //!< 最大值 必须为最后一个
}DDM_resetKBSubType;

/**
 * reset 外设子类选择   (1)
 * DDM_RESET_TYPE_TOUCHPAD    触摸屏外设二级分类
 */
typedef enum tagDDM_resetTPSubType
{
    DDM_TP_SUBTYPE_TOUCHPAD,       //!< DDM_TP_SUBTYPE_TOUCHPAD   触摸屏芯片
    DDM_TP_SUBTYPE_MAX,            //!< 最大值 必须为最后一个
}DDM_resetTPSubType;

/**
 * reset 外设子类选择   (2)
 * DDM_RESET_TYPE_DISPLAY    显示屏外设二级分类
 */
typedef enum tagDDM_resetDPSubType
{
    DDM_DP_SUBTYPE_DS,            //!< DDM_DP_SUBTYPE_DS        Display Screen,显示屏,如室内机的LCD屏
    DDM_DP_SUBTYPE_LVDS,          //!< DDM_DP_SUBTYPE_LVDS      LVDS转换芯片
    DDM_DP_SUBTYPE_eDP2MIPI,      //!< DDM_DP_SUBTYPE_eDP2MIPI  eDP2MIPI转换芯片
    DDM_DP_SUBTYPE_HDMI,          //!< DDM_DP_SUBTYPE_HDMI  HDMI转换芯片
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
    DDM_VIO_SUBTYPE_AD7842,
    DDM_VIO_SUBTYPE_MAX,            //!< 最大值 必须为最后一个
}DDM_resetVIOSubType;

/**
 * reset 外设子类选择   (4)
 * DDM_RESET_TYPE_AUDIO    音频模组外设二级分类
 */
typedef enum tagDDM_resetAIOSubType
{
    DDM_AIO_SUBTYPE_CODEC,        //!< DDM_AIO_SUBTYPE_CODEC    音频codec
    DDM_AIO_SUBTYPE_AEC,          //!< DDM_AIO_SUBTYPE_AEC      回声消除芯片
    DDM_AIO_SUBTYPE_MULTI,        //!< DDM_AIO_SUBTYPE_MULTI    音频codec和回声消除合一的芯片
    DDM_AIO_SUBTYPE_AMP,          //!< DDM_AIO_SUBTYPE_AMP      音频功放
    DDM_AIO_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetAIOSubType;

/**
 * reset 外设子类选择   (5)
 * DDM_RESET_TYPE_RF    近场射频模组外设二级分类
 */
typedef enum tagDDM_resetRFSubType
{
    DDM_RF_SUBTYPE_13D56M,        //!< DDM_RF_SUBTYPE_13D56M    13.56MHz射频芯片
    DDM_RF_SUBTYPE_125K,          //!< DDM_RF_SUBTYPE_125K      125KHz射频芯片
    DDM_RF_SUBTYPE_MAX,           //!< 最大值 必须为最后一个
}DDM_resetRFSubType;

/**
 * reset 外设子类选择   (6)
 * DDM_RESET_TYPE_WIRELESS    无线模组外设二级分类
 */
typedef enum tagDDM_resetWLSubType
{
    DDM_WL_SUBTYPE_WIFI,         //!< DDM_WL_SUBTYPE_WIFI       WIFI模组
    DDM_WL_SUBTYPE_BLUETOOTH,    //!< DDM_WL_SUBTYPE_BLUETOOTH  蓝牙模块
    DDM_WL_SUBTYPE_SUB1G,        //!< DDM_WL_SUBTYPE_SUB1G      SUB1G模块
    DDM_WL_SUBTYPE_ZIGBEE,       //!< DDM_WL_SUBTYPE_SUB1G      Zigbee模块
    DDM_WL_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetWLSubType;

/**
 * reset 外设子类选择   (7)
 * DDM_RESET_TYPE_MOBILECOMMU    移动通信外设二级分类
 * PS: 2G\3G\4G随均为USB转换,但是模组复位时间相差较大,所以需要细分.
 */
typedef enum tagDDM_resetMCSubType
{
    DDM_MC_SUBTYPE_GPRS,         //!< DDM_MC_SUBTYPE_GPRS    2G/3G/4G模块，现多为USB转
    DDM_MC_SUBTYPE_PHONE,        //!< DDM_MC_SUBTYPE_PHONE   电话模块
    DDM_MC_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetMCSubType;

/**
 * reset 外设子类选择   (8)
 * DDM_RESET_TYPE_GPS    定位模块外设二级分类
 */
typedef enum tagDDM_resetGPSSubType
{
    DDM_GPS_SUBTYPE_BDS,         //!< DDM_GPS_SUBTYPE_BDS      北斗定位模块
    DDM_GPS_SUBTYPE_GPS,         //!< DDM_GPS_SUBTYPE_GPS      GPS定位模块
    DDM_GPS_SUBTYPE_MULTI,       //!< DDM_GPS_SUBTYPE_MULTI    GPS和北斗合一的模块
    DDM_GPS_SUBTYPE_MAX,         //!< 最大值 必须为最后一个
}DDM_resetGPSSubType;

/**
 * reset 外设子类选择   (9)
 * DDM_RESET_TYPE_FINGER    指纹模组外设二级分类
 */
typedef enum tagDDM_resetFPSubType
{
    DDM_FP_SUBTYPE_FM,         //!< DDM_FP_SUBTYPE_FM    fingerprint Module 指纹模组
    DDM_FP_SUBTYPE_FH,         //!< DDM_FP_SUBTYPE_FH    Fingerpinrt Head 指纹头
    DDM_FP_SUBTYPE_MAX,        //!< 最大值 必须为最后一个
}DDM_resetFPSSubType;

/**
 * reset 外设子类选择   (10)
 * DDM_RESET_TYPE_SAFE    安全模块外设二级分类
 */
typedef enum tagDDM_resetSAFESubType
{
    DDM_SAFE_SUBTYPE_ENCRYPT,      //!< DDM_SAFE_SUBTYPE_ENCRYPT    加密模块
    DDM_SAFE_SUBTYPE_DECRYPT,      //!< DDM_SAFE_SUBTYPE_DECRYPT    解密模块
    DDM_SAFE_SUBTYPE_PSAM,         //!< DDM_SAFE_SUBTYPE_PSAM       PSAM认证模块
    DDM_SAFE_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetSAFESubType;

/**
 * reset 外设子类选择   (11)
 * DDM_RESET_TYPE_ETH     有线网模块外设二级分类
 */
typedef enum tagDDM_resetETHSubType
{
    DDM_ETH_SUBTYPE_MAC,          //!< DDM_ETH_SUBTYPE_MAC           外置MAC芯片
    DDM_ETH_SUBTYPE_PHY,          //!< DDM_ETH_SUBTYPE_PHY           外置PHY芯片
    DDM_ETH_SUBTYPE_SWITCH,       //!< DDM_ETH_SUBTYPE_SWITCH        switch芯片
    DDM_ETH_SUBTYPE_USB2ETH,      //!< DDM_ETH_SUBTYPE_USB2ETH       USB转ETH模块
    DDM_ETH_SUBTYPE_PCIE2ETH,     //!< DDM_ETH_SUBTYPE_PCIE2ETH      PCIe转ETH模块
    DDM_ETH_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetETHSubType;

/**
 * reset 外设子类选择   (12)
 * DDM_RESET_TYPE_PM     电源管理模块外设二级分类
 */
typedef enum tagDDM_resetPMSubType
{
    DDM_PM_SUBTYPE_PSE,          //!< DDM_PM_SUBTYPE_PSE          POE系统中的PSE供电芯片
    DDM_PM_SUBTYPE_PD,           //!< DDM_PM_SUBTYPE_PD           POE系统中的PD受电芯片
    DDM_PM_SUBTYPE_POE,          //!< DDM_PM_SUBTYPE_POE          POE系统芯片,包含PSE和PD
    DDM_PM_SUBTYPE_CHARGER,      //!< DDM_PM_SUBTYPE_CHARGER      充电管理芯片
    DDM_PM_SUBTYPE_ADC,          //!< DDM_PM_SUBTYPE_ADC          电量检测芯片,多为ADC芯片
    DDM_PM_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetPMSubType;

/**
 * reset 外设子类选择   (13)
 * DDM_RESET_TYPE_TM     时间管理模块外设二级分类
 */
typedef enum tagDDM_resetTMSubType
{
    DDM_TM_SUBTYPE_RTC,          //!< DDM_TM_SUBTYPE_RTC          外置RTC芯片
    DDM_TM_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetTMSubType;

/**
 * reset 外设子类选择   (14)
 * DDM_RESET_TYPE_ADC    A/D转换器外设二级分类
 */
typedef enum tagDDM_resetADCSubType
{
    DDM_ADC_SUBTYPE_AD,          //!< DDM_ADC_SUBTYPE_AD          外置AD芯片
    DDM_ADC_SUBTYPE_DA,          //!< DDM_ADC_SUBTYPE_DA          外置DA芯片
    DDM_ADC_SUBTYPE_MAX,         //!< 最大值 必须为最后一个
}DDM_resetADCSubType;

/**
 * reset 外设子类选择   (15)
 * DDM_RESET_TYPE_STORAGE    存储器外设二级分类
 */
typedef enum tagDDM_resetSTRSubType
{
    DDM_STR_SUBTYPE_EEPROM,      //!< DDM_STR_SUBTYPE_EEPROM       EEPROM存储芯片
    DDM_STR_SUBTYPE_MAX,         //!< 最大值 必须为最后一个
}DDM_resetSTRSubType;

/**
 * reset 外设子类选择   (16)
 * DDM_RESET_TYPE_SCHIP    特殊用途芯片外设二级分类
 */
typedef enum tagDDM_resetSCHIPSubType
{
    DDM_SCHIP_SUBTYPE_MCU,       //!< DDM_SCHIP_SUBTYPE_MCU       外置MCU芯片
    DDM_SCHIP_SUBTYPE_DSP,       //!< DDM_SCHIP_SUBTYPE_DSP       外置DSP芯片
    DDM_SCHIP_SUBTYPE_GPU,       //!< DDM_SCHIP_SUBTYPE_GPU       外置GPU
    DDM_SCHIP_SUBTYPE_MA2X50,       //!< DDM_SCHIP_SUBTYPE_MA2X50       外置MA2x50
    DDM_SCHIP_SUBTYPE_MAX,       //!< 最大值 必须为最后一个
}DDM_resetSCHIPSubType;

/**
 * reset 外设子类选择   (17)
 * DDM_RESET_TYPE_SENSOR    传感器外设二级分类
 */
typedef enum tagDDM_resetSENSORSubType
{
    DDM_SENSOR_SUBTYPE_PHOTOSENS,    //!< DDM_SENSOR_SUBTYPE_PHOTOSENS    光敏传感器
    DDM_SENSOR_SUBTYPE_DISTANCE,     //!< DDM_SENSOR_SUBTYPE_DISTANCE     距离传感器
    DDM_SENSOR_SUBTYPE_MAX,          //!< 最大值 必须为最后一个
}DDM_resetSENSORSubType;

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
    DDM_RESET_TYPE_MAX,          //!<?>  最大值 必须为最后一个
}DDM_resetDeviceType;

/**
 * reset 外设DDM选择
 */
typedef enum tagDDM_resetDevExtType
{
    DDM_NONE_RESET,         //!<0>  DDM_NONE_RESET         复位操作在DDM_reset中进行
    DDM_ADC_RESET,          //!<1>  DDM_ADC_RESET          复位操作在DDM_adc中进行
    DDM_ALARM_RESET,        //!<2>  DDM_ALARM_RESET        复位操作在DDM_alarm中进行
    DDM_KEYBOARD_RESET,     //!<3>  DDM_KEYBOARD_RESET     复位操作在DDM_keyboard中进行
    DDM_RTC_RESET,          //!<4>  DDM_RTC_RESET          复位操作在DDM_rtc中进行
    DDM_TOUCHPAD_RESET,     //!<5>  DDM_TOUCHPAD_RESET     复位操作在DDM_touchpad中进行
    DDM_SOUND_RESET,        //!<6>  DDM_SOUND_RESET        复位操作在DDM_sound中进行
    DDM_DISPLAY_RESET,      //!<7>  DDM_DISPLAY_RESET      复位操作在DDM_dp中进行
    DDM_RF13D56M_RESET,      //!<8>  DDM_RF13M56_RESET      复位操作在DDM_rf中进行
    DDM_MAX_RESET,          //!<?>  最大值 必须为最后一个
}DDM_resetDevExtType;


/*********************************************************************
 ******************************* 函数声明  ***************************
 *********************************************************************/

#endif /* INCLUDE_DDM_DDM_RESET_H_ */
