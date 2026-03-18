/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: IVS_HIK_inner.h
* 文件标识: HIKVISION_IVS_HIK_INNER_H
* 摘    要: 海康威视HIK文件封装内部声明文件
*
* 当前版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月13日
* 备    注:
*           
*******************************************************************************
*/

#ifndef _IVS_HIK_INNER_H_
#define _IVS_HIK_INNER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************
* 宏定义
**********************************************************************************/
	
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#define  CHECK_ERROR(state, error_code) \
	if(state)                           \
         {                              \
		     return error_code;         \
         }

#define START_CODE                  0x00000001
#define END_CODE                    0x00000002

#define CURRENT_VERSION             0x20040308

#define CONST_NUMBER_BASE           0x00001000

#define CIF_FORMAT                  0x00001001
#define QCIF_FORMAT                 0x00001002
#define FCIF_FORMAT                 0x00001003

//视频制式
#define FORMAT_PAL                  0x1001
#define FORMAT_NTSC                 0x1002

//音频通道
#define AUDIO_MONO                  0x1001
#define AUDIO_STEREO                0x1002 

//每个采样点位数；
#define BITSPERSAMP8	            8
#define BITSPERSAMP16	            16

//文件头标
#define HKM4_FILE_CODE		        0x484B4D34	//"HKM4" MPEG4视频、ADPCM音频，作者：王刚
#define HKH4_FILE_CODE		        0x484B4834	//"HKH4" H.264视频、G.722音频，作者：贾永华

//魔术数
#define MAGIC_NUMBER                0xD6D0B3FE

// 系统设置参数
// video
#define HIK_VIDEO_INTERLACE	        (1<<0)		// 4CIF时采用两场编码模式
#define HIK_STREAM_CRC		        (1<<1)		// GROUP_HEADER中的crc字段有效，对该Group中除GROUP_HEADER之外的数据做CRC校验
#define HIK_WATERMARK_VER1	        (1<<2)		// 包含水印信息(版本1,80字节)	
#define HIK_WATERMARK_RSA	        (1<<3)		// 使用512位的RSA算法对水印进行签名(签名后会增加64字节的校验数据)	

/*************************************************************************************
* 结构体定义
**************************************************************************************/

// Hikvision多媒体文件头信息. 
typedef struct HIKVISION_MEDIA_FILE_HEADER_STRU 
{
	unsigned int   start_code; 
	unsigned int   magic_number; 
	unsigned int   version; 
	unsigned int   check_sum; 
	unsigned short stream_type;
	unsigned short video_format; 
	unsigned short channels;
	unsigned short bits_per_sample; 
	unsigned int   samples_per_sec;
	
	union 
	{ 
		unsigned int picture_format;
		struct 
		{
			unsigned short image_width;
			unsigned short image_height; 
		}size;
	}image_size; 
	
	unsigned int  audio_type;
	unsigned int  system_param;
} HIKVISION_MEDIA_FILE_HEADER; 

// Hikvision组头数据结构
typedef struct GROUP_HEADER_STRU 
{
	unsigned int start_code; 
	unsigned int frame_num; 
	unsigned int time_stamp; 
	unsigned int is_audio; 
	unsigned int block_number;
	
	union
	{ 
		unsigned int picture_format;
		struct 
		{
			unsigned short image_width;
			unsigned short image_height; 
		}size;
	}image_size; 
	
	unsigned int picture_mode;
	unsigned int frame_rate;
	union
	{
		struct
		{
			unsigned int    I_quant_value;      
			unsigned int    P_quant_value;      
			unsigned int 	B_quant_value;      
		}mpeg4_quant;
		struct
		{
			unsigned short	time_extension;     
			unsigned short 	reserved[3];        
			unsigned int	stream_crc;         
		}H264_extension;                        
	}mpeg4_or_h264_info;                        
	
	unsigned int global_time;   

} GROUP_HEADER; 

// Hikvision块头数据结构
typedef struct _BLOCK_HEADER_
{
	unsigned short  type;               // 帧类型 
	unsigned short  version;            // 在I帧中加入的HIKVISION H.264 版本号，旧的版本不能解新版本的码率
	unsigned int    top_size;           // 仅使用第一个字节
	unsigned int    flags;              // H.264: 编码标记
	
	char            qp;                 // H.264: 量化系数
	char            LFIdc;              // 滤波方式：0: 进行jm20滤波, 1: 进行jm61e滤波, 7: 禁止滤波
	char            LFAlphaC0Offset;    // Alpha滤波调节参数(-6, 6)
	char            LFBetaOffset;       // Beta滤波调节参数(-6, 6)
	
	unsigned int    block_size;         // 帧字节数（帧头除外）
}BLOCK_HEADER;


#ifdef __cplusplus
}
#endif 

#endif




