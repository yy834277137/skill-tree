#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ExternalCodec.h"

/***********************************define value********************************/
#define SND_IOC_BASE                            'S'
#define SND_DEV_NAME                            "/dev/DDM/sound"


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

#ifndef BIT
#define BIT(x)                                  (1UL<<(x))
#endif

// codec
/* 设置音频输入音量*/
#define SND_IOC_SET_ADC_VOLUME                  _IOWR(SND_IOC_BASE, 0,  SND_UsrCmdArgs)

/* 设置音频输出音量*/
#define SND_IOC_SET_DAC_VOLUME                  _IOWR(SND_IOC_BASE, 2,  SND_UsrCmdArgs)

/* 设置音频采样参数*/
#define SND_IOC_SET_FORMAT                      _IOWR(SND_IOC_BASE, 3, SND_UsrCmdArgs)

/* 设置回环*/
#define SND_IOC_LOOPBACK                        _IOWR(SND_IOC_BASE, 12, SND_UsrCmdArgs)

/***********************************struct definination********************************/
typedef enum
{
    SND_SOC_DAC_ADC_NONE                        = BIT(0),
/* 输出增益设置，所有speaker输出增益、 LINE OUT 输出增益、headphone输出增益、各输出物理通道的左右增益控制 */
    SND_SOC_DAC_ALL                             = BIT(1),
    SND_SOC_DAC_LINE                            = BIT(2),
    SND_SOC_DAC_LINE_OUT_L                      = BIT(3),
    SND_SOC_DAC_LINE_OUT_R                      = BIT(4),
    SND_SOC_DAC_HP                              = BIT(5),
    SND_SOC_DAC_HP_OUT_L                        = BIT(6),
    SND_SOC_DAC_HP_OUT_R                        = BIT(7),

/* 输入增益设置，所有麦克输入增益、 左边麦克增益、右边麦克增益 */
    SND_SOC_ADC_ALL                             = BIT(16),
    SND_SOC_ADC_L                               = BIT(17),
    SND_SOC_ADC_R                               = BIT(18),
    SND_SOC_ADC_IN1                             = BIT(19),
    SND_SOC_ADC_IN2                             = BIT(20),
    SND_SOC_ADC_IN3                             = BIT(21),
    SND_SOC_ADC_INL_INR                         = BIT(22),
} SND_ChannelMask;

/* 采样音频数据的宽度 16bit 20bit 24bit 32bit 前提是芯片支持这种传输方式 */
typedef enum
{
    SND_SOC_PCM_FORMAT_S8                       = BIT(0),
    SND_SOC_PCM_FORMAT_S16                      = BIT(1),
    SND_SOC_PCM_FORMAT_S20                      = BIT(2),
    SND_SOC_PCM_FORMAT_S24                      = BIT(3),
    SND_SOC_PCM_FORMAT_S32                      = BIT(4),
}SND_PcmFormat;

typedef struct
{
    union
    {
        /* 芯片寄存器操作 */
        struct
        {
            UINT32    regAddr;
            UINT32    regVal;
        }readReg, writeReg;


        /* 音频采样参数设置和获取 */
        struct
        {
            UINT32    rate;   /* 8K=8000, 16K=16000, 44.1K=44100 依次类推 */
            UINT32    format; /* SND_PcmFormat 宏定义 */
        }rateFormat;

        /* 各个通道音频音频设置，通道采用掩码方式，支持一次设置多个通道的音频。 volume=0 是指0db值，不是静音 */
        struct
        {
            UINT32    channelMask;    /* 通道掩码，参见宏 SND_ChannelMask 的定义 */
            UINT32    volume;         /* 宏定义: SND_SOC_PCM_FORMAT_S16 SND_SOC_PCM_FORMAT_S20 等 */
        }dacVolume, adcVolume;

        /* 设置指定通道静音，MIC静音 Speaker静音，支持一次静音多个通道。 静音指直接关断该通道在codec的数据流向 */
        struct
        {
            UINT32    channelMask;    /* 通道掩码，参见宏 SND_ChannelMask 的定义 */
            UINT32    isMute;         /* 是否静音 1--静音  0--不静音 */
        }mute;

        /* 管理机对讲口控制，使用听筒还是免提 */
        struct
        {
            UINT32    isUsed;         /* 是否用听筒 1--使用听筒  0--使用免提 */
        }earpiece;

        struct
        {
            UINT32    boardType;      /* 动态初始化配置设备类型 */
        }init;

        struct
        {
            UINT32    isLoop;         /* 设置回环 */
        }loopback;
    }args;

    /* 预留参数 */
    UINT32 channel;
    UINT32 arg2;
    UINT32 arg3;
    UINT32 arg4;
    UINT32 arg5;
}SND_UsrCmdArgs;

