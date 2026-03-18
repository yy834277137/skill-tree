/***
 * @file   sal_audio.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  audio common define
 * @author liwenbin
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_AUDIO_H_
#define _SAL_AUDIO_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* 采样位宽 */
typedef enum
{
    SAL_AUDIO_BIT_WIDTH_8 = 8,      /* 8bit width */
    SAL_AUDIO_BIT_WIDTH_16 = 16,    /* 16bit width*/
    SAL_AUDIO_BIT_WIDTH_24 = 24,    /* 24bit width*/
    SAL_AUDIO_BIT_WIDTH_BUTT,
} SAL_AduioBitWidth;

/* 音频采样率 */
typedef enum
{
    SAL_AUDIO_SAMPLE_RATE_8000 = 8000,      /* 8K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_12000 = 12000,    /* 12K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_11025 = 11025,    /* 11.025K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_16000 = 16000,    /* 16K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_22050 = 22050,    /* 22.050K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_24000 = 24000,    /* 24K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_32000 = 32000,    /* 32K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_44100 = 44100,    /* 44.1K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_48000 = 48000,    /* 48K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_64000 = 64000,    /* 64K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_96000 = 96000,    /* 96K samplerate*/
    SAL_AUDIO_SAMPLE_RATE_BUTT,
} SAL_AudioSampleRate;

/* 音频帧率 */
typedef enum
{
    SAL_AUDIO_FRAME_RATE_25 = 25,
    SAL_AUDIO_FRAME_RATE_BUTT,
} SAL_AudioFrameRate;

/* 音频单双声道 */
typedef enum
{
    SAL_AUDIO_SOUND_MODE_MONO = 1,  /*mono*/
    SAL_AUDIO_SOUND_MODE_STEREO = 2, /*stereo*/
    SAL_AUDIO_SOUND_MODE_BUTT
} SAL_AudioSoundMode;

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 音频帧的基础参数 */
typedef struct
{
    /* 音频的采样位宽， 具体见枚举 SAL_AduioBitWidth*/
    UINT32 bitWidth;
    /* 音频的采样率，   具体见枚举 SAL_AudioSampleRate*/
    UINT32 sampleRate;
    /* 音频的帧率，     具体见枚举 SAL_AudioFrameRate */
    UINT32 frameRate;
    /* 音频的声道模式， 具体见枚举 SAL_AudioSoundMode */
    UINT32 soundMode;

} SAL_AudioFrameParam;


typedef struct
{
    /* 音频帧的基础参数 */
    SAL_AudioFrameParam frameParam;

    /* 基础信息，内存地址，虚拟地址需要通过操作系统位宽来做区别，兼容64bit系统 */
    UINT32 is64BitSys;
    UINT32 poolId;
    PhysAddr virAddr[4];
    PhysAddr phyAddr[4];
    UINT32 stride[4];

    /* 音频帧数据的总长 */
    UINT32 bufLen;

    /* 音频帧的时间戳 */
    UINT64 pts;

    /* 音频帧的物理来源通道，例如多麦 */
    UINT32 chanId;

    /* 私有数据，可根据需要设置 */
    Handle privateDate;

    UINT32 reserved[8];

} SAL_AudioFrameBuf;

/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */


#endif



