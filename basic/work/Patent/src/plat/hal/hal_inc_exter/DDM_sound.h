/**
 * @File: DDM_ts.h
 * @Module: DDM (Device Driver Model 设备驱动模型)
 * @Author: fushichen@hikvision.com.cn
 * @Created: 2017年1月10日
 *
 * @Description: 平台RESET模块(component)模型(内核层kernel) 与 应用层(user-land)接口
 *               功能包括 通过 DDM_IOC_RESET_TOTAL 对外设进行复位
 *
 * @Note : PCM 提供给内部模块、外部模块（如DDM）使用的公共头文件。修改需要评审
 * 
 * @Usage:操作步骤
 *
 *                  open /dev/ts 打开驱动
 *                             |
 *               ioctl DDM_IOC_RESET_TOTAL 进行外设复位
 *                             |
 *                           close
 */

#ifndef INCLUDE_DDM_DDM_SOUND_H_
#define INCLUDE_DDM_DDM_SOUND_H_

/*********************************************************************
 ******************************* 头文件  *******************************
 *********************************************************************/
/**
 * 底层公共接口
 */
typedef short int                             Int16;
typedef unsigned short int                    Uint16;

typedef int                                   Int32;
typedef unsigned int                          Uint32;
/*********************************************************************
 ******************************* 宏枚举定义  ***************************
 *********************************************************************/ 

#define DDM_DEV_SOUND_NAME        "sound"     //touchscreen 设备名称

/**
 * sound字符驱动魔数 (magic number)
 */
#define DDM_IOC_INPUT_MNUM        'S'

/* 设置音频输入音量*/
#define DDM_IOC_SOUND_SET_ADC_VOLUME        _IOWR(DDM_IOC_INPUT_MNUM, 0, DDM_soundUsrCmd)

/* 获取音频输入音量*/
#define DDM_IOC_SOUND_GET_ADC_VOLUME        _IOWR(DDM_IOC_INPUT_MNUM, 1, DDM_soundUsrCmd)

/* 设置音频输出音量*/
#define DDM_IOC_SOUND_SET_DAC_VOLUME        _IOWR(DDM_IOC_INPUT_MNUM, 2, DDM_soundUsrCmd)

/* 获取音频输出音量*/
#define DDM_IOC_SOUND_GET_DAC_VOLUME        _IOWR(DDM_IOC_INPUT_MNUM, 3, DDM_soundUsrCmd)

/*音频输出静音*/
#define DDM_IOC_SOUND_MUTE                  _IOWR(DDM_IOC_INPUT_MNUM, 4, DDM_soundUsrCmd)

/* 设置音频codec芯片的采样参数，比特率和位宽*/
#define DDM_IOC_SOUND_SET_FORMAT            _IOWR(DDM_IOC_INPUT_MNUM, 5, DDM_soundUsrCmd)

/*获取音频codec芯片的采样参数，比特率和位宽*/
#define DDM_IOC_SOUND_GET_FORMAT            _IOWR(DDM_IOC_INPUT_MNUM, 6, DDM_soundUsrCmd)



/* 音频采样率 8K 到 192K，具体看各个codec芯片支持的范围 */
#define SND_SOC_PCM_RATE_8000                   (8000)
#define SND_SOC_PCM_RATE_11025                  (11025)
#define SND_SOC_PCM_RATE_16000                  (16000)
#define SND_SOC_PCM_RATE_22050                  (22050)
#define SND_SOC_PCM_RATE_32000                  (32000)
#define SND_SOC_PCM_RATE_44100                  (44100)
#define SND_SOC_PCM_RATE_48000                  (48000)
#define SND_SOC_PCM_RATE_96000                  (96000)
#define SND_SOC_PCM_RATE_192000                 (192000)

#define BIT(nr)			(1UL << (nr))

/* 采样音频数据的宽度 16bit 20bit 24bit 32bit 前提是芯片支持这种传输方式 */
typedef enum
{
    SND_SOC_PCM_FORMAT_S8                       = BIT(0),
    SND_SOC_PCM_FORMAT_S16                      = BIT(1),
    SND_SOC_PCM_FORMAT_S20                      = BIT(2),
    SND_SOC_PCM_FORMAT_S24                      = BIT(3),
    SND_SOC_PCM_FORMAT_S32                      = BIT(4),
}SND_PcmFormat;

typedef enum
{
    SND_SOC_DAC_ADC_NONE                        = BIT(0),
/* 输出增益设置，所有speaker输出增益、 LINE OUT 输出增益、headphone输出增益、各输出物理通道的左右增益控制 */
    SND_SOC_DAC_ALL                             = BIT(1),   // 0x2
    SND_SOC_DAC_LINE                            = BIT(2),   // 0x4
    SND_SOC_DAC_LINE_OUT_L                      = BIT(3),
    SND_SOC_DAC_LINE_OUT_R                      = BIT(4),
    SND_SOC_DAC_HP                              = BIT(5),   // 0x20
    SND_SOC_DAC_HP_OUT_L                        = BIT(6),
    SND_SOC_DAC_HP_OUT_R                        = BIT(7),

/* 输入增益设置，所有麦克输入增益、 左边麦克增益、右边麦克增益 */
    SND_SOC_ADC_ALL                             = BIT(16),  // 0x10000
    SND_SOC_ADC_L                               = BIT(17),
    SND_SOC_ADC_R                               = BIT(18),
    SND_SOC_ADC_IN1                             = BIT(19),
    SND_SOC_ADC_IN2                             = BIT(20),  // 0x100000
    SND_SOC_ADC_IN3                             = BIT(21),  // 0x200000
} SND_ChannelMask;

/*********************************************************************
 ******************************* 结构体  *****************************
 *********************************************************************/
typedef struct
{
    union
    {
        /* 芯片寄存器操作 */
        struct
        {
            Uint32    regAddr;
            Uint32    regVal;
        }readReg, writeReg;

        /* 音频采样参数设置和获取 */
        struct
        {
            Uint32    rate;   /* 8K=8000, 16K=16000, 44.1K=44100 依次类推 */
            Uint32    format; /* SND_PcmFormat 宏定义 */
        }rateFormat;

        /* 各个通道音频音频设置，通道采用掩码方式，支持一次设置多个通道的音频。 volume=0 是指0db值，不是静音 */
        struct
        {
            Uint32    channelMask;    /* 通道掩码，参见宏 SND_ChannelMask 的定义 */
            Uint32    volume;         /* 宏定义: SND_SOC_PCM_FORMAT_S16 SND_SOC_PCM_FORMAT_S20 等 */
        }dacVolume, adcVolume;

        /* 设置指定通道静音，MIC静音 Speaker静音，支持一次静音多个通道。 静音指直接关断该通道在codec的数据流向 */
        struct
        {
            Uint32    channelMask;    /* 通道掩码，参见宏 SND_ChannelMask 的定义 */
            Uint32    isMute;         /* 是否静音 1--静音  0--不静音 */
        }mute;

        /* 管理机对讲口控制，使用听筒还是免提 */
        struct
        {
            Uint32    isUsed;         /* 是否用听筒 1--使用听筒  0--使用免提 */
        }earpiece;

        struct
        {
            Uint32    boardType;         /* 动态初始化配置设备类型 */
        }init;

    }args;

    /* 预留参数 */
    Uint32 channel;
    Uint32 arg2;    
    Uint32 arg3;    
    Uint32 arg4;
    Uint32 arg5;
}DDM_soundUsrCmd;

/*********************************************************************
 ******************************* 函数声明  ***************************
 *********************************************************************/


#endif