typedef struct
{
    INT32                 drvFd;
    pthread_mutex_t     lock;
} SND_UserDrvObj;

/***********************************global value********************************/

static SND_UserDrvObj gUsrDrvObj =
{
    .drvFd = -1,
};

/***********************************function definition********************************/
/*******************************************************************************
* Function      : SND_userDrvInit
* Description   : 用户态驱动初始化
* Input         : NONE
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
INT32 SND_userDrvInit()
{
    INT32 Ret = SAL_SOK;

    if (gUsrDrvObj.drvFd > 0)
    {
        //prINTf("dev is opened\n", SND_DEV_NAME);
        return Ret;
    }

    printf("------ ExternalCodec  -------\n");
    gUsrDrvObj.drvFd = open(SND_DEV_NAME, O_SYNC | O_RDWR);
    if (gUsrDrvObj.drvFd <= 0)
    {
        printf("open %s failed, will creat\n", SND_DEV_NAME);
		Ret = SAL_FAIL;
    }

    pthread_mutex_init(&gUsrDrvObj.lock, NULL);

    return Ret;
}

/*******************************************************************************
* Function      : SND_userDrvDeinit
* Description   : 用户态驱动反初始化
* Input         : NONE
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
VOID SND_userDrvDeinit()
{
    if (gUsrDrvObj.drvFd > 0)
    {
        close(gUsrDrvObj.drvFd);
        gUsrDrvObj.drvFd = -1;
        pthread_mutex_destroy(&gUsrDrvObj.lock);
    }
}

/*******************************************************************************
* Function      : SND_userDrvIoctl
* Description   : 驱动命令Ioctl
* Input         : NONE
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
static INT32 SND_userDrvIoctl(UINT32 cmd, SND_UsrCmdArgs *pCmdArgs)
{
    INT32 ret = SAL_FAIL;

    pthread_mutex_lock(&gUsrDrvObj.lock);

    ret = ioctl(gUsrDrvObj.drvFd, cmd, pCmdArgs);
    if (ret < 0)
    {
        printf("sound ioctl failed %d,drvFd=%d!\n", ret, gUsrDrvObj.drvFd);
        pthread_mutex_unlock(&gUsrDrvObj.lock);
        return SAL_FAIL;
    }

    pthread_mutex_unlock(&gUsrDrvObj.lock);

    return SAL_SOK;
}

/*******************************************************************************
* Function      : SND_setRateFormat
* Description   : 设置音频采样参数，播放的音频文件采样参数更新时需要调用此接口重新配置底层codec参数。
                  该接口在音频采样进度改变时需要调用更新底层采样率。
* Input         : - rate  : 采样率 8K 16K 11.2K 44.1K等
*               : - format: 采样数据格式 SND_SOC_PCM_FORMAT_S16、SND_SOC_PCM_FORMAT_S20、SND_SOC_PCM_FORMAT_S24
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
INT32 SND_setRateFormat(UINT32 rate)
{
    SND_UsrCmdArgs cmdArgs;
    INT32 status;

    cmdArgs.args.rateFormat.rate    = rate;
    cmdArgs.args.dacVolume.volume   = 16;
    status =  SND_userDrvIoctl(SND_IOC_SET_FORMAT, &cmdArgs);
    return status;
}

/*******************************************************************************
* Function      : setAdcVolume
* Description   : 设置MIC输入的音量，每个音频输入通道音量都可以通过这个接口设置，
                  可以独立设置某通道音量或者一起设置多个通道的音量。因为MIC输入
                  增益对音频音质影响比较大，如果不是很必须请不要设置MIC输入增益。
                  该接口较少使用。
* Input         : - channelMask: 通道掩码，使用宏SND_ChannelMask定义的值
*               : - volume     : 音量格数
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
static INT32 setAdcVolume(UINT32 channelMask, UINT32 volume)
{
    SND_UsrCmdArgs cmdArgs;
    INT32 status;

    cmdArgs.args.adcVolume.channelMask = channelMask;
    cmdArgs.args.adcVolume.volume = volume;

    status = SND_userDrvIoctl(SND_IOC_SET_ADC_VOLUME, &cmdArgs);

    return status;
}

/*******************************************************************************
* Function      : setDacVolume
* Description   : 设置Speaker放音音量，每个音频播放的物理通道都可以通过这个接口设置，
                  可以独立设置某通道音量或者一起设置多个通道的音量。声音音量为db形式
                  为了使得不同的驱动对上层的操作都统一，这里对声音进行量化出0-10格。
                  该接口经常使用和设置。
* Input         : - channelMask: 通道掩码，使用宏SND_ChannelMask定义的值
*               : - volume     : 音量格数
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
static INT32 setDacVolume(UINT32 channelMask, UINT32 volume)
{
    SND_UsrCmdArgs cmdArgs;
    INT32 status;

    cmdArgs.args.dacVolume.channelMask = channelMask;
    cmdArgs.args.dacVolume.volume = volume;

    status =  SND_userDrvIoctl(SND_IOC_SET_DAC_VOLUME, &cmdArgs);

    return status;
}

/*******************************************************************************
* Function      : GetAdcChannelMask
* Description   : 根据传入的通道数进行通道掩码的转换
* Input         : - channel: 通道号码
* Output        : - ChannelMask : 通道掩码
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/

static SND_ChannelMask GetChannelMask(UINT32 channel)
{
    SND_ChannelMask ChannelMask = 0;

    if (channel > 0)
    {
        ChannelMask = (channel == 1) ? SND_SOC_ADC_R : SND_SOC_ADC_L;
    }
    else
    {
        ChannelMask = SND_SOC_ADC_ALL;
    }

    return ChannelMask;
}


INT32 SND_setAdcVolume(UINT32 channel, UINT32 volume)
{
    SND_ChannelMask ChannelMask = 0;
    INT32 status;

    ChannelMask = GetChannelMask(channel);
    status = setAdcVolume(ChannelMask, volume);

    return status;
}

INT32 SND_setDacVolume(UINT32 channel, UINT32 volume)
{
    SND_ChannelMask ChannelMask = 0;
    INT32 status;

    printf("---------------------------\n");
    ChannelMask = SND_SOC_DAC_ALL;
    status = setDacVolume(ChannelMask, volume);

    return status;
}

INT32 SND_setLoopBack(UINT32 isLoop)
{
    SND_UsrCmdArgs cmdArgs;
    INT32 status;

    cmdArgs.args.loopback.isLoop = isLoop;
    status =  SND_userDrvIoctl(SND_IOC_LOOPBACK, &cmdArgs);
 
    return status;
}

/*******************************************************************************
* Function      : SND_getRateFormat
* Description   : 获取底层当前配置的采样参数。该接口用于实时查询底层采样率
* Input         : - pRate  : 获取采样率
*               : - pFormat: 获取采样数据格式
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
INT32 SND_getRateFormat(UINT32 *pRate, UINT32 *pFormat)
{
    //SND_UsrCmdArgs cmdArgs;
    INT32 status = 0;

    //status = SND_userDrvIoctl(DDM_IOC_SOUND_GET_FORMAT, &cmdArgs);

    //*pRate   = cmdArgs.args.rateFormat.rate;
    //*pFormat = cmdArgs.args.rateFormat.format;

    return status;
}

/*******************************************************************************
* Function      : SND_setMute
* Description   : 设置 MIC、Speaker静音。某通道从静音到不静音，将直接恢复到该通道原有的音量分贝值
* Input         : - channelMask: 通道掩码，使用宏SND_ChannelMask定义的值
*               : - isMute     : 1   0
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
INT32 SND_setMute(UINT32 channelMask, UINT32 isMute)
{
    //SND_UsrCmdArgs cmdArgs;
    INT32 status = 0;

    //cmdArgs.args.mute.channelMask = channelMask;
    //cmdArgs.args.mute.isMute      = isMute;

    //status = SND_userDrvIoctl(DDM_IOC_SOUND_MUTE, &cmdArgs);

    return status;
}

/*******************************************************************************
* Function      : SND_setEarpiece
* Description   : 设置使用听筒、或者免提作为对讲口
* Input         : - isUsed: 1 使用听筒做对讲  0 使用免提做对讲
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
INT32 SND_setEarpiece(UINT32 isUsed)
{
    //SND_UsrCmdArgs cmdArgs;
    INT32 status = 0;

    //cmdArgs.args.earpiece.isUsed = isUsed;

    // status =  SND_userDrvIoctl(SND_IOC_EARPIECE, &cmdArgs);

    return status;
}


/*******************************************************************************
* Function      : SND_setCodecInit
* Description   : 应用程序动态初始化CODEC芯片，在线切换CODEC工作状态
* Input         : - boardType:
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
INT32 SND_setCodecInit(UINT32 boardType)
{
    //SND_UsrCmdArgs cmdArgs;
    INT32 status = 0;

    //cmdArgs.args.init.boardType         = boardType;

    // status =  SND_userDrvIoctl(SND_IOC_INIT, &cmdArgs);

    return status;
}



