/*
*******************************************************************************
* 
* 版权信息：版权所有 (c) 2008, 杭州海康威视数字技术有限公司, 保留所有权利
* 
* 文件名称：descriptor_set.c
* 文件标识：HKDSC
* 摘    要：海康威视生成通用描述子的代码
*
* 当前版本：0.5
* 作    者  辛安民
* 日    期：2015年06月23日
*
* 当前版本：0.4
* 作    者：PlaySDK
* 日    期：2013年11月13日
* 备	注：
* 2008.07.22	初始版本
* 2009.02.09	将原来stream_descriptor修改为basic_descriptor, 
*				另行增加stream_descriptor，主要为rtp应用传输原始流类型和帧号
* 2010.03.25    增加timing_hrd_desriptor的接口
* 2015.06.23    增加encrypt_descriptor的接口
********************************************************************************/

#include <string.h>
#include "descriptor_set.h"
#include "memory.h"

#define RESERVED_BYTE   0xff
/******************************************************************************
* 功  能：根据 encrypt_type 和 HIK_GLOBAL_TIME *glb_time 内的信息在 buffer 里生成 hik_basic_descriptor 数据
* 参  数：  buffer          - hik_basic_descriptor 数据缓冲区
*           stream_info     - 输入参数结构体
*           encrypt_type    - 加密模式
* 返回值：hik_basic_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_basic_descriptor(unsigned char *buffer, 
                                          HIK_GLOBAL_TIME *glb_time, 
                                          unsigned int encrypt_type,
										  unsigned int company_mark,
										  unsigned int camera_mark)
{
    buffer[0]	= HIK_BASIC_DESCRIPTOR_TAG; //  descriptor_tag: 0x40
    buffer[1]	= HIK_BASIC_DESCRIPTOR_LEN - 2; //  descriptor_length;  
    buffer[2]	= (unsigned char)(company_mark >> 8); //  company_mark_h;     公司描述符，0x48, "H"
    buffer[3]	= (unsigned char)company_mark; //  company_mark_l;     公司描述符，0x4b, "K" 		
    buffer[4]	= (HIK_DESCRIPTOR_DEF_VERSION >> 8);//  def_version_h;      描述符定义版本，第一版为0x00
    buffer[5]	= HIK_DESCRIPTOR_DEF_VERSION;       //  def_version_l;      描述符定义版本，第一版为0x01
    buffer[6]	= (unsigned char)((glb_time->year - DESCRIPTOR_YEAR_BASE) & 0xff);    //  全局时间年 8bit，1表示2001年，后面类推，不得出现0
    buffer[7]	= ((unsigned char)(glb_time->month << 4) & 0xf0)      //  月4bit
                | ((unsigned char)(glb_time->date >> 1) & 0x0f);      //  日5bit
    buffer[8]	= ((unsigned char)(glb_time->date << 7) & 0x80)
                | ((unsigned char)(glb_time->hour << 2) & 0x7c)       //  时5bit
                | ((unsigned char)(glb_time->minute >> 4) & 0x03);    //  分6bit
    buffer[9]	= ((unsigned char)(glb_time->minute << 4) & 0xf0)
                | ((unsigned char)(glb_time->second >> 2) & 0x0f);    //  秒6bit
    buffer[10]	= ((unsigned char)(glb_time->second << 6) & 0xc0)     
                | (1 << 5)                                              //  插入1bit
                | ((unsigned char)(glb_time->msecond >> 5) & 0x1f);   //  毫秒10bit
    buffer[11]	= ((unsigned char)(glb_time->msecond << 3) & 0xf8)
                | ((unsigned char)(encrypt_type) & 0x07);  //  加密类型 3 bit
    buffer[12]	= (unsigned char)camera_mark;
    buffer[13]	= RESERVED_BYTE;        
    buffer[14]	= RESERVED_BYTE;
    buffer[15]	= RESERVED_BYTE;
    return HIK_BASIC_DESCRIPTOR_LEN;
}

/******************************************************************************
* 功  能：根据 stream_info 内的信息在 buffer 里生成 hik_stream_descriptor 数据
* 参  数：  buffer          - hik_stream_descriptor 数据缓冲区
*           stream_info     - 输入参数结构体
* 返回值：hik_stream_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_stream_descriptor(unsigned char *buffer, 
										  unsigned int video_stream_type,
										  unsigned int audio_stream_type,
										  unsigned int video_frame_num)
{
    buffer[0]	= HIK_STREAM_DESCRIPTOR_TAG;			//  descriptor_tag: 0x45
    buffer[1]	= HIK_STREAM_DESCRIPTOR_LEN - 2;		//  descriptor_length;  
    buffer[2]	= video_stream_type;	 
    buffer[3]	= audio_stream_type;	 
	if (video_stream_type == STREAM_TYPE_UNDEF)
	{
		video_frame_num = 0;
	}
    
	buffer[4]	= (unsigned char)(video_frame_num >> 24);
	buffer[5]	= (unsigned char)(video_frame_num >> 16);
	buffer[6]	= (unsigned char)(video_frame_num >> 8);
	buffer[7]	= (unsigned char)(video_frame_num);

    buffer[8]	= RESERVED_BYTE;
    buffer[9]	= RESERVED_BYTE;        
    buffer[10]	= RESERVED_BYTE;
    buffer[11]	= RESERVED_BYTE;
    return HIK_STREAM_DESCRIPTOR_LEN;
}

/******************************************************************************
* 功  能：根据 HIK_VIDEO_INFO *video_info 内的信息在 buffer 里生成 hik_video_descriptor 数据
* 参  数：  buffer          - hik_video_descriptor 数据缓冲区
*           video_info      - 输入参数结构体
* 返回值：hik_video_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_video_descriptor(unsigned char *buffer, HIK_VIDEO_INFO *video_info)
{
    buffer[0]	= HIK_VIDEO_DESCRIPTOR_TAG;     //  descriptor_tag: 0x42
    buffer[1]	= HIK_VIDEO_DESCRIPTOR_LEN - 2; //  descriptor_length;  

    //encoder info
    buffer[2]	= (unsigned char)(video_info->encoder_version >> 8);   //编码器版本
    buffer[3]	= (unsigned char)(video_info->encoder_version);	

    buffer[4]	= ((unsigned char)((video_info->encoder_year - DESCRIPTOR_YEAR_BASE) & 0x7f) << 1) // 编码器时间 年 7bit
                | ((unsigned char)(video_info->encoder_month & 0x0f) >> 3); // 编码器时间 月 4bit
    buffer[5]	= ((unsigned char)(video_info->encoder_month & 0x07) << 5)
                | ((unsigned char)video_info->encoder_date & 0x1f);    // 编码器时间 日 5bit

    //width and height
    buffer[6]	= (unsigned char)(video_info->width_orig >> 8);
    buffer[7]	= (unsigned char)(video_info->width_orig);
    buffer[8]	= (unsigned char)(video_info->height_orig >> 8);
    buffer[9]	= (unsigned char)(video_info->height_orig );

    //encoder_flag
    buffer[10]	= ((unsigned char)(video_info->interlace & 0x01) << 7)
                | ((unsigned char)(video_info->b_frame_num & 0x03) << 5)
                | ((unsigned char)(video_info->is_svc_stream & 0x01) << 4)
                | ((unsigned char)(video_info->use_e_frame & 0x01) << 3)
                | ((unsigned char)video_info->max_ref_num & 0x07);

    buffer[11]	= ((unsigned char)(video_info->watermark_type & 0x07) << 5)
                | ((unsigned char)(video_info->deinterlace & 0x01) << 4)
	            | ((unsigned char)(video_info->min_search_blk & 0x03) << 2)
	            | ((unsigned char)(video_info->light_storage & 0x03));

    buffer[12]	= RESERVED_BYTE;
 
    //time_info
    buffer[13]	= (unsigned char)(video_info->time_info >> 15);
    buffer[14]	= (unsigned char)(video_info->time_info >> 7);  
    buffer[15]	= ((unsigned char)(video_info->time_info << 1) & 0xfe)
                | ((unsigned char)(video_info->fixed_frame_rate & 0x01));  
    return HIK_VIDEO_DESCRIPTOR_LEN;
}

/******************************************************************************
* 功  能：根据 HIK_AUDIO_INFO *audio_info 内的信息在 buffer 里生成 hik_audio_descriptor 数据
* 参  数：  buffer          - hik_audio_descriptor 数据缓冲区
*           audio_info     - 输入参数结构体
* 返回值：hik_audio_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_audio_descriptor(unsigned char *buffer, HIK_AUDIO_INFO *audio_info)
{
    buffer[0]	= HIK_AUDIO_DESCRIPTOR_TAG;
    buffer[1]	= HIK_AUDIO_DESCRIPTOR_LEN - 2; 

    buffer[2]	= (unsigned char)(audio_info->frame_len >> 8);
    buffer[3]	= (unsigned char)(audio_info->frame_len);	
    buffer[4]	= (0x7f << 1)
                | ((unsigned char)(audio_info->audio_num  & 0x01));

    buffer[5]	= (unsigned char)(audio_info->sample_rate >> 14);
    buffer[6]   = (unsigned char)(audio_info->sample_rate >> 6);
    buffer[7]   = (unsigned char)(((audio_info->sample_rate << 2) & 0xfc) | 0x03);
    buffer[8]	= (unsigned char)(audio_info->bit_rate >> 14);
    buffer[9]	= (unsigned char)(audio_info->bit_rate >> 6);
    buffer[10]	= (unsigned char)(((audio_info->bit_rate << 2) & 0xfc) | 0x03);
    buffer[11]  = RESERVED_BYTE;
    return HIK_AUDIO_DESCRIPTOR_LEN;
}

/******************************************************************************
* 功  能：根据 HIK_ES_INFO *stream_info 内的信息在 buffer 里生成 hik_device_descriptor 数据
* 参  数：  buffer          - hik_device_descriptor 数据缓冲区
*           dev_chan_id     - 设备和通道id
* 返回值：hik_device_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_device_descriptor(unsigned char *buffer, unsigned char dev_chan_id[16])
{
    buffer[0]	= HIK_DEVICE_DESCRIPTOR_TAG;
    buffer[1]	= HIK_DEVICE_DESCRIPTOR_LEN - 2; 

    buffer[2]	= 0x48; //海康设备标志 "HK"
    buffer[3]	= 0x4b;	

    memcpy(&buffer[4], dev_chan_id, 16);
    return HIK_DEVICE_DESCRIPTOR_LEN;
}

/******************************************************************************
* 功  能：根据 HIK_VIDEO_INFO *video_info 内的信息在 buffer 里生成 hik_video_clip_descriptor 数据
* 参  数：  buffer          - hik_video_clip_descriptor 数据缓冲区
*           video_info      - 输入参数结构体
* 返回值：hik_video_clip_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_video_clip_descriptor(unsigned char *buffer, HIK_VIDEO_INFO *video_info)
{
    buffer[0]	= HIK_VIDEO_CLIP_DESCRIPTOR_TAG;
    buffer[1]	= HIK_VIDEO_CLIP_DESCRIPTOR_LEN - 2; 

    buffer[2]	= (unsigned char)(video_info->start_pos_x >> 8);
    buffer[3]	= (unsigned char)video_info->start_pos_x;	
    buffer[4]   = (unsigned char)(video_info->start_pos_y >> 8) | 0x80;
    buffer[5]   = (unsigned char)video_info->start_pos_y;

    buffer[6]	= (unsigned char)(video_info->width_play >> 8);
    buffer[7]	= (unsigned char)video_info->width_play;	
    buffer[8]	= (unsigned char)(video_info->height_play >> 8);
    buffer[9]	= (unsigned char)video_info->height_play;
    
    buffer[10]  = RESERVED_BYTE;
    buffer[11]  = RESERVED_BYTE;

    return HIK_VIDEO_CLIP_DESCRIPTOR_LEN;
}

/******************************************************************************
* 功  能：在 buffer 里生成 timing_hrd_descriptor 数据
* 参  数：  buffer          - timing_hrd_descriptor 数据缓冲区
*           frame_rate      - 帧率
*           width           - 图像宽
*           height          - 图像高
* 返回值：hik_video_clip_descriptor 的长度
******************************************************************************/
unsigned int HKDSC_fill_timing_hrd_descriptor(unsigned char *buffer, int frame_rate, int width, int height)
{
	unsigned int num_units_in_ticks;

	if (frame_rate == 0) //防止除零
	{
		frame_rate = 25;
	}

	num_units_in_ticks = 90000 / (frame_rate * 2);

	buffer[0] = HIK_TIMING_HRD_DESCRIPTOR_TAG;
	buffer[1] = HIK_TIMING_HRD_DESCRIPTOR_LEN - 2;
	buffer[2] = 0x7f;
	buffer[3] = 0xff;
	buffer[4] = (unsigned char)(num_units_in_ticks >> 24);
	buffer[5] = (unsigned char)(num_units_in_ticks >> 16);
	buffer[6] = (unsigned char)(num_units_in_ticks >> 8);
	buffer[7] = (unsigned char)num_units_in_ticks;
	buffer[8] = 0x1f;
	buffer[9] = 0xfe;
	buffer[10] = width >> 3;
	buffer[11] = height >> 3;

	return HIK_TIMING_HRD_DESCRIPTOR_LEN;
}


