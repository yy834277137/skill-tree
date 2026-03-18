/*******************************************************************************************************************************
* 
* 版权信息：版权所有 (c) 2010, 杭州海康威视软件有限公司, 保留所有权利
* 
* 文件名称：audio_codec_common.h
* 摘    要：海康威视音频编解码共用结构体及宏定义声明文件

* 当前版本：0.9
* 作    者：陈扬坤
* 日    期：2011年9月25日
* 备    注：
*******************************************************************************************************************************/

#ifndef _AUDIO_CODEC_COMMON_H_
#define _AUDIO_CODEC_COMMON_H_

#ifdef _cplusplus
extern "C"{
#endif

#include "datatypedef.h"
#include "mem_tab.h"

#define			HIK_AUDIOCODEC_LIB_S_OK                        1              /* 成功		               */
#define			HIK_AUDIOCODEC_LIB_S_FAIL                      0              /* 失败		               */

#define			HIK_AUDIOCODEC_LIB_E_PARAM_NULL                0x80000000     /* 参数指针为空              */
#define			HIK_AUDIOCODEC_LIB_E_HANDLE_NULL			   0x80000001     /* 编码handle为空            */
#define			HIK_AUDIOCODEC_LIB_E_MEMORY_NULL    		   0x80000002     /* 内存为空                  */
#define         HIK_AUDIOCODEC_LIB_E_CHANNEL_ERR               0x80000003     /* 声道错误                  */
#define			HIK_AUDIOCODEC_LIB_E_SAMPLERATE_ERR			   0x80000004     /* 采样率错误                */
#define         HIK_AUDIOCODEC_LIB_E_MAXBITS_ERR			   0x80000005     /* 超过最大比特数            */
#define         HIK_AUDIOCODEC_LIB_E_BITSCTRL_ERR			   0x80000006     /* 码率控制级别错误          */
#define         HIK_AUDIOCODEC_LIB_E_BITRATE_ERR               0x80000007     /* 码率错误                  */
#define         HIK_AUDIOCODEC_LIB_E_INPUT_SIZE_ERR            0x80000008     /* 输入数据大小错误          */
#define         HIK_AUDIOCODEC_LIB_E_MEMTAB_SIZE_ERR           0x80000009     /* 内存表大小错误            */
#define         HIK_AUDIOCODEC_LIB_E_BUF_TOO_SMALL             0x8000000a     /* 缓存太小                  */

#define         AUDIO_CODEC_BUF_SIZE                           8192           /* 输入输出缓存大小          */
#define         AUDIO_DEC_DATA_SIZE                            2048           /* 解码解析参数需要数据大小  */

/* 码率控制级别 */
typedef enum _BITCTRLLEVEL_
{
    LEVEL1  = 1,
    LEVEL2  = 2,
    LEVEL3  = 3
}BITCTRLLEVEL;

/* 编码模式 */
typedef enum _ENC_MODE_
{ 
    AMR_MR475 = 0,		/* 4.75 kb/s */ 
    AMR_MR515,          /* 5.15 kb/s */  
    AMR_MR59,			/* 5.90 kb/s */
    AMR_MR67,			/* 6.70 kb/s */
    AMR_MR74,			/* 7.40 kb/s */
    AMR_MR795,			/* 7.95 kb/s */
    AMR_MR102,			/* 10.2 kb/s */
    AMR_MR122,          /* 12.2 kb/s */   

    AMR_MRDTX,			/* 静音帧    */
    AMR_N_MODES

}AUDIO_ENC_MODE;

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
    typedef S32 HRESULT;
#endif 

/*******************************************************************************************************************************
*编码结构体
*******************************************************************************************************************************/

typedef struct _AUDIOENCInfo                     /* 信息                                                    */
{    
    U32            in_frame_size;                /* 输入一帧数据大小(BYTES)，由GetInfoParam函数返回         */    
    S32            reserved[16];                 /* 保留                                                    */
} AUDIOENC_INFO;

typedef struct _AUDIOENC_PARAM_
{
    U32            sample_rate;                  /* 采样率-I                                                */
    U32            num_channels;                 /* 声道数-I                                                */    
    U32            bitrate;                      /* 码率  -I                                                */
    
    U32            vad_dtx_enable;               /* 0表示关闭,1表示开启, G7231, G729ab, AMR编码时配置       */ 
    BITCTRLLEVEL   bit_ctrl_level;               /* 码率控制级别，级别越低，码率控制越灵敏，AAC编码时配置   */    
    S32            reserved[16];                 /* 保留                                                    */
}AUDIOENC_PARAM;

typedef struct _AUDIOENC_PROCESS_PARAM_
{
    U08            *in_buf;                      /* 输入buf	                                                */
    U08            *out_buf;                     /* 输出buf                                                 */
    U32            out_frame_size;               /* 编码一帧后的BYTE数                                      */

    S32			   g726enc_reset;                /* 重置开关                                                */
    S32            g711_type;                    /* g711编码类型,0 - U law, 1- A law                        */
    AUDIO_ENC_MODE enc_mode;                     /* 音频编码模式，AMR编码配置                               */
    S32            reserved[16];                 /* 保留                                                    */
}AUDIOENC_PROCESS_PARAM;

/*******************************************************************************************************************************
*解码结构体
*******************************************************************************************************************************/

typedef struct _AUDIODECInfo                     /* 信息                                                    */
{
    S32            nchans;					     /* 声道数                                                  */
    S32            sample_rate;                  /* 采样率                                                  */
    S32            aacdec_profile;               /* 编码用的框架                                            */
    S32            reserved[16];                 /* 保留                                                    */
} AUDIODEC_INFO;

typedef struct _AUDIODEC_PARAM
{
    S32            gdec_bitrate;                 /* G系列解码需配置码率                                     */    
    S32            reserved[16];                 /* 保留                                                    */
}AUDIODEC_PARAM;

typedef struct _AUDIODEC_PROCESS_PARAM
{
    U08            *in_buf;                      /* 输入数据buf                                             */   
    U08			   *out_buf;                     /* 输出数据buf                                             */
    U32            in_data_size;                 /* 输入in_buf内数据byte数                                  */ 
    U32            proc_data_size;               /* 输出解码库处理in_buf中数据大小bytes                     */
    U32            out_frame_size;               /* 解码一帧后数据BYTE数                                    */
    AUDIODEC_INFO  dec_info;                     /* 输出解码信息                                            */
    
    S32			   g726dec_reset;                /* 重置开关                                                */ 
    S32            g711_type;                    /* g711编码类型,0 - U law, 1- A law                        */
    S32            reserved[16];                 /* 保留                                                    */
}AUDIODEC_PROCESS_PARAM;




#ifdef _cplusplus
}
#endif


#endif