/******************************************************************************
* 功  能：在 buffer 里生成 timing_hrd_descriptor 数据
* 参  数：  buffer          - timing_hrd_descriptor 数据缓冲区
*           frame_rate      - 帧率
*           width           - 图像宽
*           height          - 图像高
* 返回值：无
******************************************************************************/
void HKDSC_fill_timing_fps_descriptor(unsigned char *buffer, unsigned int frame_rate)
{
	unsigned int num_units_in_ticks;

	if (frame_rate == 0) //防止除零
	{
		frame_rate = 25;
	}

	num_units_in_ticks = 90000 / (frame_rate * 2);

	buffer[4] = (unsigned char)(num_units_in_ticks >> 24);
	buffer[5] = (unsigned char)(num_units_in_ticks >> 16);
	buffer[6] = (unsigned char)(num_units_in_ticks >> 8);
	buffer[7] = (unsigned char)num_units_in_ticks;

}

/******************************************************************************
* 功  能：在 buffer 里生成 encrypt_descriptor 数据
* 参  数：  buffer          - encrypt_descriptor 数据缓冲区
*           encrypt_round          - 加密轮数
*           encrypt_type           - 加密类型
*           encrypt_arith          - 加密算法
*           key_len                - 密钥长度
* 返回值：encrypt_descriptor 加密描述子的长度
******************************************************************************/
unsigned int HKDSC_fill_encrypt_descriptor(unsigned char *buffer,
                                           unsigned char encrypt_round,
                                           unsigned char encrypt_type,
                                           unsigned char encrypt_arith,
                                           unsigned char key_len)
{
    unsigned int pos        = 0;
    unsigned int length_int = 0;


    // private type
    buffer[pos++] = (unsigned char)(HIK_PRIVT_ENCRYPT_INFO >> 8);
    buffer[pos++] = (unsigned char)(HIK_PRIVT_ENCRYPT_INFO);

    // 以4字节为单位，表示私有数据长度
    length_int = HIK_ENCRYPT_INFO_LEN >> 2;
    buffer[pos++] = (length_int >> 8);
    buffer[pos++] = (length_int & 0xff);

    // tag
    buffer[pos++] = HIK_STREAM_ENCRYPT_TAG;

    // length
    buffer[pos++] = HIK_ENCRYPT_INFO_LEN - 2;

    // version
    buffer[pos++] = 0x00;
    buffer[pos++] = 0x01;

    // packed type、encrypt arithmetic
    buffer[pos++] = (0 << 4)| (encrypt_arith);

    // encrypt round、key length
    buffer[pos++] = (encrypt_round << 4)| (key_len);

    // encrypt type
    buffer[pos++] = encrypt_type;

    // reserved
    buffer[pos++] = 0x01;
    
    //返回长度或者直接返回（HIK_ENCRYPT_DESCRIPTOR_LEN）
    return pos;
}
